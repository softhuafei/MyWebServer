#include "WebServer.h"


int WebServer::m_pipefd[2];

WebServer::WebServer(): m_users(MAX_FD)
{
    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    m_root = server_path;
    m_root += "/root";
    HttpResponse::m_root = m_root;
}

WebServer::~WebServer()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[0]);
    close(m_pipefd[1]);
}

void WebServer::init(int port, int thread_num, int opt_linger)
{
    m_port = port;
    m_thread_num = thread_num;
    m_OPT_LINGER = opt_linger;

    m_threadpool = std::make_unique<Threadpool>(thread_num);

    std::cout << "webServer init() done" << std::endl;
    std::cout << "port :" << std::to_string(m_port) << std::endl;
}


void WebServer::sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(m_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void WebServer::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));

    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void WebServer::eventListen()
{
    /* 创建listensocket */
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    /* 优雅关闭连接: 直到所剩数据发送完毕或超时 */
    struct linger temp = {0};
    if (1 == m_OPT_LINGER)
    {
        temp.l_onoff = 1;
        temp.l_linger = 1;
        
    }
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &temp, sizeof(temp));

    int ret = 0;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(m_port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    /* 绑定地址 */
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    /* 设置为监听 socket */
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    /* 创建epollfd内核事件表 */
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    HttpConn::m_epollfd = m_epollfd;
    
    
    addfd(m_epollfd, m_listenfd, false, 0);

    /* 创建同步信号事件的管道 */
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    setnonblocking(m_pipefd[1]);
    addfd(m_epollfd, m_pipefd[0], false, 0);

    /* 设置信号处理函数 */
    addsig(SIGPIPE, SIG_IGN, false);
    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);
}

void WebServer::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(m_epollfd, m_events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EAGAIN)
        {
            break;
        }

        for (int i = 0; i < number; ++i)
        {
            int sockfd = m_events[i].data.fd;

            /* 处理新到的客户连接 */
            if (sockfd == m_listenfd)
            {
                if (!dealConn())
                    continue;
            }
            /* 出现异常 */
            else if (m_events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
            {
                /* 关闭服务端连接 */
                m_users[sockfd].close_conn();
            }
            /* 处理信号 */
            else if ((sockfd == m_pipefd[0]) && (m_events[i].events & EPOLLIN))
            {
                dealwithsignal(timeout, stop_server);
            }
            /* 处理连接上到来的客户请求*/
            else if (m_events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            /* 发送响应到客户端 */
            else if (m_events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
    }
}

bool WebServer::dealConn()
{
    struct sockaddr_in client_addr;
    socklen_t client_addressLength = sizeof(client_addr);

    int connfd = accept(m_listenfd, (struct sockaddr *)&client_addr, &client_addressLength);
    if (connfd < 0)
    {
        return false;
    }

    if (HttpConn::m_user_count >= MAX_FD)
    {
        std::string info = "Internal server busy";
        send(connfd, info.data(), strlen(info.data()), 0);
        close(connfd);
        return false;
    }

    /* 初始化连接 */
    m_users[connfd].init(connfd, client_addr);

    // std::cout << "new conn coming, init httpConn done" << std::endl;
    return true;
}

bool WebServer::dealwithsignal(bool &timeout, bool& stop_server)
{
    int ret = 0;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);

    if (ret == -1 || ret == 0)
    {
        return false;
    }
    else 
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
                timeout = true;
                break;
            case SIGTERM:
                stop_server = true;
                break;
            default:
                break;
            }
        }
    }
    return true;
}

void WebServer::dealwithread(int sockfd)
{
    // std::cout << "sockfd" << sockfd << " coming data, reading" << std::endl;
    if (m_users[sockfd].read())
    {
        // printf("read from with the client(%s)\n", inet_ntoa(m_users[sockfd].get_address()->sin_addr));
        /* 成功读取请求对象，将请求放入工作队列 */
        m_threadpool->append(&m_users[sockfd]);
    }
    else
    {
        /* 读取数据失败，关闭服务端连接 */
        m_users[sockfd].close_conn();
    }
}

void WebServer::dealwithwrite(int sockfd)
{
    if (m_users[sockfd].write())
    {
        // printf("send data to the client(%s)\n", inet_ntoa(m_users[sockfd].get_address()->sin_addr));
        
    }
    else
    {
        /* 发送响应失败或成功完成一次短连接，关闭服务端连接 */
        m_users[sockfd].close_conn();
    }
}