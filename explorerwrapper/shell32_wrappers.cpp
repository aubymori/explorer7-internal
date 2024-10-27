#include "framework.h"
#include "autoplay.h"
#include "dbgprint.h"
#include "version.h"
#include "shell32_wrappers.h"
#include "augmentedshellfolder.h"
#include "MinHook.h"
#include "knownfolders.h"
#include "registry.h"

DWORD bEnableUWPAppsInStart = true;

typedef PVOID (WINAPI *ResolveDelayLoadedAPIAPI)(PVOID ParentModuleBase, PVOID DelayloadDescriptor, PVOID FailureDllHook, PVOID FailureSystemHook,PIMAGE_THUNK_DATA ThunkAddress,ULONG Flags);
static ResolveDelayLoadedAPIAPI ResolveDelayLoadedAPI;
static PVOID CoCreateInstanceBase;
static PVOID SHGetValueWSHCore;

//remove pintostart verb
LSTATUS WINAPI SHGetValueNEW(
  _In_         HKEY hkey,
  _In_opt_     LPCWSTR pszSubKey,
  _In_opt_     LPCWSTR pszValue,
  _Out_opt_    LPDWORD pdwType,
  _Out_opt_    LPVOID pvData,
  _Inout_opt_  LPDWORD pcbData
)
{	
	WCHAR buf[100];
	DWORD bufsize = sizeof(buf);
	if ( lstrcmp(pszValue,L"LegacyDisable") == 0 )
	if ( (RegQueryValueEx(hkey,L"MUIVerb",NULL,NULL,(LPBYTE)buf,&bufsize) == ERROR_SUCCESS) && (lstrcmpi(buf,L"@shell32.dll,-51201") == 0) ) return ERROR_SUCCESS;
	return SHGetValueW(hkey,pszSubKey,pszValue,pdwType,pvData,pcbData);
}

bool(__fastcall* IsSearchEnabled)();
extern "C" bool WINAPI IsSearchEnabledNEW()
{
	//dbgprintf(L"IsSearchEnabledNEW\n");
	return 1;
}

PVOID WINAPI ResolveDelayLoadedAPINEW(PVOID ParentModuleBase, PVOID DelayloadDescriptor, PVOID FailureDllHook, PVOID FailureSystemHook,PIMAGE_THUNK_DATA ThunkAddress,ULONG Flags)
{
	dbgprintf(L"ResolveDelayLoadedAPINEW\n");
	PVOID retfunc = ResolveDelayLoadedAPI(ParentModuleBase,DelayloadDescriptor,FailureDllHook,FailureSystemHook,ThunkAddress,Flags);
	if (retfunc == CoCreateInstanceBase)
	{
		retfunc = Shell32_CoCreateInstance;
		ThunkAddress->u1.Function = (DWORD_PTR)retfunc;
	}
	if (retfunc == SHGetValueWSHCore)
	{
		retfunc = SHGetValueNEW;
		ThunkAddress->u1.Function = (DWORD_PTR)retfunc;
	}
	return retfunc;
}

BOOL __stdcall ILIsEqualNEW(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
	dbgprintf(L"ILIsEqualNEW\n");
	IShellFolder* ppshf = 0;
	HRESULT v4 = SHGetDesktopFolder(&ppshf);
	if (v4 >= 0)
	{
		v4 = ppshf->CompareIDs(0x10000000i64, pidl1, pidl2);
		ppshf->Release();
	}
	return v4 == 0;
}

HRESULT __stdcall SHEvaluateSystemCommandTemplateNEW(PCWSTR pszCmdTemplate, PWSTR* ppszApplication, PWSTR* ppszCommandLine, PWSTR* ppszParameters)
{
	dbgprintf(L"SHEvaluateSystemCommandTemplateNEW\n");
	return S_OK;
	//return SHEvaluateSystemCommandTemplateWithOptions((unsigned __int16*)pszCmdTemplate, ppszParameters);
}

