/*
 *  Common Logic for VFD Driver
 *  By CSK (csk@live.com)
 *  www.csksoft.net
 */

#include "common/common.h"
#include "common/vfddisp.h"

#include "utils/locker.h"
#include "utils/event.h"


#include <list>

using namespace std;

#define VFD_FRAMEBUFFER_SIZE (128*64/8)
struct fb_blob_t {
    _u8 fb[VFD_FRAMEBUFFER_SIZE];
};


typedef list<fb_blob_t *> fb_list_t;
typedef list<fb_blob_t *>::iterator fb_list_itr_t;


#define GET_FBLIST() reinterpret_cast<fb_list_t *>(this->_fb_pool)


static void convert8BitToVFDFB_12864(const _u8 * rawBit, _u8 * destBit, size_t pixelCount)
{
    assert(pixelCount >= 128*64);

    for (int fbpos = 0; fbpos < VFD_FRAMEBUFFER_SIZE; ++fbpos) {
        _u8 current8bit = 0;
#if 1  
        int colPos = fbpos & (128 - 1);
        int rowPos = (fbpos >> 7) << 3;
#else
        int colPos = fbpos % 128;
        int rowPos = (fbpos / 128) * 8;
#endif          
        current8bit |= rawBit[colPos + ((rowPos + 0)<<7) ]?(0x1<<0):0;
        current8bit |= rawBit[colPos + ((rowPos + 1)<<7) ]?(0x1<<1):0;
        current8bit |= rawBit[colPos + ((rowPos + 2)<<7) ]?(0x1<<2):0;
        current8bit |= rawBit[colPos + ((rowPos + 3)<<7) ]?(0x1<<3):0;
        current8bit |= rawBit[colPos + ((rowPos + 4)<<7) ]?(0x1<<4):0;
        current8bit |= rawBit[colPos + ((rowPos + 5)<<7) ]?(0x1<<5):0;
        current8bit |= rawBit[colPos + ((rowPos + 6)<<7) ]?(0x1<<6):0;
        current8bit |= rawBit[colPos + ((rowPos + 7)<<7) ]?(0x1<<7):0;
        
        destBit[fbpos] = current8bit;
    }
}

AbsVFDDispatcher::AbsVFDDispatcher()
    : _dispatcher_working_flag(false)
{
    _fb_pool = new fb_list_t();
}

AbsVFDDispatcher::~AbsVFDDispatcher()
{
    _stopDispatcher();
    
    for (fb_list_itr_t itr=GET_FBLIST()->begin(); itr!=GET_FBLIST()->end(); ++itr)
    {
        delete *itr;
    }

    delete GET_FBLIST();
}


bool AbsVFDDispatcher::dispFrame8bit(const _u8 * framebuffer, size_t size)
{
    if (!isConnected()) return false;
    if (size<VFD_FRAMEBUFFER_SIZE*8) return false;

    fb_blob_t * blob = new fb_blob_t ();
    convert8BitToVFDFB_12864(framebuffer, blob->fb, size);

    _op_locker.lock();
        
    GET_FBLIST()->push_back(blob);

    _op_signal.set();

    _op_locker.unlock();

    return true;
}

bool AbsVFDDispatcher::dispBlankFrame()
{
    if (!isConnected()) return false;

    fb_blob_t * blob = new fb_blob_t ();
    memset(blob->fb, 0, sizeof(blob->fb));

    _op_locker.lock();
        
    GET_FBLIST()->push_back(blob);

    _op_signal.set();

    _op_locker.unlock();

    return true;

}

void   AbsVFDDispatcher::_startDispatcher()
{
    if (_dispatcher_working_flag) return ;
    _dispatcher_working_flag = true;
    pthread_create( &_thrd_dispatcher , NULL, &AbsVFDDispatcher::s_dispatcher_proc, this);
    
    
}

void   AbsVFDDispatcher::_stopDispatcher()
{
    _dispatcher_working_flag = false;
    _op_signal.set();

    pthread_join(_thrd_dispatcher, NULL);
}

void * AbsVFDDispatcher::s_dispatcher_proc(void * This)
{
    reinterpret_cast<AbsVFDDispatcher *>(This)->_dispatcher();
    return NULL;
}

void   AbsVFDDispatcher::_dispatcher()
{
    while(_dispatcher_working_flag) {
        _op_locker.lock();
        if ( GET_FBLIST()->empty() ) {
            _op_locker.unlock();
            _op_signal.wait();
            continue;
        }

        fb_blob_t * blob = GET_FBLIST()->front();
        GET_FBLIST()->pop_front ();
        _op_locker.unlock();

        _send_frame_data(blob->fb, sizeof(blob->fb));

        delete blob;
    }
}

