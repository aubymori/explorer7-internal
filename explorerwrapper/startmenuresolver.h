#pragma once
#include "appresolvernotify.h"
#include "EnumStartMenu.h"

#define INITGUID
#include <guiddef.h>
DEFINE_GUID(CLSID_StartMenuCacheAndAppResolver, 0x660B90C8, 0x73A9, 0x4B58, 0x8C, 0xAE, 0x35, 0x5B, 0x7F, 0x55, 0x34, 0x1B);
DEFINE_GUID(IID_IAppResolver7, 0x46a6eeff, 0x908e, 0x4dc6, 0x92, 0xA6, 0x64, 0xbe, 0x91, 0x77, 0xb4, 0x1c); //46a6eeff_908e_4dc6_92a6_64be9177b41c
DEFINE_GUID(IID_IAppResolver8, 0xde25675a, 0x72de, 0x44b4, 0x93, 0x73, 0x05, 0x17, 0x04, 0x50, 0xc1, 0x40); //de25675a_72de_44b4_9373_05170450c140
DEFINE_GUID(IID_IStartMenuItemsCache7, 0x05a232fd, 0x2bfb, 0x4349, 0x9d, 0x48, 0x47, 0x87, 0xf3, 0x17, 0xf5, 0x0a); //05a232fd_2bfb_4349_9d48_4787f317f50a
//DEFINE_GUID(IID_IStartMenuItemsCache8,0xbb9786b2, 0xefe6, 0x4f1e,0xa3,0xbd,0x67,0xf9,0x7d,0x00,0x85,0xbf); //bb9786b2_efe6_4f1e_a3bd_67f97d0085bf
DEFINE_GUID(IID_IStartMenuItemsCache8, 0x934332DD, 0x0B0FE, 0x41F9, 0x0BC, 0x63, 0x9C, 0x7F, 0x9F, 0x3C, 0x3A, 0x0EC); //_GUID_934332dd_b0fe_41f9_bc63_9c7f9f3c3aec
DEFINE_GUID(IID_IStartMenuItemsCache10, 0x0BA5A92AE, 0x0BFD7, 0x4916, 0x85, 0x4F, 0x6B, 0x3A, 0x40, 0x2B, 0x84, 0x0A8); //_GUID_ba5a92ae_bfd7_4916_854f_6b3a402b84a8
//DEFINE_GUID(IID_IStartMenuAppItems8,0x33f71155, 0xc2e9, 0x4ffe,0x97,0x86,0xa3,0x2d,0x98,0x57,0x7c,0xff); //33f71155_c2e9_4ffe_9786_a32d98577cff
DEFINE_GUID(IID_IStartMenuAppItems8, 0x2C5CCF3, 0x805F, 0x4654, 0x0A7, 0x0B7, 0x34, 0x0A, 0x74, 0x33, 0x53, 0x65); //02c5ccf3_805f_4654_a7b7_340a74335365

DEFINE_GUID(IID_IRegTreeOptions8, 0x7897eca6, 0x1b1b, 0x452a, 0x85, 0x81, 0xbb, 0x94, 0x82, 0xae, 0xa7, 0xcc); //7897eca6_1b1b_452a_8581_bb9482aea7cc
DEFINE_GUID(IID_IRegTreeOptions7, 0xaf4f6511, 0xf982, 0x11d0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4c, 0xD6, 0xD8); //af4f6511_f982_11d0_8595_00aa004cd6d8
DEFINE_GUID(CLSID_RegTreeOptions, 0xAF4F6510, 0xF982, 0x11D0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4C, 0xD6, 0xD8); //AF4F6510-F982-11D0-8595-00AA004CD6D8

#include <windows.h>
#include <propkey.h>
#include <propvarutil.h>
DEFINE_PROPERTYKEY(PKEY_AppUserModel_BestShortcut, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 10);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_HostEnvironment, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 14);
//DEFINE_PROPERTYKEY(PKEY_AppUserModel_IsDualMode, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 11);

