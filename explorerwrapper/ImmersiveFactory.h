#pragma once
#include "common.h"
#include "ImmersiveGUIDs.h"

// Ittr: The entirety of ImmersiveFactory is used for Windows 8.1's partial immersive state.
// It must therefore remain in place. ImmersiveShell is used instead for Windows 10, version 1507 and later.

class CImmersiveFactory : public IClassFactory
{
public:
	//IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);    
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IClassFactory
	STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
	STDMETHODIMP LockServer(BOOL fLock);
};

class CImmersiveProvider : public IServiceProvider
{
public:
	CImmersiveProvider();

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IServiceProvider
	STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void** ppv);

private:
	long m_cRef;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveMonitorManager: public IUnknown
{
public:
	STDMETHOD(GetCount)(UINT*) PURE;
	STDMETHOD(GetConnectedCount)(UINT*) PURE;
	STDMETHOD(GetAt)(UINT, IUnknown**) PURE;
	STDMETHOD(GetFromHandle)(HMONITOR, IUnknown**) PURE;
	STDMETHOD(GetFromIdentity)(ULONG, IUnknown**) PURE;
	STDMETHOD(GetImmersiveProxyMonitor)(IUnknown**) PURE;
	STDMETHOD(QueryService)(HMONITOR, REFGUID guidService, REFIID riid, void** ppv) PURE;
	STDMETHOD(QueryServiceByIdentity)(ULONG, REFGUID guidService, REFIID riid, void** ppv) PURE;
	STDMETHOD(QueryServiceFromWindow)(HWND, REFGUID guidService, REFIID riid, void** ppv) PURE;
	STDMETHOD(QueryServiceFromPoint)(tagPOINT*, REFGUID guidService, REFIID riid, void** ppv) PURE;
	STDMETHOD(MoveImmersiveMonitor)(int) PURE;
	STDMETHOD(SetImmersiveMonitor)(IUnknown*) PURE;
};

class CImmersiveMonitorManager : public IImmersiveMonitorManager
{
public:
	CImmersiveMonitorManager();

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IImmersiveMonitorManager
	STDMETHODIMP GetCount(UINT*);
	STDMETHODIMP GetConnectedCount(UINT*);
	STDMETHODIMP GetAt(UINT, IUnknown**);
	STDMETHODIMP GetFromHandle(HMONITOR, IUnknown**);
	STDMETHODIMP GetFromIdentity(ULONG, IUnknown**);
	STDMETHODIMP GetImmersiveProxyMonitor(IUnknown**);
	STDMETHODIMP QueryService(HMONITOR, REFGUID guidService, REFIID riid, void** ppv);
	STDMETHODIMP QueryServiceByIdentity(ULONG, REFGUID guidService, REFIID riid, void** ppv);
	STDMETHODIMP QueryServiceFromWindow(HWND, REFGUID guidService, REFIID riid, void** ppv);
	STDMETHODIMP QueryServiceFromPoint(tagPOINT*, REFGUID guidService, REFIID riid, void** ppv);
	STDMETHODIMP MoveImmersiveMonitor(int);
	STDMETHODIMP SetImmersiveMonitor(IUnknown*);
private:
	long m_cRef;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveLayout: public IUnknown
{
public:
	STDMETHOD(RegisterLayoutClient)(UINT, IUnknown*, ULONG*) PURE;
	STDMETHOD(UnregisterLayoutClient)(ULONG) PURE;
	STDMETHOD(RegisterForLayoutChanges)(UINT, IUnknown*, ULONG*) PURE;
	STDMETHOD(UnregisterForLayoutChanges)(ULONG) PURE;
	STDMETHOD(GetOuterWorkAreaForBand)(ULONG, tagRECT*) PURE;
	STDMETHOD(GetInnerWorkAreaForBand)(ULONG, tagRECT*) PURE;
	STDMETHOD(GetImmersiveShellWorkArea)(tagRECT*) PURE;
	STDMETHOD(InvalidateWorkArea)(ULONG) PURE;
	STDMETHOD(GetBandWorkAreaCount)(void) PURE;
	STDMETHOD(GetBandWorkAreaAt)(UINT, IUnknown**) PURE;
};

class CImmersiveLayout : public IImmersiveLayout
{
public:
	CImmersiveLayout(HMONITOR hMonitor);

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IImmersiveMonitorManager
	STDMETHODIMP RegisterLayoutClient(UINT, IUnknown*, ULONG*);
	STDMETHODIMP UnregisterLayoutClient(ULONG);
	STDMETHODIMP RegisterForLayoutChanges(UINT, IUnknown*, ULONG*);
	STDMETHODIMP UnregisterForLayoutChanges(ULONG);
	STDMETHODIMP GetOuterWorkAreaForBand(ULONG, tagRECT*);
	STDMETHODIMP GetInnerWorkAreaForBand(ULONG, tagRECT*);
	STDMETHODIMP GetImmersiveShellWorkArea(tagRECT*);
	STDMETHODIMP InvalidateWorkArea(ULONG);
	STDMETHODIMP GetBandWorkAreaCount(void);
	STDMETHODIMP GetBandWorkAreaAt(UINT, IUnknown**);
private:
	HMONITOR m_hMonitor;
	long m_cRef;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveMode: public IUnknown
{
public:
	STDMETHOD(GetMode)(DWORD*) PURE;
	STDMETHOD(SetMode)(DWORD) PURE;
};

class CImmersiveMode : public IImmersiveMode
{
public:
	CImmersiveMode();

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IImmersiveMonitorManager
	STDMETHODIMP GetMode(DWORD*);
	STDMETHODIMP SetMode(DWORD);
private:
	long m_cRef;
};

void RegisterFakeImmersive();
void UnregisterFakeImmersive();

