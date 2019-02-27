#include "base.h"

#include <raymai/Printf.h>
#include "ErrText.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define PAUSE  getc(stdin) */
#define PAUSE  puts("")

static void my_printf(
	char const *pszFmt, ...);



static void my_test_BadFmtStrHandling__0(
	char const *pszFmt,
	char const *pszPtr,
	enum RaymaiPrintf_Err expectedErr);

static void my_test_BadFmtStrHandling(void);



static void my_test_string(void);



#define F_RIGHT_ALIGN  ""
#define F_LEFT_ALIGN  "-"
#define F_POSITIVE_SIGN  "+"
#define F_SPACE_SIGN  " "
#define F_PAD_WITH_ZERO "0"

static char const * const s_arrSignedIntFlag[] = {
	F_RIGHT_ALIGN,
	F_RIGHT_ALIGN F_POSITIVE_SIGN,
	F_RIGHT_ALIGN F_SPACE_SIGN,
	F_RIGHT_ALIGN F_PAD_WITH_ZERO F_POSITIVE_SIGN,
	F_RIGHT_ALIGN F_PAD_WITH_ZERO F_SPACE_SIGN,
	F_LEFT_ALIGN,
	F_LEFT_ALIGN F_POSITIVE_SIGN,
	F_LEFT_ALIGN F_SPACE_SIGN,
	F_LEFT_ALIGN F_PAD_WITH_ZERO F_POSITIVE_SIGN,
	F_LEFT_ALIGN F_PAD_WITH_ZERO F_SPACE_SIGN,
};

#if (LONG_MAX > INT_MAX)
#define MY_SHOULD_TEST_ld  (1)
#else
#define MY_SHOULD_TEST_ld  (0)
#endif

#if defined(RAYMAI_PRINTF_ENABLE_LONGLONG)
#define MY_SHOULD_TEST_lld  (1)
#else
#define MY_SHOULD_TEST_lld  (0)
#endif

static void my_test_int_d(void);
static void my_test_int_u(void);
static void my_test_int_x(void);
static void my_test_int_X(void);
static void my_test_int_o(void);

#if MY_SHOULD_TEST_ld
static void my_test_int_ld(void);
static void my_test_int_lu(void);
static void my_test_int_lx(void);
static void my_test_int_lX(void);
static void my_test_int_lo(void);
#endif

#if MY_SHOULD_TEST_lld
static void my_test_int_lld(void);
static void my_test_int_llu(void);
static void my_test_int_llx(void);
static void my_test_int_llX(void);
static void my_test_int_llo(void);
#endif

