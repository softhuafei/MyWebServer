#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <vector>
#include <string>
#include <memory>
#include <memory.h>
#include <signal.h>

#include "threadpool/Threadpool.h"
#include "http/HttpConn.h"
#include "util/util.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数


class WebServer
{
public:
    /* 初始化 users, m_root */
    WebServer();

    /* 关闭各个FD */
    ~WebServer();

    void init(int port, int thread_num, int opt_linger);

    void eventListen();
    void eventLoop();

private:
    /* 信号处理函数 */
    static void sig_handler(int sig);
    /* 注册信号处理函数 */
    void addsig(int sig, void(handler)(int), bool restart);

    bool dealConn();
    bool dealwithsignal(bool &timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

private:
    static int m_pipefd[2];

private:
    /* 基础 */
    int m_port;
    std::string m_root;

    int m_epollfd;
    int m_listenfd;

    std::vector<HttpConn> m_users;
    /* 线程池 */
    std::unique_ptr<Threadpool> m_threadpool;
    int m_thread_num;

    //epoll_event相关
    epoll_event m_events[MAX_EVENT_NUMBER];

    int m_OPT_LINGER;   // 是否优雅关闭连接



};
