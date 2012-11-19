/*
 *  128x64 VFD Driver Common Header
 *  By CSK (csk@live.com)
 *
 */

#pragma once


class AbsVFDDispatcher
{
public:
    AbsVFDDispatcher();
    virtual ~AbsVFDDispatcher();

    virtual bool isConnected() = 0;

    virtual bool dispFrame8bit(const _u8 * framebuffer, size_t size);
    virtual bool dispBlankFrame();

protected:
    
    virtual bool _send_frame_data(const _u8 * fb, size_t size) = 0;
    
    
    void   _startDispatcher();
    void   _stopDispatcher();
    static void * s_dispatcher_proc(void * );
    
    void    _dispatcher();

    bool   _dispatcher_working_flag;

    void * _fb_pool;

    Locker _op_locker;
    Event  _op_signal;

    pthread_t _thrd_dispatcher;
};
