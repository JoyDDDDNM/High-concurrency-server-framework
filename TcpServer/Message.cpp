#include "Message.hpp"

DataHeader::DataHeader() : length{ sizeof(DataHeader) }, cmd{ CMD_ERROR } {}

Login::Login() {
    DataHeader::length = sizeof(Login);
    cmd = CMD_LOGIN;
}

LoginRet::LoginRet() {
    length = sizeof(LoginRet);
    cmd = CMD_LOGIN_RESULT;
    result = 0;
}

Logout::Logout(){
    length = sizeof(Logout);
    cmd = CMD_LOGOUT;
}

LogoutRet::LogoutRet(){
    length = sizeof(LogoutRet);
    cmd = CMD_LOGOUT_RESULT;
    result = 0;
}

NewUserJoin::NewUserJoin(){
    length = sizeof(NewUserJoin);
    cmd = CMD_NEW_USER_JOIN;
    cSocket = 0;
}