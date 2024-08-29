#include "pinnedlist.h"
#include "dbgprint.h"

bool g_bSwapModifyPointers = false;

CPinnedListWrapper::CPinnedListWrapper(IUnknown* flex, int build)
{
	m_build = build;
	if (build >= 10240 && build < 14393)
	{
		m_pinnedList25 = (IPinnedList25*)flex;
		dbgprintf(L"using IPinnedList25");
	}
	else if (build >= 14393 && build < 17763)
	{
		m_flexList = (IFlexibleTaskbarPinnedList*)flex;
		dbgprintf(L"using IFlexibleTaskbarPinnedList");
	}
	else if (build >= 17763)
	{
		m_pinnedList3 = (IPinnedList3*)flex;
		dbgprintf(L"using IPinnedlist3");
	}
}

CPinnedListWrapper::~CPinnedListWrapper()
{
	if (m_pinnedList25)
		m_pinnedList25->Release();
	if (m_flexList)
		m_flexList->Release();
	if (m_pinnedList3)
		m_pinnedList3->Release();
}

HRESULT __stdcall CPinnedListWrapper::QueryInterface(REFIID riid, void** ppvObject)
{
	if (m_pinnedList25)
		return m_pinnedList25->QueryInterface(riid, ppvObject);
	if (m_flexList)
		return m_flexList->QueryInterface(riid, ppvObject);
	if (m_pinnedList3)
		return m_pinnedList3->QueryInterface(riid, ppvObject);
}

ULONG __stdcall CPinnedListWrapper::AddRef(void)
{
	if (m_pinnedList25)
		return m_pinnedList25->AddRef();
	if (m_flexList)
		return m_flexList->AddRef();
	if (m_pinnedList3)
		return m_pinnedList3->AddRef();
}

ULONG __stdcall CPinnedListWrapper::Release(void)
{
	ULONG cref;
	if (m_pinnedList25)
		cref = m_pinnedList25->Release();
	if (m_flexList)
		cref = m_flexList->Release();
	if (m_pinnedList3)
		cref = m_pinnedList3->Release();
	if (cref == 0)
		free((void*)this);
	return cref;
}
//.text:00007FF6BEB24439 explorer.exe:$94439 #93A39
HRESULT __stdcall CPinnedListWrapper::EnumObjects(IEnumFullIDList** p1)
{
	if (m_pinnedList25)
		return m_pinnedList25->EnumObjects(p1);
	if (m_flexList)
		return m_flexList->EnumObjects(p1);
	if (m_pinnedList3)
		return m_pinnedList3->EnumObjects(p1);
}

HRESULT __stdcall CPinnedListWrapper::Modify(PCIDLIST_ABSOLUTE p1, PCIDLIST_ABSOLUTE p2)
{
	if (m_pinnedList25)
		return m_pinnedList25->Modify(p1, p2);
	if (m_flexList)
		return m_flexList->Modify(p1, p2);
	if (m_pinnedList3)
	{
		dbgprintf(L"Modify %x %x",p1,p2);

		return m_pinnedList3->Modify(p1, p2, (PLMC)18);
	}
}

HRESULT __stdcall CPinnedListWrapper::GetChangeCount(ULONG* p1)
{
	if (m_pinnedList25)
		return m_pinnedList25->GetChangeCount(p1);
	if (m_flexList)
		return m_flexList->GetChangeCount(p1);
	if (m_pinnedList3)
		return m_pinnedList3->GetChangeCount(p1);
}

HRESULT __stdcall CPinnedListWrapper::GetPinnableInfo(IDataObject* p1, int p2, IShellItem2** p3, IShellItem** p4, PWSTR* p5, INT* p6)
{
	if (m_pinnedList25)
		return m_pinnedList25->GetPinnableInfo(p1, p2, p3, p4, p5, p6);
	if (m_flexList)
		return m_flexList->GetPinnableInfo(p1, p2, p3, p4, p5, p6);
	if (m_pinnedList3)
		return m_pinnedList3->GetPinnableInfo(p1, p2, p3, p4, p5, p6);
}

HRESULT __stdcall CPinnedListWrapper::IsPinnable(IDataObject* p1, int p2)
{
	if (m_pinnedList25)
		return m_pinnedList25->IsPinnable(p1, p2);
	if (m_flexList)
		return m_flexList->IsPinnable(p1, p2);
	if (m_pinnedList3)
		return m_pinnedList3->IsPinnable(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::Resolve(HWND p1, ULONG p2, PCIDLIST_ABSOLUTE p3, PIDLIST_ABSOLUTE* p4)
{
	if (m_pinnedList25)
		return m_pinnedList25->Resolve(p1, p2, p3, p4);
	if (m_flexList)
		return m_flexList->Resolve(p1, p2, p3, p4);
	if (m_pinnedList3)
		return m_pinnedList3->Resolve(p1, p2, p3, p4);
}

HRESULT __stdcall CPinnedListWrapper::IsPinned(PCIDLIST_ABSOLUTE p1)
{
	if (m_pinnedList25)
		return m_pinnedList25->IsPinned(p1);
	if (m_flexList)
		return m_flexList->IsPinned(p1);
	if (m_pinnedList3)
		return m_pinnedList3->IsPinned(p1);
}

HRESULT __stdcall CPinnedListWrapper::GetPinnedItem(PCIDLIST_ABSOLUTE p1, PIDLIST_ABSOLUTE* p2)
{
	if (m_pinnedList25)
		return m_pinnedList25->GetPinnedItem(p1, p2);
	if (m_flexList)
		return m_flexList->GetPinnedItem(p1, p2);
	if (m_pinnedList3)
		return m_pinnedList3->GetPinnedItem(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::GetAppIDForPinnedItem(PCIDLIST_ABSOLUTE p1, PWSTR* p2)
{
	if (m_pinnedList25)
		return m_pinnedList25->GetAppIDForPinnedItem(p1, p2);
	if (m_flexList)
		return m_flexList->GetAppIDForPinnedItem(p1, p2);
	if (m_pinnedList3)
		return m_pinnedList3->GetAppIDForPinnedItem(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::ItemChangeNotify(PCIDLIST_ABSOLUTE p1, PCIDLIST_ABSOLUTE p2)
{
	if (m_pinnedList25)
		return m_pinnedList25->ItemChangeNotify(p1, p2);
	if (m_flexList)
		return m_flexList->ItemChangeNotify(p1, p2);
	if (m_pinnedList3)
		return m_pinnedList3->ItemChangeNotify(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::UpdateForRemovedItemsAsNecessary(VOID)
{
	if (m_pinnedList25)
		return m_pinnedList25->UpdateForRemovedItemsAsNecessary();
	if (m_flexList)
		return m_flexList->UpdateForRemovedItemsAsNecessary();
	if (m_pinnedList3)
		return m_pinnedList3->UpdateForRemovedItemsAsNecessary();
}
