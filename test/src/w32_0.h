#include "base.h"
#include <Windows.h>

static void * MemAllocZero(SIZE_T cb);
static void MemFree(void *ptr);

static int MyColonExHandler(
	char cType, PCSTR pszExt, int mode, void *pArg, void *pUser
);

#define RAYMAI_PRINTF_MALLOC0  MemAllocZero
#define RAYMAI_PRINTF_MEMFREE  MemFree
#define RAYMAI_PRINTF_CHARTYPE  TCHAR
#define RAYMAI_PRINTF_ENABLE_ASCII_FMTSTR
#define RAYMAI_PRINTF_ENABLE_COLONEX
#define RAYMAI_PRINTF_COLONEX_HANDLER  MyColonExHandler
#include <raymai/Printf.h>
#include <stdarg.h>
#include "ErrText.h"

#if defined(UNICODE)
static int MyColonEx_ansi(int mode, void **ppsz);
static int MyColonEx_asci(int mode, void **ppsz);
#endif
static int MyColonEx_mbcs(int cp, int mode, void **ppsz);
#if !defined(UNICODE)
static int MyColonEx_wide(int mode, void **ppsz);
#endif

static int MyColonEx_upper(int mode, TCHAR *pc);
static int MyColonEx_lower(int mode, TCHAR *pc);

static int MyW32_A2W(int cp, PCSTR pszA, PWSTR *ppszW);
static int MyW32_W2A(int cp, PCWSTR pszW, PSTR *ppszA);

enum {
	MYID_FIRST = 99,
	lbxLog
};

static HWND My__Cw(void);
static LRESULT CALLBACK My__WndProc(
	HWND hwnd, UINT msg, WPARAM w, LPARAM l);
static void MyOnCreate(HWND hwnd);
static void MyOnSize(HWND hwnd, int cx, int cy);

static void My_InitCtl(HWND hwnd);
static void MyLbAddString(HWND hCtl, PCTSTR pszStr);
static void MyLogf(HWND hwnd, PCSTR pszFmt, ...);
static void My_TestBegin(HWND hwnd);
