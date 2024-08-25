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
#include "version.h"
#include "pinnedlist.h"
//#include "Detours/detours.h"
#include "resource.h"
#include "thememanager.h"
#include "MinHook.h"

#define _WIN_BLUE 1 //Win8.1-specific changes
#define _WIN_TH1 0 //Win10TH1-specific changes - currently unused
#define _WIN_RS1 0 //Win10RS1-specific changes - currently unused
#define _WIN_RS5 0 //Win10RS5-specific changes - currently unused
#define _WIN_VB 0 //Win10VB-specific changes - currently unused
#define _DISABLE_COMPOSITION 0 //For debugging without disabling DWM

//uncomment to force classic theme
//#define FORCE_CLASSIC

BOOL g_alttabhooked;
HWND hwnd_desktop;
HWND hwnd_taskbar;
HWND hwnd_startmenu;
HWND hwnd_taskthumb;
HINSTANCE g_hInstance;
DWORD dwRegisterNotify;
HANDLE hEvent_DesktopVisible;

DWORD g_dwTrayThreadId = 0;

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

enum ACCENT_STATE : INT {				// Affects the rendering of the background of a window. These names are only for ACCENT_POLICY purposes 
	ACCENT_DISABLED = 0,					// Default value. Background is black.
	ACCENT_ENABLE_GRADIENT = 1,				// Background is GradientColor, alpha channel ignored.
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,	// Background is GradientColor.
	ACCENT_ENABLE_BLURBEHIND = 3,			// Background is GradientColor, with blur effect.
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,	// Background is GradientColor, with acrylic blur effect.
	ACCENT_ENABLE_HOSTBACKDROP = 5,			// Unknown.
	ACCENT_INVALID_STATE = 6				// Unknown. Seems to draw background fully transparent.
};

struct ACCENT_POLICY {			// Determines how a window's background is rendered.
	ACCENT_STATE	AccentState;	// Background effect.
	UINT			AccentFlags;	// Flags. Set to 2 to tell GradientColor is used, rest is unknown.
	COLORREF		GradientColor;	// Background color.
	LONG			AnimationId;	// Unknown
};

