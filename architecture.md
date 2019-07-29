# Architecture:
I/O 多路复用（事件分配器） + 非阻塞I/O + 主线程（处理请求）+ 工作线程（读、计算、写） + eventloop，即Reactor反应堆模式。

![Architecture](./images/Architecture.png)

## Reactor:
Reactor设计模式是event-driven architecture的一种实现方式，处理多个客户端向服务端请求服务的场景。每种服务在服务端可能由多个方法组成。Reactor会解耦并发请求的服务并分发给对应的事件处理器来处理。

![Reactor](./images/Reactor.png)

Reactor主要由以下几个部分构成：

- Channel {Channel.h,Channel.cpp}
- Epoll {Epoll.h,Epoll.cpp}
- EventLoop{EventLoop.h,EventLoop.cpp,EventLoopThread.h,EventLoopThread.cpp,EventLoopThreadPoll.h,EventLoopThreadPool.cpp}


