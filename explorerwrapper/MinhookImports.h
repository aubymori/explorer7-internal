#pragma once
#include "util.h"
#include "common.h"
#include "dbgprint.h"
#include "OptionConfig.h"
#include "OSVersion.h"
#include "TypeDefinitions.h"
#include "MinHook.h"
#include "NscTree.h"

HTHEME __stdcall OpenThemeData_Hook(HWND hwnd, LPCWSTR pszClassList)
{
	if (g_dwTrayThreadId > 0 && g_dwTrayThreadId != GetCurrentThreadId())
		return fOpenThemeData(hwnd, pszClassList);

	if (!AllowThemes())
		return NULL;

	LoadCurrentTheme(hwnd, pszClassList);

	if (g_currentTheme == nullptr)
		dbgprintf(L"OPENTHEMEDATA FAILED %s", pszClassList);

	themeHandles->push_back(g_currentTheme);
	return g_currentTheme;
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

	// Ittr: Windows 11 introduces issues with applying themes to the SearchFolder interface, due to the ItemsViewAccessible::Header addition
	// This is resolved by simply falling back to the system theme if the DirectUI theme call attempts to load this class
	if (g_loadedTheme && (lstrcmp(pszClassList, L"ItemsViewAccessible::Header") != 0))
	{
		theme = OpenThemeDataFromFile(g_loadedTheme, hwnd, pszClassList, flags);
	}
	else
	{
		theme = fOpenThemeDataForDpi(hwnd, pszClassList, dpi);
	}

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

void CPniMainDlg_ShowFlyoutNEW() // don't bother with the parameters as we aren't going to use them
{
	// Open Network and Sharing Center instead inside the Windows Control Panel, as a non-immersive alternative
	ShellExecuteW(nullptr, nullptr, L"control.exe", L"/name Microsoft.NetworkAndSharingCenter", nullptr, SW_SHOWNORMAL);

	// End function as we aren't going to do anything else here
	return;
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

	PROPVARIANT pv;
	ips->GetValue(PKEY_AppUserModel_ID, &pv);
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

PVOID CTaskBandPtr = 0;

VOID CTaskBand_SetWindowIconHook(PVOID This, HWND a2, HICON a3, int a4)
{
	CTaskBandPtr = This;

	auto bIsImmersiveWnd = [](HWND hwnd) -> bool
		{
			return IsShellFrameWindow && IsShellFrameWindow(hwnd); // Temporary - plan is to rewrite SWI hook and account for Centennial icons
		};

	if (bIsImmersiveWnd(a2))
	{
		HICON hc = GetUWPIcon(a2);
		if (icon)
		{
			CTaskBand_SetWindowIconOrig(This, a2, icon, a4);
		}
	}
	else
	{
		CTaskBand_SetWindowIconOrig(This, a2, a3, a4);
	}
}

VOID UpdateItemIcon(PVOID This, int a2)
{
	typedef void* (WINAPI* GetTaskItemFunc)(void*);
	typedef HWND(WINAPI* GetWindowFunc)(void*);

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
		HICON hc = GetUWPIcon(v6);
		if (hc)
		{
			SetIconThumb(This, hc, a2, 3);
		}
	}
	else
	{
		UpdateItem(This, a2);
	}

}

void SetUpThemeManager()
{
	// Initialize the theme manager and declare the types for the UXTheme apis we're hooking
	ThemeManagerInitialize();

	fOpenThemeData = decltype(fOpenThemeData)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeData"));
	fOpenThemeDataForDpi = decltype(fOpenThemeDataForDpi)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataForDpi"));
	fOpenThemeDataEx = decltype(fOpenThemeDataEx)(GetProcAddress(GetModuleHandle(L"uxtheme.dll"), "OpenThemeDataEx"));

	// Hook UXTheme-related calls for the purpose of our inactive theme system.
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeData), OpenThemeData_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeData));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataForDpi), OpenThemeDataForDpi_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataForDpi));
	MH_CreateHook(static_cast<LPVOID>(fOpenThemeDataEx), OpenThemeDataEx_Hook, reinterpret_cast<LPVOID*>(&fOpenThemeDataEx));
}

void FixNonImmersivePniDui()
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

