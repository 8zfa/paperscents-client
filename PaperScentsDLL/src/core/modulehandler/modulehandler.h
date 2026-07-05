#pragma once
#include <string>
#include <vector>
#include <memory>
#include "modulebase.h"

class ModuleHandler
{
public:
    static ModuleHandler& GetInstance();

    void Initialize();
    void Shutdown();

    void RegisterModule(ModuleBase* module);
    void OnUpdate();
    void OnRender();

    std::vector<ModuleBase*>& GetModules() { return m_Modules; }
    ModuleBase* GetModule(const std::string& name);

private:
    ModuleHandler() = default;
    ~ModuleHandler() = default;
    ModuleHandler(const ModuleHandler&) = delete;
    ModuleHandler& operator=(const ModuleHandler&) = delete;

    std::vector<ModuleBase*> m_Modules;
    bool m_Initialized = false;
};
