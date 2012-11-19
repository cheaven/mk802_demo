/*
 *  Serial Port Based 128x64 VFD Driver
 *  By CSK (csk@live.com)
 *
 *  http://www.csksoft.net
 */

#include "common/common.h"
#include "serialdrv/vfddisp_serial.h"

#include "serialdrv/net_serial.h"

#define GET_SERIALOBJ() reinterpret_cast<raw_serial *>(this->_serial_obj)

SerialVFDDispatcher::SerialVFDDispatcher()
{
    this->_serial_obj = new raw_serial();
}

SerialVFDDispatcher::~SerialVFDDispatcher()
{
    disconnect();
    delete GET_SERIALOBJ();
}

bool SerialVFDDispatcher::connect(const char * portname, int baudrate)
{
    GET_SERIALOBJ()->bind(portname, baudrate);
    if (GET_SERIALOBJ()->open()) {
        this->_startDispatcher();
        return true;
    }
    return false;
}   

bool SerialVFDDispatcher::isConnected()
{
    return GET_SERIALOBJ()->isOpened();
       
}

void SerialVFDDispatcher::disconnect()
{
    this->_stopDispatcher();

    GET_SERIALOBJ()->close();
}

bool SerialVFDDispatcher::_send_frame_data(const _u8 * fb, size_t size)
{
    // send 4 leading magic byte: 0xA5
    static const _u8 magicleading[] = { 0xA5, 0xA5, 0xA5, 0xA5};
    
    GET_SERIALOBJ()->senddata(magicleading, sizeof(magicleading));
    GET_SERIALOBJ()->waitforsent();
    
    // send the actual payload
    GET_SERIALOBJ()->senddata(fb, size);
    GET_SERIALOBJ()->waitforsent();

    return true;
}

