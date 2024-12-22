#include "OptionConfig.h"

// Ittr: Migrated all configuration here to make things clearer in dllmain

// Individual option definitions
// - To create a new definition, you must define it here and in OptionConfig.h
bool s_ClassicTheme;
bool s_DisableComposition;
bool s_EnableImmersiveShellStack;
bool s_EnableUWPAppsInStart;
bool s_RPEnabled;
int s_AcrylicAlt;
int s_ColorizationOptions;
bool s_OverrideAlpha;
DWORD s_AlphaValue;

// This is called at the beginning of the library's execution
// The format for each setting is generally:
// - DWORD init with default value
// - Applicable registry value is queried 
// - Option definition gets set:
//	-> If the value exists in the registry, it will be set
//	-> Otherwise, the default value specified with the DWORD init is set
void InitializeConfiguration()
{
	// Immersive shell stack for modern apps (e.g. PC settings)
	// - Defaults to disabled (0)
	// - Pending stability improvements before default enablement
	DWORD dwEnableUWP = 0;
	g_registry.QueryValue(L"EnableImmersive", (LPBYTE)&dwEnableUWP, sizeof(DWORD));
	s_EnableImmersiveShellStack = dwEnableUWP;
	
	// Enable modern apps in start menu programs list
	// - Defaults to enabled (1)
	// - Only applies on RS1 onwards, TH2 and earlier use the native program list
	// - When disabled, the behaviour is similar to 8.1 and earlier
	DWORD dwEnableUWPAppsInStart = 1;
	g_registry.QueryValue(L"EnableUWPAppsInStart", (LPBYTE)&dwEnableUWPAppsInStart, sizeof(DWORD));
	s_EnableUWPAppsInStart = dwEnableUWPAppsInStart;

	// Disable composition effects (e.g. Aero glass)
	// - Defaults to disabled (0)
	DWORD dwDisableComposition = 0;
	g_registry.QueryValue(L"DisableComposition", (LPBYTE)&dwDisableComposition, sizeof(DWORD));
	s_DisableComposition = (dwDisableComposition != 0);

	// Disable themes (e.g. aero.msstyles)
	// - Defaults to disabled (0)
	DWORD dwClassicTheme = 0;
	g_registry.QueryValue(L"ClassicTheme", (LPBYTE)&dwClassicTheme, sizeof(DWORD));
	if (dwClassicTheme != 0)
	{
		// What we do here:
		// 1) A debug string is outputted
		// 2) We indicate we want to disable composition first
		// 3) Then, indicate we want to disable theming
		// 4) Actually disable themes by setting the theme properties of explorer to NULL
		dbgprintf(L"Disabling themes...");
		s_DisableComposition = true;
		s_ClassicTheme = true;
		SetThemeAppProperties(NULL);
	}

	// Composited colorization options
	// - In this case we default to Translucent (1)
	// - This is because it is the only mode that works on every supported OS
	DWORD dwColorizationOptions = 1;
	g_registry.QueryValue(L"ColorizationOptions", (LPBYTE)&dwColorizationOptions, sizeof(DWORD));
	if (dwColorizationOptions != 0 && dwColorizationOptions < 5)
	{
		// Some notes on the selection statement:
		// - BlurBehind is broken from Nickel onwards, Acrylic is used instead as an alternative blur
		// - Acrylic is not supported by Win32 API until RS4, so falls back to Translucent
		// - BlurBehind, Acrylic, SolidColor are unsupported on 8.x, so falls back to Translucent
		// - Otherwise, we apply the inputted value as the mode is supported on the OS
		if (dwColorizationOptions == 2 && g_osVersion.BuildNumber() >= 22621)
		{
			s_ColorizationOptions = 3;
		}
		else if (dwColorizationOptions == 3 && g_osVersion.BuildNumber() < 17134)
		{
			s_ColorizationOptions = 1;
		}
		else if (dwColorizationOptions >= 2 && g_osVersion.BuildNumber() < 10074)
		{
			s_ColorizationOptions = 1;
		}
		else
		{
			s_ColorizationOptions = dwColorizationOptions;
		}
	}

	// Composited colorization alpha override
	// - Defaults to disabled (0)
	DWORD dwOverrideAlpha = 0;
	g_registry.QueryValue(L"OverrideAlpha", (LPBYTE)&dwOverrideAlpha, sizeof(DWORD));
	s_OverrideAlpha = dwOverrideAlpha;

	// Composited colorization alpha override value
	// - Defaults to Windows 7 alpha value (0x6B)
	// - Only used when OverrideAlpha = 1
	DWORD dwAlphaValue = 0x6B;
	g_registry.QueryValue(L"AlphaValue", (LPBYTE)&dwAlphaValue, sizeof(DWORD));
	s_AlphaValue = dwAlphaValue;

	// Select appropriate acrylic style to use
	// - Defaults to regular (0)
	// - Only used when ColorizationOptions = 3
	DWORD dwAcrylicAlt = 0;
	g_registry.QueryValue(L"AcrylicColorization", (LPBYTE)&dwAcrylicAlt, sizeof(DWORD));
	s_AcrylicAlt = dwAcrylicAlt;

	// Win8 theme classes
	// - Defaults to disabled (0)
	DWORD dwRPEnabled = 0;
	g_registry.QueryValue(L"RPEnabled", (LPBYTE)&dwRPEnabled, sizeof(DWORD));
	s_RPEnabled = dwRPEnabled;
}
