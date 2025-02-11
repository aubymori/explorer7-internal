#pragma once
#include "common.h"
#include "dbgprint.h"
#include "OptionConfig.h"
#include "OSVersion.h"
#include "TypeDefinitions.h"
#include "ThemeManager.h"

// Ittr: Code that doesn't relate to specific hooks resides here
// e.g. helper functions, HWND retrieval functions, error messages, non-descript registry changes

BOOL WINAPI RetTrue()
{
	return TRUE;
}

BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

HMODULE GetCurrentModuleHandle() //use for internal resource calls... honestly i just wanted to show it could be done 
{
	HMODULE hMod = NULL;
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&GetCurrentModuleHandle), &hMod);
	return hMod;
}

bool IsClassicTheme(void)
{
	return !IsThemeActive() || s_ClassicTheme;
}

bool AllowThemes(void)
{
	return !IsClassicTheme();
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

static HWND GetThumbnailWnd()
{
	if (!hwnd_taskthumb)
		hwnd_taskthumb = FindWindow(L"TaskListThumbnailWnd", NULL);
	return hwnd_taskthumb;
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

// this setup is created so that SetWindowTheme can apply Windows 8-era classes without causing crashing
extern HTHEME g_currentTheme = 0;

void LoadCurrentTheme(HWND hwnd, LPCWSTR pszClassList)
{
	g_currentTheme = 0;
	DWORD flags = 2;
	if ((unsigned int)GetScreenDpi() != 96)
		flags |= 1u;

	if (g_loadedTheme)
		g_currentTheme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassList, flags);
	else
		g_currentTheme = fOpenThemeData(hwnd, pszClassList);
}


// Ittr: Forcing this change fixes colorization on aero.msstyles for 1809+ on taskbar and start menu ONLY.
void EnsureWindowColorization()
{
	if (g_osVersion.BuildNumber() >= 17763)
	{
		DWORD value = 0; // initialise in memory
		DWORD colorVal = 1; // doesn't work when reduced to a single string, annoying but atleast we can use it here
		RegGetDWORD(HKEY_CURRENT_USER, sz_DesktopWindowManagerKey, L"EnableWindowColorization", &value); // output the data from attributes key...

		if (value != colorVal) // basically if the attribute value doesn't exist or is the wrong value...
		{
			RegSetDWORD(HKEY_CURRENT_USER, sz_DesktopWindowManagerKey, L"EnableWindowColorization", &colorVal); // apply folder attributes, arguably the most important part
		}
	}
}

DWORD GetColorizationColor()
{
	DWMCOLORIZATIONPARAMS colors;
	CHAR buffer[0x28];
	memset(buffer, 0, 0x28);
	DwmGetColorizationParametersOrig(&buffer);
	memcpy(&colors, (PVOID)buffer, sizeof(DWMCOLORIZATIONPARAMS));

	int a = (colors.ColorizationColor >> 24) & 0xFF;
	int r = (colors.ColorizationColor >> 16) & 0xFF;
	int g = (colors.ColorizationColor >> 8) & 0xFF;
	int b = (colors.ColorizationColor) & 0xFF;

	// thanks to microsoft we have to account for automatic colorization being bugged on 10+ as alpha is set to 0. Yay...
	if (g_osVersion.BuildNumber() >= 10074 && s_ColorizationOptions != 3 && a == 0x00 && (r != 0x00 || g != 0x00 || b != 0x00)) // only apply if it appears that the user is trying to set an actual colour - full transparency remains possible!
	{
		a = 0xC4; // we default to this as it's used by the majority of win10/11 default colours
	}

	// Approximate default Windows 8.1 translucency if user has regular 10/11 colours used and has not manually set to 0xC4
	if (s_UseWin8DefaultAlpha && a == 0xC4)
	{
		a = 0x74;
	}

	// mode 4 (gradient non-transparent is buggy) + current thumbnail edge case 
	if (s_ColorizationOptions == 4) 
	{
		a = 0xFF;
	}

	// Windows 10 and 11 users specifically without glass tools may struggle to adjust color opacity, this optional override fixes this
	if (s_OverrideAlpha && (s_ColorizationOptions == 1 || s_ColorizationOptions == 2))
	{
		a = (s_AlphaValue) & 0xFF;
	}

	if (s_ColorizationOptions == 3)
	{
		GetThemeName = (GetThemeName_t)GetProcAddress(LoadLibrary(L"uxtheme.dll"), (LPSTR)74);
		RefreshImmersiveColorPolicyState = (RefreshImmersiveColorPolicyState_t)GetProcAddress(LoadLibrary(L"uxtheme.dll"), (LPSTR)104);
		GetIsImmersiveColorUsingHighContrast = (GetIsImmersiveColorUsingHighContrast_t)GetProcAddress(LoadLibrary(L"uxtheme.dll"), (LPSTR)106);
		GetUserColorPreference = (GetUserColorPreference_t)GetProcAddress(LoadLibrary(L"uxtheme.dll"), (LPSTR)120);
		GetColorFromPreference = (GetColorFromPreference_t)GetProcAddress(LoadLibrary(L"uxtheme.dll"), (LPSTR)121);
	}

	IMMERSIVE_COLOR_TYPE imclr;

	switch (s_AcrylicAlt)
	{
		case 1:
			imclr = IMCLR_SystemAccentDark2;
			break;
		case 2:
			imclr = IMCLR_SystemAccentLight2;
			break;
		default:
			imclr = IMCLR_HardwareGutterRest;
			break;
	}

	DWORD color = (s_ColorizationOptions != 3 || s_AcrylicAlt == 3) ? ((a << 24) | (b << 16) | (g << 8) | r) : ((s_OverrideAlpha ? ((s_AlphaValue & 0xFF) << 24) : 0xCC000000) | (CImmersiveColor::GetColor(imclr) & 0xFFFFFF));
	return color;
}

ACCENT_STATE GetAccentState(bool isThumbnail)
{
	if (s_ColorizationOptions == 3) // acrylic (1803-)
		return ACCENT_ENABLE_ACRYLICBLURBEHIND;
	else if (s_ColorizationOptions == 2) // blurbehind (1507 until 11 21h2)
		return ACCENT_ENABLE_BLURBEHIND;

	if (isThumbnail) // run this block after the other ones, to ensure that pseudo-aero mode uses opaque thumbnail. using the option definition causes extreme visual bugs for some reason.
		return ACCENT_ENABLE_GRADIENT;

	// pseudo-aero & solid-color (all versions) - the replacements for option 0 & fallback for other values entered > 4
	return ACCENT_ENABLE_TRANSPARENTGRADIENT; // we use transparentgradient for both 1 and 4, as gradient has some weird hrgn side-effects on start menu

}

__forceinline WINDOWCOMPOSITIONATTRIBDATA GetTrayAccentProperties(bool isThumbnail)
{
	// to break down what happens here:
	// - we create an accent policy
	// - we take in whether thumbnail wnd is calling so we can make tweaks as needed
	// - accent state gets calculated and returned based on user preference
	// - this calculation is tweaked if thumbnail wnd flag is passed in, as pseudo-aero looks better with solid thumbnail
	// - we then define the accent flags - start menu and taskbar work fine with 0x13, thumbnail requires 0x200 to work properly on later windows 10 versions (presumably OK on earlier vers)
	// - 0x200 is then added to with extra to ensure that blurbehind mode takes in color properly
	// - we then define gradient color by pulling either DWM accent color or immersive color as applicable
	// this is then passed into attribute data which we call back into whenever we need to get accent properties without retyping this whole function

	if (g_osVersion.BuildNumber() >= 22631 && s_ColorizationOptions == 3) // Acrylic colorization misbehaves on 11. Removing 0x2 flag fixes this
	{
		WINDOWCOMPOSITIONATTRIBDATA attrData;
		ACCENT_POLICY accentPolicy;

		accentPolicy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
		accentPolicy.AccentFlags = (isThumbnail) ? (0x1 | 0x200) : (0x11); // very important that this is set up like this!
		accentPolicy.GradientColor = GetColorizationColor();

		attrData.Attrib = WCA_ACCENT_POLICY;
		attrData.pvData = &accentPolicy;
		attrData.cbData = sizeof(accentPolicy);
		return attrData;
	}

	WINDOWCOMPOSITIONATTRIBDATA attrData;
	ACCENT_POLICY accentPolicy;

	accentPolicy.AccentState = GetAccentState(isThumbnail);
	accentPolicy.AccentFlags = (isThumbnail) ? (0x1 | 0x2 | 0x200) : (0x13); // very important that this is set up like this!
	accentPolicy.GradientColor = GetColorizationColor();

	attrData.Attrib = WCA_ACCENT_POLICY;
	attrData.pvData = &accentPolicy;
	attrData.cbData = sizeof(accentPolicy);
	return attrData;
}

// Ittr: Less lines of code and more utility/reusability for setting composition attributes in future
void ForceActiveWindowAppearance(HWND hwnd)
{
	BOOL bForceActiveWindowAppearance = true;
	WINDOWCOMPOSITIONATTRIBDATA attrData;
	attrData.Attrib = WCA_FORCE_ACTIVEWINDOW_APPEARANCE;
	attrData.pvData = &bForceActiveWindowAppearance;
	attrData.cbData = sizeof(bForceActiveWindowAppearance);
	SetWindowCompositionAttribute(hwnd, &attrData);
}

const UINT ThemeChangeMessage = WM_USER + 69420;
BOOL CALLBACK RefreshWindows(HWND wnd, LPARAM prm)
{
	if (wnd == (HWND)prm) return TRUE;

	PostMessage(wnd, WM_THEMECHANGED, 0, 0);
	dbgprintf(L"themechanged sent to %i", wnd);
	return TRUE;
}

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
// this is defined here rather than in AddressImports.h so that utility ShouldAddWindowToTray can work properly
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

// Generic crash error
void CrashError()
{
	WCHAR errorText[71] = L"An unexpected error occurred and explorer7 needs to quit. We're sorry!"; // Funny brick game message go haha
	WCHAR errorTitle[16] = L"explorer7 Crash";

	MessageBoxW(NULL, errorText, errorTitle, MB_ICONERROR); // the actual error box lol
}

// Create all programs shellfolder on 1607+ where it doesn't already exist
void CreateShellFolder()
{
	//addendum: using the regular HKLM location is not viable for non-administrator users so we store in HKCU, which causes it to turn up in HKEY_USERS somewhere. 
	//this shouldn't work, but it does :P
	if (g_osVersion.BuildNumber() >= 14393) // Ittr: byebye shellfolder.reg
	{
		DWORD value = 0; // initialise in memory
		DWORD attrVal = 0x28100000; // doesn't work when reduced to a single string, annoying but atleast we can use it here
		RegGetDWORD(HKEY_CURRENT_USER, sz_ShellFolder3, L"Attributes", &value); // output the data from attributes key...

		if (value != attrVal) // basically if the attribute value doesn't exist or is the wrong value...
		{
			// we create all the relevant values. issue solved for new users - program list works out of the box now
			RegSetSZ(HKEY_CURRENT_USER, sz_ShellFolder, NULL, (DWORD*)L"Programs Folder and Fast Items"); // create clsid name
			RegSetExpandSZ(HKEY_CURRENT_USER, sz_ShellFolder2, NULL, (DWORD*)L"%SystemRoot%\\system32\\shell32.dll"); // point it to shell32
			RegSetSZ(HKEY_CURRENT_USER, sz_ShellFolder2, L"ThreadingModel", (DWORD*)L"Apartment"); // regular threading model criteria...
			RegSetDWORD(HKEY_CURRENT_USER, sz_ShellFolder3, L"Attributes", &attrVal); // apply folder attributes, arguably the most important part
		}
	}
}

// Compatibility warning for Windows 11 24H2+
void FirstRunCompatibilityWarning()
{
	if (g_osVersion.BuildNumber() >= 26100 || g_osVersion.BuildNumber() == 20348) // temporary one-off M2 warning for Win11 24H2 users, permanent for iron users
	{
		DWORD value = 0;
		RegGetDWORD(HKEY_CURRENT_USER, sz_SettingsKey, L"FirstRunVersionCheck", &value);
		if (value != 1)
		{
			MessageBoxW(NULL, L"This build of Windows is not currently supported.\n\nYou may encounter usability issues.", L"explorer7", MB_ICONEXCLAMATION);
			DWORD newValue = 1;
			RegSetDWORD(HKEY_CURRENT_USER, sz_SettingsKey, L"FirstRunVersionCheck", &newValue);
		}
	}
}

// One-off warning for pre-release version
void FirstRunPrereleaseWarning()
{
	DWORD value = 0;
	RegGetDWORD(HKEY_CURRENT_USER, sz_SettingsKey, L"FirstRunPrereleaseCheck", &value);
	if (value != 1)
	{
#ifdef PRERELEASE_COPY
		MessageBoxW(NULL, L"Evaluation copy.\nFor testing purposes only.", L"explorer7", MB_ICONEXCLAMATION);
		DWORD newValue = 1;
		RegSetDWORD(HKEY_CURRENT_USER, sz_SettingsKey, L"FirstRunPrereleaseCheck", &newValue);
#endif
	}
}

// Ittr: The following 3 functions are here rather than any specific imports header because they are used by 2 different patch types
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
	if (s_ImmersiveShell < 2) // immersive enabled
	{
		DWORD p0 = (DWORD)_ReturnAddress();
		dwExStyle = dwExStyle | WS_EX_TOOLWINDOW; // TODO is this needed?

		WCHAR charmWnd[23] = L"Shell_CharmWindow";
		if (strcmp((char*)lpClassName, (char*)charmWnd) == 0)
		{
			dwExStyle |= WS_EX_TOPMOST;
		}


		HWND ret = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hwndParent, hMenu, hInstance, lpParam);

		// We do this to eliminate the ghost window
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
	else // Preserve legacy codepath for Windows 8.1 and non-immersive users
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

	// We do this to eliminate the ghost window
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

BOOL DoesOSBuildSupportSpotlight()
{
	return (g_osVersion.BuildNumber() == 22000 && g_osVersion.BuildRevision() >= 706) || (g_osVersion.BuildNumber() >= 22598) || (g_osVersion.BuildNumber() >= 19045);
}

BOOL IsSpotlightEnabled()
{
	HKEY hKey = NULL;
	BOOL bRet = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}", 0, KEY_READ, &hKey) == ERROR_SUCCESS;
	if (bRet) RegCloseKey(hKey);
	return bRet;
}