HRESULT(__stdcall* Shell32_DllGetClassObject)(REFCLSID rclsid, const IID* const riid, LPVOID* ppv);
HRESULT __stdcall Shell32_DllGetClassObject_Hook(REFCLSID rclsid, const IID* const riid, LPVOID* ppv)
{
	HRESULT result = Shell32_DllGetClassObject(rclsid, riid, ppv);
	if (result != S_OK && (rclsid == CLSID_ProgramsFolderAndFastItems || rclsid == CLSID_ProgramsFolder || rclsid == CLSID_StartMenuFolder))
	{
		*ppv = new CProgramsFolderClassFactory(rclsid);
		return S_OK;
	}

	return result;
}

void HookShell32()
{
	dbgprintf(L"1\n");
	ResolveDelayLoadedAPI = (ResolveDelayLoadedAPIAPI)GetProcAddress(GetModuleHandle(L"kernel32.dll"),"ResolveDelayLoadedAPI");
	ChangeImportedAddress(GetModuleHandle(L"shell32.dll"), "API-MS-WIN-CORE-DELAYLOAD-L1-1-1.DLL", ResolveDelayLoadedAPI, ResolveDelayLoadedAPINEW);
	//ResolveDelayLoadedAPI = (ResolveDelayLoadedAPIAPI)GetProcAddress(GetModuleHandle(L"api-ms-win-core-delayload-l1-1-1.dll"),"ResolveDelayLoadedAPI");
	dbgprintf(L"%i\n",(unsigned long long)ResolveDelayLoadedAPI);
	dbgprintf(L"2\n");
	CoCreateInstanceBase = GetProcAddress(GetModuleHandle(L"combase.dll"),"CoCreateInstance");
	Shell32_DllGetClassObject = (decltype(Shell32_DllGetClassObject))GetProcAddress(GetModuleHandle(L"shell32.dll"),"DllGetClassObject");
	MH_CreateHook(Shell32_DllGetClassObject, Shell32_DllGetClassObject_Hook,(LPVOID*)&Shell32_DllGetClassObject);
	dbgprintf(L"3\n");

	SHGetValueWSHCore = GetProcAddress(LoadLibrary(L"shcore.dll"),"SHGetValueW");

	dbgprintf(L"5\n");
	//auto ordinal902 = GetProcAddress(LoadLibrary(L"shell32.dll"),(LPSTR)902);
	//ChangeImportedAddress(LoadLibrary(L"shell32.dll"),"shlwapi.DLL", GetProcAddress(LoadLibrary(L"shlwapi.dll"), "SHAboutInfo"), SHAboutInfoWNEW);
	ChangeImportedAddress(GetModuleHandle(0),"shell32.DLL", GetProcAddress(LoadLibrary(L"shell32.DLL"), (LPSTR)902), IsSearchEnabledNEW);

	//ChangeImportedAddress(GetModuleHandle(0),"shell32.DLL", GetProcAddress(LoadLibrary(L"shell32.DLL"), (LPSTR)719), SHParseDarwinIDFromCacheWNew);

	//todo: evaluate if this is needed
	ChangeImportedAddress(GetModuleHandle(0),"shell32.DLL", GetProcAddress(LoadLibrary(L"shell32.DLL"), "ILIsEqual"), ILIsEqualNEW);

	uintptr_t Cunt = FindPattern((uintptr_t)LoadLibrary(L"shell32.dll"), "41 8B E9 49 8B F0 48 8B DA 48 8B F9 48 8D 0D ?? ?? ?? ?? E8");
	if (Cunt && g_osVersion.BuildNumber() >= 19045)
	{
		dbgprintf(L"Cunt %i", Cunt);
		Cunt += 19;
		uint8_t* bytes = (uint8_t*)(Cunt + 5 + *reinterpret_cast<int32_t*>(Cunt + 1));
		DWORD old;
		VirtualProtect(bytes, 3, PAGE_EXECUTE_READWRITE, &old);
		bytes[0] = 0xB0;
		bytes[1] = 0x00;
		bytes[2] = 0xC3;
		VirtualProtect(bytes, 3, old, 0);
	}
	
	dbgprintf(L"6\n");
	g_registry.QueryValue(L"EnableUWPAppsInStart",(LPBYTE)&bEnableUWPAppsInStart,sizeof(DWORD));
}



