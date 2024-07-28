#pragma once
#include <initguid.h>
#include <objbase.h>

// implementation of Win32Clock
DEFINE_GUID(CLSID_TrayClock,
    0xA323554A,
    0x0FE1, 0x4E49, 0xAE, 0xE1,
    0x67, 0x22, 0x46, 0x5D, 0x79, 0x9F
);
DEFINE_GUID(IID_ITrayClock,
    0x7A5FCA8A,
    0x76B1, 0x44C8, 0xA9, 0x7C,
    0xE7, 0x17, 0x3C, 0xCA, 0x5F, 0x4F
);

MIDL_INTERFACE("7A5FCA8A-76B1-44C8-A97C-E7173CCA5F3F")
ITrayClock : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Show(HWND hWndParent, LPRECT lprc) = 0;
};

BOOL ShowTrayClock(HWND hWnd);