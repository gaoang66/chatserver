#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <map>

using json = nlohmann::json;  // 添加命名空间别名

std::string func1(){
    json js;  // 直接使用 json 类型
    js["msg_type"] = 2;
    js["from"] = "zhangsan";
    js["to"] = "lisi";
    js["msg"] = "hello!";
    std::cout << js.dump() << std::endl;  // 使用 dump() 输出格式化的 JSON
    return js.dump();
}

int main(){  
    std::string st= func1();
    json buf = json::parse(st);
    std::cout << buf["msg_type"] << std::endl;
    return 0;
}
