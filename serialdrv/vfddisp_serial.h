/*
 *  Serial Port Based 128x64 VFD Driver
 *  By CSK (csk@live.com)
 *
 *  http://www.csksoft.net
 */

#pragma once

#include "common/vfddisp.h"


class SerialVFDDispatcher : public AbsVFDDispatcher
{
public:
    enum {
        DEFAULT_BAUDRATE = 230400,

    };
    SerialVFDDispatcher();
    virtual ~SerialVFDDispatcher();

    bool connect(const char * portname, int baudrate = DEFAULT_BAUDRATE);
    void disconnect();

    virtual bool isConnected();
   
protected:
    
    virtual bool _send_frame_data(const _u8 * fb, size_t size);

    void * _serial_obj;
};