#include "thememanager.h"
#include "dbgprint.h"
#include "pathcch.h"
#include <Shlwapi.h>

decltype(GetThemeDefaults) GetThemeDefaults = 0;
decltype(LoaderLoadTheme) LoaderLoadTheme = 0;
decltype(OpenThemeDataFromFile) OpenThemeDataFromFile = 0;

CUxThemeFile* LoadedFile = 0;

void ThemeManagerInitialize()
{
	//dont bother error checking, if u dont got uxtheme, ur shit is prob already fucked and theres no saving u
	HMODULE hUxTheme = GetModuleHandleW(L"uxtheme.dll"); 
	GetThemeDefaults = (decltype(GetThemeDefaults))GetProcAddress(hUxTheme, (LPCSTR)7);
	LoaderLoadTheme = (decltype(LoaderLoadTheme))GetProcAddress(hUxTheme, (LPCSTR)92);
	OpenThemeDataFromFile = (decltype(OpenThemeDataFromFile))GetProcAddress(hUxTheme, (LPCSTR)16);

	dbgprintf(L"GetThemeDefaults %x LoaderLoadTheme %x OpenThemeDataFromFile %x\n",GetThemeDefaults, LoaderLoadTheme, OpenThemeDataFromFile);
	
	WCHAR CurrentDir[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH,CurrentDir);

	WCHAR themePath[MAX_PATH];
	PathCombineW(themePath,CurrentDir,L"theme\\aero\\aero.msstyles");
	dbgprintf(L"themePath: %s", themePath);
	
	auto hr = LoadThemeFile(themePath);
	if (hr != S_OK)
		dbgprintf(L"LOADTHEMEFILE FAILED %x\n",hr);
}

HRESULT LoadThemeFile(wchar_t* Path)
{
	HRESULT hr = S_OK;

	if (LoadedFile)
	{
		if (LoadedFile->sharableSectionView)
		{
			UnmapViewOfFile(LoadedFile->sharableSectionView);
		}
		if (LoadedFile->nsSectionView)
		{
			UnmapViewOfFile(LoadedFile->nsSectionView);
		}
		free(LoadedFile);
		LoadedFile = 0;
	}

	LoadedFile = (CUxThemeFile*)malloc(sizeof(CUxThemeFile));
	ZeroMemory(LoadedFile, sizeof(CUxThemeFile));

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
		if (LoadedFile)
		{
			if (LoadedFile->sharableSectionView)
			{
				UnmapViewOfFile(LoadedFile->sharableSectionView);
			}
			if (LoadedFile->nsSectionView)
			{
				UnmapViewOfFile(LoadedFile->nsSectionView);
			}
			free(LoadedFile);
			LoadedFile = 0;
			dbgprintf(L"LoadTHemeFile failed 1");
		}
		return hr;
	}

	HANDLE hSharable, hNonSharable;

	_OSVERSIONINFOW VersionInformation;

	//LoaderLoad(0LL, 0LL, skinpath, v13, v12, &hFileMappingObject, 0LL, 0, &v16, 0LL, 0, 0LL, 0LL, 0, 0, 0)
	VersionInformation.dwOSVersionInfoSize = 276;
	memset(&VersionInformation.dwMajorVersion, 0, 0x110uLL);
	GetVersionExW(&VersionInformation);
	if (VersionInformation.dwBuildNumber < 20000
		? LoaderLoadTheme(0LL, 0LL, Path, szColor, szSize, &hSharable, 0LL, 0, &hNonSharable, 0LL, 0, 0LL, 0LL, 0, 0, 0)
		: (unsigned int)LoaderLoadTheme(
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
			0,
			0))
	{
		if (LoadedFile)
		{
			if (LoadedFile->sharableSectionView)
			{
				UnmapViewOfFile(LoadedFile->sharableSectionView);
			}
			if (LoadedFile->nsSectionView)
			{
				UnmapViewOfFile(LoadedFile->nsSectionView);
			}
			free(LoadedFile);
			LoadedFile = 0;
			dbgprintf(L"LoadTHemeFile failed 2");
		}
		return hr;
	}

	memcpy(LoadedFile->header, "thmfile", 7);
	memcpy(LoadedFile->end, "end", 3);
	LoadedFile->sharableSectionView = MapViewOfFile(hSharable, 4, 0, 0, 0);
	LoadedFile->hSharableSection = hSharable;
	LoadedFile->nsSectionView = MapViewOfFile(hNonSharable, 4, 0, 0, 0);
	LoadedFile->hNsSection = hNonSharable;

	return S_OK;
}
