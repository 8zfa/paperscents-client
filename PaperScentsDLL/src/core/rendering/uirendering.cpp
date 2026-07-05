#include "uirendering.h"
#include "../utilities/logger.h"
#include "../modulehandler/modulehandler.h"

#include <imgui.h>
#include <string>

UIRendering& UIRendering::GetInstance()
{
    static UIRendering instance;
    return instance;
}

void UIRendering::Initialize()
{
    Logger::Log("UIRendering initializing...");
    m_Initialized = true;
}

void UIRendering::Render()
{
    if (!m_Initialized) return;

    ModuleHandler::GetInstance().OnRender();
}

void UIRendering::Shutdown()
{
    m_Initialized = false;
    Logger::Log("UIRendering shutdown");
}
