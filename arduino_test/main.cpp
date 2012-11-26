/*
 *  Simple Arduino Ping-Pong Test
 *  By CSK (csk@live.com)
 *
 */

#include "common/common.h"
#include "serialdrv/net_serial.h"


void showHelp(int argc , char * argv[] )
{
    printf("Usage:\n"
           "%s <comport>\n\n"
           "e.g. %s /dev/ttyUSB0\n", argv[0]);
}

raw_serial Serial;


int main(int argc , char * argv[] ) {
    if (argc < 2) {
        showHelp(argc, argv);
        return -1;
    }

    const char * serialport = argv[1];

    Serial.bind(serialport, 115200);
    if (!Serial.open()) {
        printf("Cannot open the serial port: %s\n", serialport);
        return -1;
    }
    
    char msg[1024];
    char recvBuf[1024];
    while(1) {
        printf("Message to Arduino:\n");
        fgets(msg, sizeof(msg), stdin);
        Serial.senddata((unsigned char *)msg,strlen(msg));
       
        // wait for data
        while (Serial.rxqueue_count()<strlen(msg)-1);

        Serial.recvdata((unsigned char *)recvBuf,strlen(msg)-1);
        recvBuf[strlen(msg)-1] = 0;


        printf("Got Message from Arduino:\n");
        printf("%s\n\n", recvBuf);
    
    }

    return 0;
}
