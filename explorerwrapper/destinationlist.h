#pragma once
#include <Guiddef.h>
#include <windows.h>
#include <ShlObj.h>

DEFINE_GUID(CLSID_AutomaticDestinationList, 
	0x0F0AE1542, 0x0F497, 0x484B, 0xA1, 0x75, 0xA2, 0x0D, 0xB0, 0x91, 0x44, 0xBA);
DEFINE_GUID(IID_AutoDestList,
	0xBC10DCE3, 0x62F2, 0x4BC6, 0xAF, 0x37, 0xDB, 0x46, 0xED, 0x78, 0x73, 0xC4);
DEFINE_GUID(IID_AutoDestList10,
	0xE9C5EF8D, 0xFD41, 0x4F72, 0xBA, 0x87, 0xEB, 0x03, 0xBA, 0xD5, 0x81, 0x7C);

DEFINE_GUID(IID_CustomDestList, 0x03f1eed2, 0x8676, 0x430b, 0xab, 0xe1, 0x76, 0x5c, 0x1d, 0x8f, 0xe1, 0x47);

MIDL_INTERFACE("bc10dce3-62f2-4bc6-af37-db46ed7873c4")
IAutoDestinationList: public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Initialize(const wchar_t*, const wchar_t*, const wchar_t*) = 0;
	virtual HRESULT STDMETHODCALLTYPE HasList(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetList(int, unsigned int, REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddUsagePoint(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE PinItem(IUnknown*, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsPinned(IUnknown*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveDestination(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetUsageData(IUnknown*, float*, FILETIME*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUsageData(IUnknown*, float*, FILETIME*) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResolveDestination(HWND, unsigned long, IShellItem*, REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE ClearList(int) = 0;
};


MIDL_INTERFACE("e9c5ef8d-fd41-4f72-ba87-eb03bad5817c")
IAutoDestinationList10: public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Initialize(const wchar_t*, const wchar_t*, const wchar_t*) = 0;
	virtual HRESULT STDMETHODCALLTYPE HasList(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetList(int, unsigned int, int, REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddUsagePoint(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE PinItem(IUnknown*, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsPinned(IUnknown*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveDestination(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetUsageData(IUnknown*, float*, FILETIME*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUsageData(IUnknown*, float*, FILETIME*) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResolveDestination(HWND, unsigned long, IShellItem*, REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE ClearList(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddUsagePointsEx(IUnknown*, int, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE BlockItem(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE ClearBlocked(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE TransferPoints(IUnknown*, IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE HasListEx(int*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetUsageDataInternal(IUnknown*, float*, FILETIME*, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUsageDataInternal(IUnknown*, int, float*, FILETIME*, unsigned int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateRenamedItems(IObjectCollection*, IObjectCollection*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveDeletedItems(IObjectCollection*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddUsagePointsForFolders(IObjectCollection*, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateCachedItems(IObjectCollection*, int*) = 0;
};

class CAutoDestWrapper : public IAutoDestinationList
{
public:
	CAutoDestWrapper(IAutoDestinationList10*);
	~CAutoDestWrapper();

	//IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	//IAutoDestinationList
	HRESULT STDMETHODCALLTYPE Initialize(const wchar_t*, const wchar_t*, const wchar_t*);
	HRESULT STDMETHODCALLTYPE HasList(int*);
	HRESULT STDMETHODCALLTYPE GetList(int, unsigned int, REFGUID, void**);
	HRESULT STDMETHODCALLTYPE AddUsagePoint(IUnknown*);
	HRESULT STDMETHODCALLTYPE PinItem(IUnknown*, int);
	HRESULT STDMETHODCALLTYPE IsPinned(IUnknown*, int*);
	HRESULT STDMETHODCALLTYPE RemoveDestination(IUnknown*);
	HRESULT STDMETHODCALLTYPE SetUsageData(IUnknown*, float*, FILETIME*);
	HRESULT STDMETHODCALLTYPE GetUsageData(IUnknown*, float*, FILETIME*);
	HRESULT STDMETHODCALLTYPE ResolveDestination(HWND, unsigned long, IShellItem*, REFGUID, void**);
	HRESULT STDMETHODCALLTYPE ClearList(int);

private:
	IAutoDestinationList10* m_dest10 = 0;
};