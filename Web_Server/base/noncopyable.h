#pragma once

// 不可拷贝类（boost）, 私有拷贝构造函数和拷贝复制操作函数
class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    noncopyable(const noncopyable&);
    const noncopyable& operator=(const noncopyable&);
};