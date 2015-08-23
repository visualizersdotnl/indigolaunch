
/*
	INDIGO Jukebox (a Dutch Game Garden initiative)
	by visualizers.nl (http://www.visualizers.nl)

	target platform: Windows 7 (Direct3D)

	third party:
	- pugixml by Arseny Kapoulkine
	- BASS audio library by Ian Luck

	compiler settings:
	- multi-threaded CRT (static)
	- target: x86 (64-bit may work, but untested)
	- optional: disable all C++ exceptions (except for /3rdparty/pugixml-1.2/src/pugixml.cpp)
	  ^ FIXME?
	- Unicode character set
*/

#include "platform.h"
#include <intrin.h> // for SIMD check
#include <dxgi.h>
#include <xinput.h>
#include "audio.h"
#include "inputcallback.h"
// #include "collection.h"
// #include "renderer.h"
#include "flow.h"
#include "timer.h"

// VS resources
#include "../VS/resource.h"

// configuration: windowed (dev. only) / full screen
const bool kWindowed = true;

// default (deaf) input callback
static void DefaultInputCallback(Input input, void *pContext) {}

// current input callback & context
static InputCallback s_pInputCallback = DefaultInputCallback;
static void *s_pInputContext = nullptr;

// hack: X360 controller(s) connected?
bool g_padConnected = false;

// global error message
static std::wstring s_lastError;
void SetLastError(const std::wstring &message) { s_lastError = message; }

// DXGI objects
static IDXGIFactory    *s_pDXGIFactory = nullptr;
static IDXGIAdapter    *s_pAdapter = nullptr;
static IDXGIOutput     *s_pDisplay = nullptr;
static DXGI_MODE_DESC   s_displayMode;

// app. window
static bool s_classRegged = false;
HWND        g_hWnd = NULL;
static bool s_wndIsActive; // set by WindowProc()

// Direct3D objects
static ID3D10Device   *s_pD3D = nullptr;
static IDXGISwapChain *s_pSwapChain = nullptr;

static bool CreateDXGI(HINSTANCE hInstance)
{
	if FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&s_pDXGIFactory)))
	{
		SetLastError(L"Can not create DXGI factory.");
		return false;
	}

	// get primary adapter
	s_pDXGIFactory->EnumAdapters(0, &s_pAdapter);
	if (nullptr == s_pAdapter)
	{
		SetLastError(L"No primary display adapter found.");
		return false;
	}

	// and it's display
	s_pAdapter->EnumOutputs(0, &s_pDisplay);
	if (nullptr == s_pDisplay)
	{
		SetLastError(L"No physical display attached to primary adapter.");
		return false;
	}

	// get current (desktop) display mode
	DXGI_MODE_DESC modeToMatch;
	modeToMatch.Width = GetSystemMetrics(SM_CXSCREEN);
	modeToMatch.Height = GetSystemMetrics(SM_CYSCREEN);
	modeToMatch.RefreshRate.Numerator = 0;
	modeToMatch.RefreshRate.Denominator = 0;
	modeToMatch.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	modeToMatch.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	modeToMatch.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	if FAILED(s_pDisplay->FindClosestMatchingMode(&modeToMatch, &s_displayMode, NULL))
	{
		SetLastError(L"Can not retrieve primary monitor's display mode.");
		return false;
	}

	if (kWindowed)
	{
		// override resolution: use a quarter of 1920*1080
		s_displayMode.Width  = 1920/2;
		s_displayMode.Height = 1080/2;
	}

	return true;
}

