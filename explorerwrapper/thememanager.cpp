#include "thememanager.h"
#include "dbgprint.h"
#include "pathcch.h"
#include "version.h"
//#include "registry.h"
#include <Shlwapi.h>

decltype(GetThemeDefaults) GetThemeDefaults = 0;
decltype(LoaderLoadTheme) LoaderLoadTheme = 0;
decltype(OpenThemeDataFromFile) OpenThemeDataFromFile = 0;

UXTHEMEFILE* g_loadedTheme = 0;

void ThemeManagerInitialize()
{
	//dont bother error checking, if u dont got uxtheme, ur shit is prob already fucked and theres no saving u
	HMODULE hUxTheme = GetModuleHandleW(L"uxtheme.dll"); 
	GetThemeDefaults = (decltype(GetThemeDefaults))GetProcAddress(hUxTheme, (LPCSTR)7);
	LoaderLoadTheme = (decltype(LoaderLoadTheme))GetProcAddress(hUxTheme, (LPCSTR)92);
	OpenThemeDataFromFile = (decltype(OpenThemeDataFromFile))GetProcAddress(hUxTheme, (LPCSTR)16);

	dbgprintf(L"GetThemeDefaults %x LoaderLoadTheme %x OpenThemeDataFromFile %x\n",GetThemeDefaults, LoaderLoadTheme, OpenThemeDataFromFile);

	WCHAR CurrentDir[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, CurrentDir);

	WCHAR themePath[MAX_PATH];
	PathCombineW(themePath, CurrentDir, L"theme\\aero\\aero.msstyles");
	dbgprintf(L"themePath: %s", themePath);

	//LSTATUS res = g_registry.QueryValue(L"Theme", (LPBYTE)themePath, sizeof(themePath));
	//dbgprintf(L"result: 0x%X, themePath: %s", res, themePath);
	
	auto hr = LoadThemeFile(themePath);
	if (hr != S_OK)
		dbgprintf(L"LOADTHEMEFILE FAILED %x\n",hr);
}

HRESULT LoadThemeFile(wchar_t* Path)
{
	HRESULT hr = S_OK;

	if (g_loadedTheme)
	{
		if (g_loadedTheme->sharableSectionView)
		{
			UnmapViewOfFile(g_loadedTheme->sharableSectionView);
		}
		if (g_loadedTheme->nsSectionView)
		{
			UnmapViewOfFile(g_loadedTheme->nsSectionView);
		}
		free(g_loadedTheme);
		g_loadedTheme = 0;
	}

	g_loadedTheme = (UXTHEMEFILE*)malloc(sizeof(UXTHEMEFILE));
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
			if (g_loadedTheme->sharableSectionView)
			{
				UnmapViewOfFile(g_loadedTheme->sharableSectionView);
			}
			if (g_loadedTheme->nsSectionView)
			{
				UnmapViewOfFile(g_loadedTheme->nsSectionView);
			}
			free(g_loadedTheme);
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
			if (g_loadedTheme->sharableSectionView)
			{
				UnmapViewOfFile(g_loadedTheme->sharableSectionView);
			}
			if (g_loadedTheme->nsSectionView)
			{
				UnmapViewOfFile(g_loadedTheme->nsSectionView);
			}
			free(g_loadedTheme);
			g_loadedTheme = 0;
			dbgprintf(L"LoadTHemeFile failed 2");
		}
		return hr;
	}

	memcpy(g_loadedTheme->header, "thmfile", 7);
	memcpy(g_loadedTheme->end, "end", 3);
	g_loadedTheme->sharableSectionView = MapViewOfFile(hSharable, 4, 0, 0, 0);
	g_loadedTheme->hSharableSection = hSharable;
	g_loadedTheme->nsSectionView = MapViewOfFile(hNonSharable, 4, 0, 0, 0);
	g_loadedTheme->hNsSection = hNonSharable;

	return S_OK;
}
