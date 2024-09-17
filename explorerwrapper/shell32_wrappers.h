#pragma once
#include "framework.h"

DEFINE_GUID(CLSID_ProgramsFolderAndFastItems,0x865E5E76, 0x0AD83, 0x4DCA, 0x0A1, 0x9, 0x50, 0x0DC, 0x21, 0x13, 0x0CE, 0x9A);
DEFINE_GUID(CLSID_MergedFolder, 0x26FDC864, 0x0BE88, 0x46E7, 0x92, 0x35, 0x3, 0x2D, 0x8E, 0x0A5, 0x16, 0x2E);
DEFINE_GUID(GUID_2f711b17_773c_41d4_93fa_7f23edcecb66, 0x2f711b17, 0x773c, 0x41d4, 0x93, 0xfa, 0x7f, 0x23, 0xed, 0xce, 0xcb, 0x66);

class CProgramsFolderClassFactory : public IClassFactory
{
public:
	//constructor
	CProgramsFolderClassFactory();
	//destructor
	~CProgramsFolderClassFactory();
	//IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	//IClassFactory
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
	HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

};