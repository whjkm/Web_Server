#include "LogFile.h"
#include "FileUtil.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace std;


LogFile::LogFile(const string& basename, int flushEveryN)
  : basename_(basename),
    flushEveryN_(flushEveryN),
    count_(0),
    mutex_(new MutexLock)         // 默认为加锁
{
    //assert(basename.find('/') >= 0);
    file_.reset(new AppendFile(basename));
}

LogFile::~LogFile()
{ }


// 追加日志文本
void LogFile::append(const char* logline, int len)
{
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
}

void LogFile::flush()
{
    MutexLockGuard lock(*mutex_);
    file_->flush();
}

void LogFile::append_unlocked(const char* logline, int len)
{
    file_->append(logline, len);                             // 调用AppendFile中的不加锁append
    ++count_;                                                // 增加行数
    if (count_ >= flushEveryN_)                              // 每 n行刷新一次缓冲区，写入文件
    {
        count_ = 0;
        file_->flush();                                      // 刷新，写入文件
    }
}