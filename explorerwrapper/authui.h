#pragma once
#include <Guiddef.h>
DEFINE_GUID(CLSID_AuthUIShutdownChoices,0x14CE31DC, 0xABc2, 0x484c, 0xb0,0x61,0xcf,0x34,0x16,0xae,0xd8,0xff); //{14CE31DC-ABC2-484C-B061-CF3416AED8FF}
#include <windows.h>
#include <DocObj.h>

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IShutdownChoices: public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Refresh(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateListener(IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetChoiceMask(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMessageWnd(HWND**) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetShowBadChoices(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetChoiceEnumerator(IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDefaultChoice(ULONG*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UserHasShutdownRights(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetChoiceName(ULONG,int,LPWSTR,UINT) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetChoiceDesc(ULONG,LPWSTR,UINT) = 0;
};

class CAuthUIWrapper: public IShutdownChoices
{
public:
	//constructor
	CAuthUIWrapper(IShutdownChoices *authui);
	//destructor
	~CAuthUIWrapper();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IShutdownChoices
    HRESULT STDMETHODCALLTYPE Refresh(void);
    HRESULT STDMETHODCALLTYPE CreateListener(IUnknown**);
	HRESULT STDMETHODCALLTYPE SetChoiceMask(ULONG);
	HRESULT STDMETHODCALLTYPE GetMessageWnd(HWND**);
	HRESULT STDMETHODCALLTYPE SetShowBadChoices(int);
	HRESULT STDMETHODCALLTYPE GetChoiceEnumerator(IUnknown**);
	HRESULT STDMETHODCALLTYPE GetDefaultChoice(ULONG*);
	HRESULT STDMETHODCALLTYPE UserHasShutdownRights(void);
	HRESULT STDMETHODCALLTYPE GetChoiceName(ULONG,int,LPWSTR,UINT);
	HRESULT STDMETHODCALLTYPE GetChoiceDesc(ULONG,LPWSTR,UINT);
private:
	IShutdownChoices *m_authui;
	long m_cRef;
};