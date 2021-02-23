#ifndef SQLCONNECTIONPOOL_H
#define SQLCONNECTIONPOOL_H

#include <mysql/mysql.h>
#include <vector>
#include <string>
#include <iostream>

#include "../lock/Locker.h"

class SqlConnectionPool
{
public:
    /* 单例模式， 基于局部静态变量 */
    static SqlConnectionPool *getInstance();
    
    /* 初始化连接 */
    void init(const std::string &url, const std::string &user, const std::string &password, \
                            const std::string &database, int port, int max_conn);

    /* 获取一个连接 */
    MYSQL* getConnection();

    /* 释放一个连接 */
    bool releaseConnection(MYSQL*);

    /* 销毁连接池 */
    void destroyPool();

    /* 获取空闲连接数 */
    // int getFreeCount();


private:
    SqlConnectionPool() { }
    ~SqlConnectionPool();

private:
    /* 最大连接数、当前连接数、空闲连接数 */
    int m_max_conn;
    // int m_cur_conn;
    // int m_free_conn;
    std::vector<MYSQL*> m_mysqlpool;
    /* 线程同步，互斥访问m_sqlpool*/
    Locker m_lock;
    Sem m_reserver_sem;

public:
    std::string m_url;
    std::string m_user;
    std::string m_password;
    std::string m_database;
    int m_port;
};


/* 将数据库连接的获取与释放通过RAII机制封装，避免手动释放 */
class ConnectionRAII
{
public:
    ConnectionRAII(MYSQL **conn, SqlConnectionPool *pool);
    ~ConnectionRAII();
private:
    MYSQL *m_connRAII;
    SqlConnectionPool* m_poolRAII;
};

#endif