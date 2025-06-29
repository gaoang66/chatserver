#pragma once
#include "group.h"
#include <vector>
#include <string>

class GroupModel{
public:
    //创建群组
    bool createGroup(Group &group);

    //加入群组
    void addGroup(int userid, int groupid, string role);

    //查询群组
    vector<Group> queryGroups(int userid);

    //群组中的发消息
    vector<int> queryGroupUsers(int userid, int groupid);
};