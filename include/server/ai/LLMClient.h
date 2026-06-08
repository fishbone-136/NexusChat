#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <string>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <json.hpp>

using json = nlohmann::json;

class LLMClient {
public:
    LLMClient();
    
    // 调用 DeepSeek API
    std::string chatCompletion(const std::string& prompt, 
                               const std::string& systemPrompt = "",
                               int maxTokens = 1024,
                               float temperature = 0.7);

    // 设置 API Key（建议从环境变量读取）
    void setApiKey(const std::string& key);

private:
    std::string apiKey_;
    const std::string host_ = "api.deepseek.com";
    const std::string path_ = "/chat/completions";
};

#endif