/*
 *      Serial Port Driver for POSIX based system
 *      By CSK (csk@live.com)
 *
 */


#pragma once


class raw_serial
{
public:
    enum{
        ANS_OK      = 0,
        ANS_TIMEOUT = -1,
        ANS_DEV_ERR = -2,
    };

    enum{
        SERIAL_RX_BUFFER_SIZE = 512,
        SERIAL_TX_BUFFER_SIZE = 128,
    };

    raw_serial();
    virtual ~raw_serial();
    virtual bool bind(const char * portname, uint32_t baudrate, uint32_t flags = NULL);
    virtual bool open();
    virtual void close();
    virtual void flush( _u32 flags);
    
    virtual _word_size_t waitfordata(_word_size_t data_count,_u32 timeout = -1, _word_size_t * returned_size = NULL);

    virtual _word_size_t senddata(const unsigned char * data, _word_size_t size);
    virtual _word_size_t recvdata(unsigned char * data, _word_size_t size);

    virtual _word_size_t waitforsent(_u32 timeout = -1, _word_size_t * returned_size = NULL);
    virtual _word_size_t waitforrecv(_u32 timeout = -1, _word_size_t * returned_size = NULL);

    virtual _word_size_t rxqueue_count();

    _u32 getTermBaudBitmap(_u32 baud);
    
    virtual bool isOpened()
    {
        return _is_serial_opened;
    }

protected:
    volatile bool   _is_serial_opened;

    bool open(const char * portname, uint32_t baudrate, uint32_t flags = NULL);
    void _init();

    char _portName[200];
    uint32_t _baudrate;
    uint32_t _flags;

    int serial_fd;

    size_t required_tx_cnt;
    size_t required_rx_cnt;
};

