#pragma once
#include <stdint.h>


namespace CurrentThread
{
    // internal
    // 每一个线程有一份独立实体 __thread
    extern __thread int t_cachedTid;                        // 线程的真实ID
    extern __thread char t_tidString[32];                   // 用string类型表示tid, 便于日志的输出
    extern __thread int t_tidStringLength;                  // string类型tid的长度
    extern __thread const char* t_threadName;               // 线程的名字
    void cacheTid();
    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))          // 为0的可能性很大
        {
            cacheTid();
        }
        return t_cachedTid;
    }

    inline const char* tidString() // for logging
    {
        return t_tidString;
    }

    inline int tidStringLength() // for logging
    {
        return t_tidStringLength;
    }

    inline const char* name()
    {
        return t_threadName;
    }
}

