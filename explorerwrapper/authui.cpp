#include "authui.h"
#include "dbgprint.h"

CAuthUIWrapper::CAuthUIWrapper(IShutdownChoices8 *authui)
{
	m_cRef = 1;
	m_authui8 = authui;
}

CAuthUIWrapper::~CAuthUIWrapper()
{
	m_authui8->Release();
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::QueryInterface(REFIID riid,void **ppvObject)
{
	return m_authui8->QueryInterface(riid,ppvObject);
	//if (riid == IID_IShutdownChoices7)
	//{
	//	dbgprintf(L"IID_IShutdownChoices7\n");
	//	HRESULT ret = m_authui8->QueryInterface(IID_IShutdownChoices8, (PVOID*)&m_startmenuiconscache8);
	//	if (ret == S_OK)
	//	{
	//		dbgprintf(L"S_OK\n");
	//		*ppvObject = static_cast<IShutdownChoices7*>(this);
	//		AddRef();
	//	}
	//	return ret;
	//}
	//return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAuthUIWrapper::AddRef(void)
{
	m_authui8->AddRef();
	return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE CAuthUIWrapper::Release(void)
{
	m_authui8->Release();
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::Refresh(void)
{
	return m_authui8->Refresh();
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::CreateListener(IUnknown** p1)
{
	return m_authui8->CreateListener(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::SetChoiceMask(ULONG p1)
{
	p1 = p1 & ~0x200000;
	dbgprintf(L"SetChoiceMask %p",p1);
	return m_authui8->SetChoiceMask(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetMessageWnd(HWND** p1)
{
	return E_NOTIMPL;
	//return m_authui->GetMessageWnd(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::SetShowBadChoices(int p1)
{
	return m_authui8->SetShowBadChoices(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetChoiceEnumerator(IUnknown** p1)
{
	return m_authui8->GetChoiceEnumerator(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetDefaultChoice(ULONG* p1)
{
	return m_authui8->GetDefaultChoice(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::UserHasShutdownRights(void)
{
	return m_authui8->UserHasShutdownRights();
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetChoiceName(ULONG p1,int p2,LPWSTR p3,UINT p4)
{
	HRESULT ret = m_authui8->GetChoiceName(p1,p2,p3,p4);
	dbgprintf(L"GetChoiceName %d %d %s %d",p1,p2,p3,p4);
	return ret;
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetChoiceDesc(ULONG p1,LPWSTR p2,UINT p3)
{
	HRESULT ret = m_authui8->GetChoiceDesc(p1,p2,p3);
	return ret;
}
