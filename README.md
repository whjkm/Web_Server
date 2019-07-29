# A High-performance Web Server

[![Build Status](https://www.travis-ci.org/whjkm/Web_Server.svg?branch=master)](https://www.travis-ci.org/whjkm/Web_Server)
[![](https://img.shields.io/badge/language-c++-orange.svg)](http://www.cplusplus.com/)
![GitHub](https://img.shields.io/github/license/whjkm/Web_Server)


## Description:
C++编写的web服务器，借鉴了《muduo网络库》的思想；使用了Reactor并发模型，非阻塞IO+线程池；解析了get、head请求；并实现了异步日志，记录服务器运行状态。

## Environment:
    -OS: ubuntu 16.04
    -Complier: g++5.4
    
## Build:
    ./build.sh

## Usage:
    ./WebServer [-t thread_numbers] [-p port] (using default log_file_path ./WebServer.log)

## Architecture：
![Architecture](./images/Architecture.png)


## Technical Point:

- 使用`Reactor`模式 + `EPOLL(ET)`边沿触发的IO多路复用技术，非阻塞IO。
- 参考了`muduo`的`one loop per pthread`思想，主线程和工作线程各自维持了一个事件循环（`eventloop`）。
- 使用了多线程充分利用多核CPU，并创建了线程池避免线程频繁创建、销毁的开销。
- 主线程只负责accept请求，并以Round Robin的方式分发给其他IO线程。
- 为减少内存泄露的可能，使用了智能指针等RAII机制。
- 使用双缓冲区技术实现了简单的异步日志系统（参考muduo）。
- 使用了基于小根堆的定时器关闭超时请求。
- 线程之间的高效通信，使用了eventfd实现了线程的异步唤醒。
- 使用状态机解析了HTTP请求，支持管线化。
- 支持优雅关闭连接。

## Count Lines of Code:
![Architecture](./images/code.png)

## Future:
未来可以考虑补充的一些功能：

- 实现一个内存池，进一步优化性能；
- 添加缓存cache系统，加快访问速度；
- 使用`docker` 部署运行；
- 对ssl 提供支持，实现https的访问。

## References：
https://github.com/linyacool/WebServer

https://github.com/chenshuo/muduo

https://github.com/grasslog/WebServer

https://github.com/viktorika/Webserver
    


