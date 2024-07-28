#include "startmenupin.h"
#include "dbgprint.h"

CreateInstance_API CreateStartMenuPinInstance;
PSTARTPINVTBL origStartPinVtbl;
PSTARTPINVTBL newStartPinVtbl;

HMODULE h_shell32;

const LPWSTR sz_StartPage2 = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage2";
const LPSTR sz_StartPin = "startpin";
const LPSTR sz_StartUnpin = "startunpin";

int WINAPI Shell32_LoadString( HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax )
{
	int result;
	if ( hInstance == h_shell32 && ( uID == 0x1505 || uID == 0x1506 || uID == 0x1508 || uID == 0x1509 ) )
	{
		//try loading shell32.dll.mui
		WCHAR locales[100];
		ULONG clangs;
		ULONG cblocales = 100;
		GetUserPreferredUILanguages(MUI_LANGUAGE_NAME,&clangs,locales,&cblocales);
		WCHAR muipath[MAX_PATH];
		GetModuleFileName(NULL,muipath,MAX_PATH);
		PathRemoveFileSpec(muipath);
		PathAddBackslash(muipath);
		PathAppend(muipath,locales);
		PathAddBackslash(muipath);
		PathAppend(muipath,L"shell32.dll.mui");		
		hInstance = LoadLibraryEx(muipath,0,LOAD_LIBRARY_AS_DATAFILE);
		int result = LoadStringW(hInstance,uID,lpBuffer,nBufferMax);
		FreeLibrary(hInstance);
		if (result == 0) //fallback - load from us
			result = LoadStringW(g_hInstance,uID,lpBuffer,nBufferMax);
	}
	else
		result = LoadStringW(hInstance,uID,lpBuffer,nBufferMax);
	return result;
}

static LRESULT RegGetDWORD(HKEY key, LPWSTR subkey, LPWSTR value, DWORD* dwVal)
{
	DWORD sz = 4;
	return SHRegGetValueW(key,subkey,value,SRRF_RT_REG_DWORD,NULL,dwVal,&sz);
}

static LRESULT RegSetDWORD(HKEY key, LPWSTR subkey, LPWSTR value, DWORD* dwVal)
{
	return SHSetValueW(key,subkey,value,REG_DWORD,dwVal,4);
}

void CStartMenuPin::QueryInterface(){};
void CStartMenuPin::AddRef(){};
void CStartMenuPin::Release(){};
void CStartMenuPin::Initialize(){};
void CStartMenuPin::NotifyPinListChange(){};
void CStartMenuPin::Unimpl1(){};
void CStartMenuPin::UpgradeItem(){};
void CStartMenuPin::IsAcceptableTarget(){};
void CStartMenuPin::Unimpl2(){};
void CStartMenuPin::SendPinRearrangeSQM(){};
void CStartMenuPin::GetPinnedAppSQMEventID(){};

LRESULT __thiscall CStartMenuPin::SetChangeCount(DWORD value)
{
	return RegSetDWORD(HKEY_CURRENT_USER,sz_StartPage2,L"FavoritesChanges",&value);
}

IStream* __thiscall CStartMenuPin::OpenPinRegStream(DWORD grfMode)
{
	return SHOpenRegStream2W(HKEY_CURRENT_USER,sz_StartPage2,L"Favorites",grfMode);
}

IStream* __thiscall CStartMenuPin::OpenLinksRegStream(DWORD grfMode)
{
	return SHOpenRegStream2W(HKEY_CURRENT_USER,sz_StartPage2,L"FavoritesResolve",grfMode);
}

DWORD __thiscall CStartMenuPin::GetPinStreamVersion()
{
	DWORD value = 0;
	RegGetDWORD(HKEY_CURRENT_USER,sz_StartPage2,L"FavoritesVersion",&value);
	return value;
}

LRESULT __thiscall CStartMenuPin::SetPinStreamVersion(DWORD value)
{
	return RegSetDWORD(HKEY_CURRENT_USER,sz_StartPage2,L"FavoritesVersion",&value);
}

HRESULT __thiscall CStartMenuPin::GetBackupSubDirName(LPWSTR szOut, int cbLen)
{
	lstrcpyn(szOut,L"StartMenu",cbLen);
	return S_OK; //...right?
}

DWORD __thiscall CStartMenuPin::IsRestricted()
{
	return SHRestricted(REST_NOSMPINNEDLIST);
}

HRESULT __thiscall CStartMenuPin::GetMenuStringID(DWORD* w)
{
	origStartPinVtbl->GetMenuStringID(this,w);
	(*w)-=5;
	return S_OK;
}

int __thiscall CStartMenuPin::GetHelpText(int id, LPWSTR buf, int nCharMax)
{	
	return Shell32_LoadString(h_shell32,id+0x1508,buf,nCharMax);	
}

LPSTR __thiscall CStartMenuPin::GetVerb(int op)
{
	if (op == 0) return sz_StartPin;
	if (op == 1) return sz_StartUnpin;
	return NULL;
}

LRESULT __thiscall CStartMenuPin::GetChangeCount(DWORD* pdwVal)
{
	*pdwVal = 0;
	return RegGetDWORD(HKEY_CURRENT_USER,sz_StartPage2,L"FavoritesChanges",pdwVal);
}

