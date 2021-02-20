#ifndef THREADPOOL_H
#define THREADPOOL_H

/**
 * 包含m_thread_num
*/
#include<deque>
#include<vector>
#include<pthread.h>     // pthread
#include<exception>     // throw std::exception
#include "../http/HttpConn.h"
#include "../lock/Locker.h"


class Threadpool 
{
public:
    Threadpool(int thread_number=8, int max_requests=10000);
    ~Threadpool() = default;

    /* 将任务放入任务队列 */ 
    bool append(HttpConn *conn);

private:
    /* 线程函数， 因为普通成员函数带有this参数，不符合pthread_create的参数形式，因此要
    * 设计为static，同时为了能够访问成员变量，需要在调用的时候将this指针作为参数传递给线程函数
    */
   static void *worker(void *arg);

   /* 实际上的线程工作函数，被worker调用(独立封装出来是为了方便访问theadpool的成员)*/
   void run();
 

private:
    int m_thread_number;
    /* 队列所能容纳的最大任务数 */                
    int m_max_requests;
    std::deque<HttpConn*> m_tasks;
    std::vector<pthread_t> m_threads;

    /* 用于同步工作队列的信号量和互斥锁 */
    Sem m_sem;
    Locker m_locker;
};


#endif