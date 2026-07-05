#include <windows.h>
#include <windowsx.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <set>
#include <json.hpp>

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "shlwapi.lib")

using json = nlohmann::json;

// ============================================================
//  CONFIG
// ============================================================
struct Config {
    bool autoInject = false;
    std::string dllPath;
    int windowX = 100;
    int windowY = 100;
    int windowW = 750;
    int windowH = 550;
};

Config g_config;
#pragma warning(push)
#pragma warning(disable : 4996)
std::string g_configPath = std::string(getenv("APPDATA")) + "\\PaperScents\\config.json";
#pragma warning(pop)

// ============================================================
//  PROCESS INFO
// ============================================================
struct ProcessInfo {
    DWORD pid;
    std::wstring name;
    bool injected;
    std::string status;
};

std::vector<ProcessInfo> g_processes;
bool g_scanning = false;
bool g_needRefresh = true;
std::string g_statusText = "No target found";
ImVec4 g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);

// ============================================================
//  PROCESS SCANNING
// ============================================================
void ScanProcesses() {
    if (g_scanning) return;
    g_scanning = true;

    std::vector<ProcessInfo> newList;
    std::set<std::wstring> seenNames;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                std::wstring name = pe.szExeFile;
                if (_wcsicmp(name.c_str(), L"javaw.exe") == 0) {
                    if (seenNames.find(name) == seenNames.end()) {
                        ProcessInfo info;
                        info.pid = pe.th32ProcessID;
                        info.name = name;
                        info.injected = false;
                        info.status = "Ready";
                        newList.push_back(info);
                        seenNames.insert(name);
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }

    for (auto& newP : newList) {
        for (auto& oldP : g_processes) {
            if (oldP.pid == newP.pid) {
                newP.injected = oldP.injected;
                newP.status = oldP.status;
                break;
            }
        }
    }
    g_processes = std::move(newList);

    if (g_processes.empty()) {
        g_statusText = "No target found";
        g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
    } else {
        bool allInjected = true;
        for (auto& p : g_processes) {
            if (!p.injected) { allInjected = false; break; }
        }
        if (allInjected) {
            g_statusText = "All injected";
            g_statusColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
        } else {
            g_statusText = "Ready to inject";
            g_statusColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
        }
    }

    g_scanning = false;
    g_needRefresh = false;
}

// ============================================================
//  INJECTION
// ============================================================
bool InjectDLL(DWORD pid, const std::string& dllPath) {
    if (!PathFileExistsA(dllPath.c_str())) {
        g_statusText = "DLL not found!";
        g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        return false;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        g_statusText = "OpenProcess failed (run as admin?)";
        g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        return false;
    }

    size_t pathSize = dllPath.size() + 1;
    LPVOID remoteMem = VirtualAllocEx(hProc, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        CloseHandle(hProc);
        g_statusText = "VirtualAllocEx failed";
        g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        return false;
    }

    if (!WriteProcessMemory(hProc, remoteMem, dllPath.c_str(), pathSize, NULL)) {
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        g_statusText = "WriteProcessMemory failed";
        g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        return false;
    }

    LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, loadLib, remoteMem, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        g_statusText = "CreateRemoteThread failed";
        g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        return false;
    }

    WaitForSingleObject(hThread, 5000);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProc);

    if (exitCode == 0) {
        g_statusText = "LoadLibrary returned NULL";
        g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        return false;
    }

    g_statusText = "Injection successful!";
    g_statusColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
    return true;
}

// ============================================================
//  CONFIG LOAD/SAVE
// ============================================================
void LoadConfig() {
    std::ifstream f(g_configPath);
    if (f.is_open()) {
        json j;
        f >> j;
        g_config.autoInject = j.value("autoInject", false);
        g_config.dllPath = j.value("dllPath", "");
        g_config.windowX = j.value("windowX", 100);
        g_config.windowY = j.value("windowY", 100);
        g_config.windowW = j.value("windowW", 750);
        g_config.windowH = j.value("windowH", 550);
    }
}

