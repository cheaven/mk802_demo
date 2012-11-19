/* 
 *  Blob Finder 
 *  By Shikai Chen
 *  
 */


#include "common/common.h"
#include <cv.h>
#include <opencv2/opencv.hpp>
#include "cv_blobfinder.h"

using namespace cv;

#define CVCONTOUR_APPROX_LEVEL  1   // Approx.threshold - the bigger it is, the simpler is the boundary 


BlobFinder::BlobFinder()
: _maxCount(DEFAULT_BLOBMAXCNT)
{
    _minSize = DEFAULT_BLOBMINSIZE;
    _maxSize = DEFAULT_BLOBMAXSIZE;

    _mem_storage = cvCreateMemStorage(0);
}

BlobFinder::~BlobFinder()
{
    if (_mem_storage) cvReleaseMemStorage(&_mem_storage);
}

void BlobFinder::setBlobSizeBound(float minSize, float maxSize)
{
    _minSize = minSize;
    _maxSize = maxSize;
    _hwratio = DEFAULT_BLOBMAX_WIDTH_HEIGHT_RATIO;
}

void BlobFinder::setMaxBlobCount(int count)
{
    _maxCount = count;
}

void BlobFinder::setMaxHWRatio(float ratio)
{
    _hwratio = ratio;
}

size_t BlobFinder::operator() (cv::Mat & input, std::vector<Blob_t>& blobs)
{
    
    static CvMemStorage*	mem_storage	= NULL;
    static CvMoments myMoments;
    
    _tmp.create(input.size(), input.type());

    input.copyTo(_tmp);

    // finding...
    cvClearMemStorage(_mem_storage);
    blobs.clear();

    CvSeq* contour_list = 0;
    CvMat cvtMat = _tmp;
    cvFindContours(&cvtMat,_mem_storage,&contour_list, sizeof(CvContour),
		CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE);

	for (CvSeq* d = contour_list; d != NULL; d=d->h_next)
	{
		bool isHole = false;
		CvSeq* c = d;
		while (c != NULL)
		{
			double area = fabs(cvContourArea( c ));
			if( area >= _minSize && area <= _maxSize)
			{
				CvSeq* approx;

				//Polygonal approximation of the segmentation
			    approx = cvApproxPoly(c,sizeof(CvContour),_mem_storage,CV_POLY_APPROX_DP, CVCONTOUR_APPROX_LEVEL,0);

				float area = (float)cvContourArea( approx, CV_WHOLE_SEQ );
				
                Rect box = cvBoundingRect(approx);
                
                if ((float)box.width/box.height > _hwratio
                    || (float)box.height/box.width > _hwratio) goto next_blob;

				cvMoments( approx, &myMoments );

                Blob_t obj( box, cvPoint(0,0), fabs(area));

				if (myMoments.m10 > -DBL_EPSILON && myMoments.m10 < DBL_EPSILON)
				{
					obj._center.x = obj._box.x + obj._box.width/2;
					obj._center.y = obj._box.y + obj._box.height/2;
				}
				else
				{
					obj._center.x = myMoments.m10 / myMoments.m00;
					obj._center.y = myMoments.m01 / myMoments.m00;
				}
                blobs.push_back(obj);
                if (blobs.size() >= _maxCount ) {
                    return blobs.size();
                }
			}//END if( area >= minArea)
next_blob:
			if (isHole)
				c = c->h_next;//one_hole->h_next is another_hole
			else
				c = c->v_next;//one_contour->h_next is one_hole
			isHole = true;
		}//END while (c != NULL)
	} 
    return blobs.size();
}

