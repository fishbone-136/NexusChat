#include "ChatService.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
#include "AIService.h"

using namespace std;
using namespace muduo;

// 服务器端时间辅助函数
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
};

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREAT_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
        // AI相关业务注册
    _msgHandlerMap.insert({AI_CHAT_MSG, std::bind(&ChatService::aiChat, this, _1, _2, _3)});

    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
};

// 服务器异常，业务重置方法
void ChatService::reset()
{
    _userModel.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登陆业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);

    if (user.getId() == id && user.getPwd() == pwd)
    {

        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已登陆";
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登陆成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 登陆成功
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询用户离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，把该用户的离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            // 查询好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> vec3;
                for (Group &group : groupVec)
                {
                    json grpjs;
                    grpjs["id"] = group.getId();
                    grpjs["groupname"] = group.getName();
                    grpjs["groupdesc"] = group.getDesc();

                    vector<string> userVec;
                    for (GroupUser &user : group.getUsers())
                    {
                        json userjs;
                        userjs["id"] = user.getId();
                        userjs["name"] = user.getName();
                        userjs["state"] = user.getState();
                        userjs["role"] = user.getRole();
                        userVec.push_back(userjs.dump());
                    }
                    grpjs["user"] = userVec;
                    vec3.push_back(grpjs.dump());
                }
                response["groups"] = vec3;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 密码错误，登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "密码错误";
        conn->send(response.dump());
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理客户端异常推出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 从MAP中删除用户连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    _redis.unsubscribe(user.getId());

    // 更新用户状态
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// // 一对一聊天业务

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    std::string msg = js["msg"].get<std::string>();
    int userid = js["id"].get<int>();
    int toid = js["to"].get<int>();

    // 1. 内容审核
    if (!checkContentModeration(msg))
    {
        json response;
        response["msgid"] = MODERATION_FAIL_MSG;
        response["id"] = 0;
        response["name"] = "系统";
        response["msg"] = "消息包含违规内容，已被拦截！";
        response["time"] = getCurrentTime();
        conn->send(response.dump());
        return;
    }

    // 2. AI助手触发检测
    std::string lowerMsg = msg;
    std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);
    
    if (lowerMsg.find("@ai") != std::string::npos || lowerMsg.find("/ask") == 0)
    {
        json aiJs = js;
        aiJs["msgid"] = AI_CHAT_MSG;
        aiChat(conn, aiJs, time);
        return;
    }

    // 3. 正常消息处理
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
    }

    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    _offlineMsgModel.insert(toid, js.dump());
}

// void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
// {
//     int toid = js["to"].get<int>();

//     {
//         lock_guard<mutex> lock(_connMutex);
//         auto it = _userConnMap.find(toid);
//         if (it != _userConnMap.end())
//         {
//             // toid在线，转发消息  服务器主动推送消息给toid用户
//             it->second->send(js.dump());
//             return;
//         }
//     }

//     User user = _userModel.query(toid);
//     if (user.getState() == "online")
//     {
//         _redis.publish(toid, js.dump());
//         return;
//     }

//     // toid不在线，存储离线消息
//     _offlineMsgModel.insert(toid, js.dump());
// }

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    std::string msg = js["msg"].get<std::string>();
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    // 内容审核（简单版）
    if (!checkContentModeration(msg)) {
        json rsp; rsp["msgid"] = MODERATION_FAIL_MSG;
        rsp["msg"] = "⚠️ 消息包含违规内容，已被拦截！";
        conn->send(rsp.dump());
        return;
    }

    std::string lowerMsg = msg;
    std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);

    // AI 触发
    if (lowerMsg.find("@ai") != std::string::npos || lowerMsg.find("/ask") == 0) {
        std::string aiReply = AIService::instance()->processAIRequest(msg, userid, groupid);
        
        json aiMsg;
        aiMsg["msgid"] = GROUP_CHAT_MSG;
        aiMsg["id"] = 0;
        aiMsg["name"] = "AI助手";
        aiMsg["groupid"] = groupid;
        aiMsg["msg"] = aiReply;
        aiMsg["time"] = getCurrentTime();

        // **关键修复**：广播给群内所有人
        vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
        useridVec.push_back(userid);  // 包含发送者

        lock_guard<mutex> lock(_connMutex);
        for (int id : useridVec) {
            auto it = _userConnMap.find(id);
            if (it != _userConnMap.end()) {
                it->second->send(aiMsg.dump());
            }
        }
        return;
    }

    // 摘要触发
    if (lowerMsg.find("/summary") == 0) {
        handleGroupSummary(conn, js, time);
        return;
    }

    // 正常群消息转发（原有逻辑）
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec) {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) {
            it->second->send(js.dump());
        } else {
            // 离线逻辑保持不变
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}

