#include "ImmersiveShell.h"
#include "dbgprint.h"
#include "OptionConfig.h"

typedef HWND(WINAPI* GetTaskmanWindow)();
typedef BOOL(WINAPI* SetTaskmanWindow)(HWND handle);

typedef BOOL(*RegisterShellHook_t)(HWND, DWORD); // 181

GetTaskmanWindow GetTaskmanWindowFunc = NULL;
SetTaskmanWindow SetTaskmanWindowFunc = NULL;

UINT shellhook = 0;
IImmersiveShellHookService* m_shellHookService;

IImmersiveShellController* m_immersiveShellController;

IClassicWindowManager* m_classicWindowManager;
IImmersiveApplicationNotificationService* m_immersiveApplicationNotificationService;
IImmersiveAppCrusher* m_immersiveAppCrusher;
IImmersiveApplicationManager* m_immersiveApplicationManager;
IImmersiveApplicationArrayService* m_immersiveApplicationArrayService;
IApplicationDataPersistence* m_applicationDataPersistence;
IApplicationViewCollection* m_applicationViewCollection;
IPinManagerInterop* m_pinManager;

DWORD m_immersiveApplicationNotificationToken;
DWORD m_immersiveAppCrusherNotificationToken;

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

BOOL RegisterShellHook(HWND hwnd, DWORD dwType)
{
	static RegisterShellHook_t fn = nullptr;
	if (!fn)
	{
		HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");
		if (hShell32)
			fn = (RegisterShellHook_t)GetProcAddress(hShell32, MAKEINTRESOURCEA(181));
	}
	return fn(hwnd, dwType);
}

LRESULT TaskmanWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
		switch (uMsg)
		{
			case WM_CREATE:
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
				if (!RegisterShellHook(hwnd, 3))
				{
					dbgprintf(L"RegisterShellHook failed\n");
				}
				return 0;
			}
			case WM_DESTROY:
			{
				if (GetTaskmanWindowFunc() == hwnd)
				{
					SetTaskmanWindowFunc(NULL);
				}
				RegisterShellHook(hwnd, 0);
				return 0;
			}
			case WM_USER + 0x3C:
			{
				if (IsWindow((HWND)(UINT_PTR)(int)lParam) && m_shellHookService)
				{
					m_shellHookService->PostShellHookMessage(wParam ? 53 : 54, lParam);
				}
				return 0;
			}
		}

		if (shellhook && uMsg == shellhook)
		{
			//LRESULT lRes = _HandleShellHook((int)wParam, lParam);

			LRESULT lRes = 0;

			/*if (m_applicationUsageTracker)
			{
				m_applicationUsageTracker->OnShellHookMessage(wParam, lParam);
			}*/

			if (m_shellHookService)
			{
				if (wParam == 38)
				{
					// if (GetPropW((HWND)lParam, L"WindowShouldReportLayoutCompletedTelemetryProp")) ...
				}

				if (wParam != 0x32)
				{
					m_shellHookService->PostShellHookMessage(wParam, lParam);
				}
				return lRes;
			}

		}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
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

void CreateTwinUI()
{
	PVOID pv;
	if (SUCCEEDED(CoCreateInstance(CLSID_ImmersiveShellBuilder, NULL, 1, IID_ImmersiveShellBuilder, &pv)))
	{
		dbgprintf(L"TwinUI factory created!");
		IImmersiveShellCreator* ImmersiveShellCreator = (IImmersiveShellCreator*)pv;
		IImmersiveShellController* controller;
		HRESULT ret = ImmersiveShellCreator->CreateShell(&controller);
		dbgprintf(L"TwinUI instance created %p %p", ret, controller);
		if (SUCCEEDED(ret))
		{
			//HRESULT ret = controller->Start();
			IStream* someinterface = (IStream*)*(DWORD*)((DWORD)controller + 0x34);
			IImmersiveBehavior* behavior;
			CoUnmarshalInterface((IStream*)someinterface, IID_ImmersiveBehavior, (PVOID*)&behavior);
			controller->SetCreationBehavior(new CImmersiveBehaviorWrapper(behavior));
			controller->Start();
			/*CreateThread(NULL,0,TwinThread,(PVOID)someinterface,0,NULL);*/
		}
		/*ret = CoCreateInstance(CLSID_ImmersiveShell,NULL,0x404,IID_ImmersiveShell,&pv);
		dbgprintf(L"Immersive Shell created: %p",ret);*/
	}
}

