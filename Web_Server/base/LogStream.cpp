#include "LogStream.h"
#include <algorithm>
#include <limits>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>


const char digits[] = "9876543210123456789";
const char* zero = digits + 9;                 // 第10位

// From muduo
// 将value 转化为字符串，用buf保存，并返回字符串长度
template<typename T> 
size_t convert(char buf[], T value)
{
    T i = value;
    char *p = buf;

    do
    {
        int lsd = static_cast<int>(i % 10);         // 得到最后一个数字， last digit
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';                                // 为负数添加负号
    }
    *p = '\0';
    std::reverse(buf, p);                          // 逆置返回

    return p - buf;
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

// 将 T类型v转为字符串， 并添加到buffer 后
template<typename T>
void LogStream::formatInteger(T v)
{
    // buffer容不下kMaxNumericSize个字符的话会被直接丢弃
    if (buffer_.avail() >= kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}


// 重载 << 
LogStream& LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

LogStream& LogStream::operator<<(long double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", v);
        buffer_.add(len);
    }
    return *this;
}