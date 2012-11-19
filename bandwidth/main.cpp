/*
 *  Display the current network usage on a VFD
 *  By CSK (csk@live.com)
 *  www.csksoft.net
 */


#include "common/common.h"
#include "serialdrv/vfddisp_serial.h"
#include <opencv2/opencv.hpp>
#include <list>
#include <vector>

using namespace cv;
using namespace std;

#define REFRESH_FPS  2

void printUsage(int argc, char * argv[]) {
    printf("%s - Display the current network usage on a VFD\n"
           "Usage: %s <serial port name>.\n"
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

struct NetUsageDesc {
    float tx_Bps;
    float rx_Bps;
};


static size_t split(char * str, vector<char *> &items) {
    items.clear();

    char * currentLeading = str;
    
    while (*str) {
        if (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
            *str = 0;

            if (str != currentLeading) {
                items.push_back(currentLeading);
            }

            currentLeading = str + 1;
        }
        
        ++str;
    }

    if (str != currentLeading) {
        items.push_back(currentLeading);
    }

    return items.size();
}

class NetworkUsageUpdater {
public:
    NetworkUsageUpdater(): _lastRetrieveUs(0), _lastRXCount(0), _lastTXCount(0) {}
    ~NetworkUsageUpdater() {}


    void retrieveUsageData() {
        static const char * RETRIEVE_SCRIPT = "./scripts/retrieve_ddwrt_bandwidth.sh";

        char  linebuffer[1024];
        
        // retrive the data via the script
        FILE * pipe = popen(RETRIEVE_SCRIPT, "r");
        if (!pipe) {
            printf("Error, cannot execute the script %s.\n", RETRIEVE_SCRIPT);
            return;
        }

        // ignore the first line
        if (fgets(linebuffer, sizeof(linebuffer), pipe) <=0 ) {
            printf("Error, the script %s retrieves invalid data.\n", RETRIEVE_SCRIPT);
            pclose(pipe);
            return ;
        }    
        
        if (fgets(linebuffer, sizeof(linebuffer), pipe) <=0 ) {
            printf("Error, the script %s retrieves invalid data.\n", RETRIEVE_SCRIPT);
            pclose(pipe);
            return ;
        }

        pclose(pipe);

        vector<char *> values;
        split(linebuffer, values);

        if (values.size() < 9) {
            printf("Error, the script %s retrieves invalid data.\n", RETRIEVE_SCRIPT);
            return ;
        }

        _u32 currentRXCount = strtoul(values[1], NULL, 10);
        _u32 currentTXCount = strtoul(values[9], NULL, 10);
        _u32 currentUs = getus();

        if (_lastRetrieveUs) {
            // the first data 
            _u32 deltaRXcount = currentRXCount - _lastRXCount;
            _u32 deltaTXcount = currentTXCount - _lastTXCount;
            _u32 deltaUs = currentUs - _lastRetrieveUs;

            if (!deltaUs) deltaUs = 1; //prevent divided-by-zero error

            NetUsageDesc desc;
            desc.rx_Bps = (float)(deltaRXcount*1000000ULL)/deltaUs;
            desc.tx_Bps = (float)(deltaTXcount*1000000ULL)/deltaUs;
            
            printf("rx=%.2f tx=%.2f trx=%u ttx=%u\n", desc.rx_Bps, desc.tx_Bps, currentRXCount, currentTXCount);

            _usage_stat.push_back(desc);
            if (_usage_stat.size() > 128) {
                _usage_stat.pop_front();
            }
        }
        
        _lastRXCount = currentRXCount;
        _lastTXCount = currentTXCount;

        _lastRetrieveUs = currentUs;
    }


    void renderToVFD(Mat & frame) {
        // clear the previous frame data
        rectangle(frame, cvRect(0, 0, frame.cols, frame.rows), cvScalar(0,0,0,0),CV_FILLED );

        // draw the split line
        cv::line(frame, cvPoint(0, frame.rows/2), cvPoint(frame.cols, frame.rows/2), cvScalar(255,255,255,255));


        // phase one: identify the scale
        float maxRxRate = 0.0f , maxTxRate = 0.0f;
        float scaleRx, scaleTx;
        for (list<NetUsageDesc>::iterator itr = _usage_stat.begin();
            itr != _usage_stat.end(); ++itr) {

            if (maxRxRate<(*itr).rx_Bps) maxRxRate = (*itr).rx_Bps;
            if (maxTxRate<(*itr).tx_Bps) maxTxRate = (*itr).tx_Bps;
        }

        const int PlottingHeight = frame.rows/2 - 2;

        scaleRx = maxRxRate/PlottingHeight;
        scaleTx = maxTxRate/PlottingHeight;

        if (scaleRx == 0.0f) scaleRx=1.0f;
        if (scaleTx == 0.0f) scaleTx=1.0f;

        // phase two: plotting
        int plotPos = 0;
        int lastscaledRx = 0;
        int lastscaledTx = 0;
        for (list<NetUsageDesc>::iterator itr = _usage_stat.begin();
            itr != _usage_stat.end(); ++itr, ++plotPos) {

            int scaledRx = (*itr).rx_Bps/scaleRx;
            int scaledTx = (*itr).tx_Bps/scaleTx;

            if (plotPos) {
                cv::line(frame, cvPoint(plotPos-1, PlottingHeight-lastscaledRx), cvPoint(plotPos, PlottingHeight-scaledRx), cvScalar(255,255,255,255));
                cv::line(frame, cvPoint(plotPos-1, frame.rows-lastscaledTx-1), cvPoint(plotPos, frame.rows-scaledTx-1), cvScalar(255,255,255,255));
            }

            lastscaledRx = scaledRx;
            lastscaledTx = scaledTx;
        }

        if (!_usage_stat.size() ) {
            return;
        }

        char msg[100];
        formatRateStr(_usage_stat.back().rx_Bps, msg, "RX:");
        cv::putText(frame,msg, cvPoint(1, 10),FONT_HERSHEY_SIMPLEX, 0.24, cvScalar(255,255,255,255));

        formatRateStr(_usage_stat.back().tx_Bps, msg, "TX:");
        cv::putText(frame,msg, cvPoint(1, 10 + frame.rows/2),FONT_HERSHEY_SIMPLEX, 0.24, cvScalar(255,255,255,255));
    }

    void formatRateStr(float rate, char * buffer, const char * leading) {
        int unit = 0; //0-Bps 1-KBps 2-MBps
        
        if (rate > 100.0f) {
            unit = 1;
            if (rate > 1024*100.0f) {
                unit=2;
            }
        }

        switch (unit) {
            case 0:
                sprintf(buffer, "%s %.2f Byte/s", leading, rate);
                break;
            case 1:
                sprintf(buffer, "%s %.2f KB/s", leading, rate/1024.0f);
                break;
            case 2:
                sprintf(buffer, "%s %.2f MB/s", leading, rate/(1024.0f * 1024.0f));
                break;
           
        }
    }

protected:
    _u64               _lastRetrieveUs;
    list<NetUsageDesc> _usage_stat;
  

    _u32               _lastRXCount;
    _u32               _lastTXCount;  
};


int main( int argc, char * argv[] ) {
    const char * serialport ;

    if (argc < 1) {
        printUsage(argc, argv);
        return -1;
    }
    
    serialport = argv[1];

    
    
    
    vfddispacher.connect(serialport);

    if (!vfddispacher.isConnected()) {
        printf("Cannot connect to the VFD deivce via %s.\n", 
            serialport);
        return -2;
    }

    cv::Mat dispFrame(64, 128, CV_8U);
    NetworkUsageUpdater netupdater;
    while (1) {
        

        // retrieve the current usage statistic from a DDWRT-Router
        netupdater.retrieveUsageData();

        // render the usage statistic to the frame
        netupdater.renderToVFD(dispFrame);

        // display the frame
        fullscaleImgToVFD(dispFrame);

        usleep(1000/REFRESH_FPS*1000);
    }
    
    return 0;
}

