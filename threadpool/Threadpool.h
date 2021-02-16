#pragma once
/**
 * 包含m_thread_num
*/
#include<deque>
#include<vector>
#include<pthread.h>     // pthread
#include<memory>        // std::shared_ptr
#include<exception>     // throw std::exception
#include "../http/httpConn.h"
#include "../lock/Locker.h"


class Threadpool 
{
public:
    Threadpool(int thread_number=8, int max_requests=10000);
    ~Threadpool() = default;

    /* 将任务放入任务队列 */ 
    bool append(std::shared_ptr<httpConn> conn);

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
    std::deque<std::shared_ptr<httpConn>> m_tasks;
    std::vector<pthread_t> m_threads;

    /* 用于同步工作队列的信号量和互斥锁 */
    Sem m_sem;
    Locker m_locker;
};

Threadpool::Threadpool(int thread_number=8, int max_requests=10000): \
 m_thread_number(thread_number), m_max_requests(max_requests), m_threads(thread_number) 
 {
    if (thread_number <= 0 || max_requests <= 0) 
    {
        throw std::exception();
    }

    /* 初始化线程 */
    for(int i = 0; i < m_thread_number; ++i) 
    {
        if (pthread_create(&m_threads[i], NULL, worker, this) != 0) 
        {
            throw std::exception();
        }

        if (pthread_detach(m_threads[i])) 
        {
            throw std::exception();
        }
    }

    /* 初始化线程同步机制类(由sem、locker的默认构造函数完成) */
}

bool Threadpool::append(std::shared_ptr<httpConn> conn) 
{
    m_locker.lock();
    if (m_tasks.size() >= m_max_requests) 
    {
        m_locker.unlock();
        return false;
    }

    m_tasks.push_back(conn);
    m_locker.unlock();
    m_sem.post();
    return true;
}

void* Threadpool::worker(void *arg) 
{
    Threadpool *pool = static_cast<Threadpool*>(arg);
    pool->run();
    return pool;
}

void Threadpool::run() 
{
    while (true) 
    {
        m_sem.wait();
        m_locker.lock();
        /* 再次检测队列不为空，避免队列在wait和lock之间发生了状态改变 */
        if (m_tasks.empty()) 
        {
            m_locker.unlock();
            continue;
        }

        /* 从队列中取出任务 */
        std::shared_ptr<httpConn> task = m_tasks.front();
        m_tasks.pop_front();

        /* 解锁 */
        m_locker.unlock();

        /* 执行任务逻辑 */
        task->process();
    }
}