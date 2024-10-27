#include "userassist.h"

IUserAssist* g_UserAssist;

extern "C" HRESULT WINAPI Explorer_CoCreateInstance(
	__in   REFCLSID rclsid,
	__in   LPUNKNOWN pUnkOuter,
	__in   DWORD dwClsContext,
	__in   REFIID riid,
	__out  LPVOID * ppv
);

HRESULT WINAPI UAQueryEntry(REFIID iid, WPARAM parsingname, PUEMINFO uem)
{	
	if (!g_UserAssist) //bugbug shpindllofclsid?
		Explorer_CoCreateInstance(CLSID_UserAssist,NULL,CLSCTX_INPROC_SERVER || CLSCTX_INPROC_HANDLER || CLSCTX_NO_CODE_DOWNLOAD,IID_IUserAssist7,(PVOID*)&g_UserAssist);
	if ( g_UserAssist )
		return g_UserAssist->QueryEntry(iid,parsingname,uem);
	else
		return E_NOINTERFACE;
}

HRESULT WINAPI UAQueryShortcut(LPITEMIDLIST pidl, PUEMINFO uem)
{
	HRESULT result;
	IShellItem* shellitem;
	result = SHCreateItemFromIDList(pidl,IID_IShellItem,(LPVOID*)&shellitem);
	if ( SUCCEEDED(result) )
	{
		LPWSTR parsingname;
		result = shellitem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING,&parsingname);
		if ( SUCCEEDED(result) )
		{
			uem->cbSize = sizeof(UEMINFO);
			uem->dwMask = 0x31;			
			result = UAQueryEntry(UAIID_SHORTCUTS,(WPARAM)parsingname,uem);		
			CoTaskMemFree(parsingname);
		}
	}
	return result;
}

LPWSTR WINAPI CoAllocString(LPWSTR src)
{	
	LPWSTR dst = (LPWSTR)CoTaskMemAlloc(((lstrlenW(src)+1)*sizeof(WCHAR)));
	lstrcpyW(dst,src);
	return dst;
}

CUserAssistXPWrapper::CUserAssistXPWrapper(IUserAssist* newAssist)
{
	m_newAssist = newAssist;
}

STDMETHODIMP_(HRESULT __stdcall) CUserAssistXPWrapper::QueryInterface(REFIID riid, void** ppv)
{
	return m_newAssist->QueryInterface(riid,ppv);
}

STDMETHODIMP_(ULONG __stdcall) CUserAssistXPWrapper::AddRef(void)
{
	if (m_newAssist)
		m_newAssist->AddRef();
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG __stdcall) CUserAssistXPWrapper::Release(void)
{
	if (m_newAssist)
		m_newAssist->Release();
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		free((void*)this);
		return 0;
	}
	return m_cRef;
}

STDMETHODIMP_(HRESULT __stdcall) CUserAssistXPWrapper::FireEvent(const GUID* pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam)
{
	return m_newAssist->FireEvent(*pguidGrp,eCmd,wParam,lParam);
}

STDMETHODIMP_(HRESULT __stdcall) CUserAssistXPWrapper::QueryEvent(const GUID* pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, PUEMINFO pui)
{
	return m_newAssist->QueryEntry(*pguidGrp,wParam,pui);
}

STDMETHODIMP_(HRESULT __stdcall) CUserAssistXPWrapper::SetEvent(const GUID* pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, PUEMINFO pui)
{
	return m_newAssist->SetEntry(*pguidGrp,wParam,pui);
}
