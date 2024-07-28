#pragma once
#define INITGUID
#include <guiddef.h>
DEFINE_GUID(CLSID_UserAssist,0xDD313E04, 0xFEFF, 0x11D1, 0x8E,0xCD,0x00,0x00,0xF8,0x7A,0x47,0x0C);
DEFINE_GUID(IID_IUserAssist,0x90d75131, 0x43a6, 0x4664, 0x9a,0xf8,0xdc,0xce,0xb8,0x5a,0x74,0x62); //90d75131_43a6_4664_9af8_dcceb85a7462
DEFINE_GUID(UAIID_SHORTCUTS,0xF4E57C4B, 0x2036, 0x45f0, 0xa9,0xab,0x44,0x3b,0xcf,0xe3,0x3d,0x9f);

#include <Windows.h>
#include <ShlObj.h>
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
    virtual HRESULT STDMETHODCALLTYPE FireEvent(REFIID, PVOID, LPWSTR, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryEntry(REFIID, LPWSTR, PUEMINFO) = 0;
	//...more of whatever
};

HRESULT WINAPI UAQueryEntry(REFIID iid, LPWSTR parsingname, PUEMINFO uem);
HRESULT WINAPI UAQueryShortcut(LPITEMIDLIST pidl, PUEMINFO uem);
LPWSTR WINAPI CoAllocString(LPWSTR src);