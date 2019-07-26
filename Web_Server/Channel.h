#pragma once
#include "Timer.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include <sys/epoll.h>


class EventLoop;
class HttpData;

// 事件分发器; 每个 Channel 只属于一个 EventLoop, 每个 Channel 只负责一个文件描述符 fd 的 IO 事件分发
// 但它并不拥有这个 fd, 也不会在析构的时候关闭这个 fd。

class Channel
{
private:
    typedef std::function<void()> CallBack;                 // 回调函数
    EventLoop *loop_;
    int fd_;                                                // 文件描述符
    __uint32_t events_;                                     // 文件描述符注册事件
    __uint32_t revents_;                                    // 就绪事件
    __uint32_t lastEvents_;

    // 方便找到上层持有该Channel的对象
    std::weak_ptr<HttpData> holder_;

private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

    CallBack readHandler_;
    CallBack writeHandler_;
    CallBack errorHandler_;
    CallBack connHandler_;

public:
    Channel(EventLoop *loop);
    Channel(EventLoop *loop, int fd);
    ~Channel();
    int getFd();
    void setFd(int fd);

    void setHolder(std::shared_ptr<HttpData> holder)
    {
        holder_ = holder;
    }
    std::shared_ptr<HttpData> getHolder()
    {
        std::shared_ptr<HttpData> ret(holder_.lock());
        return ret;
    }

    void setReadHandler(CallBack &&readHandler)
    {
        readHandler_ = readHandler;
    }
    void setWriteHandler(CallBack &&writeHandler)
    {
        writeHandler_ = writeHandler;
    }
    void setErrorHandler(CallBack &&errorHandler)
    {
        errorHandler_ = errorHandler;
    }
    void setConnHandler(CallBack &&connHandler)
    {
        connHandler_ = connHandler;
    }

    /* EPOLL 事件：
    EPOLLIN: 触发该事件（有可读数据）， EPOLLOUT: 触发该事件（可以写数据），
    EPOLLPRI: 有紧急的数据可读（有带外的数据到来）， EPOLLERR: 发生错误，
    EPOLLHUP: 被挂断， EPOLLET: 将EPOLL设为边缘触发模式（ET），相对于（LT）
    根据revents_ 的值分别调用不同的用户回调
     */
    void handleEvents()
    {
        events_ = 0;
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            events_ = 0;
            return;
        }
        if (revents_ & EPOLLERR)
        {
            if (errorHandler_) errorHandler_();
            events_ = 0;
            return;
        }
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
        {
            handleRead();
        }
        if (revents_ & EPOLLOUT)
        {
            handleWrite();
        }
        handleConn();
    }
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    void setRevents(__uint32_t ev)
    {
        revents_ = ev;
    }

    void setEvents(__uint32_t ev)
    {
        events_ = ev;
    }
    __uint32_t& getEvents()
    {
        return events_;
    }

    bool EqualAndUpdateLastEvents()
    {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    __uint32_t getLastEvents()
    {
        return lastEvents_;
    }

};

typedef std::shared_ptr<Channel> SP_Channel;