HRESULT BindToDesktop(LPCITEMIDLIST pidl, IShellFolder2** ppsfResult)
{
	HRESULT hr;
	IShellFolder* psfDesktop;
	IShellFolder2* psfDesktop2;

	*ppsfResult = NULL;

	hr = SHGetDesktopFolder(&psfDesktop);
	if (FAILED(hr))
		return hr;

	hr = psfDesktop->QueryInterface(IID_PPV_ARGS(&psfDesktop2));
	psfDesktop->Release();
	if (FAILED(hr))
		return hr;

	hr = psfDesktop2->BindToObject(pidl, NULL, IID_PPV_ARGS(ppsfResult));

	return hr;
}

STDAPI SHCacheTrackingFolder(LPCITEMIDLIST pidlRoot, int csidlTarget, IShellFolder2** ppsfCache)
{
	HRESULT hr = S_OK;

	if (!*ppsfCache)
	{
		PERSIST_FOLDER_TARGET_INFO pfti = { 0 };
		IShellFolder2* psf;
		LPITEMIDLIST pidl;

		// add FILE_ATTRIBUTE_SYSTEM to allow MUI stuff underneath this folder.
		// since its just for these tracking folders it isnt a perf hit to enable this.
		pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;
		pfti.csidl = csidlTarget | CSIDL_FLAG_PFTI_TRACKTARGET;

		if (IS_INTRESOURCE(pidlRoot))
		{
			hr = SHGetFolderLocation(NULL, PtrToInt(pidlRoot), NULL, 0, &pidl);
		}
		else
		{
			pidl = const_cast<LPITEMIDLIST>(pidlRoot);
		}

		if (SUCCEEDED(hr))
		{
			BindToDesktop(pidl, ppsfCache);
		}

		if (pidl != pidlRoot)
			ILFree(pidl);

	}
	return hr;
}
#define MAKEINTIDLIST(csidl)    (LPCITEMIDLIST)MAKEINTRESOURCE(csidl)

STDAPI SHGetIDListFromUnk(IUnknown* punk, LPITEMIDLIST* ppidl)
{
	*ppidl = NULL;

	HRESULT hr = E_NOINTERFACE;
	if (punk)
	{
		IPersistFolder2* ppf;
		IPersistIDList* pperid;
		if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARGS(&pperid))))
		{
			hr = pperid->GetIDList(ppidl);
			pperid->Release();
		}
		else if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARGS(&ppf))))
		{
			hr = ppf->GetCurFolder(ppidl);
			ppf->Release();
		}
	}
	return hr;
}

MIDL_INTERFACE("76347b91-9846-4ce7-9a57-69b910d16123")
ISetFolderEnumRestriction : IUnknown
{
	virtual HRESULT SetEnumRestriction(DWORD dwRequired, DWORD dwForbidden) = 0;
};

