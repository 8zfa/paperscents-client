#include "process.h"
#include <TlHelp32.h>
#include <set>
#include <algorithm>

std::vector<ProcessInfo> ProcessManager::Scan() {
    std::vector<ProcessInfo> results;
    std::set<std::string> seenNames;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return results;

    const wchar_t* targets[] = {
        L"javaw.exe", L"java.exe",
        L"Lunar Client.exe", L"Badlion Client.exe"
    };

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            for (const wchar_t* target : targets) {
                if (_wcsicmp(pe.szExeFile, target) == 0) {
                    char buf[MAX_PATH];
                    size_t converted = 0;
                    wcstombs_s(&converted, buf, pe.szExeFile, _TRUNCATE);
                    std::string name(buf);
                    std::string lower = name;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (seenNames.find(lower) == seenNames.end()) {
                        results.push_back({ name, pe.th32ProcessID, "Ready" });
                        seenNames.insert(lower);
                    }
                    break;
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return results;
}

bool ProcessManager::Inject(DWORD pid, const std::string& dllPath, std::string& errorOut) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
            errorOut = "Access denied. Run as Administrator.";
        else
            errorOut = "Failed to open process (error " + std::to_string(err) + ").";
        return false;
    }

    size_t pathSize = dllPath.size() + 1;
    void* remoteMem = VirtualAllocEx(hProcess, nullptr, pathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        errorOut = "Failed to allocate memory in target process.";
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), pathSize, nullptr)) {
        errorOut = "Failed to write DLL path to target process.";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLib) {
        errorOut = "Failed to locate LoadLibraryA.";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, loadLib, remoteMem, 0, nullptr);
    if (!hThread) {
        errorOut = "Failed to create remote thread.";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, 5000);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    errorOut = "";
    return true;
}
