#pragma once
#include "framework.h"
#include <cstdint>

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

static BOOL MaskCompare(PVOID pBuffer, LPCSTR lpPattern, LPCSTR lpMask)
{
	for (PBYTE value = (PBYTE)pBuffer; *lpMask; ++lpPattern, ++lpMask, ++value)
	{
		if (*lpMask == 'x' && *(LPCBYTE)lpPattern != *value)
			return FALSE;
	}

	return TRUE;
}

static __declspec(noinline) PVOID FindPatternHelper(PVOID pBase, SIZE_T dwSize, LPCSTR lpPattern, LPCSTR lpMask)
{
	for (SIZE_T index = 0; index < dwSize; ++index)
	{
		PBYTE pAddress = (PBYTE)pBase + index;

		if (MaskCompare(pAddress, lpPattern, lpMask))
			return pAddress;
	}

	return NULL;
}

inline PVOID FindPattern2(PVOID pBase, SIZE_T dwSize, LPCSTR lpPattern, LPCSTR lpMask)
{
	dwSize -= strlen(lpMask);
	return FindPatternHelper(pBase, dwSize, lpPattern, lpMask);
}

extern BOOL g_bIsArm64;

// small UNWIND_INFO implementation
typedef struct _UNWIND_INFO {
	unsigned char Version : 3;
	unsigned char Flags : 5;
} UNWIND_INFO, * PUNWIND_INFO;

static uintptr_t GetFunctionStart(uintptr_t address, uintptr_t BaseAddress)
{
	DWORD64 ImgBase = 0;
	PRUNTIME_FUNCTION Function = nullptr;
	for (auto Func = RtlLookupFunctionEntry(address, &ImgBase, NULL); Func;
		Func = RtlLookupFunctionEntry(BaseAddress + (Func->BeginAddress - 1), &ImgBase, NULL))
	{
		auto UnwindInfo = reinterpret_cast<PUNWIND_INFO>(BaseAddress + Func->UnwindInfoAddress);
		if (UnwindInfo->Flags & UNW_FLAG_CHAININFO)
			continue;

		Function = Func;
		break;
	}

	return Function ? BaseAddress + Function->BeginAddress : 0;
}

//adapted from dumper7 (ue4/5 sdk dumper), i wasnt bothered enough to write this shit from scratch
inline void* FindByString(uintptr_t baseaddress, const wchar_t* RefStr)
{
	uintptr_t ImageBase = baseaddress;
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)(ImageBase);
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
	PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeader);

	uint8_t* DataSection = nullptr;
	uint8_t* TextSection = nullptr;
	DWORD DataSize = 0;
	DWORD TextSize = 0;

	uint8_t* StringAddress = nullptr;

	for (int i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

		if (strcmp((const char*)CurrentSection.Name, ".rdata") == 0 && !DataSection)
		{
			DataSection = (uint8_t*)(CurrentSection.VirtualAddress + ImageBase);
			DataSize = CurrentSection.Misc.VirtualSize;
		}
		else if (strcmp((const char*)CurrentSection.Name, ".text") == 0 && !TextSection)
		{
			TextSection = (uint8_t*)(CurrentSection.VirtualAddress + ImageBase);
			TextSize = CurrentSection.Misc.VirtualSize;
		}
	}

	size_t refStrLen = wcslen(RefStr) * sizeof(wchar_t);
	uint64_t* refStr64 = (uint64_t*)RefStr;

	for (size_t i = 0; i < DataSize; i++)
	{
		if (*((uint64_t*)(DataSection + i)) == *refStr64)
		{
			if (memcmp(RefStr, DataSection + i, refStrLen) == 0)
			{
				StringAddress = DataSection + i;
				break;
			}
		}
	}

	if (!StringAddress)
	{
		for (size_t i = 0; i < TextSize; i++)
		{
			if (*((uint64_t*)(TextSection + i)) == *refStr64)
			{
				if (memcmp(RefStr, TextSection + i, refStrLen) == 0)
				{
					StringAddress = TextSection + i;
					break;
				}
			}
		}
	}

	for (size_t i = 0; i < TextSize; i++)
	{
		// opcode: lea
		if ((TextSection[i] == uint8_t(0x4C) || TextSection[i] == uint8_t(0x48)) && TextSection[i + 1] == uint8_t(0x8D))
		{
			const uint8_t* StrPtr = *(int32_t*)(TextSection + i + 3) + 7 + TextSection + i;

			if (StrPtr == StringAddress)
			{
				return { TextSection + i };
			}
		}
	}

	return nullptr;
}