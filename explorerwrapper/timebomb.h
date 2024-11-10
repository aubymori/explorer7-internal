#pragma once
#include <windows.h>

// A lot of stuff is weird here because we want it to be at least somewhat tough
// for people to crack.

// Comment if you do not want timebomb
#define USE_TIMEBOMB

constexpr UINT TIMEBOMB_YEAR  = 2025;
constexpr UINT TIMEBOMB_MONTH = 4; // extended to april 2025, as of 09/11/24
constexpr UINT TIMEBOMB_DAY   = 1;

constexpr char MONTH_DAYS[] =
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

constexpr UINT GetDay(UINT year, UINT month, UINT day)
{
	UINT out = (year - 1) * 365;
	for (UINT i = 0; i < month - 1; i++)
	{
		out += MONTH_DAYS[i];
	}
	out += day;
	return out;
}

constexpr UINT TIMEBOMB_DAYS = GetDay(TIMEBOMB_YEAR, TIMEBOMB_MONTH, TIMEBOMB_DAY);

volatile const char s_k32[] = {
	~'k',
	~'e',
	~'r',
	~'n',
	~'e',
	~'l',
	~'3',
	~'2',
	~'.',
	~'d',
	~'l',
	~'l',
	0xFF
};

volatile const char s_gst[] = {
	~'G',
	~'e',
	~'t',
	~'S',
	~'y',
	~'s',
	~'t',
	~'e',
	~'m',
	~'T',
	~'i',
	~'m',
	~'e',
	0xFF
};

typedef void (WINAPI *GetSystemTime_t)(LPSYSTEMTIME lpSystemTime);

void invert(volatile const char *in, char *out, size_t outSize)
{
	for (size_t i = 0; i < outSize; i++)
	{
		out[i] = ~in[i];
	}
}

__forceinline void CheckTimeBomb(void)
{
#ifdef USE_TIMEBOMB
	char k32[ARRAYSIZE(s_k32)];
	char gst[ARRAYSIZE(s_gst)];
	invert(s_k32, k32, ARRAYSIZE(k32));
	invert(s_gst, gst, ARRAYSIZE(gst));

	HMODULE hk32 = LoadLibraryA(k32);
	GetSystemTime_t pGetSystemTime = (GetSystemTime_t)GetProcAddress(hk32, gst);

	SYSTEMTIME time;
	pGetSystemTime(&time);
	if (GetDay(time.wYear, time.wMonth, time.wDay) >= TIMEBOMB_DAYS)
	{
		// non-existent ordinal
		FARPROC crash = GetProcAddress(hk32, (LPCSTR)1903);
		crash();
	}
#endif
}