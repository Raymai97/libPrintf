#include "w32_0.h"

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
		if (mode == ColonExModeFree) {
			MemFree(*(void**)pArg);
		}
#if defined(UNICODE)
		else if (strcmp(pszExt, "ansi") == 0) {
			r = MyColonEx_ansi(mode, (void**)pArg); 
		}
		else if (strcmp(pszExt, "asci") == 0) {
			r = MyColonEx_asci(mode, (void**)pArg); 
		}
#else /* no UNICODE */
		else if (strcmp(pszExt, "wide") == 0) {
			r = MyColonEx_wide(mode, (void**)pArg);
		}
#endif/* UNICODE */
		else if (strcmp(pszExt, "c932") == 0) {
			r = MyColonEx_mbcs(932, mode, (void**)pArg);
		}
		else if (strcmp(pszExt, "c1252") == 0) {
			r = MyColonEx_mbcs(1252, mode, (void**)pArg);
		}
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
	PCSTR const pszSrc = *ppsz;
	PWSTR pszW = NULL;
	int r = 0;
	r = MyW32_A2W(CP_ACP, pszSrc, mode == ColonExModePut ? &pszW : NULL);
	if (r < 0) {
		return ColonExReturnAbort;
	}
	*ppsz = pszW;
	return r;
}
#endif

#if defined(UNICODE)
static int MyColonEx_asci(int mode, void **ppsz)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	PCSTR const pszA = *ppsz;
	int n; for (n = 0; pszA[n]; ++n);
	if (mode == ColonExModePut) {
		PWSTR pszW = MemAllocZero(sizeof(WCHAR) * (n + 1));
		if (!pszW) {
			return ColonExReturnAbort;
		}
		for (n = 0; pszW[n] = pszA[n], pszA[n]; ++n);
		*ppsz = pszW;
	}
	return n;
}
#endif

static int MyColonEx_mbcs(int cp, int mode, void **ppsz)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	PCSTR const pszSrc = *ppsz;
	int r = 0;
	PWSTR pszW = NULL;
#if defined(UNICODE)
	r = MyW32_A2W(cp, pszSrc, mode == ColonExModePut ? &pszW : NULL);
	if (r < 0) goto eof;
	*ppsz = pszW;
#else
	PSTR pszA = NULL;
	r = MyW32_A2W(cp, pszSrc, &pszW);
	if (r < 0) goto eof;
	r = MyW32_W2A(CP_ACP, pszW, mode == ColonExModePut ? &pszA : NULL);
	if (r < 0) goto eof;
	*ppsz = pszA; 
#endif
eof:
#if !defined(UNICODE)
	if (pszW) { MemFree(pszW); }
#endif
	if (r < 0) {
		return ColonExReturnAbort;
	}
	return r;
}

#if !defined(UNICODE)
static int MyColonEx_wide(int mode, void **ppsz)
{
	RAYMAI_PRINTF_COLONEX_ENUM;
	PCWSTR const pszSrc = *ppsz;
	int r = 0;
	PSTR pszA = NULL;
	r = MyW32_W2A(CP_ACP, pszSrc, mode == ColonExModePut ? &pszA : NULL);
	if (r < 0) {
		return ColonExReturnAbort;
	}
	*ppsz = pszA;
	return r;
}
#endif

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

static int MyW32_A2W(int cp, PCSTR pszA, PWSTR *ppszW)
{
	int r = MultiByteToWideChar(cp, 0, pszA, -1, NULL, 0);
	if (r > 0 && ppszW) {
		PWSTR pszW = MemAllocZero(sizeof(WCHAR) * r);
		if (!pszW) {
			return -1;
		}
		MultiByteToWideChar(cp, 0, pszA, -1, pszW, r);
		*ppszW = pszW;
	}
	return r - 1;
}

static int MyW32_W2A(int cp, PCWSTR pszW, PSTR *ppszA)
{
	int r = WideCharToMultiByte(cp, 0, pszW, -1, NULL, 0, NULL, NULL);
	if (r > 0 && ppszA) {
		PSTR pszA = MemAllocZero(r);
		if (!pszA) {
			return -1;
		}
		WideCharToMultiByte(cp, 0, pszW, -1, pszA, r, NULL, NULL);
		*ppszA = pszA;
	}
	return r - 1;
}

