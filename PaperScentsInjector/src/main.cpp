#include <Windows.h>
#include "core/core.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    Core core;
    if (!core.Init(hInstance))
        return 1;

    core.Run();
    return 0;
}
