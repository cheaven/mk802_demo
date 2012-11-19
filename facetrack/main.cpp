/*
 *  Display the foreground mask on a VFD
 *  By CSK (csk@live.com)
 *  www.csksoft.net
 */


#include "common/common.h"
#include "serialdrv/vfddisp_serial.h"
#include "serialdrv/net_serial.h"

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

void printUsage(int argc, char * argv[]) {
    printf("%s - Display a face location on a VFD and track it\n"
           "Usage: %s <vfdserialport> <ptzserialport> [camera id].\n"
           , argv[0], argv[0]);
}

_u64 getus()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec*1000000LL + t.tv_nsec/1000;
}

_u8 vfdfb[128*64];

SerialVFDDispatcher vfddispacher;
raw_serial          ptz_serialchn;

bool                working_flag;
Locker              tracker_lock;

float               tracker_move_vec[2];

static void * tracker_proc(void * data) {
    
    while (working_flag) {
        float local_move_vec[2];
    
        tracker_lock.lock();
        memcpy(local_move_vec, tracker_move_vec, sizeof(tracker_move_vec));
        tracker_lock.unlock();
        
        //send magic leading
        static const _u8 magicleading[] = {0xa5, 0xa5};
        ptz_serialchn.senddata(magicleading, sizeof(magicleading));
        ptz_serialchn.waitforsent();
        _s8 ctrlSignal[2];

        if (fabs(local_move_vec[0])<1.0f && local_move_vec[0]!=0.0f) {
            local_move_vec[0] = fabs(local_move_vec[0])/local_move_vec[0];
        }    

        if (fabs(local_move_vec[1])<1.0f && local_move_vec[1]!=0.0f) {
            local_move_vec[1] = fabs(local_move_vec[1])/local_move_vec[1];
        }    


        ctrlSignal[0] = -local_move_vec[0];
        ctrlSignal[1] = local_move_vec[1];

        //printf("ctrl %d %d\n", ctrlSignal[0], ctrlSignal[1]);
        ptz_serialchn.senddata((_u8 *)ctrlSignal, sizeof(ctrlSignal));
        ptz_serialchn.waitforsent();

        usleep(1000/20*1000);
    }
    return NULL;
}

static void fullscaleImgToVFD(Mat & input) {
    // clip the frame to 128x64
    
    Mat_<uchar>& rawgray = (Mat_<uchar>&)input;
    int pos = 0;
    for (int row= (input.rows-64)/2; row <  (input.rows-64)/2+64; ++row) {
        for (int col = 0; col < 128; ++col) {
            _u8 current = rawgray(row, col);
            vfdfb[pos++] = current;
        }
    }

    // send to the vfd
    vfddispacher.dispFrame8bit(vfdfb, sizeof(vfdfb));

}

const char * cascadeName = "data/haarcascades/haarcascade_frontalface_alt.xml";
const char * nestedCascadeName = "data/haarcascades/haarcascade_eye_tree_eyeglasses.xml";

CascadeClassifier cascade, nestedCascade;
const double scale = 4.5;



static bool processingClassifier() {
    if (!nestedCascade.load(nestedCascadeName)) {
        printf("Cannot load the nestedCascade %s.\n", nestedCascadeName);
        return false;
    }

    if (!cascade.load(cascadeName)) {
        printf("Cannot load the cascadeName %s. \n", cascadeName);
        return false;
    }
    return true;
}


static void onAdjustPTZ(Point &center, int r) {
    
#if 0
    static const _u8 magicleading[] = {0xa5, 0xa5};
    //send magic leading
    ptz_serialchn.senddata(magicleading, sizeof(magicleading));
    ptz_serialchn.waitforsent();
    _s8 ctrlSignal[2];

    if (center.x>200) center.x=200;
    if (center.x<-200) center.x=-200;

    if (center.y>200) center.y=200;
    if (center.y<-200) center.y=-200;

    ctrlSignal[0] = -center.x*4/5;
    ctrlSignal[1] = center.y*4/5;

    ptz_serialchn.senddata((_u8 *)ctrlSignal, sizeof(ctrlSignal));
    ptz_serialchn.waitforsent();
#else
    if (center.x>200) center.x=200;
    if (center.x<-200) center.x=-200;

    if (center.y>200) center.y=200;
    if (center.y<-200) center.y=-200;

    float scale = 3.0f * (20.0f / r) * (20.0f / r);

    
    tracker_lock.lock();
    tracker_move_vec[0] = center.x/scale;
    tracker_move_vec[1] = center.y/scale;
    tracker_lock.unlock();
    
#endif
}

