/*
 *  Simple VFD driver validator
 *  By CSK (csk@live.com)
 *
 */

#include "common/common.h"
#include "serialdrv/vfddisp_serial.h"


void showHelp(int argc , char * argv[] )
{
    printf("Usage:\n"
           "%s <comport>\n\n"
           "e.g. %s /dev/ttyUSB0\n", argv[0]);
}

int main(int argc , char * argv[] ) {
    if (argc < 2) {
        showHelp(argc, argv);
        return -1;
    }

    const char * comport_name = argv[1];

    SerialVFDDispatcher vfddispacher;
    
    vfddispacher.connect(comport_name);

    if (!vfddispacher.isConnected()) {
        printf("Cannot connect to the VFD deivce via %s.\n", 
            comport_name);
        return -2;
    }

    _u8 framebuffer[128*64];

    while(1) {
        
        for (int RectY=0; RectY<64; ++RectY) {
            memset(framebuffer, 0, sizeof(framebuffer));
            for (int fillPos =0 ; fillPos < 128*5; ++fillPos) {
                int opPos = RectY*128 + fillPos;
                if (opPos < sizeof(framebuffer)) {
                    framebuffer[opPos] = 1;
                }
            }
            vfddispacher.dispFrame8bit(framebuffer, sizeof(framebuffer));
            usleep((1000/20)*1000);
        }

    }
    vfddispacher.disconnect();
    return 0;
}
