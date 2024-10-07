#define INITGUID
#include "framework.h"
#include "forwards.h"
#include "startmenuresolver.h"
#include "systraywrapper.h"
#include "dbgprint.h"
#include "immersiveshell.h"
#include "traynotify.h"
#include "authui.h"
#include "startmenupin.h"
#include "immersivefactory.h"
#include "projection.h"
#include "version.h"
#include "pinnedlist.h"
#include "destinationlist.h"
#include "resource.h"
#include "thememanager.h"
#include "MinHook.h"
#include "taskscheduler.h"
#include "registry.h"
#include "nsctree.h"
#include "timebomb.h"
#include "cregtree.h"
#include "shellapi.h"
#include "autoplay.h"
#include "shellitemfilter.h"
#include "shell32_wrappers.h"
#include "shellurl.h"
#include <wincodec.h>

#define _WIN_BLUE 1 //Win8.1-specific changes
#define _WIN_TH1 0 //Win10TH1-specific changes - currently unused
#define _WIN_RS1 0 //Win10RS1-specific changes - currently unused
#define _WIN_RS5 0 //Win10RS5-specific changes - currently unused
#define _WIN_VB 0 //Win10VB-specific changes - currently unused

BOOL g_alttabhooked;
HWND hwnd_desktop;
HWND hwnd_taskbar;
HWND hwnd_startmenu;
HWND hwnd_taskthumb;
HWND hwnd_taskman;
HINSTANCE g_hInstance;
DWORD dwRegisterNotify;
HANDLE hEvent_DesktopVisible;

DWORD g_dwTrayThreadId = 0;

bool g_bClassicTheme = false;
bool g_bDisableComposition = false;
bool g_enableImmersiveShellStack = false;

static WNDPROC g_prevTrayProc;
typedef DWORD(WINAPI* SHPtrParamAPI)(PVOID);
typedef PVOID(WINAPI* SHCreateDesktopAPI)(PVOID);

static SHCreateDesktopAPI SHCreateDesktopOrig;
static SHPtrParamAPI DwmGetColorizationParametersOrig;
static SHCreateDesktopAPI SHDesktopMessageLoop; //TEST

typedef HWND(WINAPI* CreateWindowInBandAPI)(DWORD, LPWSTR, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, DWORD);
static CreateWindowInBandAPI CreateWindowInBandOrig;

typedef HWND(WINAPI* CreateWindowInBandExAPI)(DWORD, LPWSTR, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID, DWORD, DWORD);
static CreateWindowInBandExAPI CreateWindowInBandExOrig;

typedef BOOL(WINAPI* GetWindowBandAPI)(HWND, DWORD*);
static GetWindowBandAPI GetWindowBandOrig;

typedef HWND(WINAPI* SetWindowBandApi)(HWND hwnd, HWND hwndInsertAfter, DWORD dwBand);
static SetWindowBandApi SetWindowBandApiOrg;

typedef BOOL(WINAPI* RegisterHotKeyApi)(HWND hwnd, int id, UINT fsMod, UINT vk);
static RegisterHotKeyApi RegisterHotKeyApiOrg;

typedef LONG(WINAPI* GetClassIconCB_t)(PVOID pThis, PVOID a2, int a3);
static GetClassIconCB_t GetClassIconCB_orig;

typedef LONG(WINAPI* setIcon_t)(PVOID pThis, HWND a2, HICON a3, int a4);
static setIcon_t SetIcon;

typedef VOID(WINAPI* updateItem_t)(PVOID pThis, int a2);
static updateItem_t UpdateItem;

typedef LONG(WINAPI* setIconThumb_t)(PVOID pThis, HICON a2, int a3, unsigned int a4);
static  setIconThumb_t SetIconThumb;

wiktorArray<HTHEME>* themeHandles;

// 7 {4376df10-a662-420b-b30d-958881461ef9}
// 8 {7A5FCA8A-76B1-44C8-A97C-E7173CCA5F4F}
DEFINE_GUID(IID_TrayClock7, 0x4376df10, 0xa662, 0x420b, 0xb3, 0x0d, 0x95, 0x88, 0x81, 0x46, 0x1e, 0xf9);
DEFINE_GUID(IID_TrayClock8, 0x7A5FCA8A, 0x76B1, 0x44C8, 0xA9, 0x7C, 0xE7, 0x17, 0x3C, 0xCA, 0x5F, 0x4F);

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

typedef BOOL(WINAPI* SetWindowCompositionAttributeAPI) (HWND hwnd, WINCOMPATTRDATA* pAttrData);
static SetWindowCompositionAttributeAPI SetWindowCompositionAttribute;

//extern declared in nsctree.h
HRESULT(__fastcall* CNSCHost_FillNSCOg)(uintptr_t nscHost);

//extern declared in version.h
COSVersion g_osVersion;

typedef struct {
	DWORD ColorizationColor;
	DWORD ColorizationAfterglow;
	DWORD ColorizationColorBalance;
	DWORD ColorizationAfterglowBalance;
	DWORD ColorizationBlurBalance;
	DWORD ColorizationGlassReflectionIntensity;
	DWORD ColorizationOpaqueBlend;
} DWMCOLORIZATIONPARAMS, * PDWMCOLORIZATIONPARAMS;

static BOOL IsRTMDWM()
{
	if (!IsCompositionActive()) return FALSE;

	DWMCOLORIZATIONPARAMS colors;
	CHAR buffer[0x28];
	memset(buffer, 0, 0x28);
	DwmGetColorizationParametersOrig(&buffer);
	memcpy(&colors, (PVOID)buffer, sizeof(DWMCOLORIZATIONPARAMS));
	return (colors.ColorizationGlassReflectionIntensity == 1);
}

static HWND GetTaskbarWnd()
{
	if (!hwnd_taskbar)
		hwnd_taskbar = FindWindow(L"Shell_TrayWnd", NULL);
	return hwnd_taskbar;
}

static BOOL CALLBACK FindSMCallback(HWND hwnd, LPARAM lParam)
{
	if (GetClassWord(hwnd, GCW_ATOM) == (ATOM)lParam && (GetProp(hwnd, L"StartMenuTag")))
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
		WNDCLASS dummy = { 0 };
		ATOM dv2atom = GetClassInfo(GetModuleHandle(NULL), L"DV2ControlHost", &dummy);
		EnumThreadWindows(GetCurrentThreadId(), FindSMCallback, (LPARAM)dv2atom);
	}
	return hwnd_startmenu;
}

static HWND GetTaskListThumbWnd()
{
	if (!hwnd_taskthumb)
		hwnd_taskthumb = FindWindow(L"TaskListThumbnailWnd", NULL);
	return hwnd_taskthumb;
}

const UINT ThemeChangeMessage = WM_USER + 69420;
BOOL CALLBACK RefreshWindows(HWND wnd, LPARAM prm)
{
	if (wnd == (HWND)prm) return TRUE;

	PostMessage(wnd, WM_THEMECHANGED, 0, 0);
	dbgprintf(L"themechanged sent to %i", wnd);
	return TRUE;
}

LRESULT CALLBACK NewTrayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (g_enableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // Ittr: for TH1+
		SetProgmanAsShell(); // misha: TODO hack

	if (uMsg == 0x56D) return 0;
	if (uMsg == ThemeChangeMessage) //reinit thememanager on themechanged, so that inactive msstyles is updated
	{
		for (int i = 0; i < themeHandles->size; ++i)
		{
			CloseThemeData(themeHandles->data[i]);
		}
		realloc(themeHandles->data, 0);
		themeHandles->size = 0;

		ThemeManagerInitialize();
		EnumWindows(RefreshWindows, (LPARAM)hwnd);

		uMsg = WM_THEMECHANGED;
		return CallWindowProc(g_prevTrayProc, hwnd, uMsg, wParam, lParam);
	}

	if (uMsg == WM_DISPLAYCHANGE || uMsg == WM_WINDOWPOSCHANGED)
	{
		RemoveProp(hwnd, L"TaskbarMonitor");
		SetProp(hwnd, L"TaskbarMonitor", (HANDLE)MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY));
		//send displaychanged to desktop
		if (uMsg == WM_DISPLAYCHANGE) PostMessage(hwnd_desktop, 0x44B, 0, 0);
	}
	if (uMsg == 0x574) //handledelayboot
	{
		if (lParam == 3)
			return CallWindowProc(g_prevTrayProc, hwnd, 0x5B5, wParam, lParam); //fire ShellDesktopSwitch event
		if (lParam == 1)
			SetEvent(hEvent_DesktopVisible);
		return 0;
	}

	return CallWindowProc(g_prevTrayProc, hwnd, uMsg, wParam, lParam);
}

void ShimDesktop8()
{
	static int InitOnce = FALSE;
	if (InitOnce) return;
	hwnd_desktop = FindWindow(L"Progman", L"Program Manager");
	HWND hwndTray = GetTaskbarWnd();
	if (!hwnd_desktop || !hwndTray) return;
	InitOnce = TRUE;
	//hook tray
	g_prevTrayProc = (WNDPROC)GetWindowLongPtr(hwndTray, GWLP_WNDPROC);
	SetWindowLongPtr(hwndTray, GWLP_WNDPROC, (LONG_PTR)NewTrayProc);
	//set monitor (doh!)
	SetProp(hwndTray, L"TaskbarMonitor", (HANDLE)MonitorFromWindow(hwndTray, MONITOR_DEFAULTTOPRIMARY));
	//init desktop	
	PostMessage(hwnd_desktop, 0x45C, 1, 1); //wallpaper
	PostMessage(hwnd_desktop, 0x45E, 0, 2); //wallpaper host
	PostMessage(hwnd_desktop, 0x45C, 2, 3); //wallpaper & icons
	PostMessage(hwnd_desktop, 0x45B, 0, 0); //final init
	PostMessage(hwnd_desktop, 0x40B, 0, 0); //pins
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
	//SHPtrParamAPI SHCloseDesktopHandle;
	//SHCloseDesktopHandle = (SHPtrParamAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"),(LPSTR)206);
	//SHCloseDesktopHandle(p1);
	return ret;
}