HRESULT GetMergedFolder(IShellFolder** ppsf, LPITEMIDLIST* ppidl,
	LPCMERGEDFOLDERINFO rgmfi, UINT cmfi)
{
	*ppidl = NULL;
	*ppsf = NULL;

	IShellFolder2* psf;
	IAugmentedShellFolder* pasf;
	HRESULT hr = CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&pasf));

	for (UINT imfi = 0; SUCCEEDED(hr) && imfi < cmfi; imfi++)
	{
		// If this is a common group and common groups are restricted, then
		// skip this item
		if ((rgmfi[imfi].uANSFlags & ASFF_COMMON) &&
			SHRestricted(REST_NOCOMMONGROUPS))
		{
			continue;
		}

		psf = NULL;    // in/out param below
		hr = SHCacheTrackingFolder(MAKEINTIDLIST(rgmfi[imfi].csidl), rgmfi[imfi].csidl, &psf);

		if (SUCCEEDED(hr))
		{
			// If this is a Start Menu folder, then apply the
			// "do not enumerate subfolders" restriction if the policy says so.
			// In which case, we cannot use the tracking folder cache.
			// (Perf note: We compare pointers directly.)
			if (rgmfi[imfi].pguidObj == &CLSID_StartMenu)
			{
				if (SHRestricted(REST_NOSTARTMENUSUBFOLDERS))
				{
					ISetFolderEnumRestriction* prest;
					if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARGS(&prest))))
					{
						prest->SetEnumRestriction(0, SHCONTF_FOLDERS); // disallow subfolders
						prest->Release();
					}
				}
			}
			//else
			//{
			//	// If this assert fires, then our perf optimization above failed.
			//	ASSERT(rgmfi[imfi].pguidObj == NULL ||
			//		!IsEqualGUID(*rgmfi[imfi].pguidObj, CLSID_StartMenu));
			//}


			hr = pasf->AddNameSpace((LPGUID)rgmfi[imfi].pguidObj, psf, NULL, rgmfi[imfi].uANSFlags, rgmfi[imfi].idk);
			if (SUCCEEDED(hr))
			{
				if (rgmfi[imfi].uANSFlags & ASFF_DEFNAMESPACE_DISPLAYNAME)
				{
					// If this assert fires, it means somebody marked two
					// folders as ASFF_DEFNAMESPACE_DISPLAYNAME, which is
					// illegal (you can have only one default)
					//ASSERT(*ppidl == NULL);
					hr = SHGetIDListFromUnk(psf, ppidl);    // copy out the pidl for this guy
				}
			}

			psf->Release();
		}
	}

	if (SUCCEEDED(hr))
		*ppsf = pasf;   // copy out the ref
	else
	{
		if (pasf)
			pasf->Release();
	}

	return hr;
}

const MERGEDFOLDERINFO c_rgmfiStartMenu[] = {
	{   CSIDL_STARTMENU | CSIDL_FLAG_CREATE,    ASFF_DEFNAMESPACE_ALL,  &CLSID_StartMenu },
	{   CSIDL_COMMON_STARTMENU,                 ASFF_COMMON,            &CLSID_StartMenu },
};

const MERGEDFOLDERINFO c_rgmfiProgramsFolder[] = {
	{   CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,     ASFF_DEFNAMESPACE_ALL,  NULL },
	{   CSIDL_COMMON_PROGRAMS,                  ASFF_COMMON,            NULL },
};

const MERGEDFOLDERINFO c_rgmfiProgramsFolderAndFastItems[] = {
	{   CSIDL_STARTMENU | CSIDL_FLAG_CREATE,    ASFF_DEFAULT | ASFF_MERGESAMEGUID,                 &CLSID_StartMenu},
	{   CSIDL_COMMON_STARTMENU,                 ASFF_COMMON | ASFF_MERGESAMEGUID,                 &CLSID_StartMenu},
	{   CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,     ASFF_DEFNAMESPACE_ALL | ASFF_MERGESAMEGUID | ASFF_SORTDOWN, NULL },
	{   CSIDL_COMMON_PROGRAMS,                  ASFF_COMMON | ASFF_MERGESAMEGUID | ASFF_SORTDOWN, NULL },
};

#define MAX_PROPERTY_SIZE       2048

//  simple inline base class.
class CBasePropertyBag : public IPropertyBag, IPropertyBag2
{
public:
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CBasePropertyBag, IPropertyBag),
			QITABENT(CBasePropertyBag, IPropertyBag2),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		if (InterlockedDecrement(&_cRef))
			return _cRef;

		delete this;
		return 0;
	}

	// IPropertyBag
	STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog) PURE;
	STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT* pVar) PURE;

	// IPropertyBag2 (note, does not derive from IPropertyBag)
	STDMETHODIMP Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError);
	STDMETHODIMP Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue);
	STDMETHODIMP CountProperties(ULONG* pcProperties);
	STDMETHODIMP GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties);
	STDMETHODIMP LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog);

