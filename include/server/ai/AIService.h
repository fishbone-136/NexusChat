#ifndef AISERVICE_H
#define AISERVICE_H

#include <string>
#include "LLMClient.h"

class AIService {
public:
    static AIService* instance();
    
    std::string processAIRequest(const std::string& userMessage, int userid = 0, int chatid = 0);
    
    // 内容审核
    bool moderateContent(const std::string& message);
    
    // 生成群消息摘要
    std::string generateSummary(const std::string& history);

private:
    AIService();
    LLMClient llmClient_;
};

#endif