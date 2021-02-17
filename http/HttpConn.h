#pragma once
#include <arpa/inet.h>   // sockaddr_in
#include "../buffer/Buffer.h"
#include "./HttpRequest.h"
#include "./HttpResponse.h"

class httpConn {
public:
    void init(int socket, const sockaddr_in &addr, char *root);
    void close_conn();
    void process();
    bool read();
    bool write();
    

private:
    int m_fd;
    sockaddr_in m_addr;

    /* 被write使用，用于分散读 */
    int m_iovCnt;
    struct iovec m_iov[2];


    Buffer m_readerBuffer;
    Buffer m_writeBuffer;

    HttpRequest m_request;
    HttpResponse m_response;

};


