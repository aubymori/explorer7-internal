#pragma once
#define INITGUID
#include "framework.h"
#include "appresolvernotify.h"
#include "EnumStartMenu.h"

#pragma region GUID definitions
DEFINE_GUID(CLSID_StartMenuCacheAndAppResolver, 0x660B90C8, 0x73A9, 0x4B58, 0x8C, 0xAE, 0x35, 0x5B, 0x7F, 0x55, 0x34, 0x1B);
DEFINE_GUID(IID_IAppResolver7, 0x46a6eeff, 0x908e, 0x4dc6, 0x92, 0xA6, 0x64, 0xbe, 0x91, 0x77, 0xb4, 0x1c); //46a6eeff_908e_4dc6_92a6_64be9177b41c
DEFINE_GUID(IID_IAppResolver8, 0xde25675a, 0x72de, 0x44b4, 0x93, 0x73, 0x05, 0x17, 0x04, 0x50, 0xc1, 0x40); //de25675a_72de_44b4_9373_05170450c140

DEFINE_GUID(IID_IStartMenuItemsCache7, 0x05a232fd, 0x2bfb, 0x4349, 0x9d, 0x48, 0x47, 0x87, 0xf3, 0x17, 0xf5, 0x0a); //05a232fd_2bfb_4349_9d48_4787f317f50a
DEFINE_GUID(IID_IStartMenuItemsCache8, 0x934332DD, 0x0B0FE, 0x41F9, 0x0BC, 0x63, 0x9C, 0x7F, 0x9F, 0x3C, 0x3A, 0x0EC); //_GUID_934332dd_b0fe_41f9_bc63_9c7f9f3c3aec
DEFINE_GUID(IID_IStartMenuItemsCache10, 0x0BA5A92AE, 0x0BFD7, 0x4916, 0x85, 0x4F, 0x6B, 0x3A, 0x40, 0x2B, 0x84, 0x0A8); //_GUID_ba5a92ae_bfd7_4916_854f_6b3a402b84a8
DEFINE_GUID(CLSID_MenuDeskBar, 0xECD4FC4FL, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);
//DEFINE_GUID(CLSID_StartMenu, 0x4622ad11, 0xff23, 0x11d0, 0x8d, 0x34, 0x0, 0xa0, 0xc9, 0xf, 0x27, 0x19);

DEFINE_GUID(IID_IStartMenuAppItems8, 0x2C5CCF3, 0x805F, 0x4654, 0x0A7, 0x0B7, 0x34, 0x0A, 0x74, 0x33, 0x53, 0x65); //02c5ccf3_805f_4654_a7b7_340a74335365
//DEFINE_GUID(SID_SMenuPopup, 0xD1E7AFEB, 0x6A2E, 0x11d0, 0x8C, 0x78, 0x0, 0xC0, 0x4F, 0xD9, 0x18, 0xB4);
DEFINE_GUID(CLSID_InternetToolbar, 0x5E6AB780L, 0x7743, 0x11CF, 0xA1, 0x2B, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

DEFINE_PROPERTYKEY(PKEY_AppUserModel_BestShortcut, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 10);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_HostEnvironment, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 14);
//DEFINE_PROPERTYKEY(PKEY_AppUserModel_IsDualMode, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 11);
#pragma endregion

MIDL_INTERFACE("46a6eeff-908e-4dc6-92a6-64be9177b41c")
IAppResolver7: public IUnknown
{
public:
	STDMETHOD(GetAppIDForShortcut)(IShellItem*, LPWSTR*) PURE;
	STDMETHOD(GetAppIDForWindow)(HWND*,DWORD*,DWORD*,DWORD*,DWORD*) PURE;
	STDMETHOD(GetAppIDForProcess)(ULONG_PTR,DWORD*,DWORD*,DWORD*,DWORD*) PURE;
	STDMETHOD(GetShortcutForProcess)(ULONG_PTR,IUnknown*) PURE;
	STDMETHOD(GetBestShortcutForAppID)(DWORD*,IUnknown*) PURE;
	STDMETHOD(GetBestShortcutAndAppIDForAppPath)(DWORD*,IUnknown*,DWORD*) PURE;
	STDMETHOD(CanPinApp)(IUnknown*) PURE;
	STDMETHOD(GetRelaunchProperties)(HWND*,DWORD*,DWORD*,DWORD*,DWORD*,DWORD*) PURE;
	STDMETHOD(GenerateShortcutFromWindowProperties)(HWND*,IUnknown*) PURE;
	STDMETHOD(GenerateShortcutFromItemProperties)(IUnknown*,IUnknown*) PURE;
};

