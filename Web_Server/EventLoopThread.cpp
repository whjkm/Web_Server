#include "EventLoopThread.h"
#include <functional>


EventLoopThread::EventLoopThread()
:   loop_(NULL),
    exiting_(false),
    thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),
    mutex_(),
    cond_(mutex_)
{ }

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    {
        loop_->quit();
        thread_.join();
    }
}

// 返回新线程中 EventLoop 对象的地址
EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        // 一直等到threadFun在Thread里真正跑起来
        while (loop_ == NULL)
            cond_.wait();
    }
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();                    // 唤醒 startLoop()
    }

    loop.loop();
    //assert(exiting_);
    loop_ = NULL;
}