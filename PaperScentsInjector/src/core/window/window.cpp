#define NOMINMAX
#include "window.h"
#include "core/config.h"
#include "core/ui.h"
#include "core/logger.h"
#include <windowsx.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    Window* self = (Window*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool Window::Create(HINSTANCE hInstance, const WindowConfig& config)
{
    m_Wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
        hInstance, LoadIcon(nullptr, IDI_APPLICATION),
        LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr,
        L"PaperScentsWindow", nullptr
    };
    RegisterClassEx(&m_Wc);

    m_Width = config.Width;
    m_Height = config.Height;

    m_Hwnd = CreateWindow(
        m_Wc.lpszClassName, config.Title,
        WS_POPUP,
        config.X, config.Y,
        config.Width, config.Height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!m_Hwnd)
    {
        Logger::Get().Error("CreateWindow failed.");
        return false;
    }

    SetWindowLongPtrW(m_Hwnd, GWLP_USERDATA, (LONG_PTR)this);

    if (!CreateDeviceD3D())
    {
        Logger::Get().Error("All D3D9 device creation attempts failed.");
        Destroy();
        return false;
    }

    ShowWindow(m_Hwnd, SW_SHOW);
    UpdateWindow(m_Hwnd);
    SetForegroundWindow(m_Hwnd);

    SetupImGui();
    return true;
}

