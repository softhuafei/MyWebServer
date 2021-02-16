# MyWebServer
## 介绍
基于C++的WebServer，具有以下特性：
- 基于半同步半反应堆的线程池
- 封装了线程同步机制类
- 基于正则表达式和有限状态机的http解析器
- 封装了buffer类，基于ET模式读取数据

## TODO LIST
- 实现webserver类
- 实现数据库连接池
- 实现CGI
- 实现定时器，定期释放不活跃连接
- 实现日志机制