DWORD WINAPI DwmGetColorizationParametersNEW(PDWMCOLORIZATIONPARAMS colors)
{
	CHAR buffer[0x28];
	memset(buffer, 0, 0x28);
	dbgprintf(L"DwmGetColorizationParameters\nColorizationColor %p\nColorizationAfterglow %p\nColorizationColorBalance %p\nColorizationAfterglowBalance %p\nColorizationBlurBalance %p\nColorizationGlassReflectionIntensity %p\nColorizationOpaqueBlend %p",
		colors->ColorizationColor, colors->ColorizationAfterglow, colors->ColorizationColorBalance, colors->ColorizationAfterglowBalance, colors->ColorizationBlurBalance, colors->ColorizationGlassReflectionIntensity, colors->ColorizationOpaqueBlend);
	DWORD ret = DwmGetColorizationParametersOrig(&buffer);
	memcpy(colors, (PVOID)buffer, sizeof(DWMCOLORIZATIONPARAMS));
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
	dbgprintf(L"SetWindowCompositionAttribute %X %x %d", hwnd, pAttrData->attribute, *(DWORD*)pAttrData->pData);
	if (_WIN_BLUE && IsCompositionActive()) //If we are 8.1 or higher for some reason Tihiy's original hack doesn't work so we forcefully run this instead
	{
		/*
			NOTE TO SELF (AND OTHERS):
			PSEUDO-AERO STILL WORKS ON 8.1 AND 10.
			TRANSPARENCY MAKES IT WORK JUST FINE.
		*/
		if (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd() || hwnd == GetTaskListThumbWnd()) //enable rtm pseudo-aero
		{
			ForceActiveWindowAppearance(hwnd);
			return SetWindowCompositionAttribute(hwnd, pAttrData);
		}
	}
	else if (!_WIN_BLUE && pAttrData->attribute == 0x10) //changed in 7->8
	{
		pAttrData->attribute = 0xF;
		if (IsRTMDWM() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd())) //enable rtm pseudo-aero
		{
			SetWindowCompositionAttribute(hwnd, pAttrData);
			WINCOMPATTRDATA rtm;
			struct ATTR13DATA
			{
				DWORD p1;
				DWORD p2;
				DWORD p3;
				DWORD p4;
			};
			ATTR13DATA attr13 = { 0 };
			attr13.p1 = 4;
			rtm.attribute = 0x13;
			rtm.pData = &attr13;
			rtm.dataSize = 0x10;
			return SetWindowCompositionAttribute(hwnd, &rtm);
		}
	}
	return SetWindowCompositionAttribute(hwnd, pAttrData);
}

HRESULT WINAPI DwmEnableBlurBehindWindowNEW(HWND hwnd, DWM_BLURBEHIND* pBlurBehind)
{
	if(hwnd == GetTaskListThumbWnd()) ForceActiveWindowAppearance(hwnd);
	//if ( IsRTMDWM() && (hwnd == GetTaskbarWnd() || hwnd == GetStartMenuWnd()) ) //enable rtm pseudo-aero
		//pBlurBehind->fEnable = 0;
	return DwmEnableBlurBehindWindow(hwnd, pBlurBehind);
}

int WINAPI SetWindowRgnNEW(HWND hWnd, HRGN hRgn, BOOL bRedraw)
{
	//don't allow to reset start menu rgn - rtm pseudo aero glitches
	if (hRgn == NULL && hWnd == GetStartMenuWnd()) return 0;
	return SetWindowRgn(hWnd, hRgn, bRedraw);
}

HRESULT WINAPI SetWindowThemeNEW(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList)
{
	//Ittr: Temporarily comment these out as unneeded - we want original 7 theme classes to be used where appropriate!
	//if ( lstrcmp(pszSubAppName,L"VerticalShowDesktop") == 0 ) return SetWindowTheme(hwnd,L"VerticalShowDesktop8",pszSubIdList);
	//if ( lstrcmp(pszSubAppName,L"ShowDesktop") == 0 ) return SetWindowTheme(hwnd,L"ShowDesktop8",pszSubIdList);
	return SetWindowTheme(hwnd, pszSubAppName, pszSubIdList);
}

UINT WINAPI SetErrorModeNEW(UINT uMode)
{
	SetCurrentProcessExplicitAppUserModelID(L"Microsoft.Windows.Explorer");

	if (g_enableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074)
		CreateTwinUI_UWP();

	return SetErrorMode(uMode);
}

typedef BOOL(WINAPI* IsShellWindow_t)(HWND);
IsShellWindow_t IsShellFrameWindow = nullptr;
//IsShellWindow_t IsShellManagedWindow = nullptr;

typedef HWND(WINAPI* GhostWindowFromHungWindow_t)(HWND);
GhostWindowFromHungWindow_t GhostWindowFromHungWindow = nullptr;

ATOM g_SecondaryTaskbarAtom;

HWND* v_hwndDesktop;



//bool IsValidTabWindowForTray(HWND hwnd)
//{
//	bool bValidTabWindow = false;
//
//	if (hwnd)
//	{
//		HWND hwndA920 = (HWND)GetPropW(hwnd, (LPCWSTR)0xA920);
//		IPropertyStore* propertyStore;
//		//wil::unique_cotaskmem_string appId;
//		if (hwndA920)
//		{
//			if (SUCCEEDED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&propertyStore))))
//			{
//				wil::PropertyStoreHelper propertyStoreHelper(propertyStore.get());
//				if (SUCCEEDED(propertyStoreHelper.GetString(PKEY_AppUserModel_ID, appId.put())) && appId)
//				{
//					wil::com_ptr<IShellItem2> shellItem2;
//					if (SUCCEEDED(SHCreateItemInKnownFolder(FOLDERID_AppsFolder, KF_FLAG_DONT_VERIFY, appId.get(), IID_PPV_ARGS(&shellItem2))))
//					{
//						bValidTabWindow = true;
//					}
//				}
//			}
//		}
//		else
//		{
//			bValidTabWindow = true;
//		}
//	}
//
//	return bValidTabWindow;
//}

BOOL ShouldAddWindowToTrayHelper(HWND hwnd)
{
	//if (Feature_WindowTabHost && !IsValidTabWindowForTray(hwnd))
	//	return FALSE;

	DWORD dwExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
	return (!GetWindow(hwnd, GW_OWNER) || (dwExStyle & WS_EX_APPWINDOW) != 0) && (dwExStyle & WS_EX_TOOLWINDOW) == 0;
}

namespace ShellManagedWindowHelper
{
	bool ShouldTreatShellManagedWindowAsNotShellManaged(HWND hwnd)
	{
		return GetPropW(hwnd, L"Microsoft.Windows.ShellManagedWindowAsNormalWindow") != 0 || GetPropW(hwnd, L"Windows.ImmersiveShell.DisableShowingMainViewOnActivation") == 0;
	}
}

enum ZBID
{
	ZBID_DEFAULT = 0,
	ZBID_DESKTOP = 1,
	ZBID_UIACCESS = 2,
	ZBID_IMMERSIVE_IHM = 3,
	ZBID_IMMERSIVE_NOTIFICATION = 4,
	ZBID_IMMERSIVE_APPCHROME = 5,
	ZBID_IMMERSIVE_MOGO = 6,
	ZBID_IMMERSIVE_EDGY = 7,
	ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
	ZBID_IMMERSIVE_INACTIVEDOCK = 9,
	ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
	ZBID_IMMERSIVE_ACTIVEDOCK = 11,
	ZBID_IMMERSIVE_BACKGROUND = 12,
	ZBID_IMMERSIVE_SEARCH = 13,
	ZBID_GENUINE_WINDOWS = 14,
	ZBID_IMMERSIVE_RESTRICTED = 15,
	ZBID_SYSTEM_TOOLS = 16,
	ZBID_LOCK = 17,
	ZBID_ABOVELOCK_UX = 18,
};

struct BandData
{
	ZBID id;
	bool bInclude;
};

static const BandData s_bandInclusionData[] =
{
	{ ZBID_DEFAULT, false },
	{ ZBID_DESKTOP, true },
	{ ZBID_UIACCESS, true },
	{ ZBID_IMMERSIVE_IHM, false },
	{ ZBID_IMMERSIVE_NOTIFICATION, false },
	{ ZBID_IMMERSIVE_APPCHROME, false },
	{ ZBID_IMMERSIVE_MOGO, false },
	{ ZBID_IMMERSIVE_EDGY, false },
	{ ZBID_IMMERSIVE_INACTIVEMOBODY, false },
	{ ZBID_IMMERSIVE_INACTIVEDOCK, false },
	{ ZBID_IMMERSIVE_ACTIVEMOBODY, false },
	{ ZBID_IMMERSIVE_ACTIVEDOCK, false },
	{ ZBID_IMMERSIVE_BACKGROUND, false },
	{ ZBID_IMMERSIVE_SEARCH, false },
	{ ZBID_GENUINE_WINDOWS, false },
	{ ZBID_IMMERSIVE_RESTRICTED, false },
	{ ZBID_SYSTEM_TOOLS, true },
	{ ZBID_LOCK, false },
	{ ZBID_ABOVELOCK_UX, false }
};

BOOL WINAPI GetWindowBandNew(HWND hwnd, DWORD* out);

