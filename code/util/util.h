#ifndef UTIL_H
#define UTIL_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>


//对文件描述符设置非阻塞
int setnonblocking(int fd);
//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);
//从内核时间表删除描述符
void removefd(int epollfd, int fd);
//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int TRIGMode);

#endif