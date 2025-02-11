#pragma once
#define INITGUID
#include "common.h"
#include "OSVersion.h"

#pragma region GUID definitions
DEFINE_GUID(CLSID_ImmersiveShellBuilder,0xc71c41f1, 0xddad, 0x42dc, 0xa8,0xfc,0xf5,0xbf,0xc6,0x1d,0xf9,0x57); //c71c41f1_ddad_42dc_a8fc_f5bfc61df957
DEFINE_GUID(IID_ImmersiveShellBuilder,0x1c56b3e4, 0xe6ea, 0x4ced, 0x8a,0x74,0x73,0xb7,0x2c,0x6b,0xd4,0x35); //1c56b3e4_e6ea_4ced_8a74_73b72c6bd435

DEFINE_GUID(IID_ImmersiveBehavior,0x139275e0, 0xd644, 0x4214, 0xb4,0x5e,0xd9,0x27,0x8c,0x4a,0x85,0x01); //139275e0_d644_4214_b45e_d9278c4a8501

DEFINE_GUID(CLSID_NowPlayingSessionManager, 0xbcbb9860, 0xc012, 0x4ad7, 0xa9, 0x38, 0x6e, 0x33, 0x7a, 0xe6, 0xab, 0xa5);

///
/// Immersive additions 02/2025
///

DEFINE_GUID(SID_ImmersiveShellHookService, 0x4624BD39, 0x5FC3, 0x44A8, 0xA8, 0x09, 0x16, 0x3A, 0x83, 0x6E, 0x90, 0x31); // 4624bd39-5fc3-44a8-a809-163a836e9031
DEFINE_GUID(IID_IClassicWindowManager, 0x6c6cbabd, 0x6e36, 0x4f9d, 0xa3, 0x49, 0xc8, 0x85, 0x2d, 0x0e, 0x7e, 0xe5); // 6c6cbabd-6e36-4f9d-a349-c8852d0e7ee5
DEFINE_GUID(IID_IImmersiveApplicationNotificationService, 0x7860c098, 0xfb29, 0x49aa, 0xa5, 0x12, 0xad, 0xac, 0xc0, 0xff, 0xee, 0x84); // 7860c098-fb29-49aa-a512-adacc0ffee84
DEFINE_GUID(SID_AppCrusher, 0x3CF1532D, 0x66FB, 0x4F8C, 0x95, 0x92, 0xDC, 0x45, 0xC6, 0x3E, 0x65, 0x22); // 3cf1532d-66fb-4f8c-9592-dc45c63e6522
DEFINE_GUID(IID_IImmersiveApplicationManager, 0xbf63999f, 0x7411, 0x40da, 0x86, 0x1c, 0xdf, 0x72, 0xc0, 0xff, 0xee, 0x84); // bf63999f-7411-40da-861c-df72c0ffee84
DEFINE_GUID(SID_ImmersiveApplicationArrayService, 0xA3C23AB7, 0x6BE2, 0x4778, 0x8E, 0xB0, 0x1A, 0xDB, 0x79, 0x77, 0xF7, 0x6A); // a3c23ab7-6be2-4778-8eb0-1adb7977f76a
DEFINE_GUID(SID_PinManager, 0xA5C8D635, 0xB4ED, 0x452B, 0x81, 0x09, 0x95, 0x01, 0x78, 0x10, 0x96, 0xD1);
#pragma endregion

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveBehavior: public IUnknown
{
public:
	STDMETHOD(OnImmersiveThreadStart)(void) PURE;
	STDMETHOD(OnImmersiveThreadStop)(void) PURE;
	STDMETHOD(GetMaximumComponentCount)(unsigned int *count) PURE;
	STDMETHOD(CreateComponent)(unsigned int number, IUnknown** component) PURE;
	STDMETHOD(ShouldCreateComponent)(unsigned int number, int* allowed) PURE;
};

MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveShellController: public IUnknown
{
public:
	STDMETHOD(Start)(void) PURE;
	STDMETHOD(Stop)(void) PURE;
	STDMETHOD(SetCreationBehavior)(IImmersiveBehavior*) PURE;
};

/// IImmersiveShellCreator is legacy and reserved for Win8.1 use
MIDL_INTERFACE("00000000-0000-0000-0000-000000000000")
IImmersiveShellCreator: public IUnknown
{
public:
	STDMETHOD(CreateShell)(IImmersiveShellController** controller) PURE;
};
///

MIDL_INTERFACE("1c56b3e4-e6ea-4ced-8a74-73b72c6bd435")
IImmersiveShellBuilder : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE CreateImmersiveShellController(IImmersiveShellController**) = 0;
};

