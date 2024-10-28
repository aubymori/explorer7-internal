#include "startmenuresolver.h"
#include "startmenupin.h"
#include "dbgprint.h"
#include "pinnedlist.h"
#include <cassert>
#include <dpa_dsa.h>
#include <shlguid.h>
#include "shell32_wrappers.h"
#include "shellitemfilter.h"
#include "augmentedshellfolder.h"
#include "shell32_wrappers.h"
#include "userassist.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shellapi.h>
#pragma function(memset)

extern "C" HRESULT WINAPI Explorer_CoCreateInstance(
	__in   REFCLSID rclsid,
	__in   LPUNKNOWN pUnkOuter,
	__in   DWORD dwClsContext,
	__in   REFIID riid,
	__out  LPVOID* ppv
);
#define GUIDSTR_MAX 38
STDAPI_(HINSTANCE) SHPinDllOfCLSID(const CLSID* pclsid)
{
	HKEY hk;
	DWORD dwSize;
	HINSTANCE hinst = NULL;
	TCHAR szClass[GUIDSTR_MAX + 64];    // CLSID\{...}\InProcServer32
	WCHAR szDllPath[MAX_PATH];

	lstrcpy(szClass, TEXT("CLSID\\"));
	StringFromCLSID(*pclsid, (LPOLESTR*)szClass + 6); // 6 = strlen("CLSID\\")
	lstrcat(szClass, TEXT("\\InProcServer32"));

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szClass, 0, KEY_QUERY_VALUE, &hk)
		== ERROR_SUCCESS) {

		// Explicitly read as unicode.  SHQueryValueEx handles REG_EXPAND_SZ
		dwSize = sizeof(szDllPath);
		if (SHQueryValueExW(hk, 0, 0, 0, szDllPath, &dwSize) == ERROR_SUCCESS) {
			hinst = LoadLibraryExW(szDllPath, NULL, 0);
		}

		RegCloseKey(hk);
	}

	return hinst;
}

IUserAssist* g_uempUa;      // 0:uninit, -1:failed, o.w.:cached obj
IUserAssist* GetUserAssist()
{
	HRESULT hr;
	IUserAssist* pua = NULL;

	if (g_uempUa == 0)
	{
		// re: CLSCTX_NO_CODE_DOWNLOAD
		// an ('impossible') failed CCI of UserAssist is horrendously slow.
		// e.g. click on the start menu, wait 10 seconds before it pops up.
		// we'd rather fail than hose perf like this, plus this class should
		// never be remote.
		// FEATURE: there must be a better way to tell if CLSCTX_NO_CODE_DOWNLOAD
		// is supported, i've sent mail to 'com' to find out...
		DWORD dwFlags = (CLSCTX_INPROC | CLSCTX_NO_CODE_DOWNLOAD);
		hr = Explorer_CoCreateInstance(CLSID_UserAssist, NULL, dwFlags, IID_IUserAssist7, (void**)&pua);
		assert(SUCCEEDED(hr) || pua == NULL);  // follow COM rules

		if (pua)
		{
			HINSTANCE hInst;

			hInst = SHPinDllOfCLSID(&CLSID_UserAssist); // cached across threads
			// we're toast if this fails!!! (but happily, that's 'impossible')
			// e.g. during logon when grpconv.exe is ShellExec'ed, we do
			// a GetUserAssist, which caches a ptr to browseui's singleton
			// object.  then when the ShellExec returns, we do CoUninit,
			// which would free up the (non-pinned) browseui.dll.  then
			// a later use of the cache would go off into space.
		}

		//ENTERCRITICAL;
		if (g_uempUa == 0) {
			g_uempUa = pua;     // xfer refcnt (if any)
			if (!pua) {
				// mark it failed so we won't try any more
				g_uempUa = (IUserAssist*)-1;
			}
			pua = NULL;
		}
		//LEAVECRITICAL;
		if (pua)
			pua->Release();
		//TraceMsg(DM_UASSIST, "sl.gua: pua=0x%x g_uempUa=%x", pua, g_uempUa);
	}

	return (g_uempUa == (IUserAssist*)-1) ? 0 : g_uempUa;
}

extern "C"
BOOL UEMIsLoaded()
{
	BOOL fRet;

	fRet = GetModuleHandle(TEXT("ole32.dll")) &&
		GetModuleHandle(TEXT("browseui.dll"));

	return fRet;
}

//***   UEMFireEvent, QueryEvent, SetEvent -- 'safe' thunks
// DESCRIPTION
//  call these so don't have to worry about cache or whether Uassist object
// even was successfully created.
//REFIID guid, PVOID wparam, LPWSTR lparam, int eCmd
HRESULT UEMFireEvent(const GUID * pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr = E_FAIL;
	IUserAssist* pua;

	pua = GetUserAssist();
	if (pua) {
		hr = pua->FireEvent(*pguidGrp, eCmd, wParam, lParam);
	}
	return hr;
}

HRESULT UEMSetEvent(const GUID * pguidGrp, WPARAM wParam, UEMINFO* pui)
{
	HRESULT hr = E_FAIL;
	IUserAssist* pua;

	pua = GetUserAssist();
	if (pua) {
		hr = pua->SetEntry(*pguidGrp, wParam, pui);
	}
	return hr;
}

HRESULT UEMQueryEvent(const GUID* pguidGrp, WPARAM wParam, UEMINFO* pui)
{
	HRESULT hr = E_FAIL;
	IUserAssist* pua;

	pua = GetUserAssist();
	if (pua) {
		hr = pua->QueryEntry(*pguidGrp, wParam, pui);
	}
	return hr;
}

#define UEIM_HIT        0x01
#define UEIM_FILETIME   0x02
#define UEM_NEWITEMCOUNT 2

void UEMRenamePidl(const GUID* pguidGrp1, IShellFolder* psf1, LPCITEMIDLIST pidl1,
	const GUID* pguidGrp2, IShellFolder* psf2, LPCITEMIDLIST pidl2)
{
	UEMINFO uei;
	uei.cbSize = sizeof(uei);
	uei.dwMask = UEIM_HIT | UEIM_FILETIME;
	if (SUCCEEDED(UEMQueryEvent(pguidGrp1, (WPARAM)psf1, &uei)) &&
		uei.R > 0)
	{
		UEMSetEvent(pguidGrp2, (WPARAM)psf2,  &uei);

		uei.R = 0;
		UEMSetEvent(pguidGrp1, (WPARAM)psf1, &uei);
	}
}

void UEMDeletePidl(const GUID* pguidGrp, IShellFolder* psf, LPCITEMIDLIST pidl)
{
	UEMINFO uei;
	uei.cbSize = sizeof(uei);
	uei.dwMask = UEIM_HIT;
	uei.R = 0;
	UEMSetEvent(pguidGrp,(WPARAM)psf, &uei);
}

void* WINAPI Alloc(long cb)
{

	return (void*)LocalAlloc(LPTR, cb);

}

BOOL WINAPI Free(void* pb)
{

	return (LocalFree((HLOCAL)pb) == NULL);

}

BOOL WINAPI Str_SetPtr(LPTSTR* ppszCurrent, LPCTSTR pszNew)
{
	int cchLength;
	LPTSTR pszOld;
	LPTSTR pszNewCopy = NULL;

	if (pszNew)
	{
		cchLength = lstrlen(pszNew);

		// alloc a new buffer w/ room for the null terminator
		pszNewCopy = (LPTSTR)Alloc((cchLength + 1) * sizeof(TCHAR));

		if (!pszNewCopy)
			return FALSE;

		lstrcpynW(pszNewCopy, pszNew, cchLength + 1);
	}
	pszOld = *ppszCurrent;
	*ppszCurrent = pszNewCopy;

	//pszOld = (LPTSTR)InterlockedExchangePointer((LPVOID*)ppszCurrent, pszNewCopy);

	if (pszOld)
		Free(pszOld);

	return TRUE;
}

//constructor
CStartMenuResolver::CStartMenuResolver(IAppResolver8* newresolver)
{
	m_cRef = 0; //?
	m_resolver8 = newresolver;
	m_startmenuitemscache8 = nullptr;
	m_startmenuitemscache10 = nullptr;
}

CStartMenuResolver::CStartMenuResolver(IStartMenuItemsCache8 *newcache)
{
	m_cRef = 0;
	m_startmenuitemscache8 = newcache;
	m_startmenuitemscache10 = nullptr;
	CoCreateInstance(
		CLSID_StartMenuCacheAndAppResolver,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IAppResolver8,
		(LPVOID *)&m_resolver8
	);
}

CStartMenuResolver::CStartMenuResolver(IStartMenuItemsCache10 *newcache)
{
	m_cRef = 0;
	m_startmenuitemscache10 = newcache;
	m_startmenuitemscache8 = nullptr;
	CoCreateInstance(
		CLSID_StartMenuCacheAndAppResolver,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IAppResolver8,
		(LPVOID*)&m_resolver8
	);
}

