#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <algorithm>
/*
* 固定大小的buffer，buffer分为三个区域，prepend，reader，writer
* prepend区域：保留区域，方便流数据的读写，[begin(), peek())
* reader区域：保存可读内容，[peek(), beginWrite())
* writer区域：可写区域，[beginWrite(), m_data.end())

* et读socker fd的数据
* 提供获取用于读写的char指针。不使用迭代器的原因，与标准io的接口兼容，方便后续扩展为自动扩容的buffer。
*/


class Buffer
{
public:
    /* 默认buffer前用于append的空间大小与buffer大小 */
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;


    explicit Buffer(size_t initialSize = kInitialSize)
    : m_buffer(kCheapPrepend+initialSize), m_readerIndex(kCheapPrepend), m_writerIndex(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    /* 获取可读与可写空间的大小 */
    size_t readableBytes() const 
    {
        return m_writerIndex - m_readerIndex;
    }

    size_t writableBytes() const
    {
        return m_buffer.size() - m_writerIndex;
    }
    /* 可prepend的空间大小 */
    size_t prependableBytes() const {
        return m_readerIndex;
    }

    /* 返回读指针，指向可读区域的第一个字符 */
    const char* peek() const
    {
        return begin() + m_readerIndex;
    }

    /* 在可读区域找 /r/n，找到,返回的指针指向/r，没找到,返回NULL */
    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    /* 用户读取可读区域的内容后，使用retrieve系列汉斯修改可读区域的范围 */
    /* 读取全部内容 */
    void retrieveAll()
    {
        m_readerIndex = kCheapPrepend;
        m_writerIndex = kCheapPrepend;
    }

    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            m_readerIndex += len;
        } 
        else
        {
            retrieveAll();
        } 

    }

    void retrieveUtil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    /* 返回可读区域的内容，并修改可读区域的范围 */
    std::string retrieveAllAsString()
    {
        std::string ret(peek(), readableBytes());
        retrieveAll();
    }

    std::string retrieveAsString(size_t len) 
    {
        assert(len <= readableBytes());
        std::string ret(peek(), len);
        retrieve(len);
        return ret;
    }




    /* 返回写指针，指向可写区域的第一个字符 */
    char* beginWrite()
    { 
        return begin() + m_writerIndex; 
    }

    const char* beginWrite() const
    { 
        return begin() + m_writerIndex; 
    }


    /* 基于ET模式的读，将fd的内容读入buffer，直到遇到EAGAIN
    * @return if read success, @c errno is saved */ 
    bool readFd(int fd, int* savedErrno);

private:
    char* begin()
    {
        return &*m_buffer.begin();
    }

    const char* begin() const
    {
        return &*m_buffer.begin();
    }



private:
    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;

    static const char kCRLF[];  /* 回车换行符 */
};

