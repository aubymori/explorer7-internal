#include "pinnedlist.h"

CPinnedListWrapper::CPinnedListWrapper(IPinnedList3* flex)
{
	m_pinnedList = flex;
}

CPinnedListWrapper::~CPinnedListWrapper()
{
	m_pinnedList->Release();
}

HRESULT __stdcall CPinnedListWrapper::QueryInterface(REFIID riid, void** ppvObject)
{
	return m_pinnedList->QueryInterface(riid, ppvObject);
}

ULONG __stdcall CPinnedListWrapper::AddRef(void)
{
	return m_pinnedList->AddRef();
}

ULONG __stdcall CPinnedListWrapper::Release(void)
{
	ULONG cref = m_pinnedList->Release();
	if (cref == 0)
		free((void*)this);
	return cref;
}

HRESULT __stdcall CPinnedListWrapper::EnumObjects(IEnumFullIDList** p1)
{
	return m_pinnedList->EnumObjects(p1);
}

HRESULT __stdcall CPinnedListWrapper::Modify(PCIDLIST_ABSOLUTE p1, PCIDLIST_ABSOLUTE p2)
{
	return m_pinnedList->Modify(p1, p2, PLMC_EXPLORER);
}

HRESULT __stdcall CPinnedListWrapper::GetChangeCount(ULONG* p1)
{
	return m_pinnedList->GetChangeCount(p1);
}

HRESULT __stdcall CPinnedListWrapper::GetPinnableInfo(IDataObject* p1, int p2, IShellItem2** p3, IShellItem** p4, PWSTR* p5, INT* p6)
{
	return m_pinnedList->GetPinnableInfo(p1, p2, p3, p4, p5, p6);
}

HRESULT __stdcall CPinnedListWrapper::IsPinnable(IDataObject* p1, int p2)
{
	return m_pinnedList->IsPinnable(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::Resolve(HWND p1, ULONG p2, PCIDLIST_ABSOLUTE p3, PIDLIST_ABSOLUTE* p4)
{
	return m_pinnedList->Resolve(p1, p2, p3, p4);
}

HRESULT __stdcall CPinnedListWrapper::IsPinned(PCIDLIST_ABSOLUTE p1)
{
	return m_pinnedList->IsPinned(p1);
}

HRESULT __stdcall CPinnedListWrapper::GetPinnedItem(PCIDLIST_ABSOLUTE p1, PIDLIST_ABSOLUTE* p2)
{
	return m_pinnedList->GetPinnedItem(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::GetAppIDForPinnedItem(PCIDLIST_ABSOLUTE p1, PWSTR* p2)
{
	return m_pinnedList->GetAppIDForPinnedItem(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::ItemChangeNotify(PCIDLIST_ABSOLUTE p1, PCIDLIST_ABSOLUTE p2)
{
	return m_pinnedList->ItemChangeNotify(p1, p2);
}

HRESULT __stdcall CPinnedListWrapper::UpdateForRemovedItemsAsNecessary(VOID)
{
	return m_pinnedList->UpdateForRemovedItemsAsNecessary();
}
