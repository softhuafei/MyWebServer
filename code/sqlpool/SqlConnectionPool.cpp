#include "./SqlConnectionPool.h"

SqlConnectionPool::~SqlConnectionPool()
{
    destroyPool();
}

void SqlConnectionPool::init(const std::string &url, const std::string &user, const std::string &password, \
                            const std::string &database, int port, int max_conn)
{
    m_url = url;
    m_user = user;
    m_password = password;
    m_database = database;
    m_port = port;
    m_max_conn = max_conn;
    // m_free_conn = max_conn;

    /* 初始化连接池 */
    for (int i = 0; i < max_conn; ++i)
    {
        MYSQL *conn = NULL;
        conn = mysql_init(conn);

        if (conn == NULL)
        {
            std::cout << "MySql Erro" << std::endl;
            exit(1);
        }

        conn = mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), database.c_str(), port, NULL, 0);

        if (conn == NULL)
        {
            std::cout << "Error: " << mysql_error(conn);
            exit(1);
        }

        m_mysqlpool.push_back(conn);
    }
    // 初始化信号量为最大连接数
    m_reserver_sem = Sem(max_conn);

}


SqlConnectionPool* SqlConnectionPool::getInstance()
{
    // C++11保证局部静态变量具有线程安全
    // 会在第一个调用getInstance的时候执行初始化
    static SqlConnectionPool connPool;
    return &connPool;
}

MYSQL* SqlConnectionPool::getConnection()
{
    m_reserver_sem.wait();
    m_lock.lock();
    if (m_mysqlpool.empty())
    {
        m_lock.unlock();
        return NULL;
    }

    MYSQL* conn = m_mysqlpool.back();
    m_mysqlpool.pop_back();

    // ++m_cur_conn;
    // --m_free_conn;
    m_lock.unlock();
    return conn;
}

bool SqlConnectionPool::releaseConnection(MYSQL *conn)
{
    if (conn == NULL)
        return false;

    m_lock.lock();
    m_mysqlpool.push_back(conn);
    m_lock.unlock();
    //释放连接原子加1
    m_reserver_sem.post();
    return true;
}

void SqlConnectionPool::destroyPool()
{
    m_lock.lock();

    for (auto iter = m_mysqlpool.begin(); iter != m_mysqlpool.end(); ++iter)
    {
        MYSQL *conn = *iter;
        mysql_close(conn);
    }

    m_mysqlpool.clear();

    m_lock.unlock();
}

// 将数据库连接的获取与释放通过RAII机制封装，避免手动释放
ConnectionRAII::ConnectionRAII(MYSQL **conn, SqlConnectionPool *pool)
{
    *conn = pool->getConnection();
    m_poolRAII = pool;
    m_connRAII = *conn;
}

ConnectionRAII::~ConnectionRAII()
{
    m_poolRAII->releaseConnection(m_connRAII);
}