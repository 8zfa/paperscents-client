#pragma once
#include <string>
#include <vector>
#include <Windows.h>

struct ProcessInfo {
    std::string Name;
    DWORD PID;
    std::string Status; // "Ready", "Injected", "Error"
};

class ProcessManager {
public:
    static std::vector<ProcessInfo> Scan();
    static bool Inject(DWORD pid, const std::string& dllPath, std::string& errorOut);
};