CStartMenuResolver::~CStartMenuResolver()
{
	if (m_resolver8)
		m_resolver8->Release();

	if (m_startmenuitemscache8)
		m_startmenuitemscache8->Release();

	if (m_startmenuitemscache10)
		m_startmenuitemscache10->Release();
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == IID_IAppResolver7)
	{
		//dbgprintf(L"IID_IAppResolver7\n");
		*ppvObject = static_cast<IAppResolver7*>(this);
		AddRef();
		return S_OK;
	}
	if (riid == IID_IStartMenuItemsCache7)
	{
		dbgprintf(L"IID_IStartMenuItemsCache7\n");
		HRESULT ret = E_NOINTERFACE;
		if (m_startmenuitemscache8)
		{
			ret = m_startmenuitemscache8->QueryInterface(IID_IStartMenuItemsCache8, (PVOID *)&m_startmenuitemscache8);
			if (ret == S_OK)
			{
				dbgprintf(L"S_OK\n");
				*ppvObject = static_cast<IStartMenuItemsCache7 *>(this);
				AddRef();
			}
		}
		else if (m_startmenuitemscache10)
		{
			ret = m_startmenuitemscache10->QueryInterface(IID_IStartMenuItemsCache10, (PVOID*)&m_startmenuitemscache10);
			if (ret == S_OK)
			{
				dbgprintf(L"S_OK 2\n");
				*ppvObject = static_cast<IStartMenuItemsCache7 *>(this);
				AddRef();
			}
		}
		return ret;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CStartMenuResolver::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE CStartMenuResolver::Release(void)
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

//IAppResolver7
HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetAppIDForShortcut(IShellItem* p1, LPWSTR* p2)
{
	dbgprintf(L"GetAppIDForShortcut");
	return m_resolver8->GetAppIDForShortcut(p1, p2);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetAppIDForWindow(HWND* p1, DWORD* p2, DWORD* p3, DWORD* p4, DWORD* p5)
{
	dbgprintf(L"GetAppIDForWindow");
	return m_resolver8->GetAppIDForWindow(p1, p2, p3, p4, p5);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetAppIDForProcess(ULONG_PTR p1, DWORD* p2, DWORD* p3, DWORD* p4, DWORD* p5)
{
	dbgprintf(L"GetAppIDForProcess");
	return m_resolver8->GetAppIDForProcess(p1, p2, p3, p4, p5);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetShortcutForProcess(ULONG_PTR p1, IUnknown* p2)
{
	dbgprintf(L"GetShortcutForProcess");
	return m_resolver8->GetShortcutForProcess(p1, p2);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetBestShortcutForAppID(DWORD* p1, IUnknown* p2)
{
	dbgprintf(L"GetBestShortcutForAppID");
	return m_resolver8->GetBestShortcutForAppID(p1, p2);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetBestShortcutAndAppIDForAppPath(DWORD* p1, IUnknown* p2, DWORD* p3)
{
	dbgprintf(L"GetBestShortcutAndAppIDForAppPath");
	return m_resolver8->GetBestShortcutAndAppIDForAppPath(p1, p2, p3);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::CanPinApp(IUnknown* p1)
{
	dbgprintf(L"CanPinApp");
	return m_resolver8->CanPinApp(p1);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetRelaunchProperties(HWND* p1, DWORD* p2, DWORD* p3, DWORD* p4, DWORD* p5, DWORD* p6)
{
	//dbgprintf(L"GetRelaunchProperties");
	return m_resolver8->GetRelaunchProperties(p1, p2, p3, p4, p5, p6, nullptr);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GenerateShortcutFromWindowProperties(HWND* p1, IUnknown* p2)
{
	dbgprintf(L"GenerateShortcutFromWindowProperties");
	return m_resolver8->GenerateShortcutFromWindowProperties(p1, p2);
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GenerateShortcutFromItemProperties(IUnknown* p1, IUnknown* p2)
{
	dbgprintf(L"GenerateShortcutFromItemProperties");
	return m_resolver8->GenerateShortcutFromItemProperties(p1, p2);
}

//IStartMenuItemsCache7
HRESULT STDMETHODCALLTYPE CStartMenuResolver::OnChangeNotify(unsigned int p1, long p2, PVOID* p3, PVOID* p4)
{
	HRESULT rslt;
	if (m_startmenuitemscache8)
		rslt = m_startmenuitemscache8->OnChangeNotify(p1, p2, p3, p4);
	else if (m_startmenuitemscache10)
		rslt = m_startmenuitemscache10->OnChangeNotify(p1, p2, p3, p4);
	dbgprintf(L"CStartMenuResolver::OnChangeNotify %p %p %p %p = %p", p1, p2, p3, p4, rslt);
	return rslt;
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::PinListChanged(void)
{
	dbgprintf(L"CStartMenuResolver::PinListChanged");
	//we need to clear MFU cache, but we don't have one!
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetPinnedItemsCount(int* pCount)
{
	dbgprintf(L"GetPinnedItemsCount");
	*pCount = 0;
	IPinnedList2* pinList2 = 0;
	HRESULT rslt = Explorer_CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER, IID_IPinnedList2, (PVOID*)&pinList2);
	if (SUCCEEDED(rslt))
	{
		IEnumFullIDList* enumidlist;
		rslt = pinList2->EnumObjects(&enumidlist);
		if (SUCCEEDED(rslt))
		{
			LPITEMIDLIST pidl;
			ULONG wat;
			while (enumidlist->Next(1, &pidl, &wat) == S_OK)
			{
				(*pCount)++;
				CoTaskMemFree(pidl);
			}
			dbgprintf(L"CStartMenuResolver::GetPinnedItemsCount = %d", *pCount);
			enumidlist->Release();
		}
		pinList2->Release();
	}
	return rslt;
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetStartMenuMFUList(unsigned int limit, IEnumStartMenuItem** penumStart, IEnumString** penumStrings, FILETIME* pNewFileTime)
{
	unsigned int cnt = 0;
	dbgprintf(L"CStartMenuResolver::GetStartMenuMFUList limit=%p filetime=%X_%X", limit, pNewFileTime->dwHighDateTime, pNewFileTime->dwLowDateTime);
	CEnumStartMenu* startenum = new CEnumStartMenu;
	*penumStart = (IEnumStartMenuItem*)startenum;
	*penumStrings = (IEnumString*)new CEnumStartMenu;
	//add pinned items
	IPinnedList2* startpinnedlist;
	IPinnedList2* taskbarpinnedlist;
	HRESULT rslt = Explorer_CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER, IID_IPinnedList2, (PVOID*)&startpinnedlist);
	if (FAILED(rslt)) return rslt;
	rslt = Explorer_CoCreateInstance(CLSID_TaskbarPin, NULL, CLSCTX_INPROC_SERVER, IID_IPinnedList2, (PVOID*)&taskbarpinnedlist);
	if (FAILED(rslt)) return rslt;
	IEnumFullIDList* enumidlist;
	rslt = startpinnedlist->EnumObjects(&enumidlist);
	if (SUCCEEDED(rslt))
	{
		STARTMENUITEM startitem = { 0 };
		while (enumidlist->Next(1, &startitem.pidlRelative, NULL) == S_OK)
		{
			IShellItem* shellitem;
			rslt = SHCreateItemFromIDList(startitem.pidlRelative, IID_IShellItem, (LPVOID*)&shellitem);
			if (SUCCEEDED(rslt))
			{
				rslt = m_resolver8->GetAppIDForShortcut(shellitem, &startitem.pszAppID);
				if (FAILED(rslt))
				{
					dbgprintf(L"GetAppIDForShortcut failed %p (shortcut broken?!)", rslt);
					rslt = shellitem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &startitem.pszAppID);
				}
				shellitem->Release();
				startitem.iPinPos = cnt;
				startenum->AddItem(&startitem);
				cnt++;
			}
		}
		enumidlist->Release();
	}
	//add separator
	STARTMENUITEM startitem = { 0 };
	startitem.iPinPos = -2;
	startenum->AddItem(&startitem);
	//add MFU items if needed
	if (limit > cnt)
	{
		//from start menu
		IStartMenuAppItems8* startitems;
		if (FAILED(m_resolver8->QueryInterface(IID_IStartMenuAppItems8, (LPVOID*)&startitems))) return S_FALSE;
		IObjectCollection* collection;
		if (FAILED(startitems->EnumItems(0, IID_IObjectCollection, (PVOID*)&collection))) return S_FALSE;
		UINT iLauncherCount = 0;
		UINT iLauncherItem;
		collection->GetCount(&iLauncherCount);
		for (iLauncherItem = 0; iLauncherItem < iLauncherCount; iLauncherItem++)
		{
			IPropertyStore* propstore;
			if (SUCCEEDED(collection->GetAt(iLauncherItem, IID_IPropertyStore, (PVOID*)&propstore)))
			{
				PROPVARIANT pvPidl;
				PROPVARIANT pvAppId;
				PROPVARIANT pvMetro;
				PROPVARIANT pvDual;
				propstore->GetValue(PKEY_AppUserModel_BestShortcut, &pvPidl);
				propstore->GetValue(PKEY_AppUserModel_ID, &pvAppId);
				propstore->GetValue(PKEY_AppUserModel_HostEnvironment, &pvMetro);
				propstore->GetValue(PKEY_AppUserModel_IsDualMode, &pvDual);
				//we're accepting only non-metro or dualmode shortcuts
				if (!pvMetro.intVal || pvDual.intVal)
				{
					STARTMENUITEM startitem = { 0 };
					if (SUCCEEDED(UAQueryShortcut((LPITEMIDLIST)pvPidl.caub.pElems, &startitem.ueminfo)) &&
						startitem.ueminfo.R && !startitem.ueminfo.fExcludeFromMFU)
						if (startpinnedlist->IsPinned((LPITEMIDLIST)pvPidl.caub.pElems) == S_FALSE) //IsPinned checks are VERY slow, at least under VMWare
							if (taskbarpinnedlist->IsPinned((LPITEMIDLIST)pvPidl.caub.pElems) == S_FALSE) //...why?!
							{
								startitem.pidlRelative = ILClone((LPITEMIDLIST)pvPidl.caub.pElems);
								startitem.pszAppID = CoAllocString(pvAppId.bstrVal);
								startitem.iPinPos = -1;
								startenum->AddItem(&startitem);
							}
				}
				propstore->Release();
			}
		}
		collection->Release();
		startitems->Release();
		//from desktop
		LPITEMIDLIST pidlitem;
		IShellFolder* dsf;
		IEnumIDList* enumdesktop;
		SHGetDesktopFolder(&dsf);
		dsf->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FASTITEMS, &enumdesktop);

		while (enumdesktop->Next(1, &pidlitem, NULL) == S_OK)
		{
			SFGAOF attrs = SFGAO_LINK;
			dsf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlitem, &attrs);

			if (attrs & SFGAO_LINK)
			{
				STARTMENUITEM startitem = { 0 };
				SHGetRealIDL(dsf, pidlitem, &startitem.pidlRelative);

				if (SUCCEEDED(UAQueryShortcut(startitem.pidlRelative, &startitem.ueminfo)) &&
					startitem.ueminfo.R && !startitem.ueminfo.fExcludeFromMFU)
					if (taskbarpinnedlist->IsPinned(startitem.pidlRelative) == S_FALSE)
						if (startpinnedlist->IsPinned(startitem.pidlRelative) == S_FALSE)
						{
							IShellItem* shellitem;
							rslt = SHCreateItemFromIDList(startitem.pidlRelative, IID_IShellItem, (LPVOID*)&shellitem);
							if (SUCCEEDED(rslt))
							{
								rslt = m_resolver8->GetAppIDForShortcut(shellitem, &startitem.pszAppID);
								if (FAILED(rslt))
								{
									dbgprintf(L"GetAppIDForShortcut failed %p (shortcut broken?!)", rslt);
									rslt = shellitem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &startitem.pszAppID);
								}
								shellitem->Release();
								startitem.iPinPos = -1;
								startenum->AddItem(&startitem);
							}
						}
			}

			ILFree(pidlitem);
		}
		enumdesktop->Release();
		dsf->Release();
	}
	startpinnedlist->Release();
	taskbarpinnedlist->Release();
	startenum->Sort();
	startenum->SetLimit(limit);
	startenum->RemoveDuplicates();
	GetSystemTimeAsFileTime(pNewFileTime);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::RegisterSMNotify(IUnknown* p1)
{
	dbgprintf(L"RegisterSMNotify");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::RegisterARNotify(IUnknown* p1)
{
	dbgprintf(L"RegisterARNotify");
	if (m_startmenuitemscache8)
		return m_startmenuitemscache8->RegisterARNotify(new CAppResolverNotify8((IAppResolverNotify7*)p1));
	else if (m_startmenuitemscache10)
		return m_startmenuitemscache10->RegisterARNotify(new CAppResolverNotify8((IAppResolverNotify7*)p1));

	return E_ABORT;
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::SetAltName(PVOID* p1, DWORD* p2, PVOID* p3)
{
	dbgprintf(L"CStartMenuResolver::SetAltName %p %p %p", p1, p2, p3);
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStartMenuResolver::GetAltName(PVOID* p1, DWORD* p2)
{
	dbgprintf(L"CStartMenuResolver::GetAltName %p %p", p1, p2);
	return E_NOTIMPL;
}

STDMETHODIMP CStartMenuCallbackBase::QueryInterface(REFIID riid, void** ppvObj)
{
	static const QITAB qit[] =
	{
		QITABENT(CStartMenuCallbackBase, IShellMenuCallback),
		QITABENT(CStartMenuCallbackBase, IObjectWithSite),
		{ 0 },
	};

	return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG __stdcall) CStartMenuCallbackBase::AddRef()
{
	return ++_cRef;
}

STDMETHODIMP_(ULONG __stdcall) CStartMenuCallbackBase::Release()
{
	assert(_cRef > 0);
	_cRef--;

	if (_cRef > 0)
		return _cRef;

	delete this;
	return 0;
}

#define REGSTR_PATH_EXPLORER             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer")
#define REGSTR_EXPLORER_WINUPDATE REGSTR_PATH_EXPLORER TEXT("\\WindowsUpdate")
#define REGSTR_PATH_SETUP                TEXT("Software\\Microsoft\\Windows\\CurrentVersion")

EXTERN_C CRITICAL_SECTION g_csDarwinAds = { 0 };

#define ENTERCRITICAL_DARWINADS EnterCriticalSection(&g_csDarwinAds)
#define LEAVECRITICAL_DARWINADS LeaveCriticalSection(&g_csDarwinAds)

// The threading concern with this variable is create/delete/add/remove. We will only remove an item 
// and delete the hdpa on the main thread. We will however add and create on both threads.
// We need to serialize access to the dpa, so we're going to grab the shell crisec.
HDPA g_hdpaDarwinAds = NULL;

typedef enum tagINSTALLSTATE
{
	INSTALLSTATE_NOTUSED = -7,  // component disabled
	INSTALLSTATE_BADCONFIG = -6,  // configuration data corrupt
	INSTALLSTATE_INCOMPLETE = -5,  // installation suspended or in progress
	INSTALLSTATE_SOURCEABSENT = -4,  // run from source, source is unavailable
	INSTALLSTATE_MOREDATA = -3,  // return buffer overflow
	INSTALLSTATE_INVALIDARG = -2,  // invalid function argument
	INSTALLSTATE_UNKNOWN = -1,  // unrecognized product or feature
	INSTALLSTATE_BROKEN = 0,  // broken
	INSTALLSTATE_ADVERTISED = 1,  // advertised feature
	INSTALLSTATE_REMOVED = 1,  // component being removed (action state, not settable)
	INSTALLSTATE_ABSENT = 2,  // uninstalled (or action state absent but clients remain)
	INSTALLSTATE_LOCAL = 3,  // installed on local drive
	INSTALLSTATE_SOURCE = 4,  // run from source, CD or net
	INSTALLSTATE_DEFAULT = 5,  // use default, local or source
} INSTALLSTATE;
#define MAX_FEATURE_CHARS  38   // maximum chars in feature name (same as string GUID)

class CDarwinAd
{
public:
	LPITEMIDLIST    _pidl;
	LPTSTR          _pszDescriptor;
	LPTSTR          _pszLocalPath;
	INSTALLSTATE    _state;

	CDarwinAd(LPITEMIDLIST pidl, LPTSTR psz)
	{
		// I take ownership of this pidl
		_pidl = pidl;
		//_pszDescriptor = psz;
		Str_SetPtr(&_pszDescriptor, psz);
	}

	void CheckInstalled()
	{
		TCHAR szProduct[GUIDSTR_MAX];
		TCHAR szFeature[MAX_FEATURE_CHARS];
		TCHAR szComponent[GUIDSTR_MAX];
		
		//if (MsiDecomposeDescriptor(_pszDescriptor, szProduct, szFeature, szComponent, NULL) == ERROR_SUCCESS)
		//{
		//	_state = MsiQueryFeatureState(szProduct, szFeature);
		//}
		//else
		//{
		//	_state = INSTALLSTATE_INVALIDARG;
		//}
		
		// Note: Cannot use ParseDarwinID since that bumps the usage count
		// for the app and we're not running the app, just looking at it.
		// Also because ParseDarwinID tries to install the app (eek!)
		//
		// Must ignore INSTALLSTATE_SOURCE because MsiGetComponentPath will
		// try to install the app even though we're just querying...
		TCHAR szCommand[MAX_PATH];
		DWORD cch = ARRAYSIZE(szCommand);
		_pszLocalPath = NULL;
		//if (_state == INSTALLSTATE_LOCAL &&
		//	MsiGetComponentPath(szProduct, szComponent, szCommand, &cch) == _state)
		//{
		//	PathUnquoteSpaces(szCommand);
		//	_pszLocalPath = szCommand;
		//}
		//else
		//{
		//	_pszLocalPath = NULL;
		//}
	}

	BOOL IsAd()
	{
		return _state == INSTALLSTATE_ADVERTISED;
	}

	~CDarwinAd()
	{
		ILFree(_pidl);
		Str_SetPtr(&_pszDescriptor, NULL);
		Str_SetPtr(&_pszLocalPath, NULL);
	}
};

int GetDarwinIndex(LPCITEMIDLIST pidlFull, CDarwinAd** ppda)
{
	int iRet = -1;
	if (g_hdpaDarwinAds)
	{
		int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
		for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
		{
			*ppda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
			if (*ppda)
			{
				if (ILIsEqual((*ppda)->_pidl, pidlFull))
				{
					iRet = ihdpa;
					break;
				}
			}
		}
	}
	return iRet;
}

STDAPI_(void) SHReValidateDarwinCache()
{
	if (g_hdpaDarwinAds)
	{
		ENTERCRITICAL_DARWINADS;
		int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
		for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
		{
			CDarwinAd* pda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
			if (pda)
			{
				pda->CheckInstalled();
			}
		}
		LEAVECRITICAL_DARWINADS;
	}
}

STDAPI_(void) SHReValidateDarwinCacheCustom()
{
	if (g_hdpaDarwinAds)
	{
		ENTERCRITICAL_DARWINADS;
		int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
		for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
		{
			CDarwinAd* pda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
			if (pda)
			{
				pda->CheckInstalled();
			}
		}
		LEAVECRITICAL_DARWINADS;
	}
}

CStartMenuCallbackBase::CStartMenuCallbackBase(BOOL fIsStartPanel)
{
	memset(this,0,sizeof(CStartMenuCallbackBase));

	_fIsStartPanel = fIsStartPanel;
	_cRef = 1;
	_dwThreadID = GetCurrentThreadId();

	TCHAR szBuf[MAX_PATH];
	DWORD cbSize = sizeof(szBuf); // SHGetValue wants sizeof

	if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_EXPLORER_WINUPDATE, TEXT("ShortcutName"),
		NULL, szBuf, &cbSize))
	{
		// Add ".lnk" if the file doesn't have an extension
		PathAddExtension(szBuf, TEXT(".lnk"));
		Str_SetPtr(&_pszWindowsUpdate, szBuf);
		//_pszWindowsUpdate = szBuf;
	}

	cbSize = sizeof(szBuf); // SHGetValue wants sizeof
	if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, TEXT("SM_ConfigureProgramsName"),
		NULL, szBuf, &cbSize))
	{
		PathAddExtension(szBuf, TEXT(".lnk"));
		//_pszConfigurePrograms = szBuf;
		Str_SetPtr(&_pszConfigurePrograms, szBuf);
	}

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_ADMINTOOLS | CSIDL_FLAG_CREATE, NULL, 0, szBuf)))
	{
		//_pszAdminTools = szBuf;
		Str_SetPtr(&_pszAdminTools, PathFindFileName(szBuf));
	}

	_RefreshSettings();

	SHReValidateDarwinCacheCustom();
}

CStartMenuCallbackBase::~CStartMenuCallbackBase()
{
	assert(_dwThreadID == GetCurrentThreadId());

	_pszWindowsUpdate = NULL;
	_pszConfigurePrograms = NULL;
	_pszAdminTools = NULL;
	_pszPrograms = NULL;

	if (_ptp2)
		_ptp2->Release();
}

void CStartMenuCallbackBase::_InitializePrograms()
{
	if (!_fInitPrograms)
	{
		// We're either initing these, or reseting them.
		TCHAR szTemp[MAX_PATH];
		SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL, 0, szTemp);
		//_pszPrograms = PathFindFileName(szTemp);
		Str_SetPtr(&_pszPrograms, PathFindFileName(szTemp));

		_fInitPrograms = TRUE;
	}
}

#define IDM_TOPLEVELSTARTMENU  0
#define IDM_RECENT              501
#define IDM_FIND                502
#define IDM_HELPSEARCH          503
#define IDM_PROGRAMS            504
#define IDM_CONTROLS            505
#define IDM_EXITWIN             506
#define IDM_SETTINGS            508
#define IDM_PRINTERS            510
#define IDM_STARTMENU           511
#define IDM_MYCOMPUTER          512
#define IDM_PROGRAMSINIT        513
#define IDM_RECENTINIT          514
#define IDM_MYDOCUMENTS         516
#define IDM_MENU_FIND           520
#define TRAY_IDM_FINDFIRST      521  // this range
#define TRAY_IDM_FINDLAST       550  // is reserved for find command
#define IDM_NETCONNECT          557
#define IDM_FAVORITES               507

STDAPI DisplayNameOf(IShellFolder* psf, LPCITEMIDLIST pidl, DWORD flags, LPTSTR psz, UINT cch)
{
	*psz = 0;
	STRRET sr;
	HRESULT hr = psf->GetDisplayNameOf(pidl, flags, &sr);
	if (SUCCEEDED(hr))
		hr = StrRetToBuf(&sr, pidl, psz, cch);
	return hr;
}

HRESULT CStartMenuCallbackBase::_FilterPidl(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl)
{
	HRESULT hr = S_FALSE;

	//assert(IS_VALID_PIDL(pidl));
	//assert(IS_VALID_CODE_PTR(psf, IShellFolder));

	if (uParent == IDM_PROGRAMS || uParent == IDM_TOPLEVELSTARTMENU)
	{
		TCHAR szChild[MAX_PATH];
		if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szChild, ARRAYSIZE(szChild))))
		{
			// HACKHACK (lamadio): This code assumes that the Display name
			// of the Programs and Commons Programs folders are the same. It
			// also assumes that the "programs" folder in the Start Menu folder
			// is the same name as the one pointed to by CSIDL_PROGRAMS.
			// Filter from top level start menu:
			//      Programs, Windows Update, Configure Programs
			if (_IsTopLevelStartMenu(uParent, psf, pidl))
			{
				if ((_pszPrograms && (0 == lstrcmpi(szChild, _pszPrograms))) ||
					(SHRestricted(REST_NOUPDATEWINDOWS) && _pszWindowsUpdate && (0 == lstrcmpi(szChild, _pszWindowsUpdate))) ||
					(SHRestricted(REST_NOSMCONFIGUREPROGRAMS) && _pszConfigurePrograms && (0 == lstrcmpi(szChild, _pszConfigurePrograms))))
				{
					hr = S_OK;
				}
			}
			else
			{
				// IDM_PROGRAMS
				// Filter from Programs:  Administrative tools.
				if (!_fShowAdminTools && _pszAdminTools && lstrcmpi(szChild, _pszAdminTools) == 0)
				{
					hr = S_OK;
				}
			}
		}
	}
	return hr;
}

