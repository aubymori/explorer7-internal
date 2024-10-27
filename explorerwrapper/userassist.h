#pragma once
#define INITGUID
#include "framework.h"

#pragma region GUID definitions
DEFINE_GUID(CLSID_UserAssist,0xDD313E04, 0xFEFF, 0x11D1, 0x8E,0xCD,0x00,0x00,0xF8,0x7A,0x47,0x0C);
DEFINE_GUID(IID_IUserAssistXP, 0xdd313e05, 0xfeff, 0x11d1, 0x8e, 0xcd, 0x0, 0x0, 0xf8, 0x7a, 0x47, 0xc); //90d75131_43a6_4664_9af8_dcceb85a7462
DEFINE_GUID(IID_IUserAssist7,0x90d75131, 0x43a6, 0x4664, 0x9a,0xf8,0xdc,0xce,0xb8,0x5a,0x74,0x62); //90d75131_43a6_4664_9af8_dcceb85a7462
DEFINE_GUID(IID_IUserAssist72,0x90d75131, 0x43a6, 0x4664, 0x9a,0xf8,0xdc,0xce,0xb8,0x5a,0x74,0x62); //90d75131_43a6_4664_9af8_dcceb85a7462
DEFINE_GUID(IID_IUserAssist10, 0x49B36D57, 0x5FD2, 0x45A7, 0x98, 0x1B, 0x6, 0x2, 0x8D, 0x57, 0x7A, 0x47); //49b36d57_5fd2_45a7_981b_06028d577a47
DEFINE_GUID(IID_IUserAssist102, 0x1F052FA3, 0x7A76, 0x4BEC, 0x96, 0x0C4, 0x0E8, 0x65, 0x0CF, 0x1B, 0x55, 0x0F1); //90d75131_43a6_4664_9af8_dcceb85a7462
DEFINE_GUID(UAIID_SHORTCUTS,0xF4E57C4B, 0x2036, 0x45f0, 0xa9,0xab,0x44,0x3b,0xcf,0xe3,0x3d,0x9f);
#pragma endregion

typedef struct tagUEMINFO {
	DWORD cbSize; /*  +0x0000  */
	DWORD dwMask; /*  +0x0004  */
	DWORD R; /*  +0x0008 40 00 00 00  */
	DWORD cLaunches; /*  +0x000c 75 00 00 00  */
	DWORD cSwitches; /*  +0x0010 75 00 00 00  */
	DWORD dwTime; /*  +0x0014  */
	FILETIME ftExecute; /*  +0x0018 ce 1a 00 00  */
	BOOL fExcludeFromMFU; /*  +0x0020 74 00 00 00  */
} UEMINFO, *PUEMINFO;

MIDL_INTERFACE("90d75131-43a6-4664-9af8-dcceb85a7462")
IUserAssist: public IUnknown
{
public:
    STDMETHOD(FireEvent)(REFIID, int, WPARAM, LPARAM) PURE;
	STDMETHOD(QueryEntry)(REFIID, WPARAM, PUEMINFO) PURE;
	STDMETHOD(SetEntry)(REFIID, WPARAM wParam, UEMINFO* pui) PURE;
	//STDMETHOD(RenameEntry)(REFIID, WPARAM wParam, UEMINFO* pui) PURE;
	//STDMETHOD(DeleteEntry)(REFIID, WPARAM wParam, UEMINFO* pui) PURE;
	//...more of whatever
};

class IUserAssistXP : public IUnknown
{
public:
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG, Release) (THIS) PURE;

	// *** IUserAssist methods ***
	STDMETHOD(FireEvent)(THIS_ const GUID* pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam) PURE;
	STDMETHOD(QueryEvent)(THIS_ const GUID* pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, PUEMINFO pui) PURE;
	STDMETHOD(SetEvent)(THIS_ const GUID* pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, PUEMINFO pui) PURE;
};

class CUserAssistXPWrapper : public IUserAssistXP
{
public:
	CUserAssistXPWrapper(IUserAssist* newAssist);

	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppv);
	STDMETHOD_(ULONG, AddRef) (THIS) ;
	STDMETHOD_(ULONG, Release) (THIS);

	// *** IUserAssist methods ***
	STDMETHOD(FireEvent)(THIS_ const GUID* pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam);
	STDMETHOD(QueryEvent)(THIS_ const GUID* pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, PUEMINFO pui);
	STDMETHOD(SetEvent)(THIS_ const GUID* pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, PUEMINFO pui);

	IUserAssist* m_newAssist;
	DWORD m_cRef;
};

HRESULT WINAPI UAQueryEntry(REFIID iid, WPARAM parsingname, PUEMINFO uem);
HRESULT WINAPI UAQueryShortcut(LPITEMIDLIST pidl, PUEMINFO uem);
LPWSTR WINAPI CoAllocString(LPWSTR src);