enum WINDOWCOMPOSITIONATTRIB : INT {	// Determines what attribute is being manipulated.
	WCA_ACCENT_POLICY = 0x13,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 0xF				// The attribute being get or set is an accent policy.
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

static HWND GetTaskListThumbWnd()
{
	if (!hwnd_taskthumb)
		hwnd_taskthumb = FindWindow(L"TaskListThumbnailWnd", NULL);
	return hwnd_taskthumb;
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
	PostMessage(hwnd_desktop,0x40B,0,0); //pins
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

//Ittr: Less lines of code and more utility/reusability for setting composition attributes in future
void ForceActiveWindowAppearance(HWND hwnd)
{
	ACCENT_POLICY policy = { ACCENT_ENABLE_ACRYLICBLURBEHIND, 1, 0x1, 1 }; //BlurBehind is just the naming as the category names were ripped from accent states. Please ignore!
	WINCOMPATTRDATA data = { WCA_FORCE_ACTIVEWINDOW_APPEARANCE, &policy, 4 };
	SetWindowCompositionAttribute(hwnd, &data);
}

BOOL WINAPI SetWindowCompositionAttributeNEW(HWND hwnd, WINCOMPATTRDATA* pAttrData)
{	
	dbgprintf(L"SetWindowCompositionAttribute %X %x %d",hwnd,pAttrData->attribute,*(DWORD*)pAttrData->pData);
	if (_WIN_BLUE && IsCompositionActive()) //If we are 8.1 or higher for some reason Tihiy's original hack doesn't work so we forcefully run this instead
	{
		//Ittr: Restore active colorization based on attribute from ForceActiveWindowAppearance function in 9600 explorer
		ForceActiveWindowAppearance(hwnd);
		return SetWindowCompositionAttribute(hwnd, pAttrData);
	}
	else if (!_WIN_BLUE && pAttrData->attribute == 0x10) //changed in 7->8
	{
		pAttrData->attribute = 0xF;
		if (IsRTMDWM() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd())) //enable rtm pseudo-aero
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
	/*if (_WIN_BLUE) --Doesn't work yet
	{
		if (IsCompositionActive() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd()))
		{
			WINCOMPATTRDATA transparency;
			struct ATTR13DATA
			{
				DWORD p1;
				DWORD p2;
				DWORD p3;
				DWORD p4;
			};
			ATTR13DATA attr13 = { 0 };
			attr13.p1 = 2;
			transparency.attribute = 0x19;
			transparency.pData = &attr13;
			transparency.dataSize = 0x16;
			SetWindowCompositionAttribute(hwnd, &transparency);

			pBlurBehind->hRgnBlur = 0i64;
			pBlurBehind->fTransitionOnMaximized = 1;
			pBlurBehind->dwFlags = 3;
			pBlurBehind->fEnable = 1;
			return DwmEnableBlurBehindWindow(hwnd, pBlurBehind);
		}
	}*/
	//else if ( IsRTMDWM() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd()) ) //enable rtm pseudo-aero
	if (!_WIN_BLUE && (IsRTMDWM() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd()))) //bad temporary hack to ensure this doesnt run on 8.1+
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
	//Ittr: Temporarily comment these out as unneeded - we want original 7 theme classes to be used where appropriate!
	//if ( lstrcmp(pszSubAppName,L"VerticalShowDesktop") == 0 ) return SetWindowTheme(hwnd,L"VerticalShowDesktop8",pszSubIdList);
	//if ( lstrcmp(pszSubAppName,L"ShowDesktop") == 0 ) return SetWindowTheme(hwnd,L"ShowDesktop8",pszSubIdList);
	return SetWindowTheme(hwnd,pszSubAppName,pszSubIdList);
}

UINT WINAPI SetErrorModeNEW( UINT uMode )
{
	SetCurrentProcessExplicitAppUserModelID(L"Microsoft.Windows.Explorer");
	return SetErrorMode(uMode);
}

typedef BOOL (WINAPI* IsShellWindow_t)(HWND);
IsShellWindow_t IsShellFrameWindow = nullptr;
IsShellWindow_t IsShellManagedWindow = nullptr;

typedef HWND(WINAPI* GhostWindowFromHungWindow_t)(HWND);
GhostWindowFromHungWindow_t GhostWindowFromHungWindow = nullptr;

//removes immersive background windows
//(Microsoft Text Input Host, Shell Experience Host, etc.)
BOOL WINAPI IsWindowVisibleNEW(HWND hWnd)
{
	if (IsShellManagedWindow)
	{
		if (IsShellManagedWindow(hWnd) && GetPropW(hWnd, L"Microsoft.Windows.ShellManagedWindowAsNormalWindow") == NULL)
			return FALSE;
	}
	if (!IsWindowVisible(hWnd))
		return FALSE;

	return TRUE;
}

__int64 (__fastcall* DwmpActivateLivePreview)(int a1, __int64 a2, __int64 a3, int a4, void* a5);
__int64 DwmpActivateLivePreviewNEW(int a1, __int64 a2, __int64 a3, int a4, void* a5)
{
	if (IsBadReadPtr(a5, 0x8))
		a5 = 0;
	return DwmpActivateLivePreview(a1,a2,a3,a4,a5);
}

//Ittr: Intercept these functions where appropriate for basic theme to be forced at compile time if required
BOOL WINAPI IsCompositionActiveNEW()
{
	if (_DISABLE_COMPOSITION) { return FALSE; }

	return IsCompositionActive();
}

HRESULT WINAPI DwmIsCompositionEnabledNEW(BOOL* pfEnabled)
{
	if (_DISABLE_COMPOSITION) { return 0x80263001; } //0x80263001 is the value to signify composition being disabled for some reason

	return DwmIsCompositionEnabled(pfEnabled);
}

//fix for classic start menu icon
typedef HANDLE(WINAPI* BrandingLoadImage_t)(
	LPCWSTR lpszModule,
	UINT    uImageId,
	UINT    type,
	int     cx,
	int     cy,
	UINT    fuLoad
);
BrandingLoadImage_t BrandingLoadImage = nullptr;
HANDLE WINAPI BrandingLoadImageNEW(
	LPCWSTR lpszModule,
	UINT    uImageId,
	UINT    type,
	int     cx,
	int     cy,
	UINT    fuLoad
)
{
	WCHAR msg[256];
	wsprintfW(msg, L"BrandingLoadImage, id: %u", uImageId);
	MessageBoxW(NULL, msg, L"debug", NULL);

	UINT uNewId = 0;
	switch (uImageId)
	{
		case 1041:
			uNewId = IDB_START;
			break;
		case 2041:
			uNewId = IDB_START_125;
			break;
		case 3041:
			uNewId = IDB_START_150;
			break;
	}

	if (uNewId)
		return LoadImageW(
			g_hInstance,
			MAKEINTRESOURCE(uNewId),
			type, cx, cy, fuLoad
		);
	else
		return BrandingLoadImage(
			lpszModule, uImageId, type,
			cx, cy, fuLoad
		);
}

//Ittr: Consolidated function for pattern byte replacements.
void ChangeImportedPattern(void* dllPattern, const char* newBytes, bool isGuid) //thank you wiktor
{
	if (dllPattern)
	{
		SIZE_T size;
		if (isGuid == true) { size = sizeof(GUID); } else { size = sizeof(newBytes); }

		//Magical byte replacement code
		DWORD old;
		VirtualProtect(dllPattern, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(dllPattern, newBytes, size);
		VirtualProtect(dllPattern, size, old, 0);
	}
}

void FixWin7TrayClock()
{
	//The bytes for the old and since-replaced Windows 7 tray clock IID
	char* iidw7TrayClock = "10 DF 76 43 62 A6 0B 42 B3 0D 95 88 81 46 1E F9";

	//Load and patch explorer EXE with the new IID used since Windows 8.1
	char bytes[] = { 0x8A, 0xCA, 0x5F, 0x7A, 0xB1, 0x76, 0xC8, 0x44, 0xA9, 0x7C, 0xE7, 0x17, 0x3C, 0xCA, 0x5F, 0x4F };
	ChangeImportedPattern((char*)FindPattern((uintptr_t)GetModuleHandle(NULL), "10 DF 76 43 62 A6 0B 42 B3 0D 95 88 81 46 1E F9"), bytes, true);
}

//Ittr: Goodbye immersive context menus and good riddance. For Win10 TH1+. In future consider build check to limit to 10240+. 
//Also to be noted that Windows 11 makes further changes here that we'll need to account for in future if we do officially support it.
void ShowWin32Menus()
{
	//The bytes are the same in both dlls for ImmersiveContextMenuHelper::CanApplyOwnerDrawToMenu function
	char* immersiveBytes = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B";

	//Load and patch both DLLs. ExplorerFrame gets called in later so we have to account for that
	char bytes[] = { 0xB0, 0x00, 0xC3 };
	ChangeImportedPattern((char*)FindPattern((uintptr_t)GetModuleHandle(L"shell32.dll"), immersiveBytes), bytes, false);
	ChangeImportedPattern((char*)FindPattern((uintptr_t)LoadLibrary(L"ExplorerFrame.dll"), immersiveBytes), bytes, false);
}

void FixAuthUI()
{
	// Newer explorer versions use this
	// CLogoffPane::_InitShutdownObjects
	const char* bytes = "48 8B 8E 98 00 00 00 48 8B 56 40 45 33 C0 48 8B 01 FF 50 18 "
						"48 8B 8E 98 00 00 00 48 8B 01 FF 50 30 8B D8 "
						"85 C0 ?? ?? "
						"48 8B 8E 98 00 00 00 48 8D 96 A0 00 00 00 48 8B 01 FF 50 20";

	// Older explorer versions use this
	// CLogoffPane::_OnCreate
	const char* bytesOld = "48 8B 8B 98 00 00 00 48 8B 53 40 45 33 C0 48 8B 01 FF 50 18 "
							"48 8B 8B 98 00 00 00 48 8B 01 FF 50 30 44 8B C8 "
							"85 C0 ?? ?? ?? ?? ?? ?? "
							"48 8B 8B 98 00 00 00 48 8D 93 A0 00 00 00 48 8B 01 FF 50 20";

	char* pattern = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), bytes);
	char* pattern1 = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), bytesOld);

	if (pattern)
	{
		DWORD old;
		char patch1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		SIZE_T size = sizeof(patch1);

		// mov rax, [rcx]
		// call qword ptr [rax+18h]
		char* inst1 = pattern + 14;
		VirtualProtect(inst1, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(inst1, patch1, size);
		VirtualProtect(inst1, size, old, 0);

		// mov rax, [rcx]
		// call qword ptr [rax+30h]
		char* inst2 = pattern + 27;
		VirtualProtect(inst2, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(inst2, patch1, size);
		VirtualProtect(inst2, size, old, 0);

		// mov rax, [rcx]
		// call qword ptr [rax+20h]
		char* inst3 = pattern + 53;
		VirtualProtect(inst3, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(inst3, patch1, size);
		VirtualProtect(inst3, size, old, 0);

	}

	if (pattern1 && !pattern) //Ittr: Only apply to CLogoffPane::_OnCreate if we need to, otherwise this causes crashing on later 7 explorer.
	{
		DWORD old;
		char patch1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		SIZE_T size = sizeof(patch1);

		// mov rax, [rcx]
		// call qword ptr [rax+18h]
		char* inst1 = pattern1 + 14;
		VirtualProtect(inst1, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(inst1, patch1, size);
		VirtualProtect(inst1, size, old, 0);

		// mov rax, [rcx]
		// call qword ptr [rax+30h]
		char* inst2 = pattern1 + 27;
		old = NULL;
		VirtualProtect(inst2, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(inst2, patch1, size);
		VirtualProtect(inst2, size, old, 0);

		// mov rax, [rcx]
		// call qword ptr [rax+20h]
		char* inst3 = pattern1 + 58;
		old = NULL;
		VirtualProtect(inst3, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(inst3, patch1, size);
		VirtualProtect(inst3, size, old, 0);
	}
}

struct pairs
{
	WCHAR* str;
	HTHEME theme;
};

int sizeCounter = 0;
pairs themes[256];


int g_fDPIAware = 0;
int g_nScreenDpi = 0;
int g_fForcedDpi = 0;
__int64 GetScreenDpi(void)
{
	int v0; // eax
	HDC DC; // rax
	HDC v3; // rbx

	if (!g_fForcedDpi)
	{
		v0 = IsProcessDPIAware();
		if (g_fDPIAware != v0 || !g_nScreenDpi)
		{
			g_fDPIAware = v0;
			g_nScreenDpi = 96;
			DC = GetDC(0LL);
			v3 = DC;
			if (DC)
			{
				g_nScreenDpi = GetDeviceCaps(DC, 88);
				ReleaseDC(0LL, v3);
			}
		}
	}
	return (unsigned int)g_nScreenDpi;
}

//CThemeManager themeManager(L"C:\\Windows\\aero.msstyles");

HTHEME(__stdcall* fOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
HTHEME(__stdcall* fOpenThemeDataForDpi)(HWND hwnd, LPCWSTR pszClassList, UINT dpi);
HTHEME(__stdcall* fOpenThemeDataEx)(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags);
HTHEME __stdcall OpenThemeData_Hook(HWND hwnd, LPCWSTR pszClassList)
{
	HTHEME theme = 0;
	dbgprintf(L"OPENTHEMEDATA %s", pszClassList);

	LPCWSTR pszClassListToUse = pszClassList;
	//if (wcscmp(pszClassList, L"Pager") == 0)
	//	pszClassListToUse = L"TrayNotifyVert::Button";

	DWORD flags = 2;
	if ((unsigned int)GetScreenDpi() != 96)
		flags |= 1u;

	if (g_loadedTheme && g_dwTrayThreadId == GetCurrentThreadId())
		theme = OpenThemeDataFromFile(g_loadedTheme,hwnd, pszClassListToUse, flags);
	else
		theme = fOpenThemeData(hwnd, pszClassList);

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATA FAILED %s", pszClassList);

	return theme;
}

HTHEME __stdcall OpenThemeDataForDpi_Hook(HWND hwnd, LPCWSTR pszClassList, UINT dpi)
{
	HTHEME theme = 0;
	dbgprintf(L"OPENTHEMEDATA %s", pszClassList);
	LPCWSTR pszClassListToUse = pszClassList;
	//if (wcscmp(pszClassList, L"Pager") == 0)
	//	pszClassListToUse = L"TrayNotifyVert::Button";

	DWORD flags = 2;
	if (dpi != 96)
		flags |= 1u;

	if (g_loadedTheme && g_dwTrayThreadId == GetCurrentThreadId())
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassListToUse, flags);
	else
		theme = fOpenThemeDataForDpi(hwnd, pszClassList,dpi);

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATAFORDPI FAILED %s", pszClassList);

	return theme;
}

HTHEME __stdcall OpenThemeDataEx_Hook(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags)
{
	HTHEME theme = 0;
	dbgprintf(L"OPENTHEMEDATA %s", pszClassList);
	LPCWSTR pszClassListToUse = pszClassList;
	//if (wcscmp(pszClassList, L"Pager") == 0)
	//	pszClassListToUse = L"TrayNotifyVert::Button";

	DWORD flags = 2;
	if ((unsigned int)GetScreenDpi() != 96)
		flags |= 1u;

	if (g_loadedTheme && g_dwTrayThreadId == GetCurrentThreadId())
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassListToUse, dwFlags | flags);
	else
		theme = fOpenThemeDataEx(hwnd, pszClassList,dwFlags);

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATAEX FAILED %s", pszClassList);

	return theme;
}

LPTHREAD_START_ROUTINE CTray__SyncThreadProc_orig = nullptr;
DWORD WINAPI CTray__SyncThreadProc_hook(LPVOID lpParameter)
{
	if (!g_dwTrayThreadId)
	{
		g_dwTrayThreadId = GetCurrentThreadId();
		dbgprintf(L"set g_dwTrayThreadId to %u", g_dwTrayThreadId);
	}

	if (CTray__SyncThreadProc_orig)
		return CTray__SyncThreadProc_orig(lpParameter);
	return 0;
}

void HookTrayThread(void)
{
	CTray__SyncThreadProc_orig = (LPTHREAD_START_ROUTINE)FindPattern(
		(uintptr_t)GetModuleHandle(NULL),
		"48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 81 EC 00 03 00 00 48 8B"
	);

	if (CTray__SyncThreadProc_orig)
	{
		MH_CreateHook(
			(void *)CTray__SyncThreadProc_orig,
			(void *)CTray__SyncThreadProc_hook,
			(void **)&CTray__SyncThreadProc_orig
		);
	}
}

/* Adjust a window's position to be pushed away from the taskbar */
SIZE AdjustWindowRectForTaskbar(RECT *lprc)
{
	HMONITOR hm = MonitorFromRect(lprc, MONITOR_DEFAULTTONEAREST);
	HDC hDC = GetDC(NULL);
	int offset = MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 96);
	ReleaseDC(NULL, hDC);

	MONITORINFO mi = { sizeof(MONITORINFO) };
	GetMonitorInfoW(hm, &mi);

	int dx = 0, dy = 0;
	PLONG plrc = (PLONG)lprc;
	PLONG plwrc = (PLONG)&mi.rcWork;
	for (int i = 0; i < 4; i++)
	{
		int curOffset = plwrc[i] - plrc[i];
		curOffset = (curOffset < 0) ? -curOffset : curOffset;

		if (curOffset < offset)
		{
			int *set = (i % 2 == 0) ? &dx : &dy;
			if (i > 1)
			{
				*set -= offset - curOffset;
			}
			else
			{
				*set += offset - curOffset;
			}
		}
	}
	return { dx, dy };
}

/* Make tray overflow float again on Windows 10. */
typedef void (*CTrayOverflow__PositionWindow_t)(void *);
CTrayOverflow__PositionWindow_t CTrayOverflow__PositionWindow_orig = nullptr;
void CTrayOverflow__PositionWindow_hook(void *pThis)
{
	CTrayOverflow__PositionWindow_orig(pThis);
	/* This code should be safe to run on lower builds,
	   but it's not necessary. */
	HWND hWnd = *(HWND *)pThis;
	if (g_osVersion.BuildNumber() >= 10240
	&& hWnd)
	{
		RECT rc;
		GetWindowRect(hWnd, &rc);

		SIZE adjust = AdjustWindowRectForTaskbar(&rc);
		SetWindowPos(
			hWnd,
			NULL,
			rc.left + adjust.cx,
			rc.top + adjust.cy,
			0, 0,
			SWP_NOSIZE | SWP_NOZORDER
		);
	}
}

void HookTrayOverflow(void)
{
	CTrayOverflow__PositionWindow_orig = (CTrayOverflow__PositionWindow_t)FindPattern(
		(uintptr_t)GetModuleHandle(NULL),
		"4C 8B DC 49 89 5B 10 57 48 81 EC D0 00 00 00 48 8B 05 CE 28 06 00 48 33 C4 48 89 84 24 C8 00 00 00"
	);

	if (CTrayOverflow__PositionWindow_orig)
	{
		MH_CreateHook(
			(void *)CTrayOverflow__PositionWindow_orig,
			(void *)CTrayOverflow__PositionWindow_hook,
			(void **)&CTrayOverflow__PositionWindow_orig
		);
	}
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
	//Ittr: Disable DWM composition as quickly as we can (if compile flag set)
	ChangeImportedAddress(GetModuleHandle(NULL),"uxtheme.dll",IsCompositionActive,IsCompositionActiveNEW);
	ThemeManagerInitialize();
	fOpenThemeData = decltype(fOpenThemeData)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeData"));
	fOpenThemeDataForDpi = decltype(fOpenThemeDataForDpi)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataForDpi"));
	fOpenThemeDataEx = decltype(fOpenThemeDataEx)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataEx"));
	MH_Initialize();
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeData), OpenThemeData_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeData));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataForDpi), OpenThemeDataForDpi_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataForDpi));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataEx), OpenThemeDataEx_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataEx));
	HookTrayThread();
	HookTrayOverflow();
	MH_EnableHook(MH_ALL_HOOKS);

	//ChangeImportedAddress(GetModuleHandle(NULL),"uxtheme.dll", GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeData"), OpenThemeData_Hook);
	//ChangeImportedAddress(GetModuleHandle(NULL),"uxtheme.dll", GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "CloseThemeData"), CloseThemeDataNEW);
	ChangeImportedAddress(GetModuleHandle(NULL),"dwmapi.dll",DwmIsCompositionEnabled,DwmIsCompositionEnabledNEW);
	//adapt colorization api
	DwmGetColorizationParametersOrig = (SHPtrParamAPI)GetProcAddress(GetModuleHandle(L"dwmapi.dll"),(LPSTR)127);
	DwmpActivateLivePreview = (decltype(DwmpActivateLivePreview))GetProcAddress(GetModuleHandle(L"dwmapi.dll"),(LPSTR)113);
	ChangeImportedAddress(GetModuleHandle(NULL),"dwmapi.dll", DwmpActivateLivePreview, DwmpActivateLivePreviewNEW);
	ChangeImportedAddress(GetModuleHandle(NULL),"dwmapi.dll",DwmGetColorizationParametersOrig,DwmGetColorizationParametersNEW);
	//8RTM - composition
	//Ittr: Restore active DWM "colorization" to previews, taskbar and start menu (how this renders is theme-dependent)
	SetWindowCompositionAttribute = (SetWindowCompositionAttributeAPI)GetProcAddress(GetModuleHandle(L"user32.dll"),"SetWindowCompositionAttribute");
	ChangeImportedAddress(GetModuleHandle(NULL),"user32.dll",SetWindowCompositionAttribute,SetWindowCompositionAttributeNEW);
	ChangeImportedAddress(GetModuleHandle(NULL),"dwmapi.dll",DwmEnableBlurBehindWindow,DwmEnableBlurBehindWindowNEW);
	ChangeImportedAddress(GetModuleHandle(NULL),"user32.dll",SetWindowRgn,SetWindowRgnNEW);
	//load functions needed for task enum hook
	HMODULE user32 = LoadLibrary(L"user32.dll");
	IsShellFrameWindow = (IsShellWindow_t)GetProcAddress(user32, (LPCSTR)2573);
	IsShellManagedWindow = (IsShellWindow_t)GetProcAddress(user32, (LPCSTR)2574);
	GhostWindowFromHungWindow = (GhostWindowFromHungWindow_t)GetProcAddress(user32, "GhostWindowFromHungWindow");
	//perform the actual hook
	ChangeImportedAddress(GetModuleHandle(NULL),"user32.dll",IsWindowVisible,IsWindowVisibleNEW);
	//change show desktop btn
	ChangeImportedAddress(GetModuleHandle(NULL),"uxtheme.dll",SetWindowTheme,SetWindowThemeNEW);
	//fix classic start menu icon (pls fix)
	/*HMODULE winbrand = LoadLibrary(L"winbrand.dll");
	BrandingLoadImage = (BrandingLoadImage_t)GetProcAddress(winbrand, "BrandingLoadImage");
	if (BrandingLoadImage)
		ChangeImportedAddress(GetModuleHandle(NULL),"winbrand.dll",BrandingLoadImage,BrandingLoadImageNEW);*/
	//shell32 - hack created startmenupin instance		
	StartMenuPin_PatchShell32();
	//shell32 - patch delayload shit
	HookShell32();
	FixWin7TrayClock();
	ShowWin32Menus(); //Remove immersive menus so taskbar behaves properly
	FixAuthUI();
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

