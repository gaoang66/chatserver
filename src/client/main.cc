#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <unordered_map>
using json = nlohmann::json;
using namespace std;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.h"
#include "user.h"
#include "public.h"

//记录当前系统登陆的用户信息
User g_currentUser;

//记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;

//记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

//显示当前登陆成功用户的基本信息
void showCurrentUserData();

//接收线程
void readTaskHandler(int clientfd);

//获取系统时间
string getCurrentTime();

//主聊天程序
void mainMenu(int);

//全局变量
bool isMainMenuRunning=false;

int main(int argc, char** argv){
    if(argc<3){
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    //解析命令行 ip+port
    char * ip =argv[1];
    uint16_t port = atoi(argv[2]);

    //创建socket连接
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd==-1){
        cerr << "socket create error" << endl;
        exit(-1);
    }

    //填写连接server的ip和port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //client和server连接
    if(connect(clientfd, (sockaddr *) &server, sizeof(server))==-1){
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    while(true){
        //显示菜单
        cout << "====================================================" << endl;
        cout << "                      1.login                       " << endl;
        cout << "                      2.register                    " << endl;
        cout << "                      3.quit                        " << endl;
        cout << "====================================================" << endl;
        cout << "choice:";
        int choice=0;
        cin >> choice;
        cin.get();

        switch(choice)
        {
            case 1: //login
            {
                int id=0;;
                char password[50]={0};
                cout << "please input your id:";
                cin >> id;
                cin.get(); //清除输入缓冲区的换行符
                cout << "please input your password:";
                cin.getline(password, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = password;      
                string request = js.dump();     

                ssize_t ret = send(clientfd, request.c_str(), request.size(), 0);
                if(ret == -1)
                {
                    cerr << "send login msg error" << endl;
                    close(clientfd);
                    exit(-1);
                }
                else
                {
                    char buffer[1024] = {0};
                    ssize_t len = recv(clientfd, buffer, sizeof(buffer), 0);
                    if(len == -1){
                        cerr << "recv login msg error" << endl;
                        close(clientfd);
                        exit(-1);
                    } 
                    else
                    {
                        json response = json::parse(buffer);
                        if(response["errno"].get<int>() == 0)
                        {
                            //记录id和name
                            g_currentUser.setId(response["id"].get<int>());
                            g_currentUser.setName(response["name"].get<string>());
                            g_currentUser.setState("online");

                            //记录当前用户好友列表信息
                            if(response.contains("friends")){
                                // 初始化
                                g_currentUserFriendList.clear();
                                
                                vector<string> vec=response["friends"];
                                for(const string &str: vec){
                                    json js = json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"].get<string>());
                                    user.setState(js["state"].get<string>());
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            //记录当前用户群组列表信息
                            if(response.contains("groups"))
                            {
                                // 初始化
                                g_currentUserGroupList.clear();

                                vector<string> vec1=response["groups"];
                                for(const string &str: vec1)
                                {
                                    json js = json::parse(str);
                                    Group group;
                                    group.setId(js["id"].get<int>());
                                    group.setName(js["groupname"].get<string>());
                                    group.setDesc(js["groupdesc"].get<string>());
                                    
                                    vector<string> vec2=js["users"];
                                    for(const auto &usrstr : vec2)
                                    {
                                        json userJson = json::parse(usrstr);
                                        GroupUser user;
                                        user.setId(userJson["id"].get<int>());
                                        user.setName(userJson["name"].get<string>());
                                        user.setState(userJson["state"].get<string>());
                                        user.setRole(userJson["role"].get<string>());
                                        group.getUsers().push_back(user);
                                    }
                                    
                                    g_currentUserGroupList.push_back(group);
                                }
                            }
                            showCurrentUserData();

                            //显示离线的消息
                            if(response.contains("offlinemsg"))
                            {
                                vector<string> vec = response["offlinemsg"];
                                if(!vec.empty())
                                {

                                    cout << "you have " << vec.size() << " offline messages:" << endl;
                                    for(const string &msg : vec){
                                        json js = json::parse(msg);
                                        
                                        if(js["msgid"] == ONE_CHAT_MSG){
                                            cout << js["time"].get<string>() <<  " from " << js["id"].get<int>() << " " 
                                                << js["name"].get<string>() << ": "
                                                << js["msg"].get<string>() << endl;
                                        }
                                        else{
                                            cout << js["time"].get<string>() << " group chat from group id: " << js["groupid"].get<int>() 
                                                << ", user id: " << js["id"].get<int>() 
                                                << ", name: " << js["name"].get<string>() 
                                                << ", msg: " << js["msg"].get<string>() << endl;
                                        }
                                    }
                                }
                            }

                            //开启接收线程,只启动一次
                            static int readthreadnumber=0;
                            if(readthreadnumber==0){
                                thread readTask(readTaskHandler, clientfd);
                                readTask.detach(); //分离线程   
                            }
                            
                            isMainMenuRunning=true;
                            //进入聊天主菜单
                            mainMenu(clientfd);
                        }
                        else
                        {
                            cerr << id << " login failed, error: " << response["errmsg"] << endl;   
                        }
                    }
                }
                break;

            }
            case 2: //register
            {
                char name[50]={0};
                char password[50]={0};
                cout << "please input your name:";
                cin.getline(name, 50);
                cout << "please input your password:";
                cin.getline(password, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = password;      
                string request = js.dump();     

                ssize_t ret = send(clientfd, request.c_str(), request.size(), 0);
                if(ret == -1)
                {
                    cerr << "send register msg error" << endl;
                    close(clientfd);
                    exit(-1);
                }
                else
                {
                    char buffer[1024] = {0};
                    ssize_t len = recv(clientfd, buffer, sizeof(buffer), 0);
                    if(len == -1){
                        cerr << "recv register msg error" << endl;
                        close(clientfd);
                        exit(-1);
                    } 
                    else
                    {
                        json response = json::parse(buffer);
                        if(response["errno"].get<int>() == 0){
                            cout << name << " register success, your id is: " << response["id"] << endl;
                        }else{
                            cerr << name << " register failed, error: " << response["errmsg"] << endl;   
                        }
                    }
                }
                break;
            }
            case 3:
            {
                close(clientfd);
                cout << "bye bye" << endl;
                exit(0);
            }
            default:
            {
                cerr << "invalid input!" << endl;
                break;
            }
        }
    }

    return 0;
}


//显示当前登陆成功用户的基本信息
void showCurrentUserData(){
    cout << "====================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "--------------------friend list---------------------" << endl;
    if(!g_currentUserFriendList.empty()){
        for(User &user:g_currentUserFriendList){
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "--------------------group list----------------------" << endl;
    if(!g_currentUserGroupList.empty()){
        for(Group & group : g_currentUserGroupList){
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for(GroupUser & user: group.getUsers()){
                cout << user.getId() << " " << user.getName() << " " << user.getState() <<
                " " << user.getRole() << endl;
            }
        }
    }
    cout << "====================================================" << endl;
}

//接收线程
void readTaskHandler(int clientfd){
    while(true){
        char buffer[1024] = {0};
        ssize_t len = recv(clientfd, buffer, sizeof(buffer), 0);
        if(len == -1|| len == 0){
            cerr << "recv msg error" << endl;
            close(clientfd);
            exit(-1);
        } 
        else{
            json js = json::parse(buffer);
            //处理接收到的消息
            if(js["msgid"] == ONE_CHAT_MSG){
                cout << js["time"].get<string>() <<  " from " << js["id"].get<int>() << " " 
                     << js["name"].get<string>() << ": "
                     << js["msg"].get<string>() << endl;
            }
            else if(js["msgid"] == GROUP_CHAT_MSG){
                cout << js["time"].get<string>() << " group chat from group id: " << js["groupid"].get<int>() 
                     << ", user id: " << js["id"].get<int>() 
                     << ", name: " << js["name"].get<string>() 
                     << ", msg: " << js["msg"].get<string>() << endl;
            }
            else if(js["msgid"] == LOGIN_MSG_ACK){
                //更新好友状态
                for(User &user : g_currentUserFriendList){
                    if(user.getId() == js["id"].get<int>()){
                        user.setState(js["state"].get<string>());
                        break;
                    }
                }
                cout << "user id: " << js["id"].get<int>() 
                     << ", name: " << js["name"].get<string>() 
                     << ", state: " << js["state"].get<string>() 
                     << " is online now." << endl;
            }
        }
    }
}

//获取系统时间
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

void help(int fd=0, string str="");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

//系统支持的客户端命令列表
unordered_map<string, string> commandMap={
    {"help", "显示所有支持的命令, 格式help"},
    {"chat", "一对一聊天, 格式chat:friend:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"loginout", "注销, 格式loginout"}
};

//注册系统支持的客户端命令处理
unordered_map<string, function<void(int,string)>> commandHandlerMap={
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

//主聊天程序
void mainMenu(int clientfd){
    help();
    char buffer[1024]={0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;

        int idx=commandbuf.find(":");
        if(-1==idx){
            command=commandbuf;
        }
        else{
            command=commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it==commandHandlerMap.end()){
            cerr << "invalid input command!" << endl;
            continue;
        }

        it->second(clientfd, commandbuf.substr(idx+1, commandbuf.size()-idx));
    }
    

}

void help(int , string){
    cout << "show command list >>> " <<endl;
    for(auto &p:commandMap){
        cout << p.first << " : " << p.second << endl; 
    }
    cout << endl;
}

void addfriend(int clientfd, string str){
    int friendid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string buffer = js.dump();

    int len=send(clientfd, buffer.c_str(), buffer.size(), 0);
    if(-1==len){
        cerr << "send addfriend msg error" << buffer << endl;
    }
}

void chat(int clientfd, string str){
    int idx = str.find(":");
    if(-1==idx){
        cerr << "chat command invalid!" << endl;
        return ;
    }

    int friendid=atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx+1);

    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["toid"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if(len==-1){
        cerr << "send chat msg error -> " << buffer << endl;
    }

}


void creategroup(int clientfd, string str){
    int idx=str.find(":");
    if(-1==idx){
        cerr << "creategroup command invalid!" << endl;
        return ;
    }
    string groupname=str.substr(0,idx);
    string groupdesc=str.substr(idx+1);

    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string buffer=js.dump();

    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if(len==-1){
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

void addgroup(int clientfd, string str){
    int groupid=atoi(str.c_str());

    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;
    string buffer = js.dump();

    int len=send(clientfd, buffer.c_str(), buffer.size(), 0);
    if(-1==len){
        cerr << "send addgroup msg error" << buffer << endl;
    }

}

void groupchat(int clientfd, string str){
    int idx=str.find(":");
    if(-1==idx){
        cerr << "groupchat command invalid!" << endl;
        return ;
    }
    int groupid=atoi(str.substr(0,idx).c_str());
    string msg=str.substr(idx+1);

    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["groupid"]=groupid;
    js["msg"]=msg;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if(len==-1){
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}

void loginout(int clientfd, string str){
    json js;
    js["msgid"]=LOGIN_OUT_MSG;
    js["id"]=g_currentUser.getId();
    string buffer = js.dump();

    int len=send(clientfd, buffer.c_str(), buffer.size(), 0);
    if(-1==len){
        cerr << "send loginout msg error" << buffer << endl;
    }else{
        isMainMenuRunning=false;
    }

}