#define SCRATCH_QCM_FIRST 1
#define SCRATCH_QCM_LAST  0x7FFF

#define SPOP_OPENMENU            1
#define SPOP_INSERTMENU_ALL      0b1111110000
#define SPOP_INSERTMENU_OPEN     0b0000010000
#define SPOP_INSERTMENU_NEXTPIC  0b0000100000
#define SPOP_INSERTMENU_LIKE     0b0001000000
#define SPOP_INSERTMENU_DISLIKE  0b0010000000
#define SPOP_INSERTMENU_INFOTIP1 0b0100000000
#define SPOP_INSERTMENU_INFOTIP2 0b1000000000
#define SPOP_CLICKMENU_FIRST     40000
#define SPOP_CLICKMENU_OPEN      40000
#define SPOP_CLICKMENU_NEXTPIC   40001
#define SPOP_CLICKMENU_LIKE      40002
#define SPOP_CLICKMENU_DISLIKE   40003
#define SPOP_CLICKMENU_LAST      40003

#define SEE_MASK_UNICODE 0x00004000

TCHAR* mini_wcschr(const TCHAR* str, TCHAR ch)
{
	const TCHAR* ptr = str;
	while (*ptr != '\0' && *ptr != ch) ptr++;
	if (*ptr == ch) return (TCHAR*)ptr;
	return NULL;
}