MIDL_INTERFACE("de25675a-72de-44b4-9373-05170450c140")
IAppResolver8: public IUnknown
{
public:
	STDMETHOD(GetAppIDForShortcut)(IShellItem*, LPWSTR*) PURE;
	STDMETHOD(GetAppIDForShortcutObject)(IUnknown*, IUnknown*, DWORD*) PURE;
	STDMETHOD(GetAppIDForWindow)(HWND*,DWORD*,DWORD*,DWORD*,DWORD*) PURE;
	STDMETHOD(GetAppIDForProcess)(ULONG_PTR,DWORD*,DWORD*,DWORD*,DWORD*) PURE;
	STDMETHOD(GetShortcutForProcess)(ULONG_PTR,IUnknown*) PURE;
	STDMETHOD(GetBestShortcutForAppID)(DWORD*,IUnknown*) PURE;
	STDMETHOD(GetBestShortcutAndAppIDForAppPath)(DWORD*,IUnknown*,DWORD*) PURE;
	STDMETHOD(CanPinApp)(IUnknown*) PURE;
	STDMETHOD(CanPinAppShortcut)(IUnknown*, IUnknown*) PURE;
	STDMETHOD(GetRelaunchProperties)(HWND*,DWORD*,DWORD*,DWORD*,DWORD*,DWORD*, int* a7) PURE;
	STDMETHOD(GenerateShortcutFromWindowProperties)(HWND*,IUnknown*) PURE;
	STDMETHOD(GenerateShortcutFromItemProperties)(IUnknown*,IUnknown*) PURE;
};

MIDL_INTERFACE("05a232fd-2bfb-4349-9d48-4787f317f50a")
IStartMenuItemsCache7: public IUnknown
{
public:
	STDMETHOD(OnChangeNotify)(unsigned int,long,PVOID*,PVOID*) PURE;
	STDMETHOD(PinListChanged)(void) PURE;
	STDMETHOD(GetPinnedItemsCount)(int*) PURE;
	STDMETHOD(GetStartMenuMFUList)(unsigned int,IEnumStartMenuItem**,IEnumString**,FILETIME*) PURE;
	STDMETHOD(RegisterSMNotify)(IUnknown*) PURE;
	STDMETHOD(RegisterARNotify)(IUnknown*) PURE;
	STDMETHOD(SetAltName)(PVOID*,DWORD*,PVOID*) PURE;
	STDMETHOD(GetAltName)(PVOID*,DWORD*) PURE;
};

//MIDL_INTERFACE("bb9786b2-efe6-4f1e-a3bd-67f97d0085bf")
MIDL_INTERFACE("934332dd-b0fe-41f9-bc63-9c7f9f3c3aec")
IStartMenuItemsCache8: public IUnknown
{
public:
	STDMETHOD(OnChangeNotify)(unsigned int,long,PVOID*,PVOID*) PURE;
	STDMETHOD(RegisterForNotifications)(void*) PURE;
	STDMETHOD(UnregisterForNotifications)(void) PURE;
	STDMETHOD(PauseNotifications)(void) PURE;
	STDMETHOD(ResumeNotifications)(void) PURE;
	STDMETHOD(RegisterARNotify)(IUnknown*) PURE;
	STDMETHOD(RefreshCache)(int) PURE;
	STDMETHOD(ReleaseGlobalCacheObject)(void) PURE;
	STDMETHOD(IsCacheMatchingLanguage)(int*) PURE;
};
MIDL_INTERFACE("ba5a92ae-bfd7-4916-854f-6b3a402b84a8")
IStartMenuItemsCache10: public IUnknown
{
public:
	STDMETHOD(OnChangeNotify)(unsigned int,long,PVOID*,PVOID*) PURE;
	STDMETHOD(RegisterForNotifications)(void*) PURE;
	STDMETHOD(UnregisterForNotifications)(void) PURE;
	STDMETHOD(PauseNotifications)(void) PURE;
	STDMETHOD(ResumeNotifications)(void) PURE;
	STDMETHOD(RegisterARNotify)(IUnknown*) PURE;
	STDMETHOD(RefreshCache)(int) PURE;
	STDMETHOD(ReleaseGlobalCacheObject)(void) PURE;
	STDMETHOD(IsCacheMatchingLanguage)(int*) PURE;
	STDMETHOD(EnableAppUsageData)(void) PURE;
};