protected:  //  methods
	CBasePropertyBag() {}       //  DONT DELETE ME
	CBasePropertyBag(DWORD grfMode) :
		_cRef(1), _grfMode(grfMode) {}

	virtual ~CBasePropertyBag() {}  //  DONT DELETE ME
	HRESULT _CanRead(void)
	{
		return STGM_WRITE != (_grfMode & (STGM_WRITE | STGM_READWRITE)) ? S_OK : E_ACCESSDENIED;
	}

	HRESULT _CanWrite(void)
	{
		return (_grfMode & (STGM_WRITE | STGM_READWRITE)) ? S_OK : E_ACCESSDENIED;
	}

protected:  //  members
	LONG _cRef;
	DWORD _grfMode;
};

STDMETHODIMP CBasePropertyBag::Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::CountProperties(ULONG* pcProperties)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog)
{
	return E_NOTIMPL;
}

typedef struct
{
	LPWSTR pszPropName;
	VARIANT variant;
} PBAGENTRY, * LPPBAGENTRY;

class CMemPropertyBag : public CBasePropertyBag
{
public:
	// IPropertyBag
	STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog);
	STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT* pVar);

protected:  // methods
	CMemPropertyBag(DWORD grfMode) : CBasePropertyBag(grfMode) {}
	~CMemPropertyBag();
	HRESULT _Find(LPCOLESTR pszPropName, PBAGENTRY** pppbe, BOOL fCreate);

	friend HRESULT SHCreatePropertyBagOnMemory(DWORD grfMode, REFIID riid, void** ppv);

protected:  // members
	HDSA _hdsaProperties;
};

INT _FreePropBagCB(LPVOID pData, LPVOID lParam)
{
	LPPBAGENTRY ppbe = (LPPBAGENTRY)pData;
	Str_SetPtrW(&ppbe->pszPropName, NULL);
	VariantClear(&ppbe->variant);
	return 1;
}

CMemPropertyBag::~CMemPropertyBag()
{
	if (_hdsaProperties)
		DSA_DestroyCallback(_hdsaProperties, _FreePropBagCB, NULL);
}

//
// manange the list of propeties in the property bag
//

HRESULT CMemPropertyBag::_Find(LPCOLESTR pszPropName, PBAGENTRY** pppbe, BOOL fCreate)
{
	int i;
	PBAGENTRY pbe = { 0 };

	*pppbe = NULL;

	// look up the property in the DSA
	// PERF: change to a DPA and sort accordingly for better perf (daviddv 110798)

	for (i = 0; _hdsaProperties && (i < DSA_GetItemCount(_hdsaProperties)); i++)
	{
		LPPBAGENTRY ppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);
		if (!StrCmpIW(pszPropName, ppbe->pszPropName))
		{
			*pppbe = ppbe;
			return S_OK;
		}
	}

	// no entry found, should we create one?

	if (!fCreate)
		return E_FAIL;

	// yes, so lets check to see if we have a DSA yet.

	if (!_hdsaProperties)
		_hdsaProperties = DSA_Create(sizeof(PBAGENTRY), 4);
	if (!_hdsaProperties)
		return E_OUTOFMEMORY;

	// we have the DSA so lets fill the record we want to put into it

	if (!Str_SetPtrW(&pbe.pszPropName, pszPropName))
		return E_OUTOFMEMORY;

	VariantInit(&pbe.variant);

	// append it to the DSA we are using

	i = DSA_AppendItem(_hdsaProperties, &pbe);
	if (-1 == i)
	{
		Str_SetPtrW(&pbe.pszPropName, NULL);
		return E_OUTOFMEMORY;
	}

	*pppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);

	return S_OK;
}


//
// IPropertyBag methods
//