#define SMINV_FORCE          0x00000080
#define UEMIID_SHELL    CLSID_ActiveDesktop     // FEATURE need better one
#define UEMIID_BROWSER  CLSID_InternetToolbar   // FEATURE need better one
#define UEME_RUNPIDL    18
#define UEMF_EVENTMON   0x00000001 
#define UEMF_INSTRUMENT 0x00000002
#define UEMF_XEVENT     (UEMF_EVENTMON | UEMF_INSTRUMENT)

HRESULT CStartMenuCallbackBase::_Promote(LPSMDATA psmd, DWORD dwFlags)
{
	if ((_fExpandoMenus || (_fIsStartPanel && (dwFlags & SMINV_FORCE))) &&
		(psmd->uIdAncestor == IDM_PROGRAMS ||
			psmd->uIdAncestor == IDM_FAVORITES))
	{
		UEMFireEvent(psmd->uIdAncestor == IDM_PROGRAMS ? &UEMIID_SHELL : &UEMIID_BROWSER,
			UEME_RUNPIDL, UEMF_XEVENT, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem);
	}
	return S_OK;
}

BOOL CStartMenuCallbackBase::_IsTopLevelStartMenu(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl)
{
	return uParent == IDM_TOPLEVELSTARTMENU ||
		(uParent == IDM_PROGRAMS && _fIsStartPanel && IsMergedFolderGUID(psf, pidl, CLSID_StartMenu));
}

LPITEMIDLIST FullPidlFromSMData(LPSMDATA psmd)
{
	LPITEMIDLIST pidlItem;
	LPITEMIDLIST pidlFolder = NULL;
	LPITEMIDLIST pidlFull = NULL;
	IAugmentedShellFolder* pasf2;
	if (SUCCEEDED(psmd->psf->QueryInterface(IID_PPV_ARGS(&pasf2))))
	{
		if (SUCCEEDED(pasf2->UnWrapIDList(psmd->pidlItem, 1, NULL, &pidlFolder, &pidlItem, NULL)))
		{
			pidlFull = ILCombine(pidlFolder, pidlItem);
			ILFree(pidlFolder);
			ILFree(pidlItem);
		}
		pasf2->Release();
	}

	if (!pidlFolder)
	{
		pidlFull = ILCombine(psmd->pidlFolder, psmd->pidlItem);
	}

	return pidlFull;
}

#define UEMIID_SHELL    CLSID_ActiveDesktop     // FEATURE need better one
#define UEMIID_BROWSER  CLSID_InternetToolbar   // FEATURE need better one

STDMETHODIMP_(int) s_DarwinAdsDestroyCallback(LPVOID pData1, LPVOID pData2)
{
	CDarwinAd* pda = (CDarwinAd*)pData1;
	if (pda)
		delete pda;
	return TRUE;
}

BOOL SHRegisterDarwinLink(LPITEMIDLIST pidlFull, LPWSTR pszDarwinID, BOOL fUpdate)
{
	BOOL fRetVal = FALSE;

	ENTERCRITICAL_DARWINADS;

	if (pidlFull)
	{
		CDarwinAd* pda = NULL;

		if (GetDarwinIndex(pidlFull, &pda) != -1 && pda)
		{
			// We already know about this link; don't need to add it
			fRetVal = TRUE;
		}
		else
		{
			pda = new CDarwinAd(pidlFull, pszDarwinID);
			if (pda)
			{
				pidlFull = NULL;    // take ownership

				// Do we have a global cache?
				if (g_hdpaDarwinAds == NULL)
				{
					// No; This is either the first time this is called, or we
					// failed the last time.
					g_hdpaDarwinAds = DPA_Create(5);
				}

				if (g_hdpaDarwinAds)
				{
					// DPA_AppendPtr returns the zero based index it inserted it at.
					if (DPA_AppendPtr(g_hdpaDarwinAds, (void*)pda) >= 0)
					{
						fRetVal = TRUE;
					}

				}
			}
		}

		if (!fRetVal)
		{
			// if we failed to create a dpa, delete this.
			delete pda;
		}
		else if (fUpdate)
		{
			// update the entry if requested
			pda->CheckInstalled();
		}
		ILFree(pidlFull);

	}
	else if (!pszDarwinID)
	{
		// NULL, NULL means "destroy darwin info, we're shutting down"
		HDPA hdpa = g_hdpaDarwinAds;
		g_hdpaDarwinAds = NULL;
		if (hdpa)
			DPA_DestroyCallback(hdpa, s_DarwinAdsDestroyCallback, NULL);
	}

	LEAVECRITICAL_DARWINADS;

	return fRetVal;
}

BOOL ProcessDarwinAd(IShellLinkDataList* psldl, LPCITEMIDLIST pidlFull)
{
	// This function does not check for the existance of a member before adding it,
	// so it is entirely possible for there to be duplicates in the list....
	BOOL fIsLoaded = FALSE;
	BOOL fFreesldl = FALSE;
	BOOL fRetVal = FALSE;

	if (!psldl)
	{
		// We will detect failure of this at use time.
		if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&psldl))))
		{
			return FALSE;
		}

		fFreesldl = TRUE;

		IPersistFile* ppf;
		OLECHAR sz[MAX_PATH];
		if (SHGetPathFromIDListW(pidlFull, sz))
		{
			if (SUCCEEDED(psldl->QueryInterface(IID_PPV_ARGS(&ppf))))
			{
				if (SUCCEEDED(ppf->Load(sz, 0)))
				{
					fIsLoaded = TRUE;
				}
				ppf->Release();
			}
		}
	}
	else
		fIsLoaded = TRUE;

	CDarwinAd* pda = NULL;
	if (fIsLoaded)
	{
		EXP_DARWIN_LINK* pexpDarwin;

		if (SUCCEEDED(psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin)))
		{
			fRetVal = SHRegisterDarwinLink(ILClone(pidlFull), pexpDarwin->szwDarwinID, TRUE);
			LocalFree(pexpDarwin);
		}
	}

	if (fFreesldl)
		psldl->Release();

	return fRetVal;
}

HRESULT CStartMenuCallbackBase::_HandleNew(LPSMDATA psmd)
{
	HRESULT hr = S_FALSE;
	if (_fExpandoMenus &&
		(psmd->uIdAncestor == IDM_PROGRAMS ||
			psmd->uIdAncestor == IDM_FAVORITES))
	{
		UEMINFO uei;
		uei.cbSize = sizeof(uei);
		uei.dwMask = UEIM_HIT;
		uei.R = UEM_NEWITEMCOUNT;
		hr = UEMSetEvent(psmd->uIdAncestor == IDM_PROGRAMS ? &UEMIID_SHELL : &UEMIID_BROWSER, (WPARAM)psmd->psf, &uei);
	}

	if (psmd->uIdAncestor == IDM_PROGRAMS)
	{
		LPITEMIDLIST pidlFull = FullPidlFromSMData(psmd);
		if (pidlFull)
		{
			ProcessDarwinAd(NULL, pidlFull);
			ILFree(pidlFull);
		}
	}
	return hr;
}

HRESULT CStartMenuCallbackBase::_GetSFInfo(SMDATA* psmd, SMINFO* psminfo)
{
	if (psminfo->dwMask & SMIM_FLAGS &&
		(psmd->uIdAncestor == IDM_PROGRAMS ||
			psmd->uIdAncestor == IDM_FAVORITES))
	{
		if (_fExpandoMenus)
		{
			psminfo->dwFlags |= _GetDemote(psmd);
		}

		// This is a little backwards. If the Restriction is On, Then we allow the feature.
		if (SHRestricted(REST_GREYMSIADS) &&
			psmd->uIdAncestor == IDM_PROGRAMS)
		{
			LPITEMIDLIST pidlFull = FullPidlFromSMData(psmd);
			if (pidlFull)
			{
				if (_IsDarwinAdvertisement(pidlFull))
				{
					psminfo->dwFlags |= SMIF_ALTSTATE;
				}
				ILFree(pidlFull);
			}
		}

		if (_ptp2)
		{
			_ptp2->ModifySMInfo(psmd, psminfo);
		}
	}
	return S_OK;
}

STDAPI_(BOOL) SMILIsAncestor(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlBelow)
{
	if (pidlParent && pidlBelow)
		return ILIsParent(pidlParent, pidlBelow, FALSE);
	else
		return FALSE;
}

HRESULT CStartMenuCallbackBase::_ProcessChangeNotify(SMDATA* psmd, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
	switch (lEvent)
	{
	case SHCNE_ASSOCCHANGED:
		SHReValidateDarwinCache();
		return S_OK;

	case SHCNE_RENAMEFOLDER:
		// NTRAID89654-2000/03/13 (lamadio): We should move the MenuOrder stream as well. 5.5.99
	case SHCNE_RENAMEITEM:
	{
		LPITEMIDLIST pidlPrograms;
		LPITEMIDLIST pidlProgramsCommon;
		LPITEMIDLIST pidlFavorites;
		SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlPrograms);
		SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlProgramsCommon);
		SHGetFolderLocation(NULL, CSIDL_FAVORITES, NULL, 0, &pidlFavorites);

		BOOL fPidl1InStartMenu = SMILIsAncestor(pidlPrograms, pidl1) ||
			SMILIsAncestor(pidlProgramsCommon, pidl1);
		BOOL fPidl1InFavorites = SMILIsAncestor(pidlFavorites, pidl1);


		// If we're renaming something from the Start Menu
		if (fPidl1InStartMenu || fPidl1InFavorites)
		{
			IShellFolder* psfFrom;
			LPCITEMIDLIST pidlFrom;
			if (SUCCEEDED(SHBindToParent(pidl1, IID_PPV_ARGS(&psfFrom), &pidlFrom)))
			{
				// Into the Start Menu
				BOOL fPidl2InStartMenu = SMILIsAncestor(pidlPrograms, pidl2) ||
					SMILIsAncestor(pidlProgramsCommon, pidl2);
				BOOL fPidl2InFavorites = SMILIsAncestor(pidlFavorites, pidl2);
				if (fPidl2InStartMenu || fPidl2InFavorites)
				{
					IShellFolder* psfTo;
					LPCITEMIDLIST pidlTo;

					if (SUCCEEDED(SHBindToParent(pidl2, IID_PPV_ARGS(&psfTo), &pidlTo)))
					{
						// Then we need to rename it
						UEMRenamePidl(fPidl1InStartMenu ? &UEMIID_SHELL : &UEMIID_BROWSER,
							psfFrom, pidlFrom,
							fPidl2InStartMenu ? &UEMIID_SHELL : &UEMIID_BROWSER,
							psfTo, pidlTo);
						psfTo->Release();
					}
				}
				else
				{
					// Otherwise, we delete it.
					UEMDeletePidl(fPidl1InStartMenu ? &UEMIID_SHELL : &UEMIID_BROWSER,
						psfFrom, pidlFrom);
				}

				psfFrom->Release();
			}
		}

		ILFree(pidlPrograms);
		ILFree(pidlProgramsCommon);
		ILFree(pidlFavorites);
	}
	break;

	case SHCNE_DELETE:
		// NTRAID89654-2000/03/13 (lamadio): We should nuke the MenuOrder stream as well. 5.5.99
	case SHCNE_RMDIR:
	{
		IShellFolder* psf;
		LPCITEMIDLIST pidl;

		if (SUCCEEDED(SHBindToParent(pidl1, IID_PPV_ARGS(&psf), &pidl)))
		{
			// NOTE favorites is the only that will be initialized
			UEMDeletePidl(psmd->uIdAncestor == IDM_FAVORITES ? &UEMIID_BROWSER : &UEMIID_SHELL,
				psf, pidl);
			psf->Release();
		}

	}
	break;

	case SHCNE_CREATE:
	case SHCNE_MKDIR:
	{
		IShellFolder* psf;
		LPCITEMIDLIST pidl;

		if (SUCCEEDED(SHBindToParent(pidl1, IID_PPV_ARGS(&psf), &pidl)))
		{
			UEMINFO uei;
			uei.cbSize = sizeof(uei);
			uei.dwMask = UEIM_HIT;
			uei.R = UEM_NEWITEMCOUNT;
			UEMSetEvent(psmd->uIdAncestor == IDM_FAVORITES ? &UEMIID_BROWSER : &UEMIID_SHELL, (WPARAM)psf, &uei);
			psf->Release();
		}

	}
	break;
	}

	return S_FALSE;
}

#define REGSTR_EXPLORER_ADVANCED REGSTR_PATH_EXPLORER TEXT("\\Advanced")

BOOL FeatureEnabled(LPTSTR pszFeature)
{
	return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, pszFeature,
		FALSE, // Don't ignore HKCU
		FALSE); // Disable this cool feature.
}

BOOL GetExplorerUserSetting(HKEY hkeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
	TCHAR szPath[MAX_PATH];
	TCHAR szPathExplorer[MAX_PATH];
	DWORD cbSize = ARRAYSIZE(szPath);
	DWORD dwType;

	PathCombine(szPathExplorer, REGSTR_PATH_EXPLORER, pszSubKey);
	if (ERROR_SUCCESS == SHGetValue(hkeyRoot, szPathExplorer, pszValue,
		&dwType, szPath, &cbSize))
	{
		// Zero in the DWORD case or NULL in the string case
		// indicates that this item is not available.
		if (dwType == REG_DWORD)
			return *((DWORD*)szPath) != 0;
		else
			return (TCHAR)szPath[0] != 0;
	}

	return -1;
}

#define ROUS_DEFAULTALLOW       0x0000
#define ROUS_DEFAULTRESTRICT    0x0001
#define ROUS_KEYALLOWS          0x0000
#define ROUS_KEYRESTRICTS       0x0002

STDAPI_(BOOL) IsRestrictedOrUserSetting(HKEY hkeyRoot, RESTRICTIONS rest, LPCTSTR pszSubKey, LPCTSTR pszValue, UINT flags)
{
	// See if the system policy restriction trumps

	DWORD dwRest = SHRestricted(rest);

	if (dwRest == 1)
		return TRUE;

	if (dwRest == 2)
		return FALSE;

	//
	//  Restriction not in place or defers to user setting.
	//
	BOOL fValidKey = GetExplorerUserSetting(hkeyRoot, pszSubKey, pszValue);

	switch (fValidKey)
	{
	case 0:     // Key is present and zero
		if (flags & ROUS_KEYRESTRICTS)
			return FALSE;       // restriction not present
		else
			return TRUE;        // ROUS_KEYALLOWS, value is 0 -> restricted

	case 1:     // Key is present and nonzero

		if (flags & ROUS_KEYRESTRICTS)
			return TRUE;        // restriction present -> restricted
		else
			return FALSE;       // ROUS_KEYALLOWS, value is 1 -> not restricted

	default:
		assert(0);  // _GetExplorerUserSetting returns exactly 0, 1 or -1.
		// Fall through

	case -1:    // Key is not present
		return (flags & ROUS_DEFAULTRESTRICT);
	}

	/*NOTREACHED*/
}

BOOL IsStartMenuChangeNotAllowed(BOOL fStartPanel)
{
	return(IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NOCHANGESTARMENU,
		TEXT("Advanced"),
		(fStartPanel ? TEXT("Start_EnableDragDrop") : TEXT("StartMenuChange")),
		ROUS_DEFAULTALLOW | ROUS_KEYALLOWS));
}

#define SMINIT_DEFAULT              0x00000000  // No Options
#define SMINIT_RESTRICT_CONTEXTMENU 0x00000001  // Don't allow Context Menus
#define SMINIT_RESTRICT_DRAGDROP    0x00000002  // Don't allow Drag and Drop
#define SMINIT_TOPLEVEL             0x00000004  // This is the top band.
#define SMINIT_DEFAULTTOTRACKPOPUP  0x00000008  // When no callback is specified, 
#define SMINIT_CACHED               0x00000010
#define SMINIT_USEMESSAGEFILTER     0x00000020
#define SMINIT_LEGACYMENU           0x00000040  // Old Menu behaviour.
#define SMINIT_CUSTOMDRAW           0x00000080   // Send SMC_CUSTOMDRAW
#define SMINIT_NOSETSITE            0x00010000  // Internal setting
#define SMINIT_VERTICAL             0x10000000  // This is a vertical menu
#define SMINIT_HORIZONTAL           0x20000000  // This is a horizontal menu    (does not inherit)
#define SMINIT_MULTICOLUMN          0x40000000  // this is a multi column menu

#define STRREG_STARTMENU TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Start Menu")
#define STRREG_STARTMENU2 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Start Menu2")
#define SMSET_SEPARATEMERGEFOLDER   0x00000200    //Insert separator when MergedFolder host changes




