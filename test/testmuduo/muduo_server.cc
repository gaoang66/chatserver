#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
using namespace std;

class ChatServer{
public:
    ChatServer(muduo::net::EventLoop* loop, //事件循环
        const muduo::net::InetAddress& listenAddr, //ip+port
        const string& nameArg):_server(loop, listenAddr, nameArg), _loop(loop)
    {
        //连接回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
        
        //读写回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3));
        
        _server.setThreadNum(4);
    }

    void run(){
        _server.start();
        _loop->loop();
    }
private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn){
        cout << "new connection: " << conn->peerAddress().toIpPort() << endl;
        cout << "new connection: " << conn->localAddress().toIpPort() << endl;
        
        if(!conn->connected()){
            conn->shutdown(); //断开连接
        }
    }
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                        muduo::net::Buffer* buffer,
                        muduo::Timestamp receiveTime)
        {
            string buf = buffer->retrieveAllAsString(); //获取数据
            conn->send(buf);
        }
    muduo::net::TcpServer _server;
    muduo::net::EventLoop *_loop;
};

int main(){
    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.run();
    return 0;
}