STDAPI VariantChangeTypeForRead(VARIANT* pvar, VARTYPE vtDesired)
{
	HRESULT hr = S_OK;

	if ((pvar->vt != vtDesired) && (vtDesired != VT_EMPTY))
	{
		VARIANT varTmp;
		VARIANT varSrc;

		// cache a copy of [in]pvar in varSrc - we will free this later
		CopyMemory(&varSrc, pvar, sizeof(varTmp));
		VARIANT* pvarToCopy = &varSrc;

		// oleaut's VariantChangeType doesn't support
		// hex number string -> number conversion, which we want,
		// so convert those to another format they understand.
		//
		// if we're in one of these cases, varTmp will be initialized
		// and pvarToCopy will point to it instead
		//
		if (VT_BSTR == varSrc.vt)
		{
			switch (vtDesired)
			{
			case VT_I1:
			case VT_I2:
			case VT_I4:
			case VT_INT:
			{
				if (StrToIntExW(varSrc.bstrVal, STIF_SUPPORT_HEX, &varTmp.intVal))
				{
					varTmp.vt = VT_INT;
					pvarToCopy = &varTmp;
				}
				break;
			}

			case VT_UI1:
			case VT_UI2:
			case VT_UI4:
			case VT_UINT:
			{
				if (StrToIntExW(varSrc.bstrVal, STIF_SUPPORT_HEX, (int*)&varTmp.uintVal))
				{
					varTmp.vt = VT_UINT;
					pvarToCopy = &varTmp;
				}
				break;
			}
			}
		}

		// clear our [out] buffer, in case VariantChangeType fails
		VariantInit(pvar);

		hr = VariantChangeType(pvar, pvarToCopy, 0, vtDesired);

		// clear the cached [in] value
		VariantClear(&varSrc);
		// if initialized, varTmp is VT_UINT or VT_UINT, neither of which need VariantClear
	}

	return hr;
}

STDMETHODIMP CMemPropertyBag::Read(LPCOLESTR pszPropName, VARIANT* pv, IErrorLog* pErrorLog)
{
	VARTYPE vtDesired = pv->vt;

	VariantInit(pv);

	HRESULT hr = _CanRead();
	if (SUCCEEDED(hr))
	{
		hr = (pszPropName && pv) ? S_OK : E_INVALIDARG;

		if (SUCCEEDED(hr))
		{
			LPPBAGENTRY ppbe;
			hr = _Find(pszPropName, &ppbe, FALSE);
			if (SUCCEEDED(hr))
			{
				hr = VariantCopy(pv, &ppbe->variant);
				if (SUCCEEDED(hr))
					hr = VariantChangeTypeForRead(pv, vtDesired);
			}
		}
	}

	return hr;
}

STDMETHODIMP CMemPropertyBag::Write(LPCOLESTR pszPropName, VARIANT* pv)
{
	HRESULT hr = _CanWrite();
	if (SUCCEEDED(hr))
	{
		hr = (pszPropName && pv) ? S_OK : E_INVALIDARG;
		if (SUCCEEDED(hr))
		{
			LPPBAGENTRY ppbe;
			hr = _Find(pszPropName, &ppbe, TRUE);
			if (SUCCEEDED(hr))
			{
				hr = VariantCopy(&ppbe->variant, pv);
			}
		}
	}

	return hr;
}

HRESULT SHCreatePropertyBagOnMemory(DWORD grfMode, REFIID riid, void** ppv)
{
	CMemPropertyBag* ppb = new CMemPropertyBag(grfMode);
	if (ppb)
	{
		HRESULT hres = ppb->QueryInterface(riid, ppv);
		ppb->Release();

		return hres;
	}
	*ppv = NULL;
	return E_OUTOFMEMORY;
}

STDAPI SHPropertyBag_WriteBOOL(IPropertyBag* ppb, LPCWSTR pszPropName, BOOL fValue)
{
	//RIPMSG(NULL != ppb, "SHPropertyBag_WriteBOOL caller passed bad ppb");
	//RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteBOOL caller passed bad pszPropName");

	HRESULT hr;

	if (ppb && pszPropName)
	{
		VARIANT va;
		va.vt = VT_BOOL;
		va.boolVal = fValue ? VARIANT_TRUE : VARIANT_FALSE;
		hr = ppb->Write(pszPropName, &va);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

HRESULT CreateMergedFolderHelper(LPCMERGEDFOLDERINFO rgmfi, UINT cmfi, REFIID riid, void** ppv)
{
	IShellFolder* psf;
	LPITEMIDLIST pidl;
	HRESULT hr = GetMergedFolder(&psf, &pidl, rgmfi, cmfi);
	if (SUCCEEDED(hr))
	{
		hr = psf->QueryInterface(riid, ppv);

		if (SUCCEEDED(hr))
		{
			IPersistPropertyBag* pppb;
			if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARGS(&pppb))))
			{
				IPropertyBag* ppb;
				if (SUCCEEDED(SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARGS(&ppb))))
				{
					// these merged folders have to be told to use new changenotification
					SHPropertyBag_WriteBOOL(ppb, L"MergedFolder\\ShellView", TRUE);
					pppb->Load(ppb, NULL);
					ppb->Release();
				}
				pppb->Release();
			}
		}

		psf->Release();
		ILFree(pidl);
	}
	return hr;
}


