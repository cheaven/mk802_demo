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

#include "cv_blobfinder.h"

using namespace cv;
using namespace std;

void printUsage(int argc, char * argv[]) {
    printf("%s - Track a light source and display its location on a VFD\n"
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

        usleep(1000/60*1000);
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


static void onAdjustPTZ(Point &center, int r) {
    
#if 0
    static const _u8 magicleading[] = {0xa5, 0xa5};
    //send magic leading
    ptz_serialchn.senddata(magicleading, sizeof(magicleading));
    ptz_serialchn.waitforsent();
    _s8 ctrlSignal[2];

    center.x*=1.5;
    center.y*=1.5;
    if (center.x>100) center.x=100;
    if (center.x<-100) center.x=-100;

    if (center.y>100) center.y=100;
    if (center.y<-100) center.y=-100;

    ctrlSignal[0] = -center.x;
    ctrlSignal[1] = center.y;

    ptz_serialchn.senddata((_u8 *)ctrlSignal, sizeof(ctrlSignal));
    ptz_serialchn.waitforsent();
#else

    if (center.x>100) center.x=100;
    if (center.x<-100) center.x=-100;

    if (center.y>100) center.y=100;
    if (center.y<-100) center.y=-100;

    float scalex0 = 14.0f ;//* (2.0f / r);
    float scaley0 = 9.0f ;//* (2.0f / r);

    float scalex = 1.2f;
    float scaley = 1.8f;
    tracker_lock.lock();
    tracker_move_vec[0] = (center.x/scalex0) * (abs(center.x)/scalex0)/scalex;
    tracker_move_vec[1] = (center.y/scaley0) * (abs(center.y)/scaley0)/scaley;
    tracker_lock.unlock();
    
#endif
}

int main( int argc, char * argv[] ) {
    // grab one frame from the camera specified via arg[1]
    int camera_id = 0;
    const char * serialport ;
    const char * serialport_ptz;

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

#if 1
    // start the tracking thread
    pthread_t thrd_tracker;
    working_flag = true;
    pthread_create(&thrd_tracker, NULL, tracker_proc, NULL);
#endif
    cv::Mat srcframe;
    cv::Mat destFrameRGB, destFrameGray_tmp;
    cv::Mat dispFrame;
    
    cap >> srcframe; // get a new frame from camera

    int srcWidth = srcframe.cols;
    
    const float scale = srcWidth/128.0f;

    const int targetHeight = srcframe.rows / scale;
    const int targetWidth  = 128;
    
    destFrameRGB.create(targetHeight,targetWidth,srcframe.type());
    destFrameGray_tmp.create(targetHeight,targetWidth,CV_8U);
    dispFrame.create(targetHeight,targetWidth, CV_8U);


    char msgbuf[100];
    vector<Blob_t> blobs;
    Point last_moving_vector;
    BlobFinder blobfinder;
    blobfinder.setMaxBlobCount(1);
    blobfinder.setMaxHWRatio(20);
    blobfinder.setBlobSizeBound(1,100000);
    
    while (1) {

        cap >> srcframe; // get a new frame from camera

        _u64 startus = getus();

        // resize the frame
        resize(srcframe, destFrameRGB, cvSize(targetWidth, targetHeight), INTER_LINEAR );

        // convert to gray scale
        cvtColor(destFrameRGB, destFrameGray_tmp, CV_RGB2GRAY);

        // threshold
        cv::threshold(destFrameGray_tmp,destFrameGray_tmp,200,255,CV_THRESH_BINARY);
   
        // find blob
        blobfinder(destFrameGray_tmp, blobs);
        _u64 timeused_us = getus() - startus;

        rectangle(dispFrame, cvRect(0, 0, dispFrame.cols, dispFrame.rows), cvScalar(0,0,0,0),CV_FILLED );

            
        for (int i =0 ; i<blobs.size(); ++i ) {
            Point center;
            int   radius;

            center = blobs[i]._center;
            radius = (blobs[i]._box.width + blobs[i]._box.height) * 0.25;

            circle (destFrameGray_tmp, center, radius, cvScalar(255,255,255,255));
            cv::line(destFrameGray_tmp, cvPoint(center.x-radius-2, center.y), cvPoint(center.x+radius+2, center.y), cvScalar(255,255,255,255));
            cv::line(destFrameGray_tmp, cvPoint(center.x, center.y-radius-2), cvPoint(center.x, center.y+radius+2), cvScalar(255,255,255,255));

            center.x-=128/2;
            center.y-=destFrameGray_tmp.rows/2;
            onAdjustPTZ(center, radius);
            last_moving_vector = center;
        } 

        if (blobs.size()==0 ) {
            last_moving_vector.x /= 1.02;
            last_moving_vector.y /= 1.02;
            
            if (fabs(last_moving_vector.x) < 0.001) last_moving_vector.x = 0;
            if (fabs(last_moving_vector.y) < 0.001) last_moving_vector.y = 0;

            onAdjustPTZ(last_moving_vector, 1);

            cv::putText(destFrameGray_tmp,"Where is it?", cvPoint(1, dispFrame.rows/2),FONT_HERSHEY_SIMPLEX, 0.5, cvScalar(255,255,255,255));
        }

        fullscaleImgToVFD(destFrameGray_tmp);
    
        //usleep(1*1000); // frame limit
    }
    
#if 1
    working_flag = false;
    pthread_join(thrd_tracker, NULL);
#endif
    return 0;
}

