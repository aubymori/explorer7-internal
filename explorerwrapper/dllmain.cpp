#include <Windows.h>
#include <shlwapi.h>
#include <Shobjidl.h>
#include <Uxtheme.h>
#include "forwards.h"
#include "startmenuresolver.h"
#include "systraywrapper.h"
#include "dbgprint.h"
#include "immersiveshell.h"
#include "traynotify.h"
#include "authui.h"
#include <commoncontrols.h>
#include <dwmapi.h>
#include "startmenupin.h"
#include "immersivefactory.h"
#include "projection.h"
#include <vector>
//#include "Detours/detours.h"

BOOL g_alttabhooked;
HWND hwnd_desktop;
HWND hwnd_taskbar;
HWND hwnd_startmenu;
HINSTANCE g_hInstance;
DWORD dwRegisterNotify;
HANDLE hEvent_DesktopVisible;

static WNDPROC g_prevTrayProc;
typedef DWORD (WINAPI *SHPtrParamAPI)(PVOID);
typedef PVOID (WINAPI *SHCreateDesktopAPI)(PVOID);

static SHCreateDesktopAPI SHCreateDesktopOrig;
static SHPtrParamAPI DwmGetColorizationParametersOrig;
static SHCreateDesktopAPI SHDesktopMessageLoop; //TEST

typedef HWND (WINAPI *CreateWindowInBandAPI)(DWORD,LPWSTR,PVOID,PVOID,PVOID,PVOID,PVOID,PVOID,PVOID,PVOID,PVOID,PVOID,DWORD);
static CreateWindowInBandAPI CreateWindowInBandOrig;
typedef BOOL (WINAPI *GetWindowBandAPI)(HWND,DWORD*);
static GetWindowBandAPI GetWindowBandOrig;

struct WINCOMPATTRDATA
{
    DWORD attribute; // the attribute to query, see below
    PVOID pData; // buffer to store the result
    ULONG dataSize; // size of the pData buffer
};

typedef BOOL (WINAPI* SetWindowCompositionAttributeAPI) (HWND hwnd, WINCOMPATTRDATA* pAttrData);
static SetWindowCompositionAttributeAPI SetWindowCompositionAttribute;

typedef struct { 
	DWORD ColorizationColor; 
    DWORD ColorizationAfterglow; 
    DWORD ColorizationColorBalance; 
    DWORD ColorizationAfterglowBalance; 
    DWORD ColorizationBlurBalance; 
    DWORD ColorizationGlassReflectionIntensity; 
    DWORD ColorizationOpaqueBlend;
} DWMCOLORIZATIONPARAMS, *PDWMCOLORIZATIONPARAMS; 

static BOOL IsRTMDWM()
{	
	if (!IsCompositionActive()) return FALSE;

	DWMCOLORIZATIONPARAMS colors;
	CHAR buffer[0x28];
	memset(buffer,0,0x28);
	DwmGetColorizationParametersOrig(&buffer);
	memcpy(&colors,(PVOID)buffer,sizeof(DWMCOLORIZATIONPARAMS));
	return (colors.ColorizationGlassReflectionIntensity == 1);
}

static HWND GetTaskbarWnd()
{
	if (!hwnd_taskbar)
		hwnd_taskbar = FindWindow(L"Shell_TrayWnd",NULL);
	return hwnd_taskbar;
}

static BOOL CALLBACK FindSMCallback(HWND hwnd, LPARAM lParam)
{	
	if ( GetClassWord(hwnd,GCW_ATOM) == (ATOM)lParam && (GetProp(hwnd,L"StartMenuTag")) )
	{
		hwnd_startmenu = hwnd;
		return FALSE;
	}
	return TRUE;
}


static HWND GetStartMenuWnd()
{
	if (!hwnd_startmenu || !IsWindow(hwnd_startmenu))
	{
		WNDCLASS dummy = {0};
		ATOM dv2atom = GetClassInfo(GetModuleHandle(NULL),L"DV2ControlHost",&dummy);
		EnumThreadWindows(GetCurrentThreadId(),FindSMCallback,(LPARAM)dv2atom);
	}
	return hwnd_startmenu;
}

