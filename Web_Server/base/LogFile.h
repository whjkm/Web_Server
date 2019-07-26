#pragma once
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"
#include <memory>
#include <string>

// TODO 提供自动归档功能
class LogFile : noncopyable
{
public:
    // 每被append flushEveryN次，flush一下，会往文件写，只不过，文件也是带缓冲区的
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);
    void flush();                                                     // 刷新
    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);               // 不加锁的 append方式   

    const std::string basename_;                                      // 日志文件的 basename
    const int flushEveryN_;

    int count_;                                                       // 计数器，检测是否需要换新文件
    std::unique_ptr<MutexLock> mutex_;                                // 加锁
    std::unique_ptr<AppendFile> file_;                                // 文件智能指针
};