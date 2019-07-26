#pragma once
#include "Condition.h"
#include "MutexLock.h"
#include "noncopyable.h"

// CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后
// 外层的start才返回
class CountDownLatch : noncopyable
{
public:
    explicit CountDownLatch(int count);             // 实例化一个倒计时器， count 指定计数个数
    void wait();                                    // 等待
    void countDown();                               // 计数减1

private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};