BOOL __stdcall GetWindowBandHelper(HWND hwnd, ZBID* out)
{
	if (GetWindowBandOrig)
	{
		return GetWindowBandNew(hwnd, (DWORD*)out);
	}

	static BOOL(__stdcall * fn)(HWND, ZBID*) = nullptr;
	if (!fn)
	{
		HMODULE h = GetModuleHandleW(L"user32.dll");
		if (h)
			fn = (decltype(fn))GetProcAddress(h, "GetWindowBand");
		//FAIL_FAST_IF_NULL(fn);
		if (!fn)
			return 0;
	}
	return fn(hwnd, out);
}


typedef BOOL(*IsShellManagedWindow_t)(HWND hwnd); // 2574
BOOL IsShellManagedWindow(HWND hwnd)
{
	static IsShellManagedWindow_t fn = nullptr;
	if (!fn)
	{
		HMODULE h = GetModuleHandleW(L"user32.dll");
		if (h)
			fn = (IsShellManagedWindow_t)GetProcAddress(h, MAKEINTRESOURCEA(2574));
		//FAIL_FAST_IF_NULL(fn);
		if (!fn)
			return 0;
	}
	return fn(hwnd);
}

bool ShouldExcludeFromTaskbar(HWND hwnd)
{
	wchar_t text[256];
	GetWindowTextW(hwnd, text, 255);

	if (!StrCmpW(text, L"Microsoft Text Input Application") || !StrCmpW(text, L"Windows Shell Experience Host") || !StrCmpW(text, L"Start") || !StrCmpW(text, L"Search"))
		return true;

	return false;
}

bool IsValidDesktopZOrderBand(HWND hwnd, BOOL bCheckShellManagedWindow)
{
	bool bValid = false;

	ZBID band;
	if (GetWindowBandHelper(hwnd, &band))
	{
		bValid = s_bandInclusionData[band].bInclude;

		//if (Feature_WindowTabHost && (HWND)GetPropW(hwnd, (LPCWSTR)0xA920))
		//	bValid = true;

		if (bValid && bCheckShellManagedWindow)
			bValid = !IsShellManagedWindow(hwnd) || ShellManagedWindowHelper::ShouldTreatShellManagedWindowAsNotShellManaged(hwnd);
	}

	if (bValid)
		bValid = !ShouldExcludeFromTaskbar(hwnd);

	return bValid;
}

bool IsWindowNotDesktopOrTray(HWND hwnd)
{
	if (!IsWindow(hwnd) || !IsValidDesktopZOrderBand(hwnd, TRUE) || hwnd == hwnd_taskbar || (v_hwndDesktop && hwnd == *v_hwndDesktop))
		return false;

	//if (GetClassWord(hwnd, GCW_ATOM) == g_SecondaryTaskbarAtom)
	//	return g_SecondaryTaskbarAtom == 0;

	return true;
}

//removes immersive background windows
//(Microsoft Text Input Host, Shell Experience Host, etc.)
BOOL WINAPI IsWindowVisibleNEW(HWND hWnd)
{
	if (!IsWindowVisible(hWnd) || !IsValidDesktopZOrderBand(hWnd, TRUE))
		return FALSE;

	BOOL bCloaked;
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &bCloaked, sizeof(BOOL));
	if (bCloaked)
		return FALSE;

	if (IsShellFrameWindow && GhostWindowFromHungWindow)
	{
		if (IsShellFrameWindow(hWnd) && !GhostWindowFromHungWindow(hWnd))
			return TRUE;
	}

	//if (IsShellManagedWindow)
	{
		if (IsShellManagedWindow(hWnd) && GetPropW(hWnd, L"Microsoft.Windows.ShellManagedWindowAsNormalWindow") == NULL)
			return FALSE;
	}

	return TRUE;
}

__int64 ShouldAddWindowToTray(HWND hwnd)
{
	BOOL ret = IsWindowNotDesktopOrTray(hwnd) && IsWindowVisibleNEW(hwnd) && ShouldAddWindowToTrayHelper(hwnd);
	//dbgprintf(L"ShouldAddWindowToTray %i", (int)ret);
	return ret;
}

__int64(__fastcall* DwmpActivateLivePreview)(int a1, __int64 a2, __int64 a3, int a4, void* a5);
__int64 DwmpActivateLivePreviewNEW(int a1, __int64 a2, __int64 a3, int a4, void* a5)
{
	if (a5 == (void*)8)
		a5 = 0;

	if (a5 && IsBadReadPtr(a5, 0x8))
		a5 = 0;
	return DwmpActivateLivePreview(a1, a2, a3, a4, a5);
}

//Ittr: Intercept these functions where appropriate for basic theme to be forced at compile time if required
BOOL WINAPI IsCompositionActiveNEW()
{
	if (g_bDisableComposition) { return FALSE; }

	return IsCompositionActive();
}

HRESULT WINAPI DwmIsCompositionEnabledNEW(BOOL* pfEnabled)
{
	if (g_bDisableComposition) { return 0x80263001; } //0x80263001 is the value to signify composition being disabled for some reason

	return DwmIsCompositionEnabled(pfEnabled);
}

HICON GetUWPIcon(HWND a2)
{
	HICON icon = NULL;
	IShellItemImageFactory* psiif = nullptr;
	IPropertyStore* ips;
	SHGetPropertyStoreForWindow(a2, IID_PPV_ARGS(&ips));
	GUID myGuid = { 0x9F4C2855, 0x9F79, 0x4B39, {0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3} };
	PROPERTYKEY propertyKey = { myGuid, 5 };
	PROPVARIANT pv;
	ips->GetValue(propertyKey, &pv);
	if (pv.vt == VT_LPWSTR)
	{
		LPCWSTR aumid = pv.pwszVal;
		SHCreateItemInKnownFolder(FOLDERID_AppsFolder, KF_FLAG_DONT_VERIFY, aumid, IID_PPV_ARGS(&psiif));
		if (psiif)
		{
			SIIGBF flags = SIIGBF_ICONONLY;
			HBITMAP hb;
			SIZE size = { GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CXICON) };
			HRESULT hr = psiif->GetImage(size, flags, &hb);
			if (SUCCEEDED(hr))
			{
				HIMAGELIST hImageList = ImageList_Create(size.cx, size.cy, ILC_COLOR32, 1, 0);
				if (ImageList_Add(hImageList, hb, NULL) != -1)
				{
					HICON hc = ImageList_GetIcon(hImageList, 0, 0);
					ImageList_Destroy(hImageList);

					// set
					icon = hc;

					DeleteObject(hb);
					psiif->Release();
				}
				DeleteObject(hb);
			}
			psiif->Release();
		}
	}
	ips->Release();
	return icon;
}

VOID UpdateItemIcon(PVOID This, int a2)
{
	typedef void* (__fastcall* GetTaskItemFunc)(void*);
	typedef HWND(__fastcall* GetWindowFunc)(void*);

	HDPA hdpaTaskThumbnails = *(HDPA*)((PBYTE)This + 0xB0);
	auto v4 = DPA_FastGetPtr(hdpaTaskThumbnails, a2);
	auto vtable = *(uintptr_t**)v4;
	GetTaskItemFunc GetTaskItem = (GetTaskItemFunc)vtable[0x60 / sizeof(uintptr_t)];
	void* v5 = GetTaskItem(v4);
	auto vtable_v5 = *(uintptr_t**)v5;
	GetWindowFunc GetWindow = (GetWindowFunc)vtable_v5[0x98 / sizeof(uintptr_t)];
	HWND v6 = GetWindow(v5);
	if (IsShellFrameWindow && IsShellFrameWindow(v6))
	{
		//OutputDebugStringW(L"uwp window");
		HICON hc = GetUWPIcon(v6);
		if (hc) SetIconThumb(This, hc, a2, 3);
	}
	else
		UpdateItem(This, a2);

}

VOID SetWindowIcon(PVOID This, HWND a2, HICON a3, int a4)
{
	if (IsShellFrameWindow && IsShellFrameWindow(a2))
	{
		HICON hc = GetUWPIcon(a2);
		if (hc) SetIcon(This, a2, hc, a4);
	}
	else
		SetIcon(This, a2, a3, a4);
}

/*
INT64 GetClassIconCB(PVOID This, struct ICONBCPARAM* a2, int a3)
{
	INT64 ret = 0;
	if (IsShellFrameWindow)
	{
		if (IsShellFrameWindow(*(HWND*)a2+1))
		{
			return 4294967294;
			//return GetClassIconCB_orig(This, a2, a3);
		}
		else
		{
			return GetClassIconCB_orig(This, a2, a3);
		}
	}
	return 0;
}
*/

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

//Ittr: Goodbye immersive context menus and good riddance. For Win10 TH1+. In future consider build check to limit to 10240+. 
//Also to be noted that Windows 11 makes further changes here that we'll need to account for in future if we do officially support it.
void ShowWin32Menus()
{
	//The bytes are the same in both dlls for ImmersiveContextMenuHelper::CanApplyOwnerDrawToMenu function
	char* immersiveBytes = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B";

	//Load and patch both DLLs. ExplorerFrame gets called in later so we have to account for that
	unsigned char bytes[] = { 0xB0, 0x00, 0xC3 };
	ChangeImportedPattern((char*)FindPattern((uintptr_t)GetModuleHandle(L"shell32.dll"), immersiveBytes), bytes, sizeof(bytes));
	ChangeImportedPattern((char*)FindPattern((uintptr_t)LoadLibrary(L"ExplorerFrame.dll"), immersiveBytes), bytes, sizeof(bytes));
}

