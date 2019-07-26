#pragma once
#include "base/Thread.h"
#include "Epoll.h"
#include "base/Logging.h"
#include "Channel.h"
#include "base/CurrentThread.h"
#include "Util.h"
#include <vector>
#include <memory>
#include <functional>

#include <iostream>
using namespace std;


class EventLoop
{
public:
    typedef std::function<void()> Functor;
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    void runInLoop(Functor&& cb);
    void queueInLoop(Functor&& cb);
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    void assertInLoopThread()
    {
        assert(isInLoopThread());
    }
    void shutdown(shared_ptr<Channel> channel)
    {
        shutDownWR(channel->getFd());
    }
    void removeFromPoller(shared_ptr<Channel> channel)
    {
        //shutDownWR(channel->getFd());
        poller_->epoll_del(channel);
    }
    void updatePoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_mod(channel, timeout);
    }
    void addToPoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_add(channel, timeout);
    }
    
private:
    // 声明顺序 wakeupFd_ > pwakeupChannel_
    bool looping_;
    shared_ptr<Epoll> poller_;                 // 实现I/O复用
    int wakeupFd_;
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;                  // mutable 可变
    std::vector<Functor> pendingFunctors_;     // 暴露给了其他线程，所以要用mutex保护
    bool callingPendingFunctors_;
    const pid_t threadId_;                     // 保存当前EventLoop所属线程ID
    shared_ptr<Channel> pwakeupChannel_;       // 用于处理wakeupFd_上的可读事件,将事件分发给handleRead()函数
    
    void wakeup();
    void handleRead();                         // waked up
    void doPendingFunctors();
    void handleConn();
};