BOOL IsCSIDLChild(int csidlParent, int csidlChild)
{
	BOOL fChild = FALSE;
	TCHAR sz1[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, csidlParent, NULL, 0, sz1)))
	{
		TCHAR sz2[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, csidlChild, NULL, 0, sz2)))
		{
			TCHAR szCommonRoot[MAX_PATH];
			if (PathCommonPrefix(sz1, sz2, szCommonRoot) ==
				lstrlen(sz1))
			{
				fChild = TRUE;
			}
		}
	}

	return fChild;
}

HRESULT GetFolderAndPidl(UINT csidl, IShellFolder** ppsf, LPITEMIDLIST* ppidl)
{
	*ppsf = NULL;
	HRESULT hr = SHGetFolderLocation(NULL, csidl, NULL, 0, ppidl);
	if (SUCCEEDED(hr))
	{
		hr = SHBindToObject(NULL, *ppidl, 0, IID_PPV_ARGS(ppsf));
		if (FAILED(hr))
		{
			ILFree(*ppidl);
			*ppidl = NULL;
		}
	}
	return hr;
}

#define SMSET_DONTREGISTERCHANGENOTIFY 0x00000020 // ShellFolder is a discontiguous child of a parent shell folder
HRESULT CStartMenuCallbackBase::InitializeProgramsShellMenu(IShellMenu* psm)
{
	HKEY hkeyPrograms = NULL;
	LPITEMIDLIST pidl = NULL;

	//_fIsStartPanel = true;

	DWORD dwInitFlags = SMINIT_VERTICAL;
	if (!FeatureEnabled(_fIsStartPanel ? TEXT("Start_ScrollPrograms") : TEXT("StartMenuScrollPrograms")))
		dwInitFlags |= SMINIT_MULTICOLUMN;

	if (IsStartMenuChangeNotAllowed(_fIsStartPanel))
		dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

	if (_fIsStartPanel)
		dwInitFlags |= SMINIT_TOPLEVEL;

	//HRESULT hr = psm->Initialize(this, 0, 0, SMINIT_VERTICAL | SMINIT_TOPLEVEL);
	HRESULT hr = psm->Initialize(this, IDM_PROGRAMS, IDM_PROGRAMS, dwInitFlags);
	if (SUCCEEDED(hr))
	{
		_InitializePrograms();

		LPCTSTR pszOrderKey = _fIsStartPanel ?
			STRREG_STARTMENU2 TEXT("\\Programs") :
			STRREG_STARTMENU TEXT("\\Programs");

		RegCreateKeyEx(HKEY_CURRENT_USER, pszOrderKey, NULL, NULL,
			REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
			NULL, &hkeyPrograms, NULL);

		IShellFolder* psf;
		BOOL fOptimize = FALSE;
		DWORD dwSmset = SMSET_TOP;

		if (_fIsStartPanel)
		{
			// Start Panel: Menu:  The Programs section is a merge of the
			// Fast Items and Programs folders with a separator between them.
			// WIKTOR: TEMP REMOVE FOR NON MERGED
			//dwSmset |= SMSET_SEPARATEMERGEFOLDER;

			//hr = GetMergedFolder(&psf, &pidl, c_rgmfiProgramsFolderAndFastItems, 4);
			//hr = GetMergedFolder(&psf, &pidl, c_rgmfiProgramsFolderAndFastItems, 4);
			hr = GetFolderAndPidl(CSIDL_COMMON_PROGRAMS, &psf, &pidl);
			if (!psf)
				MessageBox(0, L"psf", L"psf", 0);
		}
		else
		{
			// Classic Start Menu:  The Programs section is just the per-user
			// and common Programs folders merged together
			//hr = GetMergedFolder(&psf, &pidl, c_rgmfiProgramsFolder, 2);
			hr = GetFolderAndPidl(CSIDL_COMMON_PROGRAMS, &psf, &pidl);
			if (!psf)
				MessageBox(0, L"psf2", L"psf2", 0);
			// We used to register for change notify at CSIDL_STARTMENU and assumed
			// that CSIDL_PROGRAMS was a child of CSIDL_STARTMENU. Since this wasn't always the 
			// case, I removed the optimization.

			// Both panes are registered recursive. So, When CSIDL_PROGRAMS _IS_ a 
			// child of CSIDL_STARTMENU we can enter a code path where when destroying 
			// CSIDL_PROGRAMS, we unregister it. This will flush the change nofiy queue 
			// of CSIDL_STARTMENU, and blow away all of the children, including CSIDL_PROGRAMS, 
			// while we are in the middle of destroying it... See the problem? I have been adding 
			// reentrance "Blockers" but this only delayed where we crashed. 
			// What was needed was to determine if Programs was a child of the Start Menu directory.
			// if it was we need to add the optmimization. If it's not we don't have a problem.

			// WINDOWS BUG 135156(tybeam): If one of the two is redirected, then this will get optimized
			// we can't do better than this because both are registed recursive, and this will fault...
			fOptimize = IsCSIDLChild(CSIDL_STARTMENU, CSIDL_PROGRAMS)
				|| IsCSIDLChild(CSIDL_COMMON_STARTMENU, CSIDL_COMMON_PROGRAMS);
			if (fOptimize)
			{
				dwSmset |= SMSET_DONTREGISTERCHANGENOTIFY;
			}
		}

		if (SUCCEEDED(hr))
		{
			// We should have a pidl from CSIDL_Programs
			assert(pidl);

			// We should have a shell folder from the bind.
			assert(psf);

			//IEnumIDList* pEnumIDList;
			//HRESULT hr = psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
			//int count = 0;
			//LPITEMIDLIST pidl = NULL;
			//while (pEnumIDList->Next(1, &pidl, NULL) == S_OK) {
			//	count++;
			//	CoTaskMemFree(pidl);  // Free the PIDL after use
			//}
			//pEnumIDList->Release();
			//dbgprintf(L"count %i",count);

			hr = psm->SetShellFolder(psf, pidl, hkeyPrograms, dwSmset);
			psf->Release();
			ILFree(pidl);
		}

		if (FAILED(hr))
			RegCloseKey(hkeyPrograms);
	}

	return hr;
}

BOOL CStartMenuCallbackBase::_IsDarwinAdvertisement(LPCITEMIDLIST pidlFull)
{
	//return false;
	ENTERCRITICAL_DARWINADS;
	
	// NOTE: There can be two items in the hdpa. This is ok.
	BOOL fAd = FALSE;
	CDarwinAd* pda = NULL;
	int iIndex = GetDarwinIndex(pidlFull, &pda);
	// Are there any ads?
	if (iIndex != -1 && pda != NULL)
	{
		//This is a Darwin pidl. Is it installed?
		fAd = pda->IsAd();
	}
	
	LEAVECRITICAL_DARWINADS;
	return fAd;
}

void CStartMenuCallbackBase::_RefreshSettings()
{
	_fShowAdminTools = FeatureEnabled(TEXT("StartMenuAdminTools"));
}

#define SMC_FILTERPIDL          0x10000000  // The callback is called to see if an item is visible
#define SMC_GETSFINFOTIP        0x0000000C  // The callback is called to get some object

STDMETHODIMP CPersonalProgramsMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr = S_FALSE;

	switch (uMsg)
	{

	case SMC_INITMENU:
		_UpdateTrayPriv();
		break;

	case SMC_GETSFINFO:
		hr = _GetSFInfo(psmd, (SMINFO*)lParam);
		break;

	case SMC_NEWITEM:
		hr = _HandleNew(psmd);
		break;

	case SMC_FILTERPIDL:
		assert(psmd->dwMask & SMDM_SHELLFOLDER);
		hr = _FilterPidl(psmd->uIdParent, psmd->psf, psmd->pidlItem);
		break;

	case SMC_GETSFINFOTIP:
		if (!FeatureEnabled(TEXT("ShowInfoTip")))
			hr = E_FAIL;  // E_FAIL means don't show. S_FALSE means show default
		break;

	case SMC_PROMOTE:
		hr = _Promote(psmd, (DWORD)wParam);
		break;

	case SMC_SHCHANGENOTIFY:
	{
		PSMCSHCHANGENOTIFYSTRUCT pshf = (PSMCSHCHANGENOTIFYSTRUCT)lParam;
		hr = _ProcessChangeNotify(psmd, pshf->lEvent, pshf->pidl1, pshf->pidl2);
	}
	break;

	case SMC_REFRESH:
		_RefreshSettings();
		break;
	}

	return hr;
}


STDMETHODIMP_(HRESULT __stdcall) CPersonalProgramsMenuCallback::SetSite(IUnknown* punk)
{
	HRESULT hr = CObjectWithSite::SetSite(punk);
	_UpdateTrayPriv();
	return hr;
}

void CPersonalProgramsMenuCallback::_UpdateTrayPriv()
{
	if (_ptp2)
		_ptp2->Release();
	IObjectWithSite* pows;
	if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_PPV_ARGS(&pows))))
	{
		pows->GetSite(IID_PPV_ARGS(&_ptp2));
		pows->Release();
	}
}

STDMETHODIMP_(HRESULT __stdcall) CObjectWithSite::SetSite(IUnknown* punkSite)
{
	IUnknown_Set(&_punkSite, punkSite);
	return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) CObjectWithSite::GetSite(REFIID riid, void** ppvSite)
{
	return E_NOTIMPL;
}

#define SMC_DESTROY             0x0000002B  // Called when a pane is being destroyed.
#define SMC_EXEC                0x00000004  // The callback is called to execute an item
#define SMC_GETINFOTIP          0x0000000D  // The callback is called to get some object
#define SMC_BEGINENUM           0x00000013  // tell callback that we are beginning to ENUM the indicated parent
#define SMC_ENDENUM             0x00000014  // tell callback that we are ending the ENUM of the indicated paren
#define SMC_DUMPONUPDATE        0x00000035  // S_OK if host wants old trash-everything-on-update behavior (recent docs)
#define SMC_INSERTINDEX         0x0000000E  // New item insert index
#define SMSET_MERGE                 0x00000002
#define SMC_MAPACCELERATOR      0x00000015  // Called when processing an accelerator.
#define SMC_GETMINPROMOTED      0x00000018  // Returns the minimum number of promoted items
#define STARTMENU_CHEVRONCLICKED        0x00000002

DWORD GetClickCount()
{

	//This function retrieves the number of times the user has clicked on the chevron item.

	DWORD dwType;
	DWORD cbSize = sizeof(DWORD);
	DWORD dwCount = 1;      // Default to three clicks before we give up.
	// PMs what it to 1 now. Leaving back end in case they change their mind.
	SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuChevron"),
		&dwType, (BYTE*)&dwCount, &cbSize);

	return dwCount;

}

void SetClickCount(DWORD dwClickCount)
{
	SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuChevron"), REG_DWORD, &dwClickCount, sizeof(DWORD));
}

HRESULT CreateRecentMRUList(IMruDataList** ppmru)
{
	//*ppmru = CreateSharedRecentMRUList(NULL, NULL, SRMLF_COMPPIDL);
	return *ppmru ? S_OK : E_OUTOFMEMORY;
}

#define RESTOPT_INTELLIMENUS_USER       0
#define RESTOPT_INTELLIMENUS_DISABLED   1       // Match Restriction assumption: 1 == Off
#define RESTOPT_INTELLIMENUS_ENABLED    2
BOOL AreIntelliMenusEnabled()
{
	DWORD dwRest = SHRestricted(REST_INTELLIMENUS);
	if (dwRest != RESTOPT_INTELLIMENUS_USER)
		return (dwRest == RESTOPT_INTELLIMENUS_ENABLED);

	return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("IntelliMenus"),
		FALSE, TRUE); // Don't ignore HKCU, Enable Menus by default
}

STDMETHODIMP_(HRESULT __stdcall) CStartMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr = S_FALSE;
	switch (uMsg)
	{

	case SMC_CREATE:
		hr = _Create(psmd, (void**)lParam);
		break;

	case SMC_DESTROY:
		hr = _Destroy(psmd);
		break;

	case SMC_INITMENU:
		hr = _Init(psmd);
		break;

	case SMC_SFEXEC:
		hr = _ExecItem(psmd, uMsg);
		break;

	case SMC_EXEC:
		hr = _ExecHmenuItem(psmd);
		break;

	case SMC_GETOBJECT:
		hr = _GetObject(psmd, (GUID) * ((GUID*)wParam), (void**)lParam);
		break;

	case SMC_GETINFO:
		hr = _GetHmenuInfo(psmd, (SMINFO*)lParam);
		break;

	case SMC_GETSFINFOTIP:
		if (!_fShowInfoTip)
			hr = E_FAIL;  // E_FAIL means don't show. S_FALSE means show default
		break;

	case SMC_GETINFOTIP:
		hr = _GetStaticInfoTip(psmd, (LPWSTR)wParam, (int)lParam);
		break;

	case SMC_GETSFINFO:
		hr = _GetSFInfo(psmd, (SMINFO*)lParam);
		break;

	case SMC_BEGINENUM:
		if (psmd->uIdParent == IDM_RECENT)
		{
			assert(_cRecentDocs == -1);
			assert(!_pmruRecent);
			CreateRecentMRUList(&_pmruRecent);

			_cRecentDocs = 0;
			hr = S_OK;
		}
		break;

	case SMC_ENDENUM:
		if (psmd->uIdParent == IDM_RECENT)
		{
			assert(_cRecentDocs != -1);
			ATOMICRELEASE(_pmruRecent);

			_cRecentDocs = -1;
			hr = S_OK;
		}
		break;

	case SMC_DUMPONUPDATE:
		if (psmd->uIdParent == IDM_RECENT)
		{
			hr = S_OK;
		}
		break;

	case SMC_FILTERPIDL:
		assert(psmd->dwMask & SMDM_SHELLFOLDER);

		if (psmd->uIdParent == IDM_RECENT)
		{
			//  we need to filter out all but the first MAXRECENTITEMS
			//  and no folders allowed!
			hr = _FilterRecentPidl(psmd->psf, psmd->pidlItem);
		}
		else
		{
			hr = _FilterPidl(psmd->uIdParent, psmd->psf, psmd->pidlItem);
		}
		break;

	case SMC_INSERTINDEX:
		//ASSERT(lParam && IS_VALID_WRITE_PTR(lParam, int));
		*((int*)lParam) = 0;
		hr = S_OK;
		break;

	case SMC_SHCHANGENOTIFY:
	{
		PSMCSHCHANGENOTIFYSTRUCT pshf = (PSMCSHCHANGENOTIFYSTRUCT)lParam;
		hr = _ProcessChangeNotify(psmd, pshf->lEvent, pshf->pidl1, pshf->pidl2);
	}
	break;

	case SMC_REFRESH:
		if (psmd->uIdParent == IDM_TOPLEVELSTARTMENU)
		{
			hr = S_OK;

			// Refresh is only called on the top level.
			HMENU hmenu;
			IShellMenu* psm;
			_GetStaticStartMenu(&hmenu, &_hwnd);
			if (hmenu && psmd->punk && SUCCEEDED(psmd->punk->QueryInterface(IID_PPV_ARGS(&psm))))
			{
				hr = psm->SetMenu(hmenu, _hwnd, SMSET_BOTTOM | SMSET_MERGE);
				psm->Release();
			}

			_RefreshSettings();
			_fExpandoMenus = !_fIsStartPanel && AreIntelliMenusEnabled();
			_fCascadeMyDocuments = FeatureEnabled(TEXT("CascadeMyDocuments"));
			_fCascadePrinters = FeatureEnabled(TEXT("CascadePrinters"));
			_fCascadeControlPanel = FeatureEnabled(TEXT("CascadeControlPanel"));
			_fCascadeNetConnections = FeatureEnabled(TEXT("CascadeNetworkConnections"));
			_fAddOpenFolder = FeatureEnabled(TEXT("StartMenuOpen"));
			_fShowInfoTip = FeatureEnabled(TEXT("ShowInfoTip"));
			_fCascadeMyPictures = FeatureEnabled(TEXT("CascadeMyPictures"));
			_fFindMenuInvalid = TRUE;
			_dwFlags = GetInitFlags();
		}
		break;

	case SMC_DEMOTE:
		hr = _Demote(psmd);
		break;

	case SMC_PROMOTE:
		hr = _Promote(psmd, (DWORD)wParam);
		break;

	case SMC_NEWITEM:
		hr = _HandleNew(psmd);
		break;

	case SMC_MAPACCELERATOR:
		hr = _HandleAccelerator((TCHAR)wParam, (SMDATA*)lParam);
		break;

	case SMC_DEFAULTICON:
		assert(psmd->uIdAncestor == IDM_FAVORITES); // This is only valid for the Favorites menu
		hr = _GetDefaultIcon((LPWSTR)wParam, (int*)lParam);
		break;

	case SMC_GETMINPROMOTED:
		// Only do this for the programs menu
		if (psmd->uIdParent == IDM_PROGRAMS)
			*((int*)lParam) = 4;        // 4 was choosen by RichSt 9.15.98
		break;

	case SMC_CHEVRONEXPAND:

		// Has the user already seen the chevron tip enough times? (We set the bit when the count goes to zero.
		if (!(_dwFlags & STARTMENU_CHEVRONCLICKED))
		{
			// No; Then get the current count from the registry. We set a default of 3, but an admin can set this
			// to -1, that would make it so that they user sees it all the time.
			DWORD dwClickCount = GetClickCount();
			if (dwClickCount > 0)
			{
				// Since they clicked, take one off.
				dwClickCount--;

				// Set it back in.
				SetClickCount(dwClickCount);
			}

			if (dwClickCount == 0)
			{
				// Ah, the user has seen the chevron tip enought times... Stop being annoying.
				_dwFlags |= STARTMENU_CHEVRONCLICKED;
				SetInitFlags(_dwFlags);
			}
		}
		hr = S_OK;
		break;

	case SMC_DISPLAYCHEVRONTIP:

		// We only want to see the tip on the top level programs case, no where else. We also don't
		// want to see it if they've had enough.
		if (psmd->uIdParent == IDM_PROGRAMS &&
			!(_dwFlags & STARTMENU_CHEVRONCLICKED) &&
			!SHRestricted(REST_NOSMBALLOONTIP))
		{
			hr = S_OK;
		}
		break;

	case SMC_CHEVRONGETTIP:
		if (!SHRestricted(REST_NOSMBALLOONTIP))
			hr = _GetTip((LPWSTR)wParam, (LPWSTR)lParam);
		break;
	}

	return hr;
}

