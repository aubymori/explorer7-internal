#include "common.h"
#include "ImmersiveShell.h"
#include "dbgprint.h"

typedef HWND(WINAPI* GetTaskmanWindow)();
typedef BOOL(WINAPI* SetTaskmanWindow)(HWND hwnd);

GetTaskmanWindow GetTaskmanWindowFunc = NULL;
SetTaskmanWindow SetTaskmanWindowFunc = NULL;

UINT shellhook = 0;
IImmersiveShellHookService* ShellHookService;

LRESULT TaskmanWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		shellhook = RegisterWindowMessageW(L"SHELLHOOK");
		if (!shellhook)
		{
			dbgprintf(L"failed to register shellhook\n");
		}
		if (!SetTaskmanWindowFunc(hwnd))
		{
			dbgprintf(L"failed to register taskman window\n");
		}
		if (!RegisterShellHookWindow(hwnd))
		{
			dbgprintf(L"register shellhook window failed\n");
		}

	}
	else if (msg == WM_DESTROY)
	{
		if (GetTaskmanWindowFunc() == hwnd)
		{
			SetTaskmanWindowFunc(NULL);
		}
		DeregisterShellHookWindow(hwnd);
	}
	else
	{
		if (msg == shellhook && msg != WM_HOTKEY)
		{
			if (ShellHookService)
			{
				BOOL handle = TRUE;
				if ((UINT)wParam == 12)
				{
					ShellHookService->SetTargetWindowForSerialization((HWND)lParam);
				}
				else if ((UINT)wParam == 0x32)
				{
					handle = FALSE;
				}
				else if ((UINT)wParam == 7) 
				{
					// This seems to be the correct way of handling ShellHook under immersive shell.
					// It also removes the need to hook twinui.pcshell directly, so less hooking is needed.
					handle = FALSE;
					HWND hwnd_taskbar = FindWindow(L"Shell_TrayWnd", NULL);
					PostMessageW(hwnd_taskbar, 0x504, 0, 0);
				}
				if (handle)
				{
					ShellHookService->PostShellHookMessage(wParam, lParam);
				}
				return S_OK;
			}

			IServiceProvider* serviceProvider; 
			if (CoCreateInstance(_GUID_c2f03a33_21f5_47fa_b4bb_156362a2f239, nullptr, CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD, IID_PPV_ARGS(&serviceProvider)) >= 0)
			{
				serviceProvider->QueryService(SID_ImmersiveShellHookService, IID_PPV_ARGS(&ShellHookService));
			}
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CreateTaskmanWindow()
{
	// create taskman class (handles taskbar buttons)
	WNDCLASSEX taskmanclass = {};

	taskmanclass.cbClsExtra = 0;
	taskmanclass.hIcon = 0;
	taskmanclass.lpszMenuName = 0;
	taskmanclass.hIconSm = 0;
	taskmanclass.cbSize = sizeof(WNDCLASSEXW);
	taskmanclass.style = 8;
	taskmanclass.lpfnWndProc = (WNDPROC)TaskmanWndProc;
	taskmanclass.cbWndExtra = 8;
	taskmanclass.hInstance = GetModuleHandle(NULL);
	taskmanclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	taskmanclass.hbrBackground = (HBRUSH)2;
	taskmanclass.lpszClassName = TEXT("TaskmanWndClass");

	if (!RegisterClassExW(&taskmanclass))
	{
		return;
	}
	auto Taskman = CreateWindowExW(0, L"TaskmanWndClass", NULL, 0x82000000, 0, 0, 0, 0, 0, 0, 0, 0);
}

void InitializeImmersiveController()
{
	HMODULE user32 = LoadLibrary(L"user32.dll");
	GetTaskmanWindowFunc = (GetTaskmanWindow)GetProcAddress(user32, "GetTaskmanWindow");
	SetTaskmanWindowFunc = (SetTaskmanWindow)GetProcAddress(user32, "SetTaskmanWindow");

	CreateTaskmanWindow();

	IImmersiveShellBuilder* immersiveShellBuilder;
	HRESULT hr = CoCreateInstance(CLSID_ImmersiveShellBuilder, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&immersiveShellBuilder));
	if (SUCCEEDED(hr))
	{
		dbgprintf(L"TwinUI factory created!");

		IImmersiveShellController* immersiveShellController;
		hr = immersiveShellBuilder->CreateImmersiveShellController(&immersiveShellController);
		dbgprintf(L"TwinUI instance created %p %p", hr, immersiveShellController);
		if (SUCCEEDED(hr))
		{
			hr = immersiveShellController->Start();

			dbgprintf(L"Immersive Shell Controller Result: %x", hr);
		}
		immersiveShellController->Release();
	}
}
