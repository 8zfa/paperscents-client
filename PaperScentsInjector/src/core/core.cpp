#include "core.h"
#include "config.h"
#include "logger.h"

Window* g_Window = nullptr;

bool Core::Init(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    g_Window = &m_Window;

    ConfigManager::Get().Load();
    Logger::Get().Info("PaperScents Injector v1.0");

    auto& cfg = ConfigManager::Get().GetConfig();
    WindowConfig wcfg;
    wcfg.Title = L"PaperScents";
    wcfg.Width = cfg.WindowW;
    wcfg.Height = cfg.WindowH;
    wcfg.X = cfg.WindowX;
    wcfg.Y = cfg.WindowY;

    if (!m_Window.Create(hInstance, wcfg)) {
        Logger::Get().Error("Failed to create window.");
        return false;
    }

    Logger::Get().Info("Ready. Select a DLL and scan for processes.");
    return true;
}

void Core::Run() {
    MSG msg = {};
    while (true) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { Shutdown(); return; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        m_Window.Render();
    }
}

void Core::Shutdown() {
    m_Window.Destroy();
    ConfigManager::Get().Save();
}