void FixAuthUI()
{
	// Newer explorer versions use this
	// CLogoffPane::_InitShutdownObjects
	const char* bytes = "48 8B ?? 98 00 00 00 48 8B ?? 40 45 33 C0 48 8B 01 FF 50 18 "
		"48 8B ?? 98 00 00 00 48 8B 01 FF 50 30 8B D8 "
		"85 C0 ?? ?? "
		"48 8B ?? 98 00 00 00 48 8D ?? A0 00 00 00 48 8B 01 FF 50 20";

	// Older explorer versions use this
	// CLogoffPane::_OnCreate
	const char* bytesOld = "48 8B 8B 98 00 00 00 48 8B 53 40 45 33 C0 48 8B 01 FF 50 18 "
		"48 8B 8B 98 00 00 00 48 8B 01 FF 50 30 44 8B C8 "
		"85 C0 ?? ?? ?? ?? ?? ?? "
		"48 8B 8B 98 00 00 00 48 8D 93 A0 00 00 00 48 8B 01 FF 50 20";

	char* pattern = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), bytes);
	char* pattern1 = (char*)FindPattern((uintptr_t)GetModuleHandle(NULL), bytesOld);

	DWORD old;
	unsigned char patch1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	SIZE_T size = sizeof(patch1);
	if (pattern)
	{
		// mov rax, [rcx]
		// call qword ptr [rax+18h]
		char* inst1 = pattern + 14;
		ChangeImportedPattern(inst1, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+30h]
		char* inst2 = pattern + 27;
		ChangeImportedPattern(inst2, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+20h]
		char* inst3 = pattern + 53;
		ChangeImportedPattern(inst3, patch1, size);
	}

	if (pattern1 && !pattern) //Ittr: Only apply to CLogoffPane::_OnCreate if we need to, otherwise this causes crashing on later 7 explorer.
	{
		// mov rax, [rcx]
		// call qword ptr [rax+18h]
		char* inst1 = pattern1 + 14;
		ChangeImportedPattern(inst1, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+30h]
		char* inst2 = pattern1 + 27;
		ChangeImportedPattern(inst2, patch1, size);

		// mov rax, [rcx]
		// call qword ptr [rax+20h]
		char* inst3 = pattern1 + 58;
		ChangeImportedPattern(inst3, patch1, size);
	}
}


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

bool IsClassicTheme(void)
{
	return !IsThemeActive() || g_bClassicTheme;
}

bool AllowThemes(void)
{
	return !IsClassicTheme();
}

HTHEME(__stdcall* fOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
HTHEME(__stdcall* fOpenThemeDataForDpi)(HWND hwnd, LPCWSTR pszClassList, UINT dpi);
HTHEME(__stdcall* fOpenThemeDataEx)(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags);
HTHEME __stdcall OpenThemeData_Hook(HWND hwnd, LPCWSTR pszClassList)
{
	if (g_dwTrayThreadId > 0 && g_dwTrayThreadId != GetCurrentThreadId())
		return fOpenThemeData(hwnd, pszClassList);

	if (!AllowThemes())
		return NULL;

	HTHEME theme = 0;
	DWORD flags = 2;
	if ((unsigned int)GetScreenDpi() != 96)
		flags |= 1u;

	if (g_loadedTheme)
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassList, flags);
	else
		theme = fOpenThemeData(hwnd, pszClassList);

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATA FAILED %s", pszClassList);
	themeHandles->push_back(theme);
	return theme;
}

HTHEME __stdcall OpenThemeDataForDpi_Hook(HWND hwnd, LPCWSTR pszClassList, UINT dpi)
{
	if (g_dwTrayThreadId > 0 && g_dwTrayThreadId != GetCurrentThreadId())
		return fOpenThemeDataForDpi(hwnd, pszClassList, dpi);

	if (!AllowThemes())
		return NULL;

	HTHEME theme = 0;
	DWORD flags = 2;
	if (dpi != 96)
		flags |= 1u;

	if (g_loadedTheme)
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassList, flags);
	else
		theme = fOpenThemeDataForDpi(hwnd, pszClassList, dpi);

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATAFORDPI FAILED %s", pszClassList);
	themeHandles->push_back(theme);
	return theme;
}

HTHEME __stdcall OpenThemeDataEx_Hook(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags)
{
	if (g_dwTrayThreadId > 0 && g_dwTrayThreadId != GetCurrentThreadId())
		return fOpenThemeDataEx(hwnd, pszClassList, dwFlags);

	if (!AllowThemes())
		return NULL;

	HTHEME theme = 0;
	DWORD flags = 2;
	if ((unsigned int)GetScreenDpi() != 96)
		flags |= 1u;

	if (g_loadedTheme)
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassList, dwFlags | flags);
	else
		theme = fOpenThemeDataEx(hwnd, pszClassList, dwFlags);

	if (theme == nullptr)
		dbgprintf(L"OPENTHEMEDATAEX FAILED %s", pszClassList);
	themeHandles->push_back(theme);
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

	return CTray__SyncThreadProc_orig(lpParameter);
}

void HookTrayThread(void)
{
	CTray__SyncThreadProc_orig = (LPTHREAD_START_ROUTINE)FindPattern(
		(uintptr_t)GetModuleHandle(NULL),
		"48 81 EC 88 00 00 00 83 3D 02 70 06 00 00 E9 5A 2D 01 00"
	);

	if (CTray__SyncThreadProc_orig)
	{
		MH_CreateHook(
			(void*)CTray__SyncThreadProc_orig,
			(void*)CTray__SyncThreadProc_hook,
			(void**)&CTray__SyncThreadProc_orig
		);
	}
}

/* Adjust a window's position to be pushed away from the taskbar */
SIZE AdjustWindowRectForTaskbar(RECT* lprc)
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
			int* set = (i % 2 == 0) ? &dx : &dy;
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

BOOL WINAPI CalculatePopupWindowPositionNEW(
	const POINT* anchorPoint,
	const SIZE* windowSize,
	UINT         flags,
	RECT* excludeRect,
	RECT* popupWindowPosition
)
{
	BOOL res = CalculatePopupWindowPosition(
		anchorPoint, windowSize, flags,
		excludeRect, popupWindowPosition
	);
	if (IsCompositionActiveNEW() && res && (flags & TPM_WORKAREA) != 0)
	{
		SIZE adjust = AdjustWindowRectForTaskbar(popupWindowPosition);
		OffsetRect(popupWindowPosition, adjust.cx, adjust.cy);
	}
	return res;
}


HWND WINAPI CreateWindowInBandNew(DWORD dwExStyle,
	LPCWSTR lpClassName,
	LPCWSTR lpWindowName,
	DWORD dwStyle,
	int x,
	int y,
	int nWidth,
	int nHeight,
	HWND hwndParent,
	HMENU hMenu,
	HINSTANCE hInstance,
	LPVOID lpParam,
	DWORD dwBand)
{
	if (g_enableImmersiveShellStack) // UWP enabled
	{
		DWORD p0 = (DWORD)_ReturnAddress();
		dwExStyle = dwExStyle | WS_EX_TOOLWINDOW; // TODO is this needed
		HWND ret = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hwndParent, hMenu, hInstance, lpParam);

		dbgprintf(L"CREATEWINDOWINBANDNEW %i", dwBand);

		if (ret)
		{
			SetProp(ret, L"UIA_WindowVisibilityOverriden", (HANDLE)2);
			SetProp(ret, L"explorer7.WindowBand", (HANDLE)dwBand);
		}

		return ret;
	}
	else // Ittr: Preserve legacy codepath for win8.x and non-UWP users
	{
		DWORD p0 = (DWORD)_ReturnAddress();
		dwStyle = dwStyle | WS_EX_TOOLWINDOW;
		HWND ret = CreateWindowInBandOrig(dwExStyle, (LPWSTR)lpClassName, (PVOID)lpWindowName, (PVOID)dwStyle, (PVOID)x, (PVOID)y, (PVOID)nWidth, (PVOID)nHeight, hwndParent, hMenu, hInstance, lpParam, dwBand & 1);
		dbgprintf(L"%p: CreateWindowInBand %p %s %p %p %p %p %p %p %p %p %p %p %p = %p %p", p0, dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hwndParent, hMenu, hInstance, lpParam, dwBand, ret, GetLastError());
		SetProp(ret, L"explorer7.WindowBand", (HANDLE)dwBand);
		return ret;
	}
}

HWND WINAPI CreateWindowInBandExNew(DWORD exStyle, LPWSTR szClassName, PVOID p3, PVOID p4, PVOID p5, PVOID p6, PVOID p7, PVOID p8, PVOID p9, PVOID p10, PVOID p11, PVOID p12, DWORD p13, DWORD dwTypeFlags)
{
	DWORD p0 = (DWORD)_ReturnAddress();
	exStyle = exStyle | WS_EX_TOOLWINDOW;
	HWND ret = CreateWindowInBandExOrig(exStyle, szClassName, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13 & 1, dwTypeFlags);
	dbgprintf(L"%p: CreateWindowInBandEx %p %s %p %p %p %p %p %p %p %p %p %p %p = %p %p", p0, exStyle, szClassName, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, ret, GetLastError());
	dbgprintf(L"CreateWindowInBandExOrig %i", p13);

	SetProp(ret, L"UIA_WindowVisibilityOverriden", (HANDLE)2);
	SetProp(ret, L"explorer7.WindowBand", (HANDLE)p13);
	return ret;
}

BOOL WINAPI SetWindowBandNew(HWND hwnd, HWND hwndInsertAfter, DWORD flags)
{
	SetProp(hwnd, L"explorer7.WindowBand", (HANDLE)flags);
	dbgprintf(L"SetWindowBandNew %i", flags);
	return TRUE;
}

BOOL WINAPI ReturnZero()
{
	return TRUE;
}

