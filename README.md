# NexusChat - 高性能分布式集群即时通讯系统

一个基于 **muduo** 网络库的高性能分布式聊天服务器，支持集群部署，并集成 **DeepSeek LLM Agent**，实现智能对话功能。

---

## 项目亮点

- **高性能网络层**：基于 muduo Reactor 多线程模型，单机支持高并发连接
- **真正集群架构**：Nginx TCP 负载均衡 + Redis Pub/Sub 实现跨服务器消息实时同步
- **AI 智能增强**：集成 DeepSeek 大模型，支持 `@AI` 智能助手
- **完整 IM 功能**：注册登录、添加好友、创建群组、一对一/群聊、离线消息
- **智能特性**：
  - 内容审核（Moderation）
  - 群消息智能摘要（Summary）

---

## 功能演示

- **AI 智能助手**：在私聊或群聊中输入 `@AI 你的问题` 即可触发
- **群消息摘要**：群聊中输入 `/summary` 可生成当前群聊摘要
- **集群支持**：多台 ChatServer 通过 Redis 实现消息互通

---

## 技术栈

- **核心框架**：C++11 + muduo
- **数据库**：MySQL（持久化） + Redis（缓存 + Pub/Sub）
- **负载均衡**：Nginx TCP 负载均衡
- **AI 集成**：DeepSeek API + cpp-httplib
- **序列化**：nlohmann/json
- **构建工具**：CMake

---

## 快速开始

### 1. 环境准备

```bash
# 安装依赖（Ubuntu/Debian）
sudo apt update
sudo apt install mysql-server redis-server nginx cmake g++ libmysqlclient-dev libhiredis-dev libssl-dev

# 启动 MySQL 和 Redis
sudo systemctl start mysql redis-server
