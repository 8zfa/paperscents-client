# PaperScents Client

**This project is for educational purposes only.** Use at your own risk.

Fully AI-coded Minecraft 1.8.9 ghost client for Lunar Client. Built with C++17, ImGui, MinHook, and JNI.

## Structure

- `PaperScentsDLL/` — Core DLL injected into the Minecraft process
- `PaperScentsInjector/` — Injector application
- `ext/` — Dependencies (ImGui, MinHook, nlohmann/json)
- `scripts/` — Build/deploy utilities

## Build

Requires Visual Studio 2022 Build Tools with MSVC v145 and Windows 10 SDK. Built output goes to `build/x64/Release/PaperScentsDLL.dll`.
