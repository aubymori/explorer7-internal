#define INITGUID
#define PRERELEASE_COPY
#include "util.h"
#include "common.h"
#include "forwards.h"
#include "StartMenuResolver.h"
#include "TrayObject.h"
#include "dbgprint.h"
#include "ImmersiveShell.h"
#include "TrayNotify.h"
#include "AuthUI.h"
#include "StartMenuPin.h"
#include "ImmersiveFactory.h"
#include "ProjectionFactory.h"
#include "OSVersion.h"
#include "PinnedList.h"
#include "DestinationList.h"
#include "resource.h"
#include "ThemeManager.h"
#include "MinHook.h"
#include "ShellTaskScheduler.h"
#include "RegistryManager.h"
#include "NscTree.h"
#include "RegTreeOptions.h"
#include "shellapi.h"
#include "AutoPlay.h"
#include "StartMenuItemFilter.h"
#include "shell32_wrappers.h"
#include "ShellURL.h"
#include "OptionConfig.h"
#include "AddressImports.h"
#include "PatternImports.h"
#include "TypeDefinitions.h"

LRESULT CALLBACK NewTrayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // Ittr: for TH1+
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

	if (uMsg == WM_THEMECHANGED)
	{
		EnsureWindowColorization(); // Ittr: Correct colorization enablement setting for Win10/11
	}

	if (uMsg == WM_SETTINGCHANGE || uMsg == WM_ERASEBKGND || uMsg == WM_WININICHANGE) // Ittr: Fix taskbar colorization for non-legacy
	{
		if ((IsThemeActive() && !s_ClassicTheme && IsCompositionActive() && !s_DisableComposition) && hwnd == GetTaskbarWnd() && s_ColorizationOptions != 0) // Ittr: Only taskbar needs updating now, start menu and new thumbnail algo correct for themselves
		{
			SetWindowCompositionAttribute(hwnd, &GetTrayAccentProperties(false));
		}
	}

	return CallWindowProc(g_prevTrayProc, hwnd, uMsg, wParam, lParam);
}

// Ittr: Awful hack but it seems to fix it
LRESULT CALLBACK NewThumbnailProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETTINGCHANGE || uMsg == WM_ERASEBKGND || uMsg == WM_WININICHANGE) // Ittr: Fix thumbnail colorization for non-legacy
	{
		if ((IsThemeActive() && !s_ClassicTheme && IsCompositionActive() && !s_DisableComposition) && (g_osVersion.BuildNumber() >= 10074 && hwnd == GetThumbnailWnd()) && s_ColorizationOptions != 0) // Ittr: Only taskbar needs updating now, start menu and new thumbnail algo correct for themselves
		{
			SetWindowCompositionAttribute(hwnd, &GetTrayAccentProperties(true));
		}
	}

	return CallWindowProc(g_prevThumbnailProc, hwnd, uMsg, wParam, lParam);
}

