#include "modulehandler.h"
#include "../core.h"
#include "../utilities/logger.h"

ModuleHandler& ModuleHandler::GetInstance()
{
    static ModuleHandler instance;
    return instance;
}

void ModuleHandler::Initialize()
{
    Logger::Log("ModuleHandler initializing...");
    m_Initialized = true;
}

void ModuleHandler::Shutdown()
{
    Logger::Log("ModuleHandler shutting down...");
    for (auto* module : m_Modules)
    {
        if (module->IsEnabled())
            module->SetEnabled(false);
    }
    m_Modules.clear();
    m_Initialized = false;
}

void ModuleHandler::RegisterModule(ModuleBase* module)
{
    if (!module) return;
    m_Modules.push_back(module);
    Logger::Log("Registered module: %s [%s]", module->GetName().c_str(), module->GetCategoryName());
}

void ModuleHandler::OnUpdate()
{
    if (!m_Initialized) return;
    auto* java = Core::GetInstance().GetJava();
    JNIEnv* env = (java && java->IsValid()) ? java->GetEnv() : nullptr;
    if (env) env->ExceptionClear();
    for (auto* module : m_Modules)
    {
        if (module->IsEnabled())
        {
            if (env) env->ExceptionClear();
            module->OnUpdate();
        }
    }
}

void ModuleHandler::OnRender()
{
    if (!m_Initialized) return;
    auto* java = Core::GetInstance().GetJava();
    JNIEnv* env = (java && java->IsValid()) ? java->GetEnv() : nullptr;
    if (env) env->ExceptionClear();
    for (auto* module : m_Modules)
    {
        if (module->IsEnabled())
        {
            module->OnRender();
        }
    }
}

ModuleBase* ModuleHandler::GetModule(const std::string& name)
{
    for (auto* module : m_Modules)
    {
        if (module->GetName() == name)
            return module;
    }
    return nullptr;
}