// void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
// {
//     int userid = js["id"].get<int>();
//     int groupid = js["groupid"].get<int>();
//     vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

//     lock_guard<mutex> lock(_connMutex);
//     for (int id : useridVec)
//     {
//         auto it = _userConnMap.find(id);
//         if (it != _userConnMap.end())
//         {
//             // 转发群组消息
//             it->second->send(js.dump());
//         }
//         else
//         {
//             User user = _userModel.query(id);
//             if (user.getState() == "online")
//             {
//                 _redis.publish(id, js.dump());
//                 return;
//             }
//             else
//             {
//                 // 存储离线群消息
//                 _offlineMsgModel.insert(id, js.dump());
//             }
//         }
//     }
// }

void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    _offlineMsgModel.insert(userid, msg);
}


//AI 相关业务

// 内容审核
bool ChatService::checkContentModeration(const std::string& msg)
{
    return AIService::instance()->moderateContent(msg);
}

// AI聊天业务
void ChatService::aiChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    std::string userMessage = js["msg"].get<std::string>();

    // 调用AI处理
    std::string aiReply = AIService::instance()->processAIRequest(userMessage, userid);

    // 构造回复消息
    json response;
    response["msgid"] = ONE_CHAT_MSG;
    response["id"] = 0;                 
    response["name"] = "AI助手";
    response["to"] = userid;
    response["msg"] = aiReply;
    response["time"] = getCurrentTime();

    conn->send(response.dump());
}

// 群消息摘要
void ChatService::handleGroupSummary(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int groupid = js["groupid"].get<int>();

    std::string history = "【群聊历史记录】\n";
    int msgCount = 0;

    // 直接从当前用户离线消息中提取文本（最安全）
    vector<string> msgs = _offlineMsgModel.query(js["id"].get<int>());

    for (const auto& raw : msgs) {
        if (raw.find("groupid") != std::string::npos && raw.find(std::to_string(groupid)) != std::string::npos) {
            // 简单提取有用部分
            size_t pos = raw.find("msg\":\"");
            if (pos != std::string::npos) {
                std::string content = raw.substr(pos + 6);
                pos = content.find("\"");
                if (pos != std::string::npos) {
                    content = content.substr(0, pos);
                }
                if (content.length() > 80) content = content.substr(0, 80) + "...";
                history += "• " + content + "\n";
                msgCount++;
                if (msgCount >= 10) break;
            }
        }
    }

    if (msgCount == 0) {
        history += "（当前群暂无可提取的历史消息）";
    }

    std::string summary = AIService::instance()->generateSummary(history);

    json response;
    response["msgid"] = GROUP_CHAT_MSG;
    response["id"] = 0;
    response["name"] = "AI助手";
    response["groupid"] = groupid;
    response["msg"] = "【群聊摘要】\n" + summary + "\n\n（共参考 " + std::to_string(msgCount) + " 条消息）";
    response["time"] = getCurrentTime();

    // 广播给群内成员
    vector<int> groupUsers = _groupModel.queryGroupUsers(0, groupid);
    groupUsers.push_back(js["id"].get<int>());

    lock_guard<mutex> lock(_connMutex);
    for (int id : groupUsers) {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) {
            it->second->send(response.dump());
        }
    }

    LOG_INFO << "群[" << groupid << "] 摘要完成，共 " << msgCount << " 条消息";
}