void ShimDesktop8()
{
	static int InitOnce = FALSE;
	if (InitOnce) return;
	hwnd_desktop = FindWindow(L"Progman", L"Program Manager");
	HWND hwndTray = GetTaskbarWnd();
	HWND hwndThumbnail = GetThumbnailWnd();
	if (!hwnd_desktop || !hwndTray || !hwndThumbnail) return;
	InitOnce = TRUE;
	//hook tray
	g_prevTrayProc = (WNDPROC)GetWindowLongPtr(hwndTray, GWLP_WNDPROC);
	g_prevThumbnailProc = (WNDPROC)GetWindowLongPtr(hwndThumbnail, GWLP_WNDPROC);
	SetWindowLongPtr(hwndTray, GWLP_WNDPROC, (LONG_PTR)NewTrayProc);
	SetWindowLongPtr(hwndThumbnail, GWLP_WNDPROC, (LONG_PTR)NewThumbnailProc);
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

void RenderThumbnail(PVOID This, int animoffset, int bNoRedraw)
{	
	RECT rc = *(RECT*)((PBYTE)This + 0x68);
	HWND hwnd = *(HWND*)((PBYTE)This + 0x60);
	HTHEME hthem = *(HTHEME*)((PBYTE)This + 0x98);

	renderThumbnail_orig(This, animoffset, bNoRedraw);
	
	MARGINS mar;
	GetThemeMargins(hthem, NULL, 2, 0, TMT_CONTENTMARGINS, NULL, &mar);
	rc.left += mar.cxLeftWidth;
	rc.right -= mar.cxRightWidth;
	rc.top += mar.cyTopHeight;
	rc.bottom -= mar.cyBottomHeight;
	DwmpUpdateAccentBlurRect(hwnd, &rc);
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

PVOID CtaskBandPtr = 0;

VOID SetWindowIcon(PVOID This, HWND a2, HICON a3, int a4)
{
	CtaskBandPtr = This;
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
	return !IsThemeActive() || s_ClassicTheme;
}

bool AllowThemes(void)
{
	return !IsClassicTheme();
}

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
		"48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 81 EC 00 03 00 00 48 8B"
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
	if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // UWP enabled
	{
		DWORD p0 = (DWORD)_ReturnAddress();
		dwExStyle = dwExStyle | WS_EX_TOOLWINDOW; // TODO is this needed
		HWND ret = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hwndParent, hMenu, hInstance, lpParam);

		// Ittr: We do this to eliminate the ghost window. The power of trans rights compelled me to fix this :3
		BOOL shouldCloak = true;
		WCHAR titleBuffer[MAX_PATH];
		GetClassName(ret, titleBuffer, sizeof(titleBuffer));
		WCHAR afwTitle[23] = L"ApplicationFrameWindow";
		if (strcmp((char*)titleBuffer, (char*)afwTitle) == 0)
			DwmSetWindowAttribute(ret, DWMWA_CLOAK, &shouldCloak, sizeof(shouldCloak));

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

	// Ittr: We do this to eliminate the ghost window. The power of trans rights compelled me to fix this :3
	BOOL shouldCloak = true;
	WCHAR titleBuffer[MAX_PATH];
	GetClassName(ret, titleBuffer, sizeof(titleBuffer));
	WCHAR afwTitle[23] = L"ApplicationFrameWindow";
	if (strcmp((char*)titleBuffer, (char*)afwTitle) == 0)
		DwmSetWindowAttribute(ret, DWMWA_CLOAK, &shouldCloak, sizeof(shouldCloak));

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

BOOL WINAPI RegisterWindowHotkeyNew(HWND hwnd, int id, UINT mod, UINT vk)
{
	BOOL res = RegisterHotKeyApiOrg(hwnd, id, mod, vk);

	if (!res)
	{
		return TRUE;
	}

	return TRUE;
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
		return LoadImageW(hInst, name, type, cx, cy, fuLoad);
	else
		return LoadImageW(NULL, szOrbPath, IMAGE_BITMAP, 0, 0, fuLoad | LR_LOADFROMFILE);
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

void ModifyDesktopHwnd()
{
	uintptr_t desktopHwnd = FindPattern((uintptr_t)GetModuleHandle(0), "74 ?? 48 3B 3D ?? ?? ?? ?? 8D 43 01 0F 45 D8");
	if (desktopHwnd)
	{
		desktopHwnd += 2;
		v_hwndDesktop = (HWND*)(desktopHwnd + 7 + *reinterpret_cast<signed int*>(desktopHwnd + 3));
	}
}

enum BlockHotKeyRegistrationFlags : __int32
{
	BHKRF_None = 0x0,
	BHKRF_Always = 0x1,
	BHKRF_PpiEdition = 0x2,
	BHKRF_AssignedAccessMultiAppMode = 0x4,
	BHKRF_ShellLauncher = 0x8,
};

const struct IMMERSIVE_WINDOW_MESSAGE_SERVICE_HOTKEY_REGISTRATION
{
	BlockHotKeyRegistrationFlags blockFlags;
	int id;
	unsigned int fsModifiers;
	unsigned int vk;
};

// experimental hotkey fix 1 - broke uwp
HRESULT(__fastcall* CImmersiveWindowMessageService__RequestHotkeys)(void* a1, unsigned int a2, IMMERSIVE_WINDOW_MESSAGE_SERVICE_HOTKEY_REGISTRATION* a3, void* a4, unsigned int* a5);
HRESULT CImmersiveWindowMessageService__RequestHotkeys_Hook(void* a1, unsigned int a2, IMMERSIVE_WINDOW_MESSAGE_SERVICE_HOTKEY_REGISTRATION* a3, void* a4, unsigned int* a5)
{
	dbgprintf(L"CImmersiveWindowMessageService__RequestHotkeys");
	if (a3->vk == VK_LWIN || a3->vk == VK_RWIN || (a3->vk == VK_ESCAPE && a3->fsModifiers & VK_CONTROL)) // fix win key
	{
		dbgprintf(L"FIXING WINDOWS KEY");
		return S_OK;
	}

	return CImmersiveWindowMessageService__RequestHotkeys(a1,a2,a3,a4,a5);
}

// experimental hotkey fix 2 - doesn't work at present
UINT shellHook = 0;

UINT(WINAPI* fRegisterWindowMessageW)(LPCWSTR lpString);
UINT WINAPI RegisterWindowMessageWNEW(LPCWSTR lpString)
{
	dbgprintf(L"RegisterWindowMessageWNEW %s",lpString);
	if (wcscmp(L"SHELLHOOK", lpString) == 0)
	{
		dbgprintf(L"RegisterWindowMessageWNEW REDIRD");

		if (shellHook != 0)
			return shellHook;

		shellHook = fRegisterWindowMessageW(L"SHELLHOOK");
		return shellHook;
	}
	return fRegisterWindowMessageW(lpString);
}

// experimental hotkey fix 3 - buggy results but *does* appear to work
HRESULT(__fastcall* CTaskBand_HandleShellHook)(PVOID ctaskband, int id, HWND a3);

HRESULT(__fastcall* OnShellHookMessage)(void* a1);
HRESULT OnShellHookMessage_Hook(void* a1) //gets called when start menu is to be opened - a bit temperamental
{
	// key to note: at the moment, we can either do this for bugged start menu behaviour, or we can return S_OK and have no menu on the hotkey at all.
	// neither is ideal, but we can probably ship m2 like this and fix properly later

	if (CtaskBandPtr)
		return CTaskBand_HandleShellHook(CtaskBandPtr,7,0);

	return OnShellHookMessage(a1);
}

// Ittr: New method for removing immersive menus. Better inter-operability between Windows versions. Used alongside existing method.
BOOL SystemParametersInfoWNEW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	if (uiAction == SPI_GETSCREENREADER)
	{
		*(BOOL*)pvParam = TRUE;
		return TRUE;
	}

	return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
}

//Ittr: Goodbye immersive context menus and good riddance. For Win10 TH1+. 
//Also to be noted that Windows 11 makes further changes here that we'll need to account for in future if we do officially support it.
void ShowWin32Menus()
{
	if (g_osVersion.BuildNumber() >= 10074) // if user is using TH1 or later
	{
		char* CAODTM_SH32; // ImmersiveContextMenuHelper::CanApplyOwnerDrawToMenu
		char* CAODTM_EF; // same function, in ExplorerFrame.dll
		char unsigned bytes[] = { 0xC3 }; // retn

		if (g_osVersion.BuildNumber() >= 26100) // W11 Germanium onwards
		{
			CAODTM_SH32 = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 33 DB 48 8B F2 33 FF 48 8B E9";
			CAODTM_EF = CAODTM_SH32;
		}
		else if (g_osVersion.BuildNumber() >= 21996) // W11 Cobalt to W11 Nickel
		{
			// This is somewhat flawed on Cobalt, but it will have to do
			CAODTM_SH32 = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B E9 33 FF 33 D2";
			CAODTM_EF = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B E9 33 FF 33 D2 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? C7 44 24 20 50 00 00 00";
		}
		else // TH1 to VB
		{
			CAODTM_SH32 = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 70 48 8B F2 48 8B";
			CAODTM_EF = CAODTM_SH32;
		}

		ChangeImportedPattern((char*)FindPattern((uintptr_t)GetModuleHandle(L"shell32.dll"), CAODTM_SH32), bytes, sizeof(bytes)); // shell32.dll
		ChangeImportedPattern((char*)FindPattern((uintptr_t)LoadLibrary(L"ExplorerFrame.dll"), CAODTM_EF), bytes, sizeof(bytes)); // ExplorerFrame.dll

		// Ensure as much as we can that it's gone, if the above isn't enough (Win11 Cobalt, I'm looking at you...)
		// Only applied to shell32, as application to ExplorerFrame breaks the program list hover behaviour.
		ChangeImportedAddress(GetModuleHandle(L"shell32.dll"), "user32.dll", SystemParametersInfoW, SystemParametersInfoWNEW);
	}
}

void CPniMainDlg_ShowFlyoutNEW() // don't bother with the parameters as we aren't going to use them
{
	// Open Network and Sharing Center instead inside the Windows Control Panel, as a non-immersive alternative
	ShellExecuteW(nullptr, nullptr, L"control.exe", L"/name Microsoft.NetworkAndSharingCenter", nullptr, SW_SHOWNORMAL);

	// End function as we aren't going to do anything else here
	return;
}

void HandleNonImmersivePniDui()
{
	if (g_osVersion.BuildNumber() >= 10074) // not needed for 8.1
	{
		// Unable to do with patterns alone, as Microsoft removed HrOpenControlPanel
		if (!s_UseDCompFlyouts || !s_EnableImmersiveShellStack)
		{
			HMODULE pnidui = LoadLibrary(L"pnidui.dll");

			if (pnidui) // only run if DLL is present - handled like this because GE removes pnidui...
			{
				void* _ShowFlyout = (void*)FindPattern((uintptr_t)LoadLibrary(L"pnidui.dll"), "48 89 6C 24 18 56 57 41 57 48 83 EC 60");

				if (_ShowFlyout) // first run, VB to NI
				{
					MH_CreateHook(static_cast<LPVOID>(_ShowFlyout), CPniMainDlg_ShowFlyoutNEW, reinterpret_cast<LPVOID*>(&CPniMainDlg_ShowFlyout));
				}
				else
				{
					_ShowFlyout = (void*)FindPattern((uintptr_t)LoadLibrary(L"pnidui.dll"), "48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 40 8A");

					if (_ShowFlyout) // second run, RS4 to TI
					{
						MH_CreateHook(static_cast<LPVOID>(_ShowFlyout), CPniMainDlg_ShowFlyoutNEW, reinterpret_cast<LPVOID*>(&CPniMainDlg_ShowFlyout));
					}
					else
					{
						_ShowFlyout = (void*)FindPattern((uintptr_t)LoadLibrary(L"pnidui.dll"), "48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 20 40 8A FA");

						if (_ShowFlyout) // third run, TH2 to RS3
						{
							MH_CreateHook(static_cast<LPVOID>(_ShowFlyout), CPniMainDlg_ShowFlyoutNEW, reinterpret_cast<LPVOID*>(&CPniMainDlg_ShowFlyout));
						}
						else
						{
							_ShowFlyout = (void*)FindPattern((uintptr_t)LoadLibrary(L"pnidui.dll"), "48 8B C4 56 57 41 56 48 81 EC 80 01 00 00");

							if (_ShowFlyout) // fourth run, TH1
							{
								MH_CreateHook(static_cast<LPVOID>(_ShowFlyout), CPniMainDlg_ShowFlyoutNEW, reinterpret_cast<LPVOID*>(&CPniMainDlg_ShowFlyout));
							}
						}
					}
				}
			}
		}
	}
}

void HookShell32();
void HookAPIs()
{
	// 24H2+ - W32PTP
	if (g_osVersion.BuildNumber() >= 26100)
	{
		HMODULE twinui_pcshell = LoadLibrary(L"twinui.pcshell.dll");

		if (twinui_pcshell)
		{
			CTaskbandPin_CreateInstance = (CTaskbandPin_CreateInstance_t)FindPattern((uintptr_t)twinui_pcshell, "40 53 48 83 EC 20 48 8B D9 48 8D 15 ?? ?? ?? ?? B9 80 00 00 00 E8 ?? ?? ?? ?? 48 85 C0");
		}
	}

	// Change and fix core desktop components
	hEvent_DesktopVisible = CreateEvent(NULL, TRUE, FALSE, L"ShellDesktopVisibleEvent");
	SHCreateDesktopOrig = (SHCreateDesktopAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPSTR)200);
	ChangeImportedAddress(GetModuleHandle(NULL), "shell32.dll", SHCreateDesktopOrig, SHCreateDesktopNEW);
	SHDesktopMessageLoop = (SHCreateDesktopAPI)GetProcAddress(GetModuleHandle(L"shell32.dll"), (LPSTR)201);
	ChangeImportedAddress(GetModuleHandle(NULL), "shell32.dll", SHDesktopMessageLoop, SHDesktopMessageLoopNEW);

	//ChangeImportedAddress(GetModuleHandle(NULL), "user32.dll", RegisterWindowMessageW, RegisterWindowMessageWNEW);

	// Initialize the theme manager and declare the types for the UXTheme apis we're hooking
	ThemeManagerInitialize();
	fOpenThemeData = decltype(fOpenThemeData)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeData"));
	fOpenThemeDataForDpi = decltype(fOpenThemeDataForDpi)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataForDpi"));
	fOpenThemeDataEx = decltype(fOpenThemeDataEx)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataEx"));

	// ???
	ModifyDesktopHwnd();

	// We initialize the MinHook system here
	MH_Initialize();

	//CImmersiveWindowMessageService__RequestHotkeys = (decltype(CImmersiveWindowMessageService__RequestHotkeys))FindPattern((uintptr_t)LoadLibrary(L"twinui.dll"), "4C 8B DC 4D 89 43 ?? 57 41 54 41 55 41 56 41 57 48 83 EC");
	//MH_CreateHook(static_cast<LPVOID>(CImmersiveWindowMessageService__RequestHotkeys), CImmersiveWindowMessageService__RequestHotkeys_Hook, reinterpret_cast<LPVOID*>(&CImmersiveWindowMessageService__RequestHotkeys));

	fRegisterWindowMessageW = (decltype(fRegisterWindowMessageW))GetProcAddress(LoadLibraryW(L"user32.dll"),"RegisterWindowMessageW");

	// disabled - <1607 doesnt like atm + unfinished. sorry! uncomment if you're testing
	//CTaskBand_HandleShellHook = (decltype(CTaskBand_HandleShellHook))FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 55 56 57 41 54 41 55 48 83 EC ?? 83 FA 07");
	//OnShellHookMessage = (decltype(OnShellHookMessage))FindPattern((uintptr_t)LoadLibraryW(L"twinui.pcshell.dll"), "40 53 48 83 EC 20 48 8B D9 48 8B 89 ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 01 48 8B 40 ?? FF 15 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 38 83");

	// Hook UXTheme-related calls for the purpose of our inactive theme system.
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeData), OpenThemeData_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeData));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataForDpi), OpenThemeDataForDpi_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataForDpi));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataEx), OpenThemeDataEx_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataEx));
	// disabled - <1607 doesnt like atm + unfinished. sorry! uncomment if you're testing
	//MH_CreateHook(static_cast<LPVOID>(OnShellHookMessage), OnShellHookMessage_Hook, reinterpret_cast<LPVOID*>(&OnShellHookMessage));

	// Hook and update definitions of what windows should be added to the tray - largely for UWP purposes, but essentially zero-cost so included on both immersive on and off modes.
	void* _ShouldAddWindowToTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB");
	void* _IsWindowNotDesktopOrTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB FF 15 ?? ?? ?? ?? 3B C3 74 ?? 48 3B 3D");
	MH_CreateHook(static_cast<LPVOID>(_ShouldAddWindowToTray), ShouldAddWindowToTray, reinterpret_cast<LPVOID*>(&_ShouldAddWindowToTray));
	MH_CreateHook(static_cast<LPVOID>(_IsWindowNotDesktopOrTray), IsWindowNotDesktopOrTray, reinterpret_cast<LPVOID*>(&_IsWindowNotDesktopOrTray));
	
	// thumbnail fix
	if (g_osVersion.BuildNumber() >= 10074) // we don't apply to 8.1 as only pseudo-aero is supported there
	{
		void* _thumbnailrender = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 20 44 89 40 18 57 41 54 41 55 41 56 41 57 48 81 EC 90 00 00 00 48 8B F9");
		MH_CreateHook(static_cast<LPVOID>(_thumbnailrender), RenderThumbnail, reinterpret_cast<LPVOID*>(&renderThumbnail_orig));
	}

	HandleNonImmersivePniDui();

	if (s_ShowStoreAppsOnTaskbar && g_osVersion.BuildNumber() >= 10074)
	{
		void* _ctaskbandadd = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "FF F3 55 56 57 41 54 41 55 41 56 41 57 48 81 EC F8 06 00 00");
		void* _cthumbnailUpdate = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 48 8B 81 B0 00 00 00");
		SetIconThumb = (setIconThumb_t)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 49 63 D8 4C 8B 81 B0 00 00 00");

		MH_CreateHook(static_cast<LPVOID>(_ctaskbandadd), SetWindowIcon, reinterpret_cast<LPVOID*>(&SetIcon));
		MH_CreateHook(static_cast<LPVOID>(_cthumbnailUpdate), UpdateItemIcon, reinterpret_cast<LPVOID*>(&UpdateItem));
	}


	// 1. Todo in future *after* feature-set is complete: see how many of these hooks can be ChangeImportedAddress instead of MH_CreateHook (perf optimisation)
	// 2. Code stack used exclusively for UWP mode, hence the conditional statement.
	if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // Ittr: Run these hooks only if the user A) is on Windows 10 and B) has UWP enabled
	{
		// 1. This will *need* serious optimization in the near future as it singlehandedly delays program enumeration and startup by several seconds
		// 2. Prepare the taskbar and thumbnails to handle UWP icons. Further work needed for jumplists and to prevent wrongful classification as "Application Frame Host" in the first place.

		// The rest of this code block is dedicated to ensuring UWP actually runs in the first place
		CreateWindowInBandOrig = decltype(CreateWindowInBandOrig)(GetProcAddress(GetModuleHandle(L"user32.dll"), "CreateWindowInBand"));
		CreateWindowInBandExOrig = decltype(CreateWindowInBandExOrig)(GetProcAddress(GetModuleHandle(L"user32.dll"), "CreateWindowInBandEx"));
		SetWindowBandApiOrg = decltype(SetWindowBandApiOrg)(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowBand"));
		RegisterHotKeyApiOrg = decltype(RegisterHotKeyApiOrg)(GetProcAddress(GetModuleHandle(L"user32.dll"), "RegisterHotKey"));

		MH_CreateHook(static_cast<LPVOID>(CreateWindowInBandOrig), CreateWindowInBandNew, reinterpret_cast<LPVOID*>(&CreateWindowInBandOrig));
		MH_CreateHook(static_cast<LPVOID>(CreateWindowInBandExOrig), CreateWindowInBandExNew, reinterpret_cast<LPVOID*>(&CreateWindowInBandExOrig));
		MH_CreateHook(static_cast<LPVOID>(SetWindowBandApiOrg), SetWindowBandNew, reinterpret_cast<LPVOID*>(&SetWindowBandApiOrg));
		//MH_CreateHook(static_cast<LPVOID>(RegisterHotKeyApiOrg), RegisterWindowHotkeyNew, reinterpret_cast<LPVOID*>(&RegisterHotKeyApiOrg));

		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2581), RetTrue, NULL); // GetWindowTrackInfoAsync
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2563), RetTrue, NULL); // ClearForeground
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2628), RetTrue, NULL); // CreateWindowGroup
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2629), RetTrue, NULL); // DeleteWindowGroup
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2631), RetTrue, NULL); // EnableWindowGroupPolicy
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2627), RetTrue, NULL); // SetBridgeWindowChild
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2511), RetTrue, NULL); // SetFallbackForeground
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2566), RetTrue, NULL); // SetWindowArrangement
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2632), RetTrue, NULL); // SetWindowGroup
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2579), RetTrue, NULL); // SetWindowShowState
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2585), RetTrue, NULL); // UpdateWindowTrackingInfo
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2514), RetTrue, NULL); // RegisterEdgy
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2542), RetTrue, NULL); // RegisterShellPTPListener
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2537), RetTrue, NULL); // SendEventMessage
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2513), RetTrue, NULL); // SetActiveProcessForMonitor
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2564), RetTrue, NULL); // RegisterWindowArrangementCallout
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), (LPCSTR)2567), RetTrue, NULL); // EnableShellWindowManagementBehavior
		MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), "AllowSetForegroundWindow"), RetTrue, NULL);
	}

	// If we are on Windows 10 or higher, query the original program list pattern and create our hook to fix the visual issues
	if (g_osVersion.BuildNumber() >= 10240)
	{
		CNSCHost_FillNSCOg = (decltype(CNSCHost_FillNSCOg))FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 18 57 48 83 EC 30 33 DB 48 8B F9 39 99 CC 00 00 00");
		if (CNSCHost_FillNSCOg)
			MH_CreateHook(static_cast<LPVOID>(CNSCHost_FillNSCOg), CNSCHost_FillNSC, reinterpret_cast<LPVOID*>(&CNSCHost_FillNSCOg)); //this hook is in nsctree.h now
	}

	// Prevent theme overrides applying to file explorer *VERY IMPORTANT*
	HookTrayThread();

	
	// 1. shell32.dll - hack created startmenupin instance
	// 2. shell32.dll - patch delayload stuff
	StartMenuPin_PatchShell32();
	HookShell32();

	ShowWin32Menus(); // Remove immersive menus so taskbar behaves properly

	//fix classic start menu icon (pls fix)
	/*HMODULE winbrand = LoadLibrary(L"winbrand.dll");
	BrandingLoadImage = (BrandingLoadImage_t)GetProcAddress(winbrand, "BrandingLoadImage");
	if (BrandingLoadImage)
		ChangeImportedAddress(GetModuleHandle(NULL),"winbrand.dll",BrandingLoadImage,BrandingLoadImageNEW);*/

	// Handle custom start orb feature
	HookLoadImageForSizeAndFont();

	// Enable MinHook hooks at the end
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