int main( int argc, char * argv[] ) {
    // grab one frame from the camera specified via arg[1]
    int camera_id = 0;
    const char * serialport ;
    const char * serialport_ptz;
    if (!processingClassifier()) return -2;

    if (argc < 3) {
        printUsage(argc, argv);
        return -1;
    }
    
    serialport = argv[1];
    serialport_ptz = argv[2];
    if (argc > 3) camera_id = atoi(argv[3]);

    cv::VideoCapture cap(camera_id);

    if(!cap.isOpened())  // check if we succeeded
    {
        printf("failed to open the camera with id %d.\n", camera_id);
        return -1;
    }
    
    
    vfddispacher.connect(serialport);

    if (!vfddispacher.isConnected()) {
        printf("Cannot connect to the VFD deivce via %s.\n", 
            serialport);
        return -2;
    }

    ptz_serialchn.bind(serialport_ptz, 57600);
    if (!ptz_serialchn.open()) {
        printf("Cannot connect to the PTZ deivce via %s.\n", 
            serialport_ptz);
        return -3;

    }


    // start the tracking thread
    pthread_t thrd_tracker;
    working_flag = true;
    pthread_create(&thrd_tracker, NULL, tracker_proc, NULL);

    cv::Mat srcframe;
    cv::Mat destFrameRGB, destFrameGray_tmp;
    cv::Mat dispFrame, dispFrame_plot;
    
    cap >> srcframe; // get a new frame from camera

    int srcWidth = srcframe.cols;


    const int targetHeight = srcframe.rows / scale;
    const int targetWidth  = srcframe.cols / scale;
    
    destFrameRGB.create(targetHeight,targetWidth,srcframe.type());
    destFrameGray_tmp.create(targetHeight,targetWidth,CV_8U);


    const float scale2 = targetWidth/128.0f;

    const int dispFrameHeight = targetHeight/scale2;
    dispFrame.create(dispFrameHeight,128, CV_8U);
    dispFrame_plot.create(dispFrameHeight,128, CV_8U);

    char msgbuf[100];
    vector<Rect> faces;
    Point last_moving_vector;
    
    while (1) {
        faces.clear();
        cap >> srcframe; // get a new frame from camera

        // resize the frame
        resize(srcframe, destFrameRGB, cvSize(targetWidth, targetHeight), INTER_LINEAR );

        // convert to gray scale
        cvtColor(destFrameRGB, destFrameGray_tmp, CV_RGB2GRAY);

        equalizeHist( destFrameGray_tmp, destFrameGray_tmp );
   
        _u64 startus = getus();

        cascade.detectMultiScale( destFrameGray_tmp, faces,
                1.2, 2, 0
                |CV_HAAR_FIND_BIGGEST_OBJECT
                |CV_HAAR_DO_ROUGH_SEARCH
                |CV_HAAR_DO_CANNY_PRUNING 
                //| CV_HAAR_DO_ROUGH_SEARCH
                ,
                Size(20, 20) );
                
        _u64 timeused_us = getus() - startus;

        rectangle(dispFrame_plot, cvRect(0, 0, dispFrame.cols, dispFrame.rows), cvScalar(0,0,0,0),CV_FILLED );
        resize(destFrameGray_tmp, dispFrame, cvSize(128,dispFrameHeight), INTER_LINEAR );
        cv::threshold(dispFrame,dispFrame,60,255,CV_THRESH_BINARY_INV);
        int i = 0;

            
        for (vector<Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++ ) {
            Point center;
            int   radius;
            center.x = (r->x + r->width*0.5) / scale2;
            center.y = (r->y + r->height*0.5) / scale2;
            
            radius = (r->width + r->height) * 0.25 / scale2;
            circle (dispFrame_plot, center, radius, cvScalar(255,255,255,255),1.5);
            cv::line(dispFrame_plot, cvPoint(center.x-radius-2, center.y), cvPoint(center.x+radius+2, center.y), cvScalar(255,255,255,255),1);
            cv::line(dispFrame_plot, cvPoint(center.x, center.y-radius-2), cvPoint(center.x, center.y+radius+2), cvScalar(255,255,255,255),1);

            center.x-=128/2;
            center.y-=dispFrame.rows/2;
            onAdjustPTZ(center, radius);
            last_moving_vector = center;
        } 

        if (faces.size()==0 ) {
            last_moving_vector.x /= 1.5;
            last_moving_vector.y /= 1.5;
            
            if (fabs(last_moving_vector.x) < 0.01) last_moving_vector.x = 0;
            if (fabs(last_moving_vector.y) < 0.01) last_moving_vector.y = 0;

            onAdjustPTZ(last_moving_vector, 1);
            if (!last_moving_vector.x && !last_moving_vector.y) {
                rectangle(dispFrame, cvRect(0, 0, dispFrame.cols, dispFrame.rows), cvScalar(0,0,0,0),CV_FILLED );
                cv::putText(dispFrame_plot,"Where are you?", cvPoint(1, dispFrame.rows/2),FONT_HERSHEY_SIMPLEX, 0.5, cvScalar(255,255,255,255));
            } else {
                cv::putText(dispFrame_plot,"I am finding..", cvPoint(1, dispFrame.rows/2),FONT_HERSHEY_SIMPLEX, 0.5, cvScalar(255,255,255,255));

            }
        }


        for (int row= (dispFrame_plot.rows-64)/2; row <  (dispFrame_plot.rows-64)/2+64; ++row) {
            for (int col = 0; col < 128; ++col) {
                _u8 current = dispFrame_plot.at<_u8>(row, col);
                if (current) {
                    dispFrame.at<_u8>(row, col) = !dispFrame.at<_u8>(row, col);
                }
            }
        }
        fullscaleImgToVFD(dispFrame);
    
        //usleep(1*1000); // frame limit
    }
    
    working_flag = false;
    pthread_join(thrd_tracker, NULL);
    return 0;
}

