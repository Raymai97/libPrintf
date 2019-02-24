#include "base.h"

#include <raymai/Printf.h>
#include "ErrText.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define PAUSE  getc(stdin) */
#define PAUSE  puts("")

static void test_handle_bad_format_string(
	char const *pszFmt,
	char const *pszPtr,
	enum RaymaiPrintf_Err expectedErr)
{
	enum RaymaiPrintf_Err r = 0;
	printf("Format string: %s\n", pszFmt);
	printf("               %s\n", pszPtr);
	r = raymai_snprintf(0, 0, pszFmt, 1, 2, 3, 4, 5);
	assert(r == expectedErr);
	printf(" What's wrong: %s.\n", RaymaiPrintf_ErrText(r));
	PAUSE;
}

static void test_handle_bad_format_strings(void)
{
	puts("Test: Handle bad format string \n"); 
	puts("> Press ENTER to go next.\n"); 
	
	test_handle_bad_format_string(
		"%",
		" ^",
		RaymaiPrintf_Err_End);

	test_handle_bad_format_string(
		"%q",
		" ^",
		RaymaiPrintf_Err_Char);

	test_handle_bad_format_string(
		"%..u",
		"  ^",
		RaymaiPrintf_Err_Repeat);

	test_handle_bad_format_string(
		"%lhd",
		"  ^",
		RaymaiPrintf_Err_Longg);

	test_handle_bad_format_string(
		"%*.**d",
		"    ^",
		RaymaiPrintf_Err_Repeat);

	test_handle_bad_format_string(
		"%20p",
		" ''^",
		RaymaiPrintf_Err_Width);

	test_handle_bad_format_string(
		"%.8p",
		" ' ^",
		RaymaiPrintf_Err_Preci);
}

static void test_match_std_behavior(
	char const *pszFmt, ...)
{
	enum { nMax = 999 };
	char sz1[nMax] = { 0 }, sz2[nMax] = { 0 };
	int r1 = 0, r2 = 0;
	va_list ap;
	va_start(ap, pszFmt);
	r1 = vsprintf(sz1, pszFmt, ap);
	va_end(ap);
	va_start(ap, pszFmt);
	r2 = raymai_vsnprintf(sz2, nMax, pszFmt, ap);
	va_end(ap);
	assert(r1 == r2);
	assert(strcmp(sz1, sz2) == 0);
}

void test_print_string_behaviors(void)
{
	char const * arrWord[] = { "12345", "1234567890", "123456789012345" };
	enum { nWords = sizeof(arrWord) / sizeof(*arrWord) };
	int arrLen[] = { 7, 12, 18, 25 };
	enum { nMin = sizeof(arrLen) / sizeof(*arrLen) };
	int i, j; for (i = 0; i < nWords; ++i) {
		char const *pszWord = arrWord[i];
		for (j = 0; j < nMin - 1; ++j) {
			int const n1 = arrLen[j + 0];
			int const n2 = arrLen[j + 1];
			test_match_std_behavior("%s", pszWord);
			test_match_std_behavior("%*s", n1, pszWord);
			test_match_std_behavior("%.*s", n1, pszWord);
			test_match_std_behavior("%*s", n2, pszWord);
			test_match_std_behavior("%.*s", n2, pszWord);
			test_match_std_behavior("%*.*s", n1, n2, pszWord);
			test_match_std_behavior("%*.*s", n2, n1, pszWord);
		}
	}
}

void test_std(void)
{
	test_handle_bad_format_strings();
	test_print_string_behaviors();
}