// Ittr: Needs to exist for legacy or 8.1 codepath
class CImmersiveBehaviorWrapper : public IImmersiveBehavior
{
public:
	CImmersiveBehaviorWrapper(IImmersiveBehavior* behavior);
	~CImmersiveBehaviorWrapper();

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IImmersiveBehavior
	STDMETHODIMP OnImmersiveThreadStart(void);
	STDMETHODIMP OnImmersiveThreadStop(void);
	STDMETHODIMP GetMaximumComponentCount(unsigned int* count);
	STDMETHODIMP CreateComponent(unsigned int number, IUnknown** component);
	STDMETHODIMP ShouldCreateComponent(unsigned int number, int* allowed);
private:
	IImmersiveBehavior* m_behavior;
	long m_cRef;
};

void CreateTwinUI();
void CreateTwinUI_UWP();
DWORD WINAPI TwinThread( LPVOID lpParameter );

BOOL RegisterShellHook(HWND hwnd, DWORD dwType);

interface IImmersiveShellHookNotification;

MIDL_INTERFACE("914d9b3a-5e53-4e14-bbba-46062acb35a4")
IImmersiveShellHookService : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Register(const UINT* const, UINT, IImmersiveShellHookNotification*, DWORD*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE PostShellHookMessage(WPARAM, LPARAM) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTargetWindowForSerialization(HWND) = 0;
    virtual HRESULT STDMETHODCALLTYPE PostShellHookMessageWithSerialization(WPARAM, LPARAM) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateWindowApplicationId(HWND, const WCHAR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE HandleWindowReplacement(HWND, HWND) = 0;
    virtual int STDMETHODCALLTYPE IsExecutionOnSerializedThread() = 0;
};

interface IImmersiveWindowMessageService : IUnknown
{
	STDMETHOD(Register)(UINT msg, void* pNotification, UINT* pdwCookie);
	STDMETHOD(Unregister)(UINT dwCookie);
	STDMETHOD(SendMessageW)(UINT nsg, WPARAM wParam, LPARAM lParam);
	STDMETHOD(PostMessageW)(UINT nsg, WPARAM wParam, LPARAM lParam);
	STDMETHOD(RequestHotkeys)(); //todo: args
	STDMETHOD(UnrequestHotkeys)(UINT dwCookie);
	STDMETHOD(RequestWTSSessionNotification)(void* pNotification, unsigned int* pdwCookie);
	STDMETHOD(UnrequestWTSSessionNotification)(UINT dwCookie);
	STDMETHOD(RequestPowerSettingNotification)(const GUID* pPowerSettingGuid, void* pNotification, UINT* pdwCookie);
	STDMETHOD(UnrequestPowerSettingNotification)(UINT pdwCookie);
	STDMETHOD(RequestPointerDeviceNotification)(void* pNotification, int notificationType, UINT* pdwCookie);
	STDMETHOD(UnrequestPointerDeviceNotification)(UINT dwCookie);
	STDMETHOD(RegisterDwmIconicThumbnailWindow)();
};

/// 
/// New definitions
///

enum IMMERSIVE_MONITOR_FILTER_FLAGS
{
	IMMERSIVE_MONITOR_FILTER_FLAGS_NONE = 0x0,
	IMMERSIVE_MONITOR_FILTER_FLAGS_DISABLE_TRAY = 0x1,
};

enum IMMERSIVE_MONITOR_MOVE_DIRECTION
{
	IMMD_PREVIOUS,
	IMMD_NEXT,
};

MIDL_INTERFACE("880b26f8-9197-43d0-8045-8702d0d72000")
IImmersiveMonitor : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetIdentity(DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE ConnectObject(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHandle(HMONITOR*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsConnected(BOOL*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsPrimary(BOOL*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsImmersiveDisplayDevice(BOOL*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDisplayRect(RECT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOrientation(DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWorkArea(RECT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsEqual(IImmersiveMonitor*, BOOL*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsImmersiveCapable(BOOL*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEffectiveDpi(UINT*, UINT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFilterFlags(IMMERSIVE_MONITOR_FILTER_FLAGS*) = 0;
};

typedef struct _CLASSIC_WINDOWS
{
} CLASSIC_WINDOWS;

MIDL_INTERFACE("6c6cbabd-6e36-4f9d-a349-c8852d0e7ee5")
IClassicWindowManager : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetLastActiveDesktopWindow(IImmersiveMonitor*, HWND*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWindowList(CLASSIC_WINDOWS**) = 0;
	virtual HRESULT STDMETHODCALLTYPE SwitchToThisApp(const WCHAR*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateLastActivatedTime(HWND) = 0;
	virtual void STDMETHODCALLTYPE AttachApplicationId(HWND, WCHAR*) = 0;
	virtual void STDMETHODCALLTYPE HandleWindowReplaced(HWND, HWND) = 0;
};

interface IImmersiveApplication;

MIDL_INTERFACE("c6636ec2-eba1-4e6d-a995-8fa14b8b2891")
IImmersiveApplicationWindow : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetBandId(DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetNativeWindow(HWND*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetApplicationId(WCHAR**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetProcessId(DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetThreadId(DWORD*) = 0;
};

enum IMMERSIVE_APPLICATION_GET_WINDOWS_FILTER
{
	IAGWF_ANY = 0,
	IAGWF_STRONGLY_NAMED = 1,
	IAGWF_PREFER_PENDING_PRESENTED = 2,
	IAGWF_ONLY_PENDING_PRESENTED = 3,
	IAGWF_PRESENTATION = 4,
	IAGWF_FRAME = 5
};

enum IMMAPPPROPERTYSTOREFLAGS
{
	IAGPS_DEFAULT = 0x0
};

DEFINE_ENUM_FLAG_OPERATORS(IMMAPPPROPERTYSTOREFLAGS);

typedef struct tagIMMAPPTIMESTAMPS
{
	FILETIME ftCreation;
	FILETIME ftClosed;
	FILETIME ftActivation;
	FILETIME ftInactive;
	FILETIME ftVisible;
	FILETIME ftHidden;
} IMMAPPTIMESTAMPS;

enum IMMERSIVE_APPLICATION_QUERY_SERVICE_OPTION
{
	IAQSO_DEFAULT = 0,
	IAQSO_FIRST_PACKAGED = 1,
	IAQSO_ANY_PACKAGED = 2
};

enum SPLASHSCREEN_ORIENTATION_PREFERENCE
{
	SSOP_NONE = 0,
	SSOP_LANDSCAPE = 1,
	SSOP_PORTRAIT = 2,
	SSOP_LANDSCAPE_FLIPPED = 4,
	SSOP_PORTRAIT_FLIPPED = 8
};

enum NOTIFY_IMMERSIVE_APPLICATION_WINDOWS_OPTION
{
	NIAWO_ALL = 0,
	NIAWO_SKIP_SYSTEM_WINDOWS = 1,
	NIAWO_CURRENT_WINDOW_ONLY = 2,
	NIAWO_CURRENT_WINDOW_ONLY_IFF_APP = 3
};

enum NOTIFY_IMMERSIVE_APPLICATION_WINDOWS_DELIVERY_TYPE
{
	NIAWDT_POST = 0,
	NIAWDT_SENDNOTIFY = 1
};

enum IMMAPP_SETTHUMBNAIL_PREVIEW_STATE
{
	IMMSPS_VISIBLE = 0,
	IMMSPS_HIDDEN = 1
};

enum USER_INTERACTION_MODE
{
	UIM_MOUSE = 0,
	UIM_TOUCH = 1
};

enum VIEW_PRESENTATION_MODE
{
	VPM_DESKTOP = 0,
	VPM_HOLOGRAPHIC = 1
};

enum APPLICATION_VIEW_MODE
{
	AVM_DEFAULT = 0,
	AVM_COMPACT_OVERLAY = 1,
	AVM_SPANNING = 2
};

enum APPLICATION_VIEW_MODE_FLAGS
{
	AVMF_DEFAULT = 0x1,
	AVMF_COMPACT_OVERLAY = 0x2,
	AVMF_SPANNING = 0x4
};

DEFINE_ENUM_FLAG_OPERATORS(APPLICATION_VIEW_MODE_FLAGS);

enum WindowTransparencyMode
{
	WTM_TransparentWhenActive = 0,
	WTM_AlwaysOpaque = 1,
	WTM_AlwaysTransparent = 2
};

struct APPLICATION_VIEW_DATA
{
	APPLICATION_VIEW_STATE viewState;
	APPLICATION_VIEW_ORIENTATION viewOrientation;
	ADJACENT_DISPLAY_EDGES displayEdges;
	BOOL fIsOnLockScreen;
	BOOL fIsFullScreenMode;
	USER_INTERACTION_MODE userInteractionMode;
	VIEW_PRESENTATION_MODE presentationMode;
	APPLICATION_VIEW_MODE viewMode;
	APPLICATION_VIEW_MODE_FLAGS allowedViewModes;
	WindowTransparencyMode windowTransparencyMode;
	BOOL canOpenInNewTab;
};

struct IMMAPP_APPLICATION_VIEW_DATA
{
	APPLICATION_VIEW_DATA current;
	APPLICATION_VIEW_DATA deferred;
};

typedef enum __MIDL___MIDL_itf_shpriv_core_0000_0325_0002
{
	MCF_FORCE = 0,
	MCF_IF_NOT_VISIBLE = 1
} MONITOR_CHANGE_FLAGS;

typedef enum __MIDL___MIDL_itf_shpriv_core_0000_0325_0001
{
	GVS_NORMAL = 0,
	GVS_USE_SPLASHSCREEN_VISUAL = 1,
	GVS_USE_SPLASHSCREEN_VISUAL_ONCE = 2
} GHOST_VISUAL_STYLE;

typedef enum __MIDL___MIDL_itf_shpriv_core_0000_0325_0003
{
	IABF_NONE = 0x0,
	IABF_INVALID_AUTOGLOM_DESTINATION = 0x1,
	IABF_AVOID_VIEW_FOR_SWITCH = 0x2,
	IABF_FORCE_TERMINATE_ON_CLOSE = 0x80000000
} IMM_APP_BEHAVIOR_FLAGS;

DEFINE_ENUM_FLAG_OPERATORS(IMM_APP_BEHAVIOR_FLAGS);

typedef enum __MIDL___MIDL_itf_shpriv_core_0000_0325_0004
{
	GSF_LOCKSCREENACTIVATION = 0x1,
	GSF_ACTIVATION = 0x2
} GHOST_STATUS_FLAG;

DEFINE_ENUM_FLAG_OPERATORS(GHOST_STATUS_FLAG);

typedef enum __MIDL___MIDL_itf_shpriv_core_0000_0325_0005
{
	IAQ_WIN8_WINDOWING_BEHAVIOR = 0,
	IAQ_REQUIRES_1366_PORTRAIT_MIN_HEIGHT = 1,
	IAQ_SHOW_ACTIONS_MENU = 2,
	IAQ_USE_WIN8X_COMPATIBILITY_SCALING = 3,
	IAQ_FULLSCREEN_8X_LEGACY_APP = 4,
	IAQ_WIN81_WINDOWING_BEHAVIOR = 5,
	IAQ_DONT_CACHE_TITLE_BAR_SETTINGS = 6,
	IAQ_USE_PREFERRED_STANDALONE_SIZE = 7
} IMMERSIVE_APPLICATION_QUIRKS;

interface IAsyncCallback;

MIDL_INTERFACE("ea8a389b-437d-4791-aa14-5dca004bc92a")
IAsyncCallback : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Invoke(HRESULT hr) = 0;
};

MIDL_INTERFACE("7702e77c-66f6-479b-af2b-e316cff5f814")
IAsyncCallbackDispatcher : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Dispatch(IAsyncCallback * callback) = 0;
};

MIDL_INTERFACE("8b14e88b-5663-4caf-b196-c31479262831")
IImmersiveApplication : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetWindows(IMMERSIVE_APPLICATION_GET_WINDOWS_FILTER, REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetApplicationId(WCHAR**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetUniqueId(WCHAR**) = 0;
	virtual HRESULT STDMETHODCALLTYPE OpenPropertyStore(IMMAPPPROPERTYSTOREFLAGS, REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsRunning(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsVisible(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsForeground(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTimestamps(tagIMMAPPTIMESTAMPS*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsEqualByAppId(const WCHAR*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsEqualByHwnd(HWND*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsEqualByApp(IImmersiveApplication*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsViewForSameApp(IImmersiveApplication*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPackageId(int, WCHAR**) = 0;
	virtual HRESULT STDMETHODCALLTYPE BelongsToPackage(const WCHAR*, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryService(IMMERSIVE_APPLICATION_QUERY_SERVICE_OPTION, DWORD, REFGUID, REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsServiceAvailable(IMMERSIVE_APPLICATION_QUERY_SERVICE_OPTION, REFGUID, int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsApplicationWindowStronglyNamed(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE ContainsStronglyNamedWindow(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsInteractive(int*) = 0;
	virtual SPLASHSCREEN_ORIENTATION_PREFERENCE STDMETHODCALLTYPE GetManifestedOrientationPreference() = 0;
	virtual HRESULT STDMETHODCALLTYPE NotifyApplicationWindows(UINT, WPARAM, LPARAM, NOTIFY_IMMERSIVE_APPLICATION_WINDOWS_OPTION, NOTIFY_IMMERSIVE_APPLICATION_WINDOWS_DELIVERY_TYPE) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDestinationInformation(IImmersiveApplicationWindow**, tagRECT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRect(tagRECT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetThumbnailPreviewState(IMMAPP_SETTHUMBNAIL_PREVIEW_STATE) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewData(IMMAPP_APPLICATION_VIEW_DATA*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMonitor(IImmersiveMonitor*, MONITOR_CHANGE_FLAGS) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMonitor(IImmersiveMonitor**) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGhostVisualStyle(GHOST_VISUAL_STYLE) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTitle(WCHAR**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBehaviorFlags(IMM_APP_BEHAVIOR_FLAGS*) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddBehaviorFlags(IMM_APP_BEHAVIOR_FLAGS) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveBehaviorFlags(IMM_APP_BEHAVIOR_FLAGS) = 0;
	virtual HRESULT STDMETHODCALLTYPE IncrementGhostAnimationWaitCount(UINT) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddGhostStatusFlag(GHOST_STATUS_FLAG) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveGhostStatusFlag(GHOST_STATUS_FLAG) = 0;
	virtual HRESULT STDMETHODCALLTYPE InvokeCharms() = 0;
	virtual HRESULT STDMETHODCALLTYPE OnMinSizePreferencesUpdated(HWND*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsSplashScreenPresented(int*) = 0;
	virtual int STDMETHODCALLTYPE IsQuirkEnabled(IMMERSIVE_APPLICATION_QUIRKS) = 0;
	virtual HRESULT STDMETHODCALLTYPE TryInvokeBack(IAsyncCallback*) = 0;
	virtual HRESULT STDMETHODCALLTYPE RequestCloseAsync(REFGUID, void**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCanHandleCloseRequest(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPositionerMonitor(IImmersiveMonitor*) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsTitleBarDrawnByApp(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDisplayName(WCHAR**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetIsOccluded(int*) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetIsOccluded(int) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetWindowingEnvironmentConfig(IUnknown*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPersistingStateName(WCHAR**) = 0;
};

enum IMM_APP_CHANGED
{
	IAC_UNKNOWN = 0,
	IAC_STARTED = 1,
	IAC_FOREGROUND = 2,
	IAC_BACKGROUND = 3,
	IAC_SHOWN = 4,
	IAC_HIDDEN = 5,
	IAC_CLOSED = 6,
	IAC_WINDOWPRESENTATIONDECLINED = 7,
	IAC_PRESENTEDWINDOWCHANGED = 8,
	IAC_MOBODYENTERED = 9,
	IAC_MOBODYEXIT = 10,
	IAC_HASPACKAGEID = 11,
	IAC_FORGOTTEN = 12,
	IAC_INTERACTIVE = 13,
	IAC_NON_INTERACTIVE = 14,
	IAC_PENDINGPRESENTEDWINDOWCANCELED = 15,
	IAC_FORGOTTEN_FOR_TERMINATION = 16,
	IAC_MONITORCHANGED = 17,
};

enum IMM_APP_SERVICE_NOTIFY_FLAGS
{
	IASNF_SERVICE_UNREGISTERED = 0x0,
	IASNF_SERVICE_REGISTERED = 0x1,
	IASNF_SERVICE_CHANGE_ON_PRESNTED_WINDOW = 0x2,
	IASNF_SERVICE_IS_WINDOW_SERVICE = 0x4,
};

DEFINE_ENUM_FLAG_OPERATORS(IMM_APP_SERVICE_NOTIFY_FLAGS);

MIDL_INTERFACE("e23730be-13f1-4f0b-88ab-76d5e3beb5b7")
IImmersiveApplicationNotification : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE ApplicationChanged(IImmersiveApplication*, IMM_APP_CHANGED, HWND) = 0;
// below is not in nickel so need to adjust for that
	virtual HRESULT STDMETHODCALLTYPE ServiceAvailabilityChanged(IImmersiveApplication*, REFGUID, IMM_APP_SERVICE_NOTIFY_FLAGS) = 0;
};

MIDL_INTERFACE("7860c098-fb29-49aa-a512-adacc0ffee84")
IImmersiveApplicationNotificationService : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Register(IImmersiveApplicationNotification*, DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD) = 0;
};

enum APPCRUSHER_INPUT
{
	ACI_TOUCH = 0,
	ACI_MOUSE = 1,
	ACI_KEYBOARD = 2,
	ACI_CONTEXTMENU = 3,
	ACI_TASKBAR = 4,
	ACI_TITLEBAR = 5,
	ACI_CAPTIONCONTROLS = 6,
	ACI_MULTITASKINGVIEW = 7,
	ACI_API = 8,
	ACI_HANGDETECTION = 9,
	ACI_INVALID = 10,
};

enum APPCRUSHER_SOURCE
{
	ACS_BACKSTACK = 0,
	ACS_VISIBLE = 1,
	ACS_LIST = 2,
	ACS_APPTIP = 3,
	ACS_PLACEMODE = 4,
	ACS_TASKBAR = 5,
	ACS_TITLEBAR = 6,
	ACS_CAPTIONCONTROLS = 7,
	ACS_MULTITASKINGVIEW = 8,
	ACS_API = 9,
	ACS_HANGDETECTION = 10,
	ACS_INVALID = 11,
};

enum APPCRUSHER_CLOSE_OPTIONS
{
	ACCO_NONE = 0x0,
	ACCO_NO_CLOSE_ANIMATION = 0x1,
	ACCO_NO_FOREGROUND_RIGHT_TRANSFER = 0x2,
	ACCO_FORCE = 0x4,
	ACCO_NO_CONFIRMAPPCLOSE_NOTIFY = 0x8,
	ACCO_NO_FALLBACK_FOREGROUND = 0x10,
	ACCO_SYSTEM_ACTION = 0x20,
};

DEFINE_ENUM_FLAG_OPERATORS(APPCRUSHER_CLOSE_OPTIONS);

enum APPCRUSHER_MINIMIZE_FLAGS
{
	ACMF_NONE = 0x0,
	ACMF_USE_DESKTOP_ANIMATIONS = 0x1,
};

DEFINE_ENUM_FLAG_OPERATORS(APPCRUSHER_MINIMIZE_FLAGS);

MIDL_INTERFACE("1bf3dc4c-07a0-4f3f-87f8-5a625cd2e7ad")
IImmersiveAppCrusherNotification : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE AppClosing(IImmersiveApplication*) = 0;
};

MIDL_INTERFACE("103231ae-04cb-4e5e-b63d-e3ce47cd0f0a")
IImmersiveAppCrusher : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE CloseAllApps() = 0;
	virtual HRESULT STDMETHODCALLTYPE CloseApp(IImmersiveApplication*, APPCRUSHER_INPUT, APPCRUSHER_SOURCE, APPCRUSHER_CLOSE_OPTIONS) = 0;
	virtual HRESULT STDMETHODCALLTYPE CloseAppWithTelemetryCookie(IImmersiveApplication*, APPCRUSHER_INPUT, APPCRUSHER_SOURCE, APPCRUSHER_CLOSE_OPTIONS, DWORD) = 0;
	virtual HRESULT STDMETHODCALLTYPE CloseApps(IObjectArray*, APPCRUSHER_INPUT, APPCRUSHER_SOURCE, APPCRUSHER_CLOSE_OPTIONS) = 0;
	virtual HRESULT STDMETHODCALLTYPE MinimizeApp(IImmersiveApplication*, APPCRUSHER_MINIMIZE_FLAGS) = 0;
	virtual HRESULT STDMETHODCALLTYPE Register(IImmersiveAppCrusherNotification*, DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTaskCompletionFormatString(size_t, IUnknown*) = 0; //HSTRING rather than IUnknown
};

struct APPLICATION_VIEW_DATA_UPDATE;

enum SET_IMM_APP_POS {};
enum IAM_ACTIVATE_APPLICATION_OPTION {};
enum SET_APPLICATION_WINDOW_OPTION {};

enum IAM_DESKTOP_SWITCH_OPTION
{
	IDSO_NONE = 0x0,
	IDSO_ACTIVATE_DESKTOP_ON_INVOKE_MONITOR = 0x1,
	IDSO_CLOAK_IMMERSIVE_BACKGROUND_ON_ALL_MONITORS = 0x2,
	IDSO_NO_FOREGROUND_MOVE = 0x4,
	IDSO_FULLSCREEN_SWITCH_ON_INVOKE_MONITOR = 0x8,
	IDSO_CLOAK_FULLSCREEN_IMMERSIVE_BACKROUNDS = 0x10,
	IDSO_ALLOW_SWITCH_FROM_ANY_PROCESS = 0x20,
};

DEFINE_ENUM_FLAG_OPERATORS(IAM_DESKTOP_SWITCH_OPTION);

interface IAsyncCallback;

MIDL_INTERFACE("bf63999f-7411-40da-861c-df72c0ffee84")
IImmersiveApplicationManager : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SwitchToShellWindow();
	virtual HRESULT STDMETHODCALLTYPE GetForegroundApplication(IImmersiveApplication**);
	virtual HRESULT STDMETHODCALLTYPE ForgetApplication(const WCHAR*);
	virtual HRESULT STDMETHODCALLTYPE ForgetApplicationByHostId(const WCHAR*, unsigned __int64);
	virtual HRESULT STDMETHODCALLTYPE SetApplicationPos(IImmersiveApplication*, const APPLICATION_VIEW_DATA_UPDATE*, SET_IMM_APP_POS, void**);
	virtual HRESULT STDMETHODCALLTYPE ActivateApplication(IImmersiveApplication*, IAM_ACTIVATE_APPLICATION_OPTION);
	virtual HRESULT STDMETHODCALLTYPE ForgetApplicationsInPackage(const WCHAR*);
	virtual HRESULT STDMETHODCALLTYPE SetApplicationWindow(HWND, SET_APPLICATION_WINDOW_OPTION);
	virtual HRESULT STDMETHODCALLTYPE RefreshMonitorMappings();
	virtual HRESULT STDMETHODCALLTYPE SwitchToDesktop(const POINT*, IAM_DESKTOP_SWITCH_OPTION);
	virtual HRESULT STDMETHODCALLTYPE SwitchToDesktopOnMonitor(IImmersiveMonitor*, IAM_DESKTOP_SWITCH_OPTION);
	virtual void STDMETHODCALLTYPE HandleNewApplicationProcess(const WCHAR*, void*);
	virtual int STDMETHODCALLTYPE RemoveApplicationWithPendingTermination(IImmersiveApplication*);
	virtual HRESULT STDMETHODCALLTYPE AddDelayedOperation(IAsyncCallback*);
	virtual HRESULT STDMETHODCALLTYPE ClearCachedForegroundWindow();
};

interface IImmersiveApplicationArray;

MIDL_INTERFACE("a3c23ab7-6be2-4778-8eb0-1adb7977f76a")
IImmersiveApplicationArrayService : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetImmersiveApplicationArray(IImmersiveApplicationArray**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetApplication(const WCHAR*, IImmersiveApplication**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAllApplicationsByAppID(const WCHAR*, IObjectArray**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetApplicationByWindow(HWND, IImmersiveApplication**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRunningApplicationsFromPackageId(const WCHAR*, IImmersiveApplicationArray**) = 0;
};

enum PERSISTED_APPLICATION_DATA_FLAGS
{
	PADF_NONE = 0x0,
	PADF_INCLUDE_IN_TASKBAR = 0x1,
};

DEFINE_ENUM_FLAG_OPERATORS(PERSISTED_APPLICATION_DATA_FLAGS);

MIDL_INTERFACE("b70e7ce8-5eb0-4e5a-af6c-05e57f1e87a3")
IApplicationDataPersistence : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SetPerSessionApplicationFlags(const WCHAR*, PERSISTED_APPLICATION_DATA_FLAGS) = 0;
	virtual HRESULT STDMETHODCALLTYPE ClearPerSessionApplicationFlags(const WCHAR*, PERSISTED_APPLICATION_DATA_FLAGS) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPerSessionApplicationFlags(const WCHAR*, PERSISTED_APPLICATION_DATA_FLAGS*) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAllPerSessionApplicationData(IObjectArray**) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveAllPackageData(const WCHAR*) = 0;
};

MIDL_INTERFACE("372e1d3b-38d3-42e4-a15b-8ab2b178f513")
IApplicationView : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE _6() = 0;
	virtual HRESULT STDMETHODCALLTYPE _7() = 0;
	virtual HRESULT STDMETHODCALLTYPE _8() = 0;
	virtual HRESULT STDMETHODCALLTYPE _9() = 0;
	virtual HRESULT STDMETHODCALLTYPE _10() = 0;
	virtual HRESULT STDMETHODCALLTYPE _11() = 0;
	virtual HRESULT STDMETHODCALLTYPE _12() = 0;
	virtual HRESULT STDMETHODCALLTYPE _13() = 0;
	virtual HRESULT STDMETHODCALLTYPE _14() = 0;
	virtual HRESULT STDMETHODCALLTYPE _15() = 0;
	virtual HRESULT STDMETHODCALLTYPE _16() = 0;
	virtual HRESULT STDMETHODCALLTYPE _17() = 0;
	virtual HRESULT STDMETHODCALLTYPE _18() = 0;
	virtual HRESULT STDMETHODCALLTYPE _19() = 0;
	virtual HRESULT STDMETHODCALLTYPE _20() = 0;
	virtual HRESULT STDMETHODCALLTYPE _21() = 0;
	virtual HRESULT STDMETHODCALLTYPE _22() = 0;
	virtual HRESULT STDMETHODCALLTYPE _23() = 0;
	virtual HRESULT STDMETHODCALLTYPE _24() = 0;
	virtual HRESULT STDMETHODCALLTYPE _25() = 0;
	virtual HRESULT STDMETHODCALLTYPE _26() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetShowInSwitchers(BOOL*) = 0;
};

interface IApplicationViewChangeListener;

MIDL_INTERFACE("1841c6d7-4f9d-42c0-af41-8747538f10e5")
IApplicationViewCollection : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetViews(IObjectArray**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewsByZOrder(IObjectArray**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewsByAppUserModelId(const WCHAR*, IObjectArray**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewForHwnd(HWND, IApplicationView**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewForApplication(IImmersiveApplication*, IApplicationView**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewForAppUserModelId(const WCHAR*, IApplicationView**) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetViewInFocus(IApplicationView**) = 0;
	virtual HRESULT STDMETHODCALLTYPE TryGetLastActiveVisibleView(IApplicationView**) = 0;
	virtual HRESULT STDMETHODCALLTYPE RefreshCollection() = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterForApplicationViewChanges(IApplicationViewChangeListener*, UINT*) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterForApplicationViewChanges(UINT) = 0;
};

enum PINNEDLISTMODIFYCALLER
{
	PMC_APPRESOLVERMIGRATION = 0,
	PMC_APPRESOLVERUNINSTALL = 1,
	PMC_APPRESOLVERUNPINUNIQUEID = 2,
	PMC_CONTENTDELIVERYMANAGERBROKER = 3,
	PMC_CONTEXTMENU = 4,
	PMC_DEFAULTMFUCHANGE = 5,
	PMC_DEFAULTMFUPIN = 6,
	PMC_DEFAULTMFUPINAUX = 7,
	PMC_DEFAULTMFUTRYPIN = 8,
	PMC_DEFAULTMFUUPGRADE = 9,
	PMC_IEXPLORERCOMMAND = 10,
	PMC_JUMPVIEWBROKER = 11,
	PMC_PINNEDLISTLAYOUT = 12,
	PMC_PINNEDLISTNONEXIST = 13,
	PMC_PINNEDLISTREORDERLAYOUT = 14,
	PMC_PINNEDLISTUNRESOLVE = 15,
	PMC_RETAILDEMO = 16,
	PMC_SHELLLINK = 17,
	PMC_STARTMENU = 18,
	PMC_STARTMNU = 19,
	PMC_TASKBANDBADSHORTCUT = 20,
	PMC_TASKBANDBROKENPIN = 21,
	PMC_TASKBANDDEDUPPIN = 22,
	PMC_TASKBANDINSERT = 23,
	PMC_TASKBANDMODIFY = 24,
	PMC_TASKBANDPIN = 25,
	PMC_TASKBANDPINGROUP = 26,
	PMC_TASKBANDREORDER = 27,
	PMC_TASKBARPINNABLESURFACEBROKER = 28,
	PMC_TASKBARPINNABLESURFACEBROKERMIGRATION = 29,
	PMC_TASKBARPINNINGBROKERFACTORY = 30,
	PMC_TESTCODE = 31,
	PMC_UNIFIEDTILEMODELBROKER = 32,
	PMC_TRIMOOBEPINS = 33,
	PMC_MICROSOFTEDGE = 34,
	PMC_UPDATESHORTCUT = 35,
	PMC_CLOUDDEFAULTLAYOUT = 36,
	PMC_BROWSERDECLUTTER = 37
};

enum TaskbarLayoutType
{
	TLT_0,
	TLT_1,
	TLT_2,
};

MIDL_INTERFACE("d75f625f-6df9-4874-970d-17cbf846f00d")
IPinManagerInterop : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE PinItemToTaskbarShim(PCUIDLIST_ABSOLUTE, PINNEDLISTMODIFYCALLER) = 0;
	virtual HRESULT STDMETHODCALLTYPE PinItemFromTrustedCaller(PCUIDLIST_ABSOLUTE, PINNEDLISTMODIFYCALLER) = 0;
	virtual HRESULT STDMETHODCALLTYPE ApplyPrependDefaultTaskbarLayout() = 0;
	virtual HRESULT STDMETHODCALLTYPE ApplyAppendDefaultTaskbarLayout() = 0; // @Warning: This appeared somewhere after the time of 22621.1992's release
	virtual HRESULT STDMETHODCALLTYPE ApplyInPlaceTaskbarLayout(TaskbarLayoutType) = 0;
	virtual HRESULT STDMETHODCALLTYPE ApplyReorderTaskbarLayout(TaskbarLayoutType, int) = 0;
};

MIDL_INTERFACE("87d9e034-56d0-4f8c-be59-997b01754710")
IPinManagerInterop2 : IPinManagerInterop
{
	virtual HRESULT STDMETHODCALLTYPE UnpinTaskbarItem(PCUIDLIST_ABSOLUTE, PINNEDLISTMODIFYCALLER) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdatePinnedTaskbarItem(PCUIDLIST_ABSOLUTE, PCUIDLIST_ABSOLUTE, PINNEDLISTMODIFYCALLER) = 0;
};