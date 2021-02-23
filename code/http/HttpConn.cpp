#include "./HttpConn.h"
#include "../util/util.h"


int HttpConn::m_epollfd = -1;
int HttpConn::m_user_count = 0;

void HttpConn::init(int socket, const sockaddr_in &addr)
{
    m_fd = socket;
    m_addr = addr;

    m_iov_cnt = 0;
    m_bytes_to_send = 0;
    m_bytes_have_send = 0;

    addfd(m_epollfd, m_fd, true, 1);
    ++m_user_count;

    /* 清空buffer */
    m_write_buff.retrieveAll();
    m_reader_buff.retrieveAll();
}

void HttpConn::init()
{
    /* 清空buffer */
    m_write_buff.retrieveAll();
    m_reader_buff.retrieveAll();
    
    m_request.init();

}

void HttpConn::close_conn()
{
    if (m_fd != -1)
    {
        // printf("close %d\n", m_fd);
        removefd(m_epollfd, m_fd);
        m_fd = -1;
        m_user_count--;
    }
    m_response.unmap();
}

void HttpConn::process()
{
    // std::cout << "HttpConn process" << std::endl;
    // std::cout << "HttpConn read_buff" << std::endl;
    // std::cout << std::string(m_reader_buff.peek(), m_reader_buff.readableBytes());

    HttpRequest::HTTP_CODE ret = m_request.parse(m_reader_buff);

    // std::cout << "request parse result\n";
    // std::cout << "Url: " << m_request.getUrl() << std::endl; 

    /* 不是一个完整的请求 */
    if (ret == HttpRequest::NO_REQUEST)
    {
        modfd(m_epollfd, m_fd, EPOLLIN, 1);
        /* 直接返回，等待数据再次到来时再处理，m_request 会保存当前的状态 */
        return;
    }

    // std::cout << "Init response" << std::endl;
    /* 初始化HttpResponse， 设置所需信息*/
    m_response.init(m_request.getUrl(), m_request.isKeepAlive(), ret, m_request.getMethod());
    
    /* 构造响应 */
    // std::cout << "make response" << std::endl;
    bool write_ret = m_response.process_response(m_write_buff);
    /* 写buffer空间不够,无法解决，只能关闭连接，一般不会出现这种情况 */
    if (!write_ret)
    {
        close_conn();
        return;
    }

    /* 响应状态行 */
    m_iov[0].iov_base = const_cast<char*>(m_write_buff.peek());
    m_iov[0].iov_len = m_write_buff.readableBytes();
    m_iov_cnt = 1;
    m_bytes_to_send = m_write_buff.readableBytes();

    /* 文件 */
    if (m_response.file_len() > 0 && m_response.file())
    {
        m_iov[1].iov_base = m_response.file();
        m_iov[1].iov_len = m_response.file_len();
        m_iov_cnt = 2;
        m_bytes_to_send += m_response.file_len();
    }

    // std::cout << "write Buff" << std::endl;
    // std::cout << std::string(static_cast<char*>(m_iov[0].iov_base), m_iov[0].iov_len) << std::endl;
    // std::cout << std::string(static_cast<char*>(m_iov[1].iov_base), m_iov[1].iov_len) << std::endl;

    modfd(m_epollfd, m_fd, EPOLLOUT, 1);
}


/* 循环读取客户数据，直到无数据可读或对方关闭连接, ET模式， */
bool HttpConn::read()
{
    int ret_errno = 0;
    return m_reader_buff.readFd(m_fd, &ret_errno);
}


/* 通过分散写 发送数据到客户端 
    false: 表示关闭连接
    true: 表示保持连接
*/
bool HttpConn::write()
{
    //若要发送的数据长度为0
    //表示响应报文为空，一般不会出现这种情况
    if (m_bytes_to_send == 0)
    {
        modfd(m_epollfd, m_fd, EPOLLIN, 1);
        init();
        return true;
    }

    int temp = 0;
    while (1)
    {
        // std::cout << "m_bytes_have_send： " << m_bytes_have_send << std::endl;
        // std::cout << "m_bytes_to_send: " << m_bytes_to_send << std::endl;
        temp = writev(m_fd, m_iov, m_iov_cnt);
        if (temp < 0)
        {
            /* 判断缓冲区是否满了 */
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_fd, EPOLLOUT, 1);
                return true;
            }

            m_response.unmap();
            return false;
        }

        m_bytes_have_send += temp;
        m_bytes_to_send -= temp;

        if (m_bytes_have_send >= m_iov[0].iov_len)
        {
            m_iov[0].iov_len = 0;
            m_iov[1].iov_base = m_response.file() + (m_bytes_have_send - m_write_buff.readableBytes());
            m_iov[1].iov_len = m_bytes_to_send;
        }
        else
        {
            m_iov[0].iov_base = const_cast<char*>(m_write_buff.peek()) + m_bytes_have_send;
            m_iov[0].iov_len = m_write_buff.readableBytes() - m_bytes_have_send;
        }

        if (m_bytes_to_send <= 0)
        {
            m_response.unmap();
            modfd(m_epollfd, m_fd, EPOLLIN, 1);

            if (m_request.isKeepAlive())
            {
                init();
                return true;
            }
            else
            {
                /* @FIXME 为何不在此close_conn*/
                return false;
            }
        }

    }


}