DWORD __thiscall CStartMenuPin::GetRemovedChangeCount()
{
	DWORD value = 0;
	RegGetDWORD(HKEY_CURRENT_USER,sz_StartPage2,L"FavoritesRemovedChanges",&value);
	return value;
}

LRESULT __thiscall CStartMenuPin::SetRemovedChangeCount(DWORD value)
{
	return RegSetDWORD(HKEY_CURRENT_USER,sz_StartPage2,L"FavoritesRemovedChanges",&value);
}

#pragma function(memcpy)
HRESULT WINAPI NewCreateStartMenuPinInstance(PVOID dummy,REFIID riid,PVOID* ppv)
{
	WCHAR iid[40];
	StringFromGUID2(riid,iid,40);
	IUnknown* pinobj;
	HRESULT rslt = CreateStartMenuPinInstance(dummy,IID_IShellExtInit,(PVOID*)&pinobj);
	if ( SUCCEEDED(rslt) )
	{
		PSTARTPINOBJ startobj = (PSTARTPINOBJ)pinobj;
		dbgprintf(L"CreateStartMenuPin pStartPinVtbl %p",startobj->pStartPinVtbl);
		//init vtbl hijack
		if (!newStartPinVtbl)
		{
			CStartMenuPin* stmenu = new CStartMenuPin;
			PSTARTPINOBJ ourobj = (PSTARTPINOBJ)static_cast<IStartMenuShellExtInit*>(stmenu);
			PSTARTPINVTBL ourvtbl = ourobj->pStartPinVtbl;
			delete stmenu;

			newStartPinVtbl = (PSTARTPINVTBL)malloc(sizeof(STARTPINVTBL));
			memcpy(newStartPinVtbl,startobj->pStartPinVtbl,sizeof(STARTPINVTBL));
			newStartPinVtbl->SetChangeCount = ourvtbl->SetChangeCount;
			newStartPinVtbl->OpenPinRegStream = ourvtbl->OpenPinRegStream;
			newStartPinVtbl->OpenLinksRegStream = ourvtbl->OpenLinksRegStream;
			newStartPinVtbl->GetPinStreamVersion = ourvtbl->GetPinStreamVersion;
			newStartPinVtbl->SetPinStreamVersion = ourvtbl->SetPinStreamVersion;
			newStartPinVtbl->GetBackupSubDirName = ourvtbl->GetBackupSubDirName;
			newStartPinVtbl->IsRestricted = ourvtbl->IsRestricted;
			newStartPinVtbl->GetChangeCount = ourvtbl->GetChangeCount;
			newStartPinVtbl->SetRemovedChangeCount = ourvtbl->SetRemovedChangeCount;
			newStartPinVtbl->GetRemovedChangeCount = ourvtbl->GetRemovedChangeCount;		
			newStartPinVtbl->GetVerb = ourvtbl->GetVerb;
			newStartPinVtbl->GetMenuStringID = ourvtbl->GetMenuStringID;
			newStartPinVtbl->GetHelpText = ourvtbl->GetHelpText;

			origStartPinVtbl = startobj->pStartPinVtbl;
		}
		startobj->pStartPinVtbl = newStartPinVtbl;
		//return asked interface
		rslt = pinobj->QueryInterface(riid,ppv);
		pinobj->Release();
	}
	return rslt;
}

void StartMenuPin_PatchShell32() //x32 only!!!
{	
	h_shell32 = GetModuleHandle(L"shell32.dll");
	ChangeImportedAddress(h_shell32,"api-ms-win-core-libraryloader-l1-2-0.dll",GetProcAddress(GetModuleHandle(L"kernelbase.dll"),"LoadStringW"),Shell32_LoadString);

	DWORD_PTR addr = FindPattern((uintptr_t)h_shell32,"48 85 C0 0F 85 ?? ?? ?? ?? 45 8B C5 4C 8D 15 ?? ?? ?? ??");
	if (addr)
		addr += 15;
	else
	{
		addr = FindPattern((uintptr_t)h_shell32,"41 8B FD 48 8D 1D ?? ?? ?? ?? 4C 8D 3D");
		if (addr)
			addr += 12;
		else
		{
			dbgprintf(L"StartMenuPin_PatchShell32 SIG DID NOT WORK!!!\n");
			dbgprintf(L"StartMenuPin_PatchShell32 SIG DID NOT WORK!!!\n");
			dbgprintf(L"StartMenuPin_PatchShell32 SIG DID NOT WORK!!!\n");
			return; 
		}
	}
	//DWORD_PTR addr = (DWORD_PTR)GetProcAddress(h_shell32,"DllGetClassObject") + 0x85;
	addr = addr + 4 + *(DWORD*)addr;
	PSHELLGUIDS table = (PSHELLGUIDS)addr;

	dbgprintf(L"Got table at %p",table);
	while ( &table->rclsid )
	{
		if ( table->rclsid == CLSID_StartMenuPin )
		{
			DWORD old;
			VirtualProtect(table,sizeof(SHELLGUIDS),PAGE_EXECUTE_READWRITE,&old);
			CreateStartMenuPinInstance = table->CreateFunc;
			dbgprintf(L"CreateStartMenuPinInstance = %p",CreateStartMenuPinInstance);
			table->CreateFunc = NewCreateStartMenuPinInstance;
			break;
		}
		table++;
	}
}

