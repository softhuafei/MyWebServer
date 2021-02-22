#ifndef LOCKER_H
#define LOCKER_H


#include <exception>
#include <semaphore.h>  // sem
#include <pthread.h>    // pthread_mutex
/*
封装 线程同步机制
*/

/* 封装信号量 */
class Sem 
{
public:
    Sem() 
    {
        if (sem_init(&m_sem, 0, 0) != 0) 
        {
            throw std::exception();
        }
        
    }

    Sem(int value) 
    {
        if (sem_init(&m_sem, 0, value) != 0) 
        {
            throw std::exception();
        }
    }

    ~Sem() 
    {
        sem_destroy(&m_sem);
    }

    bool wait() 
    {
        return sem_wait(&m_sem) == 0;
    }

    bool post() 
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem; 
};

/* 封装 互斥锁*/
class Locker 
{
public:
    Locker() 
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
            throw std::exception();

    }

    ~Locker() 
    {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock() 
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock() 
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t* get() 
    {
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;

};

/* 封装 条件变量 */
class Cond {
public:
    Cond() 
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
            throw std::exception();
    }

    ~Cond() 
    {
        pthread_cond_destroy(&m_cond);
    }


    bool wait(pthread_mutex_t* mutex) 
    {
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, mutex);
        return ret == 0;
    }
    
    bool timewait(pthread_mutex_t* mutex, struct timespec t) 
    {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, mutex, &t);
        return ret == 0;
    }

    bool signal() 
    {
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast() 
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    pthread_cond_t m_cond;
};


#endif