void UpdateTrayWindowDefinitions()
{
	// Hook and update definitions of what windows should be added to the tray - largely for UWP purposes, but essentially zero-cost so included on both immersive on and off modes.
	void* _ShouldAddWindowToTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB");
	void* _IsWindowNotDesktopOrTray = (void*)FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 33 DB FF 15 ?? ?? ?? ?? 3B C3 74 ?? 48 3B 3D");

	MH_CreateHook(_ShouldAddWindowToTray, ShouldAddWindowToTray, &_ShouldAddWindowToTray);
	MH_CreateHook(_IsWindowNotDesktopOrTray, IsWindowNotDesktopOrTray, &_IsWindowNotDesktopOrTray);
}

void SetProgramListNscTreeAttributes()
{
	// If we are on Windows 10 or higher, query the original program list pattern and create our hook to fix the visual issues
	if (g_osVersion.BuildNumber() >= 10074)
	{
		CNSCHost_FillNSCOg = (decltype(CNSCHost_FillNSCOg))FindPattern((uintptr_t)GetModuleHandle(0), "48 89 5C 24 18 57 48 83 EC 30 33 DB 48 8B F9 39 99 CC 00 00 00");
		if (CNSCHost_FillNSCOg)
		{
			MH_CreateHook(static_cast<LPVOID>(CNSCHost_FillNSCOg), CNSCHost_FillNSC, reinterpret_cast<LPVOID*>(&CNSCHost_FillNSCOg)); //this hook is in nsctree.h now
		}
	}
}

void HandleThumbnailColorization()
{
	// CTaskListThumbnailWnd::_Render
	// Thumbnail rendering fix for colorization modes
	if (g_osVersion.BuildNumber() >= 10074) // we don't apply to 8.1 as only pseudo-aero is supported there
	{
		char* CTaskListThumbnailWnd_Render = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 20 44 89 40 18 57 41 54 41 55 41 56 41 57 48 81 EC 90 00 00 00 48 8B F9";
		void* CTLWRPattern = (void*)FindPattern((uintptr_t)GetModuleHandle(NULL), CTaskListThumbnailWnd_Render);

		if (CTLWRPattern)
		{
			MH_CreateHook(CTLWRPattern, RenderThumbnail, reinterpret_cast<LPVOID*>(&renderThumbnail_orig));
		}
		else // 7779 and 7785
		{
			CTaskListThumbnailWnd_Render = "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC A0 00 00 00 48 8B 05 ?? ?? 04 00 48 33 C4";
			CTLWRPattern = (void*)FindPattern((uintptr_t)GetModuleHandle(NULL), CTaskListThumbnailWnd_Render);

			if (CTLWRPattern)
			{
				MH_CreateHook(CTLWRPattern, RenderThumbnail, reinterpret_cast<LPVOID*>(&renderThumbnail_orig));
			}
		}
	}
}

