#pragma once
#define INITGUID
#include "common.h"

#pragma region GUID definitions
DEFINE_GUID(_GUID_c2f03a33_21f5_47fa_b4bb_156362a2f239, 0xC2F03A33, 0x21F5, 0x47FA, 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39); // c2f03a33_21f5_47fa_b4bb_156362a2f239
DEFINE_GUID(CLSID_ImmersiveShellBuilder, 0xC71C41F1, 0xDDAD, 0x42DC, 0xA8, 0xFC, 0xF5, 0xBF, 0xC6, 0x1D, 0xF9, 0x57); // c71c41f1_ddad_42dc_a8fc_f5bfc61df957
DEFINE_GUID(SID_ImmersiveShellHookService, 0x4624BD39, 0x5FC3, 0x44A8, 0xA8, 0x09, 0x16, 0x3A, 0x83, 0x6E, 0x90, 0x31); // 4624bd39_5fc3_44a8-a809_163a836e9031
#pragma endregion

interface IImmersiveShellCreationBehavior;

MIDL_INTERFACE("ffffffff-ffff-ffff-ffff-ffffffffffff")
IImmersiveShellController : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Start() = 0;
	virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCreationBehavior(IImmersiveShellCreationBehavior*) = 0;
};

MIDL_INTERFACE("1c56b3e4-e6ea-4ced-8a74-73b72c6bd435")
IImmersiveShellBuilder : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE CreateImmersiveShellController(IImmersiveShellController**) = 0;
};

void InitializeImmersiveController();

interface IImmersiveShellHookNotification;

MIDL_INTERFACE("914d9b3a-5e53-4e14-bbba-46062acb35a4")
IImmersiveShellHookService : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Register(const UINT_PTR* const, UINT, IImmersiveShellHookNotification*, DWORD*) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD) = 0;
	virtual HRESULT STDMETHODCALLTYPE PostShellHookMessage(WPARAM, LPARAM) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTargetWindowForSerialization(HWND) = 0;
	virtual HRESULT STDMETHODCALLTYPE PostShellHookMessageWithSerialization(WPARAM, LPARAM) = 0;
	virtual HRESULT STDMETHODCALLTYPE UpdateWindowApplicationId(HWND, const WCHAR*) = 0;
	virtual HRESULT STDMETHODCALLTYPE HandleWindowReplacement(HWND, HWND) = 0;
	virtual int STDMETHODCALLTYPE IsExecutionOnSerializedThread() = 0;
};

