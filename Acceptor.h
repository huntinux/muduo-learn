#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <Channel.h>

#include <netinet/in.h>

namespace muduo
{
namespace net
{

class EventLoop;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : boost::noncopyable
{
 public:
  typedef boost::function<void (int sockfd, struct sockaddr &in_addr, socklen_t in_len)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const char *port, const char *address=NULL, bool reuseport=true);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop* loop_;
  int acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listenning_;
};

}
}

#endif  // MUDUO_NET_ACCEPTOR_H
