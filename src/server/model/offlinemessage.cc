#include "offlinemessage.h"
#include "db.h"
using namespace std;

void OfflineMsgModel::insert(int userid, string msg){
    char sql[1024]={0};
    snprintf(sql, sizeof(sql), "insert into offlinemessage values('%d', '%s')",
            userid, msg.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }

}

//删除离线消息
void OfflineMsgModel::remove(int userid){
    char sql[1024]={0};
    snprintf(sql, sizeof(sql), "delete from offlinemessage where userid=%d",
            userid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//查询离线消息
vector<string> OfflineMsgModel::query(int userid){
    char sql[1024]={0};
    snprintf(sql, sizeof(sql), "select message from offlinemessage where userid=%d",
            userid);

    MySQL mysql;
    vector<string> vec;
    if(mysql.connect()){
        MYSQL_RES * res= mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row= mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}