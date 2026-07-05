#pragma once
#include "window/window.h"

extern Window* g_Window;

class Core {
public:
    bool Init(HINSTANCE hInstance);
    void Run();
    void Shutdown();
private:
    Window m_Window;
    HINSTANCE m_hInstance;
};
