#pragma once
#define INITGUID
#include "framework.h"

DEFINE_GUID(CLSID_AutoPlayUI,0x83b52078, 0xE93E, 0x425B, 0x92,0x6F,0xDE,0x61,0x69,0x87,0x5E,0x41); //83b52078_e93e_425b_926f_de6169875e41
DEFINE_GUID(IID_AutoPlayUI,0x9394e091, 0xa034, 0x4f1b, 0xb0,0x88,0x5b,0x53,0xa0,0x6c,0x65,0xfa); //9394e091_a034_4f1b_b088_5b53a06c65fa

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IAutoPlayUI: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE InitVolumeAutoplay(IUnknown *,LPCWSTR,LPCWSTR,ULONG,ULONG,ULONG,LPCWSTR,LPCWSTR,int,LPCWSTR,LPCWSTR,HWND) = 0;
	virtual HRESULT STDMETHODCALLTYPE InitNoContentAutoplay(IUnknown *,REFGUID,LPCWSTR,ULONG,int,LPCWSTR,LPCWSTR,LPCWSTR) = 0;
	virtual HRESULT STDMETHODCALLTYPE InitDirectAutoPlay(IUnknown *,LPCWSTR,HWND) = 0;
	virtual HRESULT STDMETHODCALLTYPE ToastPromptForChkDsk(LPCWSTR,int *,int *) = 0;
	virtual HRESULT STDMETHODCALLTYPE LaunchDeviceHandler(LPCWSTR,LPCWSTR,LPCWSTR) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsDialogClosed(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE SniffComplete(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE CloseDialog(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddContentType(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE MoreInterfaceArrived(LPCWSTR) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetChkDskCompleted(void) = 0;
};

class CAutoPlayWrapper: public IAutoPlayUI
{
public:
	//constructor
	CAutoPlayWrapper(IAutoPlayUI *autoui);
	//destructor
	~CAutoPlayWrapper();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IAutoPlayUI
	HRESULT STDMETHODCALLTYPE InitVolumeAutoplay(IUnknown *,LPCWSTR,LPCWSTR,ULONG,ULONG,ULONG,LPCWSTR,LPCWSTR,int,LPCWSTR,LPCWSTR,HWND);
	HRESULT STDMETHODCALLTYPE InitNoContentAutoplay(IUnknown *,REFGUID,LPCWSTR,ULONG,int,LPCWSTR,LPCWSTR,LPCWSTR);
	HRESULT STDMETHODCALLTYPE InitDirectAutoPlay(IUnknown *,LPCWSTR,HWND);
	HRESULT STDMETHODCALLTYPE ToastPromptForChkDsk(LPCWSTR,int *,int *);
	HRESULT STDMETHODCALLTYPE LaunchDeviceHandler(LPCWSTR,LPCWSTR,LPCWSTR);
	HRESULT STDMETHODCALLTYPE IsDialogClosed(void);
	HRESULT STDMETHODCALLTYPE SniffComplete(ULONG);
	HRESULT STDMETHODCALLTYPE CloseDialog(void);
	HRESULT STDMETHODCALLTYPE AddContentType(ULONG);
	HRESULT STDMETHODCALLTYPE MoreInterfaceArrived(LPCWSTR);
	HRESULT STDMETHODCALLTYPE SetChkDskCompleted(void);
private:
	IAutoPlayUI *m_autoui;
	long m_cRef;
};

HRESULT WINAPI Shell32_CoCreateInstance(
  __in   REFCLSID rclsid,
  __in   LPUNKNOWN pUnkOuter,
  __in   DWORD dwClsContext,
  __in   REFIID riid,
  __out  LPVOID *ppv
);
