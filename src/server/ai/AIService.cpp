#include "AIService.h"
#include <muduo/base/Logging.h>

AIService* AIService::instance() {
    static AIService service;
    return &service;
}

AIService::AIService() {
    // 这里可以读取环境变量设置 Key
    const char* key = getenv("DEEPSEEK_API_KEY");
    if (key) {
        llmClient_.setApiKey(key);
        LOG_INFO << "DeepSeek API Key 已加载";
    } else {
        LOG_WARN << "未设置 DEEPSEEK_API_KEY 环境变量";
    }
}

bool AIService::moderateContent(const std::string& message) {
    std::string prompt = "请判断以下聊天消息是否包含违规内容（政治敏感、色情、暴力、广告等）。只回答“是”或“否”：\n" + message;
    std::string result = llmClient_.chatCompletion(prompt, "", 200, 0.3);
    
    return result.find("否") != std::string::npos || result.find("安全") != std::string::npos;
}

std::string AIService::processAIRequest(const std::string& userMessage, int userid, int chatid) {
    std::string systemPrompt = "你是群聊/私聊中的智能助手，请友好、专业、简洁地回答用户问题。";
    return llmClient_.chatCompletion(userMessage, systemPrompt, 1024, 0.7);
}

std::string AIService::generateSummary(const std::string& history) {
    std::string prompt = "请用简洁的中文总结以下群聊记录，突出重点和关键信息（控制在150字以内）：\n" + history;
    return llmClient_.chatCompletion(prompt, "", 512, 0.5);
}