
对muduo的学习记录

## v0.1

该版本是对第八章的总结，代码是从muduo源码中提取出来的，有一些小改动。该版本主要用于理解作者的框架。
其中EventLoop::loop() 是最主要的循环，它使用poller获取就绪的channel列表，然后调用每个channel的handleEvent来处理发生的事件。
这样用户就只需要写好回调函数，然后将channel注册到poller中就可以了。在关心的事件发生时，EventLoop会自动调用回调函数。

Channel是将fd与事件相关回调函数联系起来的类。

Poller是IO multiplexing的抽象类，EPollPoller是基于epoll的实现。负责事件的注册、事件的查询。

