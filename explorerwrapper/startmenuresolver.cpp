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
		_pszDescriptor = psz;
		//Str_SetPtr(&_pszDescriptor, psz);
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
	: _cRef(1), _fIsStartPanel(fIsStartPanel)
{
	_dwThreadID = GetCurrentThreadId();

	TCHAR szBuf[MAX_PATH];
	DWORD cbSize = sizeof(szBuf); // SHGetValue wants sizeof

	if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_EXPLORER_WINUPDATE, TEXT("ShortcutName"),
		NULL, szBuf, &cbSize))
	{
		// Add ".lnk" if the file doesn't have an extension
		PathAddExtension(szBuf, TEXT(".lnk"));
		//Str_SetPtr(&_pszWindowsUpdate, szBuf);
		_pszWindowsUpdate = szBuf;
	}

	cbSize = sizeof(szBuf); // SHGetValue wants sizeof
	if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, TEXT("SM_ConfigureProgramsName"),
		NULL, szBuf, &cbSize))
	{
		PathAddExtension(szBuf, TEXT(".lnk"));
		_pszConfigurePrograms = szBuf;
		//Str_SetPtr(&_pszConfigurePrograms, szBuf);
	}

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_ADMINTOOLS | CSIDL_FLAG_CREATE, NULL, 0, szBuf)))
	{
		_pszAdminTools = szBuf;
		//Str_SetPtr(&_pszAdminTools, PathFindFileName(szBuf));
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
		_pszPrograms = PathFindFileName(szTemp);
		//Str_SetPtr(&_pszPrograms, PathFindFileName(szTemp));

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

	assert(IS_VALID_PIDL(pidl));
	assert(IS_VALID_CODE_PTR(psf, IShellFolder));

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

HRESULT CStartMenuCallbackBase::_Promote(LPSMDATA psmd, DWORD dwFlags)
{
	//if ((_fExpandoMenus || (_fIsStartPanel && (dwFlags & SMINV_FORCE))) &&
	//	(psmd->uIdAncestor == IDM_PROGRAMS ||
	//		psmd->uIdAncestor == IDM_FAVORITES))
	//{
	//	UEMFireEvent(psmd->uIdAncestor == IDM_PROGRAMS ? &UEMIID_SHELL : &UEMIID_BROWSER,
	//		UEME_RUNPIDL, UEMF_XEVENT, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem);
	//}
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
	IAugmentedShellFolder2* pasf2;
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
#define SMSET_DONTREGISTERCHANGENOTIFY 0x00000020 // ShellFolder is a discontiguous child of a parent shell folder
HRESULT CStartMenuCallbackBase::InitializeProgramsShellMenu(IShellMenu* psm)
{
	HKEY hkeyPrograms = NULL;
	LPITEMIDLIST pidl = NULL;

	_fIsStartPanel = true;

	DWORD dwInitFlags = SMINIT_VERTICAL;
	if (!FeatureEnabled(_fIsStartPanel ? TEXT("Start_ScrollPrograms") : TEXT("StartMenuScrollPrograms")))
		dwInitFlags |= SMINIT_MULTICOLUMN;

	if (IsStartMenuChangeNotAllowed(_fIsStartPanel))
		dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

	if (_fIsStartPanel)
		dwInitFlags |= SMINIT_TOPLEVEL;

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
			dwSmset |= SMSET_SEPARATEMERGEFOLDER;
			hr = GetMergedFolder(&psf, &pidl, c_rgmfiProgramsFolderAndFastItems,
				4);
		}
		else
		{
			// Classic Start Menu:  The Programs section is just the per-user
			// and common Programs folders merged together
			hr = GetMergedFolder(&psf, &pidl, c_rgmfiProgramsFolder,
				2);

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

			IEnumIDList* pEnumIDList;
			HRESULT hr = psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
			int count = 0;
			LPITEMIDLIST pidl = NULL;
			while (pEnumIDList->Next(1, &pidl, NULL) == S_OK) {
				count++;
				CoTaskMemFree(pidl);  // Free the PIDL after use
			}
			pEnumIDList->Release();
			dbgprintf(L"count %i",count);

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

STDMETHODIMP_(HRESULT __stdcall) CStartMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CStartMenuCallback::SetSite(IUnknown* punk)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CStartMenuCallback::GetSite(REFIID riid, void** ppvOut)
{
	return E_NOTIMPL;
}

CStartMenuCallback::CStartMenuCallback()
{
}

CStartMenuCallback::~CStartMenuCallback()
{
}

HRESULT CStartMenuCallback::_ExecHmenuItem(LPSMDATA psmdata)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_Init(SMDATA* psmdata)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_Create(SMDATA* psmdata, void** pvUserData)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_Destroy(SMDATA* psmdata)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_GetHmenuInfo(SMDATA* psmd, SMINFO* sminfo)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_GetObject(LPSMDATA psmd, REFIID riid, void** ppvObj)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_CheckRestricted(DWORD dwRestrict, BOOL* fRestricted)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_FilterRecentPidl(IShellFolder* psf, LPCITEMIDLIST pidl)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_Demote(LPSMDATA psmd)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_GetTip(LPWSTR pstrTitle, LPWSTR pstrTip)
{
	return E_NOTIMPL;
}

DWORD CStartMenuCallback::_GetDemote(SMDATA* psmd)
{
	return 0;
}

HRESULT CStartMenuCallback::_HandleAccelerator(TCHAR ch, SMDATA* psmdata)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_GetDefaultIcon(LPWSTR psz, int* piIndex)
{
	return E_NOTIMPL;
}

void CStartMenuCallback::_GetStaticStartMenu(HMENU* phmenu, HWND* phwnd)
{
}

HRESULT CStartMenuCallback::_GetStaticInfoTip(SMDATA* psmd, LPWSTR pszTip, int cch)
{
	return E_NOTIMPL;
}

DWORD CStartMenuCallback::GetInitFlags()
{
	return 0;
}

void CStartMenuCallback::SetInitFlags(DWORD dwFlags)
{
}

HRESULT CStartMenuCallback::_InitializeFindMenu(IShellMenu* psm)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::_ExecItem(LPSMDATA, UINT)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::VerifyCSIDL(int idCmd, int csidl, IShellMenu* psm)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::VerifyMergedGuy(BOOL fPrograms, IShellMenu* psm)
{
	return E_NOTIMPL;
}

void CStartMenuCallback::_UpdateDocsMenuItemNames(IShellMenu* psm)
{
}

void CStartMenuCallback::_UpdateDocumentsShellMenu(IShellMenu* psm)
{
}

HRESULT CStartMenuCallback::InitializeFastItemsShellMenu(IShellMenu* psm)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::InitializeCSIDLShellMenu(int uId, int csidl, LPTSTR pszRoot, LPTSTR pszValue, DWORD dwPassInitFlags, DWORD dwSetFlags, BOOL fAddOpen, IShellMenu* psm)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::InitializeDocumentsShellMenu(IShellMenu* psm)
{
	return E_NOTIMPL;
}

HRESULT CStartMenuCallback::InitializeSubShellMenu(int idCmd, IShellMenu* psm)
{
	return E_NOTIMPL;
}