HWND v_hwndTray;


static HWND GetTrayWnd()
{
	if (!v_hwndTray)
		v_hwndTray = FindWindow(L"Shell_TrayWnd", NULL);
	return v_hwndTray;
}

void CreateTwinUI_UWP()
{
	if (s_ImmersiveShell == 1)
	{
		auto user32 = LoadLibrary(TEXT("user32.dll"));
		GetTaskmanWindowFunc = (GetTaskmanWindow)GetProcAddress(user32, "GetTaskmanWindow");
		SetTaskmanWindowFunc = (SetTaskmanWindow)GetProcAddress(user32, "SetTaskmanWindow");

		CreateTaskManWindow();
	}

	//IImmersiveShellBuilder* immersiveShellBuilder;
	//if (SUCCEEDED(CoCreateInstance(CLSID_ImmersiveShellBuilder, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&immersiveShellBuilder))))
	//{
	//	SUCCEEDED(immersiveShellBuilder->CreateImmersiveShellController(&m_immersiveShellController)); // Outputted to telemetry
	//}

	IImmersiveShellCreator* ImmersiveShellCreator;
	if (SUCCEEDED(CoCreateInstance(CLSID_ImmersiveShellBuilder, NULL, CLSCTX_INPROC_SERVER, IID_ImmersiveShellBuilder, (LPVOID*)&ImmersiveShellCreator)))
	{
		dbgprintf(L"TwinUI factory created!");

		IImmersiveShellController* controller;
		HRESULT ret = ImmersiveShellCreator->CreateShell(&controller);
		dbgprintf(L"TwinUI instance created %p %p", ret, controller);
		if (SUCCEEDED(ret))
		{
			//controller->SetCreationBehavior((IImmersiveBehavior*)136);
			HRESULT hr = controller->Start();
			if (SUCCEEDED(hr))
			{
				GUID guidImmersiveShell;
				CLSIDFromString(L"{c2f03a33-21f5-47fa-b4bb-156362a2f239}", &guidImmersiveShell);

				GUID SID_ImmersiveShellHookService;
				CLSIDFromString(L"{4624bd39-5fc3-44a8-a809-163a836e9031}", &SID_ImmersiveShellHookService);

				GUID SID_Unknown;
				CLSIDFromString(L"{914d9b3a-5e53-4e14-bbba-46062acb35a4}", &SID_Unknown);

				IServiceProvider* serviceProvider;
				if (CoCreateInstance(guidImmersiveShell, nullptr, CLSCTX_NO_CODE_DOWNLOAD | CLSCTX_LOCAL_SERVER,
					IID_IServiceProvider, (LPVOID*)&serviceProvider) >= 0)
				{
					serviceProvider->QueryService(SID_ImmersiveShellHookService, SID_Unknown, (void**)&m_shellHookService);
					if (m_shellHookService && s_ImmersiveShell == 1)
						m_shellHookService->SetTargetWindowForSerialization(FindWindow(L"Shell_TrayWnd", NULL));

					serviceProvider->QueryService(IID_IClassicWindowManager, SID_Unknown, (void**)&m_classicWindowManager);
					serviceProvider->QueryService(IID_IImmersiveApplicationNotificationService, SID_Unknown, (void**)&m_immersiveApplicationNotificationService);
					serviceProvider->QueryService(SID_AppCrusher, SID_Unknown, (void**)&m_immersiveAppCrusher);
					serviceProvider->QueryService(IID_IImmersiveApplicationManager, SID_Unknown, (void**)&m_immersiveApplicationManager);
					serviceProvider->QueryService(SID_ImmersiveApplicationArrayService, SID_Unknown, (void**)&m_immersiveApplicationArrayService);
					serviceProvider->QueryService(__uuidof(IApplicationDataPersistence), SID_Unknown, (void**)&m_applicationDataPersistence);

					serviceProvider->QueryService(__uuidof(IApplicationViewCollection), SID_Unknown, (void**)&m_applicationViewCollection);

					if (g_osVersion.BuildNumber() >= 19045 || g_osVersion.BuildNumber() >= 22631)
					{
						serviceProvider->QueryService(SID_PinManager, SID_Unknown, (void**)&m_pinManager);
					}

					serviceProvider->Release();
				}
				else
				{

					HRESULT hr = 0;
					if (!serviceProvider)
					{
						hr = CoCreateInstance(
							guidImmersiveShell,
							nullptr,
							CLSCTX_NO_CODE_DOWNLOAD | CLSCTX_LOCAL_SERVER,
							__uuidof(IServiceProvider),
							(void**)serviceProvider
						);
					}
				}
			}
			else
			{
				// reset immersive shell controller. todo: better resetting

				controller->Stop();
				controller->Release();

			}

			dbgprintf(L"Immersive Shell Controller Result: %x", hr);

			if (m_immersiveApplicationNotificationService)
				m_immersiveApplicationNotificationService->Register(NULL, &m_immersiveApplicationNotificationToken);

			if (m_immersiveAppCrusher)
				m_immersiveAppCrusher->Register(NULL, &m_immersiveAppCrusherNotificationToken);
		}
	}
}

