#pragma once
#include <Guiddef.h>
DEFINE_GUID(CLSID_AuthUIShutdownChoices,0x14CE31DC, 0xABc2, 0x484c, 0xb0,0x61,0xcf,0x34,0x16,0xae,0xd8,0xff); //{14CE31DC-ABC2-484C-B061-CF3416AED8FF}
DEFINE_GUID(IID_IShutdownChoices8,0x811CA537, 0x0FEEC, 0x4041, 0x9F, 0x51, 0x0E7, 0x0BD, 0x0A5, 0x96, 0x60, 0x1C); //811ca537_feec_4041_9f51_e7bda596601c
DEFINE_GUID(IID_IShutdownChoices7,0x0F678FCDE, 0x0EB44, 0x4B6E, 0x9B, 0x75, 0x0CC, 0x4A, 0x66, 0x1F, 0x52, 0x63); //f678fcde_eb44_4b6e_9b75_cc4a661f5263
#include <windows.h>
#include <DocObj.h>

MIDL_INTERFACE("f678fcde-eb44-4b6e-9b75-cc4a661f5263")
IShutdownChoices7: public IUnknown
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

MIDL_INTERFACE("811ca537-feec-4041-9f51-e7bda596601c")
IShutdownChoices8: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Refresh(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateListener(IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetChoiceMask(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetChoiceMask(ULONG*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDefaultUIChoiceMask(ULONG*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetShowBadChoices(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetChoiceEnumerator(IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDefaultChoice(ULONG*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UserHasShutdownRights(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetChoiceName(ULONG,int,LPWSTR,UINT) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetChoiceDesc(ULONG,LPWSTR,UINT) = 0;
};

class CAuthUIWrapper: public IShutdownChoices7
{
public:
	//constructor
	CAuthUIWrapper(IShutdownChoices8 *authui8);
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
	IShutdownChoices8* m_authui8;
	long m_cRef;
};