// Used even when immersive UI is not active in some cases..?
void HookImmersive()
{
	HMODULE immersiveui = LoadLibrary(L"Windows.UI.Immersive.dll");
	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	CreateWindowInBandOrig = (CreateWindowInBandAPI)GetProcAddress(hUser32, "CreateWindowInBand");

	if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074)
		CreateWindowInBandExOrig = (CreateWindowInBandExAPI)GetProcAddress(hUser32, "CreateWindowInBand");

	GetWindowBandOrig = (GetWindowBandAPI)GetProcAddress(hUser32, "GetWindowBand");
	ChangeImportedAddress(immersiveui, "user32.dll", CreateWindowInBandOrig, CreateWindowInBandNew);
	ChangeImportedAddress(immersiveui, "user32.dll", GetWindowBandOrig, GetWindowBandNew);
	ChangeImportedAddress(immersiveui, "user32.dll", GetUserObjectInformation, GetUserObjectInformationNew);
	ChangeImportedAddress(immersiveui, "user32.dll", SetTimer, SetTimer_WUI);

	if (!s_EnableImmersiveShellStack || g_osVersion.BuildNumber() < 10074) // Ittr: If user *either* has UWP disabled, or they are NOT on Windows 10, run legacy window band code
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

// Basically this allows explorer to actually work on builds >9200
void PatchShunimpl()
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

