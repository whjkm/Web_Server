#pragma once
#include "noncopyable.h"
#include <string>


// 封装Log 文件的打开, 写入， 并在类析构的时候关闭文件
class AppendFile : noncopyable
{
public:
    explicit AppendFile(std::string filename);
    ~AppendFile();
    // append 会向文件写
    void append(const char *logline, const size_t len);
    void flush();

private:
    size_t write(const char *logline, size_t len);
    FILE* fp_;
    char buffer_[64*1024];                    // 缓冲区大小
};