/*
CExtractConstIcon::AddRef(void)
CAppResolver::Release(void)
CAppResolver::OnChangeNotify(uint,long,_ITEMIDLIST_ABSOLUTE const *,_ITEMIDLIST_ABSOLUTE const *)
CAppResolver::RegisterForNotifications(IAppResolverProxy *)
CAppResolver::UnregisterForNotifications(void)
CAppResolver::PauseNotifications(void)
CAppResolver::ResumeNotifications(void)
CAppResolver::RegisterARNotify(IAppResolverNotify *)
CAppResolver::RefreshCache(START_MENU_REFRESH_CACHE_FLAGS)
CAppResolver::ReleaseGlobalCacheObject(void)
CAppResolver::IsCacheMatchingLanguage(int *)
*/

/*
?AddRef@CCommonParentUndoUnit@@WBA@EAAKXZ ; [thunk]:CCommonParentUndoUnit::AddRef`adjustor{16}' (void)
?Release@CAppResolver@@WBA@EAAKXZ ; [thunk]:CAppResolver::Release`adjustor{16}' (void)
?EnumItems@CAppResolver@@UEAAJW4START_MENU_APP_ITEMS_FLAGS@@AEBU_GUID@@PEAPEAX@Z ; CAppResolver::EnumItems(START_MENU_APP_ITEMS_FLAGS,_GUID const &,void * *)
?GetItem@CAppResolver@@UEAAJW4START_MENU_APP_ITEMS_FLAGS@@PEBGAEBU_GUID@@PEAPEAX@Z ; CAppResolver::GetItem(START_MENU_APP_ITEMS_FLAGS,ushort const *,_GUID const &,void * *)
?GetItemByAppPath@CAppResolver@@UEAAJPEBGAEBU_GUID@@PEAPEAX@Z ; CAppResolver::GetItemByAppPath(ushort const *,_GUID const &,void * *)
*/

//MIDL_INTERFACE("33f71155-c2e9-4ffe-9786-a32d98577cff")
MIDL_INTERFACE("02c5ccf3-805f-4654-a7b7-340a74335365")
IStartMenuAppItems8: public IUnknown
{
public:
	STDMETHOD(EnumItems)(int, REFIID, PVOID*) PURE;
	STDMETHOD(GetItem)(int, LPWSTR, const IID& riid, PVOID*) PURE;
	STDMETHOD(GetItemByAppPath)(const WCHAR*, _GUID const&, void**) PURE;
};

