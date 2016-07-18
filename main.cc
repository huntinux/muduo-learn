#include <EventLoop.h>
#include <Channel.h>
#include <sys/timerfd.h>
#include <iostream>

muduo::net::EventLoop* g_loop;

void timeout()
{
    std::cout << "Timeout!" << std::endl;
    g_loop->quit();
}

int main()
{
    muduo::net::EventLoop loop;
    g_loop = &loop;

    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(-1 == timerfd) {
        std::cerr << "timerfd_create faile" << std::endl;
        return -1;
    }
    muduo::net::Channel channel(&loop, timerfd);
    channel.setReadCallback(timeout);
    channel.enableReading();

    struct itimerspec howlong;
    howlong.it_value.tv_sec = 2;
    ::timerfd_settime(timerfd, 0, &howlong, NULL);
    loop.loop();
    ::close(timerfd);
    return 0;
}
