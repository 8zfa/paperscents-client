#define NOMINMAX
#include "ui.h"
#include "core.h"
#include "config.h"
#include "logger.h"
#include "process.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>
#include <algorithm>
#include <Commdlg.h>

static std::vector<ProcessInfo> s_Processes;
static std::vector<int> s_InjectedPIDs;
static std::string s_SearchFilter;
static std::string s_SelectedDllPath;
static bool s_ScanRequested = true;
static bool s_Injecting = false;
static double s_LaunchTime = 0.0;
static float s_LogScrollToBottom = 0.0f;
static char s_SearchBuf[128] = "";

static void OpenFileDialog() {
    OPENFILENAMEA ofn = {};
    char buf[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "DLL Files\0*.dll\0All Files\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameA(&ofn)) {
        s_SelectedDllPath = buf;
        ConfigManager::Get().GetConfig().LastDllPath = buf;
        ConfigManager::Get().MarkDirty();
    }
}

static void RenderTitleBar(float width) {
    auto& cfg = ConfigManager::Get().GetConfig();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wPos = ImGui::GetWindowPos();

    // Title bar drag handling
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##title_bar_drag", ImVec2(width - 80, 40));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ReleaseCapture();
        SendMessage(g_Window->GetHandle(), WM_NCLBUTTONDOWN, HTCAPTION, 0);
    }

    dl->AddRectFilled(wPos, ImVec2(wPos.x + width, wPos.y + 40), IM_COL32(0x2D, 0x2D, 0x44, 255));
    dl->AddRectFilledMultiColor(
        ImVec2(wPos.x, wPos.y + 38), ImVec2(wPos.x + width, wPos.y + 40),
        IM_COL32(0x6C, 0x63, 0xFF, 0), IM_COL32(0x6C, 0x63, 0xFF, 0),
        IM_COL32(0x6C, 0x63, 0xFF, 80), IM_COL32(0x6C, 0x63, 0xFF, 80));

    if (g_Window && g_Window->FontTitle && g_Window->FontSmall) {
        ImGui::PushFont(g_Window->FontTitle);
        ImGui::SetCursorPos(ImVec2(14, 10));
        ImGui::TextColored(ImVec4(0.42f, 0.39f, 1.0f, 1), "PaperScents");

        ImGui::SameLine(0, 4);
        ImGui::PushFont(g_Window->FontSmall);
        ImGui::TextColored(ImVec4(0.53f, 0.53f, 0.67f, 1), "injector");
        ImGui::PopFont();
        ImGui::PopFont();
    }

    ImGui::SetCursorPos(ImVec2(width - 72, 6));
    if (g_Window && g_Window->FontSmall) ImGui::PushFont(g_Window->FontSmall);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.53f, 0.53f, 0.67f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.53f, 0.53f, 0.67f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4);

    if (ImGui::Button(" _ ", ImVec2(28, 26))) {
        ShowWindow(GetActiveWindow(), SW_MINIMIZE);
    }
    ImGui::SameLine(0, 2);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.26f, 0.21f, 0.8f));
    if (ImGui::Button(" X ", ImVec2(28, 26))) {
        cfg.WindowX = (int)ImGui::GetWindowPos().x;
        cfg.WindowY = (int)ImGui::GetWindowPos().y;
        cfg.WindowW = (int)ImGui::GetWindowSize().x;
        cfg.WindowH = (int)ImGui::GetWindowSize().y;
        ConfigManager::Get().Save();
        PostQuitMessage(0);
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    if (g_Window && g_Window->FontSmall) ImGui::PopFont();
}

