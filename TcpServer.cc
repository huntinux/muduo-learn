#include <TcpServer.h>

#include <Acceptor.h>
#include <EventLoop.h>

#include <boost/bind.hpp>

#include <stdio.h>  // snprintf

#include <iostream>

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
            const char *port, const char *address)
  : loop_(loop),
    ipPort_(port),
    name_(string("TcpServer")),
    acceptor_(new Acceptor(loop, port, address)),
    connectionCallback_(NULL),
    messageCallback_(NULL),
    nextConnId_(1)
{
  acceptor_->setNewConnectionCallback(
      boost::bind(&TcpServer::newConnection, this, _1, _2, _3));
    std::cout << "New TcpServer " << this << std::endl;
}

TcpServer::~TcpServer()
{
  loop_->assertInLoopThread();
    std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing" << std::endl;

  for (ConnectionMap::iterator it(connections_.begin());
      it != connections_.end(); ++it)
  {
    TcpConnectionPtr conn = it->second;
    it->second.reset();
    conn->getLoop()->runInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
    conn.reset();
  }
}

void TcpServer::start()
{
  //if (started_.getAndSet(1) == 0)
  //{
  //threadPool_->start(threadInitCallback_);
  //
  //  assert(!acceptor_->listenning());
  //  loop_->runInLoop(
  //      boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  //}
  acceptor_->listen();
}

extern void printf_address(int fd, struct sockaddr *in_addr, socklen_t in_len, const char *msg = "");
void TcpServer::newConnection(int sockfd, struct sockaddr &in_addr, socklen_t in_len)
{
  loop_->assertInLoopThread();
  //EventLoop* ioLoop = threadPool_->getNextLoop();
  char buf[64];
  snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;
  std::cout << "connName:" << connName << std::endl;

  printf_address(sockfd, &in_addr, in_len);

  //LOG_INFO << "TcpServer::newConnection [" << name_
  //         << "] - new connection [" << connName
  //         << "] from " << peerAddr.toIpPort();
  //InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          in_addr, 
                                          in_addr));
                                          //localAddr,
                                          //peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      boost::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
  loop_->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  // FIXME: unsafe
  //loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
  removeConnectionInLoop(conn);
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  std::cout << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name() << std::endl;
  size_t n = connections_.erase(conn->name());
  (void)n;
  assert(n == 1);
  //EventLoop* ioLoop = conn->getLoop();
  //ioLoop->queueInLoop(
  //    boost::bind(&TcpConnection::connectDestroyed, conn));
  //conn.connectDestroyed();
}

