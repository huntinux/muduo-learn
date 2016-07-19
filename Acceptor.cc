#include <Acceptor.h>

#include <EventLoop.h>

#include <boost/bind.hpp>

#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <netinet/tcp.h> // TCP_NODELAY 
#include <arpa/inet.h>   // inet_pton 
#include <netdb.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#define INVALID_SOCKET -1
void printf_address(int fd, struct sockaddr *in_addr, socklen_t in_len, const char *msg = "")
{
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	if (getnameinfo(in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV) == 0)
	{
		printf("[%8d]%s:  (host=%s, port=%s)\n", fd, msg, hbuf, sbuf);
	}
}
bool make_socket_reusable(int sfd)
{
	int enable = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&enable, sizeof(int)) < 0)
	{
		printf("error setsockopt SO_REUSEADDR\n");
		return false;
	}
	return true;
}
bool make_socket_non_blocking(int sfd)
{
	int flags;

	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1)
	{
		perror("fcntl");
		return false;
	}

	flags |= O_NONBLOCK;
	if (fcntl(sfd, F_SETFL, flags) == -1)
	{
		perror("fcntl");
		return false;
	}
	flags = 1;
	if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flags, sizeof(int)) < 0)
	{
		printf("setsockopt TCP_NODELAY error, errno %d", errno);
	}
	return true;
}
static int create_and_bind(const char *port, const char *address=NULL)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s;
	int sfd;

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* All interfaces */

	s = getaddrinfo(address, port, &hints, &result);
	if (s != 0)
	{
		printf("getaddrinfo: %s", gai_strerror(s));
		return INVALID_SOCKET;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == INVALID_SOCKET)
			continue;
		int enable = 1;
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&enable, sizeof(int)) < 0)
		{
			perror("error setsockopt SO_REUSEADDR");
		}

		s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0)
		{
			/* We managed to bind successfully! */
			printf_address(sfd, rp->ai_addr, rp->ai_addrlen, "Listen on");
			break;
		}
		close(sfd);
	}

	if (rp == NULL)
	{
		printf("Could not bind\n");
		sfd = INVALID_SOCKET;
	}

	freeaddrinfo(result);

	return sfd;
}

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const char *port, const char *address, bool reuseport)
  : loop_(loop),
    acceptSocket_(create_and_bind(port, address)),
    acceptChannel_(loop, acceptSocket_),
    listenning_(false)
{
  assert(acceptSocket_ >= 0);
  make_socket_non_blocking(acceptSocket_);
  if(reuseport) make_socket_reusable(acceptSocket_);
  acceptChannel_.setReadCallback(boost::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void Acceptor::listen()
{
  loop_->assertInLoopThread();
  listenning_ = true;
  int r = ::listen(acceptSocket_, SOMAXCONN);
  assert(r != -1);
  acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
  loop_->assertInLoopThread();

  struct sockaddr in_addr;
  socklen_t in_len;
  int connfd;
  in_len = sizeof in_addr;
  connfd = accept(acceptSocket_, &in_addr, &in_len);
  if (connfd >= 0)
  {
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (newConnectionCallback_)
    {
      newConnectionCallback_(connfd, in_addr, in_len);
    }
    else
    {
      ::close(connfd);
    }
  }
  else
  {
    std::cout << "in Acceptor::handleRead" << std::endl;
  }
}