class CStartMenuResolver : public IAppResolver7, IStartMenuItemsCache7
{
public:
	CStartMenuResolver(IAppResolver8* newresolver);
	CStartMenuResolver(IStartMenuItemsCache8 *newcache);
	CStartMenuResolver(IStartMenuItemsCache10 *newcache);
	~CStartMenuResolver();

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG)  Release(void);

	//IAppResolver7
	STDMETHODIMP GetAppIDForShortcut(IShellItem*, LPWSTR*);
	STDMETHODIMP GetAppIDForWindow(HWND*, DWORD*, DWORD*, DWORD*, DWORD*);
	STDMETHODIMP GetAppIDForProcess(ULONG_PTR, DWORD*, DWORD*, DWORD*, DWORD*);
	STDMETHODIMP GetShortcutForProcess(ULONG_PTR, IUnknown*);
	STDMETHODIMP GetBestShortcutForAppID(DWORD*, IUnknown*);
	STDMETHODIMP GetBestShortcutAndAppIDForAppPath(DWORD*, IUnknown*, DWORD*);
	STDMETHODIMP CanPinApp(IUnknown*);
	STDMETHODIMP GetRelaunchProperties(HWND*, DWORD*, DWORD*, DWORD*, DWORD*, DWORD*);
	STDMETHODIMP GenerateShortcutFromWindowProperties(HWND*, IUnknown*);
	STDMETHODIMP GenerateShortcutFromItemProperties(IUnknown*, IUnknown*);

	//IStartMenuItemsCache7
	STDMETHODIMP OnChangeNotify(unsigned int, long, PVOID*, PVOID*);
	STDMETHODIMP PinListChanged(void);
	STDMETHODIMP GetPinnedItemsCount(int*);
	STDMETHODIMP GetStartMenuMFUList(unsigned int, IEnumStartMenuItem**, IEnumString**, FILETIME*);
	STDMETHODIMP RegisterSMNotify(IUnknown*);
	STDMETHODIMP RegisterARNotify(IUnknown*);
	STDMETHODIMP SetAltName(PVOID*, DWORD*, PVOID*);
	STDMETHODIMP GetAltName(PVOID*, DWORD*);
private:
	IAppResolver8* m_resolver8;
	IStartMenuItemsCache8* m_startmenuitemscache8;
	IStartMenuItemsCache10* m_startmenuitemscache10;
	long m_cRef;
};

class CObjectWithSite : public IObjectWithSite
{
public:
	CObjectWithSite() { m_punkSite = NULL; };
	virtual ~CObjectWithSite() { if (m_punkSite) { m_punkSite->Release(); } }

	//*** IUnknown ****
	// (client must provide!)

	//*** IObjectWithSite ***
	STDMETHOD(SetSite)(IUnknown* punkSite);
	STDMETHOD(GetSite)(REFIID riid, void** ppvSite);

protected:
	IUnknown* m_punkSite;
};

#undef  INTERFACE
#define INTERFACE   ITrayPriv

MIDL_INTERFACE("4622AD10-FF23-11D0-8D34-00A0C90F2719")
ITrayPriv : public IOleWindow
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG, Release) (THIS) PURE;

	// *** IOleWindow methods ***
	STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
	STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

	// *** ITrayPriv methods ***
	STDMETHOD(ExecItem)(THIS_ IShellFolder * psf, LPCITEMIDLIST pidl) PURE;
	STDMETHOD(GetFindCM)(THIS_ HMENU hmenu, UINT idFirst, UINT idLast, IContextMenu * *ppcmFind) PURE;
	STDMETHOD(GetStaticStartMenu)(THIS_ HMENU * phmenu) PURE;
};

// ITrayPriv2 - new for Whistler
//
// Purpose: Allows Explorer Start Menu object to participate in customdraw.
//
#undef  INTERFACE
#define INTERFACE   ITrayPriv2

DEFINE_GUID(IID_ITrayPriv2, 0x9e83c057, 0x6823, 0x4f1f, 0xbf, 0xa3, 0x74, 0x61, 0xd4, 0x0a, 0x81, 0x73);

MIDL_INTERFACE("9E83C057-6823-4F1F-BFA3-7461D40A8173")
ITrayPriv2 : public ITrayPriv
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG, Release) (THIS) PURE;

	// *** IOleWindow methods ***
	STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
	STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

	// *** ITrayPriv methods ***
	STDMETHOD(ExecItem)(THIS_ IShellFolder * psf, LPCITEMIDLIST pidl) PURE;
	STDMETHOD(GetFindCM)(THIS_ HMENU hmenu, UINT idFirst, UINT idLast, IContextMenu * *ppcmFind) PURE;
	STDMETHOD(GetStaticStartMenu)(THIS_ HMENU * phmenu) PURE;

	// *** ITrayPriv2 methods ***
	STDMETHOD(ModifySMInfo)(THIS_ IN LPSMDATA psmd, IN OUT SMINFO * psminfo) PURE;
};
#undef  INTERFACE

