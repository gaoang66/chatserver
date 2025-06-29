#pragma once
#include <hiredis/hiredis.h>
#include <functional>
using namespace std;
#include <string>

class Redis{
public:
    Redis();
    ~Redis();

    //连接redis
    bool connect();

    //向redis指定通道channel发布消息
    bool publish(int channel, string message);

    //向redis指定channel订阅消息
    bool subscribe(int channel);

    //取消订阅
    bool unsubscribe(int channel);

    //从独立线程接受订阅通道的消息
    void observer_channel_message();

    //初始化业务向service的回调函数
    void init_notify_handler(function<void(int,string)>);

private:
    //hiredis同步上下文对象，负责publish
    redisContext * _pubilsh_context;

    //hiredis同步上下文对象，负责subscribe
    redisContext * _subscribe_context;

    //回调函数，给service上报
    function<void(int, string)> _notify_message_handler;
};