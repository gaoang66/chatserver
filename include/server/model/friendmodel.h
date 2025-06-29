#pragma once

#include "user.h"
#include <vector>

//好友表的数据操作类
class FriendModel{
public:
    //添加好友
    void insert(int userid, int friendid);
    //删除好友
    bool remove(int userid, int friendid);
    //查询用户的好友列表
    vector<User> query(int userid);
};
