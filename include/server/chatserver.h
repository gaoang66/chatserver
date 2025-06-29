#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;
#include <string>

class ChatServer
{
private:
    TcpServer _server; //muduo中的服务类对象
    muduo::net::EventLoop *_loop; //muduo库中的指向事件循环的指针
    //连接回调
    void onConnection(const TcpConnectionPtr&);
    //消息回调
    void onMessage(const TcpConnectionPtr&,
                    Buffer*,
                    Timestamp);

public:
    // 初始化对象
    ChatServer(muduo::net::EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    //启动服务
    void start();
};
