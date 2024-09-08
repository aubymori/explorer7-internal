#include "immersiveshell.h"
#include "dbgprint.h"

DWORD WINAPI TwinThread( LPVOID lpParameter )
{
	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
	IImmersiveBehavior* behavior;
	HRESULT ret = CoUnmarshalInterface((IStream*)lpParameter, IID_ImmersiveBehavior, (PVOID*)&behavior);
	dbgprintf(L"IImmersiveBehavior %p %p",ret,behavior);
	UINT count;	
	behavior->GetMaximumComponentCount(&count);
	UINT i;
	for (i=0;i<count;i++)
	{
		dbgprintf(L"creating TwinUI component %d",i);
		IUnknown* component;
		HRESULT ret = behavior->CreateComponent(i,&component);
		dbgprintf(L"created TwinUI component %p %p",ret,component);
	}
	return 0;
}

void CreateTwinUI()
{
	IImmersiveShellCreator* ImmersiveShellCreator;
	if ( SUCCEEDED(CoCreateInstance(CLSID_ImmersiveShellBuilder,NULL, CLSCTX_INPROC_SERVER, IID_ImmersiveShellBuilder, (LPVOID*)&ImmersiveShellCreator)))
	{
		dbgprintf(L"TwinUI factory created!");

		IImmersiveShellController* controller;
		HRESULT ret = ImmersiveShellCreator->CreateShell(&controller);
		dbgprintf(L"TwinUI instance created %p %p",ret,controller);		
		if ( SUCCEEDED(ret) )
		{	
			HRESULT hr = controller->Start();

			dbgprintf(L"Immersive Shell Controller Result: %x", hr);
		}
	}
}


CImmersiveBehaviorWrapper::CImmersiveBehaviorWrapper(IImmersiveBehavior *behavior)
{
	m_cRef = 1;
	m_behavior = behavior;
	m_behavior->AddRef();
}

CImmersiveBehaviorWrapper::~CImmersiveBehaviorWrapper()
{
	dbgprintf(L"CImmersiveBehaviorWrapper::~CImmersiveBehaviorWrapper()");
	m_behavior->Release();
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::QueryInterface(REFIID riid,void **ppvObject)
{
	WCHAR iid[100];
	StringFromGUID2(riid,iid,100);
	dbgprintf(L"CImmersiveBehaviorWrapper::QueryInterface %s",iid);
	if (riid == IID_ImmersiveBehavior)
	{
		*ppvObject = static_cast<IImmersiveBehavior*>(this);
		return S_OK;
	}
	return m_behavior->QueryInterface(riid,ppvObject);
}

ULONG STDMETHODCALLTYPE CImmersiveBehaviorWrapper::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE CImmersiveBehaviorWrapper::Release(void)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::release()");
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRef;
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::OnImmersiveThreadStart(void)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::OnImmersiveThreadStart");
	return m_behavior->OnImmersiveThreadStart();
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::OnImmersiveThreadStop(void)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::OnImmersiveThreadStop");
	return m_behavior->OnImmersiveThreadStart();
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::GetMaximumComponentCount(unsigned int *count)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::GetMaximumComponentCount %p",count);
	return m_behavior->GetMaximumComponentCount(count);
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::CreateComponent(unsigned int number, IUnknown** component)
{	
	//if (number == 1) DebugBreak();
	HRESULT ret = m_behavior->CreateComponent(number,component);
	dbgprintf(L"CImmersiveBehaviorWrapper::CreateComponent %d = %p",number,ret);
	/*IUnknown* wtf = *component;
	HRESULT ret2 = wtf->QueryInterface(IID_ImmersiveShell,(PVOID*)&wtf);
	dbgprintf(L"CImmersiveBehaviorWrapper::GetInterfaceList %p",ret2);*/
	return ret;
}

HRESULT STDMETHODCALLTYPE CImmersiveBehaviorWrapper::ShouldCreateComponent(unsigned int number, int* allowed)
{
	dbgprintf(L"CImmersiveBehaviorWrapper::ShouldCreateComponent %d %p",number,allowed);	
	if (number == 9)
	{
		*allowed = 0;
		return S_OK;
	}		
	return m_behavior->ShouldCreateComponent(number,allowed);
}