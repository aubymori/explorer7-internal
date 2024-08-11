#include <Windows.h>
#include <Shlwapi.h>
#include "autoplay.h"
#include "dbgprint.h"
#include <ShlObj.h>

typedef PVOID (WINAPI *ResolveDelayLoadedAPIAPI)(PVOID ParentModuleBase, PVOID DelayloadDescriptor, PVOID FailureDllHook, PVOID FailureSystemHook,PIMAGE_THUNK_DATA ThunkAddress,ULONG Flags);
static ResolveDelayLoadedAPIAPI ResolveDelayLoadedAPI;
static PVOID CoCreateInstanceBase;
static PVOID SHGetValueWSHCore;

//remove pintostart verb
LSTATUS WINAPI SHGetValueNEW(
  _In_         HKEY hkey,
  _In_opt_     LPCWSTR pszSubKey,
  _In_opt_     LPCWSTR pszValue,
  _Out_opt_    LPDWORD pdwType,
  _Out_opt_    LPVOID pvData,
  _Inout_opt_  LPDWORD pcbData
)
{	
	WCHAR buf[100];
	DWORD bufsize = sizeof(buf);
	if ( lstrcmp(pszValue,L"LegacyDisable") == 0 )
	if ( (RegQueryValueEx(hkey,L"MUIVerb",NULL,NULL,(LPBYTE)buf,&bufsize) == ERROR_SUCCESS) && (lstrcmpi(buf,L"@shell32.dll,-51201") == 0) ) return ERROR_SUCCESS;
	return SHGetValueW(hkey,pszSubKey,pszValue,pdwType,pvData,pcbData);
}

__int64(__fastcall* SHAboutInfoWorker)(unsigned __int16*, unsigned int);
__int64 __fastcall SHAboutInfoWNEW(unsigned __int16* a1, unsigned int a2)
{
	dbgprintf(L"SHAboutInfoWorker\n");
	return SHAboutInfoWorker(a1, a2);
}

bool(__fastcall* IsSearchEnabled)();
extern "C" bool WINAPI IsSearchEnabledNEW()
{
	//dbgprintf(L"IsSearchEnabledNEW\n");
	return 1;
}

PVOID WINAPI ResolveDelayLoadedAPINEW(PVOID ParentModuleBase, PVOID DelayloadDescriptor, PVOID FailureDllHook, PVOID FailureSystemHook,PIMAGE_THUNK_DATA ThunkAddress,ULONG Flags)
{
	dbgprintf(L"ResolveDelayLoadedAPINEW\n");
	PVOID retfunc = ResolveDelayLoadedAPI(ParentModuleBase,DelayloadDescriptor,FailureDllHook,FailureSystemHook,ThunkAddress,Flags);
	if (retfunc == CoCreateInstanceBase)
	{
		retfunc = Shell32_CoCreateInstance;
		ThunkAddress->u1.Function = (DWORD_PTR)retfunc;
	}
	if (retfunc == SHGetValueWSHCore)
	{
		retfunc = SHGetValueNEW;
		ThunkAddress->u1.Function = (DWORD_PTR)retfunc;
	}
	return retfunc;
}

BOOL __stdcall ILIsEqualNEW(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
	dbgprintf(L"ILIsEqualNEW\n");
	IShellFolder* ppshf = 0;
	HRESULT v4 = SHGetDesktopFolder(&ppshf);
	if (v4 >= 0)
	{
		v4 = ppshf->CompareIDs(0x10000000i64, pidl1, pidl2);
		ppshf->Release();
	}
	return v4 == 0;
}

HRESULT __stdcall SHEvaluateSystemCommandTemplateNEW(PCWSTR pszCmdTemplate, PWSTR* ppszApplication, PWSTR* ppszCommandLine, PWSTR* ppszParameters)
{
	dbgprintf(L"SHEvaluateSystemCommandTemplateNEW\n");
	return S_OK;
	//return SHEvaluateSystemCommandTemplateWithOptions((unsigned __int16*)pszCmdTemplate, ppszParameters);
}

void HookShell32()
{
	dbgprintf(L"1\n");
	ResolveDelayLoadedAPI = (ResolveDelayLoadedAPIAPI)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"ResolveDelayLoadedAPI");
	ChangeImportedAddress(GetModuleHandle(L"shell32.dll"), "API-MS-WIN-CORE-DELAYLOAD-L1-1-1.DLL", ResolveDelayLoadedAPI, ResolveDelayLoadedAPINEW);
	//ResolveDelayLoadedAPI = (ResolveDelayLoadedAPIAPI)GetProcAddress(GetModuleHandle(L"api-ms-win-core-delayload-l1-1-1.dll"),"ResolveDelayLoadedAPI");
	dbgprintf(L"%i\n",(unsigned long long)ResolveDelayLoadedAPI);
	dbgprintf(L"2\n");
	CoCreateInstanceBase = GetProcAddress(GetModuleHandle(L"combase.dll"),"CoCreateInstance");
	dbgprintf(L"3\n");

	SHGetValueWSHCore = GetProcAddress(LoadLibrary(L"shcore.dll"),"SHGetValueW");

	dbgprintf(L"4\n");
	SHAboutInfoWorker = (decltype(SHAboutInfoWorker))GetProcAddress(LoadLibrary(L"shlwapi.dll"),"SHAboutInfoWorker");
	dbgprintf(L"5\n");
	//auto ordinal902 = GetProcAddress(LoadLibrary(L"shell32.dll"),(LPSTR)902);
	ChangeImportedAddress(LoadLibrary(L"shell32.dll"),"shlwapi.DLL", GetProcAddress(LoadLibrary(L"shlwapi.dll"), "SHAboutInfo"), SHAboutInfoWNEW);
	ChangeImportedAddress(GetModuleHandle(0),"shell32.DLL", GetProcAddress(LoadLibrary(L"shell32.DLL"), (LPSTR)902), IsSearchEnabledNEW);
	ChangeImportedAddress(GetModuleHandle(0),"shell32.DLL", GetProcAddress(LoadLibrary(L"shell32.DLL"), "ILIsEqual"), ILIsEqualNEW);
	ChangeImportedAddress(GetModuleHandle(0),"shell32.DLL", GetProcAddress(LoadLibrary(L"shell32.DLL"), "SHEvaluateSystemCommandTemplate"), SHEvaluateSystemCommandTemplateNEW);
	//ChangeExportedAddress_ORDINAL(GetModuleHandle(L"shell32.DLL"), 902, "wrp64.#699");
	dbgprintf(L"6\n");

	//auto ordinal161 = GetProcAddress(LoadLibrary(L"shell32.dll"), (LPSTR)161);
	//ChangeImportedAddress(GetModuleHandle(L"shell32.dll"), "shlwapi.DLL", ordinal161, SHAboutInfoWNEW);
}