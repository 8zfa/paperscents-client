#include "javahook.h"
#include "../utilities/logger.h"

JavaHook& JavaHook::GetInstance()
{
    static JavaHook instance;
    return instance;
}

bool JavaHook::Initialize(JNIEnv* env)
{
    m_Env = env;
    Logger::Log("JavaHook initialized");
    return true;
}

void JavaHook::Shutdown()
{
    Logger::Log("JavaHook shutting down");
    m_Env = nullptr;
}
