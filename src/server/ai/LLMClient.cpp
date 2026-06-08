// #include "LLMClient.h"
// #include <iostream>
// #include <muduo/base/Logging.h>

// LLMClient::LLMClient() {
//     // 可从环境变量读取
//     const char* key = getenv("DEEPSEEK_API_KEY");
//     if (key) {
//         apiKey_ = key;
//     }
// }

// void LLMClient::setApiKey(const std::string& key) {
//     apiKey_ = key;
// }

// std::string LLMClient::chatCompletion(const std::string& prompt, 
//                                       const std::string& systemPrompt,
//                                       int maxTokens, float temperature) {
//     if (apiKey_.empty()) {
//         LOG_ERROR << "DeepSeek API Key is empty!";
//         return "错误：API Key 未设置";
//     }

//     httplib::SSLClient cli(host_, 443);
//     cli.enable_server_certificate_verification(false);  // HTTPS

//     json messages = json::array();

//     if (!systemPrompt.empty()) {
//         messages.push_back({{"role", "system"}, {"content", systemPrompt}});
//     }
//     messages.push_back({{"role", "user"}, {"content", prompt}});

//     json body = {
//         {"model", "deepseek-v4-flash"},     
//         {"messages", messages},
//         {"max_tokens", maxTokens},
//         {"temperature", temperature},
//         {"stream", false}
//     };

//     httplib::Headers headers = {
//         {"Authorization", "Bearer " + apiKey_},
//         {"Content-Type", "application/json"}
//     };

//     auto res = cli.Post(path_.c_str(), headers, body.dump(), "application/json");

//     if (res && res->status == 200) {
//         try {
//             json resp = json::parse(res->body);
//             return resp["choices"][0]["message"]["content"].get<std::string>();
//         } catch (...) {
//             return "解析 AI 回复失败";
//         }
//     } else {
//         LOG_ERROR << "DeepSeek API 请求失败: " << (res ? std::to_string(res->status) : "无响应");
//         return "AI 服务暂时不可用，请稍后重试";
//     }
// }


#include "LLMClient.h"
#include <muduo/base/Logging.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

LLMClient::LLMClient() {
    const char* key = getenv("DEEPSEEK_API_KEY");
    if (key) apiKey_ = key;
}

void LLMClient::setApiKey(const std::string& key) { apiKey_ = key; }

std::string LLMClient::chatCompletion(const std::string& prompt, 
                                      const std::string& systemPrompt,
                                      int maxTokens, float temperature) {
    if (apiKey_.empty()) return "API Key 未设置";

    httplib::SSLClient cli(host_);
    cli.enable_server_certificate_verification(false);

    json messages = json::array();
    if (!systemPrompt.empty()) messages.push_back({{"role", "system"}, {"content", systemPrompt}});
    messages.push_back({{"role", "user"}, {"content", prompt}});

    json body = {
        {"model", "deepseek-v4-flash"},
        {"messages", messages},
        {"max_tokens", maxTokens},
        {"temperature", temperature},
        {"stream", false}
    };

    httplib::Headers headers = {
        {"Authorization", "Bearer " + apiKey_},
        {"Content-Type", "application/json"}
    };

    auto res = cli.Post(path_.c_str(), headers, body.dump(), "application/json");

    if (res && res->status == 200) {
        try {
            json resp = json::parse(res->body);
            std::string content = resp["choices"][0]["message"]["content"].get<std::string>();
            LOG_INFO << "AI 回复长度: " << content.length();
            return content;
        } catch (const std::exception& e) {
            LOG_ERROR << "JSON 解析失败: " << e.what();
            return "AI 返回内容解析失败，请重试";
        }
    } 
    LOG_ERROR << "API 请求失败";
    return "AI 服务暂时不可用，请稍后重试";
}