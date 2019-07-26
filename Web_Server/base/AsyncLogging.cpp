#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <functional>


AsyncLogging::AsyncLogging(std::string logFileName_,int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(logFileName_),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_(),
    latch_(1)
{
    assert(logFileName_.size() > 1);
    // 初始化缓冲区
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}


// 发送方
void AsyncLogging::append(const char* logline, int len)
{
    MutexLockGuard lock(mutex_);
    // most common case: buffer is not full, copy data here
    if (currentBuffer_->avail() > len)
        currentBuffer_->append(logline, len);
    else   // buffer is full, push it, and find next spare buffer
    {
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();

        if (nextBuffer_)           // is there is one already, use it
            currentBuffer_ = std::move(nextBuffer_);       // 移动， 而非复制
        else   // allocate a new one
            currentBuffer_.reset(new Buffer);              // Rarely happens
        currentBuffer_->append(logline, len);
        cond_.notify();                                    // 唤醒后端开始写入日志数据
    }
}


// 后端接收方实现，线程调用的函数， 主要用于周期性的flush数据到日志文件中
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_);
    BufferPtr newBuffer1(new Buffer);                     // 两块空闲的buffer, 以备在临界区内交换
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();                                  // 初始化
    newBuffer2->bzero();
    BufferVector buffersToWrite;                          // 写入文件
    buffersToWrite.reserve(16);
    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
        
         // swap out what need to be written, keep CS short
        {
            MutexLockGuard lock(mutex_);
            // 如果 buffer 为空，那么表示没有数据需要写入文件， 那么就等待指定的时间
            if (buffers_.empty())  // unusual usage!
            {
                cond_.waitForSeconds(flushInterval_);
            }

            // 无论 cond是因为什么被唤醒， 都要将currentBuffer_ 放到buffers_ 中
            buffers_.push_back(currentBuffer_);              // 移动
            currentBuffer_.reset();

            currentBuffer_ = std::move(newBuffer1);          // 移动
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);         // 内部指针交换
            }
        }

        assert(!buffersToWrite.empty());
         
        // 如果将要写入文件的buffer 个数大于25，那么将多余数据删除， 消息堆积
        if (buffersToWrite.size() > 25)
        {
            //char buf[256];
            // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
            //          Timestamp::now().toFormattedString().c_str(),
            //          buffersToWrite.size()-2);
            //fputs(buf, stderr);
            //output.append(buf, static_cast<int>(strlen(buf)));

            // 丢掉多余日志， 以腾出内存， 仅保留两块缓冲区
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }
        
        // output buffersTowrite to file
        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }
        
        // 仅保留两个buffer
        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }
        
        // re-fill newBuffer1 and newBuffer2
        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