STDMETHODIMP_(HRESULT __stdcall) CStartMenuCallback::SetSite(IUnknown* punk)
{
	ATOMICRELEASE(_punkSite);
	_punkSite = punk;
	if (punk)
	{
		_punkSite->AddRef();
	}

	return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) CStartMenuCallback::GetSite(REFIID riid, void** ppvOut)
{
	if (_ptp)
		return _ptp->QueryInterface(riid, ppvOut);
	else
		return E_NOINTERFACE;
}

#define IDS_FIND_MNEMONIC       0x7674 
CStartMenuCallback::CStartMenuCallback() : _cRecentDocs(-1)
{
	memset((void*)(__int64(this) + sizeof(CStartMenuCallbackBase)),0,sizeof(CStartMenuCallback) - sizeof(CStartMenuCallbackBase));
	_cRecentDocs = -1;
	_punkSite = 0;
	LoadString(GetModuleHandle(L"shellxp.dll"), IDS_FIND_MNEMONIC, _szFindMnemonic, ARRAYSIZE(_szFindMnemonic));
}

CStartMenuCallback::~CStartMenuCallback()
{
	ATOMICRELEASE(_pcmFind);
	ATOMICRELEASE(_ptp);
	ATOMICRELEASE(_pmruRecent);
}
#define IsInRange(item,min_val,max_val) \
            (((item) >= min_val) && ((item) <= max_val))

STDAPI_(void) SetICIKeyModifiers(DWORD* pfMask)
{
	assert(pfMask);

	if (GetKeyState(VK_SHIFT) < 0)
	{
		*pfMask |= CMIC_MASK_SHIFT_DOWN;
	}

	if (GetKeyState(VK_CONTROL) < 0)
	{
		*pfMask |= CMIC_MASK_CONTROL_DOWN;
	}
}
#define IDM_MYDOCUMENTS         516
#define IDM_OPEN_FOLDER         517
#define IDM_MYPICTURES          518
#define IDM_CSC                 553

HRESULT ShowFolder(UINT csidl)
{
	LPITEMIDLIST pidl;
	if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl)))
	{
		SHELLEXECUTEINFO shei = { 0 };

		shei.cbSize = sizeof(shei);
		shei.fMask = SEE_MASK_IDLIST;
		shei.nShow = SW_SHOWNORMAL;
		shei.lpVerb = TEXT("open");
		shei.lpIDList = pidl;
		ShellExecuteEx(&shei);
		ILFree(pidl);
	}
	return S_OK;
}

void _ExecRegValue(LPCTSTR pszValue)
{
	TCHAR szPath[MAX_PATH];
	DWORD cbSize = ARRAYSIZE(szPath);

	if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_EXPLORER_ADVANCED, pszValue,
		NULL, szPath, &cbSize))
	{
		SHELLEXECUTEINFO shei = { 0 };
		shei.cbSize = sizeof(shei);
		shei.nShow = SW_SHOWNORMAL;
		shei.lpParameters = PathGetArgs(szPath);
		PathRemoveArgs(szPath);
		shei.lpFile = szPath;
		ShellExecuteEx(&shei);
	}
}

HRESULT ExecStaticStartMenuItem(int idCmd, BOOL fAllUsers, BOOL fOpen)
{
	int csidl = -1;
	HRESULT hr = E_OUTOFMEMORY;
	SHELLEXECUTEINFO shei = { 0 };
	switch (idCmd)
	{
	case IDM_PROGRAMS:          csidl = fAllUsers ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS; break;
	case IDM_FAVORITES:         csidl = CSIDL_FAVORITES; break;
	case IDM_MYDOCUMENTS:       csidl = CSIDL_PERSONAL; break;
	case IDM_MYPICTURES:        csidl = CSIDL_MYPICTURES; break;
	case IDM_CONTROLS:          csidl = CSIDL_CONTROLS;  break;
	case IDM_PRINTERS:          csidl = CSIDL_PRINTERS;  break;
	case IDM_NETCONNECT:        csidl = CSIDL_CONNECTIONS; break;
	default:
		return E_FAIL;
	}

	if (csidl != -1)
	{
		SHGetFolderLocation(NULL, csidl, NULL, 0, (LPITEMIDLIST*)&shei.lpIDList);
	}

	if (shei.lpIDList)
	{
		shei.cbSize = sizeof(shei);
		shei.fMask = SEE_MASK_IDLIST;
		shei.nShow = SW_SHOWNORMAL;
		shei.lpVerb = fOpen ? TEXT("open") : TEXT("explore");
		hr = ShellExecuteEx(&shei) ? S_OK : E_FAIL;
		ILFree((LPITEMIDLIST)shei.lpIDList);
	}

	return hr;
}

HRESULT CStartMenuCallback::_ExecHmenuItem(LPSMDATA psmd)
{
	HRESULT hr = S_FALSE;
	if (IsInRange(psmd->uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST) && _pcmFind)
	{
		CMINVOKECOMMANDINFOEX ici = { 0 };
		ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
		ici.lpVerb = (LPSTR)MAKEINTRESOURCE(psmd->uId - TRAY_IDM_FINDFIRST);
		ici.nShow = SW_NORMAL;

		// record if shift or control was being held down
		SetICIKeyModifiers(&ici.fMask);

		_pcmFind->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
		hr = S_OK;
	}
	else
	{
		switch (psmd->uId)
		{
		case IDM_OPEN_FOLDER:
			switch (psmd->uIdParent)
			{
			case IDM_CONTROLS:
				hr = ShowFolder(CSIDL_CONTROLS);
				break;

			case IDM_PRINTERS:
				hr = ShowFolder(CSIDL_PRINTERS);
				break;

			case IDM_NETCONNECT:
				hr = ShowFolder(CSIDL_CONNECTIONS);
				break;

			case IDM_MYPICTURES:
				hr = ShowFolder(CSIDL_MYPICTURES);
				break;

			case IDM_MYDOCUMENTS:
				hr = ShowFolder(CSIDL_PERSONAL);
				break;
			}
			break;

		case IDM_NETCONNECT:
			hr = ShowFolder(CSIDL_CONNECTIONS);
			break;

		case IDM_MYDOCUMENTS:
			hr = ShowFolder(CSIDL_PERSONAL);
			break;

		case IDM_MYPICTURES:
			hr = ShowFolder(CSIDL_MYPICTURES);
			break;

		case IDM_CSC:
			_ExecRegValue(TEXT("StartMenuSyncAll"));
			break;

		default:
			hr = ExecStaticStartMenuItem(psmd->uId, FALSE, TRUE);
			break;
		}
	}
	return hr;
}

typedef struct
{
	BITBOOL _fInitialized;
} SMUSERDATA;
typedef char TBOOL;              
HRESULT CStartMenuCallback::_Init(SMDATA* psmdata)
{
	HRESULT hr = S_FALSE;
	IShellMenu* psm;
	if (psmdata->punk && SUCCEEDED(hr = psmdata->punk->QueryInterface(IID_PPV_ARGS(&psm))))
	{
		switch (psmdata->uIdParent)
		{
		case IDM_TOPLEVELSTARTMENU:
		{
			if (psmdata->pvUserData && !((SMUSERDATA*)psmdata->pvUserData)->_fInitialized)
			{
				//TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : Initializing Toplevel Start Menu");
				//((SMUSERDATA*)psmdata->pvUserData)->_fInitialized = TRUE;

				HMENU hmenu;

				//TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : First Time, and correct parameters");

				_GetStaticStartMenu(&hmenu, &_hwnd);
				if (hmenu)
				{
					HMENU   hmenuOld = NULL;
					HWND    hwnd;
					DWORD   dwFlags;

					psm->GetMenu(&hmenuOld, &hwnd, &dwFlags);
					if (hmenuOld != NULL)
					{
						TBOOL(DestroyMenu(hmenuOld));
					}
					hr = psm->SetMenu(hmenu, _hwnd, SMSET_BOTTOM);
					//TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : SetMenu(HMENU 0x%x, HWND 0x%x", hmenu, _hwnd);
				}

				_fExpandoMenus = !_fIsStartPanel && AreIntelliMenusEnabled();
				_fCascadeMyDocuments = FeatureEnabled(TEXT("CascadeMyDocuments"));
				_fCascadePrinters = FeatureEnabled(TEXT("CascadePrinters"));
				_fCascadeControlPanel = FeatureEnabled(TEXT("CascadeControlPanel"));
				_fCascadeNetConnections = FeatureEnabled(TEXT("CascadeNetworkConnections"));
				_fAddOpenFolder = FeatureEnabled(TEXT("StartMenuOpen"));
				_fShowInfoTip = FeatureEnabled(TEXT("ShowInfoTip"));
				_fCascadeMyPictures = FeatureEnabled(TEXT("CascadeMyPictures"));
				_dwFlags = GetInitFlags();
			}
			else if (!_fInitedShowTopLevelStartMenu)
			{
				_fInitedShowTopLevelStartMenu = TRUE;
				psm->InvalidateItem(NULL, SMINV_REFRESH);
			}

			// Verify that the Fast items is still pointing to the right location
			if (SUCCEEDED(hr))
			{
				hr = VerifyMergedGuy(FALSE, psm);
			}
			break;
		}
		case IDM_MENU_FIND:
			if (_fFindMenuInvalid)
			{
				hr = _InitializeFindMenu(psm);
				_fFindMenuInvalid = FALSE;
			}
			break;

		case IDM_PROGRAMS:
			// Verify the programs menu is still pointing to the right location
			hr = VerifyMergedGuy(TRUE, psm);
			break;

		case IDM_FAVORITES:
			hr = VerifyCSIDL(IDM_FAVORITES, CSIDL_FAVORITES, psm);
			break;

		case IDM_MYDOCUMENTS:
			hr = VerifyCSIDL(IDM_MYDOCUMENTS, CSIDL_PERSONAL, psm);
			break;

		case IDM_MYPICTURES:
			hr = VerifyCSIDL(IDM_MYPICTURES, CSIDL_MYPICTURES, psm);
			break;

		case IDM_RECENT:
			_UpdateDocumentsShellMenu(psm);
			_UpdateDocsMenuItemNames(psm);
			hr = VerifyCSIDL(IDM_RECENT, CSIDL_RECENT, psm);
			break;
		case IDM_CONTROLS:
			hr = VerifyCSIDL(IDM_CONTROLS, CSIDL_CONTROLS, psm);
			break;
		case IDM_PRINTERS:
			hr = VerifyCSIDL(IDM_PRINTERS, CSIDL_PRINTERS, psm);
			break;
		}

		psm->Release();
	}

	return hr;
}

HRESULT CStartMenuCallback::_Create(SMDATA* psmdata, void** ppvUserData)
{
	*ppvUserData = new SMUSERDATA;

	return S_OK;
}

HRESULT CStartMenuCallback::_Destroy(SMDATA* psmdata)
{
	if (psmdata->pvUserData)
	{
		delete (SMUSERDATA*)psmdata->pvUserData;
		psmdata->pvUserData = NULL;
	}

	return S_OK;
}

#define IDM_FILERUN                 401
#define IDM_LOGOFF                  402
#define IDM_EJECTPC                 410
#define IDM_SETTINGSASSIST          411
#define IDM_TRAYPROPERTIES          413
#define IDM_UPDATEWIZARD            414
#define IDM_UPDATE_SEP              415
#define IDM_MU_DISCONNECT           5000
#define IDM_MU_SECURITY             5001

#define IDI_DVDDRIVE                291
#define IDI_MEDIACDAUDIOPLUS        292
#define IDI_MEDIACDEMPTY            293
#define IDI_MEDIACDROM              294
#define IDI_MEDIACDR                295
#define IDI_MEDIACDRW               296
#define IDI_MEDIADVDRAM             297
#define IDI_MEDIADVDR               298
#define IDI_AUDIOPLAYER             299
#define IDI_DEVICETAPEDRIVE         300
#define IDI_DEVICEOPTICALDRIVE      301
#define IDI_MEDIABLANKCD            302
#define IDI_MEDIACOMPFLASH          303
#define IDI_MEDIADVDROM             304
#define IDI_MEDIAMEMSTICK           305
#define IDI_MEDIAPCMCIA             306
#define IDI_MEDIASECUREDIGITALMEDIA 307
#define IDI_MEDIASMARTMEDIA         308
#define IDI_DEVICECAMERA            309
#define IDI_DEVICECELLPHONE         310
#define IDI_DEVICEHTTPPRINT         311
#define IDI_DEVICEJAZDRIVE          312
#define IDI_DEVICEZIPDRIVE          313
#define IDI_DEVICEPOCKETPC          314
#define IDI_DEVICESCANNER           315
#define IDI_DEVICESTI               316
#define IDI_DEVICEVIDEOCAM          317
#define IDI_MEDIADVDRW              318
#define IDI_TASK_NEWFOLDER          319
#define IDI_TASK_SENDTOCD           320
#define IDI_CPTASK_32CPLS           321
#define IDI_CLASSICSM_FAVORITES     322
#define IDI_CLASSICSM_FIND          323
#define IDI_CLASSICSM_HELP          324
#define IDI_CLASSICSM_LOGOFF        325
#define IDI_CLASSICSM_PROGS         326
#define IDI_CLASSICSM_RECENTDOCS    327
#define IDI_CLASSICSM_RUN           328
#define IDI_CLASSICSM_SHUTDOWN      329
#define IDI_CLASSICSM_SETTINGS      330
#define IDI_CLASSICSM_UNDOCK        331
#define IDI_TASK_SEARCHDS           337
#define IDI_NONE                    338
#define II_MU_STSECURITY     47
#define II_MU_STDISCONN      48

#define II_STCPANEL          35
#define II_STSPROGS          36
#define II_STPRNTRS          37
#define II_STFONTS           38
#define II_STTASKBR          39

#define II_CDAUDIO           40
#define II_TREE              41
#define II_STCPROGS          42
#define II_STFAVORITES       43
#define II_STLOGOFF          44
#define II_STFLDRPROP        45
#define II_WINUPDATE         46

#define IDI_MYDOCS                      100
#define IDI_MYPICS                      101
#define IDI_CSC                 179     // ClientSideCaching
#define IDI_NETCONNECT          175

