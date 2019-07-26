#pragma once
#include "CountDownLatch.h"
#include "MutexLock.h"
#include "Thread.h"
#include "LogStream.h"
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>


// 复制启动一个log 线程， 专门用来将log 写入LogFile， 应用了"双缓冲技术", 实际上是4块缓冲区
// 定时或者被填满时， 将缓冲区中的数据写入LogFile
class AsyncLogging : noncopyable
{
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging()
    {
        if (running_)
            stop();
    }
    void append(const char* logline, int len);

    void start()
    {
        running_ = true;
        // 构造函数中 latch_ 的值为1， 运行之后减为0
        thread_.start();
        latch_.wait();           // 必须等到latch_变为0才能从start函数中返回，初始化完成，等待日志线程启动
    }

    void stop()
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }


private:
    void threadFunc();
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
    typedef std::shared_ptr<Buffer> BufferPtr;                  // 自动管理对象生命期
    const int flushInterval_;                                   // 超时时间
    bool running_;                                              // 是否正在运行
    std::string basename_;
    Thread thread_;                                             // 执行该异步日志的线程
    MutexLock mutex_;
    Condition cond_;
    BufferPtr currentBuffer_;                                   // 当前缓冲
    BufferPtr nextBuffer_;                                      // 预备缓冲
    BufferVector buffers_;                                      // 待写入文件的已填满的缓冲
    CountDownLatch latch_;                                      // 倒数计数，用于等待线程启动
};