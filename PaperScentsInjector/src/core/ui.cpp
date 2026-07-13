#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui.h"
#include "core.h"
#include "config.h"
#include "logger.h"
#include "process.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>

static std::vector<ProcessInfo> s_Processes;
static std::vector<int> s_InjectedPIDs;
static std::string s_SelectedDllPath;
static float s_StartTime = 0.0f;

static float s_SlideProgress = 0.0f;
static float s_ToggleAnim = 0.0f;
static float s_LogProgress = 0.0f;

static void OpenFileDialog()
{
    OPENFILENAMEA ofn = {};
    char buf[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "DLL Files\0*.dll\0All Files\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameA(&ofn))
    {
        s_SelectedDllPath = buf;
        ConfigManager::Get().GetConfig().LastDllPath = buf;
        ConfigManager::Get().MarkDirty();
        Logger::Get().Info("Selected DLL: " + s_SelectedDllPath);
    }
}

static ImFont* Fallback(ImFont* f)
{
    return f ? f : ImGui::GetFont();
}

static float FontSize(ImFont* f)
{
    return f ? f->FontSize : ImGui::GetFontSize();
}

static bool GradientButton(const char* label, ImVec2 pos, ImVec2 size, ImU32 colorTop, ImU32 colorBot, float rounding, float alpha = 1.0f)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool hovered = ImGui::IsMouseHoveringRect(pos, pos + size);
    bool held = ImGui::IsMouseDown(0) && hovered;

    static float scaleAnim = 1.0f;
    float dt = ImGui::GetIO().DeltaTime;
    scaleAnim = hovered ? std::min(1.05f, scaleAnim + dt * 6.0f) : std::max(1.0f, scaleAnim - dt * 6.0f);

    ImVec2 scl(size.x * scaleAnim, size.y * scaleAnim);
    ImVec2 off((size.x - scl.x) * 0.5f, (size.y - scl.y) * 0.5f);
    ImVec2 bp = pos + off;

    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(label, size);
    bool clicked = ImGui::IsItemClicked();

    ImVec4 cTop = ImGui::ColorConvertU32ToFloat4(colorTop);
    ImVec4 cBot = ImGui::ColorConvertU32ToFloat4(colorBot);
    cTop.w *= alpha;
    cBot.w *= alpha;

    if (held)
    {
        cTop = ImVec4(0.30f, 0.28f, 0.70f, alpha);
        cBot = ImVec4(0.25f, 0.22f, 0.60f, alpha);
    }
    else if (hovered)
    {
        cTop.x = std::min(1.0f, cTop.x + 0.12f);
        cTop.y = std::min(1.0f, cTop.y + 0.12f);
        cTop.z = std::min(1.0f, cTop.z + 0.12f);
        cBot.x = std::min(1.0f, cBot.x + 0.08f);
        cBot.y = std::min(1.0f, cBot.y + 0.08f);
        cBot.z = std::min(1.0f, cBot.z + 0.08f);
    }

    dl->AddRectFilled(bp, bp + scl, ImGui::ColorConvertFloat4ToU32(cTop), rounding);
    dl->AddRect(bp, bp + scl, IM_COL32(255, 255, 255, (int)(30 * alpha)), rounding, 0, 1.0f);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos(bp.x + (scl.x - textSize.x) * 0.5f, bp.y + (scl.y - textSize.y) * 0.5f);
    dl->AddText(textPos, IM_COL32(255, 255, 255, (int)(255 * alpha)), label);

    return clicked;
}

static const char* StatusText()
{
    if (s_Processes.empty()) return "Ready";
    for (auto& p : s_Processes)
        if (p.Status == "Error") return "Error";
    for (auto& p : s_Processes)
        if (p.Status == "Injecting...") return "Injecting";
    for (int ip : s_InjectedPIDs)
    {
        bool allInjected = true;
        for (auto& p : s_Processes)
        {
            bool found = false;
            for (int ip2 : s_InjectedPIDs) { if ((int)p.PID == ip2) { found = true; break; } }
            if (!found) { allInjected = false; break; }
        }
        if (allInjected) return "Injected";
    }
    return "Ready";
}

