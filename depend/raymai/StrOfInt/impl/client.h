/*
	StrOfInt is a header-only library.
	By default, it defines a static function called StrOfInt.
	By #define you may define
		the function name,
		the pointer to char type,
		the integer type,
		the signness of the integer type.
	As such, it may be included more than once.

	StrOfInt is low-level. It does not format the result string.
	It does not null-terminate the result string.
	It does not put any prefix except negative sign for radix 10.
	This design is intentional, to give maximum control to caller.
	e.g. To put string of integer in the middle of a string.

	"pszStr" can be NULL if you don't need the result string.
	If "pszStr" is NULL, "nStr" must be zero.
	If "pszStr" is not NULL, "nStr" must be non-zero.
	"nStr" specify max number of chars (exclude null-termi) to be written.
	If "nStr" is negative, strlen(pszStr) will be used instead.
	"radix" must be either 2 (bin), 8 (oct), 10 (dec) or 16 (hex).
	
	On success:
		The function returns the number of characters of string of int.
	
	On error:
		The function returns negative value such as StrOfInt_errBufBoom.
*/

#ifndef STROFINT_FUNCNAME
#define STROFINT_FUNCNAME  StrOfInt
#endif

#ifndef STROFINT_PSTRTYPE
#define STROFINT_PSTRTYPE  char *
#endif

#ifndef STROFINT_INTTYPE
#define STROFINT_INTTYPE  int
#endif

#ifndef STROFINT_SIGNED
#define STROFINT_SIGNED  (1)
#endif

#ifndef STROFINT_UPPERHEX
#define STROFINT_UPPERHEX  (1)
#endif

#ifndef STROFINT__ERR_ENUM
#define STROFINT__ERR_ENUM
enum {
	StrOfInt_errBufBoom = -1,
	StrOfInt_errBadArg = -2
};
#endif/* STROFINT__ERR_ENUM */

#define STROFINT__CONSUME(c) \
	++n; if (p) { if (n > nStr) { return StrOfInt_errBufBoom; } *p++ = (c); }

#if (STROFINT_UPPERHEX)
#define STROFINT__DIGITCHARS  "0123456789ABCDEF"
#else
#define STROFINT__DIGITCHARS  "0123456789abcdef"
#endif

static int
STROFINT_FUNCNAME(
	STROFINT_PSTRTYPE const pszStr, int nStr,
	STROFINT_INTTYPE const intVal, int const radix)
{
	typedef STROFINT_INTTYPE rInt;
	typedef STROFINT_PSTRTYPE rPStr;
	int n = 0;
	rInt dInt = intVal; 
	rPStr p = pszStr, p2 = NULL;
	if (pszStr) {
		if (nStr == 0) { return StrOfInt_errBufBoom; }
		else if (nStr < 0) { for (; pszStr[nStr]; ++nStr); }
	}
	else {
		if (nStr != 0) { return StrOfInt_errBadArg; }
	}
	switch (radix) {
	case 2: case 8: case 10: case 16: break;
	default: return StrOfInt_errBadArg;
	}
	if (dInt == 0) { STROFINT__CONSUME('0'); return 1; }
	while (dInt != 0) {
		int iDigit = (int)(dInt % radix);
#if (STROFINT_SIGNED)
		if (iDigit < 0) { iDigit = -iDigit; }
#endif
		STROFINT__CONSUME(STROFINT__DIGITCHARS[iDigit]);
		dInt /= (rInt)radix;
	}
#if (STROFINT_SIGNED)
	if (radix == 10 && intVal < 0) { STROFINT__CONSUME('-'); }
#endif
	if (p) for (p2 = --p, p = pszStr; p < p2; ++p, --p2) {
		int c = *p; *p = *p2; *p2 = *(rPStr)&c;
	}
	return n;
}

#undef STROFINT__CONSUME
#undef STROFINT__DIGITCHARS
