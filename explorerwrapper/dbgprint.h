#pragma once
#include <Windows.h>
extern "C" void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

extern HINSTANCE g_hInstance;

void dbgvprintf(LPCWSTR format, void* _argp);
void dbgprintf(LPCWSTR format, ...);

BOOL WINAPI ChangeImportedAddress_FARPROC( HMODULE hModule, LPSTR modulename, FARPROC origfunc, FARPROC newfunc );
__declspec(noinline) BOOL WINAPI ChangeImportedAddress_ORDINAL( HMODULE hModule, LPSTR modulename, ULONGLONG origOrdinal, FARPROC newfunc );
__declspec(noinline) BOOL WINAPI ChangeExportedAddress_ORDINAL( HMODULE hModule, ULONGLONG origOrdinal, LPCSTR newForward);
#define ChangeImportedAddress(hModule,modulename,origproc,newproc) ChangeImportedAddress_FARPROC(hModule,modulename,(FARPROC)origproc,(FARPROC)newproc)
#define ChangeImportedAddressORDINAL(hModule,modulename,origproc,newproc) ChangeImportedAddress_ORDINAL(hModule,modulename,origproc,(FARPROC)newproc)
#define ChangeExportedAddress_ORDINAL(hModule,origproc,newproc) ChangeExportedAddress_ORDINAL(hModule,origproc,newproc)

static int
IsSpaceCS(char ch)
{
	return ((ch == ' ') || (ch == '\t'));
}

static int
IsDigitCS(char ch)
{
	return (((ch >= '0') && (ch <= '9')));
}

static int
isAlphaCS(char c)
{
	/*
	 * Depends on ASCII-like character ordering.
	 */
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ? 1 : 0);
}

static int
isUpperCS(int c)
{
	return (c >= 'A' && c <= 'Z');
}

static unsigned long
strtoulCUSTOM(const char* nptr, char** endptr, int base)
{
	const char* s = nptr;
	unsigned long acc;
	int c;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (IsSpaceCS(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	}
	else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
		c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (IsDigitCS(c))
			c -= '0';
		else if (isAlphaCS(c))
			c -= isUpperCS(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULONG_MAX;
		//errno = ERANGE;
	}
	else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char*)(any ? s - 1 : nptr);
	return (acc);
}

template <class T>
struct wiktorArray
{
	int size;
	T* data;

	void push_back(T InputData)
	{
		data = (T*)realloc(data, sizeof(T) * (size + 1));
		data[size++] = InputData;
	}

	~wiktorArray()
	{
		if (data)
			free(data);
	}
};

static wiktorArray<int> patternToByte(const char* pattern)
{
	auto bytes = wiktorArray<int>{};
	const auto start = const_cast<char*>(pattern);
	const auto end = const_cast<char*>(pattern) + strlen(pattern);

	for (auto current = start; current < end; ++current)
	{
		if (*current == '?')
		{
			++current;
			if (*current == '?')
				++current;
			bytes.push_back(-1);
		}
		else { bytes.push_back(strtoulCUSTOM(current, &current, 16)); }
	}
	return bytes;
}

static uintptr_t FindPattern(uintptr_t baseAddress, const char* signature)
{
	const auto dosHeader = (PIMAGE_DOS_HEADER)baseAddress;
	const auto ntHeaders = (PIMAGE_NT_HEADERS)((unsigned char*)baseAddress + dosHeader->e_lfanew);

	const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = patternToByte(signature);
	const auto scanBytes = reinterpret_cast<unsigned char*>(baseAddress);

	const auto s = patternBytes.size;
	const auto d = patternBytes.data;

	for (auto i = 0ul; i < sizeOfImage - s; ++i)
	{
		bool found = true;
		for (auto j = 0ul; j < s; ++j)
		{
			if (scanBytes[i + j] != d[j] && d[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			uintptr_t address = reinterpret_cast<uintptr_t>(&scanBytes[i]);
			return address;
		}
	}

	return NULL;
}