class CStartMenuCallbackBase
	: public IShellMenuCallback
	,public CObjectWithSite
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

protected:
	CStartMenuCallbackBase(BOOL fIsStartPanel = FALSE);
	~CStartMenuCallbackBase();

	void _InitializePrograms();
	HRESULT _FilterPidl(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl);
	HRESULT _Promote(LPSMDATA psmd, DWORD dwFlags);
	BOOL _IsTopLevelStartMenu(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl);
	HRESULT _HandleNew(LPSMDATA psmd);
	HRESULT _GetSFInfo(SMDATA* psmd, SMINFO* psminfo);
	HRESULT _ProcessChangeNotify(SMDATA* psmd, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

	HRESULT InitializeProgramsShellMenu(IShellMenu* psm);

	virtual DWORD _GetDemote(SMDATA* psmd) { return 0; }
	BOOL _IsDarwinAdvertisement(LPCITEMIDLIST pidlFull);

	void _RefreshSettings();

	int m_cRef;

	DWORD m_dwThreadId;

	LPWSTR m_pszPrograms;
	LPWSTR m_pszWindowsUpdate;
	LPWSTR m_pszConfigurePrograms;
	LPWSTR m_pszAdminTools;

	ITrayPriv2* m_pTrayPriv2;

	BOOL m_fExpandoMenus;
	BOOL m_fShowAdminTools;
	BOOL m_fIsStartPanel;
	BOOL m_fInitPrograms;
};

class CPersonalProgramsMenuCallback : public CStartMenuCallbackBase
{
public:
	CPersonalProgramsMenuCallback() : CStartMenuCallbackBase(TRUE) { m_pTrayPriv2 = 0; }

	// *** IShellMenuCallback methods ***
	STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// *** IObjectWithSite methods *** (overriding CObjectWithSite)
	STDMETHODIMP SetSite(IUnknown* punk);

public:
	HRESULT Initialize(IShellMenu* psm)
	{
		return InitializeProgramsShellMenu(psm);
	}

private:
	void _UpdateTrayPriv();

};
typedef DWORD   BITBOOL;
typedef DWORD MRULISTF;

typedef int(__stdcall* MRUDATALISTCOMPARE)(
	const BYTE* __MIDL_0023,
	const BYTE* __MIDL_0024,
	int __MIDL_0025);

MIDL_INTERFACE("fe787bcb-0ee8-44fb-8c89-12f508913c40")
IMruDataList : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE InitData(
		/* [in] */ UINT uMax,
		/* [in] */ MRULISTF flags,
		/* [in] */ HKEY hKey,
		/* [string][in] */ LPCWSTR pszSubKey,
		/* [in] */ MRUDATALISTCOMPARE pfnCompare) = 0;

	virtual HRESULT STDMETHODCALLTYPE AddData(
		/* [size_is][in] */ const BYTE* pData,
		/* [in] */ DWORD cbData,
		/* [out] */ DWORD* pdwSlot) = 0;

	virtual HRESULT STDMETHODCALLTYPE FindData(
		/* [size_is][in] */ const BYTE* pData,
		/* [in] */ DWORD cbData,
		/* [out] */ int* piIndex) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetData(
		/* [in] */ int iIndex,
		/* [size_is][out] */ BYTE* pData,
		/* [in] */ DWORD cbData) = 0;

	virtual HRESULT STDMETHODCALLTYPE QueryInfo(
		/* [in] */ int iIndex,
		/* [out][in] */ DWORD* pdwSlot,
		/* [out][in] */ DWORD* pcbData) = 0;

	virtual HRESULT STDMETHODCALLTYPE Delete(
		/* [in] */ int iIndex) = 0;

};

// <BEGIN RegStr.h COPIED CODE>
#define REGSTR_PATH_EXPLORER             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer")
#define REGSTR_PATH_SETUP                TEXT("Software\\Microsoft\\Windows\\CurrentVersion")
// <END RegStr.h COPIED CODE>

// <BEGIN msi.h COPIED CODE>
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
// </END msi.h COPIED CODE>

