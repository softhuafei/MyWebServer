# MyWebServer
## 介绍
基于C++的WebServer，具有以下特性：
- 基于半同步半反应堆的线程池
- 封装了线程同步机制类
- 基于正则表达式和有限状态机的http解析器
- 封装了buffer类，基于ET模式读取数据

## 0.1 webbench1.5 测试结果

```
webbench-1.5 git:(dev) ✗ ./webbench -c 10500 -t 5 http://127.0.0.1:9006/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:9006/
10500 clients, running 5 sec.

Speed=221832 pages/min, 499095 bytes/sec.
Requests: 18486 susceed, 0 failed.

```
## TODO LIST
- 实现数据库连接池
- 实现CGI
- 实现定时器，定期释放不活跃连接
- 实现日志机制