static void DestroyDXGI()
{
	SAFE_RELEASE(s_pDisplay);
	SAFE_RELEASE(s_pAdapter);
	SAFE_RELEASE(s_pDXGIFactory);
}

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0); // terminate message loop
		g_hWnd = NULL;      // DefWindowProc() will call DestroyWindow()
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			s_pInputCallback(kInputBack, s_pInputContext);
			break;
			
		case VK_RETURN:
			s_pInputCallback(kInputStart, s_pInputContext);
			break;

		case VK_UP:
		case VK_DOWN:
			break;
			
		case VK_LEFT:
			s_pInputCallback(kInputLeft, s_pInputContext);
			break;
			
		case VK_RIGHT:
			s_pInputCallback(kInputRight, s_pInputContext);
			break;
		}

		break;

	case WM_ACTIVATE:
		switch (LOWORD(wParam))
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			if (false == kWindowed) 
			{
				// (re)assign WS_EX_TOPMOST style
				SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
			
			s_wndIsActive = true;
			break;
		
		case WA_INACTIVE:
			if (false == kWindowed) 
			{
				// push window to bottom of the Z order
				SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
			
			s_wndIsActive = false;
			break;
		};

	case WM_SIZE:
		break; // ALT+ENTER is blocked, all else is ignored or scaled if the window type permits it.
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static bool CreateAppWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INDIGO));
	wndClass.hCursor = NULL;
	wndClass.hbrBackground = (kWindowed) ? (HBRUSH) GetStockObject(BLACK_BRUSH) : NULL;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = L"RenderWindow";
	wndClass.hIconSm = NULL;
	
	if (0 == RegisterClassEx(&wndClass))
	{
		SetLastError(L"Can not create application window (RegisterClassEx() failed).");
		return false;
	}

	s_classRegged = true;
	
	DWORD windowStyle, exWindowStyle;
	if (kWindowed)
	{
		// windowed style
		windowStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU;
		exWindowStyle = 0;
	}
	else
	{
		// full screen style (WS_EX_TOPMOST assigned by WM_ACTIVATE)
		windowStyle = WS_POPUP;
		exWindowStyle = 0;
	}

	// calculate full window size
	RECT wndRect = { 0, 0, s_displayMode.Width, s_displayMode.Height };
	AdjustWindowRectEx(&wndRect, windowStyle, FALSE, exWindowStyle);
	const int wndWidth = wndRect.right - wndRect.left;
	const int wndHeight = wndRect.bottom - wndRect.top;

	g_hWnd = CreateWindowEx(
		exWindowStyle,
		L"RenderWindow",
		L"INDIGO Jukebox",
		windowStyle,
		0, 0, // always pop up on primary display's desktop area!
		wndWidth, wndHeight,
		NULL,
		NULL,
		hInstance,
		nullptr);	

	if (NULL == g_hWnd)
	{
		SetLastError(L"Can not create application window (CreateWindowEx() failed).");
		return false;
	}

	ShowWindow(g_hWnd, (kWindowed) ? nCmdShow : SW_SHOW);

	return true;
}

static bool UpdateAppWindow(bool &renderFrame)
{
	// skip frame unless otherwise specified
	renderFrame = false;

	// got a message to process?
	MSG msg;
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			// quit!
			return false;
		}
		
		// dispatch message
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	else
	{
		// no message: window alive?
		if (NULL != g_hWnd)
		{
			if (!kWindowed && s_wndIsActive)
			{
				// kill cursor for active full screen window
				SetCursor(NULL);
			}

			// render frame if windowed or full screen window has focus
			if (kWindowed || s_wndIsActive)
			{
				renderFrame = true;
			}
			else 
			{
				// full screen window out of focus: relinquish rest of time slice
				Sleep(0); 
			}
		}
	}
	
	// continue!
	return true;
}

void DestroyAppWindow(HINSTANCE hInstance)
{
	if (NULL != g_hWnd)
	{
		DestroyWindow(g_hWnd);
		g_hWnd = NULL;
	}	
	
	if (s_classRegged)
	{
		UnregisterClass(L"RenderWindow", hInstance);
	}
}

static bool CreateDirect3D()
{
	// create device
#if _DEBUG
	const UINT Flags = D3D10_CREATE_DEVICE_SINGLETHREADED | D3D10_CREATE_DEVICE_DEBUG;
#else
	const UINT Flags = D3D10_CREATE_DEVICE_SINGLETHREADED;
#endif

	if SUCCEEDED(D3D10CreateDevice(
		s_pAdapter,
		D3D10_DRIVER_TYPE_HARDWARE,
		NULL,
		Flags,
		D3D10_SDK_VERSION,
		&s_pD3D))
	{
		// create swap chain
		DXGI_SWAP_CHAIN_DESC swapDesc;
		swapDesc.BufferDesc = s_displayMode;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.BufferCount = 3;
		swapDesc.OutputWindow = g_hWnd;
		swapDesc.Windowed = kWindowed;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapDesc.Flags = 0; // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		if SUCCEEDED(s_pDXGIFactory->CreateSwapChain(s_pD3D, &swapDesc, &s_pSwapChain))
		{
			VERIFY(S_OK == s_pDXGIFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_WINDOW_CHANGES));
			return true;
		}
	}

	// either one failed, passing it off as the same error
	std::wstringstream message;
	message << L"Can't create Direct3D 10.0 device.\n\n";
	message << ((true == kWindowed) ? L"Type: windowed.\n" : L"Type: full screen.\n");
	message << L"Resolution: " << s_displayMode.Width << L"*" << s_displayMode.Width << L".";
	// FIXME: add reason (using DxErr).
	SetLastError(message.str());
	return false;
}

static void DestroyDirect3D()
{
	if (false == kWindowed && nullptr != s_pSwapChain)
		s_pSwapChain->SetFullscreenState(FALSE, nullptr);
	
	SAFE_RELEASE(s_pSwapChain);
	SAFE_RELEASE(s_pD3D);
}

