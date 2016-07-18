#include <EventLoop.h>
#include <Channel.h>
#include <Acceptor.h>
#include <sys/timerfd.h>
#include <iostream>

extern void printf_address(int fd, struct sockaddr *in_addr, socklen_t in_len, const char *msg = "");

void newConnection(int sockfd, struct sockaddr &in_addr, socklen_t in_len)
{
    printf_address(sockfd, &in_addr, in_len);
    ::write(sockfd, "jinger\n", 8);
    ::close(sockfd);
}

int main()
{
    muduo::net::EventLoop loop;
    muduo::net::Acceptor acceptor(&loop, "8090");
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();
    loop.loop();
    return 0;
}
