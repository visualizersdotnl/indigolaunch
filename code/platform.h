
// indigolaunch -- main header (*always* include on top)

#pragma once

// ignore:
#pragma warning(disable:4530) // unwind semantics missing

// APIs
#include <windows.h>
#include <d3d10.h>
// #include <d3dx10.h> // FIXME

// CRT & STL
#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <algorithm>
#include <exception>
#include <memory>

// declared in main.cpp (hack)
extern bool g_padConnected;

// declared in main.cpp
void SetLastError(const std::wstring &message);

// declared in main.cpp
extern HWND g_hWnd;

// basic utilities
#include "util.h"
