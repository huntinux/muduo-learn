#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <vector>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
namespace net
{

class Channel;
class Poller;

class EventLoop : boost::noncopyable
{
 public:
  typedef boost::function<void()> Functor;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for scoped_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void loop();

  void quit() { quit_ = true; }

  // internal usage
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }

  //bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  bool isInLoopThread() const { return 0 != pthread_equal(threadId_, pthread_self()); }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  void handleRead();  // waked up

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;

  bool looping_; /* atomic */
  //const pid_t threadId_;
  const pthread_t threadId_;
  boost::scoped_ptr<Poller> poller_;
  bool quit_;

  // scratch variables
  ChannelList activeChannels_;
  Channel* currentActiveChannel_;

};

}
}
#endif  // MUDUO_NET_EVENTLOOP_H
