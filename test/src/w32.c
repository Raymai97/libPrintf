#include <Windows.h>

static int MyColonExHandler(
	char cType, PCSTR pszExt, int mode, void *pArg, void *pUser
);

#define RAYMAI_PRINTF_CHARTYPE  TCHAR
#define RAYMAI_PRINTF_ENABLE_ASCII_FMTSTR
#define RAYMAI_PRINTF_ENABLE_COLONEX
#define RAYMAI_PRINTF_COLONEX_HANDLER  MyColonExHandler
#include <raymai/Printf.h>
#include "ErrText.h"

static int MyColonEx_ansi(int mode, void **ppsz);
static int MyColonEx_ascii(int mode, void **ppsz);
static int MyColonEx_wide(int mode, void **ppsz);

static int MyColonEx_upper(int mode, TCHAR *pc);
static int MyColonEx_lower(int mode, TCHAR *pc);

static void * MemAllocZero(SIZE_T cb)
{
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}

static void MemFree(void *ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

static int MyColonExHandler(
	char cType, PCSTR pszExt, int mode, void *pArg, void *pUser)
{
	UNREFERENCED_PARAMETER(pUser);
	RAYMAI_PRINTF_COLONEX_ENUM;
	int r = ColonExReturnNoOp;
	switch (cType) {
	case 's':
#if defined(UNICODE)
		if (strcmp(pszExt, "ansi") == 0) {
			if (mode == ColonExModeFree) { MemFree(*(void**)pArg); }
			else { r = MyColonEx_ansi(mode, (void**)pArg); }
		}
		else if (strcmp(pszExt, "ascii") == 0) {
			if (mode == ColonExModeFree) { MemFree(*(void**)pArg); }
			else { r = MyColonEx_ascii(mode, (void**)pArg); }
		}
#else /* no UNICODE */
		if (strcmp(pszExt, "wide") == 0) {
			if (mode == ColonExModeFree) { MemFree(*(void**)pArg); }
			else { r = MyColonEx_wide(mode, (void**)pArg); }
		}
#endif/* UNICODE */
		break;
	case 'c':
		if (strcmp(pszExt, "upper") == 0) {
			r = MyColonEx_upper(mode, (TCHAR*)pArg);
		}
		else if (strcmp(pszExt, "lower") == 0) {
			r = MyColonEx_lower(mode, (TCHAR*)pArg);
		}
		break;
	}
	return r;
}

#if defined(UNICODE)
static int MyColonEx_ansi(int mode, void **ppsz)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	PCSTR const pszA = *ppsz;
	int nMax = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0);
	if (mode == ColonExModePut) {
		PWSTR pszW = MemAllocZero(sizeof(WCHAR) * nMax);
		if (!pszW) {
			printf("OUT OF MEM\n"); return ColonExReturnAbort;
		}
		MultiByteToWideChar(CP_ACP, 0, pszA, -1, pszW, nMax);
		*ppsz = pszW;
	}
	return nMax - 1;
}
static int MyColonEx_ascii(int mode, void **ppsz)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	PCSTR const pszA = *ppsz;
	int n; for (n = 0; pszA[n]; ++n);
	if (mode == ColonExModePut) {
		PWSTR pszW = MemAllocZero(sizeof(WCHAR) * (n + 1));
		if (!pszW) {
			printf("OUT OF MEM\n"); return ColonExReturnAbort;
		}
		for (n = 0; pszW[n] = pszA[n], pszA[n]; ++n);
		*ppsz = pszW;
	}
	return n;
}
#else /* no UNICODE */
static int MyColonEx_wide(int mode, void **ppsz)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	PCWSTR const pszW = *ppsz;
	int cbMax = WideCharToMultiByte(CP_ACP, 0, pszW, -1, NULL, 0, NULL, NULL);
	if (mode == ColonExModePut) {
		PSTR pszA = MemAllocZero(cbMax);
		if (!pszA) {
			printf("OUT OF MEM\n"); return ColonExReturnAbort;
		}
		WideCharToMultiByte(CP_ACP, 0, pszW, -1, pszA, cbMax, NULL, NULL);
		*ppsz = pszA;
	}
	return cbMax - 1;
}
#endif/* UNICODE */

static int MyColonEx_upper(int mode, TCHAR *pc)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	if (mode == ColonExModePut) {
		if (*pc >= 'a' && *pc <= 'z') {
			*pc -= 'a' - 'A';
		}
		return 1;
	}
	return ColonExReturnNoOp;
}
static int MyColonEx_lower(int mode, TCHAR *pc)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	if (mode == ColonExModePut) {
		if (*pc >= 'A' && *pc <= 'Z') {
			*pc += 'a' - 'A';
		}
		return 1;
	}
	return ColonExReturnNoOp;
}


void test_w32(void)
{
	/* TODO: Test code to demonstrate how useful ColonEx is in Win32 */
}
