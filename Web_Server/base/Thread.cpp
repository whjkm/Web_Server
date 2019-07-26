#include "Thread.h"
#include "CurrentThread.h"
#include <memory>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
using namespace std;


namespace CurrentThread
{
    __thread int t_cachedTid = 0;                   // 用来缓存的 ID
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char* t_threadName = "default";
}


// pid_t 进程号的类型：int; pthread 的 id 号可能出现一样的。
// 获取真实的线程 id 唯一标识 tid
pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}


// 第一次会缓存 tid， 并不会每次都 systemcall, 提高了效率
void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

// 为了在线程中保留name,tid这些数据
// 线程数据类
struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t* tid_;
    CountDownLatch* latch_;

    ThreadData(const ThreadFunc &func, const string& name, pid_t *tid, CountDownLatch *latch)
    :   func_(func),
        name_(name),
        tid_(tid),
        latch_(latch)
    { }

    void runInThread()
    {
        *tid_ = CurrentThread::tid();                             // 得到线程 tid
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);           // 设置线程名        

        func_();                                                  // 线程执行函数
        CurrentThread::t_threadName = "finished";
    }
};


// 线程启动
void *startThread(void* obj)
{
    ThreadData* data = static_cast<ThreadData*>(obj);             // 派生类指针转化为基类指针，obj 是派生类的 this 指针
    data->runInThread();
    delete data;
    return NULL;
}


Thread::Thread(const ThreadFunc &func, const string &n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(n),
    latch_(1)
{
    setDefaultName();
}


// 线程安全的析构函数，析构时确认 thread 没有 join, 才会执行析构，即线程的析构不会等待线程结束。
Thread::~Thread()
{
    // 如果thread 对象的生命周期长于线程，那么可以通过 join 等待线程结束。否则 thread析构时会自动 detach线程，避免资源泄露。
    if (started_ && !joined_)
        pthread_detach(pthreadId_);                 // 如果没有 join 就 detach
}


void Thread::setDefaultName()
{
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");       // 格式化字符串
        name_ = buf;
    }
}

// 线程启动
void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &startThread, data))
    {
        started_ = false;
        delete data;
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}


// 等待线程
int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}