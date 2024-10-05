#pragma once
#include "framework.h"

DEFINE_GUID(GUID_88df9332_6adb_4604_8218_508673ef7f8a, 0x88df9332, 0x6adb, 0x4604, 0x82, 0x18, 0x50, 0x86, 0x73, 0xef, 0x7f, 0x8a);
DEFINE_GUID(GUID_4f33718d_bae1_4f9b_96f2_d2a16e683346, 0x4f33718d, 0xbae1, 0x4f9b, 0x96, 0xf2, 0xd2, 0xa1, 0x6e, 0x68, 0x33, 0x46);

typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned long uint;
typedef unsigned char uchar;

MIDL_INTERFACE("4f33718d-bae1-4f9b-96f2-d2a16e683346")
IShellURL7 : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE ParseFromOutsideSource(ushort const*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUrl(ushort*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetUrl(ushort const*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDisplayName(ushort*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPidl(LPITEMIDLIST*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPidl(LPITEMIDLIST) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPidlAndArgs(LPITEMIDLIST, ushort const*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetArgs(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddPath(LPITEMIDLIST) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCancelObject(void*) = 0;
	virtual HRESULT STDMETHODCALLTYPE StartAsyncPathParse(HWND__*, ushort const*, ulong, void*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetParseResult(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetUsnSource(ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUsnSource(ulong*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetNavFlags(int, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCookie(ulong*) = 0;
	virtual HRESULT STDMETHODCALLTYPE Execute(void*, int*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCurrentWorkingDir(LPITEMIDLIST) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMessageBoxParent(HWND__*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPidlNoGenerate(LPITEMIDLIST*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetStandardParsingFlags(int) = 0;
};

MIDL_INTERFACE("88df9332-6adb-4604-8218-508673ef7f8a")
IShellURL10 : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE ParseFromOutsideSource(ushort const*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUrl(ushort*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetUrl(ushort const*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDisplayName(ushort*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPidl(LPITEMIDLIST*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPidl(LPITEMIDLIST) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPidlAndArgs(LPITEMIDLIST, ushort const*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetArgs(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddPath(LPITEMIDLIST) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCancelObject(void*) = 0;
	virtual HRESULT STDMETHODCALLTYPE StartAsyncPathParse(HWND__*, ushort const*, ulong, void*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetParseResult(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetRequestID(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRequestID(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetNavFlags(int, int) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCookie(ulong*) = 0;
	virtual HRESULT STDMETHODCALLTYPE Execute(void*, int*, ulong) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCurrentWorkingDir(LPITEMIDLIST) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMessageBoxParent(HWND__*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPidlNoGenerate(LPITEMIDLIST*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetStandardParsingFlags(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUrlAlloc(ushort**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDisplayNameAlloc(ushort**) = 0;
};

class CShellURLWrapper : public IShellURL7
{
public:
	CShellURLWrapper(IShellURL10* actual);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE ParseFromOutsideSource(ushort const*, ulong);
	HRESULT STDMETHODCALLTYPE GetUrl(ushort*, ulong);
	HRESULT STDMETHODCALLTYPE SetUrl(ushort const*, ulong);
	HRESULT STDMETHODCALLTYPE GetDisplayName(ushort*, ulong);
	HRESULT STDMETHODCALLTYPE GetPidl(LPITEMIDLIST*);
	HRESULT STDMETHODCALLTYPE SetPidl(LPITEMIDLIST);
	HRESULT STDMETHODCALLTYPE SetPidlAndArgs(LPITEMIDLIST, ushort const*);
	HRESULT STDMETHODCALLTYPE GetArgs(void);
	HRESULT STDMETHODCALLTYPE AddPath(LPITEMIDLIST);
	HRESULT STDMETHODCALLTYPE SetCancelObject(void*);
	HRESULT STDMETHODCALLTYPE StartAsyncPathParse(HWND__*, ushort const*, ulong, void*);
	HRESULT STDMETHODCALLTYPE GetParseResult(void);
	HRESULT STDMETHODCALLTYPE SetUsnSource(ulong);
	HRESULT STDMETHODCALLTYPE GetUsnSource(ulong*);
	HRESULT STDMETHODCALLTYPE SetNavFlags(int, int);
	HRESULT STDMETHODCALLTYPE GetCookie(ulong*);
	HRESULT STDMETHODCALLTYPE Execute(void*, int*, ulong);
	HRESULT STDMETHODCALLTYPE SetCurrentWorkingDir(LPITEMIDLIST);
	HRESULT STDMETHODCALLTYPE SetMessageBoxParent(HWND__*);
	HRESULT STDMETHODCALLTYPE GetPidlNoGenerate(LPITEMIDLIST*);
	HRESULT STDMETHODCALLTYPE GetStandardParsingFlags(int);

	ULONG m_cRef;
	IShellURL10* m_actual;
};