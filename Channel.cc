#include <iostream>
#include <Channel.h>
#include <EventLoop.h>

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
  : loop_(loop),
    fd_(fd__),
    events_(0),
    revents_(0),
    index_(-1)
{
}

Channel::~Channel()
{
  //if (loop_->isInLoopThread())
  //{
  //  assert(!loop_->hasChannel(this));
  //}
}

void Channel::update()
{
  loop_->updateChannel(this);
}

void Channel::remove()
{
  assert(isNoneEvent());
  loop_->removeChannel(this);
}

//void Channel::handleEvent(Timestamp receiveTime)
void Channel::handleEvent()
{
  std::cout << reventsToString() << std::endl;
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
  {
    if (closeCallback_) closeCallback_();
  }

  if (revents_ & POLLNVAL)
  {
    std::cout << "fd = " << fd_ << " Channel::handle_event() POLLNVAL" << std::endl;
  }

  if (revents_ & (POLLERR | POLLNVAL))
  {
    if (errorCallback_) errorCallback_();
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
  {
    if (readCallback_) readCallback_();
  }
  if (revents_ & POLLOUT)
  {
    if (writeCallback_) writeCallback_();
  }
}

string Channel::reventsToString() const
{
  return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
  return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str().c_str();
}