BOOL WINAPI RegisterWindowHotkeyNew(HWND hwnd, int id, UINT mod, UINT vk)
{
	BOOL res = RegisterHotKeyApiOrg(hwnd, id, mod, vk);

	if (!res)
	{
		return TRUE;
	}

	return TRUE;
}

BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void GetOrbDPIAndPos(LPWSTR fName)
{
	APPBARDATA abd;
	abd.cbSize = sizeof(APPBARDATA);
	SHAppBarMessage(ABM_GETTASKBARPOS, &abd);

	HDC screen = GetDC(NULL);
	double hPixelsPerInch = GetDeviceCaps(screen, LOGPIXELSX);
	double vPixelsPerInch = GetDeviceCaps(screen, LOGPIXELSY);
	ReleaseDC(NULL, screen);
	double dpi = (hPixelsPerInch + vPixelsPerInch) * 0.5;

	if (dpi >= 120)
	{
		if (dpi >= 144)
		{
			if (dpi >= 192)
			{
				if (abd.uEdge == ABE_LEFT || abd.uEdge == ABE_RIGHT) StringCchCopyW(fName, MAX_PATH, L"6808");
				else if (abd.uEdge == ABE_TOP) StringCchCopyW(fName, MAX_PATH, L"6812");
				else StringCchCopyW(fName, MAX_PATH, L"6804");
			}
			else
			{
				if (abd.uEdge == ABE_LEFT || abd.uEdge == ABE_RIGHT) StringCchCopyW(fName, MAX_PATH, L"6807");
				else if (abd.uEdge == ABE_TOP) StringCchCopyW(fName, MAX_PATH, L"6811");
				else StringCchCopyW(fName, MAX_PATH, L"6803");
			}
		}
		else
		{
			if (abd.uEdge == ABE_LEFT || abd.uEdge == ABE_RIGHT) StringCchCopyW(fName, MAX_PATH, L"6806");
			else if (abd.uEdge == ABE_TOP) StringCchCopyW(fName, MAX_PATH, L"6810");
			else StringCchCopyW(fName, MAX_PATH, L"6802");
		}
	}
	else
	{
		if (abd.uEdge == ABE_LEFT || abd.uEdge == ABE_RIGHT) StringCchCopyW(fName, MAX_PATH, L"6805");
		else if (abd.uEdge == ABE_TOP) StringCchCopyW(fName, MAX_PATH, L"6809");
		else StringCchCopyW(fName, MAX_PATH, L"6801");
	}
}

HANDLE __stdcall LoadImageW_CallHook(HINSTANCE hInst, LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad)
{
	dbgprintf(L"LoadImageW_CallHook has been called!");

	WCHAR szExeDir[MAX_PATH];
	GetModuleFileNameW(NULL, szExeDir, MAX_PATH);
	WCHAR* backslash = StrRChrW(szExeDir, NULL, L'\\');
	if (*backslash == L'\\')
		*backslash = L'\0';

	WCHAR szOrbDir[MAX_PATH];
	LSTATUS res = g_registry.QueryValue(L"OrbDirectory", (LPBYTE)szOrbDir, sizeof(szOrbDir));
	if (!*szOrbDir || ERROR_SUCCESS != res)
		return LoadImageW(hInst, name, type, cx, cy, fuLoad);

	WCHAR szOrbFile[MAX_PATH];
	GetOrbDPIAndPos(szOrbFile);

	WCHAR szOrbPath[MAX_PATH * 3];
	wsprintfW(
		szOrbPath,
		L"%s\\orbs\\%s\\%s.bmp",
		szExeDir,
		szOrbDir,
		szOrbFile
	);

	if (FileExists(szOrbPath) == FALSE)
	{
		return LoadImageW(hInst, name, type, cx, cy, fuLoad);
	}
	else
	{
		return LoadImageW(NULL, szOrbPath, IMAGE_BITMAP, 0, 0, fuLoad | LR_LOADFROMFILE);
	}
}

void HookLoadImageForSizeAndFont()
{
	auto callLoadImage = (uintptr_t)FindPattern((uintptr_t)GetModuleHandle(0), "FF 15 ?? ?? ?? ?? 48 89 43 ?? 48 85 C0 74 ?? 4C 8D ?? ?? ?? BA ?? ?? ?? ?? 48 8B C8 FF 15");
	if (callLoadImage)
	{
		//write a nop
		DWORD old;
		VirtualProtect((void*)callLoadImage, 1, PAGE_EXECUTE_READWRITE, &old);
		*reinterpret_cast<char*>(callLoadImage) = 0x90;
		VirtualProtect((void*)callLoadImage, 1, old, 0);

		callLoadImage += 1;

		// write a call to our function
		DetourCall((void*)callLoadImage, LoadImageW_CallHook);


	}

	char* callDrawExtended = (char*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 57 48 83 EC 30 33 DB 48 8B F9 48 39 59 40");
	if (!callDrawExtended) return;

	if (callDrawExtended)
	{
		unsigned char bytes[] = { 0xB0,0x01,0xC3 };
		ChangeImportedPattern(callDrawExtended, bytes, sizeof(bytes));
	}
}

HRESULT WINAPI SHCoCreateInstanceNew(PCWSTR pszCLSID, const CLSID* pclsid, IUnknown* pUnkOuter, IID& riid, void** ppv)
{
	HRESULT res = SHCoCreateInstance(pszCLSID, pclsid, pUnkOuter, riid, ppv);
	if (res != S_OK && riid == GUID_88df9332_6adb_4604_8218_508673ef7f8a)
	{
		IShellURL10* shellurl10;
		res = SHCoCreateInstance(pszCLSID, pclsid, pUnkOuter, GUID_4f33718d_bae1_4f9b_96f2_d2a16e683346, (void**)&shellurl10);
		*ppv = new CShellURLWrapper(shellurl10);
	}
	return res;
}

typedef HRESULT(WINAPI *SHGetUserPicturePath_t)(LPCWSTR pszUsername, DWORD dwFlags, LPWSTR pszPath, DWORD cchPathMax);
SHGetUserPicturePath_t SHGetUserPicturePath = nullptr;

HRESULT WINAPI SHGetUserPicturePathNEW(LPCWSTR pszUsername, DWORD dwFlags, LPWSTR pszPath)
{
	DWORD dwHideUserPic = 0;
	g_registry.QueryValue(L"HideUserPicture", (LPBYTE)&dwHideUserPic, sizeof(DWORD));
	if (dwHideUserPic)
	{
		*pszPath = L'\0';
		return E_FAIL;
	}
	// The XP version of this func assumes MAX_PATH, so just pass that to the new one.
	return SHGetUserPicturePath(pszUsername, dwFlags, pszPath, MAX_PATH);
}

// Windows 10 gives the user pic path to a 126x126 bitmap. XP gave a 48x48 bitmap.
// XP uses nearest neighbor scaling, so the 126x126 bitmap looks pretty awful.
// This hook resizes it to 48x48 using a better scaling algorithm.
HANDLE WINAPI LoadImageWNEW(HINSTANCE hInst, LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad)
{
	IWICImagingFactory *factory = nullptr;
	IWICBitmapDecoder *decoder = nullptr;
	IWICBitmapFrameDecode *frameDecode = nullptr;
	IWICBitmapScaler *scaler = nullptr;
	IWICBitmapSource *convertedBitmap = nullptr;
	HBITMAP hbmUser = NULL;

	dbgprintf(L"LoadImageW call");
	// The user bitmap is the only one in explorer.exe that is loaded from file.
	if (fuLoad & LR_LOADFROMFILE)
	{
		dbgprintf(L"Load from file: %s", name);
		HRESULT hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&factory)
		);
		if (SUCCEEDED(hr))
		{
			dbgprintf(L"Create factory succeeded");
			hr = factory->CreateDecoderFromFilename(
				name,
				NULL,
				GENERIC_READ,
				WICDecodeMetadataCacheOnDemand,
				&decoder
			);
			if (SUCCEEDED(hr))
			{
				dbgprintf(L"Create decoder succeeded");
				hr = decoder->GetFrame(0, &frameDecode);
				if (SUCCEEDED(hr))
				{
					dbgprintf(L"Create frameDecode succeeded");
					hr = factory->CreateBitmapScaler(&scaler);
					if (SUCCEEDED(hr))
					{
						dbgprintf(L"Create scaler succeeded");
						hr = WICConvertBitmapSource(
							GUID_WICPixelFormat32bppPBGRA,
							frameDecode,
							&convertedBitmap
						);
						if (SUCCEEDED(hr))
						{
							dbgprintf(L"Convert bitmap succeeded");
							hr = scaler->Initialize(
								convertedBitmap,
								48, 48, WICBitmapInterpolationModeFant
							);
							if (SUCCEEDED(hr))
							{
								dbgprintf(L"Scaler init succeeded");
								UINT width = 0;
								UINT height = 0;
								scaler->GetSize(&width, &height);
								dbgprintf(L"w: %u h: %u", width, height);
								size_t bufferSize = width * height * 4;
								BYTE *buffer = new BYTE[bufferSize];
								ZeroMemory(buffer, bufferSize);
								hr = scaler->CopyPixels(
									nullptr,
									width * 4,
									bufferSize,
									buffer
								);
								if (SUCCEEDED(hr))
								{
									dbgprintf(L"Copy pixels succeeded");
									hbmUser = CreateBitmap(width, height, 1, 32, buffer);
								}
								else
									dbgprintf(L"Copy pixels failed with HR 0x%X", hr);
								delete[] buffer;
								if (hbmUser)
									dbgprintf(L"Create HBITMAP succeeded");
							}
						}
					}
				}
			}
		}
	}

	// Cleanup
	if (factory)
		factory->Release();
	if (decoder)
		decoder->Release();
	if (frameDecode)
		frameDecode->Release();
	if (scaler)
		scaler->Release();
	if (convertedBitmap)
		convertedBitmap->Release();

	if (hbmUser)
		return hbmUser;

	dbgprintf(L"Returning original");
	return LoadImageW(hInst, name, type, cx, cy, fuLoad);
}

