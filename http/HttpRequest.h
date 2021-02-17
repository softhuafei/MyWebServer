#pragma once

#include <string>
#include "../buffer/Buffer.h"

class HttpRequest {
public:
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT,
        CHECK_STATE_FINISH
    };

    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    HttpRequest() { init(); }

    void init();
    HTTP_CODE parse(Buffer &buffer);
    
    bool isKeepAlive() const 
    {
        return m_linger;
    }

    std::string getHost() const
    {
        return m_host;
    }

    std::string getUrl() const
    {
        return m_url;
    }

    METHOD getMethod() const
    {
        return m_method;
    }

    std::string getVersion() const
    {
        return m_version;
    }

    std::string getContent() const
    {
        return m_content;
    }


private:
    
    HTTP_CODE parse_request_line(const std::string &text);
    HTTP_CODE parse_headers(const std::string &text);
    HTTP_CODE parse_content(const std::string &text);
    

    
    CHECK_STATE m_check_state;
    HTTP_CODE m_http_code;

    /* 请求行的信息 */
    METHOD m_method;
    std::string m_url;
    std::string m_version;
    /* 请求头 */
    std::string m_host;
    int m_content_length;
    bool m_linger;
    /* 请求正文 */
    std::string m_content;
};
