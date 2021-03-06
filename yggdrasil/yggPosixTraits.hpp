#ifndef YGG_POSIX_TRAITS_HPP
#define YGG_POSIX_TRAITS_HPP

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <cassert>

namespace ygg
{

class PosixMutex
{
    friend class PosixCondVar;
public:
    PosixMutex() 
    {
        pthread_mutex_init(&mMutex, NULL);
    }
    ~PosixMutex() 
    {
        pthread_mutex_destroy(&mMutex);
    }
    void lock() 
    {
        pthread_mutex_lock(&mMutex );
    }
    void unlock() 
    {
        pthread_mutex_unlock(&mMutex );
    }
private:
    pthread_mutex_t mMutex;
};

class PosixCondVar
{
public:
    PosixCondVar(PosixMutex& mutex) 
     : mCondMutex(mutex.mMutex) 
    {
        pthread_cond_init(&mCond, NULL);
    }
    ~PosixCondVar() 
    {
        pthread_cond_destroy(&mCond);
    }
    void wait() 
    {
        pthread_cond_wait(&mCond, &mCondMutex);
    }
    void signal() 
    {
        pthread_cond_signal(&mCond);
    }
private:
    pthread_cond_t   mCond;
    pthread_mutex_t& mCondMutex;
};

class PosixThread
{
public:
    typedef bool(*ThreadFunc)(void*);
    typedef bool(*FinalizeFunc)();
private:
    struct ThreadData
    {
        ThreadData(ThreadFunc tFunc, FinalizeFunc fFunc, void* param)
         : mThreadFunc(tFunc), 
           mFinalizeFunc(fFunc),
           mParam(param)
        {}
        ThreadFunc   mThreadFunc;
        FinalizeFunc mFinalizeFunc;
        void*        mParam;
    };
public:
    PosixThread(const char *tName, uint32_t size, uint32_t prio, 
                ThreadFunc threadFunc, FinalizeFunc finalizeFunc = NULL, 
                void* params = NULL)
     : mName(tName)
    {
        (void)size;
        (void)prio;
        ThreadData* tData = new ThreadData(threadFunc, finalizeFunc, params);
        pthread_create(&mThread, NULL, thdStart, tData);
    }

    static void* thdStart(void* data)
    {
        ThreadData* tData = (ThreadData*)data; 
        bool should_quit = false;
        while(!should_quit) {       
            should_quit = tData->mThreadFunc(tData->mParam);
        }
        
        if(tData->mFinalizeFunc) {
            tData->mFinalizeFunc();
        }
        delete tData;
        return NULL;
    }
    static void sleepMilliseconds(uint32_t ms)
    {
        const uint32_t KILO = 1000;
        usleep(ms*KILO);
    }
private:
    pthread_t    mThread;
    std::string  mName;
};

class PosixDevice
{
public:
    enum Mode 
    {
        IN,
        OUT,
        INOUT        
    };
    struct Params 
    {
        std::string mDeviceName;
    };
public:
    PosixDevice(const Params& params, const Mode mode)
    {
        int omode = 0;
        switch(mode) {
            case IN:    omode = O_RDONLY | O_NOCTTY;
                        break;
            case OUT:   omode = O_CREAT | O_TRUNC | O_WRONLY | O_NOCTTY;
                        break;
            case INOUT: omode = O_CREAT | O_TRUNC | O_RDWR | O_NOCTTY;
                        break;
        }
        mDesc = ::open(params.mDeviceName.c_str(), omode, 0644);
    }
    ~PosixDevice()
    {
        close();
    }
    void close()
    {
        if(mDesc) {
            ::close(mDesc);
        }
        mDesc = -1;
    }
    bool read(void* b, uint32_t size)
    {
        uint32_t bytes_read = 0;
        while(size-bytes_read) {
            bytes_read += ::read(mDesc, (uint8_t*)b + bytes_read, size-bytes_read);
        }
        // return false if error on the device
        return true;
    }
    bool write(const void* b, uint32_t size) 
    {
        uint32_t bytes_write = 0;
        while(size-bytes_write) {
            bytes_write += ::write(mDesc, (uint8_t*)b + bytes_write, size-bytes_write);
        }
        // return false if error on the device
        return true;
    }
    bool isOpen() 
    {
        // how else this can be checked?
        return mDesc >= 0;
    }

private:
    int mDesc;
};

class PosixUtils
{
public:
    static uint32_t getMilliseconds()
    {
        static struct timespec sTime;
        const uint32_t KILO = 1000;
        const uint32_t MEGA = 1000000;
        clock_gettime(CLOCK_MONOTONIC, &sTime);
        return (uint32_t) ((sTime.tv_sec * KILO) + (sTime.tv_nsec / MEGA));
    }
};


class PosixSystemTraits
{
public:
    typedef PosixMutex   MutexType;
    typedef PosixCondVar CondType;
    typedef PosixThread  ThreadType;
    typedef PosixDevice  DeviceType;
    typedef PosixUtils   Utils;
};

} // namespace ygg

#endif //YGG_POSIX_TRAITS_HPP