#include "CPUID.h"

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	// change path to target root
	SetCurrentDirectory(L"..\\");

	// check for SSE2
	int cpuInfo[4];
	__cpuid(cpuInfo, 1);
	if (0 == (cpuInfo[3] & CPUID_FEAT_EDX_SSE2))
	{
		MessageBox(NULL, L"Processor does not support SSE2 instructions.", L"Error!", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	// initialize DXGI
	if (CreateDXGI(hInstance))
	{
		// create app. window
		if (CreateAppWindow(hInstance, nCmdShow))
		{
			// initialize BASS audio library
			if (Audio_Create(-1, g_hWnd))
			{
				// initialize Direct3D
				if (CreateDirect3D())
				{
					// create renderer
					Renderer renderer(*s_pD3D, *s_pSwapChain);
					if (renderer.Create())
					{
						// attach Renderer instance to HWND
						SetWindowLongPtr(g_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&renderer));

						// for now, create a single collection of titles (PC games)
						Collection collection(L"games");
						if (collection.Create(renderer))
						{
							// create flow
							Flow flow(collection, renderer);
							if (flow.Create())
							{
								Timer timer;
							
								// enter (render) loop
								bool renderFrame;
								while (true == UpdateAppWindow(renderFrame))
								{
									// update input callback
									s_pInputCallback = flow.GetInputCallback(&s_pInputContext);

									if (!flow.InLaunchMode())
									{
										// evaluate X360 controller input for UI
										XINPUT_KEYSTROKE keystroke;
										while (ERROR_SUCCESS == XInputGetKeystroke(XUSER_INDEX_ANY, XINPUT_FLAG_GAMEPAD, &keystroke))
										{
											// support repeat presses: L/R
											if (0 == (keystroke.Flags & ~(XINPUT_KEYSTROKE_KEYDOWN|XINPUT_KEYSTROKE_REPEAT)))
											{
												switch (keystroke.VirtualKey)
												{
												case VK_PAD_LTRIGGER:
												case VK_PAD_LSHOULDER:
												case VK_PAD_LTHUMB_LEFT:
												case VK_PAD_DPAD_LEFT:
													s_pInputCallback(kInputLeft, s_pInputContext);
													break;

												case VK_PAD_RTRIGGER:
												case VK_PAD_RSHOULDER:
												case VK_PAD_LTHUMB_RIGHT:
												case VK_PAD_DPAD_RIGHT:
													s_pInputCallback(kInputRight, s_pInputContext);
													break;
												}
											}

											// ignore repeat presses: START/BACK
											if (0 == (keystroke.Flags & ~XINPUT_KEYSTROKE_KEYDOWN)) // no XINPUT_KEYSTROKE_REPEAT!
											{
												switch (keystroke.VirtualKey)
												{
												case VK_PAD_START:
												case VK_PAD_A:
													s_pInputCallback(kInputStart, s_pInputContext);
													break;

												case VK_PAD_BACK:
												case VK_PAD_B:
													s_pInputCallback(kInputBack, s_pInputContext);
													break;
												}
											}
										}

										// check if any X360 controller is connected
										bool isConnected = false;
										for (DWORD dwUserIndex = 0; dwUserIndex < 4; ++dwUserIndex)
										{
											XINPUT_STATE state;
											if (ERROR_SUCCESS == XInputGetState(dwUserIndex, &state))
											{
												isConnected = true;
												break;
											}
										}
										
										// hack
										g_padConnected = isConnected;
									}
									else
									{
										// check if any X360 controller has the kill switch pressed
										for (DWORD dwUserIndex = 0; dwUserIndex < 4; ++dwUserIndex)
										{
											XINPUT_STATE state;
											if (ERROR_SUCCESS == XInputGetState(dwUserIndex, &state))
											{
												const XINPUT_GAMEPAD &pad = state.Gamepad;
												if (pad.wButtons & XINPUT_GAMEPAD_A &&
													pad.wButtons & XINPUT_GAMEPAD_B &&
													pad.wButtons & XINPUT_GAMEPAD_X &&
													pad.wButtons & XINPUT_GAMEPAD_Y &&
													pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER &&
													pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
												{
													s_pInputCallback(kInputKillSwitch, s_pInputContext);
												}
											}
										}
									}

									// update
									if (!flow.Update(renderFrame, timer.Get()))
									{
										// top-level UI closed: quit!
										break;
									}
									
									// frame rendered?
									if (true == renderFrame)
									{
										// yes: flip/blit
										const HRESULT hRes = s_pSwapChain->Present((true == kWindowed) ? 0 : 1, 0);
//										const HRESULT hRes = s_pSwapChain->Present(0, 0);
										// hack: ignoring DXGI_ERROR_DEVICE_RESET
									}
								}

								// fix me: check why I felt this is necessary
								D3DX10UnsetAllDeviceObjects(s_pD3D);
							}
						}

						// remove Renderer instance from HWND
						SetWindowLongPtr(g_hWnd, GWLP_USERDATA, NULL);
					}
				}

				DestroyDirect3D();
			}
			
			Audio_Destroy();
		}
		
		DestroyAppWindow(hInstance);
	}

	DestroyDXGI();

	if (!s_lastError.empty())
	{
		MessageBox(NULL, s_lastError.c_str(), L"Error!", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	return 0;
}
