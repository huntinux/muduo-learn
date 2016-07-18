#include <EventLoop.h>
#include <Channel.h>
#include <Poller.h>
#include <boost/bind.hpp>
#include <signal.h>
#include <pthread.h>

#include <iostream>

using namespace muduo;
using namespace muduo::net;

const int kPollTimeMs = 10000;
__thread EventLoop* t_loopInThisThread = 0;

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    threadId_(pthread_self()),
    poller_(Poller::newDefaultPoller(this)),
    quit_(false),
    currentActiveChannel_(NULL)
{
    std::cout << "EventLoop created " << this << " in thread " << threadId_ << std::endl;
  if (t_loopInThisThread)
  {
    std::cout << "Another EventLoop " << t_loopInThisThread
      << " exists in this thread " << threadId_ << std::endl;
  }
  else
  {
    t_loopInThisThread = this;
  }
}

EventLoop::~EventLoop()
{
  std::cout << "EventLoop " << this << " of thread " << threadId_
    << " destructs in thread " << pthread_self() << std::endl;
  t_loopInThisThread = NULL;
}

/**
 * 最主要的循环
 * 使用poller获得就绪事件对应的channels（存放在容器activeChannels_中）
 * 调用channel的事件处理函数进行处理handleEvent
 */
void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;

    std::cout << "EventLoop " << this << " start looping" << std::endl;

  while (!quit_)
  {
    activeChannels_.clear();
    //pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    poller_->poll(kPollTimeMs, &activeChannels_);

    printActiveChannels();

    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      currentActiveChannel_ = *it;
      //currentActiveChannel_->handleEvent(pollReturnTime_);
      currentActiveChannel_->handleEvent();
    }
    currentActiveChannel_ = NULL;
  }

    std::cout << "EventLoop " << this << " stop looping" << std::endl;
  looping_ = false;
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  std::cout << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
    << ", current thread id = " <<  pthread_self() << std::endl;
}

void EventLoop::printActiveChannels() const
{
  for (ChannelList::const_iterator it = activeChannels_.begin();
      it != activeChannels_.end(); ++it)
  {
    const Channel* ch = *it;
      std::cout << "{" << ch->reventsToString() << "} " << std::endl;
  }
}

