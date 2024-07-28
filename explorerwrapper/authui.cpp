#include "authui.h"
#include "dbgprint.h"

CAuthUIWrapper::CAuthUIWrapper(IShutdownChoices *authui)
{
	m_cRef = 1;
	m_authui = authui;
}

CAuthUIWrapper::~CAuthUIWrapper()
{
	m_authui->Release();
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::QueryInterface(REFIID riid,void **ppvObject)
{
	return m_authui->QueryInterface(riid,ppvObject);
}

ULONG STDMETHODCALLTYPE CAuthUIWrapper::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE CAuthUIWrapper::Release(void)
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::Refresh(void)
{
	return m_authui->Refresh();
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::CreateListener(IUnknown** p1)
{
	return m_authui->CreateListener(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::SetChoiceMask(ULONG p1)
{
	p1 = p1 & ~0x200000;
	dbgprintf(L"SetChoiceMask %p",p1);
	return m_authui->SetChoiceMask(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetMessageWnd(HWND** p1)
{
	return m_authui->GetMessageWnd(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::SetShowBadChoices(int p1)
{
	return m_authui->SetShowBadChoices(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetChoiceEnumerator(IUnknown** p1)
{
	return m_authui->GetChoiceEnumerator(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetDefaultChoice(ULONG* p1)
{
	return m_authui->GetDefaultChoice(p1);
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::UserHasShutdownRights(void)
{
	return m_authui->UserHasShutdownRights();
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetChoiceName(ULONG p1,int p2,LPWSTR p3,UINT p4)
{
	HRESULT ret = m_authui->GetChoiceName(p1,p2,p3,p4);
	dbgprintf(L"GetChoiceName %d %d %s %d",p1,p2,p3,p4);
	return ret;
}

HRESULT STDMETHODCALLTYPE CAuthUIWrapper::GetChoiceDesc(ULONG p1,LPWSTR p2,UINT p3)
{
	HRESULT ret = m_authui->GetChoiceDesc(p1,p2,p3);
	return ret;
}