//TODO: Migrate to use ChangeImportedPattern or equivalent when said function is finalised. Not migrated yet due to importance of shunimpl
//Basically this allows explorer to actually work on builds >9200
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
#ifdef FORCE_CLASSIC
				SetThemeAppProperties(0);
#endif
				AssFuckShunimpl();

				dbgprintf(L"Dll Attach\n");
				g_hInstance = hModule;
				if ( GetModuleHandle(L"DisplaySwitch.exe") )
				{
					dbgprintf(L"loaded into displayswitch %p %s!",GetCurrentProcessId(),GetCommandLine());
					HookImmersive();
				}
				else
				{
					HookAPIs();
				}
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
	if ((rclsid == CLSID_StartMenuPin || rclsid == CLSID_TaskbarPin) && riid == IID_IPinnedList2 && result == E_NOINTERFACE)
	{
		int build = g_osVersion.BuildNumber();
		IID id = IID_IFlexibleTaskbarPinnedList;
		if (build >= 17763)
			id = IID_IPinnedList3;

		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, id, ppv);
		*ppv = new CPinnedListWrapper((IUnknown*)*ppv, build);
	}
	if (result == S_OK && rclsid == CLSID_SysTray) //wrap stobject
	{
		dbgprintf(L"wrap stobject\n");
		*ppv = new CSysTrayWrapper((IOleCommandTarget*)*ppv);
	}
	if (rclsid == CLSID_AuthUIShutdownChoices) //wrap authui
	{
		dbgprintf(L"wrap authui\n");
		int build = g_osVersion.BuildNumber();
		if (*ppv)
		{
			dbgprintf(L"good\n");
			*ppv = new CAuthUIWrapper((IUnknown*)*ppv, build);
		}
		else
		{
			IID dk = IID_IShutdownChoices8;
			if (build >= 10240)
				dk = IID_IShutdownChoices10;

			result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, dk, ppv);
			if (*ppv)
			{
				dbgprintf(L"good 2\n");
				*ppv = new CAuthUIWrapper((IUnknown*)*ppv, build);
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
