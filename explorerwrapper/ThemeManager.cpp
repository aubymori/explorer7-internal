#include "common.h"
#include "ThemeManager.h"
#include "dbgprint.h"
#include "pathcch.h"
#include "OSVersion.h"
#include "RegistryManager.h"

decltype(GetThemeDefaults) GetThemeDefaults = 0;
decltype(LoaderLoadTheme) LoaderLoadTheme = 0;
decltype(OpenThemeDataFromFile) OpenThemeDataFromFile = 0;

UXTHEMEFILE *g_loadedTheme = 0;

void FreeTheme(UXTHEMEFILE* file)
{
	if (file)
	{
		if (file->pbSharableData)
		{
			UnmapViewOfFile(file->pbSharableData);
		}
		if (file->pbNonSharableData)
		{
			UnmapViewOfFile(file->pbNonSharableData);
		}

		CloseHandle(file->hNonSharableSection);
		CloseHandle(file->hSharableSection);

		free(file);
	}
}

DWORD WINAPI DelayFreeThread(LPVOID lParam)
{
	//wait 1 sec
	Sleep(1000);

	FreeTheme((UXTHEMEFILE*)lParam);

	return 0;
}

void ThemeManagerInitialize()
{
	//dont bother error checking, if u dont got uxtheme, ur system is prob already messed up and theres no saving u
	HMODULE hUxTheme = GetModuleHandleW(L"uxtheme.dll");
	GetThemeDefaults = (decltype(GetThemeDefaults))GetProcAddress(hUxTheme, (LPCSTR)7);
	LoaderLoadTheme = (decltype(LoaderLoadTheme))GetProcAddress(hUxTheme, (LPCSTR)92);
	OpenThemeDataFromFile = (decltype(OpenThemeDataFromFile))GetProcAddress(hUxTheme, (LPCSTR)16);

	dbgprintf(L"GetThemeDefaults %x LoaderLoadTheme %x OpenThemeDataFromFile %x\n", GetThemeDefaults, LoaderLoadTheme, OpenThemeDataFromFile);

	// get directory of explorer.exe (NOT the working directory)
	WCHAR szExeDir[MAX_PATH];
	GetModuleFileNameW(NULL, szExeDir, MAX_PATH);
	WCHAR *backslash = StrRChrW(szExeDir, NULL, L'\\');
	if (*backslash == L'\\')
		*backslash = L'\0';

	WCHAR szThemeName[MAX_PATH];
	LSTATUS res = g_registry.QueryValue(L"Theme", (LPBYTE)szThemeName, sizeof(szThemeName));
	if (!*szThemeName || ERROR_SUCCESS != res)
		StringCchCopyW(szThemeName, MAX_PATH, L"aero");

	dbgprintf(L"theme name: %s", szThemeName);

	WCHAR szThemePath[MAX_PATH * 2];
	wsprintfW(
		szThemePath,
		L"%s\\theme\\%s.msstyles",
		szExeDir,
		szThemeName
	);

	dbgprintf(L"theme path: %s", szThemePath);

	auto hr = LoadThemeFile(szThemePath);
	if (hr != S_OK)
		dbgprintf(L"LOADTHEMEFILE FAILED %x\n", hr);
}

HRESULT LoadThemeFile(wchar_t *Path)
{
	HRESULT hr = S_OK;

	if (g_loadedTheme)
	{
		//create delay free thread
		//CreateThread(0,0, DelayFreeThread,g_loadedTheme,0,0);
		FreeTheme(g_loadedTheme);
		g_loadedTheme = 0;
	}

	g_loadedTheme = (UXTHEMEFILE *)malloc(sizeof(UXTHEMEFILE));
	ZeroMemory(g_loadedTheme, sizeof(UXTHEMEFILE));

	WCHAR szColor[MAX_PATH];
	WCHAR szSize[MAX_PATH];

	hr = GetThemeDefaults(
		Path,
		szColor,
		ARRAYSIZE(szColor),
		szSize,
		ARRAYSIZE(szSize)
	);
	if (hr != S_OK)
	{
		if (g_loadedTheme)
		{
			if (g_loadedTheme->pbSharableData)
			{
				UnmapViewOfFile(g_loadedTheme->pbSharableData);
			}
			if (g_loadedTheme->pbNonSharableData)
			{
				UnmapViewOfFile(g_loadedTheme->pbNonSharableData);
			}
			CloseHandle(g_loadedTheme->hNonSharableSection);
			CloseHandle(g_loadedTheme->hSharableSection);
			//free(g_loadedTheme);
			g_loadedTheme = 0;
			dbgprintf(L"LoadTHemeFile failed 1");
		}
		return hr;
	}

	HANDLE hSharable, hNonSharable;
	if (g_osVersion.BuildNumber() < 20000
		? LoaderLoadTheme(0LL, 0LL, Path, szColor, szSize, &hSharable, 0LL, 0, &hNonSharable, 0LL, 0, 0LL, 0LL, 0, 0, 0)
		: ((LoaderLoadTheme_t_win11)LoaderLoadTheme)(
			0LL,
			0LL,
			Path,
			szColor,
			szSize,
			&hSharable,
			0LL,
			0,
			&hNonSharable,
			0LL,
			0,
			0LL,
			0LL,
			0,
			0))
	{
		if (g_loadedTheme)
		{
			if (g_loadedTheme->pbSharableData)
			{
				UnmapViewOfFile(g_loadedTheme->pbSharableData);
			}
			if (g_loadedTheme->pbNonSharableData)
			{
				UnmapViewOfFile(g_loadedTheme->pbNonSharableData);
			}
			CloseHandle(g_loadedTheme->hNonSharableSection);
			CloseHandle(g_loadedTheme->hSharableSection);
			//free(g_loadedTheme);
			g_loadedTheme = 0;
			dbgprintf(L"LoadTHemeFile failed 2");
		}
		return hr;
	}

	memcpy(g_loadedTheme->szHead, "thmfile", ARRAYSIZE(g_loadedTheme->szHead));
	memcpy(g_loadedTheme->szTail, "end", ARRAYSIZE(g_loadedTheme->szTail));
	g_loadedTheme->pbSharableData = (BYTE*)MapViewOfFile(hSharable, FILE_MAP_READ, 0, 0, 0);
	g_loadedTheme->hSharableSection = hSharable;
	g_loadedTheme->pbNonSharableData = (BYTE*)MapViewOfFile(hNonSharable, FILE_MAP_READ, 0, 0, 0);
	g_loadedTheme->hNonSharableSection = hNonSharable;

	return S_OK;
}
