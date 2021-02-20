
#include <regex>
#include <iostream>
#include <strings.h> // strncasecmp
#include "../http/HttpRequest.h"


HttpRequest::HttpRequest()
{
    init();
}

void HttpRequest::init() 
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_http_code = NO_REQUEST;
    m_method = GET;
    m_url = m_version = m_host = m_content = "";
    m_content_length = 0;
    m_linger = false;
}

HttpRequest::HTTP_CODE HttpRequest::parse(Buffer &buffer)
{
    std::string text = "";
    const char* lineEnd = NULL;
    HTTP_CODE ret = NO_REQUEST;

    /**两种情况下可继续解析
     * 1. 当前正在解析的部分为消息体，因为post请求的消息体并不以\r\n结尾，因此直接进入循环，根据buffer中剩余的内容的大小
     * 与content-length字段来判断请求是否完整。
     * 2. 解析的部分不是消息体，且有完整的一行
    */
    while ((m_check_state != CHECK_STATE_FINISH) && 
            ((m_check_state == CHECK_STATE_CONTENT) || ((lineEnd = buffer.findCRLF()) != NULL)))
    {
        /* 解析请求行和头部，每次取出一行内容 */
        if (m_check_state != CHECK_STATE_CONTENT) 
        {
            text = std::string(buffer.peek(), lineEnd);
            buffer.retrieveUtil(lineEnd+2);
        }
        /* 由于消息体中没有CRLF，因此先取出全部内容判断消息体是否完整 */
        else
        {
            text = std::string(buffer.peek(), buffer.readableBytes());
            /* 不需要更新buffer，因为 
            * 如果是完整的，则得到一个完整的请求，不需要再管理buffer的状态，
            * 如果是不完整的，则等剩余的数据到来再解析。*/
        }
            

        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            else if (ret == GET_REQUEST)
            {
                return GET_REQUEST;
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
                return ret;
            m_check_state = CHECK_STATE_FINISH;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

HttpRequest::HTTP_CODE HttpRequest::parse_request_line(const std::string &line)
{
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/1.1$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) 
    {   
        if (strcasecmp(subMatch[1].str().c_str(), "GET") == 0) 
        {
            m_method = GET;
        } 
        else if (strcasecmp(subMatch[1].str().c_str(), "POST") == 0) 
        {
            m_method = POST;
        } 
        else 
        {
            return BAD_REQUEST;
        }


        // std::string path = subMatch[2];
        // /* 去处url中ip:port部分 @FIXME 去除https*/
        // size_t rootPos = path.find("/");
        // if (rootPos == std::string::npos)
        //     return BAD_REQUEST;
        m_url = subMatch[2];

        /* 当url为/时，显示判断界面 */
        if (m_url.compare("/") == 0)
        {
            m_url = "/judge.html";
        }
        
        m_version = "HTTP/1.1";
        m_check_state = CHECK_STATE_HEADER;
        return NO_REQUEST;
    }
    return BAD_REQUEST;
}

HttpRequest::HTTP_CODE HttpRequest::parse_headers(const std::string &text)
{
    /* 遇到空行 */
    if (text.empty())
    {
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        m_check_state = CHECK_STATE_FINISH;
        return GET_REQUEST;
    }

    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(text, subMatch, patten)) 
    {
        if (strcasecmp(subMatch[1].str().c_str(), "Connection") == 0) 
        {
            if (strcasecmp(subMatch[2].str().c_str(), "keep-alive") == 0)
            {
                m_linger = true;
            }
            else
            {
                m_linger = false;
            }
            
        }
        else if (strcasecmp(subMatch[1].str().c_str(), "Content-Length") == 0)
        {
            /* @FIXME 如果不是数字怎么处理？ */
            m_content_length = atol(subMatch[2].str().c_str());
        }
        else if (strcasecmp(subMatch[1].str().c_str(), "Host") == 0)
        {
            m_host = subMatch[2];
        }
        else 
        {
            std::cout << "oop! UNKNOW header: " << subMatch[1] << std::endl;;
        }
    }
    return NO_REQUEST;
}

HttpRequest::HTTP_CODE HttpRequest::parse_content(const std::string &text)
{
    if (text.size() >= m_content_length)
    {
        /* POST请求中最后输入的是用户名和密码 */
        m_content = text.substr(0, m_content_length);
        return GET_REQUEST;
    }

    return NO_REQUEST;
}