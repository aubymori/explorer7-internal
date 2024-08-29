#ifndef _STARTMENUPIN_H
#define _STARTMENUPIN_H

#define INITGUID
#include <guiddef.h>
DEFINE_GUID(CLSID_StartMenuPin,0xA2A9545D, 0xA0C2, 0x42B4, 0x97,0x08,0xA0,0xB2,0xBA,0xDD,0x77,0xC8); //{A2A9545D-A0C2-42B4-9708-A0B2BADD77C8}
DEFINE_GUID(CLSID_TaskbarPin,0x90AA3A4E, 0x1CBA, 0x4233, 0xB8,0xBB,0x53,0x57,0x73,0xD4,0x84,0x49);

#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>


typedef HRESULT (WINAPI* CreateInstance_API)(PVOID,REFIID,PVOID*);
	typedef struct { 
		PVOID dunno1;
		DWORD_PTR dunno2;
		REFCLSID rclsid;
		CreateInstance_API CreateFunc; 
	} SHELLGUIDS, *PSHELLGUIDS; 

void StartMenuPin_PatchShell32();

/*MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IContextMenuShort
{
public:
	virtual void QueryInterface() = 0;
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryContextMenu( HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) = 0;
};*/

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IStartMenuShellExtInit
{
public:
	virtual void QueryInterface() = 0;
	virtual void AddRef() = 0;
	virtual void Release() = 0;
	virtual void Initialize() = 0;
	virtual LRESULT __thiscall SetChangeCount(DWORD value) = 0;
	virtual IStream* __thiscall OpenPinRegStream(DWORD grfMode) = 0;
	virtual IStream* __thiscall OpenLinksRegStream(DWORD grfMode) = 0;
	virtual void NotifyPinListChange() = 0;
	virtual DWORD __thiscall GetPinStreamVersion() = 0;
	virtual LRESULT __thiscall SetPinStreamVersion(DWORD value) = 0;
	virtual void Unimpl1() = 0;
	virtual void UpgradeItem() = 0;
	virtual HRESULT __thiscall GetBackupSubDirName(LPWSTR szOut, int cbLen) = 0;
	virtual void IsAcceptableTarget() = 0;
	virtual DWORD __thiscall IsRestricted() = 0;
	virtual void Unimpl2() = 0;
	virtual HRESULT __thiscall GetMenuStringID(DWORD* w) = 0;
	virtual int __thiscall GetHelpText(int,LPWSTR,int) = 0;
	virtual LRESULT __thiscall GetChangeCount(DWORD* pdwVal) = 0;
	virtual LPSTR __thiscall GetVerb(int op) = 0;
	virtual void SendPinRearrangeSQM() = 0;
	virtual LRESULT __thiscall SetRemovedChangeCount(DWORD value);
	virtual DWORD __thiscall GetRemovedChangeCount() = 0;
	virtual void GetPinnedAppSQMEventID() = 0;
};

class CStartMenuPin /* : public IStartMenuShellExtInit*//*, public IContextMenuShort*/
{
public:
	//constructor
	//CStartMenuPin();
	//destructor
	//~CStartMenuPin();
	void QueryInterface();
	void AddRef();
	void Release();
	void Initialize();
	LRESULT __thiscall SetChangeCount(DWORD value);
	IStream* __thiscall OpenPinRegStream(DWORD grfMode);
	IStream* __thiscall OpenLinksRegStream(DWORD grfMode);
	void NotifyPinListChange();
	DWORD __thiscall GetPinStreamVersion();
	LRESULT __thiscall SetPinStreamVersion(DWORD value);
	void Unimpl1();
	void UpgradeItem();
	HRESULT __thiscall GetBackupSubDirName(LPWSTR szOut, int cbLen);
	void IsAcceptableTarget();
	DWORD __thiscall IsRestricted();
	void Unimpl2();
	HRESULT __thiscall GetMenuStringID(DWORD* w);
	int __thiscall GetHelpText(int,LPWSTR,int);
	LRESULT __thiscall GetChangeCount(DWORD* pdwVal);
	LPSTR __thiscall GetVerb(int op);
	void SendPinRearrangeSQM();
	LRESULT __thiscall SetRemovedChangeCount(DWORD value);
	DWORD __thiscall GetRemovedChangeCount();	
	void GetPinnedAppSQMEventID();
	/*HRESULT STDMETHODCALLTYPE QueryContextMenu( HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);*/
};

typedef HRESULT (__thiscall* GetMenuStringID_API)(PVOID,DWORD*);
/*typedef HRESULT (__thiscall* QueryContextMenu_API)(PVOID,HMENU,UINT,UINT,UINT,UINT);*/

typedef struct { 
	PVOID QueryInterface;
	PVOID AddRef;
	PVOID Release;
	PVOID Initialize;
	PVOID SetChangeCount;
	PVOID OpenPinRegStream;
	PVOID OpenLinksRegStream;
	PVOID NotifyPinListChange;
	PVOID GetPinStreamVersion;
	PVOID SetPinStreamVersion;
	PVOID Unimpl1;
	PVOID UpgradeItem;
	PVOID GetBackupSubDirName;
	PVOID IsAcceptableTarget;
	PVOID IsRestricted;
	PVOID Unimpl2;
	GetMenuStringID_API GetMenuStringID;
	PVOID GetHelpText;
	PVOID GetChangeCount;
	PVOID GetVerb;
	PVOID SendPinRearrangeSQM;
	PVOID SetRemovedChangeCount;
	PVOID GetRemovedChangeCount;
	PVOID GetPinnedAppSQMEventID;
} STARTPINVTBL, *PSTARTPINVTBL; 

typedef struct {
	PSTARTPINVTBL pStartPinVtbl;
} STARTPINOBJ, *PSTARTPINOBJ;


#endif