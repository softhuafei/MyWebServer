#pragma once
#include <string>
#include <sys/stat.h>    // stat
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/mman.h>    // mmap, munmap
#include <unordered_map>
#include "./HttpRequest.h"
#include "../buffer/Buffer.h"
class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string &root, const std::string &url, bool isKeepAlive, HttpRequest::HTTP_CODE code);

    bool process_response(Buffer &buff);

    /* 获取指向文件内容的内存指针 */
    char* file();
    size_t file_len() const;
    void unmap();

private:
    /* 将网站根目录和url文件拼接，然后通过stat判断该文件属性。
    另外，为了提高访问速度，通过mmap进行映射，将普通文件映射到内存逻辑地址。
    根据解析情况修改Http Code
    */
    void do_request();

    bool add_status_line(Buffer &buff, int code);
    bool add_headers(Buffer &buff, size_t content_length);
    bool add_content(Buffer &buff, int code);

    bool build_error_response(Buffer &buff, int code);

private:
    /* 构造response所需要的信息 */
    HttpRequest::HTTP_CODE m_http_code;
    std::string m_root;
    std::string m_url;
    std::string m_real_file;
    bool m_linger;

    /* 保存映射的文件地址 */
    char* m_file_addr; 
    struct stat m_file_stat;

    /* 状态码对应的title*/
    static const std::unordered_map<int, std::string> CODE_STATE;

    /* 除200外的状态码对应的content */
    static const std::unordered_map<int, std::string> CODE_FORM;
};
