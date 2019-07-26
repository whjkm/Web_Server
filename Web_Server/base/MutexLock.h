#pragma once
#include "noncopyable.h"
#include <pthread.h>
#include <cstdio>


// 对pthread_mutex 的简单封装
class MutexLock: noncopyable
{
public:
    MutexLock()
    {
        pthread_mutex_init(&mutex, NULL);
    }
    ~MutexLock()
    {
        pthread_mutex_lock(&mutex);
        pthread_mutex_destroy(&mutex);
    }
    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_t *get()
    {
        return &mutex;
    }
private:
    pthread_mutex_t mutex;

// 友元类不受访问权限影响
private:
    friend class Condition;
};


// RAII: 在构造函数中申请分配资源，在析构函数中释放资源
// 将资源或者状态与对象的生命周期绑定
class MutexLockGuard: noncopyable
{
public:
    // 用作用域代表临界区， 处理作用域就自动释放锁
    explicit MutexLockGuard(MutexLock &_mutex):
    mutex(_mutex)
    {
        mutex.lock();
    }
    ~MutexLockGuard()
    {
        mutex.unlock();
    }
private:
    MutexLock &mutex;
};