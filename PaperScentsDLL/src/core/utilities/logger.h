#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstdarg>

class Logger
{
public:
    static void Init()
    {
        AllocConsole();
        freopen_s(&s_File, "CONOUT$", "w", stdout);
        SetConsoleTitleA("PaperScents DLL");
        Log("Logger initialized");
    }

    static void Shutdown()
    {
        if (s_File)
        {
            fclose(s_File);
            s_File = nullptr;
        }
        FreeConsole();
    }

    static void Log(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        printf("[PaperScents] %s\n", buffer);
    }

private:
    static inline FILE* s_File = nullptr;
};
