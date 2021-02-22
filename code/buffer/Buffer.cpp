
#include <errno.h>
#include <sys/types.h>  // 
#include <sys/socket.h>
#include "./Buffer.h"

const char Buffer::kCRLF[] = "\r\n";

bool Buffer::readFd(int fd, int* saveErrno) 
{
    /* 没有可写入的空间 */
    if (writableBytes() == 0)
    {
        return false;
    }
    int bytes_read = 0;

    while (true)
    {
        bytes_read = recv(fd, beginWrite(), writableBytes(), 0);
        if (bytes_read == -1) 
        {
            *saveErrno = errno;
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                break;
            }
            return false;
        }
        /* 对端关闭连接 */
        else if (bytes_read == 0)
        {
            return false;
        }
        else
        {
            m_writerIndex += bytes_read;
        }
    }
    return true;
}