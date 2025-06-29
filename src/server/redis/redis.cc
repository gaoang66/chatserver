#include "redis.h"
#include <iostream>
#include <thread>


Redis::Redis():_pubilsh_context(nullptr), _subscribe_context(nullptr){

}
Redis::~Redis(){
    if(_pubilsh_context!=nullptr){
        redisFree(_pubilsh_context);
    }
    if(_subscribe_context!=nullptr){
        redisFree(_subscribe_context);
    }
}

//连接redis
bool Redis::connect(){
    //负责publish消息的上下文连接
    _pubilsh_context=redisConnect("127.0.0.1", 6379);
    if(!_pubilsh_context){
        cerr << "connect redis failed!" << endl;
        return false;
    }

    //负责subscribe消息的上下文连接
    _subscribe_context=redisConnect("127.0.0.1", 6379);
    if(!_subscribe_context){
        cerr << "connect redis failed!" << endl;
        return false;
    }

    //在单独线程监听通道上的事件，有消息给业务层上报
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

//向redis指定通道channel发布消息
bool Redis::publish(int channel, string message){
    redisReply * reply =(redisReply*)redisCommand(_pubilsh_context, "PUBLISH %d %s", channel, message.c_str());
    if(!reply){
        cerr << "publish command failed!" << endl;
        return false;
    }

    freeReplyObject(reply);
    return true;
}

//向redis指定channel订阅消息
bool Redis::subscribe(int channel){
    if(REDIS_ERR == redisAppendCommand(_subscribe_context, "SUBSCRIBE %d", channel)){
        cerr << "subscribe command failed!";
        return false;
    }

    int done=0;
    while(!done){
        if(REDIS_ERR==redisBufferWrite(_subscribe_context, &done)){
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}

//取消订阅
bool Redis::unsubscribe(int channel){
    if(REDIS_ERR == redisAppendCommand(_subscribe_context, "UNSUBSCRIBE %d", channel)){
        cerr << "unsubscribe command failed!";
        return false;
    }

    int done=0;
    while(!done){
        if(REDIS_ERR==redisBufferWrite(_subscribe_context, &done)){
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}

//从独立线程接受订阅通道的消息
void Redis::observer_channel_message(){
    redisReply *reply=nullptr;
    while(REDIS_OK==redisGetReply(_subscribe_context, (void**)&reply)){
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
    
        }
        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

//初始化业务向service的回调函数
void Redis::init_notify_handler(function<void(int,string)> fn){
    this->_notify_message_handler = fn;
}
