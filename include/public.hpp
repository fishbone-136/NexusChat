#ifndef PUBLIC_H
#define PUBLIC_H
/*
server 和 client 的公共文件
*/
enum EnMsgType
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    LOGINOUT_MSG,
    REG_MSG,
    REG_MSG_ACK,
    ONE_CHAT_MSG,
    ADD_FRIEND_MSG,

    CREAT_GROUP_MSG,
    ADD_GROUP_MSG,
    GROUP_CHAT_MSG,

    AI_CHAT_MSG = 100,  
    MODERATION_FAIL_MSG,
    SUMMARY_MSG          

    
};

#endif