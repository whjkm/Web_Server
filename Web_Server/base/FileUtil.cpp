#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

AppendFile::AppendFile(string filename):   
    fp_(fopen(filename.c_str(), "ae"))
{
    // 用户提供缓冲区
    setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
    fclose(fp_);    // 关闭文件
}

// 向文件中写长度为len 的logline
void AppendFile::append(const char* logline, const size_t len)
{
    // 写入文件
    size_t n = this->write(logline, len);
    size_t remain = len - n;
    while (remain > 0)
    {
        size_t x = this->write(logline + n, remain);
        // 写入失败
        if (x == 0)
        {
            int err = ferror(fp_);
            if (err)
                fprintf(stderr, "AppendFile::append() failed !\n");     // 报错信息
            break;
        }
        // n 表示已经写入的数
        n += x;
        remain = len - n;     // 还需写入的数
    }
}

void AppendFile::flush()
{
    fflush(fp_);             // 刷新缓冲区
}

size_t AppendFile::write(const char* logline, size_t len)
{
    return fwrite_unlocked(logline, 1, len, fp_);              // 写文件的不加锁版本，线程不安全，效率高
}