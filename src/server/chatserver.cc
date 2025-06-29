#include "chatserver.h"
#include <functional>
#include <string>
#include <nlohmann/json.hpp>
#include "chatservice.h"

using json=nlohmann::json;

//网络层

ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg):_server(loop, listenAddr, nameArg), _loop(loop)
{
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _server.setThreadNum(4);
}

void ChatServer::start(){
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn,
                    Buffer* buffer,
                    Timestamp time)
{
    string buf=buffer->retrieveAllAsString();
    //反序列化
    json js = json::parse(buf);

    //通过js["msgid"]获取 ==》 业务handler ==》conn js time 
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js,time);
}


