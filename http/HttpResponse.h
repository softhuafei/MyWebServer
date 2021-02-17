#pragma once
#include <string>
#include <sys/stat.h>    // stat
#include "./HttpRequest.h"
#include "../buffer/Buffer.h"
class HttpResponse
{
public:
    bool process_response(HttpRequest::HTTP_CODE ret, Buffer &buff);

    /* 获取指向保存文件内容的指针 */
    char* file();
    size_t fileLen() const;

    void unmap();

private:
    /* 将网站根目录和url文件拼接，然后通过stat判断该文件属性。
    另外，为了提高访问速度，通过mmap进行映射，将普通文件映射到内存逻辑地址。
    根据解析情况返回HTTP CODE
    */
    HttpRequest::HTTP_CODE do_request();
    

    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

private:
    /* 构造response所需要的信息 */
    HttpRequest::HTTP_CODE m_ret;
    std::string m_root;
    std::string m_url;
    std::string m_realFile;
    bool m_isKeepAlive;

    /* 保存映射的文件地址 */
    char* m_file; 
    struct stat m_fileStat;

};
