#include "chatservice.h"
#include "public.h"
#include "offlinemessage.h"
#include <string>
#include <muduo/base/Logging.h>
#include <iostream>
#include <vector>

//·业务层

ChatService::ChatService(){
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    
    //处理群组消息
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    _msgHandlerMap.insert({LOGIN_OUT_MSG, std::bind(&ChatService::loginout, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    //连接redis服务器
    if(_redis.connect()){
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this,
        std::placeholders::_1, std::placeholders::_2));
    }
}
    

//获取单例对象的接口函数
ChatService* ChatService::instance(){
    static ChatService service;
    return &service;
}

//重置服务器状态
void ChatService::reset(){
    //将在线用户状态更新为离线
    _userModel.resetState();     
}

//登陆业务
void  ChatService::login(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
    int id=js["id"];
    cout << "login id: " << id << endl;
    string pwd = js["password"];
    User user=_userModel.query(id);
    cout << "login user: " << user.getId() << " " << user.getPassword() << endl;
    cout << "client user: " << id << " " << pwd << endl;
    if(user.getId()==id && user.getPassword()==pwd){


        if(user.getState()=="online"){

            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=2;
            response["err_msg"]="该账号已经登录，请输入新账号";
            conn->send(response.dump());
        }else{
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            //用户登陆成功后，向redis订阅
            _redis.subscribe(id);

            //更新状态
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=0;
            response["id"]=user.getId();
            response["name"]=user.getName();

            //查询离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty()){
                response["offlinemsg"]=vec;
                _offlineMsgModel.remove(id);
            }

            //查询好友列表
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty()){
                vector<string> friendVec;
                for(auto &friendUser : userVec){
                    json friendJson;
                    friendJson["id"]=friendUser.getId();
                    friendJson["name"]=friendUser.getName();
                    friendJson["state"]=friendUser.getState();
                    friendVec.push_back(friendJson.dump());
                }
                response["friends"]=friendVec;
            }

            //查询群组消息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());

        }
    }   
    else{
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=1;
        response["errmsg"]="用户名或密码错误";
        conn->send(response.dump());
    }
}

//注册业务
void  ChatService::reg(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
    string name=js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);

    if(state){
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["id"]=user.getId();
        response["errno"]=0;
        conn->send(response.dump());
    }
    else{
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());
    }
}

MsgHandler ChatService::getHandler(int msgid){
    auto it = _msgHandlerMap.find(msgid);
    if(it==_msgHandlerMap.end()){
        return [=](const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
            LOG_ERROR << "msgid: " << msgid << " can not find hanlder!";
        };

    }
    else{
        return _msgHandlerMap[msgid];
    }
}

void ChatService::clientCloseException(const muduo::net::TcpConnectionPtr &conn){
    User user;
    {
        lock_guard<mutex> lock(_connMutex);

        for(auto it = _userConnMap.begin(); it!=_userConnMap.end(); it++){
            if(it->second==conn){
                //删除连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;

            }
        }
    }

    _redis.unsubscribe(user.getId());

    //更新用户状态
    
   if(user.getId() != -1){
        user.setState("offline");
        _userModel.updateState(user);
    }
    
}


void ChatService::oneChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
    int toid = js["toid"].get<int>();
    
    {
        lock_guard<mutex> lock(_connMutex);
        auto it =_userConnMap.find(toid);
        if(it!=_userConnMap.end()){
            //在线，转发消息    服务器推送给toid
            it->second->send(js.dump());
            return;
        }
    }

    User user=_userModel.query(toid);
    if(user.getState()=="online"){
        _redis.publish(toid, js.dump());
        return;
    }

    _offlineMsgModel.insert(toid, js.dump());
}

void ChatService::addFriend(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
    int userid=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

//创建群组业务
void ChatService::createGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
    int userid=js["id"].get<int>();
    string name=js["groupname"];
    string desc=js["groupdesc"];

    Group group(-1, name,desc);
    if(_groupModel.createGroup(group)){
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

//加入群组业务
void ChatService::addGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

//群组聊天业务
void ChatService::groupChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int id:useridVec){

        auto it =_userConnMap.find(id);
        if(it!=_userConnMap.end()){
            it->second->send(js.dump());
        }
        else{
            User user =_userModel.query(id);

            if(user.getState()=="online"){
                _redis.publish(id, js.dump());
            }
            else{
                _offlineMsgModel.insert(id,js.dump());
            }
        }
    }
}

void ChatService::loginout(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time){

    int userid=js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(userid);

        if(it!=_userConnMap.end()){
            _userConnMap.erase(it);
        }
    }

    //用户注销，在redis取消订阅
    _redis.unsubscribe(userid);

    //更新用户状态
    
   User user(userid, "", "", "offline");
   _userModel.updateState(user);
}


void ChatService::handleRedisSubscribeMessage(int userid, string msg){
    // json js = json::parse(msg.c_str());

    lock_guard<mutex> lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end()){
        // it->second->send(js.dump());
        it->second->send(msg);
        return;
    }

    _offlineMsgModel.insert(userid, msg);
}


