/*
 *  Utility Functions
 *  Lock System
 *  By CSK
 */

#pragma once


class Locker
{
public:
    enum LOCK_STATUS
    {
        LOCK_OK = 1,
        LOCK_TIMEOUT = -1,
        LOCK_FAILED = 0
    };

    Locker(){
        init();
    }

    ~Locker()
    {
        release();
    }

    Locker::LOCK_STATUS lock(unsigned long timeout = 0xFFFFFFFF)
    {
        if (timeout == 0xFFFFFFFF){
            if (pthread_mutex_lock(&_lock) == 0) return LOCK_OK;
        }
        else if (timeout == 0)
        {
            if (pthread_mutex_trylock(&_lock) == 0) return LOCK_OK;
        }
        else
        {
            timespec wait_time;
            timeval now;
            gettimeofday(&now,NULL);

            now.tv_sec += timeout;

            wait_time.tv_sec = now.tv_sec;
            wait_time.tv_nsec = now.tv_usec * 1000;

            switch (pthread_mutex_timedlock(&_lock,&wait_time))
            {
            case 0:
                return LOCK_OK;
            case ETIMEDOUT:
                return LOCK_TIMEOUT;
            }
        }

        return LOCK_FAILED;
    }


    void unlock()
    {
        pthread_mutex_unlock(&_lock);
    }

    pthread_mutex_t *getLockHandle()
    {
        return &_lock;
    }



protected:
    void    init()
    {
        //release();
        pthread_mutex_init(&_lock, NULL);
    }

    void    release()
    {
        unlock(); //force unlock before release
        pthread_mutex_destroy(&_lock);
    }

    pthread_mutex_t _lock;
};

