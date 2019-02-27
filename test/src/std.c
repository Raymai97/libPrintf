#include "std_0.h"

static void my_printf(
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
	if (strcmp(sz1, sz2) != 0) {
		printf("Fmtstr: [%s]\n", pszFmt);
		va_start(ap, pszFmt);
		vprintf("   arg: [%d]\n", ap);
		va_end(ap);
		printf("   std: [%s]\n", sz1);
		printf("   ray: [%s]\n", sz2);
	}
	assert(r1 == r2);
	assert(strcmp(sz1, sz2) == 0);
}

static void my_test_BadFmtStrHandling__0(
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

static void my_test_BadFmtStrHandling(void)
{
	puts("> Press ENTER to go next.\n"); 
	
	my_test_BadFmtStrHandling__0(
		"%",
		" ^",
		RaymaiPrintf_Err_End);

	my_test_BadFmtStrHandling__0(
		"%q",
		" ^",
		RaymaiPrintf_Err_Char);

	my_test_BadFmtStrHandling__0(
		"%..u",
		"  ^",
		RaymaiPrintf_Err_Repeat);

	my_test_BadFmtStrHandling__0(
		"%lhd",
		"  ^",
		RaymaiPrintf_Err_Longg);

	my_test_BadFmtStrHandling__0(
		"%*.**d",
		"    ^",
		RaymaiPrintf_Err_Repeat);

	my_test_BadFmtStrHandling__0(
		"%20p",
		" ''^",
		RaymaiPrintf_Err_Width);

	my_test_BadFmtStrHandling__0(
		"%.8p",
		" ' ^",
		RaymaiPrintf_Err_Preci);
}

static void my_test_string(void)
{
	char const * arrWord[] = { "12345", "1234567890", "123456789012345" };
	int arrNum[] = { 7, 12, 18, 25 };
	int i, j; for (i = 0; i < ARR_SIZE(arrWord); ++i) {
		char const *pszWord = arrWord[i];
		for (j = 0; j < ARR_SIZE(arrNum) - 1; ++j) {
			int const n1 = arrNum[j + 0];
			int const n2 = arrNum[j + 1];
			my_printf("%s", pszWord);
			my_printf("%*s", n1, pszWord);
			my_printf("%.*s", n1, pszWord);
			my_printf("%*s", n2, pszWord);
			my_printf("%.*s", n2, pszWord);
			my_printf("%*.*s", n1, n2, pszWord);
			my_printf("%*.*s", n2, n1, pszWord);
		}
	}
}

#if defined(RAYMAI_PRINTF_ENABLE_LONGLONG)
#define H_PriType  "lld"
#define H_IntType  RAYMAI_PRINTF_LONGLONGTYPE
#define H_FunName  my_test_int_lld
#include "std_d.h"
#endif

#if (LONG_MAX > INT_MAX)
#define H_PriType  "ld"
#define H_IntType  long
#define H_FunName  my_test_int_ld
#include "std_d.h"
#endif

#define H_PriType  "d"
#define H_IntType  int
#define H_FunName  my_test_int_d
#include "std_d.h"

void test_std(void)
{
	puts("\nTest: Bad format string handling \n");
	my_test_BadFmtStrHandling();
	puts("\nTest: String \n");
	my_test_string();
	puts("\nTest: Int (%d) \n");
	my_test_int_d();
#if MY_SHOULD_TEST_ld
	puts("\nTest: Int (%ld) \n");
	my_test_int_ld();
#endif
#if MY_SHOULD_TEST_lld
	puts("\nTest: Int (%lld) \n");
	my_test_int_lld();
#endif
}
