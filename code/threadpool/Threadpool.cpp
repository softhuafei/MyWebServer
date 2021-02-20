#include "./Threadpool.h"

Threadpool::Threadpool(int thread_number/*=8*/, int max_requests/*=10000*/): \
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

bool Threadpool::append(HttpConn *conn) 
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
        HttpConn* task = m_tasks.front();
        m_tasks.pop_front();

        /* 解锁 */
        m_locker.unlock();

        /* 执行任务逻辑 */
        task->process();
    }
}