void RenderStoreAppsOnTaskbar()
{
	if (s_ShowStoreAppsOnTaskbar && g_osVersion.BuildNumber() >= 10074)
	{
		// Part 1: CTaskListThumbnailWnd::_SetIcon
		// Must be defined so that it can be called by our hook functions
		// However, we only assign the definition if we can actually detect it to begin with
		char* CTaskListThumbnailWnd_SetIcon = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 49 63 D8 4C 8B 81 B0 00 00 00";
		setIconThumb_t CTLTWSIPattern = (setIconThumb_t)FindPattern((uintptr_t)GetModuleHandle(NULL), CTaskListThumbnailWnd_SetIcon);

		if (CTLTWSIPattern)
		{
			SetIconThumb = CTLTWSIPattern;
		}
		else // 7779 and 7785
		{
			CTaskListThumbnailWnd_SetIcon = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B 81 B0 00 00 00 49 63 D8";
			CTLTWSIPattern = (setIconThumb_t)FindPattern((uintptr_t)GetModuleHandle(NULL), CTaskListThumbnailWnd_SetIcon);

			if (CTLTWSIPattern)
			{
				SetIconThumb = CTLTWSIPattern;
			}
			else
			{
				// In this case if we are unable to find the definition, return here so the function doesn't apply the hooks
				// This prevents stability issues if we are unable to define this properly
				return;
			}
		}

		// Part 2: CTaskBand::_SetWindowIcon 
		// Must be hooked accordingly so the icon can be overridden as necessary for the TaskItem buttons
		char* CTaskBand_SetWindowIcon = "FF F3 55 56 57 41 54 41 55 41 56 41 57 48 81 EC F8 06 00 00";
		void* CTBSWIPattern = (void*)FindPattern((uintptr_t)GetModuleHandle(NULL), CTaskBand_SetWindowIcon);

		if (CTBSWIPattern)
		{
			MH_CreateHook(static_cast<LPVOID>(CTBSWIPattern), CTaskBand_SetWindowIconHook, reinterpret_cast<LPVOID*>(&CTaskBand_SetWindowIconOrig));
		}
		else // 7779 and 7785
		{
			CTaskBand_SetWindowIcon = "4C 8B DC 49 89 5B 08 49 89 73 10 49 89 7B 18 4D 89 63 20 55 48 8B EC";
			CTBSWIPattern = (void*)FindPattern((uintptr_t)GetModuleHandle(NULL), CTaskBand_SetWindowIcon);

			if (CTBSWIPattern)
			{
				MH_CreateHook(static_cast<LPVOID>(CTBSWIPattern), CTaskBand_SetWindowIconHook, reinterpret_cast<LPVOID*>(&CTaskBand_SetWindowIconOrig));
			}
		}

		// Part 3: CTaskListThumbnailWnd::_UpdateItemIcon
		// Hooking this function will allow the thumbnail icon to be updated as applicable
		char* CTaskListThumbnailWnd_UpdateItemIcon = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 48 8B 81 B0 00 00 00";
		void* CTLTWUIIPattern = (void*)FindPattern((uintptr_t)GetModuleHandle(NULL), CTaskListThumbnailWnd_UpdateItemIcon);

		if (CTLTWUIIPattern)
		{
			MH_CreateHook(static_cast<LPVOID>(CTLTWUIIPattern), UpdateItemIcon, reinterpret_cast<LPVOID*>(&UpdateItem));
		}
	}
}

void CreateImmersiveShell()
{
	// NOTE: Some of the patch functions are in util.h rather than an imports header because they are used with several patch types
	////////////////////////////////
	// 1. Todo in future *after* feature-set is complete: see how many of these hooks can be ChangeImportedAddress instead of MH_CreateHook (perf optimisation)
	// 2. Code stack used exclusively for UWP mode, hence the conditional statement.
	if (s_EnableImmersiveShellStack == 1) // Run these hooks only if the user has UWP enabled
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
		MH_CreateHook(static_cast<LPVOID>(RegisterHotKeyApiOrg), RegisterWindowHotkeyNew, reinterpret_cast<LPVOID*>(&RegisterHotKeyApiOrg));

		int iImportsToPatch[] = {
			2511, // SetFallbackForeground
			2513, // SetActiveProcessForMonitor
			2514, // RegisterEdgy
			2537, // SendEventMessage
			2542, // RegisterShellPTPListener
			2563, // ClearForeground
			2564, // RegisterWindowArrangementCallout
			2566, // SetWindowArrangement
			2567, // EnableShellWindowManagementBehavior
			2579, // SetWindowShowState
			2581, // GetWindowTrackInfoAsync
			2585, // UpdateWindowTrackingInfo
			2627, // SetBridgeWindowChild
			2628, // CreateWindowGroup
			2629, // DeleteWindowGroup
			2631, // EnableWindowGroupPolicy
			2632, // SetWindowGroup
		};

		for (int i = 0; i < ARRAYSIZE(iImportsToPatch); ++i)
		{
			MH_CreateHook(GetProcAddress(GetModuleHandle(L"user32.dll"), MAKEINTRESOURCEA(iImportsToPatch[i])), RetTrue, NULL);
		}
	}
}

void ChangeMinhookImports()
{
	MH_Initialize();

	SetUpThemeManager(); // Local visual style management init
	FixNonImmersivePniDui(); // Non-immersive network flyout handling
	UpdateTrayWindowDefinitions(); // Ensure tray exclusion is corrected for modern Windows
	SetProgramListNscTreeAttributes(); // Restore the relevant contents to the program list
	HandleThumbnailColorization(); // Thumbnail colorization to match
	RenderStoreAppsOnTaskbar(); // UWP icon rendering for the taskbar
	CreateImmersiveShell(); // Immersive shell initialisation
}