bool Window::CreateDeviceD3D()
{
    m_D3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_D3D)
    {
        Logger::Get().Error("Direct3DCreate9(D3D_SDK_VERSION) returned null.");
        return false;
    }

    D3DFORMAT bbFormats[] = {
        D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_R5G6B5, D3DFMT_UNKNOWN
    };

    struct DepthOption { bool enable; D3DFORMAT fmt; };
    DepthOption depthOpts[] = {
        { true,  D3DFMT_D24S8 },
        { true,  D3DFMT_D24X8 },
        { true,  D3DFMT_D16 },
        { false, D3DFMT_UNKNOWN }
    };

    DWORD intervals[] = {
        D3DPRESENT_INTERVAL_ONE,
        D3DPRESENT_INTERVAL_IMMEDIATE,
        D3DPRESENT_INTERVAL_DEFAULT
    };

    DWORD vpModes[] = {
        D3DCREATE_MIXED_VERTEXPROCESSING,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        D3DCREATE_HARDWARE_VERTEXPROCESSING
    };

    D3DDEVTYPE devTypes[] = {
        D3DDEVTYPE_HAL, D3DDEVTYPE_SW, D3DDEVTYPE_REF
    };

    UINT adapterCount = m_D3D->GetAdapterCount();

    for (UINT adapter = 0; adapter < adapterCount; ++adapter)
    {
        D3DADAPTER_IDENTIFIER9 adapterId;
        if (FAILED(m_D3D->GetAdapterIdentifier(adapter, 0, &adapterId)))
            continue;

        for (auto& devType : devTypes)
        {
            if (devType == D3DDEVTYPE_SW || devType == D3DDEVTYPE_REF)
                if (adapter < adapterCount - 1)
                    continue;

            for (auto& vp : vpModes)
            {
                for (auto& bbFmt : bbFormats)
                {
                    for (auto& depth : depthOpts)
                    {
                        for (auto& interval : intervals)
                        {
                            D3DPRESENT_PARAMETERS pp = {};
                            pp.Windowed = TRUE;
                            pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                            pp.BackBufferFormat = bbFmt;
                            pp.BackBufferWidth = (UINT)m_Width;
                            pp.BackBufferHeight = (UINT)m_Height;
                            pp.EnableAutoDepthStencil = depth.enable;
                            if (depth.enable)
                                pp.AutoDepthStencilFormat = depth.fmt;
                            pp.PresentationInterval = interval;
                            pp.hDeviceWindow = m_Hwnd;

                            HRESULT hr = m_D3D->CreateDevice(
                                adapter, devType, m_Hwnd, vp, &pp, &m_Device);

                            if (SUCCEEDED(hr))
                            {
                                m_D3dpp = pp;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

void Window::ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = m_Device->Reset(&m_D3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

bool Window::LoadTextureFromFile(const char* filename, LPDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
{
    wchar_t wFilename[MAX_PATH];
    size_t converted = 0;
    mbstowcs_s(&converted, wFilename, filename, _TRUNCATE);

    Gdiplus::Bitmap bmp(wFilename);
    if (bmp.GetLastStatus() != Gdiplus::Ok)
        return false;

    int w = bmp.GetWidth();
    int h = bmp.GetHeight();

    Gdiplus::Rect rect(0, 0, w, h);
    Gdiplus::BitmapData bd;
    if (bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd) != Gdiplus::Ok)
        return false;

    LPDIRECT3DTEXTURE9 texture = nullptr;
    if (FAILED(m_Device->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr)))
    {
        bmp.UnlockBits(&bd);
        return false;
    }

    D3DLOCKED_RECT locked;
    if (SUCCEEDED(texture->LockRect(0, &locked, nullptr, 0)))
    {
        const unsigned char* src = (const unsigned char*)bd.Scan0;
        unsigned char* dst = (unsigned char*)locked.pBits;
        for (int y = 0; y < h; ++y)
        {
            memcpy(dst + y * locked.Pitch, src + y * bd.Stride, w * 4);
        }
        texture->UnlockRect(0);
    }

    bmp.UnlockBits(&bd);

    *out_texture = texture;
    *out_width = w;
    *out_height = h;
    return true;
}

static ImFont* TryLoadFont(ImFontAtlas* atlas, const char* name, float size)
{
    const char* paths[] = {
        name,
        ("ext\\fonts\\" + std::string(name)).c_str(),
        ("..\\ext\\fonts\\" + std::string(name)).c_str(),
        ("C:\\PaperScents\\ext\\fonts\\" + std::string(name)).c_str()
    };
    for (const char* path : paths)
    {
        ImFont* f = atlas->AddFontFromFileTTF(path, size);
        if (f) return f;
    }
    return atlas->AddFontDefault();
}

void Window::SetupImGui()
{
    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&m_GdiplusToken, &gsi, nullptr);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    FontTitle = TryLoadFont(io.Fonts, "Roboto-Bold.ttf", 22.0f);
    FontNormal = TryLoadFont(io.Fonts, "Roboto-Medium.ttf", 15.0f);
    FontSmall = TryLoadFont(io.Fonts, "Roboto-Regular.ttf", 13.0f);
    FontBold = TryLoadFont(io.Fonts, "Roboto-Bold.ttf", 15.0f);

    ImGui_ImplWin32_Init(m_Hwnd);
    ImGui_ImplDX9_Init(m_Device);

    const char* logoPaths[] = { "logo.png", "ext\\logo.png", "..\\ext\\logo.png", "C:\\PaperScents\\ext\\logo.png" };
    for(const char* path : logoPaths)
    {
        if (LoadTextureFromFile(path, &m_UserLogoTexture, &m_UserLogoWidth, &m_UserLogoHeight))
        {
            Logger::Get().Info(std::string("Loaded logo: ") + path);
            break;
        }
    }
}

void Window::Render()
{
    if (m_DeviceLost)
    {
        Sleep(100);
        HRESULT hr = m_Device->TestCooperativeLevel();
        if (hr == D3DERR_DEVICELOST)
            return;
        if (hr == D3DERR_DEVICENOTRESET)
            ResetDevice();
        m_DeviceLost = false;
    }

    m_Device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(13, 13, 13), 1.0f, 0);
    if (m_Device->BeginScene() == D3D_OK)
    {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        m_Device->EndScene();
    }

    HRESULT hr = m_Device->Present(nullptr, nullptr, nullptr, nullptr);
    if (hr == D3DERR_DEVICELOST)
        m_DeviceLost = true;
}

void Window::Destroy()
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (m_UserLogoTexture)
    {
        m_UserLogoTexture->Release();
        m_UserLogoTexture = nullptr;
    }

    if (m_GdiplusToken)
        Gdiplus::GdiplusShutdown(m_GdiplusToken);

    if (m_Device)
    {
        m_Device->Release();
        m_Device = nullptr;
    }
    if (m_D3D)
    {
        m_D3D->Release();
        m_D3D = nullptr;
    }

    DestroyWindow(m_Hwnd);
    UnregisterClass(m_Wc.lpszClassName, m_Wc.hInstance);
}
