/*
 *  Display the foreground mask on a VFD
 *  By CSK (csk@live.com)
 *  www.csksoft.net
 */


#include "common/common.h"
#include "serialdrv/vfddisp_serial.h"
#include <opencv2/opencv.hpp>
#include "cv_foregroundextractor.h"

using namespace cv;


void printUsage(int argc, char * argv[]) {
    printf("%s - Display the foreground image on a VFD\n"
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

_u8 vfdfb[128*64];

SerialVFDDispatcher vfddispacher;


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

ForegroundDetector fg_detector;


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
    
    
    vfddispacher.connect(serialport);

    if (!vfddispacher.isConnected()) {
        printf("Cannot connect to the VFD deivce via %s.\n", 
            serialport);
        return -2;
    }

    cv::Mat srcframe;
    cv::Mat destFrameRGB, destFrameGray_tmp;

    
    cap >> srcframe; // get a new frame from camera
    // convert the frame size to 128xN
    int srcWidth = srcframe.cols;

    float scale = srcWidth/128.0f;

    const int targetHeight = srcframe.rows / scale;
    const int targetWidth  = 128;
    
    destFrameRGB.create(targetHeight,targetWidth,srcframe.type());
    destFrameGray_tmp.create(targetHeight,targetWidth,CV_8U);

    char msgbuf[100];
    // phase1 background learing
    {
        cv::Mat  msgFrame(64, 128, CV_8U);

        fg_detector.toggleBackgroundLearning(true);

        _u64 startus = getus();
        _u64 remaning_us;
        while( (remaning_us = getus() - startus) < 2000*1000) {
            cap >> srcframe; // get a new frame from camera
            // resize the frame
            resize(srcframe, destFrameRGB, cvSize(targetWidth, targetHeight));
            
            // convert to gray scale
       //     cvtColor(destFrameRGB, destFrameGray_tmp, CV_RGB2GRAY);

            // feed to the detector
            fg_detector(destFrameRGB);

            // disp msg
            cv::rectangle(msgFrame, cvRect(0, 0, 128, 64), cvScalar(0,0,0,0),CV_FILLED );
            sprintf(msgbuf,"BKGND Learning, %d..." , remaning_us/1000);
            putText(msgFrame, msgbuf, cvPoint(1, 32), FONT_HERSHEY_SIMPLEX, 0.3, cvScalar(255,255,255,255));
            
            fullscaleImgToVFD(msgFrame);
        }
    }

    fg_detector.toggleBackgroundLearning(false);

    // phase2 foregnd dectection
    while (1) {
        
        cap >> srcframe; // get a new frame from camera

        // resize the frame
        resize(srcframe, destFrameRGB, cvSize(targetWidth, targetHeight));

        // convert to gray scale
      //  cvtColor(destFrameRGB, destFrameGray_tmp, CV_RGB2GRAY);

        Mat * msk = fg_detector(destFrameRGB);

       
        fullscaleImgToVFD(*msk);
    
        usleep(1*1000); // frame limit
    }
    
    return 0;
}

