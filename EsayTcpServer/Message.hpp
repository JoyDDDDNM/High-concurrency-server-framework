
#ifndef _MESSAGE_HEADER_HPP_
#define _MESSAGE_HEADER_HPP_
enum CMD {
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};

struct DataHeader {
    DataHeader() {
        length = sizeof(DataHeader);
        cmd = CMD_ERROR;
    }
    short length;
    short cmd;
};

struct Login : public DataHeader {
    Login() {
        length = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char password[32];
    char data[32];
};

struct LoginRet : public DataHeader {
    LoginRet() {
        length = sizeof(LoginRet);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
    }
    int result;
    char data[92];
};

struct Logout : public DataHeader {
    Logout() {
        length = sizeof(Logout);
        cmd = CMD_LOGOUT;
    }
    char userName[32];
};

struct LogoutRet : public DataHeader {
    LogoutRet() {
        length = sizeof(LogoutRet);
        cmd = CMD_LOGOUT_RESULT;
        result = 0;
    }
    int result;
};

struct NewUserJoin : public DataHeader {
    NewUserJoin() {
        length = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        cSocket = 0;
    }
    int cSocket;
};

#endif