typedef struct
{
	WCHAR wszMenuText[MAX_PATH];
	WCHAR wszHelpText[MAX_PATH];
	int   iIcon;
} SEARCHEXTDATA, * LPSEARCHEXTDATA;
#define IS_VALID_WRITE_PTR(ptr, type) \
   (IsBadWritePtr((PVOID)(ptr), sizeof(type)) ? \
    (dbgprintf(L"invalid %s write pointer - %#08lx", (PCSTR)"P"#type, (ptr)), FALSE) : \
    TRUE)
HRESULT CStartMenuCallback::_GetHmenuInfo(SMDATA* psmd, SMINFO* psminfo)
{
	if (!psminfo || IsBadReadPtr(psminfo, 8))
		return E_FAIL;

	const static struct
	{
		UINT idCmd;
		int  iImage;
	} s_mpcmdimg[] = { // Top level menu
					   { IDM_PROGRAMS,       -IDI_CLASSICSM_PROGS },
					   { IDM_FAVORITES,      -IDI_CLASSICSM_FAVORITES },
					   { IDM_RECENT,         -IDI_CLASSICSM_RECENTDOCS },
					   { IDM_SETTINGS,       -IDI_CLASSICSM_SETTINGS },
					   { IDM_MENU_FIND,      -IDI_CLASSICSM_FIND },
					   { IDM_HELPSEARCH,     -IDI_CLASSICSM_HELP },
					   { IDM_FILERUN,        -IDI_CLASSICSM_RUN },
					   { IDM_LOGOFF,         -IDI_CLASSICSM_LOGOFF },
					   { IDM_EJECTPC,        -IDI_CLASSICSM_UNDOCK },
					   { IDM_EXITWIN,        -IDI_CLASSICSM_SHUTDOWN },
					   { IDM_MU_SECURITY,    II_MU_STSECURITY },
					   { IDM_MU_DISCONNECT,  II_MU_STDISCONN  },
					   { IDM_SETTINGSASSIST, -IDI_CLASSICSM_SETTINGS },
					   { IDM_CONTROLS,       II_STCPANEL },
					   { IDM_PRINTERS,       II_STPRNTRS },
					   { IDM_TRAYPROPERTIES, II_STTASKBR },
					   { IDM_MYDOCUMENTS,    -IDI_MYDOCS},
					   { IDM_CSC,            -IDI_CSC},
					   { IDM_NETCONNECT,     -IDI_NETCONNECT},
	};


	assert(IS_VALID_WRITE_PTR(psminfo, SMINFO));

	int iIcon = -1;
	DWORD dwFlags = psminfo->dwFlags;
	MENUITEMINFO mii = { 0 };
	HRESULT hr = S_FALSE;

	if (psminfo->dwMask & SMIM_ICON)
	{
		if (IsInRange(psmd->uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST))
		{
			// The find menu extensions pack their icon into their data member of
			// Menuiteminfo....
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA;
			if (GetMenuItemInfo(psmd->hmenu, psmd->uId, MF_BYCOMMAND, &mii))
			{
				LPSEARCHEXTDATA psed = (LPSEARCHEXTDATA)mii.dwItemData;

				if (psed && !IsBadReadPtr(psed,8))
					psminfo->iIcon = psed->iIcon;
				else
					psminfo->iIcon = -1;

				dwFlags |= SMIF_ICON;
				hr = S_OK;
			}
		}
		else
		{
			if (psmd->uId == IDM_MYPICTURES)
			{
				LPITEMIDLIST pidlMyPics = SHCloneSpecialIDList(NULL, CSIDL_MYPICTURES, FALSE);
				if (pidlMyPics)
				{
					LPCITEMIDLIST pidlObject;
					IShellFolder* psf;
					hr = SHBindToParent(pidlMyPics, IID_PPV_ARGS(&psf), &pidlObject);
					if (SUCCEEDED(hr))
					{
						SHMapPIDLToSystemImageListIndex(psf, pidlObject, &psminfo->iIcon);
						dwFlags |= SMIF_ICON;
						psf->Release();
					}
					ILFree(pidlMyPics);
				}
			}
			else
			{
				UINT uIdLocal = psmd->uId;
				if (uIdLocal == IDM_OPEN_FOLDER)
					uIdLocal = psmd->uIdAncestor;

				for (int i = 0; i < _ARRAYSIZE(s_mpcmdimg); i++)
				{
					if (s_mpcmdimg[i].idCmd == uIdLocal)
					{
						iIcon = s_mpcmdimg[i].iImage;
						break;
					}
				}

				if (iIcon != -1)
				{
					dwFlags |= SMIF_ICON;
					psminfo->iIcon = Shell_GetCachedImageIndex(TEXT("shell32.dll"), iIcon, 0);
					hr = S_OK;
				}
			}
		}
	}

	if (psminfo->dwMask & SMIM_FLAGS)
	{
		psminfo->dwFlags = dwFlags;

		if ((psmd->uId == IDM_CONTROLS && _fCascadeControlPanel) ||
			(psmd->uId == IDM_PRINTERS && _fCascadePrinters) ||
			(psmd->uId == IDM_MYDOCUMENTS && _fCascadeMyDocuments) ||
			(psmd->uId == IDM_NETCONNECT && _fCascadeNetConnections) ||
			(psmd->uId == IDM_MYPICTURES && _fCascadeMyPictures))
		{
			psminfo->dwFlags |= SMIF_SUBMENU;
			hr = S_OK;
		}
		else switch (psmd->uId)
		{
		case IDM_FAVORITES:
		case IDM_PROGRAMS:
			psminfo->dwFlags |= SMIF_DROPCASCADE;
			hr = S_OK;
			break;
		}
	}

	return hr;
}

class CStartContextMenu : IContextMenu
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	// IContextMenu
	STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
	STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT* pRes, LPSTR pszName, UINT cchMax);

	CStartContextMenu(int idCmd) : _idCmd(idCmd), _cRef(1) {};
private:
	int _cRef;
	virtual ~CStartContextMenu() {};

	int _idCmd;
};

STDMETHODIMP CStartContextMenu::QueryInterface(REFIID riid, void** ppvObj)
{

	static const QITAB qit[] =
	{
		QITABENT(CStartContextMenu, IContextMenu),
		{ 0 },
	};

	return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CStartContextMenu::AddRef(void)
{
	return ++_cRef;
}

STDMETHODIMP_(ULONG) CStartContextMenu::Release(void)
{
	assert(_cRef > 0);
	_cRef--;

	if (_cRef > 0)
		return _cRef;

	delete this;
	return 0;
}

HMENU SHLoadMenuPopup(HINSTANCE hinst, UINT id)
{
	static HMODULE shlwapi = LoadLibraryW(L"shlwapi.dll");
	static HMENU(__fastcall * fSHLoadMenuPopup)(HINSTANCE a1, unsigned __int16 a2) = (decltype(fSHLoadMenuPopup))GetProcAddress(shlwapi,MAKEINTRESOURCEA(177));

	return fSHLoadMenuPopup(hinst,id);
}

#define SMCM_STARTMENU_FIRST        0x5000
#define SMCM_OPEN                   (SMCM_STARTMENU_FIRST + 0)
#define SMCM_EXPLORE                (SMCM_STARTMENU_FIRST + 1)
#define SMCM_OPEN_ALLUSERS          (SMCM_STARTMENU_FIRST + 2)
#define SMCM_EXPLORE_ALLUSERS       (SMCM_STARTMENU_FIRST + 3)
#define MENU_STARTMENUSTATICITEMS   359

STDAPI_(BOOL) _SHIsMenuSeparator2(HMENU hm, int i, BOOL* pbIsNamed)
{
	MENUITEMINFO mii;
	BOOL bLocal;

	if (!pbIsNamed)
		pbIsNamed = &bLocal;

	*pbIsNamed = FALSE;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.cch = 0;    // WARNING: We MUST initialize it to 0!!!
	if (GetMenuItemInfo(hm, i, TRUE, &mii) && (mii.fType & MFT_SEPARATOR))
	{
		// NOTE that there is a bug in either 95 or NT user!!!
		// 95 returns 16 bit ID's and NT 32 bit therefore there is a
		// the following may fail, on win9x, to evaluate to false
		// without casting
		*pbIsNamed = ((WORD)mii.wID != (WORD)-1);
		return TRUE;
	}
	return FALSE;
}

STDAPI_(void) _SHPrettyMenu(HMENU hm)
{
	BOOL bSeparated = TRUE;
	BOOL bWasNamed = TRUE;

	for (int i = GetMenuItemCount(hm) - 1; i > 0; --i)
	{
		BOOL bIsNamed;
		if (_SHIsMenuSeparator2(hm, i, &bIsNamed))
		{
			if (bSeparated)
			{
				// if we have two separators in a row, only one of which is named
				// remove the non named one!
				if (bIsNamed && !bWasNamed)
				{
					DeleteMenu(hm, i + 1, MF_BYPOSITION);
					bWasNamed = bIsNamed;
				}
				else
				{
					DeleteMenu(hm, i, MF_BYPOSITION);
				}
			}
			else
			{
				bWasNamed = bIsNamed;
				bSeparated = TRUE;
			}
		}
		else
		{
			bSeparated = FALSE;
		}
	}

	// The above loop does not handle the case of many separators at
	// the beginning of the menu
	while (_SHIsMenuSeparator2(hm, 0, NULL))
	{
		DeleteMenu(hm, 0, MF_BYPOSITION);
	}
}
#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))
// IContextMenu
STDMETHODIMP CStartContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	HRESULT hr = E_FAIL;

	//todo: get this popup
	//HMENU hmenuStartMenu = 0;

	HMENU hmenuStartMenu = SHLoadMenuPopup(LoadLibraryW(L"shellxp.dll"), MENU_STARTMENUSTATICITEMS);

	if (hmenuStartMenu)
	{
		TCHAR szCommon[MAX_PATH];
		BOOL fAddCommon = (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU, NULL, 0, szCommon));
	
		if (fAddCommon)
			fAddCommon = IsUserAnAdmin();
	
		// Since we don't show this on the start button when the user is not an admin, don't show it here... I guess...
		if (_idCmd != IDM_PROGRAMS || !fAddCommon)
		{
			DeleteMenu(hmenuStartMenu, SMCM_OPEN_ALLUSERS, MF_BYCOMMAND);
			DeleteMenu(hmenuStartMenu, SMCM_EXPLORE_ALLUSERS, MF_BYCOMMAND);
		}
	
		if (Shell_MergeMenus(hmenu, hmenuStartMenu, 0, indexMenu, idCmdLast, uFlags))
		{
			SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
			_SHPrettyMenu(hmenu);
			hr = ResultFromShort(GetMenuItemCount(hmenuStartMenu));
		}
	
		DestroyMenu(hmenuStartMenu);
	}

	return hr;
}
#define HIWORD64        HIWORD

#define SMCM_STARTMENU_FIRST        0x5000
#define SMCM_OPEN                   (SMCM_STARTMENU_FIRST + 0)
#define SMCM_EXPLORE                (SMCM_STARTMENU_FIRST + 1)
#define SMCM_OPEN_ALLUSERS          (SMCM_STARTMENU_FIRST + 2)
#define SMCM_EXPLORE_ALLUSERS       (SMCM_STARTMENU_FIRST + 3)

STDMETHODIMP CStartContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
	HRESULT hr = E_FAIL;
	if (HIWORD64(lpici->lpVerb) == 0)
	{
		BOOL fAllUsers = FALSE;
		BOOL fOpen = TRUE;
		switch (LOWORD(lpici->lpVerb))
		{
		case SMCM_OPEN_ALLUSERS:
			fAllUsers = TRUE;
		case SMCM_OPEN:
			// fOpen = TRUE;
			break;

		case SMCM_EXPLORE_ALLUSERS:
			fAllUsers = TRUE;
		case SMCM_EXPLORE:
			fOpen = FALSE;
			break;

		default:
			return S_FALSE;
		}

		hr = ExecStaticStartMenuItem(_idCmd, fAllUsers, fOpen);
	}

	// Ahhh Don't handle verbs!!!
	return hr;

}

STDMETHODIMP CStartContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pRes, LPSTR pszName, UINT cchMax)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_GetObject(LPSMDATA psmd, REFIID riid, void** ppvOut)
{
	HRESULT hr = E_FAIL;
	UINT    uId = psmd->uId;

	//ASSERT(ppvOut);
	//ASSERT(IS_VALID_READ_PTR(psmd, SMDATA));

	*ppvOut = NULL;

	if (IsEqualGUID(riid, IID_IShellMenu))
	{
		IShellMenu* psm = NULL;
		hr = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&psm));
		if (SUCCEEDED(hr))
		{
			hr = InitializeSubShellMenu(uId, psm);

			if (FAILED(hr))
			{
				psm->Release();
				psm = NULL;
			}
		}

		*ppvOut = psm;
	}
	else if (IsEqualGUID(riid, IID_IContextMenu))
	{
		//
		//  NOTE - we dont allow users to open the recent folder this way - ZekeL - 1-JUN-99
		//  because this is really an internal folder and not a user folder.
		//

		switch (uId)
		{
		case IDM_PROGRAMS:
		case IDM_FAVORITES:
		case IDM_MYDOCUMENTS:
		case IDM_MYPICTURES:
		case IDM_CONTROLS:
		case IDM_PRINTERS:
		case IDM_NETCONNECT:
		{
			CStartContextMenu* pcm = new CStartContextMenu(uId);
			if (pcm)
			{
				hr = pcm->QueryInterface(riid, ppvOut);
				pcm->Release();
			}
			else
				hr = E_OUTOFMEMORY;
		}
		}
	}
	return hr;
}

HRESULT CStartMenuCallback::_CheckRestricted(DWORD dwRestrict, BOOL* fRestricted)
{
	return E_NOTIMPL;
}
#define SHGetAttributesOf(pidl, prgfInOut) SHGetNameAndFlags(pidl, 0, NULL, 0, prgfInOut)

STDAPI SHCoInitialize(void)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
	{
		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
	}
	return hr;
}

HRESULT SHBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void** ppv, LPCITEMIDLIST* ppidlLast)
{
	return SHBindToFolderIDListParent(NULL, pidl, riid, ppv, ppidlLast);
}

enum {
	OBJCOMPATF_OTNEEDSSFCACHE = 0x00000001,
	OBJCOMPATF_NO_WEBVIEW = 0x00000002,
	OBJCOMPATF_UNBINDABLE = 0x00000004,
	OBJCOMPATF_PINDLL = 0x00000008,
	OBJCOMPATF_NEEDSFILESYSANCESTOR = 0x00000010,
	OBJCOMPATF_NOTAFILESYSTEM = 0x00000020,
	OBJCOMPATF_CTXMENU_NOVERBS = 0x00000040,
	OBJCOMPATF_CTXMENU_LIMITEDQI = 0x00000080,
	OBJCOMPATF_COCREATESHELLFOLDERONLY = 0x00000100,
	OBJCOMPATF_NEEDSSTORAGEANCESTOR = 0x00000200,
	OBJCOMPATF_NOLEGACYWEBVIEW = 0x00000400,
	OBJCOMPATF_BLOCKSHELLSERVICEOBJECT = 0x00000800,
};
typedef DWORD OBJCOMPATFLAGS;
typedef struct _CLSIDCOMPAT
{
	const GUID* pclsid;
	OBJCOMPATFLAGS flags;
}CLSIDCOMPAT, * PCLSIDCOMPAT;

STDAPI IUnknown_GetClassID(IUnknown* punk, CLSID* pclsid)
{
	static HMODULE shlwapi = LoadLibraryW(L"shlwapi.dll");
	static HRESULT(__fastcall * fIUnknown_GetClassID)(IUnknown * punk, CLSID * pclsid) = (decltype(fIUnknown_GetClassID))GetProcAddress(shlwapi, MAKEINTRESOURCEA(175));
	return fIUnknown_GetClassID(punk,pclsid);
}

