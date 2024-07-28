#pragma once
#define INITGUID
#include <guiddef.h>

DEFINE_GUID(CLSID_ImmersiveShell,0xc2f03a33, 0x21f5, 0x47fa, 0xb4,0xbb,0x15,0x63,0x62,0xa2,0xf2,0x39); //c2f03a33_21f5_47fa_b4bb_156362a2f239
DEFINE_GUID(IID_ImmersiveShellProvider,0x6D5140C1, 0x7436, 0x11CE, 0x80,0x34,0x00,0xaa,0x00,0x60,0x09,0xfa);

DEFINE_GUID(SID_IImmersiveMonitorService,0x47094E3A, 0x0CF2, 0x430F, 0x80,0x6F,0xCF,0x9E,0x4F,0x0F,0x12,0xDD); //{47094E3A-0CF2-430F-806F-CF9E4F0F12DD}
DEFINE_GUID(IID_IImmersiveMonitorService,0x4D4C1E64, 0xE410, 0x4FAA, 0xBA,0xFA,0x59,0xCA,0x06,0x9B,0xFE,0xC2); //{4D4C1E64-E410-4FAA-BAFA-59CA069BFEC2}

DEFINE_GUID(SID_IImmersiveLayout,0xE2304C77, 0xD2A6, 0x43AE, 0x82,0x40,0x08,0x7E,0x7E,0x51,0x0F,0xE8); //{E2304C77-D2A6-43AE-8240-087E7E510FE8}
DEFINE_GUID(IID_IImmersiveLayout,0xD770B2AD, 0x8F5E, 0x4B8E, 0xB3,0xDF,0xF0,0x5A,0x2A,0xB5,0x28,0x7C); //{D770B2AD-8F5E-4B8E-B3DF-F0 5A 2A B5 28 7C}

DEFINE_GUID(IID_IImmersiveMode,0x2814FACC, 0x5F7E, 0x43A5, 0x94,0x8C,0xC3,0xBD,0xC0,0xFF,0xEE,0x84); //{2814FACC-5F7E-43A5-948C-C3BDC0FFEE84}

DEFINE_GUID(IID_IIWpnPlatform,0x9FA045CB, 0xB9B3, 0x47BA, 0x84,0x2F,0xE2,0xAB,0x45,0x8F,0x2B,0x0C);

DEFINE_GUID(CLSID_PushNotificationPlatformCF,0x4655840e, 0xAB1A, 0x49D0, 0xA4,0xC4,0x26,0x1F,0xA1,0xC2,0x0E,0x86); //{4655840e-ab1a-49d0-a4c4-261fa1c20e86}
DEFINE_GUID(CLSID_PushNotificationPlatform,0x0C9281F9, 0x6DA1, 0x4006, 0x87,0x29,0xDE,0x6E,0x6B,0x61,0x58,0x1C);


// []

#include <windows.h>

class CImmersiveFactory : public IClassFactory
{
public:
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IClassFactory
	HRESULT STDMETHODCALLTYPE CreateInstance( IUnknown * pUnkOuter, REFIID riid, void ** ppvObject );
	HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock );
};