MIDL_INTERFACE("46a6eeff-908e-4dc6-92a6-64be9177b41c")
IAppResolver7: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetAppIDForShortcut(IShellItem*, LPWSTR*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAppIDForWindow(HWND*,DWORD*,DWORD*,DWORD*,DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAppIDForProcess(ULONG_PTR,DWORD*,DWORD*,DWORD*,DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetShortcutForProcess(ULONG_PTR,IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBestShortcutForAppID(DWORD*,IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBestShortcutAndAppIDForAppPath(DWORD*,IUnknown*,DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE CanPinApp(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRelaunchProperties(HWND*,DWORD*,DWORD*,DWORD*,DWORD*,DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GenerateShortcutFromWindowProperties(HWND*,IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GenerateShortcutFromItemProperties(IUnknown*,IUnknown*) = 0;
};

MIDL_INTERFACE("de25675a-72de-44b4-9373-05170450c140")
IAppResolver8: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetAppIDForShortcut(IShellItem*, LPWSTR*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAppIDForShortcutObject(IUnknown*, IUnknown*, DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAppIDForWindow(HWND*,DWORD*,DWORD*,DWORD*,DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAppIDForProcess(ULONG_PTR,DWORD*,DWORD*,DWORD*,DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetShortcutForProcess(ULONG_PTR,IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBestShortcutForAppID(DWORD*,IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBestShortcutAndAppIDForAppPath(DWORD*,IUnknown*,DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE CanPinApp(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE CanPinAppShortcut(IUnknown*, IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRelaunchProperties(HWND*,DWORD*,DWORD*,DWORD*,DWORD*,DWORD*, int* a7) = 0;
	virtual HRESULT STDMETHODCALLTYPE GenerateShortcutFromWindowProperties(HWND*,IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GenerateShortcutFromItemProperties(IUnknown*,IUnknown*) = 0;
};

MIDL_INTERFACE("05a232fd-2bfb-4349-9d48-4787f317f50a")
IStartMenuItemsCache7: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE OnChangeNotify(unsigned int,long,PVOID*,PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE PinListChanged(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPinnedItemsCount(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetStartMenuMFUList(unsigned int,IEnumStartMenuItem**,IEnumString**,FILETIME*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterSMNotify(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterARNotify(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetAltName(PVOID*,DWORD*,PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAltName(PVOID*,DWORD*) = 0;
};

//MIDL_INTERFACE("bb9786b2-efe6-4f1e-a3bd-67f97d0085bf")
MIDL_INTERFACE("934332dd-b0fe-41f9-bc63-9c7f9f3c3aec")
IStartMenuItemsCache8: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE OnChangeNotify(unsigned int,long,PVOID*,PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterForNotifications(void*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterForNotifications(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE PauseNotifications(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResumeNotifications(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterARNotify(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RefreshCache(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReleaseGlobalCacheObject(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsCacheMatchingLanguage(int*) = 0;
};
MIDL_INTERFACE("ba5a92ae-bfd7-4916-854f-6b3a402b84a8")
IStartMenuItemsCache10: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE OnChangeNotify(unsigned int,long,PVOID*,PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterForNotifications(void*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterForNotifications(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE PauseNotifications(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResumeNotifications(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterARNotify(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RefreshCache(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReleaseGlobalCacheObject(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsCacheMatchingLanguage(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnableAppUsageData(void) = 0;
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
	virtual HRESULT STDMETHODCALLTYPE EnumItems(int, REFIID, PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItem(int, LPWSTR, const IID& riid, PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetItemByAppPath(const WCHAR*, _GUID const&, void**) = 0;
};

class CStartMenuResolver : public IAppResolver7, IStartMenuItemsCache7
{
public:
	//constructor
	CStartMenuResolver(IAppResolver8* newresolver);
	//destructor
	~CStartMenuResolver();
	//IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	//IAppResolver7
	HRESULT STDMETHODCALLTYPE GetAppIDForShortcut(IShellItem*, LPWSTR*);
	HRESULT STDMETHODCALLTYPE GetAppIDForWindow(HWND*, DWORD*, DWORD*, DWORD*, DWORD*);
	HRESULT STDMETHODCALLTYPE GetAppIDForProcess(ULONG_PTR, DWORD*, DWORD*, DWORD*, DWORD*);
	HRESULT STDMETHODCALLTYPE GetShortcutForProcess(ULONG_PTR, IUnknown*);
	HRESULT STDMETHODCALLTYPE GetBestShortcutForAppID(DWORD*, IUnknown*);
	HRESULT STDMETHODCALLTYPE GetBestShortcutAndAppIDForAppPath(DWORD*, IUnknown*, DWORD*);
	HRESULT STDMETHODCALLTYPE CanPinApp(IUnknown*);
	HRESULT STDMETHODCALLTYPE GetRelaunchProperties(HWND*, DWORD*, DWORD*, DWORD*, DWORD*, DWORD*);
	HRESULT STDMETHODCALLTYPE GenerateShortcutFromWindowProperties(HWND*, IUnknown*);
	HRESULT STDMETHODCALLTYPE GenerateShortcutFromItemProperties(IUnknown*, IUnknown*);
	//IStartMenuItemsCache7
	HRESULT STDMETHODCALLTYPE OnChangeNotify(unsigned int, long, PVOID*, PVOID*);
	HRESULT STDMETHODCALLTYPE PinListChanged(void);
	HRESULT STDMETHODCALLTYPE GetPinnedItemsCount(int*);
	HRESULT STDMETHODCALLTYPE GetStartMenuMFUList(unsigned int, IEnumStartMenuItem**, IEnumString**, FILETIME*);
	HRESULT STDMETHODCALLTYPE RegisterSMNotify(IUnknown*);
	HRESULT STDMETHODCALLTYPE RegisterARNotify(IUnknown*);
	HRESULT STDMETHODCALLTYPE SetAltName(PVOID*, DWORD*, PVOID*);
	HRESULT STDMETHODCALLTYPE GetAltName(PVOID*, DWORD*);
private:
	IAppResolver8* m_resolver8;
	IStartMenuItemsCache8* m_startmenuiconscache8;
	IStartMenuItemsCache10* m_startmenuiconscache10;
	long m_cRef;
};