static void RenderStatusBar(float width) {
    auto& cfg = ConfigManager::Get().GetConfig();
    float t = (float)ImGui::GetTime();
    float pulse = (sinf(t * 2.0f) + 1.0f) * 0.5f;

    ImU32 dotColor;
    const char* statusText;
    if (s_Injecting) {
        dotColor = IM_COL32(0xFF, 0xA7, 0x26, 255);
        statusText = "Injecting...";
    } else if (s_Processes.empty()) {
        dotColor = IM_COL32(0xE0, 0x40, 0x40, (int)(180 + 75 * pulse));
        statusText = "No target";
    } else if (s_InjectedPIDs.empty()) {
        dotColor = IM_COL32(0x4C, 0xAF, 0x50, (int)(180 + 75 * pulse));
        statusText = "Ready";
    } else {
        dotColor = IM_COL32(0x4C, 0xAF, 0x50, 255);
        statusText = "Injected";
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wPos = ImGui::GetWindowPos();

    ImVec2 dotCenter(wPos.x + 18, wPos.y + 18);
    dl->AddCircleFilled(dotCenter, 5, dotColor);
    dl->AddCircle(dotCenter, 8, IM_COL32(0x6C, 0x63, 0xFF, 60), 0, 2.0f);

    if (g_Window && g_Window->FontBold) ImGui::PushFont(g_Window->FontBold);
    ImGui::SetCursorPos(ImVec2(32, 9));
    ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.92f, 1), statusText);
    if (g_Window && g_Window->FontBold) ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(width - 156, 8));
    ImGui::TextColored(ImVec4(0.53f, 0.53f, 0.67f, 1), "Auto-Inject");

    ImGui::SameLine(width - 76);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImVec2 toggleSize(48, 24);
    ImGui::SetCursorPos(ImVec2(width - 70, 4));

    bool autoVal = cfg.AutoInject;
    ImGui::PushStyleColor(ImGuiCol_FrameBg,
        autoVal ? ImVec4(0.42f, 0.39f, 1.0f, 1) : ImVec4(0.27f, 0.27f, 0.40f, 1));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
        autoVal ? ImVec4(0.48f, 0.45f, 1.0f, 1) : ImVec4(0.30f, 0.30f, 0.45f, 1));
    if (ImGui::Checkbox("##autoinject", &autoVal)) {
        cfg.AutoInject = autoVal;
        ConfigManager::Get().MarkDirty();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    ImGui::SameLine(width - 30);
    if (ImGui::Button("Log", ImVec2(28, 24))) {
        cfg.LogVisible = !cfg.LogVisible;
        ConfigManager::Get().MarkDirty();
    }
}

