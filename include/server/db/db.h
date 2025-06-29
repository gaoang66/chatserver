#pragma once

#include <mysql/mysql.h>
#include <string>
#include <muduo/base/Logging.h>
using namespace std;

//数据库
class MySQL
{
public:
    MySQL();
    ~MySQL();

    bool connect();

    bool update(string sql);

    MYSQL_RES* query(string sql);

    MYSQL* getConnection();

private:
    MYSQL *_conn;
};