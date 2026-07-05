#pragma once
#include <Windows.h>
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

struct WindowConfig
{
    LPCWSTR Title = L"PaperScents";
    int Width = 850;
    int Height = 580;
    int X = 100;
    int Y = 100;
};

class Window
{
public:
    bool Create(HINSTANCE hInstance, const WindowConfig& config);
    void Render();
    void Destroy();
    HWND GetHandle() const { return m_Hwnd; }

    ImFont* FontTitle = nullptr;
    ImFont* FontNormal = nullptr;
    ImFont* FontSmall = nullptr;
    ImFont* FontBold = nullptr;

private:
    bool CreateDeviceD3D();
    void ResetDevice();
    void SetupImGui();

    HWND m_Hwnd = nullptr;
    WNDCLASSEX m_Wc = {};
    LPDIRECT3D9 m_D3D = nullptr;
    LPDIRECT3DDEVICE9 m_Device = nullptr;
    D3DPRESENT_PARAMETERS m_D3dpp = {};
    bool m_DeviceLost = false;
    int m_Width = 850;
    int m_Height = 580;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
