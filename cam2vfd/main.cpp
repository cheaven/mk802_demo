/*
 *  Display the camera image on a VFD
 *  By CSK (csk@live.com)
 *  www.csksoft.net
 */


#include "common/common.h"
#include "serialdrv/vfddisp_serial.h"
#include <opencv2/opencv.hpp>

using namespace cv;


//#define DISPLAY_FPS_LIMIT 40 //20fps max for serial based communication channel


void printUsage(int argc, char * argv[]) {
    printf("%s - Display the camera image on a VFD\n"
           "Usage: %s <serial port name> [camera id].\n"
           , argv[0], argv[0]);
}

_u64 getus()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec*1000000LL + t.tv_nsec/1000;
}


int main( int argc, char * argv[] ) {
    // grab one frame from the camera specified via arg[1]
    int camera_id = 0;
    const char * serialport ;

    if (argc < 2) {
        printUsage(argc, argv);
        return -1;
    }
    
    serialport = argv[1];
    if (argc > 2) camera_id = atoi(argv[2]);

    cv::VideoCapture cap(camera_id);

    if(!cap.isOpened())  // check if we succeeded
    {
        printf("failed to open the camera with id %d.\n", camera_id);
        return -1;
    }
    
    SerialVFDDispatcher vfddispacher;
    
    vfddispacher.connect(serialport);

    if (!vfddispacher.isConnected()) {
        printf("Cannot connect to the VFD deivce via %s.\n", 
            serialport);
        return -2;
    }

    cv::Mat srcframe;
    cv::Mat destFrameRGB, destFrameGray_tmp;

    _u8 vfdfb[128*64];
    cap >> srcframe; // get a new frame from camera
    // convert the frame size to 128xN
    int srcWidth = srcframe.cols;

    float scale = srcWidth/128.0f;

    const int targetHeight = srcframe.rows / scale;
    const int targetWidth  = 128;
    
    destFrameRGB.create(targetHeight,targetWidth,srcframe.type());
    destFrameGray_tmp.create(targetHeight,targetWidth,CV_8U);
    while (1) {
        
        cap >> srcframe; // get a new frame from camera

        _u64 startus = getus();
        // resize the frame
        resize(srcframe, destFrameRGB, cvSize(targetWidth, targetHeight));

        // convert to gray scale
        cvtColor(destFrameRGB, destFrameGray_tmp, CV_RGB2GRAY);

        // threshold
        cv::threshold(destFrameGray_tmp,destFrameGray_tmp,128,255,CV_THRESH_BINARY);

        _u64 used_time_us = getus() - startus;

        char msgbuf[100];
        sprintf(msgbuf, "us: %d", used_time_us);
        //putText(destFrameGray_tmp, msgbuf, cvPoint(0, targetHeight/2), FONT_HERSHEY_SIMPLEX, 0.3, cvScalar(255,255,255,255));

        // clip the frame to 128x64

        Mat_<uchar>& rawgray = (Mat_<uchar>&)destFrameGray_tmp;
        int pos = 0;
        for (int row= (targetHeight-64)/2; row <  (targetHeight-64)/2+64; ++row) {
            for (int col = 0; col < 128; ++col) {
                _u8 current = rawgray(row, col);
                vfdfb[pos++] = current;
            }
        }

        // send to the vfd
        vfddispacher.dispFrame8bit(vfdfb, sizeof(vfdfb));
    
        usleep(1*1000); // frame limit
    }
    
    return 0;
}

