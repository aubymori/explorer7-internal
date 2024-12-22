#pragma once
#define INITGUID
#include "framework.h"

DEFINE_GUID(CLSID_ImmersiveShellBuilder,0xc71c41f1, 0xddad, 0x42dc, 0xa8,0xfc,0xf5,0xbf,0xc6,0x1d,0xf9,0x57); //c71c41f1_ddad_42dc_a8fc_f5bfc61df957
DEFINE_GUID(IID_ImmersiveShellBuilder,0x1c56b3e4, 0xe6ea, 0x4ced, 0x8a,0x74,0x73,0xb7,0x2c,0x6b,0xd4,0x35); //1c56b3e4_e6ea_4ced_8a74_73b72c6bd435

DEFINE_GUID(IID_ImmersiveBehavior,0x139275e0, 0xd644, 0x4214, 0xb4,0x5e,0xd9,0x27,0x8c,0x4a,0x85,0x01); //139275e0_d644_4214_b45e_d9278c4a8501

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveBehavior: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE OnImmersiveThreadStart(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE OnImmersiveThreadStop(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMaximumComponentCount(unsigned int *count) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateComponent(unsigned int number, IUnknown** component) = 0;
	virtual HRESULT STDMETHODCALLTYPE ShouldCreateComponent(unsigned int number, int* allowed) = 0;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveShellController: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Start(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Stop(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCreationBehavior(IImmersiveBehavior*) = 0;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveShellCreator: public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE CreateShell(IImmersiveShellController** controller) = 0;
};

class CImmersiveBehaviorWrapper : public IImmersiveBehavior
{
public:
	//constructor
	CImmersiveBehaviorWrapper(IImmersiveBehavior *behavior);
	//destructor
	~CImmersiveBehaviorWrapper();
	//IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);    
    ULONG STDMETHODCALLTYPE AddRef( void);    
    ULONG STDMETHODCALLTYPE Release( void);
	//IImmersiveBehavior
	HRESULT STDMETHODCALLTYPE OnImmersiveThreadStart(void);
	HRESULT STDMETHODCALLTYPE OnImmersiveThreadStop(void);
	HRESULT STDMETHODCALLTYPE GetMaximumComponentCount(unsigned int *count);
	HRESULT STDMETHODCALLTYPE CreateComponent(unsigned int number, IUnknown** component);
	HRESULT STDMETHODCALLTYPE ShouldCreateComponent(unsigned int number, int* allowed);
private:
	IImmersiveBehavior *m_behavior;
	long m_cRef;
};

void CreateTwinUI();
DWORD WINAPI TwinThread( LPVOID lpParameter );