static void RenderProcessList(float width, float height) {
    auto& cfg = ConfigManager::Get().GetConfig();
    float t = (float)ImGui::GetTime();

    ImGui::SetCursorPos(ImVec2(12, 12));
    if (g_Window && g_Window->FontNormal) ImGui::PushFont(g_Window->FontNormal);
    ImGui::SetNextItemWidth(width - 90);
    if (ImGui::InputTextWithHint("##search", "Search processes...", s_SearchBuf, sizeof(s_SearchBuf),
        ImGuiInputTextFlags_AutoSelectAll)) {
        s_SearchFilter = s_SearchBuf;
    }

    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.39f, 1.0f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.45f, 1.0f, 1));
    if (ImGui::Button("Scan", ImVec2(56, 24))) {
        s_ScanRequested = true;
        Logger::Get().Info("Scanning for processes...");
    }
    ImGui::PopStyleColor(2);

    ImGui::SetCursorPos(ImVec2(12, 42));
    float listH = height - 56;
    ImGui::BeginChild("ProcList", ImVec2(width - 24, listH), false,
        ImGuiWindowFlags_NoScrollbar);

    if (s_ScanRequested) {
        s_ScanRequested = false;

        auto fresh = ProcessManager::Scan();

        // Keep only injected PIDs that still exist
        std::vector<int> stillInjected;
        for (int pid : s_InjectedPIDs) {
            for (auto& p : fresh) {
                if ((int)p.PID == pid) { stillInjected.push_back(pid); break; }
            }
        }
        s_InjectedPIDs = stillInjected;

        // Auto-inject into any new processes
        if (cfg.AutoInject && !s_SelectedDllPath.empty()) {
            for (auto& p : fresh) {
                bool already = false;
                for (int ip : s_InjectedPIDs) {
                    if (ip == (int)p.PID) { already = true; break; }
                }
                if (!already) {
                    std::string err;
                    Logger::Get().Info("Auto-injecting: " + p.Name + " (PID:" + std::to_string(p.PID) + ")");
                    if (ProcessManager::Inject(p.PID, s_SelectedDllPath, err)) {
                        s_InjectedPIDs.push_back(p.PID);
                        Logger::Get().Success("Injected PID " + std::to_string(p.PID));
                    } else {
                        Logger::Get().Error("Auto-inject failed PID " + std::to_string(p.PID) + ": " + err);
                    }
                }
            }
        }

        s_Processes = std::move(fresh);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wPos = ImGui::GetWindowPos();
    ImVec2 cPos = ImGui::GetCursorScreenPos();
    float rowH = 32;

    std::string filter = s_SearchFilter;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    int idx = 0;
    for (auto& proc : s_Processes) {
        std::string nameLower = proc.Name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        if (!filter.empty() && nameLower.find(filter) == std::string::npos) {
            idx++;
            continue;
        }

        bool isInjected = false;
        for (int ip : s_InjectedPIDs) { if (ip == (int)proc.PID) { isInjected = true; break; } }

        float slideOffset = (float)(1.0 - std::min(1.0, (t - s_LaunchTime - idx * 0.03) * 3.0));
        float itemX = wPos.x + 12 + slideOffset * 60;

        ImRect rowRect(itemX, cPos.y, itemX + width - 24, cPos.y + rowH);

        bool hovered = ImGui::IsMouseHoveringRect(rowRect.Min, rowRect.Max);
        if (hovered) {
            dl->AddRectFilled(rowRect.Min, rowRect.Max, IM_COL32(0x6C, 0x63, 0xFF, 25), 6);
        }

        if (g_Window && g_Window->FontNormal) ImGui::PushFont(g_Window->FontNormal);
        ImGui::SetCursorScreenPos(ImVec2(itemX + 10, cPos.y + 7));

        ImU32 textColor = isInjected ? IM_COL32(0x4C, 0xAF, 0x50, 255) :
            (hovered ? IM_COL32(0xEA, 0xEA, 0xEA, 255) : IM_COL32(0xBB, 0xBB, 0xCC, 255));
        ImGui::TextColored(ImColor(textColor), "%s", proc.Name.c_str());

        ImGui::SameLine(itemX + 280);
        ImGui::TextColored(ImVec4(0.53f, 0.53f, 0.67f, 1), "PID: %lu", proc.PID);

        ImGui::SameLine(itemX + 380);
        if (isInjected) {
            ImGui::TextColored(ImVec4(0.30f, 0.69f, 0.31f, 1), "Injected");
        } else if (hovered && !s_Injecting) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.39f, 1.0f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.45f, 1.0f, 1));
            if (ImGui::SmallButton("Inject")) {
                if (!s_SelectedDllPath.empty()) {
                    s_Injecting = true;
                    std::string err;
                    Logger::Get().Info("Injecting into " + proc.Name + " (PID:" + std::to_string(proc.PID) + ")");
                    if (ProcessManager::Inject(proc.PID, s_SelectedDllPath, err)) {
                        s_InjectedPIDs.push_back(proc.PID);
                        Logger::Get().Success("Injected into PID " + std::to_string(proc.PID));
                    } else {
                        Logger::Get().Error("Injection failed: " + err);
                    }
                    s_Injecting = false;
                } else {
                    Logger::Get().Warning("No DLL selected.");
                }
            }
            ImGui::PopStyleColor(2);
        } else {
            ImGui::TextColored(ImVec4(0.53f, 0.53f, 0.67f, 1), "Ready");
        }

        if (g_Window && g_Window->FontNormal) ImGui::PopFont();

        cPos.y += rowH;
        ImGui::SetCursorScreenPos(cPos);
        idx++;
    }

    ImGui::EndChild();
}

static void RenderBottomBar(float width) {
    auto& cfg = ConfigManager::Get().GetConfig();

    ImGui::SetCursorPos(ImVec2(12, 8));

    if (g_Window && g_Window->FontSmall) ImGui::PushFont(g_Window->FontSmall);
    std::string dllLabel = s_SelectedDllPath.empty() ? "No DLL selected" : s_SelectedDllPath;
    if (dllLabel.length() > 50) dllLabel = "..." + dllLabel.substr(dllLabel.length() - 47);
    ImGui::TextColored(ImVec4(0.53f, 0.53f, 0.67f, 1), "%s", dllLabel.c_str());
    if (g_Window && g_Window->FontSmall) ImGui::PopFont();

    ImGui::SameLine(width - 180);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.27f, 0.27f, 0.44f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.33f, 0.33f, 0.53f, 1));
    if (ImGui::Button("Select DLL", ImVec2(78, 28))) {
        OpenFileDialog();
        if (!s_SelectedDllPath.empty())
            Logger::Get().Info("Selected DLL: " + s_SelectedDllPath);
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(width - 96);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.39f, 1.0f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.45f, 1.0f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.32f, 0.85f, 1));
    if (ImGui::Button("Inject All", ImVec2(78, 28))) {
        if (s_SelectedDllPath.empty()) {
            Logger::Get().Warning("No DLL selected.");
        } else {
            s_Injecting = true;
            for (auto& p : s_Processes) {
                bool already = false;
                for (int ip : s_InjectedPIDs) { if (ip == (int)p.PID) { already = true; break; } }
                if (already) continue;
                std::string err;
                if (ProcessManager::Inject(p.PID, s_SelectedDllPath, err)) {
                    s_InjectedPIDs.push_back(p.PID);
                    Logger::Get().Success("Injected into " + p.Name + " (PID:" + std::to_string(p.PID) + ")");
                } else {
                    Logger::Get().Error("Failed " + p.Name + ": " + err);
                }
            }
            s_Injecting = false;
        }
    }
    ImGui::PopStyleColor(3);
}

