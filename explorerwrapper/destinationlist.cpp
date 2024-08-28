#include "destinationlist.h"

CAutoDestWrapper::CAutoDestWrapper(IAutoDestinationList10* destlist)
{
	m_dest10 = destlist;
}

CAutoDestWrapper::~CAutoDestWrapper()
{
	m_dest10->Release();
}

HRESULT __stdcall CAutoDestWrapper::QueryInterface(REFIID riid, void** ppvObject)
{
	return m_dest10->QueryInterface(riid, ppvObject);
}

ULONG __stdcall CAutoDestWrapper::AddRef(void)
{
	return m_dest10->AddRef();
}

ULONG __stdcall CAutoDestWrapper::Release(void)
{
	ULONG cref;
	cref = m_dest10->Release();
	if (cref == 0)
		free((void*)this);
	return cref;
}

HRESULT __stdcall CAutoDestWrapper::Initialize(const wchar_t* p1, const wchar_t* p2, const wchar_t* p3)
{
	return m_dest10->Initialize(p1, p2, p3);
}

HRESULT __stdcall CAutoDestWrapper::HasList(int* p1)
{
	return m_dest10->HasList(p1);
}

HRESULT __stdcall CAutoDestWrapper::GetList(int p1, unsigned int p2, REFGUID p3, void** p4)
{
	return m_dest10->GetList(p1, p2, 0, p3, p4);
}

HRESULT __stdcall CAutoDestWrapper::AddUsagePoint(IUnknown* p1)
{
	return m_dest10->AddUsagePoint(p1);
}

HRESULT __stdcall CAutoDestWrapper::PinItem(IUnknown* p1, int p2)
{
	return m_dest10->PinItem(p1, p2);
}

HRESULT __stdcall CAutoDestWrapper::IsPinned(IUnknown* p1, int* p2)
{
	return m_dest10->IsPinned(p1, p2);
}

HRESULT __stdcall CAutoDestWrapper::RemoveDestination(IUnknown* p1)
{
	return m_dest10->RemoveDestination(p1);
}

HRESULT __stdcall CAutoDestWrapper::SetUsageData(IUnknown* p1, float* p2, FILETIME* p3)
{
	return m_dest10->SetUsageData(p1, p2, p3);
}

HRESULT __stdcall CAutoDestWrapper::GetUsageData(IUnknown* p1, float* p2, FILETIME* p3)
{
	return m_dest10->GetUsageData(p1, p2, p3);
}

HRESULT __stdcall CAutoDestWrapper::ResolveDestination(HWND p1, unsigned long p2, IShellItem* p3, REFGUID p4, void** p5)
{
	return m_dest10->ResolveDestination(p1, p2, p3, p4, p5);
}

HRESULT __stdcall CAutoDestWrapper::ClearList(int p1)
{
	return m_dest10->ClearList(p1);
}
