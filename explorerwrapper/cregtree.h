#pragma once
#define INITGUID
#include "framework.h"

DEFINE_GUID(IID_IRegTreeOptions8, 0x7897eca6, 0x1b1b, 0x452a, 0x85, 0x81, 0xbb, 0x94, 0x82, 0xae, 0xa7, 0xcc); //7897eca6_1b1b_452a_8581_bb9482aea7cc
DEFINE_GUID(IID_IRegTreeOptions7, 0xaf4f6511, 0xf982, 0x11d0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4c, 0xD6, 0xD8); //af4f6511_f982_11d0_8595_00aa004cd6d8
DEFINE_GUID(CLSID_RegTreeOptions, 0xAF4F6510, 0xF982, 0x11D0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4C, 0xD6, 0xD8); //AF4F6510-F982-11D0-8595-00AA004CD6D8

MIDL_INTERFACE("7897eca6-1b1b-452a-8581-bb9482aea7cc")
IRegTreeOptions8 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE InitTree(HWND, HKEY, LPCSTR, LPCSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE WalkTree(unsigned int) = 0;
    virtual HRESULT STDMETHODCALLTYPE ToggleItem(_TREEITEM*) = 0;
    virtual HRESULT STDMETHODCALLTYPE ShowHelp(_TREEITEM*, ULONG) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEventListener(IUnknown*) = 0;
};

class CRegTreeOptionsWrapper : public IRegTreeOptions8
{
public:
    CRegTreeOptionsWrapper(IRegTreeOptions8*);
    ~CRegTreeOptionsWrapper();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // IRegTreeOptions
    HRESULT STDMETHODCALLTYPE InitTree(HWND, HKEY, LPCSTR, LPCSTR);
    HRESULT STDMETHODCALLTYPE WalkTree(unsigned int);
    HRESULT STDMETHODCALLTYPE ToggleItem(_TREEITEM*);
    HRESULT STDMETHODCALLTYPE ShowHelp(_TREEITEM*, ULONG);
    HRESULT STDMETHODCALLTYPE SetEventListener(IUnknown*);
private:
    IRegTreeOptions8* regtree;
    long m_cref;
};