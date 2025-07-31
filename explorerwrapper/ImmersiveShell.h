#pragma once
#include "common.h"
#include "ImmersiveGUIDs.h"

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
	virtual HRESULT STDMETHODCALLTYPE InvokeShellHookMessage(WPARAM, LPARAM) = 0; // Added during Windows 11 development at some point - investigate further
};

