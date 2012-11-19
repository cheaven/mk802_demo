/*
 *  Opencv functionality validator
 *  By CSK
 *
 */

#include "common/common.h"

#include <opencv2/opencv.hpp>


int main( int argc, char * argv[] ) {
    // grab one frame from the camera specified via arg[1]
    int camera_id = 0;

    if (argc > 1) camera_id = atoi(argv[1]);

    cv::VideoCapture cap(camera_id);

    if(!cap.isOpened())  // check if we succeeded
    {
        printf("failed to open the camera with id %d.\n", camera_id);
        return -1;
    }
    
    // capture one frame
    cv::Mat frame;
    cap >> frame; // get a new frame from camera
    
    // save to file
    imwrite("camera_captured.png", frame);

    return 0;
}