const int spop_insertmenu_ops[] = { SPOP_INSERTMENU_OPEN, SPOP_INSERTMENU_NEXTPIC, 0, SPOP_INSERTMENU_LIKE, SPOP_INSERTMENU_DISLIKE };
void SpotlightHelper(DWORD dwOp, HWND hWnd, HMENU hMenu, LPPOINT pPt)
{
	HRESULT hr = S_OK;
	LPITEMIDLIST pidl = NULL;
	SFGAOF sfgao = 0;
	if (SUCCEEDED(hr = SHParseDisplayName(L"::{2CC5CA98-6485-489A-920E-B3E88A6CCCE3}", NULL, &pidl, 0, &sfgao)))
	{
		IShellFolder* psf = NULL;
		LPCITEMIDLIST pidlChild;
		if (SUCCEEDED(hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&psf, &pidlChild)))
		{
			IContextMenu* pcm = NULL;
			if (SUCCEEDED(hr = psf->GetUIObjectOf(hWnd, 1, &pidlChild, IID_IContextMenu, NULL, (void**)&pcm)))
			{
				HMENU hMenu2 = CreatePopupMenu();
				if (hMenu2)
				{
					if (SUCCEEDED(hr = pcm->QueryContextMenu(hMenu2, 0, SCRATCH_QCM_FIRST, SCRATCH_QCM_LAST, CMF_NORMAL)))
					{
						if (dwOp == SPOP_OPENMENU)
						{
							int iCmd = TrackPopupMenuEx(hMenu2, TPM_RETURNCMD, pPt->x, pPt->y, hWnd, NULL);
							if (iCmd > 0)
							{
								CMINVOKECOMMANDINFOEX info = { 0 };
								info.cbSize = sizeof(info);
								info.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
								info.hwnd = hWnd;
								info.lpVerb = MAKEINTRESOURCEA(iCmd - SCRATCH_QCM_FIRST);
								info.lpVerbW = MAKEINTRESOURCEW(iCmd - SCRATCH_QCM_FIRST);
								info.nShow = SW_SHOWNORMAL;
								info.ptInvoke = *pPt;
								pcm->InvokeCommand((CMINVOKECOMMANDINFO*)&info);
							}
						}
						else if (!(dwOp & ~SPOP_INSERTMENU_ALL))
						{
							MENUITEMINFOW mii;
							int i = ARRAYSIZE(spop_insertmenu_ops) - 1;
							while (1)
							{
								if (i == -1 ? ((dwOp & SPOP_INSERTMENU_INFOTIP1) || (dwOp & SPOP_INSERTMENU_INFOTIP2)) : (dwOp & spop_insertmenu_ops[i]))
								{
									mii.cbSize = sizeof(MENUITEMINFOW);
									mii.fMask = MIIM_FTYPE | MIIM_STRING;
									mii.cch = 0;
									mii.dwTypeData = NULL;
									if (i <= 0 ?
										(i == 0 ?
											!RegQueryValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", NULL, (PLONG)&mii.cch) :
											!RegGetValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", L"InfoTip", RRF_RT_REG_SZ, NULL, NULL, (LPDWORD)&mii.cch)
											) :
										GetMenuItemInfoW(hMenu2, i, TRUE, &mii))
									{
										WCHAR* buf = (WCHAR*)malloc(++mii.cch * sizeof(WCHAR));
										if (buf)
										{
											mii.dwTypeData = buf;
											if (i <= 0 ?
												(i == 0 ?
													!RegQueryValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", mii.dwTypeData, (PLONG)&mii.cch) :
													!RegGetValueW(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\{2cc5ca98-6485-489a-920e-b3e88a6ccce3}", L"InfoTip", RRF_RT_REG_SZ, NULL, mii.dwTypeData, (LPDWORD)&mii.cch)
													) :
												GetMenuItemInfoW(hMenu2, i, TRUE, &mii))
											{
												if (i == -1)
												{
													WCHAR* pCInit = mii.dwTypeData;
													WCHAR* pC = mini_wcschr(mii.dwTypeData, L'\r');
													if (pC)
													{
														pC[0] = 0;

														pC++;
														WCHAR* pC2 = mini_wcschr(pC, L'\r');
														if (pC2)
														{
															pC2[0] = 0;
														}
														mii.dwTypeData = pC;

														mii.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA | MIIM_STATE;
														mii.wID = 3999 + i - 1;
														mii.dwItemData = SPOP_CLICKMENU_FIRST + i - 1;
														mii.fType = MFT_STRING;
														mii.fState = MFS_DISABLED;
														if (dwOp & SPOP_INSERTMENU_INFOTIP2)
														{
															InsertMenuItemW(hMenu, 3, TRUE, &mii);
														}

														mii.dwTypeData = pCInit;
													}
												}
												mii.fMask = MIIM_ID | MIIM_STRING | MIIM_DATA | (i == -1 ? MIIM_STATE : 0);
												mii.wID = 3999 + i;
												mii.dwItemData = SPOP_CLICKMENU_FIRST + i;
												mii.fType = MFT_STRING;
												if (i == -1) mii.fState = MFS_DISABLED;
												if (i != -1 || (i == -1 && (dwOp & SPOP_INSERTMENU_INFOTIP1)))
												{
													InsertMenuItemW(hMenu, 3, TRUE, &mii);
												}
											}
											free(buf);
										}
									}
								}
								i--;
								if (i < -1) break;
							}
							mii.fMask = MIIM_FTYPE | MIIM_DATA;
							mii.dwItemData = 0;
							mii.fType = MFT_SEPARATOR;
							InsertMenuItemW(hMenu, 3, TRUE, &mii);
						}
						else if (dwOp >= SPOP_CLICKMENU_FIRST && dwOp <= SPOP_CLICKMENU_LAST)
						{
							MENUITEMINFOW mii;
							mii.cbSize = sizeof(MENUITEMINFOW);
							mii.fMask = MIIM_ID;
							if (GetMenuItemInfoW(hMenu2, dwOp - SPOP_CLICKMENU_FIRST, TRUE, &mii))
							{
								CMINVOKECOMMANDINFOEX info = { 0 };
								info.cbSize = sizeof(info);
								info.fMask = CMIC_MASK_UNICODE;
								info.hwnd = hWnd;
								info.lpVerb = MAKEINTRESOURCEA(mii.wID - SCRATCH_QCM_FIRST);
								info.lpVerbW = MAKEINTRESOURCEW(mii.wID - SCRATCH_QCM_FIRST);
								info.nShow = SW_SHOWNORMAL;
								pcm->InvokeCommand((CMINVOKECOMMANDINFO*)&info);
							}
						}
					}
					DestroyMenu(hMenu2);
				}
				pcm->Release();
			}
			psf->Release();
		}
		CoTaskMemFree(pidl);
	}
}