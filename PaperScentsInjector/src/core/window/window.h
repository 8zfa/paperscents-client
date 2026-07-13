#pragma once
#include <Windows.h>
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

struct WindowConfig
{
    LPCWSTR Title = L"PaperScents";
    int Width = 580;
    int Height = 440;
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
    LPDIRECT3DDEVICE9 GetDevice() const { return m_Device; }
    IDirect3DTexture9* GetUserLogoTexture() const { return m_UserLogoTexture; }
    int GetUserLogoWidth() const { return m_UserLogoWidth; }
    int GetUserLogoHeight() const { return m_UserLogoHeight; }

    ImFont* FontTitle = nullptr;
    ImFont* FontNormal = nullptr;
    ImFont* FontSmall = nullptr;
    ImFont* FontBold = nullptr;

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    bool LoadTextureFromFile(const char* filename, LPDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height);

private:
    bool CreateDeviceD3D();
    void ResetDevice();
    void SetupImGui();

    HWND m_Hwnd = nullptr;
    WNDCLASSEX m_Wc = {};
    LPDIRECT3D9 m_D3D = nullptr;
    LPDIRECT3DDEVICE9 m_Device = nullptr;
    LPDIRECT3DTEXTURE9 m_UserLogoTexture = nullptr;
    int m_UserLogoWidth = 0;
    int m_UserLogoHeight = 0;
    D3DPRESENT_PARAMETERS m_D3dpp = {};
    bool m_DeviceLost = false;
    int m_Width = 580;
    int m_Height = 440;
    ULONG_PTR m_GdiplusToken = 0;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