LRESULT CALLBACK NewTrayProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == 0x56D) return 0;
	if (uMsg == WM_DISPLAYCHANGE || uMsg == WM_WINDOWPOSCHANGED)
	{
		RemoveProp(hwnd,L"TaskbarMonitor");
		SetProp(hwnd,L"TaskbarMonitor",(HANDLE)MonitorFromWindow(hwnd,MONITOR_DEFAULTTOPRIMARY));
		//send displaychanged to desktop
		if (uMsg == WM_DISPLAYCHANGE) PostMessage(hwnd_desktop,0x44B,0,0);
	}
	if (uMsg == 0x574) //handledelayboot
	{
		if (lParam == 3)
			return CallWindowProc(g_prevTrayProc,hwnd,0x5B5,wParam,lParam); //fire ShellDesktopSwitch event
		if (lParam == 1)
			SetEvent(hEvent_DesktopVisible);
		return 0;
	}
	return CallWindowProc(g_prevTrayProc,hwnd,uMsg,wParam,lParam);	
}

void ShimDesktop8()
{
	static int InitOnce = FALSE;
	if (InitOnce) return;
	hwnd_desktop = FindWindow(L"Progman",L"Program Manager");
	HWND hwndTray = GetTaskbarWnd();
	if (!hwnd_desktop || !hwndTray ) return;
	InitOnce = TRUE;
	//hook tray
	g_prevTrayProc = (WNDPROC)GetWindowLongPtr(hwndTray,GWLP_WNDPROC);
	SetWindowLongPtr(hwndTray,GWLP_WNDPROC,(LONG_PTR)NewTrayProc);
	//set monitor (doh!)
	SetProp(hwndTray,L"TaskbarMonitor",(HANDLE)MonitorFromWindow(hwndTray,MONITOR_DEFAULTTOPRIMARY));
	//init desktop	
	PostMessage(hwnd_desktop,0x45C,1,1); //wallpaper
	PostMessage(hwnd_desktop,0x45E,0,2); //wallpaper host
	PostMessage(hwnd_desktop,0x45C,2,3); //wallpaper & icons
	PostMessage(hwnd_desktop,0x45B,0,0); //final init
}

PVOID WINAPI SHCreateDesktopNEW(PVOID p1)
{
	PVOID ret = SHCreateDesktopOrig(p1);
	ShimDesktop8();
	return ret;
}

PVOID WINAPI SHDesktopMessageLoopNEW(PVOID p1)
{
	PVOID ret = SHDesktopMessageLoop(p1);
	SHPtrParamAPI SHCloseDesktopHandle;
	SHCloseDesktopHandle = (SHPtrParamAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"),(LPSTR)206);
	SHCloseDesktopHandle(p1);
	return ret;
}

DWORD WINAPI DwmGetColorizationParametersNEW(PDWMCOLORIZATIONPARAMS colors)
{
	CHAR buffer[0x28];
	memset(buffer,0,0x28);
	dbgprintf(L"DwmGetColorizationParameters\nColorizationColor %p\nColorizationAfterglow %p\nColorizationColorBalance %p\nColorizationAfterglowBalance %p\nColorizationBlurBalance %p\nColorizationGlassReflectionIntensity %p\nColorizationOpaqueBlend %p",
		colors->ColorizationColor,colors->ColorizationAfterglow,colors->ColorizationColorBalance,colors->ColorizationAfterglowBalance,colors->ColorizationBlurBalance,colors->ColorizationGlassReflectionIntensity,colors->ColorizationOpaqueBlend);
	DWORD ret = DwmGetColorizationParametersOrig(&buffer);
	memcpy(colors,(PVOID)buffer,sizeof(DWMCOLORIZATIONPARAMS));
	return ret;
}

BOOL WINAPI SetWindowCompositionAttributeNEW(HWND hwnd, WINCOMPATTRDATA* pAttrData)
{	
	dbgprintf(L"SetWindowCompositionAttribute %X %x %d",hwnd,pAttrData->attribute,*(DWORD*)pAttrData->pData);
	if (pAttrData->attribute == 0x10) //changed in 7->8
	{
		pAttrData->attribute = 0xF;
		if ( IsRTMDWM() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd()) ) //enable rtm pseudo-aero
		{
			SetWindowCompositionAttribute(hwnd,pAttrData);
			WINCOMPATTRDATA rtm;
			struct ATTR13DATA
			{
				DWORD p1;
				DWORD p2;
				DWORD p3;
				DWORD p4;
			};
			ATTR13DATA attr13 = {0};
			attr13.p1 = 4;
			rtm.attribute = 0x13;
			rtm.pData = &attr13;
			rtm.dataSize = 0x10;
			return SetWindowCompositionAttribute(hwnd,&rtm);
		}
	}
	return SetWindowCompositionAttribute(hwnd,pAttrData);
}

