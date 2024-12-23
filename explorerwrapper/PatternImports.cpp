#include "PatternImports.h"

void RemoveLoadAnimationDataMap()
{
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
}

void RemoveGetClassIdForShellTarget()
{
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

// Ittr: Get rid of the immersive start menu and stop it appearing on TH1+ when UWP is on.
// This is very important and also extremely fragile.
// I'll also be honest - I haven't tested 1703 because who actually uses 1703
void DisableImmersiveStart()
{
	if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // because we don't want to run this thing if user isn't using UWP
	{
		char* ShowStartView; // XamlLauncher::ShowStartView
		unsigned char bytes[] = { 0xC3 }; // retn

		// load correct library - TH1 to RS1 use twinui, RS2 onwards use twinui.pcshell
		HMODULE twinui = (g_osVersion.BuildNumber() >= 15063) ? LoadLibrary(L"twinui.pcshell.dll") : LoadLibrary(L"twinui.dll");

		// so far there's only a few major revisions of this function as of 06-11-24
		// this may require further testing/advancement on windows 11 in co-ordination with partners
		if (g_osVersion.BuildNumber() >= 16299) // RS3 onwards
			ShowStartView = "48 89 5C 24 20 55 56 57 48 81 EC ?? 01 00 00 48 8B 05 ?? ?? ?? 00 48 33 C4 48 89 84 24 ?? 01 00 00 48 83 B9 ?? ?? 00 00 00";
		else if (g_osVersion.BuildNumber() >= 15063) // RS2
			ShowStartView = "48 89 5C 24 20 55 56 57 48 83 EC 30 48 83 B9 F8 00 00 00 00 41 8B E8";
		else if (g_osVersion.BuildNumber() >= 10074) // TH1 to RS1
			ShowStartView = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 83 B9 ?? 00 00 00 00";

		ChangeImportedPattern((char*)FindPattern((uintptr_t)twinui, ShowStartView), bytes, sizeof(bytes)); //byebye
	}

}

// Ittr: Get rid of the immersive search interface and prevent it appearing on TH1+ with ImmersiveShell enabled
// Otherwise, when invoked, takes up half the screen.
// Just use the Windows 7 start menu search - the functionality is much superior to this
void DisableImmersiveSearch()
{
	if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // because we don't want to run this thing if user isn't using UWP
	{
		char* CDEVSI; // CortanaDesktopExperienceView::ShowInternal
		// (preceded by CCortanaExperienceManager::ShowInternal in TH1-RS1)
		unsigned char bytes[] = { 0xC3 }; // retn

		// load correct library - TH1 to RS1 use twinui, RS2 onwards use twinui.pcshell (dll introduced in RS1 but not used widely)
		HMODULE twinui = (g_osVersion.BuildNumber() >= 15063) ? LoadLibrary(L"twinui.pcshell.dll") : LoadLibrary(L"twinui.dll");

		// seven different variants to account for as of 06-11-24
		if (g_osVersion.BuildNumber() >= 19041) // VB onwards
			CDEVSI = "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 20 57 48 83 EC 20 41 8B ?? 41 8B ?? 48 8B FA";
		else if (g_osVersion.BuildNumber() >= 18362) // 19H1 to 19H2
			CDEVSI = "40 55 53 56 57 41 54 41 56 41 57 48 8B EC 48 81 EC 80 00 00 00";
		else if (g_osVersion.BuildNumber() >= 17134) // RS4 to RS5
			CDEVSI = "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 20 57 48 83 EC 20 41 8B ?? 41 8B ?? 48 8B FA";
		else if (g_osVersion.BuildNumber() >= 16299) // RS3
			CDEVSI = "40 55 53 56 57 41 56 48 8D 6C 24 C9 48 81 EC 90 00 00 00";
		else if (g_osVersion.BuildNumber() >= 15063) // RS2
			CDEVSI = "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 20 41 8B D9 41 8B E8 48 8B F2";
		else if (g_osVersion.BuildNumber() >= 14393) // RS1
			CDEVSI = "40 55 53 56 57 41 56 48 8B EC 48 83 EC 70";
		else if (g_osVersion.BuildNumber() >= 10240) // TH1 to TH2
			CDEVSI = "48 8B C4 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC 90 00 00 00";

		// if user is using 19H1 or higher, search was reimplemented, which means we kill it twice
		if (g_osVersion.BuildNumber() >= 18362)
		{
			// because once wasn't enough.

			char* SCFOS; // XamlLauncherState::ShowCortanaFromOpenStart
			// exists in RS5, but not used until 19H1
			// replaced by XamlLauncherState::ShowSearchFromOpenStart in W11 Nickel

			if (g_osVersion.BuildNumber() >= 22621) // W11 Nickel onwards
				SCFOS = "48 89 54 24 10 55 53 56 57 41 54 41 56 41 57 48 8B EC 48 83 EC";
			else if (g_osVersion.BuildNumber() >= 19041) // VB onwards
				SCFOS = "48 89 54 24 10 55 53 56 57 41 56 41 57 48 8B EC 48 83 EC";
			else if (g_osVersion.BuildNumber() >= 18362) // 19H1 to 19H2
				SCFOS = "48 89 54 24 10 55 53 56 57 41 56 48 8B EC 48 83 EC 40 48 C7 45 E0 FE FF FF FF";

			ChangeImportedPattern((char*)FindPattern((uintptr_t)twinui, SCFOS), bytes, sizeof(bytes)); //byebye again
		}

		ChangeImportedPattern((char*)FindPattern((uintptr_t)twinui, CDEVSI), bytes, sizeof(bytes)); //byebye
	}
}

// Ittr: Get rid of the half-broken TaskView interface and prevent it appearing on TH1+ when UWP is on.
// This isn't detrimental to user experience to enable, but it completely breaks with the intended user experience.
// TaskView also causes crashing on earlier (pre-GE) Windows 11 which we can now avoid by disabling the remains of the feature.
// For some reason, Germanium and later already disable this. We're not complaining.
void DisableTaskView()
{
	if (s_EnableImmersiveShellStack && g_osVersion.BuildNumber() >= 10074) // because we don't want to run this thing if user isn't using UWP
	{
		char* TaskViewHostShow; // XamlAllUpViewHost::Show 
		// (preceded by CAllUpViewHost::Show in TH1-RS1, replaced by TaskViewHost::Show in W11 Nickel)
		unsigned char bytes[] = { 0xC3 }; // retn

		// load correct library - TH1 to RS1 use twinui, RS2 onwards use twinui.pcshell (dll introduced in RS1 but not used widely)
		HMODULE twinui = (g_osVersion.BuildNumber() >= 15063) ? LoadLibrary(L"twinui.pcshell.dll") : LoadLibrary(L"twinui.dll");

		// this function is particularly annoying - the signature is different in some way for many versions of Windows 10/11
		// in some cases, it changes and reverts again in later versions
		// :/
		if (g_osVersion.BuildNumber() >= 22621) // W11 Nickel onwards
			TaskViewHostShow = "40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC 30 03 00 00";
		else if (g_osVersion.BuildNumber() >= 21996) // W11 Cobalt
			TaskViewHostShow = "48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 81 EC 20 03 00 00";
		else if (g_osVersion.BuildNumber() >= 19041) // VB
			TaskViewHostShow = "48 89 5C 24 20 56 57 41 54 41 55 41 57 48 81 EC";
		else if (g_osVersion.BuildNumber() >= 17763) // RS5 to 19H2
			TaskViewHostShow = "4C 8B DC 57 41 54 41 55 41 56 41 57 48 81 EC 40 03 00 00";
		else if (g_osVersion.BuildNumber() >= 15063) // RS2 to RS4
			TaskViewHostShow = "4C 8B DC ?? 41 54 41 55 41 56 41 57 48 83 EC";
		else if (g_osVersion.BuildNumber() >= 10586) // TH2 to RS1
			TaskViewHostShow = "48 89 5C 24 20 55 56 57 41 54 41 55 41 56 41 57 ?? ?? ?? ?? ?? ?? ?? ?? 48 81 EC 50 02 00 00";
		else if (g_osVersion.BuildNumber() >= 10074) // TH1
			TaskViewHostShow = "48 89 5C 24 20 55 56 57 41 54 41 55 41 56 41 57 ?? ?? ?? ?? ?? ?? ?? ?? 48 81 EC E0 01 00 00";

		ChangeImportedPattern((char*)FindPattern((uintptr_t)twinui, TaskViewHostShow), bytes, sizeof(bytes)); //byebye
	}
}

void DisableWin11AltTab()
{
	if (g_osVersion.BuildNumber() >= 21996) // build check because this is unnecessary for windows 10
	{
		//Ittr: Why? Because it causes it to crash and its stupid
		char* immersiveBytes = "40 53 48 83 EC 20 83 79 ?? 02 74 17";

		//Load and patch DLL
		unsigned char bytes[] = { 0xB0, 0x00, 0xC3 };
		ChangeImportedPattern((char*)FindPattern((uintptr_t)LoadLibrary(L"twinui.pcshell.dll"), immersiveBytes), bytes, sizeof(bytes)); //byebye
	}
}

void FixWin11SearchIcon()
{
	// Ittr: An accidental change that actually works. Not complaining at all
	// Tested on 22000 and 26100
	// Not yet tested on Nickel (226xx)
	if (g_osVersion.BuildNumber() >= 21996) // build check because this is unnecessary for windows 10
	{
		char* searchBytes;

		if (g_osVersion.BuildNumber() >= 26100)
			searchBytes = "40 55 48 8B EC 48 83 EC 40"; // SHIsFileExplorerInTabletMode()
		else
			searchBytes = "48 89 5C 24 20 55 48 8B EC"; // SHIsFileExplorerInTabletMode()


		unsigned char bytes[] = { 0xB0, 0x00, 0xC3 };
		ChangeImportedPattern((char*)FindPattern((uintptr_t)LoadLibrary(L"ExplorerFrame.dll"), searchBytes), bytes, sizeof(bytes));
	}
}

void ChangePatternImports()
{
	// 1. Remove Windows 8+ animation msstyle classes so that legacy msstyles from Vista onwards are compatible with our theming system
	// 2. Remove Windows 8+ immersive shell msstyle classes so that legacy msstyles from Vista onwards are compatible with our theming system
	RemoveLoadAnimationDataMap();
	RemoveGetClassIdForShellTarget();

	// Responsible for fixing CLogoffOptions
	FixAuthUI();
	
	// Disable various unwanted immersive interfaces
	DisableImmersiveStart(); // Remove Windows 10+ immersive start menu for UWP mode (doesn't fix hotkeys yet)
	DisableImmersiveSearch(); // Remove Windows 10+ immersive search menu for UWP mode
	DisableTaskView(); // Remove Windows 10+ virtual desktops functionality for UWP mode
	DisableWin11AltTab(); // Disable XAML UI because it crashes (Win+Tab will still need to separately be accounted for on Cobalt and possibly Nickel. M3?)
	FixWin11SearchIcon(); // Prevents search icon from being mangled by a buggy tablet mode implementation (cheers Microsoft)
}