class CImmersiveProvider : public IServiceProvider
{
public:
	//constructor
	CImmersiveProvider();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IServiceProvider
	HRESULT STDMETHODCALLTYPE QueryService( REFGUID guidService, REFIID riid, void **ppv );
private:
	long m_cRef;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveMonitorManager: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetCount(UINT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetConnectedCount(UINT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAt(UINT,IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFromHandle(HMONITOR,IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFromIdentity(ULONG,IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetImmersiveProxyMonitor(IUnknown**) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryService(HMONITOR, REFGUID guidService, REFIID riid, void **ppv ) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryServiceByIdentity(ULONG, REFGUID guidService, REFIID riid, void **ppv ) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryServiceFromWindow(HWND, REFGUID guidService, REFIID riid, void **ppv ) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryServiceFromPoint(tagPOINT*, REFGUID guidService, REFIID riid, void **ppv ) = 0;
	virtual HRESULT STDMETHODCALLTYPE MoveImmersiveMonitor(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetImmersiveMonitor(IUnknown*) = 0;
};

class CImmersiveMonitorManager : public IImmersiveMonitorManager
{
public:
	//constructor
	CImmersiveMonitorManager();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IImmersiveMonitorManager
	HRESULT STDMETHODCALLTYPE GetCount(UINT*);
	HRESULT STDMETHODCALLTYPE GetConnectedCount(UINT*);
	HRESULT STDMETHODCALLTYPE GetAt(UINT,IUnknown**);
	HRESULT STDMETHODCALLTYPE GetFromHandle(HMONITOR,IUnknown**);
	HRESULT STDMETHODCALLTYPE GetFromIdentity(ULONG,IUnknown**);
	HRESULT STDMETHODCALLTYPE GetImmersiveProxyMonitor(IUnknown**);
	HRESULT STDMETHODCALLTYPE QueryService(HMONITOR, REFGUID guidService, REFIID riid, void **ppv );
	HRESULT STDMETHODCALLTYPE QueryServiceByIdentity(ULONG, REFGUID guidService, REFIID riid, void **ppv );
	HRESULT STDMETHODCALLTYPE QueryServiceFromWindow(HWND, REFGUID guidService, REFIID riid, void **ppv );
	HRESULT STDMETHODCALLTYPE QueryServiceFromPoint(tagPOINT*, REFGUID guidService, REFIID riid, void **ppv );
	HRESULT STDMETHODCALLTYPE MoveImmersiveMonitor(int);
	HRESULT STDMETHODCALLTYPE SetImmersiveMonitor(IUnknown*);
private:
	long m_cRef;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveLayout: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE RegisterLayoutClient(UINT,IUnknown*,ULONG*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterLayoutClient(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterForLayoutChanges(UINT,IUnknown*,ULONG*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterForLayoutChanges(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOuterWorkAreaForBand(ULONG,tagRECT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetInnerWorkAreaForBand(ULONG,tagRECT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetImmersiveShellWorkArea(tagRECT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE InvalidateWorkArea(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBandWorkAreaCount(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBandWorkAreaAt(UINT,IUnknown**) = 0;
};

class CImmersiveLayout : public IImmersiveLayout
{
public:
	//constructor
	CImmersiveLayout(HMONITOR hMonitor);
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IImmersiveMonitorManager
	HRESULT STDMETHODCALLTYPE RegisterLayoutClient(UINT,IUnknown*,ULONG*);
	HRESULT STDMETHODCALLTYPE UnregisterLayoutClient(ULONG);
	HRESULT STDMETHODCALLTYPE RegisterForLayoutChanges(UINT,IUnknown*,ULONG*);
	HRESULT STDMETHODCALLTYPE UnregisterForLayoutChanges(ULONG);
	HRESULT STDMETHODCALLTYPE GetOuterWorkAreaForBand(ULONG,tagRECT*);
	HRESULT STDMETHODCALLTYPE GetInnerWorkAreaForBand(ULONG,tagRECT*);
	HRESULT STDMETHODCALLTYPE GetImmersiveShellWorkArea(tagRECT*);
	HRESULT STDMETHODCALLTYPE InvalidateWorkArea(ULONG);
	HRESULT STDMETHODCALLTYPE GetBandWorkAreaCount(void);
	HRESULT STDMETHODCALLTYPE GetBandWorkAreaAt(UINT,IUnknown**);
private:
	HMONITOR m_hMonitor;
	long m_cRef;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveMode: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetMode(DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMode(DWORD) = 0;
};

class CImmersiveMode : public IImmersiveMode
{
public:
	//constructor
	CImmersiveMode();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IImmersiveMonitorManager
	HRESULT STDMETHODCALLTYPE GetMode(DWORD*);
	HRESULT STDMETHODCALLTYPE SetMode(DWORD);
private:
	long m_cRef;
};

void RegisterFakeImmersive();
void UnregisterFakeImmersive();