static HWND My__Cw(void)
{
	WNDCLASS wc = { 0 };
	wc.lpszClassName = TEXT("Test_Raymai_Printf.w32");
	wc.lpfnWndProc = My__WndProc;
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	return CreateWindowEx(0,
		wc.lpszClassName, wc.lpszClassName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		NULL, NULL, NULL, NULL);
}

static LRESULT CALLBACK My__WndProc(
	HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
	LRESULT lResult = 0;
	BOOL overriden = FALSE;
	if (msg == WM_CREATE) {
		MyOnCreate(hwnd);
	}
	else if (msg == WM_SIZE) {
		MyOnSize(hwnd, (short)LOWORD(l), (short)HIWORD(l));
	}
	if (!overriden) {
		lResult = DefWindowProc(hwnd, msg, w, l);
	}
	return lResult;
}

static void MyOnCreate(HWND hwnd)
{
	My_InitCtl(hwnd);
	My_TestBegin(hwnd);
}

static void MyOnSize(HWND hwnd, int cx, int cy)
{
	HWND hCtl = GetDlgItem(hwnd, lbxLog);
	MoveWindow(hCtl, 10, 10, cx - 20, cy - 20, TRUE);
}

static void My_InitCtl(HWND hwnd)
{
	HWND hCtl = NULL;
	hCtl = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("ListBox"), NULL,
		WS_CHILD | WS_VISIBLE | LBS_NOINTEGRALHEIGHT,
		0, 0, 0, 0, hwnd,
		(HMENU)(WPARAM)lbxLog, NULL, NULL);
	SetFocus(hCtl);
}

static void MyLbAddString(HWND hCtl, PCTSTR pszStr)
{
	int count = (int)SendMessage(hCtl, LB_GETCOUNT, 0, 0);
	SendMessage(hCtl, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)pszStr);
	SendMessage(hCtl, LB_SETCURSEL, count, 0L);
}

static void MyLogf(HWND hwnd, PCSTR pszFmt, ...)
{
	HWND const hCtl = GetDlgItem(hwnd, lbxLog);
	PTSTR pszLog = NULL;
	va_list ap;
	va_start(ap, pszFmt);
	if (raymai_vasprintf(&pszLog, pszFmt, ap) >= 0) {
		PTSTR pszLine = pszLog, p;
		for (p = pszLog; *p; ++p) {
			if (*p == '\n') {
				*p++ = 0;
				MyLbAddString(hCtl, pszLine);
				pszLine = p;
			}
		}
		MyLbAddString(hCtl, pszLine);
	}
	va_end(ap);
	if (pszLog) { MemFree(pszLog); }
}

static void My_TestBegin(HWND hwnd)
{
	PCSTR const psz932A =
		"\x8B\xBF\x82\xAF\x8A\x6D\x82\xA9\x82\xC8\x8F\xD5\x93\xAE";
	PCWSTR const psz932W =
		L"\x97FF\x3051\x78BA\x304B\x306A\x885D\x52D5";
	PCSTR const psz1252A =
		"\xA5 Yen dollar, \xA9 Copyright, \xAE Reserved";
	PCWSTR const psz1252W =
		L"\x00A5 Yen dollar, \x00A9 Copyright, \x00AE Reserved";

	MyLogf(hwnd, "\nsource: US CP1252");
	MyLogf(hwnd, "%%:ansi:s = %:ansi:s", psz1252A);
	MyLogf(hwnd, "%%:asci:s = %:asci:s", psz1252A);
	MyLogf(hwnd, "%%:c1252:s = %:c1252:s", psz1252A);
	MyLogf(hwnd, "\nsource: Windows Unicode");
	MyLogf(hwnd, "%%:wide:s = %:wide:s", psz1252W);
	MyLogf(hwnd, "\nsource: Japan CP932");
	MyLogf(hwnd, "%%:ansi:s = %:ansi:s", psz932A);
	MyLogf(hwnd, "%%:asci:s = %:asci:s", psz932A);
	MyLogf(hwnd, "%%:c932:s = %:c932:s", psz932A);
	MyLogf(hwnd, "\nsource: Windows Unicode");
	MyLogf(hwnd, "%%:wide:s = %:wide:s", psz932W);
}

void test_w32(void)
{
	HWND hwnd = My__Cw();
	ShowWindow(hwnd, SW_SHOWNORMAL);
	while (IsWindow(hwnd)) {
		MSG msg = { 0 };
		if (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
