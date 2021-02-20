#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <arpa/inet.h>   // sockaddr_in
#include <sys/uio.h>     // readv/writev
#include <string>
#include <iostream>
#include "../buffer/Buffer.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

class HttpConn {
public:
    /* 初始化新接受的连接 */
    void init(int socket, const sockaddr_in &addr);

    void close_conn();
    /* 解析请求并构造返回， threadpool调用 */
    void process();

    /* 读取fd上的请求数据， webServer调用 */
    bool read();

    /* 发送响应， webServer调用 */ 
    bool write();
    
    sockaddr_in *get_address()
    {
        return &m_addr;
    }

private:
    /* 重置连接状态, 清空buffer，重置request解析状态 */
    void init();

public:
    static int m_epollfd;
    static int m_user_count;

private:
    int m_fd;
    sockaddr_in m_addr;


    /* 被write使用，用于分散读 */
    int m_iov_cnt;
    struct iovec m_iov[2];
    size_t m_bytes_to_send;
    size_t m_bytes_have_send;

    Buffer m_reader_buff;
    Buffer m_write_buff;

    HttpRequest m_request;
    HttpResponse m_response;
};


#endif