HRESULT WINAPI DwmEnableBlurBehindWindowNEW(HWND hwnd, DWM_BLURBEHIND *pBlurBehind)
{
	if ( IsRTMDWM() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd()) ) //enable rtm pseudo-aero
		pBlurBehind->fEnable = 0;
	return DwmEnableBlurBehindWindow(hwnd,pBlurBehind);
}

int WINAPI SetWindowRgnNEW( HWND hWnd, HRGN hRgn, BOOL bRedraw )
{	
	//don't allow to reset start menu rgn - rtm pseudo aero glitches
	if ( hRgn == NULL && hWnd == GetStartMenuWnd() ) return 0;
	return SetWindowRgn(hWnd,hRgn,bRedraw);
}

HRESULT WINAPI SetWindowThemeNEW(HWND hwnd,LPCWSTR pszSubAppName,LPCWSTR pszSubIdList)
{
	if ( lstrcmp(pszSubAppName,L"VerticalShowDesktop") == 0 ) return SetWindowTheme(hwnd,L"VerticalShowDesktop8",pszSubIdList);
	if ( lstrcmp(pszSubAppName,L"ShowDesktop") == 0 ) return SetWindowTheme(hwnd,L"ShowDesktop8",pszSubIdList);
	return SetWindowTheme(hwnd,pszSubAppName,pszSubIdList);
}

UINT WINAPI SetErrorModeNEW( UINT uMode )
{
	SetCurrentProcessExplicitAppUserModelID(L"Microsoft.Windows.Explorer");
	return SetErrorMode(uMode);
}

__int64 (__fastcall* DwmpActivateLivePreview)(int a1, __int64 a2, __int64 a3, int a4, void* a5);
__int64 DwmpActivateLivePreviewNEW(int a1, __int64 a2, __int64 a3, int a4, void* a5)
{
	if (IsBadReadPtr(a5, 0x8))
		a5 = 0;
	return DwmpActivateLivePreview(a1,a2,a3,a4,a5);
}