void SaveConfig() {
    json j;
    j["autoInject"] = g_config.autoInject;
    j["dllPath"] = g_config.dllPath;
    j["windowX"] = g_config.windowX;
    j["windowY"] = g_config.windowY;
    j["windowW"] = g_config.windowW;
    j["windowH"] = g_config.windowH;

    std::string dir = g_configPath.substr(0, g_configPath.find_last_of("\\"));
    CreateDirectoryA(dir.c_str(), NULL);
    std::ofstream f(g_configPath);
    if (f.is_open()) {
        f << j.dump(4);
    }
}

// ============================================================
//  D3D9 HELPERS
// ============================================================
bool CreateD3D9Device(HWND hwnd, IDirect3D9* d3d, IDirect3DDevice9** device) {
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = hwnd;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.EnableAutoDepthStencil = FALSE;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    D3DDEVTYPE devTypes[] = { D3DDEVTYPE_HAL, D3DDEVTYPE_SW, D3DDEVTYPE_REF };
    DWORD vpFlags[] = {
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        D3DCREATE_MIXED_VERTEXPROCESSING,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING
    };
    D3DFORMAT bbFormats[] = { D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_R5G6B5 };
    D3DFORMAT depthFormats[] = { D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D32 };
    UINT presentIntervals[] = {
        D3DPRESENT_INTERVAL_ONE,
        D3DPRESENT_INTERVAL_IMMEDIATE,
        D3DPRESENT_INTERVAL_TWO,
        D3DPRESENT_INTERVAL_THREE,
        D3DPRESENT_INTERVAL_FOUR
    };

    for (UINT adapter = 0; adapter < d3d->GetAdapterCount(); ++adapter) {
        for (auto devType : devTypes) {
            for (auto vp : vpFlags) {
                for (auto bbFmt : bbFormats) {
                    pp.BackBufferFormat = bbFmt;
                    HRESULT hr = d3d->CreateDevice(adapter, devType, hwnd, vp, &pp, device);
                    if (SUCCEEDED(hr) && *device) {
                        return true;
                    }
                }
            }
        }
    }

    // Last resort: reference device
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, device);
    return SUCCEEDED(hr) && *device;
}

// ============================================================
//  WIN32 + IMGUI
// ============================================================
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LPDIRECT3DDEVICE9 g_resetDevice = nullptr;
static bool g_needReset = false;