static void RenderLogWindow() {
    auto& cfg = ConfigManager::Get().GetConfig();
    if (!cfg.LogVisible) return;

    ImGui::SetNextWindowSize(ImVec2(580, 160), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(
        (float)cfg.WindowX + 10, (float)cfg.WindowY + (float)cfg.WindowH - 170), ImGuiCond_FirstUseEver);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.18f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.18f, 0.18f, 0.27f, 1));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.18f, 0.18f, 0.27f, 1));

    if (ImGui::Begin("Log", &cfg.LogVisible, ImGuiWindowFlags_NoCollapse)) {
        auto& entries = Logger::Get().GetEntries();

        ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

        for (size_t i = 0; i < entries.size(); i++) {
            auto& e = entries[i];
            const char* label = "";
            ImVec4 color;
            switch (e.Level) {
            case LogInfo:    label = "[INFO]";    color = ImVec4(0.53f, 0.53f, 0.67f, 1); break;
            case LogSuccess: label = "[OK]";      color = ImVec4(0.30f, 0.69f, 0.31f, 1); break;
            case LogWarning: label = "[WARN]";    color = ImVec4(1.00f, 0.65f, 0.15f, 1); break;
            case LogError:   label = "[ERR]";     color = ImVec4(0.96f, 0.26f, 0.21f, 1); break;
            }
            ImGui::TextColored(color, "%s", label);
            ImGui::SameLine();
            ImGui::TextUnformatted(e.Message.c_str());
        }

        if (s_LogScrollToBottom > 0) {
            ImGui::SetScrollHereY(1.0f);
            s_LogScrollToBottom -= 0.01f;
            if (s_LogScrollToBottom < 0) s_LogScrollToBottom = 0;
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}

void RenderUI() {
    auto& cfg = ConfigManager::Get().GetConfig();
    float t = (float)ImGui::GetTime();

    if (s_LaunchTime == 0.0) {
        s_LaunchTime = t;
        if (!cfg.LastDllPath.empty()) {
            s_SelectedDllPath = cfg.LastDllPath;
        }
    }

    float fadeAlpha = (float)std::min(1.0, (t - s_LaunchTime) * 3.0);
    if (fadeAlpha > 0.999f) fadeAlpha = 1.0f;

    ImGui::GetStyle().Alpha = fadeAlpha;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.18f, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.27f, 0.27f, 0.40f, 1));

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)cfg.WindowW, (float)cfg.WindowH));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("MainWindow", nullptr, flags)) {
        float w = ImGui::GetWindowWidth();
        float h = ImGui::GetWindowHeight();

        RenderTitleBar(w);

        ImGui::SetCursorPos(ImVec2(0, 44));
        ImGui::BeginChild("StatusBar", ImVec2(w, 32), false, ImGuiWindowFlags_NoScrollbar);
        RenderStatusBar(w);
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(0, 80));
        float midH = h - 144;
        ImGui::BeginChild("MidSection", ImVec2(w, midH), false, ImGuiWindowFlags_NoScrollbar);
        RenderProcessList(w, midH);
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(0, h - 56));
        ImGui::BeginChild("BottomBar", ImVec2(w, 56), false, ImGuiWindowFlags_NoScrollbar);
        RenderBottomBar(w);
        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

    RenderLogWindow();

    static double lastPerScan = 0;
    double scanInterval = cfg.AutoInject ? 2.0 : 3.0;
    if (t - lastPerScan > scanInterval) {
        s_ScanRequested = true;
        lastPerScan = t;
    }

    if (ConfigManager::Get().GetConfig().AutoInject != cfg.AutoInject) {
        ConfigManager::Get().MarkDirty();
    }

    static double lastSave = 0;
    if (t - lastSave > 5.0) {
        cfg.WindowX = (int)ImGui::GetWindowPos().x;
        cfg.WindowY = (int)ImGui::GetWindowPos().y;
        cfg.WindowW = (int)ImGui::GetWindowSize().x;
        cfg.WindowH = (int)ImGui::GetWindowSize().y;
        ConfigManager::Get().Save();
        lastSave = t;
    }
}