// Ittr: Below has to exist for non-UWP mode to work

CImmersiveBehaviorWrapper::CImmersiveBehaviorWrapper(IImmersiveBehavior* behavior)
{
	m_cRef = 1;
	m_behavior = behavior;
	m_behavior->AddRef();
}

CImmersiveBehaviorWrapper::~CImmersiveBehaviorWrapper()
{
	dbgprintf(L"CImmersiveBehaviorWrapper::~CImmersiveBehaviorWrapper()");
	m_behavior->Release();
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::QueryInterface(REFIID riid, void** ppvObject)
{
	WCHAR iid[100];
	StringFromGUID2(riid, iid, 100);
	dbgprintf(L"CImmersiveBehaviorWrapper::QueryInterface %s", iid);
	if (riid == IID_ImmersiveBehavior)
	{
		*ppvObject = static_cast<IImmersiveBehavior*>(this);
		return S_OK;
	}
	return m_behavior->QueryInterface(riid, ppvObject);
}

ULONG STDMETHODCALLTYPE CImmersiveBehaviorWrapper::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE CImmersiveBehaviorWrapper::Release(void)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::release()");
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::OnImmersiveThreadStart(void)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::OnImmersiveThreadStart");
	return m_behavior->OnImmersiveThreadStart();
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::OnImmersiveThreadStop(void)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::OnImmersiveThreadStop");
	return m_behavior->OnImmersiveThreadStart();
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::GetMaximumComponentCount(unsigned int* count)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::GetMaximumComponentCount %p", count);
	return m_behavior->GetMaximumComponentCount(count);
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::CreateComponent(unsigned int number, IUnknown** component)
{
	//if (number == 1) DebugBreak();
	HRESULT ret = m_behavior->CreateComponent(number, component);
	dbgprintf(L"CImmersiveBehaviorWrapper::CreateComponent %d = %p", number, ret);
	/*IUnknown* wtf = *component;
	HRESULT ret2 = wtf->QueryInterface(IID_ImmersiveShell,(PVOID*)&wtf);
	dbgprintf(L"CImmersiveBehaviorWrapper::GetInterfaceList %p",ret2);*/
	return ret;
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::ShouldCreateComponent(unsigned int number, int* allowed)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::ShouldCreateComponent %d %p", number, allowed);
	if (number == 9)
	{
		*allowed = 0;
		return S_OK;
	}
	return m_behavior->ShouldCreateComponent(number, allowed);
}