static const GUID GUID_AECOZIPARCHIVE =
{ 0xE9779583, 0x939D, 0x11ce, { 0x8a, 0x77, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };
// {49707377-6974-6368-2E4A-756E6F644A01}
static const GUID CLSID_WS_FTP_PRO_EXPLORER =
{ 0x49707377, 0x6974, 0x6368, {0x2E, 0x4A,0x75, 0x6E, 0x6F, 0x64, 0x4A, 0x01} };
// {49707377-6974-6368-2E4A-756E6F644A0A}
static const GUID CLSID_WS_FTP_PRO =
{ 0x49707377, 0x6974, 0x6368, {0x2E, 0x4A,0x75, 0x6E, 0x6F, 0x64, 0x4A, 0x0A} };
// {2bbbb600-3f0a-11d1-8aeb-00c04fd28d85}
static const GUID CLSID_KODAK_DC260_ZOOM_CAMERA =
{ 0x2bbbb600, 0x3f0a, 0x11d1, {0x8a, 0xeb, 0x00, 0xc0, 0x4f, 0xd2, 0x8d, 0x85} };
// {00F43EE0-EB46-11D1-8443-444553540000}
static const GUID GUID_MACINDOS =
{ 0x00F43EE0, 0xEB46, 0x11D1, { 0x84, 0x43, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };
static const GUID CLSID_EasyZIP =
{ 0xD1069700, 0x932E, 0x11cf, { 0xAB, 0x59, 0x00, 0x60, 0x8C, 0xBF, 0x2C, 0xE0} };

static const GUID CLSID_PAGISPRO_FOLDER =
{ 0x7877C8E0, 0x8B13, 0x11D0, { 0x92, 0xC2, 0x00, 0xAA, 0x00, 0x4B, 0x25, 0x6F} };
// {61E285C0-DCF4-11cf-9FF4-444553540000}
static const GUID CLSID_FILENET_IDMDS_NEIGHBORHOOD =
{ 0x61e285c0, 0xdcf4, 0x11cf, { 0x9f, 0xf4, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

static const GUID CLSID_NOVELLX =
{ 0xb8777200, 0xd640, 0x11ce, { 0xb9, 0xaa, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

static const GUID CLSID_PGP50_CONTEXTMENU =  //{969223C0-26AA-11D0-90EE-444553540000}
{ 0x969223C0, 0x26AA, 0x11D0, { 0x90, 0xEE, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

static const GUID CLSID_QUICKFINDER_CONTEXTMENU = //  {CD949A20-BDC8-11CE-8919-00608C39D066}
{ 0xCD949A20, 0xBDC8, 0x11CE, { 0x89, 0x19, 0x00, 0x60, 0x8C, 0x39, 0xD0, 0x66} };

static const GUID CLSID_HERCULES_HCTNT_V1001 = // {921BD320-8CB5-11CF-84CF-885835D9DC01}
{ 0x921BD320, 0x8CB5, 0x11CF, { 0x84, 0xCF, 0x88, 0x58, 0x35, 0xD9, 0xDC, 0x01} };

typedef struct {
	DWORD flag;
	LPCTSTR psz;
} FLAGMAP;

DWORD _GetMappedFlags(HKEY hk, const FLAGMAP* pmaps, DWORD cmaps)
{
	DWORD dwRet = 0;
	for (DWORD i = 0; i < cmaps; i++)
	{
		if (NOERROR == SHGetValue(hk, NULL, pmaps[i].psz, NULL, NULL, NULL))
			dwRet |= pmaps[i].flag;
	}

	return dwRet;
}
#define OCFMAPPING(ocf)     {OBJCOMPATF_##ocf, TEXT(#ocf)}
#define ACFMAPPING(acf)     {ACF_##acf, TEXT(#acf)}

DWORD _GetRegistryObjectCompatFlags(REFGUID clsid)
{
	DWORD dwRet = 0;
	TCHAR szGuid[GUIDSTR_MAX];
	TCHAR sz[MAX_PATH];
	HKEY hk;

	StringFromCLSID(clsid, (LPOLESTR*)szGuid);
	wnsprintf(sz, ARRAYSIZE(sz), TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Objects\\%s"), szGuid);

	if (NOERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE, &hk))
	{
		static const FLAGMAP rgOcfMaps[] = {
			OCFMAPPING(OTNEEDSSFCACHE),
			OCFMAPPING(NO_WEBVIEW),
			OCFMAPPING(UNBINDABLE),
			OCFMAPPING(PINDLL),
			OCFMAPPING(NEEDSFILESYSANCESTOR),
			OCFMAPPING(NOTAFILESYSTEM),
			OCFMAPPING(CTXMENU_NOVERBS),
			OCFMAPPING(CTXMENU_LIMITEDQI),
			OCFMAPPING(COCREATESHELLFOLDERONLY),
			OCFMAPPING(NEEDSSTORAGEANCESTOR),
			OCFMAPPING(NOLEGACYWEBVIEW),
		};

		dwRet = _GetMappedFlags(hk, rgOcfMaps, ARRAYSIZE(rgOcfMaps));
		RegCloseKey(hk);
	}

	return dwRet;
}

STDAPI_(OBJCOMPATFLAGS) SHGetObjectCompatFlags(IUnknown* punk, const CLSID* pclsid)
{
	HRESULT hr = E_INVALIDARG;
	OBJCOMPATFLAGS ocf = 0;
	CLSID clsid;
	if (punk)
		hr = IUnknown_GetClassID(punk, &clsid);
	else if (pclsid)
	{
		clsid = *pclsid;
		hr = S_OK;
	}

	if (SUCCEEDED(hr))
	{
		static const CLSIDCOMPAT s_rgCompat[] =
		{
			{&CLSID_WS_FTP_PRO_EXPLORER,
				OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_PINDLL },
			{&CLSID_WS_FTP_PRO,
				OBJCOMPATF_UNBINDABLE},
			{&GUID_AECOZIPARCHIVE,
				OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_NO_WEBVIEW},
			{&CLSID_KODAK_DC260_ZOOM_CAMERA,
				OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_PINDLL},
			{&GUID_MACINDOS,
				OBJCOMPATF_NO_WEBVIEW},
			{&CLSID_EasyZIP,
				OBJCOMPATF_NO_WEBVIEW},
			{&CLSID_PAGISPRO_FOLDER,
				OBJCOMPATF_NEEDSFILESYSANCESTOR},
			{&CLSID_FILENET_IDMDS_NEIGHBORHOOD,
				OBJCOMPATF_NOTAFILESYSTEM},
			{&CLSID_NOVELLX,
				OBJCOMPATF_PINDLL},
			{&CLSID_PGP50_CONTEXTMENU,
				OBJCOMPATF_CTXMENU_LIMITEDQI},
			{&CLSID_QUICKFINDER_CONTEXTMENU,
				OBJCOMPATF_CTXMENU_NOVERBS},
			{&CLSID_HERCULES_HCTNT_V1001,
				OBJCOMPATF_PINDLL},
				//
				//  WARNING DONT ADD NEW COMPATIBILITY HERE - ZekeL - 18-OCT-99
				//  Add new entries to the registry.  each component 
				//  that needs compatibility flags should register 
				//  during selfregistration.  (see the RegExternal
				//  section of selfreg.inx in shell32 for an example.)  
				//  all new flags should be added to the FLAGMAP array.
				//
				//  the register under:
				//  HKLM\SW\MS\Win\CV\ShellCompatibility\Objects
				//      \{CLSID}
				//          FLAGNAME    //  requires no value
				//
				//  NOTE: there is no version checking
				//  but we could add it as the data attached to 
				//  the flags, and compare with the version 
				//  of the LocalServer32 dll.
				//  
				{NULL, 0}
		};

		for (int i = 0; s_rgCompat[i].pclsid; i++)
		{
			if (IsEqualGUID(clsid, *(s_rgCompat[i].pclsid)))
			{
				//  we could check version based
				//  on what is in under HKCR\CLSID\{clsid}
				ocf = s_rgCompat[i].flags;
				break;
			}
		}

		ocf |= _GetRegistryObjectCompatFlags(clsid);

	}

	return ocf;
}

STDAPI_(DWORD) SHGetAttributes(IShellFolder* psf, LPCITEMIDLIST pidl, DWORD dwAttribs)
{
	// like SHBindToObject, if psf is NULL, use absolute pidl
	LPCITEMIDLIST pidlChild;
	if (!psf)
	{
		SHBindToParent(pidl, IID_PPV_ARGS(&psf), &pidlChild);
	}
	else
	{
		psf->AddRef();
		pidlChild = pidl;
	}

	DWORD dw = 0;
	if (psf)
	{
		dw = dwAttribs;
		dw = SUCCEEDED(psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlChild, &dw)) ? (dwAttribs & dw) : 0;
		if ((dw & SFGAO_FOLDER) && (dw & SFGAO_CANMONIKER) && !(dw & SFGAO_STORAGEANCESTOR) && (dwAttribs & SFGAO_STORAGEANCESTOR))
		{
			if (OBJCOMPATF_NEEDSSTORAGEANCESTOR & SHGetObjectCompatFlags(psf, NULL))
			{
				//  switch SFGAO_CANMONIKER -> SFGAO_STORAGEANCESTOR
				dw |= SFGAO_STORAGEANCESTOR;
				dw &= ~SFGAO_CANMONIKER;
			}
		}
	}

	if (psf)
	{
		psf->Release();
	}

	return dw;
}
#define SHCoUninitialize(hr) if (SUCCEEDED(hr)) CoUninitialize()
HRESULT SHGetNameAndFlags(LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName, UINT cchName, DWORD* pdwAttribs)
{
	if (pszName)
	{
		//VDATEINPUTBUF(pszName, TCHAR, cchName);
		*pszName = 0;
	}

	HRESULT hrInit = SHCoInitialize();

	IShellFolder* psf;
	LPCITEMIDLIST pidlLast;
	HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARGS(&psf), &pidlLast);
	if (SUCCEEDED(hr))
	{
		if (pszName)
			hr = DisplayNameOf(psf, pidlLast, dwFlags, pszName, cchName);

		if (SUCCEEDED(hr) && pdwAttribs)
		{
			//RIP(*pdwAttribs);    // this is an in-out param
			*pdwAttribs = SHGetAttributes(psf, pidlLast, *pdwAttribs);
		}

		psf->Release();
	}

	SHCoUninitialize(hrInit);
	return hr;
}

BOOL LinkGetInnerPidl(IShellFolder* psf, LPCITEMIDLIST pidl, LPITEMIDLIST* ppidlOut, DWORD* pdwAttr)
{
	*ppidlOut = NULL;

	IShellLink* psl;
	HRESULT hr = psf->GetUIObjectOf(NULL, 1, &pidl, __uuidof(**(&psl)), 0, IID_PPV_ARGS_Helper(&psl));
	if (SUCCEEDED(hr))
	{
		psl->GetIDList(ppidlOut);

		if (*ppidlOut)
		{
			if (FAILED(SHGetAttributesOf(*ppidlOut, pdwAttr)))
			{
				ILFree(*ppidlOut);
				*ppidlOut = NULL;
			}
		}
		psl->Release();
	}
	return (*ppidlOut != NULL);
}
#define MAXRECENTDOCS 15
HRESULT CStartMenuCallback::_FilterRecentPidl(IShellFolder* psf, LPCITEMIDLIST pidl)
{
	HRESULT hr = S_OK;

	//ASSERT(IS_VALID_PIDL(pidl));
	//ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
	//ASSERT(_cRecentDocs != -1);
	//
	//ASSERT(_cRecentDocs <= MAXRECENTDOCS);

	//  if we already reached our limit, dont go over...
	if (_pmruRecent && (_cRecentDocs < MAXRECENTDOCS))
	{
		//  we now must take a looksee for it...
		int iItem;
		DWORD dwAttr = SFGAO_FOLDER | SFGAO_BROWSABLE;
		LPITEMIDLIST pidlTrue;

		//  need to find out if the link points to a folder...
		//  because we dont want
		if (SUCCEEDED(_pmruRecent->FindData((BYTE*)pidl, ILGetSize(pidl), &iItem))
			&& LinkGetInnerPidl(psf, pidl, &pidlTrue, &dwAttr))
		{
			if (!(dwAttr & SFGAO_FOLDER))
			{
				//  we have a link to something that isnt a folder 
				hr = S_FALSE;
				_cRecentDocs++;
			}

			ILFree(pidlTrue);
		}
	}

	//  return S_OK if you dont want to show this item...

	return hr;
}

HRESULT CStartMenuCallback::_Demote(LPSMDATA psmd)
{
	//We want to for the UEM to demote pidlFolder, 
	// then tell the Parent menuband (If there is one)
	// to invalidate this pidl.
	HRESULT hr = S_FALSE;

	if (_fExpandoMenus &&
		(psmd->uIdAncestor == IDM_PROGRAMS ||
			psmd->uIdAncestor == IDM_FAVORITES))
	{
		UEMINFO uei;
		uei.cbSize = sizeof(uei);
		uei.dwMask = UEIM_HIT;
		uei.R = 0;
		hr = UEMSetEvent(psmd->uIdAncestor == IDM_PROGRAMS ? &UEMIID_SHELL : &UEMIID_BROWSER, (WPARAM)psmd->psf, &uei);
	}
	return hr;
}
#define IDS_CHEVRONTIPTITLE     0x768F
#define IDS_CHEVRONTIP          0x7690
HRESULT CStartMenuCallback::_GetTip(LPWSTR pstrTitle, LPWSTR pstrTip)
{
	if (pstrTitle == NULL ||
		pstrTip == NULL)
	{
		return S_FALSE;
	}

	LoadString(GetModuleHandle(L"shellxp.dll"), IDS_CHEVRONTIPTITLE, pstrTitle, MAX_PATH);
	LoadString(GetModuleHandle(L"shellxp.dll"), IDS_CHEVRONTIP, pstrTip, MAX_PATH);

	// Why would this fail?
	//ASSERT(pstrTitle[0] != L'\0' && pstrTip[0] != L'\0');
	return S_OK;
}

DWORD CStartMenuCallback::_GetDemote(SMDATA* psmd)
{
	UEMINFO uei;
	DWORD dwFlags = 0;

	uei.cbSize = sizeof(uei);
	uei.dwMask = UEIM_HIT;
	if (SUCCEEDED(UEMQueryEvent(psmd->uIdAncestor == IDM_PROGRAMS ? &UEMIID_SHELL : &UEMIID_BROWSER, (WPARAM)psmd->psf,  &uei)))
	{
		if (uei.R == 0)
		{
			dwFlags |= SMIF_DEMOTED;
		}
	}

	return dwFlags;
}

HRESULT CStartMenuCallback::_HandleAccelerator(TCHAR ch, SMDATA* psmdata)
{
	// Since we renamed the 'Find' menu to 'Search' the PMs wanted to have
	// an upgrade path for users (So they can continue to use the old accelerator
	// on the new menu item.)
	// To enable this, when toolbar detects that there is not an item in the menu
	// that contains the key that has been pressed, then it sends a TBN_ACCL.
	// This is intercepted by mnbase, and translated into SMC_ACCEL. 
	if (CharUpper((LPTSTR)ch) == CharUpper((LPTSTR)_szFindMnemonic[0]))
	{
		psmdata->uId = IDM_MENU_FIND;
		return S_OK;
	}

	return S_FALSE;
}

HRESULT CStartMenuCallback::_GetDefaultIcon(LPWSTR psz, int* piIndex)
{
	DWORD cbSize = MAX_PATH;
	HRESULT hr = AssocQueryString(0, ASSOCSTR_DEFAULTICON, TEXT("InternetShortcut"), NULL, psz, &cbSize);
	if (SUCCEEDED(hr))
	{
		*piIndex = PathParseIconLocation(psz);
	}

	return hr;
}

void CStartMenuCallback::_GetStaticStartMenu(HMENU* phmenu, HWND* phwnd)
{
	*phmenu = NULL;
	*phwnd = NULL;

	IMenuPopup* pmp;
	// The first one should be the bar that the start menu is sitting in.
	if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_PPV_ARGS(&pmp))))
	{
		// Its site should be CStartMenuHost;
		if (SUCCEEDED(IUnknown_GetSite(pmp, IID_PPV_ARGS(&_ptp))))
		{
			// Don't get upset if this fails
			_ptp->QueryInterface(IID_PPV_ARGS(&_ptp2));

			_ptp->GetStaticStartMenu(phmenu);
			IUnknown_GetWindow(_ptp, phwnd);

			if (!_poct)
				_ptp->QueryInterface(IID_PPV_ARGS(&_poct));
		}
		//else
		//	TraceMsg(TF_MENUBAND, "CStartMenuCallback::_SetSite : Failed to aquire CStartMenuHost");

		pmp->Release();
	}
}
#define IDS_CONTROL_TIP         0x768A
#define IDS_PRINTERS_TIP        0x768B
#define IDS_TRAYPROP_TIP        0x768C
#define IDS_MYDOCS_TIP          0x768D
#define IDS_NETCONNECT_TIP      0x768E
#define IDS_MYPICS_TIP          0x76A5
HRESULT CStartMenuCallback::_GetStaticInfoTip(SMDATA* psmd, LPWSTR pszTip, int cch)
{
	if (!_fShowInfoTip)
		return E_FAIL;

	HRESULT hr = E_FAIL;

	const static struct
	{
		UINT idCmd;
		UINT idInfoTip;
	} s_mpcmdTip[] =
	{
#if 0   // No tips for the Toplevel. Keep this here because I bet that someone will want them...
	   { IDM_PROGRAMS,       IDS_PROGRAMS_TIP },
	   { IDM_FAVORITES,      IDS_FAVORITES_TIP },
	   { IDM_RECENT,         IDS_RECENT_TIP },
	   { IDM_SETTINGS,       IDS_SETTINGS_TIP },
	   { IDM_MENU_FIND,      IDS_FIND_TIP },
	   { IDM_HELPSEARCH,     IDS_HELP_TIP },        // Redundant?
	   { IDM_FILERUN,        IDS_RUN_TIP },
	   { IDM_LOGOFF,         IDS_LOGOFF_TIP },
	   { IDM_EJECTPC,        IDS_EJECT_TIP },
	   { IDM_EXITWIN,        IDS_SHUTDOWN_TIP },
#endif
	   // Settings Submenu
	   { IDM_CONTROLS,       IDS_CONTROL_TIP },
	   { IDM_PRINTERS,       IDS_PRINTERS_TIP },
	   { IDM_TRAYPROPERTIES, IDS_TRAYPROP_TIP },
	   { IDM_NETCONNECT,     IDS_NETCONNECT_TIP },

	   // Recent Folder
	   { IDM_MYDOCUMENTS,    IDS_MYDOCS_TIP },
	   { IDM_MYPICTURES,     IDS_MYPICS_TIP },
	};


	for (int i = 0; i < _ARRAYSIZE(s_mpcmdTip); i++)
	{
		if (s_mpcmdTip[i].idCmd == psmd->uId)
		{
			TCHAR szTip[MAX_PATH];
			if (LoadString(LoadLibraryW(L"shellxp.dll"), s_mpcmdTip[i].idInfoTip, szTip, ARRAYSIZE(szTip)))
			{
				SHTCharToUnicode(szTip, pszTip, cch);
				hr = S_OK;
			}
			break;
		}
	}

	return hr;
}

DWORD CStartMenuCallback::GetInitFlags()
{
	DWORD dwType;
	DWORD cbSize = sizeof(DWORD);
	DWORD dwFlags = 0;
	SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuInit"),
		&dwType, (BYTE*)&dwFlags, &cbSize);
	return dwFlags;
}

void CStartMenuCallback::SetInitFlags(DWORD dwFlags)
{
	//SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuInit"), REG_DWORD, &dwFlags, sizeof(DWORD));
}

HRESULT CStartMenuCallback::_InitializeFindMenu(IShellMenu* psm)
{
	HRESULT hr = E_FAIL;

	psm->Initialize(this, IDM_MENU_FIND, IDM_MENU_FIND, SMINIT_VERTICAL);

	HMENU hmenu = CreatePopupMenu();
	if (hmenu)
	{
		ATOMICRELEASE(_pcmFind);

		if (_ptp)
		{
			if (SUCCEEDED(_ptp->GetFindCM(hmenu, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST, &_pcmFind)))
			{
				IContextMenu2* pcm2;
				_pcmFind->QueryInterface(IID_PPV_ARGS(&pcm2));
				if (pcm2)
				{
					pcm2->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)hmenu, 0);
					pcm2->Release();
				}
			}

			if (_pcmFind)
			{
				hr = psm->SetMenu(hmenu, NULL, SMSET_TOP);
				// Don't Release _pcmFind
			}
		}

		// Since we failed to create the ShellMenu
		// we need to dispose of this HMENU
		if (FAILED(hr))
			DestroyMenu(hmenu);
	}

	return hr;
}