static HWND g_mainHwnd = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_NCHITTEST:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);
        if (pt.y < 40) {
            RECT wc;
            GetClientRect(hWnd, &wc);
            if (pt.x < wc.right - 90) {
                return HTCAPTION;
            }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    case WM_SIZE:
        if (g_resetDevice && wParam != SIZE_MINIMIZED)
            g_needReset = true;
        return 0;
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    LoadConfig();

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"PaperScentsWindow";
    RegisterClassEx(&wc);

    DWORD winStyle = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    RECT wr = { 0, 0, g_config.windowW, g_config.windowH };
    AdjustWindowRectEx(&wr, winStyle, FALSE, WS_EX_TOPMOST | WS_EX_LAYERED);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"PaperScentsWindow",
        L"PaperScents Injector",
        winStyle,
        g_config.windowX, g_config.windowY,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInstance, NULL
    );
    if (!hwnd) return 1;
    g_mainHwnd = hwnd;

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    // D3D9
    LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        DestroyWindow(hwnd);
        UnregisterClass(L"PaperScentsWindow", hInstance);
        MessageBoxA(NULL, "Direct3D9 is not available on this system.", "PaperScents", MB_ICONERROR);
        return 1;
    }

    LPDIRECT3DDEVICE9 device = NULL;
    if (!CreateD3D9Device(hwnd, d3d, &device)) {
        d3d->Release();
        DestroyWindow(hwnd);
        UnregisterClass(L"PaperScentsWindow", hInstance);
        MessageBoxA(NULL, "Failed to create D3D9 device.\nTry updating your GPU drivers.", "PaperScents", MB_ICONERROR);
        return 1;
    }
    g_resetDevice = device;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.Fonts->AddFontDefault();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);

    // Style
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.18f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.42f, 0.39f, 1.0f, 0.8f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.48f, 0.45f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.28f, 0.60f, 0.8f);
    style.FrameRounding = 4.0f;
    style.WindowRounding = 8.0f;

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    ScanProcesses();

    MSG msg = {};
    auto lastScan = std::chrono::steady_clock::now();

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastScan).count() > 2000) {
            if (!g_scanning) {
                g_needRefresh = true;
                std::thread(ScanProcesses).detach();
            }
            lastScan = now;
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ===== UI =====
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("PaperScents", NULL,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings);

        ImVec2 winSize = ImGui::GetWindowSize();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Title bar
        draw->AddRectFilled(ImVec2(0, 0), ImVec2(winSize.x, 40), IM_COL32(30, 30, 46, 255));
        // Logo icon
        draw->AddRectFilled(ImVec2(8, 6), ImVec2(34, 34), IM_COL32(108, 99, 255, 255), 6);
        draw->AddRectFilled(ImVec2(11, 9), ImVec2(31, 31), IM_COL32(30, 30, 46, 255), 4);
        draw->AddText(ImVec2(14, 11), IM_COL32(108, 99, 255, 255), "PS");
        // Brand name
        draw->AddText(ImVec2(42, 9), IM_COL32(220, 220, 240, 255), "PaperScents");
        draw->AddText(ImVec2(158, 18), IM_COL32(136, 136, 170, 255), "v1.0");

        float btnY = 6.0f;
        float btnH = 28.0f, btnW = 28.0f;
        bool maximized = IsZoomed(g_mainHwnd);

        // Minimize
        ImGui::SetCursorPos(ImVec2(winSize.x - 86, btnY));
        if (ImGui::InvisibleButton("minimize", ImVec2(btnW, btnH)))
            ShowWindow(g_mainHwnd, SW_MINIMIZE);
        draw->AddRectFilled(ImVec2(winSize.x - 86, btnY), ImVec2(winSize.x - 86 + btnW, btnY + btnH), IM_COL32(255,255,255,20), 4);
        draw->AddLine(ImVec2(winSize.x - 73, btnY + 18), ImVec2(winSize.x - 65, btnY + 18), IM_COL32(200,200,200,255), 2);

        // Maximize / Restore
        ImGui::SetCursorPos(ImVec2(winSize.x - 56, btnY));
        if (ImGui::InvisibleButton("maximize", ImVec2(btnW, btnH)))
            ShowWindow(g_mainHwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
        draw->AddRectFilled(ImVec2(winSize.x - 56, btnY), ImVec2(winSize.x - 56 + btnW, btnY + btnH), IM_COL32(255,255,255,20), 4);
        if (maximized) {
            draw->AddRect(ImVec2(winSize.x - 46, btnY + 6), ImVec2(winSize.x - 38, btnY + 20), IM_COL32(200,200,200,255), 0, 0, 2);
            draw->AddRect(ImVec2(winSize.x - 48, btnY + 8), ImVec2(winSize.x - 40, btnY + 22), IM_COL32(200,200,200,255), 0, 0, 2);
        } else {
            draw->AddRect(ImVec2(winSize.x - 48, btnY + 6), ImVec2(winSize.x - 38, btnY + 20), IM_COL32(200,200,200,255), 0, 0, 2);
        }

        // Close
        ImGui::SetCursorPos(ImVec2(winSize.x - 28, btnY));
        if (ImGui::InvisibleButton("close", ImVec2(btnW, btnH)))
            PostQuitMessage(0);
        draw->AddRectFilled(ImVec2(winSize.x - 28, btnY), ImVec2(winSize.x - 28 + btnW, btnY + btnH), IM_COL32(255,255,255,20), 4);
        draw->AddLine(ImVec2(winSize.x - 22, btnY + 8), ImVec2(winSize.x - 12, btnY + 18), IM_COL32(200,200,200,255), 2);
        draw->AddLine(ImVec2(winSize.x - 22, btnY + 18), ImVec2(winSize.x - 12, btnY + 8), IM_COL32(200,200,200,255), 2);

        ImGui::SetCursorPosY(50);

        // Status
        ImGui::TextColored(g_statusColor, "o %s", g_statusText.c_str());
        ImGui::SameLine(winSize.x - 150);
        ImGui::Checkbox("Auto", &g_config.autoInject);

        ImGui::Separator();

        // Process list
        ImGui::BeginChild("procList", ImVec2(0, winSize.y - 180), true);
        if (g_processes.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No target process found.");
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Launch Minecraft to see the process.");
        } else {
            for (auto& proc : g_processes) {
                ImGui::PushID((int)proc.pid);
                int wlen = (int)proc.name.size();
                int mlen = WideCharToMultiByte(CP_UTF8, 0, proc.name.c_str(), wlen, NULL, 0, NULL, NULL);
                std::string label(mlen, 0);
                WideCharToMultiByte(CP_UTF8, 0, proc.name.c_str(), wlen, &label[0], mlen, NULL, NULL);
                label += "  (PID: " + std::to_string(proc.pid) + ")";
                ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
                ImGui::SameLine(winSize.x - 140);
                ImVec4 col = proc.injected ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                ImGui::TextColored(col, proc.injected ? "Injected" : "Ready");
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        // Bottom controls
        ImGui::Separator();
        if (ImGui::Button("Browse DLL", ImVec2(100, 30))) {
            OPENFILENAMEA ofn = {};
            char file[260] = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = "DLL files\0*.dll\0All files\0*.*\0";
            ofn.lpstrFile = file;
            ofn.nMaxFile = sizeof(file);
            ofn.Flags = OFN_FILEMUSTEXIST;
            if (GetOpenFileNameA(&ofn)) {
                g_config.dllPath = file;
                SaveConfig();
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s", g_config.dllPath.empty() ? "No DLL selected" : g_config.dllPath.c_str());

        ImGui::SameLine(winSize.x - 140);
        if (ImGui::Button("Inject", ImVec2(120, 40))) {
            if (g_processes.empty()) {
                g_statusText = "No target process found!";
                g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            } else if (g_config.dllPath.empty()) {
                g_statusText = "Please select a DLL first.";
                g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            } else {
                bool anySuccess = false;
                for (auto& p : g_processes) {
                    if (!p.injected) {
                        p.status = "Injecting...";
                        if (InjectDLL(p.pid, g_config.dllPath)) {
                            p.injected = true;
                            p.status = "Success";
                            anySuccess = true;
                        } else {
                            p.status = "Error";
                        }
                    }
                }
                if (!anySuccess && !g_processes.empty()) {
                    g_statusText = "Injection failed for all processes.";
                    g_statusColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
                }
            }
        }

        // Auto-inject
        if (g_config.autoInject && !g_processes.empty() && !g_config.dllPath.empty()) {
            for (auto& p : g_processes) {
                if (!p.injected) {
                    p.status = "Auto-injecting...";
                    if (InjectDLL(p.pid, g_config.dllPath)) {
                        p.injected = true;
                        p.status = "Success";
                    } else {
                        p.status = "Error";
                    }
                }
            }
        }

        ImGui::End();

        ImGui::EndFrame();
        ImGui::Render();

        if (g_needReset)
        {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            D3DPRESENT_PARAMETERS pp = {};
            pp.Windowed = TRUE;
            pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            pp.hDeviceWindow = hwnd;
            pp.BackBufferFormat = D3DFMT_X8R8G8B8;
            pp.EnableAutoDepthStencil = FALSE;
            pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
            if (SUCCEEDED(device->Reset(&pp)))
                ImGui_ImplDX9_CreateDeviceObjects();
            g_needReset = false;
        }

        device->Clear(0, NULL, D3DCLEAR_TARGET,
            D3DCOLOR_XRGB(26, 26, 46), 1.0f, 0);
        if (device->BeginScene() == D3D_OK) {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            device->EndScene();
        }
        device->Present(NULL, NULL, NULL, NULL);
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    device->Release();
    d3d->Release();
    DestroyWindow(hwnd);
    UnregisterClass(L"PaperScentsWindow", hInstance);
    return 0;
}