// Fix user pic in XP Explorer
void FixWinXPUserPic()
{
	BYTE *UpdateUserInfo = (BYTE *)FindPattern((uintptr_t)GetModuleHandle(NULL), "48 89 9C 24 ?? ?? ?? ?? 75 1B 85 C0 0F 84 ?? ?? ?? ?? B9 1B 00 00 00 FF 15 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ??");
	const BYTE Patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0x90 };
	if (UpdateUserInfo)
	{
		ChangeImportedPattern(
			UpdateUserInfo + 23,
			Patch, sizeof(Patch)
		);
		dbgprintf(L"Found the pattern");
	}
	else
		dbgprintf(L"Did not Found the pattern");

	// This is the 10 SHGetUserPicturePath,
	SHGetUserPicturePath = (SHGetUserPicturePath_t)GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPCSTR)261);
	// shunimpl'd
	ChangeImportedAddress(GetModuleHandle(NULL), "shell32.dll", GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPCSTR)233), SHGetUserPicturePathNEW);
	// Fix bad scaling
	ChangeImportedAddress(GetModuleHandle(NULL), "user32.dll", LoadImageW, LoadImageWNEW);
}

HRESULT(WINAPI* DrawThemeText_orig)(HTHEME hTheme,
	HDC hdc,
	int iPartId,
	int iStateId,
	LPCWSTR pszText,
	int cchText,
	DWORD dwTextFlags,
	DWORD dwTextFlags2,
	LPCRECT pRect);
HRESULT
WINAPI
DrawThemeText_hook(
	HTHEME hTheme,
	HDC hdc,
	int iPartId,
	int iStateId,
	LPCWSTR pszText,
	int cchText,
	DWORD dwTextFlags,
	DWORD dwTextFlags2,
	LPRECT pRect)
{
	//RECT    rc;
	//GetClientRect(WindowFromDC(hdc), &rc);
	//GetThemeBackgroundContentRect(hTheme, hdc, iPartId, iStateId, pRect, pRect);
	//TCHAR className[256];
	//if (GetClassName(WindowFromDC(hdc), className, 256))

	/*
	left;
	top;
	right;
	bottom;
	*/

	if (pRect)
	{
		dbgprintf(L"rect: %p %p %i %i %s %i %i %i %i %i %i %i",hTheme,hdc,iPartId,iStateId,pszText,cchText,dwTextFlags,dwTextFlags2,pRect->left,pRect->top,pRect->right,pRect->bottom);
		//pRect->right *= 2;
	}
	//if (wcscmp(className, L"Start::Button") == 0)
	//{
	//	pRect->left = 10;
	//	pRect->top = 2;
	//	pRect->right = 24;
	//	pRect->bottom = 4;
	//}

	return DrawThemeText_orig(hTheme,hdc,iPartId,iStateId,pszText,cchText,dwTextFlags, dwTextFlags2, pRect);
}

