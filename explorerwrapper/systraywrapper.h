#pragma once
#include <Guiddef.h>
DEFINE_GUID(CLSID_SysTray,0x35CEC8A3, 0x2BE6, 0x11D2, 0x87,0x73,0x92,0xE2,0x20,0x52,0x41,0x53);
DEFINE_GUID(CGID_ShellServiceObject,0x214D2, 0x0000, 0x0000, 0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
#include <windows.h>
#include <DocObj.h>

class CSysTrayWrapper: public IOleCommandTarget
{
public:
	//constructor
	CSysTrayWrapper(IOleCommandTarget *stobject8);
	//destructor
	~CSysTrayWrapper();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IOleCommandTarget
    HRESULT STDMETHODCALLTYPE QueryStatus( const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    HRESULT STDMETHODCALLTYPE Exec( const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);
	//our stuff
	void DoInitSysTray( void);
private:
	BOOL initialized;
	IOleCommandTarget *m_stobject8;
	long m_cRef;
};

/*static CSysTrayWrapper* SingleCTrayWrapper;
VOID CALLBACK InitLaterTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);*/