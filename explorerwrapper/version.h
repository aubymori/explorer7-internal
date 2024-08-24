#pragma once
#include <Windows.h>

class COSVersion
{
private:
	RTL_OSVERSIONINFOEXW m_osvi;
	void _FillVersionInfo();

public:
	COSVersion();
	ULONG BuildNumber();
	ULONG MajorVersion();
	ULONG MinorVersion();
};

static COSVersion g_osVersion;