static ImU32 StatusColor()
{
    std::string s = StatusText();
    if (s == "Ready") return IM_COL32(0x4C, 0xAF, 0x50, 255);
    if (s == "Injecting") return IM_COL32(0xFF, 0xA7, 0x26, 255);
    if (s == "Error") return IM_COL32(0xE0, 0x40, 0x40, 255);
    return IM_COL32(0x4C, 0xAF, 0x50, 255);
}

static std::string FriendlyName(const std::string& exe)
{
    std::string lower = exe;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "javaw.exe") return "Minecraft 1.8.9";
    return exe;
}

void RenderUI()
{
    auto& cfg = ConfigManager::Get().GetConfig();
    float t = ImGui::GetTime();
    float dt = ImGui::GetIO().DeltaTime;

    if (s_StartTime == 0.0f)
    {
        s_StartTime = t;
        if (!cfg.LastDllPath.empty()) s_SelectedDllPath = cfg.LastDllPath;
    }

    float fade = std::min(1.0f, (t - s_StartTime) / 0.30f);
    ImGui::GetStyle().Alpha = fade;

    float w = 580.0f;
    float h = 440.0f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    static bool styleInit = false;
    if (!styleInit)
    {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 0.0f;
        s.FrameRounding = 8.0f;
        s.GrabRounding = 8.0f;
        s.FramePadding = ImVec2(10, 8);
        s.ItemSpacing = ImVec2(12, 10);
        s.Colors[ImGuiCol_Button] = ImVec4(0.42f, 0.39f, 1.0f, 0.8f);
        s.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.48f, 0.45f, 1.0f, 1.0f);
        s.Colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.32f, 0.8f, 1.0f);
        s.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        s.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        s.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.20f, 0.8f);
        s.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.28f, 0.8f);
        s.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.10f, 0.10f, 0.15f, 0.8f);
        s.Colors[ImGuiCol_CheckMark] = ImVec4(0.42f, 0.39f, 1.0f, 1.0f);
        styleInit = true;
    }

    ImGui::Begin("##Main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetWindowPos();
    ImVec2 p1 = p0 + ImVec2(w, h);

    dl->AddRectFilled(p0, p1, IM_COL32(0x0D, 0x0D, 0x0D, 245), 12.0f);
    dl->AddRect(p0, p1, IM_COL32(0x3A, 0x3A, 0x4A, 255), 12.0f, 0, 1.5f);

    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##drag", ImVec2(w - 100, 60));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        ReleaseCapture();
        SendMessage(g_Window->GetHandle(), WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    float cx = w * 0.5f;

    // ── Branding (Top-Center) ──
    IDirect3DTexture9* logoTexture = g_Window->GetUserLogoTexture();
    if (logoTexture)
    {
        float logoW = (float)g_Window->GetUserLogoWidth();
        float logoH = (float)g_Window->GetUserLogoHeight();
        float aspect = logoW / logoH;
        float targetH = 44.0f;
        float targetW = targetH * aspect;
        dl->AddImage((ImTextureID)logoTexture,
            ImVec2(p0.x + cx - targetW * 0.5f, p0.y + 10),
            ImVec2(p0.x + cx + targetW * 0.5f, p0.y + 10 + targetH));
    }
    else
    {
        ImFont* titleFont = Fallback(g_Window->FontTitle);
        ImVec2 brandSize = titleFont->CalcTextSizeA(FontSize(g_Window->FontTitle), FLT_MAX, 0, "PaperScents");
        dl->AddText(titleFont, FontSize(g_Window->FontTitle),
            ImVec2(p0.x + cx - brandSize.x * 0.5f, p0.y + 10),
            IM_COL32(0x6C, 0x63, 0xFF, 255), "PaperScents");

        ImFont* smallFont = Fallback(g_Window->FontSmall);
        ImVec2 subSize = smallFont->CalcTextSizeA(FontSize(g_Window->FontSmall), FLT_MAX, 0, "injector");
        dl->AddText(smallFont, FontSize(g_Window->FontSmall),
            ImVec2(p0.x + cx - subSize.x * 0.5f, p0.y + 34),
            IM_COL32(0x88, 0x88, 0xAA, 255), "injector");
    }

    // ── Minimize & Close (Top-Right) ──
    ImGui::SetCursorPos(ImVec2(w - 68, 14));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.50f, 0.4f));
    if (ImGui::Button("_", ImVec2(24, 24)))
        ShowWindow(g_Window->GetHandle(), SW_MINIMIZE);
    ImGui::PopStyleColor(2);

    ImGui::SetCursorPos(ImVec2(w - 36, 14));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.26f, 0.21f, 0.8f));
    if (ImGui::Button("X", ImVec2(24, 24)))
    {
        cfg.WindowX = (int)p0.x;
        cfg.WindowY = (int)p0.y;
        ConfigManager::Get().Save();
        PostQuitMessage(0);
    }
    ImGui::PopStyleColor(2);

    // ── Status Indicator (Top-Left) ──
    float statusY = 22.0f;
    ImU32 dotCol = StatusColor();
    ImVec2 dotCenter(p0.x + 30.0f, p0.y + statusY + 10.0f);
    std::string statusStr = StatusText();
    if (statusStr == "Ready" || statusStr == "Injecting")
    {
        float dotPulse = (sinf(t * 4.0f) + 1.0f) * 0.5f;
        ImU32 ringCol = IM_COL32((dotCol >> 16) & 0xFF, (dotCol >> 8) & 0xFF, dotCol & 0xFF, (int)(30 + dotPulse * 50));
        dl->AddCircle(dotCenter, 4.0f + dotPulse * 4.0f, ringCol, 0, 1.5f);
    }
    dl->AddCircleFilled(dotCenter, 4.0f, dotCol);
    {
        ImFont* f = Fallback(g_Window->FontNormal);
        dl->AddText(f, FontSize(g_Window->FontNormal),
            ImVec2(p0.x + 44.0f, p0.y + statusY + 1.0f),
            IM_COL32(180, 180, 200, 255), statusStr.c_str());
    }

    // ── Slide Card Animation ──
    bool hasProc = !s_Processes.empty();
    if (hasProc)
        s_SlideProgress = std::min(1.0f, s_SlideProgress + dt * 4.5f);
    else
        s_SlideProgress = std::max(0.0f, s_SlideProgress - dt * 4.5f);

    float cardW = 420.0f;
    float cardH = 130.0f;
    float cardX = p0.x + (w - cardW) * 0.5f;

    if (s_SlideProgress > 0.0f)
    {
        float cardY = p0.y + 90.0f + (1.0f - s_SlideProgress) * 40.0f;
        ImU32 cardBg = IM_COL32(0x15, 0x15, 0x22, (int)(255 * s_SlideProgress));
        ImU32 cardBorder = IM_COL32(0x2A, 0x2A, 0x3A, (int)(180 * s_SlideProgress));

        dl->AddRectFilled(ImVec2(cardX, cardY), ImVec2(cardX + cardW, cardY + cardH), cardBg, 10.0f);
        dl->AddRect(ImVec2(cardX, cardY), ImVec2(cardX + cardW, cardY + cardH), cardBorder, 10.0f, 0, 1.0f);

        ImVec2 iconC(cardX + 42, cardY + cardH * 0.5f);
        dl->AddCircleFilled(iconC, 22, IM_COL32(0x6C, 0x63, 0xFF, (int)(30 * s_SlideProgress)), 0);
        dl->AddCircle(iconC, 22, IM_COL32(0x6C, 0x63, 0xFF, (int)(80 * s_SlideProgress)), 0, 1.0f);

        {
            ImFont* f = Fallback(g_Window->FontNormal);
            dl->AddText(f, FontSize(g_Window->FontNormal),
                ImVec2(iconC.x - 9, iconC.y - 10),
                IM_COL32(255, 255, 255, (int)(255 * s_SlideProgress)), "M");
        }

        if (!s_Processes.empty())
        {
            auto& fp = s_Processes[0];
            {
                ImFont* f = Fallback(g_Window->FontNormal);
                dl->AddText(f, FontSize(g_Window->FontNormal),
                    ImVec2(cardX + 80, cardY + 22),
                    IM_COL32(240, 240, 250, (int)(255 * s_SlideProgress)),
                    FriendlyName(fp.Name).c_str());
            }
            {
                ImFont* f = Fallback(g_Window->FontSmall);
                std::string pidStr = "PID: " + std::to_string(fp.PID);
                dl->AddText(f, FontSize(g_Window->FontSmall),
                    ImVec2(cardX + 80, cardY + 48),
                    IM_COL32(140, 140, 160, (int)(255 * s_SlideProgress)), pidStr.c_str());

                std::string dllLabel = s_SelectedDllPath.empty() ? "No DLL selected" : s_SelectedDllPath;
                if (dllLabel.length() > 42) dllLabel = "..." + dllLabel.substr(dllLabel.length() - 39);
                dl->AddText(f, FontSize(g_Window->FontSmall),
                    ImVec2(cardX + 80, cardY + 68),
                    IM_COL32(100, 100, 120, (int)(255 * s_SlideProgress)), dllLabel.c_str());
            }

            float btnW = 120.0f;
            float btnH = 32.0f;
            float btnX = cardX + cardW - btnW - 20;
            float btnY = cardY + cardH - btnH - 18;

            bool alreadyInjected = false;
            for (int ip : s_InjectedPIDs)
                if (ip == (int)fp.PID) { alreadyInjected = true; break; }

            if (alreadyInjected)
            {
                ImFont* f = Fallback(g_Window->FontBold);
                dl->AddText(f, FontSize(g_Window->FontBold),
                    ImVec2(btnX + 12, btnY + 6),
                    IM_COL32(76, 175, 80, (int)(255 * s_SlideProgress)), "Injected");
            }
            else
            {
                if (GradientButton("Inject", ImVec2(btnX, btnY), ImVec2(btnW, btnH),
                    IM_COL32(0x6C, 0x63, 0xFF, 255), IM_COL32(0x3F, 0x3D, 0x9E, 255), 8.0f, s_SlideProgress))
                {
                    if (s_SelectedDllPath.empty())
                    {
                        Logger::Get().Warning("No DLL selected.");
                    }
                    else
                    {
                        fp.Status = "Injecting...";
                        DWORD pid = fp.PID;
                        std::string dll = s_SelectedDllPath;
                        Logger::Get().Info("Injecting into " + fp.Name + " (PID:" + std::to_string(pid) + ")");
                        std::thread([pid, dll]() {
                            std::string err;
                            if (ProcessManager::Inject(pid, dll, err))
                            {
                                s_InjectedPIDs.push_back(pid);
                                Logger::Get().Success("Injection succeeded!");
                            }
                            else
                            {
                                Logger::Get().Error(err);
                            }
                        }).detach();
                    }
                }
            }
        }
    }

    if (s_SlideProgress < 1.0f)
    {
        float waitAlpha = 1.0f - s_SlideProgress;
        float waitPulse = (sinf(t * 3.5f) + 1.0f) * 0.5f;
        ImU32 waitCol = IM_COL32(150, 150, 170, (int)(waitAlpha * (120 + waitPulse * 135)));
        {
            ImFont* f = Fallback(g_Window->FontNormal);
            ImVec2 ws = f->CalcTextSizeA(FontSize(g_Window->FontNormal), FLT_MAX, 0, "Waiting for Minecraft...");
            dl->AddText(f, FontSize(g_Window->FontNormal),
                ImVec2(p0.x + cx - ws.x * 0.5f, p0.y + 130.0f), waitCol, "Waiting for Minecraft...");
        }
        {
            ImFont* f = Fallback(g_Window->FontSmall);
            ImVec2 ss = f->CalcTextSizeA(FontSize(g_Window->FontSmall), FLT_MAX, 0, "Launch Lunar Client or vanilla to detect");
            dl->AddText(f, FontSize(g_Window->FontSmall),
                ImVec2(p0.x + cx - ss.x * 0.5f, p0.y + 155.0f),
                IM_COL32(100, 100, 120, (int)(waitAlpha * 255)), "Launch Lunar Client or vanilla to detect");
        }
    }

    // ── Controls: DLL Browse ──
    float ctrlY = p0.y + 245.0f;
    float browseW = 320.0f;
    float browseH = 34.0f;
    float browseX = p0.x + 30.0f;

    dl->AddRectFilled(ImVec2(browseX, ctrlY), ImVec2(browseX + browseW, ctrlY + browseH), IM_COL32(0x15, 0x15, 0x22, 255), 8.0f);
    dl->AddRect(ImVec2(browseX, ctrlY), ImVec2(browseX + browseW, ctrlY + browseH), IM_COL32(0x2A, 0x2A, 0x3A, 255), 8.0f, 0, 1.0f);

    std::string dllText = s_SelectedDllPath.empty() ? "Select client DLL..." : s_SelectedDllPath;
    if (dllText.length() > 38) dllText = "..." + dllText.substr(dllText.length() - 35);
    ImU32 dllCol = s_SelectedDllPath.empty() ? IM_COL32(90, 90, 110, 255) : IM_COL32(200, 200, 210, 255);

    {
        ImFont* f = Fallback(g_Window->FontSmall);
        dl->AddText(f, FontSize(g_Window->FontSmall),
            ImVec2(browseX + 14.0f, ctrlY + 10.0f), dllCol, dllText.c_str());
    }

    ImGui::SetCursorScreenPos(ImVec2(browseX, ctrlY));
    if (ImGui::InvisibleButton("##browseBox", ImVec2(browseW, browseH)))
        OpenFileDialog();

    float browseBtnX = browseX + browseW + 10.0f;
    if (GradientButton("Browse", ImVec2(browseBtnX, ctrlY), ImVec2(90.0f, browseH),
        IM_COL32(0x2E, 0x2E, 0x3F, 255), IM_COL32(0x20, 0x20, 0x2F, 255), 8.0f, 1.0f))
    {
        OpenFileDialog();
    }

    // ── Auto-Inject Toggle ──
    float toggleY = p0.y + 295.0f;
    float toggleX = p0.x + 30.0f;
    float toggleW = 38.0f;
    float toggleH = 20.0f;

    ImGui::SetCursorScreenPos(ImVec2(toggleX, toggleY));
    if (ImGui::InvisibleButton("##toggle", ImVec2(toggleW, toggleH)))
    {
        cfg.AutoInject = !cfg.AutoInject;
        ConfigManager::Get().MarkDirty();
    }

    if (cfg.AutoInject)
        s_ToggleAnim = std::min(1.0f, s_ToggleAnim + dt * 8.0f);
    else
        s_ToggleAnim = std::max(0.0f, s_ToggleAnim - dt * 8.0f);

    ImVec4 bgOff(0.12f, 0.12f, 0.18f, 1.0f);
    ImVec4 bgOn(0.42f, 0.39f, 1.0f, 1.0f);
    ImVec4 bgCur(
        bgOff.x + (bgOn.x - bgOff.x) * s_ToggleAnim,
        bgOff.y + (bgOn.y - bgOff.y) * s_ToggleAnim,
        bgOff.z + (bgOn.z - bgOff.z) * s_ToggleAnim,
        1.0f
    );
    dl->AddRectFilled(ImVec2(toggleX, toggleY), ImVec2(toggleX + toggleW, toggleY + toggleH),
        ImGui::ColorConvertFloat4ToU32(bgCur), 10.0f);
    dl->AddRect(ImVec2(toggleX, toggleY), ImVec2(toggleX + toggleW, toggleY + toggleH),
        IM_COL32(255, 255, 255, 20), 10.0f);

    float thumbX = toggleX + 10.0f + s_ToggleAnim * 18.0f;
    dl->AddCircleFilled(ImVec2(thumbX, toggleY + 10.0f), 8.0f, IM_COL32(255, 255, 255, 255));

    {
        ImFont* f = Fallback(g_Window->FontNormal);
        dl->AddText(f, FontSize(g_Window->FontNormal),
            ImVec2(toggleX + 48.0f, toggleY + 1.0f),
            IM_COL32(150, 150, 170, 255), "Auto-Inject");
    }

    // ── Log Panel ──
    if (cfg.LogVisible)
        s_LogProgress = std::min(1.0f, s_LogProgress + dt * 5.0f);
    else
        s_LogProgress = std::max(0.0f, s_LogProgress - dt * 5.0f);

    float logMaxH = 150.0f;
    float logH = s_LogProgress * logMaxH;
    float sheetY = p1.y - logH - 45.0f;

    if (s_LogProgress > 0.0f)
    {
        dl->AddRectFilled(ImVec2(p0.x, sheetY), ImVec2(p1.x, p1.y - 45.0f), IM_COL32(0x0A, 0x0A, 0x0F, 245));
        dl->AddLine(ImVec2(p0.x, sheetY), ImVec2(p1.x, sheetY), IM_COL32(0x20, 0x20, 0x30, 255), 1.0f);

        ImGui::SetCursorScreenPos(ImVec2(p0.x + 15.0f, sheetY + 10.0f));
        ImGui::BeginChild("##logScroll", ImVec2(w - 30.0f, logH - 15.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 3));

        auto& entries = Logger::Get().GetEntries();
        for (auto& e : entries)
        {
            const char* label = "";
            ImU32 col;
            switch (e.Level)
            {
            case LogInfo:    label = "[i]"; col = IM_COL32(0x88, 0x88, 0xAA, 255); break;
            case LogSuccess: label = "[+]"; col = IM_COL32(0x4C, 0xAF, 0x50, 255); break;
            case LogWarning: label = "[!]"; col = IM_COL32(0xFF, 0xA7, 0x26, 255); break;
            case LogError:   label = "[x]"; col = IM_COL32(0xE0, 0x40, 0x40, 255); break;
            }
            ImDrawList* ldl = ImGui::GetWindowDrawList();
            ImVec2 lpos = ImGui::GetCursorScreenPos();
            ldl->AddText(lpos, col, label);
            ImGui::SameLine(22.0f);
            ImGui::TextUnformatted(e.Message.c_str());
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::PopStyleVar();
        ImGui::EndChild();
    }

    // ── Bottom Bar ──
    float bottomBarY = p1.y - 45.0f;
    dl->AddRectFilled(ImVec2(p0.x, bottomBarY), p1, IM_COL32(0x0A, 0x0A, 0x0F, 255), 12.0f, ImDrawFlags_RoundCornersBottom);
    dl->AddLine(ImVec2(p0.x, bottomBarY), ImVec2(p1.x, bottomBarY), IM_COL32(0x20, 0x20, 0x30, 255), 1.0f);

    ImGui::SetCursorScreenPos(ImVec2(p0.x, bottomBarY));
    if (ImGui::InvisibleButton("##logHeader", ImVec2(w, 45.0f)))
    {
        cfg.LogVisible = !cfg.LogVisible;
        ConfigManager::Get().MarkDirty();
    }

    {
        ImFont* f = Fallback(g_Window->FontNormal);
        float textLogY = bottomBarY + 12.0f;
        dl->AddText(f, FontSize(g_Window->FontNormal),
            ImVec2(p0.x + 24.0f, textLogY), IM_COL32(140, 140, 160, 255), "Activity Log");
        const char* arrow = cfg.LogVisible ? "v" : ">";
        dl->AddText(f, FontSize(g_Window->FontNormal),
            ImVec2(p0.x + 115.0f, textLogY + 1.0f), IM_COL32(100, 100, 120, 255), arrow);

        auto& entries = Logger::Get().GetEntries();
        std::string lastMsg = entries.empty() ? "System ready." : entries.back().Message;
        if (lastMsg.length() > 50) lastMsg = lastMsg.substr(0, 47) + "...";
        dl->AddText(f, FontSize(g_Window->FontNormal),
            ImVec2(p0.x + 140.0f, textLogY), IM_COL32(90, 90, 110, 255), lastMsg.c_str());
    }

    ImGui::End();
    ImGui::PopStyleVar(3);

    // ── Background Scan ──
    static double lastScan = 0;
    double interval = s_Processes.empty() ? 1.0 : 2.0;
    if (t - lastScan > interval)
    {
        lastScan = t;
        std::thread([]() {
            auto fresh = ProcessManager::Scan();
            std::vector<int> stillInjected;
            for (int pid : s_InjectedPIDs)
                for (auto& p : fresh)
                    if ((int)p.PID == pid) { stillInjected.push_back(pid); break; }
            s_InjectedPIDs = stillInjected;

            auto& cfg = ConfigManager::Get().GetConfig();
            if (cfg.AutoInject && !s_SelectedDllPath.empty())
            {
                for (auto& p : fresh)
                {
                    bool already = false;
                    for (int ip : s_InjectedPIDs) { if (ip == (int)p.PID) { already = true; break; } }
                    if (!already)
                    {
                        std::string err;
                        if (ProcessManager::Inject(p.PID, s_SelectedDllPath, err))
                        {
                            s_InjectedPIDs.push_back(p.PID);
                            Logger::Get().Success("Injection succeeded!");
                        }
                        else
                            Logger::Get().Error(err);
                    }
                }
            }
            s_Processes = std::move(fresh);
        }).detach();
    }

    // ── Periodic Save ──
    static double lastSave = 0;
    if (t - lastSave > 5.0)
    {
        ConfigManager::Get().Save();
        lastSave = t;
    }
}
