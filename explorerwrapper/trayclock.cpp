#include "trayclock.h"

BOOL ShowTrayClock(HWND hWnd)
{
    if (!hWnd) return FALSE;
    HRESULT hr = S_OK;
    ITrayClock *pClock = NULL;
    hr = CoCreateInstance(
        (REFCLSID)CLSID_TrayClock,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
        (REFIID)IID_ITrayClock,
        (void**)&pClock
    );
    if (SUCCEEDED(hr))
    {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        pClock->Show(hWnd, &rc);
        pClock->Release();
    }
    return TRUE;
}