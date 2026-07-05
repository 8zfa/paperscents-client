#pragma once
#include <Windows.h>

class Time
{
public:
    static void Init()
    {
        s_StartTime = GetTickCount64();
    }

    static long long GetCurrentTimeMs()
    {
        return GetTickCount64() - s_StartTime;
    }

    static double GetDeltaTime()
    {
        long long now = GetTickCount64();
        double delta = (double)(now - s_LastFrame) / 1000.0;
        s_LastFrame = now;
        return delta;
    }

    static void SleepMs(DWORD ms)
    {
        Sleep(ms);
    }

private:
    static inline long long s_StartTime = 0;
    static inline long long s_LastFrame = 0;
};
