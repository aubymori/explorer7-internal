#include "associationelement.h"

CAssociationElementWrapper::CAssociationElementWrapper(IAssociationElement *assoc)
	: m_assoc(assoc)
{
	m_assoc->AddRef();
}

CAssociationElementWrapper::~CAssociationElementWrapper()
{
	if (m_assoc)
		m_assoc->Release();
}

STDMETHODIMP CAssociationElementWrapper::QueryInterface(REFIID riid, void **ppv)
{
	if (ppv && riid == IID_IAssociationElementXP)
	{
		*ppv = static_cast<IAssociationElementXP *>(this);
		AddRef();
		return S_OK;
	}
	return m_assoc->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CAssociationElementWrapper::AddRef()
{
	return m_assoc->AddRef();
}

STDMETHODIMP_(ULONG) CAssociationElementWrapper::Release()
{
	ULONG cRef = m_assoc->Release();
	if (0 == cRef)
		delete this;
	return cRef;
}

STDMETHODIMP CAssociationElementWrapper::QueryString(ASSOCQUERY p1, PCWSTR p2, PWSTR *p3)
{
	return m_assoc->QueryString(p1, p2, p3);
}

STDMETHODIMP CAssociationElementWrapper::QueryDword(ASSOCQUERY p1, PCWSTR p2, DWORD *p3)
{
	return m_assoc->QueryDword(p1, p2, p3);
}

STDMETHODIMP CAssociationElementWrapper::QueryExists(ASSOCQUERY p1, PCWSTR p2)
{
	return m_assoc->QueryExists(p1, p2);
}

STDMETHODIMP CAssociationElementWrapper::QueryDirect(ASSOCQUERY p1, PCWSTR p2, FLAGGED_BYTE_BLOB **p3)
{
	return m_assoc->QueryDirect(p1, p2, p3);
}

STDMETHODIMP CAssociationElementWrapper::QueryObject(ASSOCQUERY p1, PCWSTR p2, REFIID p3, PVOID *p4)
{
	return m_assoc->QueryObject(p1, p2, p3, p4);
}