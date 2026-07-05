#include "window.h"
#include "core/config.h"
#include "core/ui.h"
#include "core/logger.h"

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
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED || !self)
            return 0;
        self->m_Width = (int)LOWORD(lParam);
        self->m_Height = (int)HIWORD(lParam);
        self->m_D3dpp.BackBufferWidth = (UINT)self->m_Width;
        self->m_D3dpp.BackBufferHeight = (UINT)self->m_Height;
        self->m_DeviceLost = true;
        return 0;
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

static const char* HResultToString(HRESULT hr)
{
    switch (hr)
    {
    case D3DERR_DEVICELOST: return "D3DERR_DEVICELOST";
    case D3DERR_INVALIDCALL: return "D3DERR_INVALIDCALL";
    case D3DERR_NOTAVAILABLE: return "D3DERR_NOTAVAILABLE";
    case D3DERR_OUTOFVIDEOMEMORY: return "D3DERR_OUTOFVIDEOMEMORY";
    case D3DERR_DRIVERINTERNALERROR: return "D3DERR_DRIVERINTERNALERROR";
    case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
    case D3DERR_DEVICENOTRESET: return "D3DERR_DEVICENOTRESET";
    default: return "UNKNOWN";
    }
}

bool Window::CreateDeviceD3D()
{
    m_D3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_D3D)
    {
        Logger::Get().Error("Direct3DCreate9(D3D_SDK_VERSION) returned null.");
        return false;
    }

    // backbuffer formats to try (in order of preference)
    D3DFORMAT bbFormats[] = {
        D3DFMT_X8R8G8B8,
        D3DFMT_A8R8G8B8,
        D3DFMT_R5G6B5,
        D3DFMT_UNKNOWN
    };

    // depth/stencil formats to try (empty entry means no depth)
    struct DepthOption { bool enable; D3DFORMAT fmt; };
    DepthOption depthOpts[] = {
        { true,  D3DFMT_D24S8 },
        { true,  D3DFMT_D24X8 },
        { true,  D3DFMT_D16 },
        { false, D3DFMT_UNKNOWN }
    };

    // present intervals
    DWORD intervals[] = {
        D3DPRESENT_INTERVAL_ONE,
        D3DPRESENT_INTERVAL_IMMEDIATE,
        D3DPRESENT_INTERVAL_DEFAULT
    };

    // vertex processing modes
    DWORD vpModes[] = {
        D3DCREATE_MIXED_VERTEXPROCESSING,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        D3DCREATE_HARDWARE_VERTEXPROCESSING
    };

    // device types
    D3DDEVTYPE devTypes[] = {
        D3DDEVTYPE_HAL,
        D3DDEVTYPE_SW,
        D3DDEVTYPE_REF
    };

    UINT adapterCount = m_D3D->GetAdapterCount();

    for (UINT adapter = 0; adapter < adapterCount; ++adapter)
    {
        D3DADAPTER_IDENTIFIER9 adapterId;
        if (FAILED(m_D3D->GetAdapterIdentifier(adapter, 0, &adapterId)))
            continue;

        for (auto& devType : devTypes)
        {
            // Skip SW/REF unless HAL failed on all previous adapters
            if (devType == D3DDEVTYPE_SW || devType == D3DDEVTYPE_REF)
            {
                if (adapter < adapterCount - 1)
                    continue; // only try SW/REF on the last adapter
            }

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
                                char buf[256];
                                sprintf_s(buf, "D3D9 device created: adapter=%u (%hs) devType=%d vp=0x%X bbFmt=%d depth=%d/%d interval=%d",
                                    adapter, adapterId.Description, (int)devType, vp,
                                    (int)bbFmt, depth.enable, depth.enable ? (int)depth.fmt : -1, interval);
                                Logger::Get().Info(buf);
                                m_D3dpp = pp;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    Logger::Get().Error("All D3D9 device creation combinations exhausted.");
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

void Window::SetupImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    FontTitle = io.Fonts->AddFontFromFileTTF("ext\\fonts\\Roboto-Bold.ttf", 20.0f);
    FontNormal = io.Fonts->AddFontFromFileTTF("ext\\fonts\\Roboto-Medium.ttf", 16.0f);
    FontSmall = io.Fonts->AddFontFromFileTTF("ext\\fonts\\Roboto-Regular.ttf", 13.0f);
    FontBold = io.Fonts->AddFontFromFileTTF("ext\\fonts\\Roboto-Bold.ttf", 16.0f);

    ImGui_ImplWin32_Init(m_Hwnd);
    ImGui_ImplDX9_Init(m_Device);
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

    m_Device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(26, 26, 46), 1.0f, 0);
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
