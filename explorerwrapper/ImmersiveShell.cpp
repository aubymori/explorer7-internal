#include "common.h"
#include "ImmersiveShell.h"
#include "dbgprint.h"

typedef HWND(WINAPI* GetTaskmanWindow)();
typedef BOOL(WINAPI* SetTaskmanWindow)(HWND handle);

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

			GUID guidImmersiveShell;
			CLSIDFromString(L"{c2f03a33-21f5-47fa-b4bb-156362a2f239}", &guidImmersiveShell);

			GUID SID_ImmersiveShellHookService;
			CLSIDFromString(L"{4624bd39-5fc3-44a8-a809-163a836e9031}", &SID_ImmersiveShellHookService);

			GUID SID_Unknown;
			CLSIDFromString(L"{914d9b3a-5e53-4e14-bbba-46062acb35a4}", &SID_Unknown);

			IServiceProvider* ImmersiveShell;
			if (CoCreateInstance(guidImmersiveShell, 0, 0x404u, IID_IServiceProvider, (LPVOID*)&ImmersiveShell) >= 0)
			{
				ImmersiveShell->QueryService(SID_ImmersiveShellHookService, SID_Unknown, (void**)&ShellHookService);
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

	IImmersiveShellCreator* ImmersiveShellCreator;
	HRESULT hr = CoCreateInstance(CLSID_ImmersiveShellBuilder, NULL, CLSCTX_INPROC_SERVER, IID_ImmersiveShellBuilder, (LPVOID*)&ImmersiveShellCreator);
	if (SUCCEEDED(hr))
	{
		dbgprintf(L"TwinUI factory created!");

		IImmersiveShellController* controller;
		hr = ImmersiveShellCreator->CreateShell(&controller);
		dbgprintf(L"TwinUI instance created %p %p", hr, controller);
		if (SUCCEEDED(hr))
		{
			hr = controller->Start();

			dbgprintf(L"Immersive Shell Controller Result: %x", hr);
		}
	}
}
