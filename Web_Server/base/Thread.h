#pragma once
#include "CountDownLatch.h"
#include "noncopyable.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>


class Thread : noncopyable
{
public:
    typedef std::function<void ()> ThreadFunc;
    explicit Thread(const ThreadFunc&, const std::string& name = std::string());     // 不能被隐式调用，只能被显式调用
    ~Thread();
    void start();
    int join();
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

private:
    void setDefaultName();
    bool started_;                                            // if the thread started
    bool joined_;
    pthread_t pthreadId_;
    pid_t tid_;                                              // 管理生命期
    ThreadFunc func_;                                        // 线程回调函数
    std::string name_;
    CountDownLatch latch_;
};