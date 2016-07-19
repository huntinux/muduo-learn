#include <EventLoop.h>
#include <Channel.h>
#include <Acceptor.h>
#include <sys/timerfd.h>
#include <iostream>
#include <Buffer.h>
#include <TcpServer.h>

extern void printf_address(int fd, struct sockaddr *in_addr, socklen_t in_len, const char *msg = "");

#ifdef ACCEPTOR_TEST
void newConnection(int sockfd, struct sockaddr &in_addr, socklen_t in_len)
{
    printf_address(sockfd, &in_addr, in_len);
    ::write(sockfd, "jinger\n", 8);
    ::close(sockfd);
}

void runInLoopTest() {
    std::cout << "runInLoopTest " << std::endl;
}
#endif

void onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if(conn->connected()) {
        std::cout << "New connection" << std::endl;
    } else {
        std::cout << "Connection failed" << std::endl;
    }
}

void onMessage(const muduo::net::TcpConnectionPtr& conn,
               muduo::net::Buffer *buffer)
              //const char* data,
              //ssize_t len)
{
    const std::string readbuf = buffer->retrieveAllAsString();
    std::cout << "Receive :" << readbuf.size()<< " bytes." << std::endl
              << "Content:"  << readbuf << std::endl; 
}

int main()
{

    muduo::net::EventLoop loop;
    muduo::net::TcpServer server(&loop, "8090");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();
    loop.loop();

#ifdef BUFFER_TEST
    muduo::net::Buffer buff;
    buff.append("Buffer", 6);
    std::cout << buff.retrieveAllAsString() << std::endl;
#endif

#ifdef ACCEPTOR_TEST
    muduo::net::EventLoop loop;
    muduo::net::Acceptor acceptor(&loop, "8090");
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();
    loop.runInLoop(runInLoopTest);
    loop.loop();
#endif

    return 0;
}
