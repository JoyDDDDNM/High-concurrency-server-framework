
#ifndef _MESSAGE_HEADER_HPP_
#define _MESSAGE_HEADER_HPP_

#include <memory>

enum CMD {
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};

struct DataHeader {
    DataHeader();
    short length;
    short cmd;
};

struct Login : public DataHeader {
    Login();
    char userName[32];
    char password[32];
    char data[32];
};

struct LoginRet : public DataHeader {
    LoginRet();
    int result;
    char data[92];
};

struct Logout : public DataHeader {
    Logout();
    char userName[32];
};

struct LogoutRet : public DataHeader {
    LogoutRet();
    int result;
};

struct NewUserJoin : public DataHeader {
    NewUserJoin();
    int cSocket;
};

using DataHeaderPtr = std::shared_ptr<DataHeader>;

#endif
