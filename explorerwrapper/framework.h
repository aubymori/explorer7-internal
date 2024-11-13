#pragma once

// Windows Header Files
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <ShlObj.h>
#include <Shobjidl.h>
#include <Uxtheme.h>
#include <guiddef.h>
#include <dwmapi.h>
#include <winscard.h>
#include <propkey.h>
#include <DocObj.h>
#include <KnownFolders.h>
#include <bcrypt.h>
#include <propvarutil.h>
#include <strsafe.h>

extern bool g_bGinaUI;

#define NOATOMICRELESEFUNC
#ifndef ATOMICRELEASE
#ifdef __cplusplus
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->Release();} }
#else
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->lpVtbl->Release(punkT);} }
#endif

// doing this as a function instead of inline seems to be a size win.
//
#ifdef NOATOMICRELESEFUNC
#define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#else
#   ifdef __cplusplus
#       define ATOMICRELEASE(p) IUnknown_SafeReleaseAndNullPtr(p)
#   else
#       define ATOMICRELEASE(p) IUnknown_AtomicRelease((LPVOID*)&p)
#   endif
#endif
#endif //ATOMICRELEASE