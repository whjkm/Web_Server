#pragma once
#include "LogStream.h"
#include <pthread.h>
#include <string.h>
#include <string>
#include <stdio.h>

class AsyncLogging;


// 对外接口
class Logger
{
public:
    Logger(const char *fileName, int line);
    ~Logger();
    LogStream& stream() { return impl_.stream_; }                // 返回一个Logstream对象

    static void setLogFileName(std::string fileName)
    {
        logFileName_ = fileName;
    }
    static std::string getLogFileName()
    {
        return logFileName_;
    }

private:
    class Impl
    {
    public:
        Impl(const char *fileName, int line);
        void formatTime();                                      // 格式化时间

        LogStream stream_;
        int line_;                                              // 行号
        std::string basename_;
    };
    Impl impl_;
    static std::string logFileName_;
};

#define LOG Logger(__FILE__, __LINE__).stream()