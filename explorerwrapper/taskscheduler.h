#pragma once
#define INITGUID
#include "framework.h"

DEFINE_GUID(IID_IShellTaskScheduler7, 0x6CCB7BE0, 0x6807, 0x11D0, 0x0B8, 0x10, 0x0, 0x0C0, 0x4F, 0x0D7, 0x6, 0x0EC); //{6CCB7BE0-6807-11D0-B810-00C04FD706EC}
DEFINE_GUID(IID_IShellTaskSchedulerSettings7, 0x4BC6CE0A, 0x2B39, 0x4F63, 0x89, 0x0C1, 0x3B, 0x0EA, 0x0A7, 0x0BD, 0x0E0, 0x2A); //_GUID_4bc6ce0a_2b39_4f63_89c1_3beaa7bde02a
DEFINE_GUID(IID_IShellTaskSchedulerSettings8, 0x0A8272E00, 0x0A569, 0x40D2, 0x9D, 0x0AC, 0x0B7, 0x75, 0x6F, 0x0A0, 0x92, 0xC4); //_GUID_a8272e00_a569_40d2_9dac_b7756fa092c4

/*
CShellTaskScheduler::QueryInterface(_GUID const &,void * *)
CShellTaskScheduler::AddRef(void)
CShellTaskScheduler::Release(void)
CShellTaskScheduler::AddTask(IRunnableTask *,_GUID const &,unsigned __int64,ulong)
CShellTaskScheduler::RemoveTasks(_GUID const &,unsigned __int64,int)
CShellTaskScheduler::CountTasks(_GUID const &)
CShellTaskScheduler::Status(ulong,ulong)
CShellTaskScheduler::AddTask2(IRunnableTask *,_GUID const &,unsigned __int64,ulong,ulong)
CShellTaskScheduler::MoveTask(_GUID const &,unsigned __int64,ulong,ulong)
*/
MIDL_INTERFACE("6CCB7BE0-6807-11D0-B810-00C04FD706EC")
IShellTaskScheduler7
{
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
    virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddTask(IRunnableTask*,_GUID const&,unsigned __int64,ULONG) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveTasks(_GUID const&,unsigned __int64,int) = 0;
    virtual __int64 STDMETHODCALLTYPE CountTasks(_GUID const&) = 0;
    virtual HRESULT STDMETHODCALLTYPE Status(ULONG, ULONG) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddTask2(IRunnableTask*,_GUID const&,unsigned __int64, ULONG, ULONG) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveTask(_GUID const&,unsigned __int64, ULONG, ULONG) = 0;
};

MIDL_INTERFACE("4bc6ce0a-2b39-4f63-89c1-3beaa7bde02a")
IShellTaskSchedulerSettings7
{
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
    virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetWorkerThreadCountMax(int) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetWorkerThreadPriority(int) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFlags(DWORD,DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFlags(DWORD*) = 0;
};

MIDL_INTERFACE("a8272e00-a569-40d2-9dac-b7756fa092c4")
IShellTaskSchedulerSettings8
{
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
    virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetWorkerThreadCountMax(int) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWorkerThreadCountMax(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetWorkerThreadPriority(int) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFlags(DWORD,DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFlags(DWORD*) = 0;
};

class CShellTaskSchedulerSettingsWrapper : public IShellTaskSchedulerSettings7
{
public:

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);
    HRESULT STDMETHODCALLTYPE SetWorkerThreadCountMax(int);
    HRESULT STDMETHODCALLTYPE SetWorkerThreadPriority(int);
    HRESULT STDMETHODCALLTYPE SetFlags(DWORD, DWORD);
    HRESULT STDMETHODCALLTYPE GetFlags(DWORD*);

    IShellTaskSchedulerSettings8* m_TaskSchedulerSettings;

    CShellTaskSchedulerSettingsWrapper(IShellTaskSchedulerSettings8* actualInstance)
    {
        m_TaskSchedulerSettings = actualInstance;
    }
};

class CShellTaskSchedulerWrapper : public IShellTaskScheduler7
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);
    HRESULT STDMETHODCALLTYPE AddTask(IRunnableTask*, _GUID const&, unsigned __int64, ULONG);
    HRESULT STDMETHODCALLTYPE RemoveTasks(_GUID const&, unsigned __int64, int);
    __int64 STDMETHODCALLTYPE CountTasks(_GUID const&);
    HRESULT STDMETHODCALLTYPE Status(ULONG, ULONG);
    HRESULT STDMETHODCALLTYPE AddTask2(IRunnableTask*, _GUID const&, unsigned __int64, ULONG, ULONG);
    HRESULT STDMETHODCALLTYPE MoveTask(_GUID const&, unsigned __int64, ULONG, ULONG);

    IShellTaskScheduler7* m_TaskScheduler;

    CShellTaskSchedulerWrapper(IShellTaskScheduler7* actualInstance)
    {
        m_TaskScheduler = actualInstance;
    };
};