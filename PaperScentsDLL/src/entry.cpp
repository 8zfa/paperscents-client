#include "entry.h"
#include "core/core.h"

DWORD WINAPI InitThread(LPVOID lpParam)
{
    Core::GetInstance().Init((HMODULE)lpParam);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved)
{
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, InitThread, hModule, 0, nullptr);
        if (hThread)
            CloseHandle(hThread);
    }
    else if (ulReason == DLL_PROCESS_DETACH)
    {
        Core::GetInstance().Shutdown();
    }
    return TRUE;
}