void HookShell32();
void HookAPIs()
{
	DWORD dwEnableUWP = 1;
	g_registry.QueryValue(L"EnableImmersive", (LPBYTE)&dwEnableUWP, sizeof(DWORD)); // Ittr: shortened the name for convenience but still not sure on it at the moment. TODO determine name before release
	g_enableImmersiveShellStack = (dwEnableUWP != 0);

	hEvent_DesktopVisible = CreateEvent(NULL, TRUE, FALSE, L"ShellDesktopVisibleEvent");
	//change desktop
	SHCreateDesktopOrig = (SHCreateDesktopAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPSTR)200);
	ChangeImportedAddress(GetModuleHandle(NULL), "shell32.dll", SHCreateDesktopOrig, SHCreateDesktopNEW);
	SHDesktopMessageLoop = (SHCreateDesktopAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPSTR)201);
	ChangeImportedAddress(GetModuleHandle(NULL), "shell32.dll", SHDesktopMessageLoop, SHDesktopMessageLoopNEW);

	ChangeImportedAddress(GetModuleHandle(NULL), "shell32.dll", GetProcAddress(GetModuleHandle(L"shell32.dll"), "SHCoCreateInstance"), SHCoCreateInstanceNew);

	//ChangeImportedAddress(GetModuleHandle(NULL),"shell32.dll", GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPSTR)902), GetProcAddress(GetModuleHandle(L"shunimpl.dll"),(LPSTR)473));
	//change appid
	ChangeImportedAddress(GetModuleHandle(NULL), "kernel32.dll", SetErrorMode, SetErrorModeNEW);
	//Ittr: Disable DWM composition as quickly as we can (if compile flag set)
	ChangeImportedAddress(GetModuleHandle(NULL), "uxtheme.dll", IsCompositionActive, IsCompositionActiveNEW);

	FixWinXPUserPic();

	void* LoadAnimationDataMap = FindByString((uintptr_t)GetModuleHandle(L"uxtheme.dll"), L"AMAP");
	if (LoadAnimationDataMap)
	{
		LoadAnimationDataMap = (void*)GetFunctionStart((uintptr_t)LoadAnimationDataMap, (uintptr_t)GetModuleHandle(L"uxtheme.dll"));

		//byebye
		DWORD old;
		VirtualProtect(LoadAnimationDataMap, 1, PAGE_EXECUTE_READWRITE, &old);
		*reinterpret_cast<char*>(LoadAnimationDataMap) = 0xC3;
		VirtualProtect(LoadAnimationDataMap, 1, old, 0);
	}
	else
		MessageBox(0, L"SHite", L"FUck", 0);

	void* GetClassIdForShellTarget = FindByString((uintptr_t)GetModuleHandle(L"uxtheme.dll"), L"Immersive");
	if (GetClassIdForShellTarget)
	{
		GetClassIdForShellTarget = (void*)GetFunctionStart((uintptr_t)GetClassIdForShellTarget, (uintptr_t)GetModuleHandle(L"uxtheme.dll"));

		//byebye
		DWORD old;
		VirtualProtect(GetClassIdForShellTarget, 1, PAGE_EXECUTE_READWRITE, &old);
		*reinterpret_cast<char*>(GetClassIdForShellTarget) = 0xC3;
		VirtualProtect(GetClassIdForShellTarget, 1, old, 0);
	}

	ThemeManagerInitialize();
	fOpenThemeData = decltype(fOpenThemeData)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeData"));
	fOpenThemeDataForDpi = decltype(fOpenThemeDataForDpi)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataForDpi"));
	fOpenThemeDataEx = decltype(fOpenThemeDataEx)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataEx"));

	void* _ctaskbandadd = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "FF F3 55 56 57 41 54 41 55 41 56 41 57 48 81 EC F8 06 00 00");
	//void* _ctaskbandclasscb = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B F1 48 8B 0A 49 8B F8 48 8B DA");
	//addIcon_orig = (addIcon_t)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 48 89 6C 24 18 56 57 41 54 48 83 EC 20 45 33 E4");
	void* _cthumbnailUpdate = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 48 8B 81 B0 00 00 00");
	SetIconThumb = (setIconThumb_t)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 49 63 D8 4C 8B 81 B0 00 00 00");

	//void* fillnsc = (void*)FindPattern((uintptr_t)GetModuleHandle(0),"48 89 5C 24 18 57 48 83 EC 30 33 DB 48 8B F9 39 99 CC 00 00 00");
	CNSCHost_FillNSCOg = (decltype(CNSCHost_FillNSCOg))FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 18 57 48 83 EC 30 33 DB 48 8B F9 39 99 CC 00 00 00");
	void* _ShouldAddWindowToTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB");
	void* _IsWindowNotDesktopOrTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB FF 15 ?? ?? ?? ?? 3B C3 74 ?? 48 3B 3D");

	uintptr_t desktopHwnd = FindPattern((uintptr_t)GetModuleHandle(0), "74 ?? 48 3B 3D ?? ?? ?? ?? 8D 43 01 0F 45 D8");
	if (desktopHwnd)
	{
		desktopHwnd += 2;
		v_hwndDesktop = (HWND*)(desktopHwnd + 7 + *reinterpret_cast<signed int*>(desktopHwnd + 3));
	}

	MH_Initialize();
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeData), OpenThemeData_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeData));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataForDpi), OpenThemeDataForDpi_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataForDpi));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataEx), OpenThemeDataEx_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataEx));
	MH_CreateHook(static_cast<LPVOID>(_ShouldAddWindowToTray), ShouldAddWindowToTray, reinterpret_cast<LPVOID*>(&_ShouldAddWindowToTray));
	MH_CreateHook(static_cast<LPVOID>(_IsWindowNotDesktopOrTray), IsWindowNotDesktopOrTray, reinterpret_cast<LPVOID*>(&_IsWindowNotDesktopOrTray));
	MH_CreateHook(static_cast<LPVOID>(DrawThemeText), DrawThemeText_hook, reinterpret_cast<LPVOID*>(&DrawThemeText_orig));


	if (g_enableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // Ittr: Only execute this code if we are running in immersive mode.
	{
		// This will *need* serious optimization in the near future as it singlehandedly delays program enumeration and startup by several seconds
		MH_CreateHook(static_cast<LPVOID>(_ctaskbandadd), SetWindowIcon, reinterpret_cast<LPVOID*>(&SetIcon));
		MH_CreateHook(static_cast<LPVOID>(_cthumbnailUpdate), UpdateItemIcon, reinterpret_cast<LPVOID*>(&UpdateItem));
		//MH_CreateHook(static_cast<LPVOID>(_ctaskbandclasscb), GetClassIconCB, reinterpret_cast<LPVOID*>(&GetClassIconCB_orig));
	}


	// Todo in future *after* feature-set is complete: see how many of these hooks can be ChangeImportedAddress instead of MH_CreateHook (perf optimisation)
	// UWP stuff
	if (g_enableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // Ittr: Run these hooks only if the user A) is on Windows 10 and B) has UWP enabled
	{
		CreateWindowInBandOrig = decltype(CreateWindowInBandOrig)(GetProcAddress(GetModuleHandle(L"user32.dll"), "CreateWindowInBand"));
		CreateWindowInBandExOrig = decltype(CreateWindowInBandExOrig)(GetProcAddress(GetModuleHandle(L"user32.dll"), "CreateWindowInBandEx"));
		SetWindowBandApiOrg = decltype(SetWindowBandApiOrg)(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowBand"));
		RegisterHotKeyApiOrg = decltype(RegisterHotKeyApiOrg)(GetProcAddress(GetModuleHandle(L"user32.dll"), "RegisterHotKey"));

		MH_CreateHook(static_cast<LPVOID>(CreateWindowInBandOrig), CreateWindowInBandNew, reinterpret_cast<LPVOID*>(&CreateWindowInBandOrig));
		MH_CreateHook(static_cast<LPVOID>(CreateWindowInBandExOrig), CreateWindowInBandExNew, reinterpret_cast<LPVOID*>(&CreateWindowInBandExOrig));
		MH_CreateHook(static_cast<LPVOID>(SetWindowBandApiOrg), SetWindowBandNew, reinterpret_cast<LPVOID*>(&SetWindowBandApiOrg));
		MH_CreateHook(static_cast<LPVOID>(RegisterHotKeyApiOrg), RegisterWindowHotkeyNew, reinterpret_cast<LPVOID*>(&RegisterHotKeyApiOrg));

		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2581), ReturnZero, NULL); // GetWindowTrackInfoAsync
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2563), ReturnZero, NULL); // ClearForeground
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2628), ReturnZero, NULL); // CreateWindowGroup
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2629), ReturnZero, NULL); // DeleteWindowGroup
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2631), ReturnZero, NULL); // EnableWindowGroupPolicy
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2627), ReturnZero, NULL); // SetBridgeWindowChild
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2511), ReturnZero, NULL); // SetFallbackForeground
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2566), ReturnZero, NULL); // SetWindowArrangement
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2632), ReturnZero, NULL); // SetWindowGroup
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2579), ReturnZero, NULL); // SetWindowShowState
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2585), ReturnZero, NULL); // UpdateWindowTrackingInfo
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2514), ReturnZero, NULL); // RegisterEdgy
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2542), ReturnZero, NULL); // RegisterShellPTPListener
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2537), ReturnZero, NULL); // SendEventMessage
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2513), ReturnZero, NULL); // SetActiveProcessForMonitor
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2564), ReturnZero, NULL); // RegisterWindowArrangementCallout
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2567), ReturnZero, NULL); // EnableShellWindowManagementBehavior
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), "AllowSetForegroundWindow"), ReturnZero, NULL);
	}

	if (CNSCHost_FillNSCOg && g_osVersion.BuildNumber() >= 10240)
		MH_CreateHook(static_cast<LPVOID>(CNSCHost_FillNSCOg), CNSCHost_FillNSC, reinterpret_cast<LPVOID*>(&CNSCHost_FillNSCOg)); //this hook is in nsctree.h now
	HookTrayThread();


	//ChangeImportedAddress(GetModuleHandle(NULL),"uxtheme.dll", GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeData"), OpenThemeData_Hook);
	//ChangeImportedAddress(GetModuleHandle(NULL),"uxtheme.dll", GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "CloseThemeData"), CloseThemeDataNEW);
	ChangeImportedAddress(GetModuleHandle(NULL), "dwmapi.dll", DwmIsCompositionEnabled, DwmIsCompositionEnabledNEW);
	//adapt colorization api
	DwmGetColorizationParametersOrig = (SHPtrParamAPI)GetProcAddress(GetModuleHandle(L"dwmapi.dll"), (LPSTR)127);
	DwmpActivateLivePreview = (decltype(DwmpActivateLivePreview))GetProcAddress(GetModuleHandle(L"dwmapi.dll"), (LPSTR)113);
	ChangeImportedAddress(GetModuleHandle(NULL), "dwmapi.dll", DwmpActivateLivePreview, DwmpActivateLivePreviewNEW);
	ChangeImportedAddress(GetModuleHandle(NULL), "dwmapi.dll", DwmGetColorizationParametersOrig, DwmGetColorizationParametersNEW);

	//8RTM - composition
	//Ittr: Restore active DWM "colorization" to previews, taskbar and start menu (how this renders is theme-dependent)
	SetWindowCompositionAttribute = (SetWindowCompositionAttributeAPI)GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute");
	ChangeImportedAddress(GetModuleHandle(NULL), "user32.dll", SetWindowCompositionAttribute, SetWindowCompositionAttributeNEW);
	ChangeImportedAddress(GetModuleHandle(NULL), "dwmapi.dll", DwmEnableBlurBehindWindow, DwmEnableBlurBehindWindowNEW);
	ChangeImportedAddress(GetModuleHandle(NULL), "user32.dll", SetWindowRgn, SetWindowRgnNEW);
	//load functions needed for task enum hook
	HMODULE user32 = LoadLibrary(L"user32.dll");
	IsShellFrameWindow = (IsShellWindow_t)GetProcAddress(user32, (LPCSTR)2573);
	//IsShellManagedWindow = (IsShellWindow_t)GetProcAddress(user32, (LPCSTR)2574);
	GhostWindowFromHungWindow = (GhostWindowFromHungWindow_t)GetProcAddress(user32, "GhostWindowFromHungWindow");
	//perform the actual hook
	ChangeImportedAddress(GetModuleHandle(NULL), "user32.dll", IsWindowVisible, IsWindowVisibleNEW);
	//change show desktop btn
	ChangeImportedAddress(GetModuleHandle(NULL), "uxtheme.dll", SetWindowTheme, SetWindowThemeNEW);
	//overflow positioning if user is using TH1 or higher
	if (g_osVersion.BuildNumber() >= 10240)
		ChangeImportedAddress(GetModuleHandle(NULL), "user32.dll", GetProcAddress(GetModuleHandle(L"user32.dll"), (LPSTR)"CalculatePopupWindowPosition"), CalculatePopupWindowPositionNEW);
	//fix classic start menu icon (pls fix)
	/*HMODULE winbrand = LoadLibrary(L"winbrand.dll");
	BrandingLoadImage = (BrandingLoadImage_t)GetProcAddress(winbrand, "BrandingLoadImage");
	if (BrandingLoadImage)
		ChangeImportedAddress(GetModuleHandle(NULL),"winbrand.dll",BrandingLoadImage,BrandingLoadImageNEW);*/
		//shell32 - hack created startmenupin instance		
	StartMenuPin_PatchShell32();
	//shell32 - patch delayload shit
	HookShell32();
	ShowWin32Menus(); //Remove immersive menus so taskbar behaves properly
	FixAuthUI();

	// query disable comp value
	DWORD dwDisableComposition = 0;
	g_registry.QueryValue(L"DisableComposition", (LPBYTE)&dwDisableComposition, sizeof(DWORD));
	g_bDisableComposition = (dwDisableComposition != 0);

	// query classic theme
	DWORD dwClassicTheme = 0;
	g_registry.QueryValue(L"ClassicTheme", (LPBYTE)&dwClassicTheme, sizeof(DWORD));
	if (dwClassicTheme != 0)
	{
		dbgprintf(L"setting classic theme");
		g_bDisableComposition = true; // classic theme never had comp, duh
		g_bClassicTheme = true;
		SetThemeAppProperties(NULL);
	}

	HookLoadImageForSizeAndFont();

	//enable hooks at end
	MH_EnableHook(MH_ALL_HOOKS);
}


BOOL WINAPI GetUserObjectInformationNew(HANDLE hObj, int nIndex, PVOID pvInfo, DWORD nLength, LPDWORD lpnLengthNeeded)
{
	lstrcpy(LPWSTR(pvInfo), L"Winlogon");
	return TRUE;
}

BOOL WINAPI GetWindowBandNew(HWND hwnd, DWORD* out)
{
	BOOL ret = GetWindowBandOrig(hwnd, out);
	DWORD origband = (DWORD)GetProp(GetAncestor(hwnd, GA_ROOTOWNER), L"explorer7.WindowBand");
	//dbgprintf(L"GetWindowBand %p %p %p",hwnd,*out,origband);
	if (origband && out) *out = origband;
	return ret;
}

UINT_PTR WINAPI SetTimer_WUI(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
	if (nIDEvent == 0x2252CE37)
		ShowWindow(hWnd, SW_HIDE);
	return SetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc);
}


