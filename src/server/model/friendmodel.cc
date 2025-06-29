#include <friendmodel.h>
#include <db.h>
#include <iostream>

void FriendModel::insert(int userid, int friendid){
    char sql[1024]={0};
    snprintf(sql, sizeof(sql), "insert into friend values('%d', '%d')",
            userid, friendid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }else{
        cout << "加好友出错" << endl;
    }
}

//查询用户的好友列表
vector<User> FriendModel::query(int userid){
    char sql[1024]={0};
    snprintf(sql, sizeof(sql), 
        "select a.id, a.name, a.state from user a inner join friend b on a.id=b.friendid where b.userid='%d'", 
        userid);

    MySQL mysql;
    vector<User> vec;
    if(mysql.connect()){
        MYSQL_RES *res=mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}