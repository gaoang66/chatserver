#pragma once
#include "user.h"

class GroupUser:public User{
public:
    void setRole(string role) {this->role=role;}
    string getRole() {return this->role;}

private:
    string role;
};