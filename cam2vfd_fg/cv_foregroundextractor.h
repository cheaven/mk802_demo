/*
 *  RoboPeak Project
 *  Copyright 2009 - 2012
 *
 *  MOG method based foreground detector
 *  By Shikai Chen
 */

#pragma once

class ForegroundDetector {
public:
    ForegroundDetector();
    ~ForegroundDetector();

    void toggleBackgroundLearning(bool enable = true);
    cv::Mat * operator() (const cv::Mat & input);
protected:
    cv::Mat _output;
    bool _isInited;
    bool  _isBkLearningEnabled;
#if 0
    cv::BackgroundSubtractorMOG2 _bgsubtractor;
#else
    cv::BackgroundSubtractorMOG  _bgsubtractor;
#endif
};
