#pragma once
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include "usermodel.h"
#include "offlinemessage.h"
#include <mutex>
#include "friendmodel.h"
#include "groupmodel.h"
#include "redis.h"

using json=nlohmann::json;
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)>;

class ChatService
{
private:
    ChatService();

    //存储消息id和对应的业务处理方法
    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    UserModel _userModel;

    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendModel;

    GroupModel _groupModel;

    //存储在线用户的通信连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> _userConnMap;

    //定义互斥锁
    mutex _connMutex;

    //redis对象
    Redis _redis;
    
public:
    //获取单例对象的接口函数
    static ChatService * instance();

    //登陆业务
    void login(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);

    //注册业务
    void reg(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);

    //一对一聊天业务
    void oneChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);

    //添加好友业务
    void addFriend(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);

    //创建群组业务
    void createGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);

    //加入群组业务
    void addGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);

    //群组聊天业务
    void groupChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);

    void loginout(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);


    //获取消息对应的处理器
    MsgHandler getHandler(int);

    //处理客户端异常退出
    void clientCloseException(const muduo::net::TcpConnectionPtr &conn);

    //重置服务器状态
    void reset();

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);
};
