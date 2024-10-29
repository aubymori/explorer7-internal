#pragma once
#include "framework.h"

#pragma region GUID definitions
DEFINE_GUID(IID_IAssociationElementXP, 0xE58B1ABF, 0x9596, 0x4DBA, 0x89,0x97, 0x89,0xDC,0xDE,0xF4,0x69,0x92);
DEFINE_GUID(IID_IAssociationElement, 0xD8F6AD5B, 0xB44F, 0x4BCC, 0x88,0xFD, 0xEB,0x34,0x73,0xDB,0x75,0x02);
#pragma endregion

typedef enum tagASSOCQUERY ASSOCQUERY;

MIDL_INTERFACE("E58B1ABF-9596-4DBA-8997-89DCDEF46992")
IAssociationElementXP: public IUnknown
{
public:
	STDMETHOD(QueryString)(ASSOCQUERY, PCWSTR, PWSTR *) PURE;
	STDMETHOD(QueryDword)(ASSOCQUERY, PCWSTR, DWORD *) PURE;
	STDMETHOD(QueryExists)(ASSOCQUERY, PCWSTR) PURE;
	STDMETHOD(QueryDirect)(ASSOCQUERY, PCWSTR, FLAGGED_BYTE_BLOB **) PURE;
	STDMETHOD(QueryObject)(ASSOCQUERY, PCWSTR, REFIID, PVOID *) PURE;
};

MIDL_INTERFACE("D8F6AD5B-B44F-4BCC-88FD-EB3473DB7502")
IAssociationElement: public IUnknown
{
public:
	STDMETHOD(QueryString)(ASSOCQUERY, PCWSTR, PWSTR *) PURE;
	STDMETHOD(QueryDword)(ASSOCQUERY, PCWSTR, DWORD *) PURE;
	STDMETHOD(QueryGuid)(ASSOCQUERY, PCWSTR, GUID *) PURE;
	STDMETHOD(QueryExists)(ASSOCQUERY, PCWSTR) PURE;
	STDMETHOD(QueryDirect)(ASSOCQUERY, PCWSTR, FLAGGED_BYTE_BLOB **) PURE;
	STDMETHOD(QueryObject)(ASSOCQUERY, PCWSTR, REFIID, PVOID *) PURE;
};

class CAssociationElementWrapper : public IAssociationElementXP
{
private:
	IAssociationElement *m_assoc;
	
public:
	CAssociationElementWrapper(IAssociationElement *assoc);
	~CAssociationElementWrapper();

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// IAssociationElement
	STDMETHODIMP QueryString(ASSOCQUERY p1, PCWSTR p2, PWSTR *p3);
	STDMETHODIMP QueryDword(ASSOCQUERY p1, PCWSTR p2, DWORD *p3);
	STDMETHODIMP QueryExists(ASSOCQUERY p1, PCWSTR p2);
	STDMETHODIMP QueryDirect(ASSOCQUERY p1, PCWSTR p2, FLAGGED_BYTE_BLOB **p3);
	STDMETHODIMP QueryObject(ASSOCQUERY p1, PCWSTR p2, REFIID p3, PVOID *p4);
};