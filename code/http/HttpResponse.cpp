#include "./HttpResponse.h"

const std::unordered_map<int, std::string> HttpResponse::CODE_STATE = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 500, "Internal Error" }
};

const std::unordered_map<int, std::string> HttpResponse::CODE_FORM = {
    { 200, "<html><body> request ok with empty file </body></html>" }, 
    { 400, "Your request has bad syntax or is inherently impossible to staisfy.\n" },
    { 403, "You do not have permission to get file form this server.\n" },
    { 404, "The requested file was not found on this server.\n" },
    { 500, "There was an unusual problem serving the request file.\n" } 
};

std::string HttpResponse::m_root;

HttpResponse::HttpResponse()
{
    m_root = "";
    m_url = "";
    m_real_file = "";
    m_linger =  false;
    m_http_code = HttpRequest::HTTP_CODE::NO_REQUEST;
    m_file_addr = NULL;
    m_file_stat = {0};
}

HttpResponse::~HttpResponse()
{
    unmap();
}

void HttpResponse::init(const std::string &url, bool isKeepAlive, HttpRequest::HTTP_CODE code,
                        HttpRequest::METHOD method)
{
    m_url = url;
    m_linger = isKeepAlive;
    m_http_code = code;
    m_method = method;
    
    if (m_file_addr)
        unmap();
    m_file_addr = NULL;
    m_file_stat = {0};
}


void HttpResponse::do_request()
{
    m_real_file = m_root;

    // POST 请求，处理登陆/注册
    /* TODO 不同的页面 */
    m_real_file = m_root + m_url;


    /* 判断所请求的文件是否合法 */
    /* 所请求的文件不存在 */
    if (stat(m_real_file.data(), &m_file_stat) < 0)
    {
        m_http_code = HttpRequest::NO_RESOURCE;
        return;
    }

    /* 不可读 */
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        m_http_code = HttpRequest::FORBIDDEN_REQUEST;
        return;
    }

    /* 是目录 */
    if (S_ISDIR(m_file_stat.st_mode))
    {
        m_http_code = HttpRequest::BAD_REQUEST;
        return;
    }

    /* mmap将文件直接映射到内存，提高访问速度 */
    int fd = open(m_real_file.data(), O_RDONLY);
    m_file_addr = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    m_http_code = HttpRequest::FILE_REQUEST;
}

void HttpResponse::unmap()
{
    if (m_file_addr)
    {
        munmap(m_file_addr, m_file_stat.st_size);
        m_file_addr = NULL;
    }
}

char* HttpResponse::file()
{
    return m_file_addr;
}

size_t HttpResponse::file_len() const
{
    return m_file_stat.st_size;
}


bool HttpResponse::process_response(Buffer &buff)
{
    /* 解析url*/
    if (m_http_code == HttpRequest::GET_REQUEST)
        do_request();

    switch(m_http_code)
    {
        case HttpRequest::INTERNAL_ERROR:
        {
            if (!build_error_response(buff, 500))
                return false;
            break;
        }
        case HttpRequest::BAD_REQUEST:
        {
            if (!build_error_response(buff, 400))
                return false;
            break;

        }
        case HttpRequest::NO_RESOURCE:
        {
            if (!build_error_response(buff, 404))
                return false;
            break;

        }
        case HttpRequest::FORBIDDEN_REQUEST:
        {
            if (!build_error_response(buff, 403))
                return false;
            break;
        }
        case HttpRequest::FILE_REQUEST:
        {
            add_status_line(buff, 200);
            if (file_len() != 0)
            {
                add_headers(buff, m_file_stat.st_size);
                return true;
            }
            else
            {
                add_headers(buff, CODE_FORM.find(200)->second.size());
                if (!add_content(buff, 200))
                    return false;
            }
        }
        default:
        {
            return false;
        }
    }
    return true; 
}

    
bool HttpResponse::add_status_line(Buffer &buff, int code)
{
    bool ret = buff.append("HTTP/1.1 " + std::to_string(code) + " " + CODE_STATE.find(code)->second + "\r\n");
    return ret;
}

bool HttpResponse::add_headers(Buffer &buff, size_t content_length)
{
    /* add connection*/
    buff.append("Connection: ");
    if(m_linger) 
    {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } 
    else
    {
        buff.append("close\r\n");
    }

    /* add content length*/
    buff.append("Content-Length: " + std::to_string(content_length) + "\r\n");

    /* add blank line*/
    return buff.append("\r\n");
}

bool HttpResponse::add_content(Buffer &buff, int code)
{
    bool ret = buff.append(CODE_FORM.find(code)->second);
    return ret;
}

bool HttpResponse::build_error_response(Buffer &buff, int code)
{
    add_status_line(buff, code);
    add_headers(buff, CODE_FORM.find(code)->second.size());
    return add_content(buff, code);
}
