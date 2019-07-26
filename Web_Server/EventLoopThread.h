#pragma once
#include "base/Condition.h"
#include "base/MutexLock.h"
#include "base/Thread.h"
#include "base/noncopyable.h"
#include "EventLoop.h"


/* IO线程不一定是主线程，我们可以在任何一个线程创建并运行 EventLoop。
一个程序也可以有不止一个线程，我们可以按照优先级将不同的 socket分给不同的 IO线程，避免优先级反转。
启动一个线程执行一个 EventLoop, "one loop per thread"
 */
class EventLoopThread :noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};