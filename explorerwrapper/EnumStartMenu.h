#pragma once
#include "framework.h"
#include "startmenupin.h"
#include "userassist.h"

typedef struct { 
	LPITEMIDLIST pidlParent;	//+0	//+0
	LPITEMIDLIST pidlRelative;	//+4	//+8
	LPWSTR pszAppID;	//+8	//+10
	UEMINFO ueminfo;	//+Ch	//+18
	int iPinPos;		//+30h	//+3C
	DWORD_PTR fNewApp;	//+34h	//+44
} STARTMENUITEM, *PSTARTMENUITEM;	//38h	//48h

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IEnumStartMenuItem: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Next( ULONG celt, PSTARTMENUITEM rgelt, ULONG * pceltFetched ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Skip( ULONG celt ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumStartMenuItem**) = 0;
};

class CEnumStartMenu : public IEnumStartMenuItem
{
public:
	//constructor
	CEnumStartMenu();
	//destructor
	~CEnumStartMenu();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IEnumUnknown
	HRESULT STDMETHODCALLTYPE Clone(IEnumStartMenuItem **ppenum);
	HRESULT STDMETHODCALLTYPE Next(ULONG celt,PSTARTMENUITEM rgelt,ULONG *pceltFetched);
	HRESULT STDMETHODCALLTYPE Reset();
	HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
	//our methods
	void STDMETHODCALLTYPE AddItem(PSTARTMENUITEM item);
	void STDMETHODCALLTYPE Sort();
	void STDMETHODCALLTYPE RemoveDuplicates();
	void STDMETHODCALLTYPE SetLimit(long limit);
private:
	long m_cRef;
	long m_count;
	long m_enumidx;
	long m_limit;
	HDSA hArrItems;
};