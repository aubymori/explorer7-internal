#pragma once
#include "common.h"

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IAppResolverNotify7: public IUnknown
{
public:
	STDMETHOD(RegisterNotification)(UINT, long, PVOID*, int) PURE;
	STDMETHOD(UnregisterNotification)(UINT) PURE;
	STDMETHOD(AddHotkey)(UINT, PVOID*, PVOID*, int) PURE;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IAppResolverNotify8: public IUnknown
{
public:
	STDMETHOD(OnChangeNotify)(long, PVOID*, PVOID*) PURE;
	STDMETHOD(AddHotkey)(UINT, PVOID*, PVOID*, int) PURE;
};

class CAppResolverNotify8: public IAppResolverNotify8
{
public:
	//constructor
	CAppResolverNotify8(IAppResolverNotify7 *oldresolver);

	//destructor
	~CAppResolverNotify8();

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//IAppResolverNotify8
	STDMETHODIMP OnChangeNotify(long, PVOID*, PVOID*);
	STDMETHODIMP AddHotkey(UINT, PVOID*, PVOID*, int);

private:
	IAppResolverNotify7 *m_resolver7;
	long m_cRef;
};