HRESULT CStartMenuCallback::_ExecItem(LPSMDATA psmd, UINT uMsg)
{
	assert(_dwThreadID == GetCurrentThreadId());
	return _ptp->ExecItem(psmd->psf, psmd->pidlItem);
}

HRESULT CStartMenuCallback::VerifyCSIDL(int idCmd, int csidl, IShellMenu* psm)
{
	DWORD dwFlags;
	LPITEMIDLIST pidl;
	IShellFolder* psf;
	HRESULT hr = S_OK;
	if (SUCCEEDED(psm->GetShellFolder(&dwFlags, &pidl, IID_PPV_ARGS(&psf))))
	{
		psf->Release();

		LPITEMIDLIST pidlCSIDL;
		if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidlCSIDL)))
		{
			// If the pidl of the IShellMenu is not equal to the
			// SpecialFolder Location, then we need to update it so they are...
			if (!ILIsEqual(pidlCSIDL, pidl))
			{
				hr = InitializeSubShellMenu(idCmd, psm);
			}
			ILFree(pidlCSIDL);
		}
		ILFree(pidl);
	}

	return hr;
}

HRESULT GetFilesystemInfo(IShellFolder* psf, LPITEMIDLIST* ppidlRoot, int* pcsidl)
{
	assert(psf);
	IPersistFolder3* ppf;
	HRESULT hr = E_FAIL;

	*pcsidl = 0;
	*ppidlRoot = 0;
	if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARGS(&ppf))))
	{
		PERSIST_FOLDER_TARGET_INFO pfti = { 0 };

		if (SUCCEEDED(ppf->GetFolderTargetInfo(&pfti)))
		{
			*pcsidl = pfti.csidl;
			if (-1 != pfti.csidl)
				hr = S_OK;

			ILFree(pfti.pidlTargetFolder);
		}

		if (SUCCEEDED(hr))
			hr = ppf->GetCurFolder(ppidlRoot);

		ppf->Release();
	}
	return hr;
}

HRESULT CStartMenuCallback::VerifyMergedGuy(BOOL fPrograms, IShellMenu* psm)
{
	DWORD dwFlags;
	LPITEMIDLIST pidl;
	HRESULT hr = S_OK;
	IAugmentedShellFolder* pasf;
	if (SUCCEEDED(psm->GetShellFolder(&dwFlags, &pidl, IID_PPV_ARGS(&pasf))))
	{
		IShellFolder* psf;
		// There are 2 things in the merged namespace: CSIDL_PROGRAMS and CSIDL_COMMON_PROGRAMS
		for (int i = 0; i < 2; i++)
		{
			if (SUCCEEDED(pasf->QueryNameSpace(i, 0, &psf)))
			{
				int csidl;
				LPITEMIDLIST pidlFolder;

				if (SUCCEEDED(GetFilesystemInfo(psf, &pidlFolder, &csidl)))
				{
					LPITEMIDLIST pidlCSIDL;
					if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidlCSIDL)))
					{
						// If the pidl of the IShellMenu is not equal to the
						// SpecialFolder Location, then we need to update it so they are...
						if (!ILIsEqual(pidlCSIDL, pidlFolder))
						{

							// Since one of these things has changed,
							// we need to update the string cache
							// so that we do proper filtering of 
							// the programs item.
							_fInitPrograms = FALSE;
							if (fPrograms)
								hr = InitializeProgramsShellMenu(psm);
							else
								hr = InitializeFastItemsShellMenu(psm);

							i = 100;   // break out of the loop.
						}
						ILFree(pidlCSIDL);
					}
					ILFree(pidlFolder);
				}
				psf->Release();
			}
		}

		ILFree(pidl);
		pasf->Release();
	}

	return hr;
}

STDAPI GetMyDocumentsDisplayName(LPTSTR pszPath, UINT cch)
{
	*pszPath = 0;
	LPITEMIDLIST pidl;
	if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl)))
	{
		SHGetNameAndFlags(pidl, SHGDN_NORMAL, pszPath, cch, NULL);
		ILFree(pidl);
	}
	return *pszPath ? S_OK : E_FAIL;
}

void _FixMenuItemName(IShellMenu* psm, UINT uID, LPTSTR pszNewMenuName)
{
	HMENU hMenu;
	assert(NULL != psm);
	if (SUCCEEDED(psm->GetMenu(&hMenu, NULL, NULL)))
	{
		MENUITEMINFO mii = { 0 };
		TCHAR szMenuName[256];
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_TYPE;
		mii.dwTypeData = szMenuName;
		mii.cch = ARRAYSIZE(szMenuName);
		szMenuName[0] = TEXT('\0');
		if (::GetMenuItemInfo(hMenu, uID, FALSE, &mii))
		{
			if (0 != StrCmp(szMenuName, pszNewMenuName))
			{
				// The mydocs name has changed, update the menu item:
				mii.dwTypeData = pszNewMenuName;
				if (::SetMenuItemInfo(hMenu, uID, FALSE, &mii))
				{
					SMDATA smd;
					smd.dwMask = SMDM_HMENU;
					smd.uId = uID;
					psm->InvalidateItem(&smd, SMINV_ID | SMINV_REFRESH);
				}
			}
		}
	}
}

HRESULT GetMyPicsDisplayName(LPTSTR pszBuffer, UINT cchBuffer)
{
	LPITEMIDLIST pidlMyPics = SHCloneSpecialIDList(NULL, CSIDL_MYPICTURES, FALSE);
	if (pidlMyPics)
	{
		HRESULT hRet = SHGetNameAndFlags(pidlMyPics, SHGDN_NORMAL, pszBuffer, cchBuffer, NULL);
		ILFree(pidlMyPics);
		return hRet;
	}
	return E_FAIL;
}

void CStartMenuCallback::_UpdateDocsMenuItemNames(IShellMenu* psm)
{
	TCHAR szBuffer[MAX_PATH];

	if (_fHasMyDocuments)
	{
		if (SUCCEEDED(GetMyDocumentsDisplayName(szBuffer, _ARRAYSIZE(szBuffer))))
			_FixMenuItemName(psm, IDM_MYDOCUMENTS, szBuffer);
	}

	if (_fHasMyPictures)
	{
		if (SUCCEEDED(GetMyPicsDisplayName(szBuffer, _ARRAYSIZE(szBuffer))))
			_FixMenuItemName(psm, IDM_MYPICTURES, szBuffer);
	}
}

#define MENU_STARTMENU_MYDOCS           401

void CStartMenuCallback::_UpdateDocumentsShellMenu(IShellMenu* psm)
{
	// Add/Remove My Documents and My Pictures items of menu

	BOOL fMyDocs = !SHRestricted(REST_NOSMMYDOCS);
	if (fMyDocs)
	{
		LPITEMIDLIST pidl;
		fMyDocs = SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl));
		if (fMyDocs)
			ILFree(pidl);
	}

	BOOL fMyPics = !SHRestricted(REST_NOSMMYPICS);
	if (fMyPics)
	{
		LPITEMIDLIST pidl;
		fMyPics = SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_MYPICTURES, NULL, 0, &pidl));
		if (fMyPics)
			ILFree(pidl);
	}

	// Do not update menu if not different than currently have
	if (fMyDocs != (BOOL)_fHasMyDocuments || fMyPics != (BOOL)_fHasMyPictures)
	{
		HMENU hMenu = SHLoadMenuPopup(LoadLibraryW(L"shellxp.dll"), MENU_STARTMENU_MYDOCS);
		if (hMenu)
		{
			if (!fMyDocs)
				DeleteMenu(hMenu, IDM_MYDOCUMENTS, MF_BYCOMMAND);
			if (!fMyPics)
				DeleteMenu(hMenu, IDM_MYPICTURES, MF_BYCOMMAND);
			// Reset section of menu
			psm->SetMenu(hMenu, _hwnd, SMSET_TOP);
		}

		// Cache what folders are available
		_fHasMyDocuments = fMyDocs;
		_fHasMyPictures = fMyPics;
	}
}
#define SMSET_NOEMPTY               0x00000004
HRESULT CStartMenuCallback::InitializeFastItemsShellMenu(IShellMenu* psm)
{
	DWORD dwFlags = SMINIT_TOPLEVEL | SMINIT_VERTICAL;

	if (IsStartMenuChangeNotAllowed(_fIsStartPanel))
		dwFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

	HRESULT hr = psm->Initialize(this, 0, ANCESTORDEFAULT, dwFlags);
	if (SUCCEEDED(hr))
	{
		_InitializePrograms();

		// Add the fast item folder to the top of the menu
		IShellFolder* psfFast;
		LPITEMIDLIST pidlFast;
		//hr = GetMergedFolder(&psfFast, &pidlFast, c_rgmfiStartMenu, 2);
		hr = GetFolderAndPidl(CSIDL_STARTMENU, &psfFast, &pidlFast);

		if (!psfFast)
			MessageBox(0,L"psfFast",L"psfFast",0);
		if (SUCCEEDED(hr))
		{
			HKEY hMenuKey = NULL;   // WARNING: pmb2->Initialize() will always owns hMenuKey, so don't close it

			RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU, NULL, NULL,
				REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
				NULL, &hMenuKey, NULL);

			//TraceMsg(TF_MENUBAND, "Root Start Menu Key Is %d", hMenuKey);
			hr = psm->SetShellFolder(psfFast, pidlFast, hMenuKey, SMSET_TOP | SMSET_NOEMPTY);

			psfFast->Release();
			ILFree(pidlFast);
		}
	}

	return hr;
}


#define MENU_STARTMENU_OPENFOLDER       402
HRESULT CStartMenuCallback::InitializeCSIDLShellMenu(int uId, int csidl, LPTSTR pszRoot, LPTSTR pszValue, DWORD dwPassInitFlags, DWORD dwSetFlags, BOOL fAddOpen, IShellMenu* psm)
{
	DWORD dwInitFlags = SMINIT_VERTICAL | dwPassInitFlags;

	if (IsStartMenuChangeNotAllowed(_fIsStartPanel))
		dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

	psm->Initialize(this, uId, uId, dwInitFlags);

	LPITEMIDLIST pidl;
	IShellFolder* psfFolder;
	HRESULT hr = GetFolderAndPidl(csidl, &psfFolder, &pidl);
	if (SUCCEEDED(hr))
	{
		HKEY hKey = NULL;

		if (pszRoot)
		{
			TCHAR szPath[MAX_PATH];
			StrCpyN(szPath, pszRoot, ARRAYSIZE(szPath));
			if (pszValue)
			{
				PathAppend(szPath, pszValue);
			}

			RegCreateKeyEx(HKEY_CURRENT_USER, szPath, NULL, NULL,
				REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
				NULL, &hKey, NULL);
		}

		// Point the menu to the shellfolder
		hr = psm->SetShellFolder(psfFolder, pidl, hKey, dwSetFlags);
		if (SUCCEEDED(hr))
		{
			if (fAddOpen && _fAddOpenFolder)
			{
				HMENU hMenu = SHLoadMenuPopup(LoadLibraryW(L"shellxp.dll"), MENU_STARTMENU_OPENFOLDER);
				if (hMenu)
				{
					psm->SetMenu(hMenu, _hwnd, SMSET_BOTTOM);
				}
			}
		}
		else
			RegCloseKey(hKey);

		psfFolder->Release();
		ILFree(pidl);
	}

	return hr;
}

HRESULT CStartMenuCallback::InitializeDocumentsShellMenu(IShellMenu* psm)
{
	HRESULT hr = InitializeCSIDLShellMenu(IDM_RECENT, CSIDL_RECENT, NULL, NULL,
		SMINIT_RESTRICT_DRAGDROP, SMSET_BOTTOM, FALSE,
		psm);

	// Initializing, reset cache bits for top part of menu
	_fHasMyDocuments = FALSE;
	_fHasMyPictures = FALSE;

	return hr;
}
#define STRREG_FAVORITES TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Favorites")

#define SMSET_USEBKICONEXTRACTION   0x00000008   // Use the background icon extractor
#define SMSET_HASEXPANDABLEFOLDERS  0x00000010   // Need to call SHIsExpandableFolder
HRESULT CStartMenuCallback::InitializeSubShellMenu(int idCmd, IShellMenu* psm)
{
	HRESULT hr = E_FAIL;

	switch (idCmd)
	{
	case IDM_PROGRAMS:
		hr = InitializeProgramsShellMenu(psm);
		break;

	case IDM_RECENT:
		hr = InitializeDocumentsShellMenu(psm);
		break;

	case IDM_MENU_FIND:
		hr = _InitializeFindMenu(psm);
		break;

	case IDM_FAVORITES:
		hr = InitializeCSIDLShellMenu(IDM_FAVORITES, CSIDL_FAVORITES, STRREG_FAVORITES,
			NULL, 0, SMSET_HASEXPANDABLEFOLDERS | SMSET_USEBKICONEXTRACTION, FALSE,
			psm);
		break;

	case IDM_CONTROLS:
		hr = InitializeCSIDLShellMenu(IDM_CONTROLS, CSIDL_CONTROLS, STRREG_STARTMENU,
			TEXT("ControlPanel"), 0, 0, TRUE,
			psm);
		break;

	case IDM_PRINTERS:
		hr = InitializeCSIDLShellMenu(IDM_PRINTERS, CSIDL_PRINTERS, STRREG_STARTMENU,
			TEXT("Printers"), 0, 0, TRUE,
			psm);
		break;

	case IDM_MYDOCUMENTS:
		hr = InitializeCSIDLShellMenu(IDM_MYDOCUMENTS, CSIDL_PERSONAL, STRREG_STARTMENU,
			TEXT("MyDocuments"), 0, 0, TRUE,
			psm);
		break;

	case IDM_MYPICTURES:
		hr = InitializeCSIDLShellMenu(IDM_MYPICTURES, CSIDL_MYPICTURES, STRREG_STARTMENU,
			TEXT("MyPictures"), 0, 0, TRUE,
			psm);
		break;

	case IDM_NETCONNECT:
		hr = InitializeCSIDLShellMenu(IDM_NETCONNECT, CSIDL_CONNECTIONS, STRREG_STARTMENU,
			TEXT("NetConnections"), 0, 0, TRUE,
			psm);
		break;
	}

	return hr;
}