HRESULT WINAPI Shell32_CoCreateInstance(
	__in   REFCLSID rclsid,
	__in   LPUNKNOWN pUnkOuter,
	__in   DWORD dwClsContext,
	__in   REFIID riid,
	__out  LPVOID* ppv
)
{
	HRESULT result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	if (rclsid == CLSID_ProgramsFolderAndFastItems && result != S_OK)
	{
		IShellFolder* ShellFolder;
		HRESULT result = CreateMergedFolderHelper(c_rgmfiProgramsFolderAndFastItems, _ARRAYSIZE(c_rgmfiProgramsFolderAndFastItems), IID_IShellFolder, (void**)&ShellFolder);
		result = ShellFolder->QueryInterface(riid, ppv);
		ShellFolder->Release();
		return result;
	}

	
	if (result == S_OK && rclsid == CLSID_AutoPlayUI)
	{
		*ppv = new CAutoPlayWrapper((IAutoPlayUI*)*ppv);
	}

	return result;
}

CProgramsFolderClassFactory::CProgramsFolderClassFactory(REFCLSID clsid)
{
	this->clsid = clsid;
}

CProgramsFolderClassFactory::~CProgramsFolderClassFactory()
{
}

HRESULT __stdcall CProgramsFolderClassFactory::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}
	if (riid == IID_IClassFactory)
	{
		*ppvObject = static_cast<IClassFactory*>(this);
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG __stdcall CProgramsFolderClassFactory::AddRef(void)
{
	return 1;
}

ULONG __stdcall CProgramsFolderClassFactory::Release(void)
{
	return 1;
}


HRESULT __stdcall CProgramsFolderClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
	if (pUnkOuter) return CLASS_E_NOAGGREGATION;
	if (this->clsid == CLSID_ProgramsFolderAndFastItems)
	{
		IShellFolder* ShellFolder;
		HRESULT result = CreateMergedFolderHelper(c_rgmfiProgramsFolderAndFastItems,_ARRAYSIZE(c_rgmfiProgramsFolderAndFastItems), IID_IShellFolder, (void**)&ShellFolder);
		result = ShellFolder->QueryInterface(riid, ppvObject);
		ShellFolder->Release();
		return result;
	}
	else if (this->clsid == CLSID_ProgramsFolder)
	{
		IShellFolder* ShellFolder;
		HRESULT result = CreateMergedFolderHelper(c_rgmfiProgramsFolder, _ARRAYSIZE(c_rgmfiProgramsFolder), IID_IShellFolder, (void**)&ShellFolder);
		result = ShellFolder->QueryInterface(riid, ppvObject);
		ShellFolder->Release();
		return result;
	}
	else if (this->clsid == CLSID_StartMenuFolder)
	{
		IShellFolder* ShellFolder;
		HRESULT result = CreateMergedFolderHelper(c_rgmfiStartMenu, _ARRAYSIZE(c_rgmfiStartMenu), IID_IShellFolder, (void**)&ShellFolder);
		result = ShellFolder->QueryInterface(riid, ppvObject);
		ShellFolder->Release();
		return result;
	}
	return E_NOTIMPL;
}

HRESULT __stdcall CProgramsFolderClassFactory::LockServer(BOOL fLock)
{
	return S_OK;
}

