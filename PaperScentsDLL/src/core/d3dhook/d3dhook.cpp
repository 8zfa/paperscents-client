#include <Windows.h>
#include <gl/GL.h>

#include "d3dhook.h"
#include "../utilities/logger.h"
#include "../modulehandler/modulehandler.h"
#include "../rendering/uirendering.h"
#include "../rendering/menu/menu.h"
#include "../rendering/neverlose/gui.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl2.h>

#include <MinHook.h>
#include <string>

typedef BOOL(WINAPI* wglSwapBuffers_t)(HDC hdc);
static wglSwapBuffers_t g_OriginalSwapBuffers = nullptr;
static bool g_ImGuiInitialized = false;
static HWND g_GameHWND = nullptr;
static HDC g_GameHDC = nullptr;
static HGLRC g_OriginalGLContext = nullptr;
static HGLRC g_MenuGLContext = nullptr;

static int g_FrameCount = 0;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static WNDPROC g_OriginalWndProc = nullptr;

static void PollKeys()
{
    auto& modules = ModuleHandler::GetInstance().GetModules();
    static std::unordered_map<int, bool> prevStates;
    for (auto* mod : modules)
    {
        int k = mod->GetKeybind();
        if (k <= 0) continue;
        bool down = GetAsyncKeyState(k) & 0x8000;
        bool& prev = prevStates[k];
        if (down && !prev)
            mod->Toggle();
        prev = down;
    }
}

static LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Menu::GetInstance().IsOpen())
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
    }

    return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
}

static void SetupImGui()
{
    g_MenuGLContext = wglCreateContext(g_GameHDC);
    wglMakeCurrent(g_GameHDC, g_MenuGLContext);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLint m_viewport[4];
    glGetIntegerv(GL_VIEWPORT, m_viewport);
    glOrtho(0, m_viewport[2], m_viewport[3], 0, 1, -1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0, 0, 0, 0);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.Fonts->AddFontDefault();

    // Load Roboto for menu text
    char dllPath[MAX_PATH];
    if (GetModuleFileNameA(GetModuleHandleA("PaperScentsDLL.dll"), dllPath, sizeof(dllPath)))
    {
        std::string baseDir = dllPath;
        baseDir = baseDir.substr(0, baseDir.find_last_of("\\/"));
        baseDir = baseDir.substr(0, baseDir.find_last_of("\\/"));
        baseDir = baseDir.substr(0, baseDir.find_last_of("\\/"));
        baseDir = baseDir.substr(0, baseDir.find_last_of("\\/"));
        std::string fontPath = baseDir + "\\ext\\fonts\\Roboto-Medium.ttf";

        ImFontConfig fontCfg;
        fontCfg.SizePixels = 14.0f;
        ImFont* roboto = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f, &fontCfg);
        if (roboto)
        {
            ImFontConfig boldCfg;
            boldCfg.SizePixels = 14.0f;
            std::string boldFont = baseDir + "\\ext\\fonts\\Roboto-Bold.ttf";
            io.Fonts->AddFontFromFileTTF(boldFont.c_str(), 14.0f, &boldCfg);
        }
    }

    g_NeverloseGUI.ApplyStyle();

    g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(g_GameHWND, GWLP_WNDPROC, (LONG_PTR)WndProcHook);

    ImGui_ImplWin32_Init(g_GameHWND);
    ImGui_ImplOpenGL2_Init();

    g_ImGuiInitialized = true;
    Logger::Log("ImGui initialized for OpenGL overlay");
}

static void RenderImGui()
{
    PollKeys();

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ModuleHandler::GetInstance().OnUpdate();
    UIRendering::GetInstance().Render();
    Menu::GetInstance().Render();

    // Fullscreen overlay for HUD rendering
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("##Overlay", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground);
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

HWND GetGameWindowHandle()
{
    return g_GameHWND;
}

static BOOL WINAPI SwapBuffersHook(HDC hdc)
{
    g_FrameCount++;
    HWND hwnd = WindowFromDC(hdc);

    // Handle fullscreen toggle: reinitialize when window handle changes
    if (g_ImGuiInitialized && hwnd != g_GameHWND)
    {
        Logger::Log("Window changed (fullscreen toggle), reinitializing ImGui...");
        if (g_OriginalWndProc && g_GameHWND)
            SetWindowLongPtr(g_GameHWND, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (g_MenuGLContext)
        {
            wglDeleteContext(g_MenuGLContext);
            g_MenuGLContext = nullptr;
        }
        g_ImGuiInitialized = false;
        g_OriginalWndProc = nullptr;
    }

    g_OriginalGLContext = wglGetCurrentContext();
    g_GameHDC = hdc;
    g_GameHWND = hwnd;

    if (!g_ImGuiInitialized)
    {
        Logger::Log("SwapBuffersHook called, initializing ImGui...");
        SetupImGui();
    }

    if (g_FrameCount <= 10 || (g_FrameCount % 300 == 0))
    {
        Logger::Log("SwapBuffersHook frame %d", g_FrameCount);
    }

    wglMakeCurrent(g_GameHDC, g_MenuGLContext);
    RenderImGui();
    wglMakeCurrent(g_GameHDC, g_OriginalGLContext);

    return g_OriginalSwapBuffers(hdc);
}

bool InitD3DHook()
{
    Logger::Log("Initializing OpenGL hook...");

    HMODULE opengl = GetModuleHandleA("opengl32.dll");
    if (!opengl)
    {
        Logger::Log("opengl32.dll not found");
        return false;
    }

    void* swapAddr = GetProcAddress(opengl, "wglSwapBuffers");
    if (!swapAddr)
    {
        Logger::Log("wglSwapBuffers not found in opengl32.dll");
        return false;
    }

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
    {
        Logger::Log("MH_Initialize failed: %s", MH_StatusToString(status));
        return false;
    }

    status = MH_CreateHook(swapAddr, SwapBuffersHook, (void**)&g_OriginalSwapBuffers);
    if (status != MH_OK)
    {
        Logger::Log("MH_CreateHook wglSwapBuffers failed: %s", MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(swapAddr);
    if (status != MH_OK)
    {
        Logger::Log("MH_EnableHook failed: %s", MH_StatusToString(status));
        return false;
    }

    Logger::Log("OpenGL hook installed successfully");
    return true;
}

void ShutdownD3DHook()
{
    if (g_ImGuiInitialized)
    {
        if (g_OriginalWndProc && g_GameHWND)
        {
            SetWindowLongPtr(g_GameHWND, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
        }
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (g_MenuGLContext)
        {
            wglDeleteContext(g_MenuGLContext);
            g_MenuGLContext = nullptr;
        }
        g_ImGuiInitialized = false;
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    Logger::Log("OpenGL hook shut down");
}
