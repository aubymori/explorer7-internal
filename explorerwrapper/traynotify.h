#pragma once
#define INITGUID
#include "framework.h"

DEFINE_GUID(CLSID_TrayNotify,0x25DEAD04, 0x1EAC, 0x4911, 0x9e,0x3a,0xad,0x0a,0x4a,0xb5,0x60,0xfd); //
DEFINE_GUID(IID_ITrayNotify7,0xfb852b2c, 0x6bad, 0x4605, 0x95,0x51,0xf1,0x5f,0x87,0x83,0x09,0x35); //fb852b2c_6bad_4605_9551_f1 5f 87 83 09 35
DEFINE_GUID(IID_ITrayNotify8,0xd133ce13, 0x3537, 0x48ba, 0x93,0xa7,0xaf,0xcd,0x5d,0x20,0x53,0xb4); //d133ce13_3537_48ba_93a7_af cd 5d 20 53 b4

MIDL_INTERFACE("fb852b2c-6bad-4605-9551-f15f87830935")
ITrayNotify7: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE RegisterCallback(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPreference(PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnableAutoTray(int) = 0;
};

MIDL_INTERFACE("d133ce13-3537-48ba-93a7-afcd5d2053b4")
ITrayNotify8: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE RegisterCallback(IUnknown*,ULONG*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterCallback(ULONG) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPreference(PVOID*) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnableAutoTray(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE DoAction(PVOID*,int) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetWindowingEnvironmentConfig(IUnknown*) = 0;
};

class CTrayNotifyFactory : public IClassFactory
{
public:
	//constructor
	CTrayNotifyFactory(IClassFactory* origfactory);
	//destructor
	~CTrayNotifyFactory();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IClassFactory
	HRESULT STDMETHODCALLTYPE CreateInstance( IUnknown * pUnkOuter, REFIID riid, void ** ppvObject );
	HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock );
private:
	IClassFactory* m_origfactory;
	long m_cRef;
};

class CTrayNotifyWrapper : public ITrayNotify8
{
public:
	//constructor
	CTrayNotifyWrapper(ITrayNotify7* notify7);
	//destructor
	~CTrayNotifyWrapper();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//ITrayNotify8
	HRESULT STDMETHODCALLTYPE RegisterCallback(IUnknown*,ULONG*);
	HRESULT STDMETHODCALLTYPE UnregisterCallback(ULONG);
	HRESULT STDMETHODCALLTYPE SetPreference(PVOID*);
	HRESULT STDMETHODCALLTYPE EnableAutoTray(int);
	HRESULT STDMETHODCALLTYPE DoAction(PVOID*,int);
	HRESULT STDMETHODCALLTYPE SetWindowingEnvironmentConfig(IUnknown*);
private:
	ITrayNotify7* m_notify7;
	long m_cRef;
};
