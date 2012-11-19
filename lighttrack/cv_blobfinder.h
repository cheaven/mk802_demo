/*
 *  RoboPeak Project
 *  Copyright 2009 - 2012
 *
 *  Machine Vision based rpmini localization
 *  Blob Finder
 *  By Shikai Chen
 */


#pragma once

struct Blob_t
{
    cv::Rect    _box;
    cv::Point2f _center;
    float       _area;

    Blob_t(const cv::Rect & box, const cv::Point2f & center, float area)
        :   _box(box), _center(center), _area(area) {}

    Blob_t()
        : _box(0,0,0,0), _center(0,0), _area(0)
    {}
};


class BlobFinder
{
public:
    enum {
        DEFAULT_BLOBMAXCNT  = 100,
        DEFAULT_BLOBMAXSIZE = 100000,
        DEFAULT_BLOBMINSIZE = 2,
        DEFAULT_BLOBMAX_WIDTH_HEIGHT_RATIO = 3,
    };

    BlobFinder();
    ~BlobFinder();

    void setBlobSizeBound(float minSize, float maxSize);
    void setMaxBlobCount(int count);
    void setMaxHWRatio(float ratio);
    size_t operator() (cv::Mat & input, std::vector<Blob_t>& blobs);

protected:
    float _minSize, _maxSize;
    float _hwratio;
    size_t _maxCount;
    cv::Mat _tmp;
    CvMemStorage*	_mem_storage;
};


//void FindBlobs(IplImage *src, std::vector<Blob_t>& blobs, int minArea, int maxArea);
