#ifndef __USER_H__
#define __USER_H__
#include <cstdio>
#include <string>
#include <tuple>
#include <unordered_map>
#include "util.hpp"

//用户类
class User {
    // usrename -> <uid , password>
    std::unordered_map<std::string, std::pair<int, std::string>> userlist;

   public:
    User(const char* userPath) {
        FILE* fp = fopen(userPath, "r");
        //空文件指针，报错退出
        if (fp == nullptr) {
            perror("fopen");
            exit(1);
        }
        char buf[1024];
        while (fgets(buf, 1024, fp) != nullptr) {
            auto com = split(buf, " ");
            if (com.size() != 3) {
                continue;
            }
            if (com[2].back() == '\n') {
                com[2].pop_back();
            }
            // 0 uid , 1 user ,2 password
            userlist[com[1]] = std::make_pair(atoi(com[0].c_str()), com[2]);
        }
        fclose(fp);
    }

    int login(std::string user, std::string pw) {
        auto it = userlist.find(user);
        if (it == userlist.end()) {
            return -1;
        }
        if (it->second.second == pw) {
            return it->second.first;
        }
        return -1;
    }
};

#endif