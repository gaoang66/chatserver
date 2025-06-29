#pragma once

#include "user.h"

//User表的数据操作类
class UserModel{
public:
    //插入信息
    bool insert(User &user);
    //查询信息
    User query(int id);
    //更新信息
    bool updateState(User &user);
    //重置状态信息
    void resetState();

};