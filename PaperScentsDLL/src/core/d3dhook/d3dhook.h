#pragma once

#include <Windows.h>

bool InitD3DHook();
void ShutdownD3DHook();
HWND GetGameWindowHandle();
