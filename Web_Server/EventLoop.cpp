#include "EventLoop.h"
#include "base/Logging.h"
#include "Util.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
using namespace std;

__thread EventLoop* t_loopInThisThread = 0;    // 当前线程运行的EventLoop对象

int createEventfd()
{   
    // EFD_CLOEXEC  fork子进程时不继承
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();                               // 立即终止当前进程, 产生异常程序终止
    }
    return evtfd;
}

EventLoop::EventLoop()
:   looping_(false),
    poller_(new Epoll()),
    wakeupFd_(createEventfd()),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    pwakeupChannel_(new Channel(this, wakeupFd_))
{
    // 检查当前线程是否已经创建了其他EventLoop对象
    if (t_loopInThisThread)
    {
        //LOG << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    //pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
    poller_->epoll_add(pwakeupChannel_, 0);
}

void EventLoop::handleConn()
{
    //poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET | EPOLLONESHOT), 0);
    updatePoller(pwakeupChannel_, 0);
}


EventLoop::~EventLoop()
{
    //wakeupChannel_->disableAll();
    //wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = NULL;
}

/* 通过eventfd可以实现线程间通信，其他线程向EventLoop添加任务，通过wakeup向eventfd写一个int
eventfd的回调函数handleRead读取这个int，从而相当于EventLoop被唤醒。
 */
void EventLoop::wakeup()
{
    uint64_t one = 1;                                             // 往wakeupFd_ 中写个"1"
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);     // 写入数据
    if (n != sizeof one)
    {
        LOG<< "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

// 对wakeupFd_ 读出数据
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);              // 读出数据
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    //pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}


// 在它的IO线程内执行某个用户任务回调，如果用户在当前IO线程调用这个函数，回调会同步进行;
// 如果在其他线程调用 runInLoop, cb会被加入队列, IO线程会被唤醒来调用这个Functor。
void EventLoop::runInLoop(Functor&& cb)
{
    if (isInLoopThread())
        cb();
    else
        queueInLoop(std::move(cb));            // 转移，没有多余的复制。
}

// 将cb 放入队列，并在必要时唤醒IO线程，只有在IO线程的事件回调中调用才无须唤醒。
void EventLoop::queueInLoop(Functor&& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    
    // 如果调用queueInLoop 的线程不是IO线程，需要唤醒；
    // 或者在IO线程中调用，而此时正在调用pendding functor, 也需要唤醒。
    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}


// 不能跨线程调用，只能在创建该对象的线程中调用
void EventLoop::loop()
{
    assert(!looping_);                     // 如果不是当前线程就终止
    assert(isInLoopThread());              // 断言当前是否处于创建该对象的线程中
    looping_ = true;
    quit_ = false;
    //LOG_TRACE << "EventLoop " << this << " start looping";
    std::vector<SP_Channel> ret;
    while (!quit_)
    {
        ret.clear();
        ret = poller_->poll();             // 获得当前的就绪事件的channel 列表
        eventHandling_ = true;
        for (auto &it : ret)               // 依次调用每个channel 的事件处理函数
            it->handleEvents();
        eventHandling_ = false;
        doPendingFunctors();
        poller_->handleExpired();          // 处理超时事件
    }
    looping_ = false;
}

// 执行pendingFunctors_ 中的任务回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);     // 交换到局部变量 functors 中
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctors_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}