void HookShell32();
void HookAPIs()
{
	hEvent_DesktopVisible = CreateEvent(NULL,TRUE,FALSE,L"ShellDesktopVisibleEvent");
	//change desktop
	SHCreateDesktopOrig = (SHCreateDesktopAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"),(LPSTR)200);
	ChangeImportedAddress(GetModuleHandle(NULL),"shell32.dll",SHCreateDesktopOrig,SHCreateDesktopNEW);
	SHDesktopMessageLoop = (SHCreateDesktopAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"),(LPSTR)201);
	ChangeImportedAddress(GetModuleHandle(NULL),"shell32.dll",SHDesktopMessageLoop,SHDesktopMessageLoopNEW);

	//ChangeImportedAddress(GetModuleHandle(NULL),"shell32.dll", GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPSTR)902), GetProcAddress(GetModuleHandle(L"shunimpl.dll"),(LPSTR)473));
	//change appid
	ChangeImportedAddress(GetModuleHandle(NULL),"kernel32.dll",SetErrorMode,SetErrorModeNEW);
	//adapt colorization api
	DwmGetColorizationParametersOrig = (SHPtrParamAPI)GetProcAddress(GetModuleHandle(L"dwmapi.dll"),(LPSTR)127);
	DwmpActivateLivePreview = (decltype(DwmpActivateLivePreview))GetProcAddress(GetModuleHandle(L"dwmapi.dll"),(LPSTR)113);
	ChangeImportedAddress(GetModuleHandle(NULL),"dwmapi.dll", DwmpActivateLivePreview, DwmpActivateLivePreviewNEW);
	ChangeImportedAddress(GetModuleHandle(NULL),"dwmapi.dll",DwmGetColorizationParametersOrig,DwmGetColorizationParametersNEW);
	//8RTM - composition
	SetWindowCompositionAttribute = (SetWindowCompositionAttributeAPI)GetProcAddress(GetModuleHandle(L"user32.dll"),"SetWindowCompositionAttribute");
	//ChangeImportedAddress(GetModuleHandle(NULL),"user32.dll",SetWindowCompositionAttribute,SetWindowCompositionAttributeNEW);
	//ChangeImportedAddress(GetModuleHandle(NULL),"dwmapi.dll",DwmEnableBlurBehindWindow,DwmEnableBlurBehindWindowNEW);
	ChangeImportedAddress(GetModuleHandle(NULL),"user32.dll",SetWindowRgn,SetWindowRgnNEW);
	//change show desktop btn
	ChangeImportedAddress(GetModuleHandle(NULL),"uxtheme.dll",SetWindowTheme,SetWindowThemeNEW);
	//shell32 - hack created startmenupin instance		
	StartMenuPin_PatchShell32();
	//shell32 - patch delayload shit
	HookShell32();
}

HWND WINAPI CreateWindowInBandNew(DWORD exStyle, LPWSTR szClassName, PVOID p3, PVOID p4, PVOID p5, PVOID p6, PVOID p7, PVOID p8, PVOID p9, PVOID p10, PVOID p11, PVOID p12, DWORD p13)
{
	DWORD p0 = (DWORD)_ReturnAddress();
	exStyle = exStyle | WS_EX_TOOLWINDOW;
	HWND ret = CreateWindowInBandOrig(exStyle,szClassName,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13 & 1);
	dbgprintf(L"%p: CreateWindowInBand %p %s %p %p %p %p %p %p %p %p %p %p %p = %p %p",p0,exStyle,szClassName,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,ret,GetLastError());
	SetProp(ret,L"explorer7.WindowBand",(HANDLE)p13);
	return ret;
}

BOOL WINAPI GetUserObjectInformationNew( HANDLE hObj, int nIndex, PVOID pvInfo, DWORD nLength, LPDWORD lpnLengthNeeded )
{	
	lstrcpy(LPWSTR(pvInfo),L"Winlogon");
	return TRUE;
}

BOOL WINAPI GetWindowBandNew(HWND hwnd,DWORD* out)
{
	BOOL ret = GetWindowBandOrig(hwnd,out);
	DWORD origband = (DWORD)GetProp(GetAncestor(hwnd,GA_ROOTOWNER),L"explorer7.WindowBand");
	dbgprintf(L"GetWindowBand %p %p %p",hwnd,*out,origband);
	if (origband && out) *out = origband;	
	return ret;
}

UINT_PTR WINAPI SetTimer_WUI( HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc )
{
	if ( nIDEvent == 0x2252CE37 )
		ShowWindow(hWnd,SW_HIDE);
	return SetTimer(hWnd,nIDEvent,uElapse,lpTimerFunc);
}

void HookImmersive()
{
	HMODULE immersiveui = LoadLibrary(L"Windows.UI.Immersive.dll");	
	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	CreateWindowInBandOrig = (CreateWindowInBandAPI)GetProcAddress(hUser32,"CreateWindowInBand");
	GetWindowBandOrig = (GetWindowBandAPI)GetProcAddress(hUser32,"GetWindowBand");
	ChangeImportedAddress(immersiveui,"user32.dll",CreateWindowInBandOrig,CreateWindowInBandNew);
	ChangeImportedAddress(immersiveui,"user32.dll",GetWindowBandOrig,GetWindowBandNew);
	ChangeImportedAddress(immersiveui,"user32.dll",GetUserObjectInformation,GetUserObjectInformationNew);
	ChangeImportedAddress(immersiveui,"user32.dll",SetTimer,SetTimer_WUI);
	//bugbug!!!
	ChangeImportedAddress(GetModuleHandle(L"twinui.dll"),"user32.dll",CreateWindowInBandOrig,CreateWindowInBandNew);
	ChangeImportedAddress(GetModuleHandle(L"authui.dll"),"user32.dll",CreateWindowInBandOrig,CreateWindowInBandNew);
	ChangeImportedAddress(GetModuleHandle(L"shell32.dll"),"user32.dll",CreateWindowInBandOrig,CreateWindowInBandNew);

	ChangeImportedAddress(GetModuleHandle(L"twinapi.dll"),"user32.dll",CreateWindowInBandOrig,CreateWindowInBandNew);
	ChangeImportedAddress(GetModuleHandle(L"Windows.UI.dll"),"user32.dll",CreateWindowInBandOrig,CreateWindowInBandNew);
}

FARPROC
WINAPI
GetProcAddress_Hook(
	HMODULE hModule,
	LPCSTR lpProcName
)
{
	//dbgprintf(L"GetProcAddress Hook\n");
	return GetProcAddress(hModule,lpProcName);
}



void AssFuckShunimpl()
{
	char* dllmainSHUNIMPL = (char*)FindPattern((uintptr_t)GetModuleHandle(L"shunimpl.dll"),"48 83 EC 28 83 FA 01");

	if (dllmainSHUNIMPL)
	{
		char bytes[] = { 0xB0,0x01,0xC3 };

		DWORD old;
		VirtualProtect(dllmainSHUNIMPL, sizeof(bytes), PAGE_EXECUTE_READWRITE, &old);
		memcpy(dllmainSHUNIMPL, bytes, sizeof(bytes));
		VirtualProtect(dllmainSHUNIMPL, sizeof(bytes), old, 0);
	}
	
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			{
				AssFuckShunimpl();

				dbgprintf(L"Dll Attach\n");
				g_hInstance = hModule;
				if ( GetModuleHandle(L"DisplaySwitch.exe") )
				{
					dbgprintf(L"loaded into displayswitch %p %s!",GetCurrentProcessId(),GetCommandLine());
					HookImmersive();
				}
				else
					HookAPIs();		
			}
			break;
		case DLL_THREAD_ATTACH:
			{			
				if (!g_alttabhooked && GetModuleHandle(L"alttab.dll"))
				{
					CreateWindowInBandOrig = (CreateWindowInBandAPI)GetProcAddress(GetModuleHandle(L"user32.dll"),"CreateWindowInBand");
					ChangeImportedAddress(GetModuleHandle(L"alttab.dll"),"user32.dll",CreateWindowInBandOrig,CreateWindowInBandNew);				
					g_alttabhooked = TRUE;				
				}
			}
			break;
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}



extern "C" HRESULT WINAPI Explorer_CoCreateInstance(
  __in   REFCLSID rclsid,
  __in   LPUNKNOWN pUnkOuter,
  __in   DWORD dwClsContext,
  __in   REFIID riid,
  __out  LPVOID *ppv
)
{
	HRESULT result;
	if (rclsid == CLSID_SysTray) //create Metro before tray
	{
		dbgprintf(L"create Metro before tray\n");
		HookImmersive();
		//CreateTwinUI();
	}
	if (rclsid == CLSID_RegTreeOptions && riid == IID_IRegTreeOptions7) //upgrading RegTreeOptions interface
		result = CoCreateInstance(rclsid,pUnkOuter,dwClsContext,IID_IRegTreeOptions8,ppv);	
	else
		result = CoCreateInstance(rclsid,pUnkOuter,dwClsContext,riid,ppv);

	if (rclsid == CLSID_StartMenuCacheAndAppResolver && result == E_NOINTERFACE)
	{
		dbgprintf(L"Explorer_CoCreateInstance: Resolver7 using iappresolver8\n");
		PVOID rslvr8 = NULL;
		CoCreateInstance(rclsid,pUnkOuter,dwClsContext,IID_IAppResolver8,&rslvr8);
		//create our object
		CStartMenuResolver* resolver7 = new CStartMenuResolver((IAppResolver8*)rslvr8);
		result = resolver7->QueryInterface(riid,ppv);
		if (result == S_OK)
			dbgprintf(L"Explorer_CoCreateInstance: Resolver7 using iappresolver8 IS OK!!\n");
	}
	if (result == S_OK && rclsid == CLSID_SysTray) //wrap stobject
	{
		dbgprintf(L"wrap stobject\n");
		*ppv = new CSysTrayWrapper((IOleCommandTarget*)*ppv);
	}
	if (rclsid == CLSID_AuthUIShutdownChoices) //wrap authui
	{
		dbgprintf(L"wrap authui\n");
		if (*ppv)
		{
			dbgprintf(L"good\n");
			*ppv = new CAuthUIWrapper((IShutdownChoices8*)*ppv);
		}
		else
		{
			CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_IShutdownChoices8, ppv);
			if (*ppv)
			{
				dbgprintf(L"good 2\n");
				*ppv = new CAuthUIWrapper((IShutdownChoices8*)*ppv);
			}
		}

	}
	return result;
}

extern "C" HRESULT WINAPI Explorer_CoRegisterClassObject(
  REFCLSID rclsid,     //Class identifier (CLSID) to be registered
  IUnknown * pUnk,     //Pointer to the class object
  DWORD dwClsContext,  //Context for running executable code
  DWORD flags,         //How to connect to the class object
  LPDWORD  lpdwRegister
)
{
	if ( rclsid == CLSID_TrayNotify)
	{
		pUnk = new CTrayNotifyFactory((IClassFactory*)pUnk);
		//register immersive shell fake too
		RegisterFakeImmersive();
		//and projection
		RegisterProjection();
	}

	HRESULT rslt = CoRegisterClassObject(rclsid,pUnk,dwClsContext,flags,lpdwRegister);

	if ( rclsid == CLSID_TrayNotify)
		dwRegisterNotify = *lpdwRegister;

	return rslt;
}


extern "C" HRESULT WINAPI Explorer_CoRevokeClassObject( DWORD dwRegister )
{	
	if (dwRegister == dwRegisterNotify)
	{
		UnregisterFakeImmersive();
		UnregisterProjection();
	}
	return CoRevokeClassObject(dwRegister);
}
