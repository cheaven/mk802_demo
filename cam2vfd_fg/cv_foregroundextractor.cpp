/*
 *  RoboPeak Project
 *  Copyright 2009 - 2012
 *
 *  MOG method based foreground detector
 *  By Shikai Chen
 */


#include "common/common.h"
#include <opencv2/opencv.hpp>
#include "cv_foregroundextractor.h"

using namespace cv;


static const float BACKGROUND_LEARNING_RATIO_STATIC = 0.0001f;

ForegroundDetector::ForegroundDetector()
: _isInited(false)
, _isBkLearningEnabled(true)
{
    // magic number from the OpenCV example
#if 1
    _bgsubtractor.noiseSigma = 20;
#else
    
    _bgsubtractor.nShadowDetection = 0;
#endif
    
}

ForegroundDetector::~ForegroundDetector()
{

}

void ForegroundDetector::toggleBackgroundLearning(bool enable)
{
    _isBkLearningEnabled = enable;
}

Mat * ForegroundDetector::operator() (const Mat & input)
{
    _bgsubtractor(input, _output, (_isInited && _isBkLearningEnabled)?-1:BACKGROUND_LEARNING_RATIO_STATIC);

    if (!_isInited) {
        _isInited = true;
    }

    return &_output;
}