void HookImmersive()
{
	HMODULE immersiveui = LoadLibrary(L"Windows.UI.Immersive.dll");
	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	CreateWindowInBandOrig = (CreateWindowInBandAPI)GetProcAddress(hUser32, "CreateWindowInBand");

	if (g_enableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074)
		CreateWindowInBandExOrig = (CreateWindowInBandExAPI)GetProcAddress(hUser32, "CreateWindowInBand");

	GetWindowBandOrig = (GetWindowBandAPI)GetProcAddress(hUser32, "GetWindowBand");
	ChangeImportedAddress(immersiveui, "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);
	ChangeImportedAddress(immersiveui, "user32.dll", GetWindowBandOrig, GetWindowBandNew);
	ChangeImportedAddress(immersiveui, "user32.dll", GetUserObjectInformation, GetUserObjectInformationNew);
	ChangeImportedAddress(immersiveui, "user32.dll", SetTimer, SetTimer_WUI);

	if (!g_enableImmersiveShellStack || g_osVersion.BuildNumber() < 10074) // Ittr: If user *either* has UWP disabled, or they are NOT on Windows 10, run legacy window band code
	{
		//bugbug!!!
		ChangeImportedAddress(GetModuleHandle(L"twinui.dll"), "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);
		ChangeImportedAddress(GetModuleHandle(L"authui.dll"), "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);
		ChangeImportedAddress(GetModuleHandle(L"shell32.dll"), "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);

		ChangeImportedAddress(GetModuleHandle(L"twinapi.dll"), "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);
		ChangeImportedAddress(GetModuleHandle(L"Windows.UI.dll"), "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);
	}
}

FARPROC
WINAPI
GetProcAddress_Hook(
	HMODULE hModule,
	LPCSTR lpProcName
)
{
	//dbgprintf(L"GetProcAddress Hook\n");
	return GetProcAddress(hModule, lpProcName);
}

//TODO: Migrate to use ChangeImportedPattern or equivalent when said function is finalised. Not migrated yet due to importance of shunimpl
//Basically this allows explorer to actually work on builds >9200
void AssFuckShunimpl()
{
	uintptr_t shunImpl = (uintptr_t)GetModuleHandle(L"shunimpl.dll");
	if (!shunImpl) return;
	char* dllmainSHUNIMPL = (char*)FindPattern(shunImpl, "48 83 EC 28 83 FA 01");

	if (dllmainSHUNIMPL)
	{
		unsigned char bytes[] = { 0xB0,0x01,0xC3 };
		ChangeImportedPattern(dllmainSHUNIMPL, bytes, sizeof(bytes));
	}

}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	CheckTimeBomb();

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		AssFuckShunimpl();

		themeHandles = new wiktorArray<HTHEME>();
		themeHandles->data = 0;
		themeHandles->size = 0;

		dbgprintf(L"Dll Attach\n");
		g_hInstance = hModule;
		if (GetModuleHandle(L"DisplaySwitch.exe"))
		{
			dbgprintf(L"loaded into displayswitch %p %s!", GetCurrentProcessId(), GetCommandLine());
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
			CreateWindowInBandOrig = (CreateWindowInBandAPI)GetProcAddress(GetModuleHandle(L"user32.dll"), "CreateWindowInBand");
			ChangeImportedAddress(GetModuleHandle(L"alttab.dll"), "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);
			g_alttabhooked = TRUE;
		}
	}
	break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		realloc(themeHandles->data, 0);
		themeHandles->size = 0;
		delete themeHandles;

		break;
	}
	return TRUE;
}

extern "C" HRESULT WINAPI Explorer_CoCreateInstance(
	__in   REFCLSID rclsid,
	__in   LPUNKNOWN pUnkOuter,
	__in   DWORD dwClsContext,
	__in   REFIID riid,
	__out  LPVOID* ppv
)
{
	HRESULT result;
	result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

	if (rclsid == CLSID_PersonalStartMenu && riid == IID_IShellItemFilter && result != S_OK && g_osVersion.BuildNumber() >= 10074) //Ittr: as far as im aware doesnt cause crashing on 1507/11. needs further checking when im awake
	{
		auto shellItemFilter = new CStartMenuItemFilter();
		result = shellItemFilter->QueryInterface(riid, ppv);
	}

	if (rclsid == CLSID_SysTray) //create Metro before tray
	{
		dbgprintf(L"create Metro before tray\n");
		HookImmersive();

		if (g_enableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // Ittr: Only create TWinUI UWP mode here if we are going to use it
			CreateTwinUI_UWP();

	}
	if (rclsid == CLSID_RegTreeOptions && riid == IID_IRegTreeOptions7) //upgrading RegTreeOptions interface
	{

		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_IRegTreeOptions8, ppv);
		*ppv = new CRegTreeOptionsWrapper((IRegTreeOptions8*)*ppv);
	}

	if (riid == IID_IAuthUILogonSound7 && result != S_OK)
	{
		dbgprintf(L"Wrap authuilogonsound7\n");
		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_IAuthUILogonSound10, ppv);
	}

	if (rclsid == CLSID_UserAssist && result != S_OK)
	{
		if (riid == IID_IUserAssist7)
			result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_IUserAssist10, ppv);
		else if (riid == IID_IUserAssist72)
			result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_IUserAssist102, ppv);
		else
		{
			dbgprintf(L"Warning, unknown useraassist riid!!!!!");
			dbgprintf(L"Warning, unknown useraassist riid!!!!!");
		}
	}

	if (rclsid == CLSID_StartMenuCacheAndAppResolver && result != S_OK)
	{
		if (riid == IID_IAppResolver7)
		{
			//dbgprintf(L"Explorer_CoCreateInstance: Resolver7 using iappresolver8\n");
			PVOID rslvr8 = NULL;
			CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_IAppResolver8, &rslvr8);
			//create our object

			CStartMenuResolver* resolver7 = new CStartMenuResolver((IAppResolver8*)rslvr8);
			result = resolver7->QueryInterface(riid, ppv);
			//if (result == S_OK)
				//dbgprintf(L"Explorer_CoCreateInstance: Resolver7 using iappresolver8 IS OK!!\n");
		}
		else if (riid == IID_IStartMenuItemsCache7)
		{
			int build = g_osVersion.BuildNumber();
			IID iid = IID_IStartMenuItemsCache8;
			if (build >= 14393)
				iid = IID_IStartMenuItemsCache10;

			void* newcache = nullptr;
			CoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, &newcache);

			CStartMenuResolver* resolver7 = nullptr;
			if (build >= 14393)
				resolver7 = new CStartMenuResolver((IStartMenuItemsCache10*)newcache);
			else
				resolver7 = new CStartMenuResolver((IStartMenuItemsCache8*)newcache);

			result = resolver7->QueryInterface(riid, ppv);
			if (result == S_OK)
				dbgprintf(L"Explorer_CoCreateInstance: Cache7 using IStartMenuItemsCache8/10 is OK!!\n");
		}
	}
	if ((rclsid == CLSID_StartMenuPin || rclsid == CLSID_TaskbarPin) /* && riid == IID_IPinnedList2*/ && result != S_OK)
	{
		int build = g_osVersion.BuildNumber();
		IID id = IID_IPinnedList25;

		if (build >= 14393 && build < 17763)
			id = IID_IFlexibleTaskbarPinnedList;
		else if (build >= 17763)
			id = IID_IPinnedList3;

		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, id, ppv);
		*ppv = new CPinnedListWrapper((IUnknown*)*ppv, build);
	}

	if (riid == IID_AutoDestList && result != S_OK)
	{
		dbgprintf(L"USE 10 AUTODESTLIST!!!!\n");
		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_AutoDestList10, ppv);
		*ppv = new CAutoDestWrapper((IAutoDestinationList10*)*ppv);
	}
	if (riid == IID_CustomDestList && result != S_OK)
	{
		dbgprintf(L"CUSTOMDESTLIST!!!!\n");
		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_CustomDestList10, ppv);
		if (result != S_OK || !*ppv)
		{
			result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_CustomDestList1507, ppv);
			*ppv = new CCustomDestWrapper((IInternalCustomDestList1507*)*ppv);
		}
		else
			*ppv = new CCustomDestWrapper((IInternalCustomDestList10*)*ppv);
	}
	if (riid == IID_IShellTaskScheduler7)
	{
		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
		dbgprintf(L"wrap IID_IShellTaskScheduler7\n");
		*ppv = new CShellTaskSchedulerWrapper((IShellTaskScheduler7*)*ppv);
	}
	if (result == S_OK && rclsid == CLSID_SysTray) //wrap stobject
	{
		dbgprintf(L"wrap stobject\n");
		*ppv = new CSysTrayWrapper((IOleCommandTarget*)*ppv);
	}
	if (rclsid == CLSID_AuthUIShutdownChoices && result != S_OK) //wrap authui
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
	if (riid == IID_TrayClock7 && result != S_OK)
		result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, IID_TrayClock8, ppv);

	return result;
}

extern "C" HRESULT WINAPI Explorer_CoRegisterClassObject(
	REFCLSID rclsid,     //Class identifier (CLSID) to be registered
	IUnknown* pUnk,     //Pointer to the class object
	DWORD dwClsContext,  //Context for running executable code
	DWORD flags,         //How to connect to the class object
	LPDWORD  lpdwRegister
)
{
	if (rclsid == CLSID_TrayNotify)
	{
		pUnk = new CTrayNotifyFactory((IClassFactory*)pUnk);
		if (g_osVersion.BuildNumber() < 10240) // Ittr: temporarily gate fakeimmersive to 8.1 due to functional issues (e.g. hanging) with 10 - TODO re-enable for non
		{
			//register immersive shell fake too
			RegisterFakeImmersive();
			//and projection
			RegisterProjection();
		}
	}

	HRESULT rslt = CoRegisterClassObject(rclsid, pUnk, dwClsContext, flags, lpdwRegister);

	if (rclsid == CLSID_TrayNotify)
		dwRegisterNotify = *lpdwRegister;

	return rslt;
}

extern "C" HRESULT WINAPI Explorer_CoRevokeClassObject(DWORD dwRegister)
{
	if (dwRegister == dwRegisterNotify)
	{
		if (g_osVersion.BuildNumber() < 10240) // Ittr: temporarily gate fakeimmersive to 8.1 due to functional issues (e.g. hanging) with 10
		{
			UnregisterFakeImmersive();
			UnregisterProjection();
		}
	}
	return CoRevokeClassObject(dwRegister);
}
