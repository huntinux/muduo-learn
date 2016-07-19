#include <TcpConnection.h>
#include <Channel.h>
#include <EventLoop.h>

#include <iostream>

#include <boost/bind.hpp>

#include <errno.h>
#include <sys/socket.h>

using namespace muduo;
using namespace muduo::net;

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                	         struct sockaddr &localAddr,
                             struct sockaddr &peerAddr)
  : loop_(loop),
    name_(nameArg),
    state_(kConnecting),
    socket_(sockfd),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024),
    reading_(true)
{
  channel_->setReadCallback(
      boost::bind(&TcpConnection::handleRead, this));
  channel_->setWriteCallback(
      boost::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(
      boost::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(
      boost::bind(&TcpConnection::handleError, this));
  std::cout << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << sockfd << std::endl;
  //socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    std::cout << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd()
    << " state=" << stateToString() << std::endl;
  assert(state_ == kDisconnected);
}

/*
bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
  return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof buf);
  return buf;
}
*/

//void TcpConnection::send(const void* data, int len)
//{
//  //send(StringPiece(static_cast<const char*>(data), len));
//}

//void TcpConnection::send(const StringPiece& message)
void TcpConnection::send(const void* data, int len)
{
  if (state_ == kConnected)
  {
    //if (loop_->isInLoopThread())
    //{
    //  sendInLoop(message);
    //}
    //else
    //{
    //  //loop_->runInLoop(
    //  //    boost::bind(&TcpConnection::sendInLoop,
    //  //                this,     // FIXME
    //  //                message.as_string()));
    //                //std::forward<string>(message)));
    //}
    sendInLoop(data, len);
  }
}

// FIXME efficiency!!!
//void TcpConnection::send(Buffer* buf)
//{
//  if (state_ == kConnected)
//  {
//    if (loop_->isInLoopThread())
//    {
//      sendInLoop(buf->peek(), buf->readableBytes());
//      buf->retrieveAll();
//    }
//    else
//    {
//      loop_->runInLoop(
//          boost::bind(&TcpConnection::sendInLoop,
//                      this,     // FIXME
//                      buf->retrieveAllAsString()));
//                    //std::forward<string>(message)));
//    }
//  }
//}

//void TcpConnection::sendInLoop(const StringPiece& message)
//{
//  sendInLoop(message.data(), message.size());
//}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if (state_ == kDisconnected)
  {
      std::cout << "disconnected, give up writing" << std::endl;
    return;
  }
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        //loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
        writeCompleteCallback_(shared_from_this());
      }
    }
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
          std::cout << "TcpConnection::sendInLoop" << std::endl;
        if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
        {
          faultError = true;
        }
      }
    }
  }

  assert(remaining <= len);
  if (!faultError && remaining > 0)
  {
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      //loop_->queueInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        highWaterMarkCallback_(shared_from_this(), oldLen + remaining);
    }
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting())
  {
    // we are not writing
    //socket_->shutdownWrite();
      ::shutdown(socket_, SHUT_WR);
  }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

//void TcpConnection::forceClose()
//{
//  // FIXME: use compare and swap
//  if (state_ == kConnected || state_ == kDisconnecting)
//  {
//    setState(kDisconnecting);
//    loop_->queueInLoop(boost::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
//  }
//}
//
//void TcpConnection::forceCloseWithDelay(double seconds)
//{
//  if (state_ == kConnected || state_ == kDisconnecting)
//  {
//    setState(kDisconnecting);
//    loop_->runAfter(
//        seconds,
//        makeWeakCallback(shared_from_this(),
//                         &TcpConnection::forceClose));  // not forceCloseInLoop to avoid race condition
//  }
//}

void TcpConnection::forceCloseInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

const char* TcpConnection::stateToString() const
{
  switch (state_)
  {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  //socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
  loop_->runInLoop(boost::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading())
  {
    channel_->enableReading();
    reading_ = true;
  }
}

/*
void TcpConnection::stopRead()
{
  loop_->runInLoop(boost::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading())
  {
    channel_->disableReading();
    reading_ = false;
  } 
}
*/

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  //channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();

    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead()
{
  loop_->assertInLoopThread();
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0)
  {
    //messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	//messageCallback_(shared_from_this(), &inputBuffer_.data(), inputBuffer.size());
	messageCallback_(shared_from_this(), &inputBuffer_);
  }
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    errno = savedErrno;
      std::cout << "TcpConnection::handleRead" << std::endl;
    handleError();
  }
}

void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting())
  {
    ssize_t n = ::write(channel_->fd(),
                               outputBuffer_.peek(),
                               outputBuffer_.readableBytes());
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0)
      {
        channel_->disableWriting();
        if (writeCompleteCallback_)
        {
          //loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
          writeCompleteCallback_(shared_from_this());
        }
        if (state_ == kDisconnecting)
        {
          shutdownInLoop();
        }
      }
    }
    else
    {
        std::cout << "TcpConnection::handleWrite" << std::endl;
       if (state_ == kDisconnecting)
       {
         shutdownInLoop();
       }
    }
  }
  else
  {
      std::cout << "Connection fd = " << channel_->fd()
      << " is down, no more writing" << std::endl;
  }
}

void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();
    std::cout << "fd = " << channel_->fd() << " state = " << stateToString() << std::endl;
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  // must be the last line
  closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
  //int err = sockets::getSocketError(channel_->fd());
  //LOG_ERROR << "TcpConnection::handleError [" << name_
  //          << "] - SO_ERROR = " << err << " " << strerror_tl(err);
    std::cout << "This function is not implement" << std::endl;
}