class CDarwinAd
{
public:
	LPWSTR m_szLocalPath;
	LPWSTR m_szDescriptor;
	INSTALLSTATE m_installState;
	LPITEMIDLIST m_pidl;

	CDarwinAd(LPITEMIDLIST pidl, LPWSTR psz);
	~CDarwinAd();

	void CheckInstalled();
	BOOL IsAd();
};

class CStartMenuCallback : public CStartMenuCallbackBase
{
public:
	// IShellMenuCallback:
	STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// IObjectWithSite:
	STDMETHODIMP SetSite(IUnknown* punk);
	STDMETHODIMP GetSite(REFIID riid, void** ppvOut);

	CStartMenuCallback();

	HRESULT InitializeFastItemsShellMenu(IShellMenu* psm);
	HRESULT InitializeCSIDLShellMenu(int uId, int csidl, LPTSTR pszRoot, LPTSTR pszValue,
		DWORD dwPassInitFlags, DWORD dwSetFlags, BOOL fAddOpen,
		IShellMenu* psm);
	HRESULT InitializeDocumentsShellMenu(IShellMenu* psm);
	HRESULT InitializeSubShellMenu(int idCmd, IShellMenu* psm);

private:
	virtual ~CStartMenuCallback();

	IContextMenu* m_pContextMenuFind;
	ITrayPriv* m_pTrayPriv;
	IUnknown* m_punkSite;
	IOleCommandTarget* m_pOleCommandTarget;
	bool m_fAddOpenFolder : 1;
	bool m_fCascadeMyDocuments : 1;
	bool m_fCascadePrinters : 1;
	bool m_fCascadeControlPanel : 1;
	bool m_fFindMenuInvalid : 1;
	bool m_fCascadeNetConnections : 1;
	bool m_fShowInfoTip : 1;
	bool m_fHasInitShowTopLevelStartMenu : 1;
	bool m_fCascadeMyPictures : 1;

	bool m_fHasMyDocuments : 1;
	bool m_fHasMyPictures : 1;

	WCHAR m_szFindMnemonic[2];

	HWND m_hWnd;

	IMruDataList* m_pMruRecent;
	DWORD m_cRecentDocs;

	DWORD m_dwFlags;
	DWORD m_dwChevronCount;

	HRESULT _ExecHmenuItem(LPSMDATA psmdata);
	HRESULT _Init(SMDATA* psmdata);
	HRESULT _Create(SMDATA* psmdata, void** pvUserData);
	HRESULT _Destroy(SMDATA* psmdata);
	HRESULT _GetHmenuInfo(SMDATA* psmd, SMINFO* sminfo);
	HRESULT _GetObject(LPSMDATA psmd, REFIID riid, void** ppvObj);
	HRESULT _CheckRestricted(DWORD dwRestrict, BOOL* fRestricted);
	HRESULT _FilterRecentPidl(IShellFolder* psf, LPCITEMIDLIST pidl);
	HRESULT _Demote(LPSMDATA psmd);
	HRESULT _GetTip(LPWSTR pstrTitle, LPWSTR pstrTip);
	DWORD _GetDemote(SMDATA* psmd);
	HRESULT _HandleAccelerator(TCHAR ch, SMDATA* psmdata);
	HRESULT _GetDefaultIcon(LPWSTR psz, int* piIndex);
	void _GetStaticStartMenu(HMENU* phmenu, HWND* phwnd);
	HRESULT _GetStaticInfoTip(SMDATA* psmd, LPWSTR pszTip, int cch);

	// helper functions
	DWORD GetInitFlags();
	void  SetInitFlags(DWORD dwFlags);
	HRESULT _InitializeFindMenu(IShellMenu* psm);
	HRESULT _ExecItem(LPSMDATA, UINT);
	HRESULT VerifyCSIDL(int idCmd, int csidl, IShellMenu* psm);
	HRESULT VerifyMergedGuy(BOOL fPrograms, IShellMenu* psm);
	void _UpdateDocsMenuItemNames(IShellMenu* psm);
	void _UpdateDocumentsShellMenu(IShellMenu* psm);
};

HRESULT SHCoInitialize(void);