// Where we need to close explorer silently (such as to block people from using awful, horrendous software...)
void ExitExplorerSilently()
{
	// we do these blocks of code like this, so that the 0xc0000142 error doesn't appear
	LPDWORD exitCode;
	GetExitCodeProcess(L"explorer.exe", exitCode);
	ExitProcess((UINT)exitCode); // exit explorer
}

// Initialize the inactive theme engine
void ThemeHandlesInit()
{
	themeHandles = new wiktorArray<HTHEME>();
	themeHandles->data = 0;
	themeHandles->size = 0;
}

// Terminate inactive theme engine when needed
void EndThemeHandles()
{
	realloc(themeHandles->data, 0);
	themeHandles->size = 0;
	delete themeHandles;
}

// WINDOWS 11
void InitPinnedListHack()
{
	// == CPINNEDLIST HACK ==

	HMODULE twinui_pcshell = LoadLibrary(L"twinui.pcshell.dll");

	// CTaskbandPin_CreateInstance
	if (twinui_pcshell)
	{
		// Method 1: Direct function preamble (dangerous; breaks if they modify the fields of CTaskbandPin or its superclass(es))
		// 40 53 48 83 EC 20 48 8B D9 48 8D 15 ?? ?? ?? ?? B9 80 00 00 00 E8 ?? ?? ?? ?? 48 85 C0
		/*matchCTaskbandPinCreateInstance = (PBYTE)FindPattern(
			pFile,
			dwSize,
			"\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8D\x15\x00\x00\x00\x00\xB9\x80\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x85\xC0",
			"xxxxxxxxxxxx????xxxxxx????xxx",
			&numMatchesCTaskbandPinCreateInstance
		);*/

		// Method 2: winrt::Windows::Internal::Shell::implementation::PinManager::IsItemPinned
		// 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 83 64 24 ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 8B 8D ?? ?? ?? ?? 85 C0
		//                                                                   ^^^^^^^^^^^
		PBYTE matchCTaskbandPinCreateInstance = (PBYTE)FindPattern((uintptr_t)twinui_pcshell, "48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 83 64 24 ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 8B 8D ?? ?? ?? ?? 85 C0");

		if (matchCTaskbandPinCreateInstance)
		{
			matchCTaskbandPinCreateInstance += 21;
			matchCTaskbandPinCreateInstance += 5 + *(int*)(matchCTaskbandPinCreateInstance + 1);

		}

		if (!matchCTaskbandPinCreateInstance)
		{
			// wil::out_param() destructor inlined
			// 0F 1F 44 00 00 48 83 64 24 ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 8B 8D ?? ?? ?? ?? 85 C0
			//                                                    ^^^^^^^^^^^
			matchCTaskbandPinCreateInstance = (PBYTE)FindPattern((uintptr_t)twinui_pcshell, "0F 1F 44 00 00 48 83 64 24 ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 8B 8D ?? ?? ?? ?? 85 C0");

			if (matchCTaskbandPinCreateInstance)
			{
				matchCTaskbandPinCreateInstance += 16;
				matchCTaskbandPinCreateInstance += 5 + *(int*)(matchCTaskbandPinCreateInstance + 1);
			}
		}

		if (matchCTaskbandPinCreateInstance)
		{
			CTaskbandPin_CreateInstance = (CTaskbandPin_CreateInstance_t)matchCTaskbandPinCreateInstance;
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	// Ittr: We initialise values for closing program if incompatible software is present
	WCHAR programPath[MAX_PATH] = L"\\Stardock\\WindowBlinds 11\\unins000.exe";
	WCHAR blacklistPath[MAX_PATH];
	ExpandEnvironmentStringsW(L"%ProgramFiles%", (LPWSTR)blacklistPath, sizeof(blacklistPath));
	lstrcat(blacklistPath, programPath);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		PatchShunimpl();

		if (GetFileAttributesW((LPCWSTR)blacklistPath) != INVALID_FILE_ATTRIBUTES) // Windowblinds blockage part 1 - create user-facing error
			CrashError(); // The user-facing crash message - we do these blocks of code like this, so that the 0xc0000142 error doesn't appear

		if (g_osVersion.BuildNumber() >= 26100)
		{
			InitPinnedListHack();
		}

		CreateShellFolder(); // Fix shell folder for 1607+...
		EnsureWindowColorization(); // Correct colorization enablement setting for Win10/11
		FirstRunCompatibilityWarning(); // Warn users on Windows 11 24H2+ and Server 2022 of potential problems
		FirstRunPrereleaseWarning(); // Warn users if this is a pre-release build that this is the case on first run ONLY
		ThemeHandlesInit(); // Basically start the inactive theme management process

		dbgprintf(L"Dll Attach\n");

		// Ittr: Load user configuration from the registry
		// - Important that we do this first before applying API hooks
		InitializeConfiguration();

		// Ittr: Handle pattern byte replacement patches, usually for disabling or fixing features
		ChangePatternImports();

		// Ittr: Handle address import changes, usually for rewriting or modifying results from API
		ChangeAddressImports();

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

		if (GetFileAttributes((LPCWSTR)blacklistPath) != INVALID_FILE_ATTRIBUTES) // Windowblinds blockage part 2 - actually stops the program from running
			ExitExplorerSilently(); //byebye WB users

	}
	break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		EndThemeHandles();
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

		if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // Ittr: Only create TWinUI UWP mode here if we are going to use it
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
		{
			id = IID_IFlexibleTaskbarPinnedList;
		}
		else if (build >= 17763)
		{
			id = IID_IPinnedList3;
		}

		if (rclsid == CLSID_TaskbarPin && CTaskbandPin_CreateInstance && build >= 26100) // Windows 11...
		{
			CTaskbandPin_W32PTP* pTaskbandPin;
			result = CTaskbandPin_CreateInstance(&pTaskbandPin);
			dbgprintf(L"CTaskbandPin_CreateInstance result: %p", result);
			if (SUCCEEDED(result))
			{
				result = ((IUnknown*)pTaskbandPin)->QueryInterface(id, ppv);
				dbgprintf(L"CTaskbandPin_CreateInstance result 2: %p", result);
				((IUnknown*)pTaskbandPin)->Release();
			}
		}
		else
		{
			result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, id, ppv);
		}

		if (SUCCEEDED(result))
		{
			*ppv = new CPinnedListWrapper((IUnknown*)*ppv, build);
		}

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
			if (build >= 10074)
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
		if (g_osVersion.BuildNumber() < 10074) // Ittr: gate fakeimmersive to 8.1 due to functional issues (e.g. hanging) with 10 - restoring this on 10 is now seemingly unnecessary
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
		if (g_osVersion.BuildNumber() < 10240) // Ittr: gate fakeimmersive to 8.1 due to functional issues (e.g. hanging) with 10
		{
			UnregisterFakeImmersive();
			UnregisterProjection();
		}
	}
	return CoRevokeClassObject(dwRegister);
}
