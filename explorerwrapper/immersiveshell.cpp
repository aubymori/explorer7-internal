#include "immersiveshell.h"
#include "dbgprint.h"

typedef HWND(WINAPI* GetTaskmanWindow)();
typedef BOOL(WINAPI* SetTaskmanWindow)(HWND handle);
typedef HRESULT(CALLBACK* SetShellWindow)(HWND hwnd);

GetTaskmanWindow GetTaskmanWindowFunc = NULL;
SetTaskmanWindow SetTaskmanWindowFunc = NULL;
SetShellWindow SetShellWindowFunc = NULL;

UINT shellhook = 0;
IImmersiveShellHookService* ShellHookService;

static bool successfullySetShellWindow = false;

DWORD WINAPI TwinThread( LPVOID lpParameter )
{
	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
	IImmersiveBehavior* behavior;
	HRESULT ret = CoUnmarshalInterface((IStream*)lpParameter, IID_ImmersiveBehavior, (PVOID*)&behavior);
	dbgprintf(L"IImmersiveBehavior %p %p",ret,behavior);
	UINT count;	
	behavior->GetMaximumComponentCount(&count);
	UINT i;
	for (i=0;i<count;i++)
	{
		dbgprintf(L"creating TwinUI component %d",i);
		IUnknown* component;
		HRESULT ret = behavior->CreateComponent(i,&component);
		dbgprintf(L"created TwinUI component %p %p",ret,component);
	}
	return 0;
}


LRESULT TaskmanWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
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
		if (msg != shellhook && msg != WM_HOTKEY)
		{
			return DefWindowProc(hwnd, msg, w, l);
		}

		if (ShellHookService)
		{
			BOOL handle = TRUE;
			if ((UINT)w == 12)
			{
				ShellHookService->SetTargetWindowForSerialization((HWND)l);
			}
			else if ((UINT)w == 0x32)
			{
				handle = FALSE;
			}
			if (handle)
			{
				ShellHookService->PostShellHookMessage(w, l);
			}
			return 0;
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
			if (FAILED(ImmersiveShell->QueryService(SID_ImmersiveShellHookService, SID_Unknown, (void**)&ShellHookService)))
			{
				
			}
		}
	}
	return DefWindowProc(hwnd, msg, w, l);
}

void CreateTaskManWindow()
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

void SetProgmanAsShell()
{
	BOOL res = false;
	HRESULT err;
	if (SetShellWindowFunc && !successfullySetShellWindow)
	{
		HWND progMan = FindWindow(TEXT("Progman"), TEXT("Program Manager"));

		if (SetShellWindowFunc(progMan))
		{
			successfullySetShellWindow = true;
		}
	}
}

void CreateTwinUI()
{
	auto user32 = LoadLibrary(TEXT("user32.dll"));
	GetTaskmanWindowFunc = (GetTaskmanWindow)GetProcAddress(user32, "GetTaskmanWindow");
	SetTaskmanWindowFunc = (SetTaskmanWindow)GetProcAddress(user32, "SetTaskmanWindow");
	SetShellWindowFunc = (SetShellWindow)GetProcAddress(user32, "SetShellWindow");


	CreateTaskManWindow();

	IImmersiveShellCreator* ImmersiveShellCreator;
	if ( SUCCEEDED(CoCreateInstance(CLSID_ImmersiveShellBuilder,NULL, CLSCTX_INPROC_SERVER, IID_ImmersiveShellBuilder, (LPVOID*)&ImmersiveShellCreator)))
	{
		dbgprintf(L"TwinUI factory created!");

		IImmersiveShellController* controller;
		HRESULT ret = ImmersiveShellCreator->CreateShell(&controller);
		dbgprintf(L"TwinUI instance created %p %p", ret, controller);
		if ( SUCCEEDED(ret) )
		{	
			HRESULT hr = controller->Start();

			dbgprintf(L"Immersive Shell Controller Result: %x", hr);
		}
	}
}