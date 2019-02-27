/*
	Raymai_Printf implements the famous printf functions in such a way
	that provides some C99 features without requiring C99 compiler.

	Specifiers support:
	* Char   * String   * Integer (Dec/Oct/Hex)
	  %c       %s         %d %i %u %o %x %X
	* Pointer
	  %p

	There are 5 standard flags. My implementation supports all of them
	except '#' flag as I personally despise its behavior.

	Width and precision are implemented according to the C standard.
	|- For string:
	| > width = minimum strlen
	| > preci = maximum strlen
	|
	|- For integer:
	| > width = minimum strlen
	| > preci = minimum digit count
	| >> If no preci dot, it prints ((int)0) as "0".
	| >> Else if preci is 0, it prints ((int)0) as "".
	| >> "%.d" are consider equivalent to "%.0d".

	Length sub-specifier modifies the length of specifier's data type.
	For integer, all C99 standard ones are implemented:
		hh (signed char), h (short), l (long), ll (long long),
		j (intmax_t), z (size_t), t (ptrdiff_t)
		where intmax_t is long or long long, depends on whether
		RAYMAI_PRINTF_ENABLE_LONGLONG is defined.
	For char and string, length sub-specifiers are NOT supported.
		Use ColonEx instead. It offers much more than what these
		LSS could ever offer.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef va_copy
# define H__MeDefined_VA_COPY
# if defined(__GNUC__)
#  define va_copy  __va_copy
# else/* hope for the best */
#  define va_copy(dst, src)  ((dst) = (src))
# endif
#endif/* va_copy */

#ifndef RAYMAI_PRINTF_MALLOC0
#define RAYMAI_PRINTF_MALLOC0(cb)  calloc(cb, 1)
#endif

#ifndef RAYMAI_PRINTF_MEMFREE
#define RAYMAI_PRINTF_MEMFREE(ptr)  free(ptr)
#endif

#ifndef RAYMAI_PRINTF_CHARTYPE
#define RAYMAI_PRINTF_CHARTYPE  char
#endif

#ifndef RAYMAI_PRINTF_PSTRTYPE
#define RAYMAI_PRINTF_PSTRTYPE  RAYMAI_PRINTF_CHARTYPE *
#endif

#ifndef RAYMAI_PRINTF_PPSTRTYPE
#define RAYMAI_PRINTF_PPSTRTYPE  RAYMAI_PRINTF_CHARTYPE **
#endif

#ifndef RAYMAI_PRINTF_PCSTRTYPE
#define RAYMAI_PRINTF_PCSTRTYPE  RAYMAI_PRINTF_CHARTYPE const *
#endif

#ifndef RAYMAI_PRINTF_LONGLONGTYPE
#define RAYMAI_PRINTF_LONGLONGTYPE  long long
#endif

#ifndef RAYMAI_PRINTF_ULONGLONGTYPE
#define RAYMAI_PRINTF_ULONGLONGTYPE  unsigned long long
#endif

#if defined(RAYMAI_PRINTF_ENABLE_LONGLONG)
# define H__IntMaxType  RAYMAI_PRINTF_LONGLONGTYPE
# define H__UIntMaxType  RAYMAI_PRINTF_ULONGLONGTYPE
# if defined(_WIN64) || defined (__LP64__)
#  ifndef RAYMAI_PRINTF_PTR_IS_LONGLONG
#   define RAYMAI_PRINTF_PTR_IS_LONGLONG
#  endif
# endif
#else
# define H__IntMaxType  long
# define H__UIntMaxType  unsigned long
#endif

#if defined(RAYMAI_PRINTF_ENABLE_ASCII_FMTSTR)
# define H__FmtstrChar  char
# define H__FmtstrPCStr  char const *
#else
# define H__FmtstrChar  RAYMAI_PRINTF_CHARTYPE
# define H__FmtstrPCStr  RAYMAI_PRINTF_PCSTRTYPE
#endif

#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
/*
	mode Count: callback should return strlen only. *pArg is ignored.
	mode Put: callback should return strlen after modifying *pArg.
	mode Free: callback should free *pArg if dynamic allocated.
	return >= 1: callback return strlen of transformed string.
	return == 0: callback want no string output.
	return NoOp: callback no op, want default impl.
	return Abort: callback want to abort printf.
*/
# ifndef RAYMAI_PRINTF_COLONEX_ENUM
# define RAYMAI_PRINTF_COLONEX_ENUM \
	enum { ColonExModeCount, ColonExModePut, ColonExModeFree }; \
	enum { ColonExReturnNoOp = -1, ColonExReturnAbort = -2 }
# endif
/*
	if (type == 'c')
	{ char src = *(char*)pArg;
	  *(char*)pArg = dst; }
	if (type == 's')
	{ char const *src = *(void**)pArg;
	  *(void**)pArg = dst; }
*/
typedef int(*RaymaiPrintf_ColonEx_Handler)(
	H__FmtstrChar type,
	H__FmtstrPCStr pszUsrId,
	int mode, void *pArg, void *pUser);
# ifndef RAYMAI_PRINTF_COLONEX_HANDLER
# error "ColonEx Handler not defined!"
# endif
# ifndef RAYMAI_PRINTF_COLONEX_USRID_MAX
# define RAYMAI_PRINTF_COLONEX_USRID_MAX  (99)
# endif/* USRID_MAX no incl null-termi */
#endif/* RAYMAI_PRINTF_ENABLE_COLONEX */

/*
	When non-standard feature that takes callback is enabled,
	put '^' after '%' to specify `void *pUser` for the callback.

	For example raymai_printf("%:extension_usrid:s", "Test")
	  should be raymai_printf("%^:extension_usrid:s", pUser, "Test").
*/
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
# define H__Feature_Callback
#endif


enum RaymaiPrintf_Err {
	/* Failed to output to buffer or file */
	RaymaiPrintf_Err_Put = INT_MIN,
	/* Unexpected char in format string */
	RaymaiPrintf_Err_Char,
	/* Unexpected end of format string */
	RaymaiPrintf_Err_End,
	/* The type does not support precision modifier */
	RaymaiPrintf_Err_Preci,
	/* The type does not support width modifier */
	RaymaiPrintf_Err_Width,
	/* Unsupported or invalid length sub-specifier */
	RaymaiPrintf_Err_Longg,
	/* The char was not meant to be repeated here */
	RaymaiPrintf_Err_Repeat,
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
	/* User identifier in format string exceed buffer limit */
	/* such as RAYMAI_PRINTF_COLONEX_USRID_MAX */
	RaymaiPrintf_Err_UsrIdBomb,
#endif
#if defined(H__Feature_Callback)
	/* User callback want to abort */
	RaymaiPrintf_Err_UsrAbort,
	/* User callback returned unacceptable value */
	RaymaiPrintf_Err_UsrRetVal,
#endif
	/* Unexpected error */
	RaymaiPrintf_Err_Unexpected
};


#define STROFINT_FUNCNAME  raymai_printf__StrOfInt
#define STROFINT_PSTRTYPE  RAYMAI_PRINTF_PSTRTYPE
#define STROFINT_INTTYPE  H__IntMaxType
#define STROFINT_SIGNED  (1)
#include <raymai/StrOfInt.h>
#undef STROFINT_FUNCNAME
#undef STROFINT_PSTRTYPE
#undef STROFINT_INTTYPE
#undef STROFINT_SIGNED

#define STROFINT_FUNCNAME  raymai_printf__StrOfUInt
#define STROFINT_PSTRTYPE  RAYMAI_PRINTF_PSTRTYPE
#define STROFINT_INTTYPE  H__UIntMaxType
#define STROFINT_SIGNED  (0)
#include <raymai/StrOfInt.h>
#undef STROFINT_FUNCNAME
#undef STROFINT_PSTRTYPE
#undef STROFINT_INTTYPE
#undef STROFINT_SIGNED


static int raymai_snprintf(
	RAYMAI_PRINTF_PSTRTYPE pszBuf,
	size_t nBufMax,
	H__FmtstrPCStr pszFmt, ...);


static int raymai_printf__impl(
	H__FmtstrPCStr const pszFmt,
	va_list ap,
	int(*const pfnPutc)(void *pImpl, RAYMAI_PRINTF_CHARTYPE theChar),
	void * const pImpl)
{
#define H__BadTimingForFlag \
	((flags & (fMetDigit | fMetPreciDot)) || (longg != 0))

#define H__Consume(C) \
	if (pfnPutc) { if (pfnPutc(pImpl, C) < 0) { \
		err = RaymaiPrintf_Err_Put; goto eof; \
	} } ++cch;

#define H__ResetSpec \
	flags = width = preci = longg = 0

	typedef RAYMAI_PRINTF_CHARTYPE rChar;
	typedef RAYMAI_PRINTF_PCSTRTYPE rPCStr;
	typedef H__IntMaxType rIntMax;
	typedef H__UIntMaxType rUIntMax;
#if defined(RAYMAI_PRINTF_ENABLE_LONGLONG)
	typedef RAYMAI_PRINTF_LONGLONGTYPE rLongLong;
	typedef RAYMAI_PRINTF_ULONGLONGTYPE rULongLong;
#endif
	enum {
		fSpecifier = 1 << 0,

		/* ("% d/% d ", 123, -456) --> "  123/-456" */
		fSpaceSign = 1 << 1,

		/* (" %+d/%+d ", 123, -456) --> " +123/-456 " */
		fPositiveSign = 1 << 2,

		/* (" [%-15s][%15s] ", "leftAlign", "rightAlign") */
		fLeftAlign = 1 << 3,

		/* Use '0' to pad width of right-aligned integer */
		fPadWithZero = 1 << 4,

		/* Flags like '+', '-' and ' ' must before width/preci */
		fMetDigit = 1 << 6,

		/* Digit before dot is width */
		/* Digit after dot is preci */
		fMetPreciDot = 1 << 7,

		/* (" %*d ", width, val) */
		fWidthSet = 1 << 8,

		/* (" %.*d ", preci, val) */
		fPreciSet = 1 << 9,

#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
		/* (" %:extension_name:s ", pszStr) */
		fColonEx = 1 << 10,
#endif
		fDummy
	};
	enum {
		/* e.g. %ld */
		lLong = 1 << 0,

		/* e.g. %lld */
		lLongLong = 1 << 1,

		/* e.g. %hu (short) */
		lHalf = 1 << 2,

		/* e.g. %hhu (char) */
		lHalfHalf = 1 << 3,

		/* e.g. %Lf */
		lLongDouble = 1 << 4,

		/* e.g. %zu (sizeof) */
		l_size_t = 1 << 5,

		/* e.g. %td */
		l_ptrdiff_t = 1 << 6,

		/* e.g. %jd */
		l_intmax_t = 1 << 7
	};
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
	RAYMAI_PRINTF_COLONEX_ENUM;
	typedef RaymaiPrintf_ColonEx_Handler  ColonEx_Handler;
	enum {
		ColonEx_UsrId_Max = RAYMAI_PRINTF_COLONEX_USRID_MAX
	};
	ColonEx_Handler const pfnColonEx = RAYMAI_PRINTF_COLONEX_HANDLER;
	H__FmtstrChar szColonEx[ColonEx_UsrId_Max + 1] = { 0 };
#endif
#if defined(H__Feature_Callback)
	void *pUser = NULL;
#endif
	int flags = 0, cch = 0, width = 0, preci = 0, longg = 0, err = 0;
	H__FmtstrPCStr p = pszFmt;
	for (; *p; ++p) switch (*p) {

	case '%':
		if ((flags & fSpecifier) == 0) {
			flags = fSpecifier;
		}
		else { H__ResetSpec; H__Consume(*p); }
		continue;

#if defined(H__Feature_Callback)
	case '^':
		if (flags & fSpecifier) {
			if (pUser) {
				err = RaymaiPrintf_Err_Repeat; goto eof;
			}
			switch (p[1]) {
			default: goto case_default;
			case ':':
				pUser = va_arg(ap, void*);
				++p;
				goto case_ColonEx;
			}
		}
		else { goto case_default; }
		continue;
#endif

#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
	case ':':
	case_ColonEx:
		if (flags & fSpecifier) {
			int i = 0;
			if (flags & fColonEx) {
				err = RaymaiPrintf_Err_Repeat; goto eof;
			}
			switch (p[-1]) {
			default: err = RaymaiPrintf_Err_Char; goto eof;
			case '%': case '^': break;
			}
			for (i = 0; i < ColonEx_UsrId_Max; ++i) {
				H__FmtstrChar const c = p[i + 1];
				if (c == 0) {
					err = RaymaiPrintf_Err_End; goto eof;
				}
				if (c == ':') { szColonEx[i] = 0; break; }
				szColonEx[i] = c;
			}
			if (i >= ColonEx_UsrId_Max) {
				err = RaymaiPrintf_Err_UsrIdBomb; goto eof;
			}
			flags |= fColonEx;
			p += i + 1;
		}
		else { goto case_default; }
		continue;
#endif

	case 'h': /* LENGTH SUB-SPECIFIER */
		if (flags & fSpecifier) {
			if (longg == 0) { longg |= lHalf; }
			else if (longg & lHalf) { longg |= lHalfHalf; }
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
		}
		else { goto case_default; }
		continue;

	case 'l': /* LENGTH SUB-SPECIFIER */
		if (flags & fSpecifier) {
			if (longg == 0) { longg |= lLong; }
#if defined(RAYMAI_PRINTF_ENABLE_LONGLONG)
			else if (longg & lLong) { longg |= lLongLong; }
#endif
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
		}
		else { goto case_default; }
		continue;

	case 'L': /* LENGTH SUB-SPECIFIER */
		if (flags & fSpecifier) {
			if (longg == 0) { longg |= lLongDouble; }
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
		}
		else { goto case_default; }
		continue;

	case 'j': /* LENGTH SUB-SPECIFIER */
		if (flags & fSpecifier) {
			if (longg == 0) { longg |= l_intmax_t; }
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
		}
		else { goto case_default; }
		continue;

	case 't': /* LENGTH SUB-SPECIFIER */
		if (flags & fSpecifier) {
			if (longg == 0) { longg |= l_ptrdiff_t; }
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
		}
		else { goto case_default; }
		continue;

	case 'z': /* LENGTH SUB-SPECIFIER */
		if (flags & fSpecifier) {
			if (longg == 0) { longg |= l_size_t; }
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
		}
		else { goto case_default; }
		continue;

	case 'p': /* pointer */
		if (flags & fSpecifier) {
			enum { MaxSz = 99 };
			void *ptr = NULL;
			H__FmtstrChar szFmt[MaxSz] = { 0 }, *qFmt = NULL;
			rChar szPtr[MaxSz] = { 0 }, *qPtr = NULL;
			char const *r = NULL;
			if (width > 0) {
				err = RaymaiPrintf_Err_Width; goto eof;
			}
			if (preci > 0) {
				err = RaymaiPrintf_Err_Preci; goto eof;
			}
			if (longg != 0) {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
			ptr = va_arg(ap, void*);
#if defined(M_I86) /* Intel 16-bit segmented memory model */
# if defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)
			r = "%.4X";
# else /* memory model that defaults to 32-bit far pointer */
			r = "%.4X:%.4X";
# endif
#else /* Flat memory model */
# if defined(RAYMAI_PRINTF_PTR_IS_LONGLONG)
			r = "0x%.*llX";
# else /* 32-bit */
			r = "0x%.*lX";
# endif
#endif
			for (qFmt = szFmt; *qFmt = *r, *r; ++qFmt, ++r);
			raymai_snprintf(szPtr, MaxSz, szFmt,
#if defined(M_I86) /* 16-bit segmented memory model */
# if defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)
				(unsigned short)(ptr)
# else
				(unsigned short)(((unsigned long)ptr) >> 16),
				(unsigned short)(((unsigned long)ptr) >> 0)
# endif
#else /* Flat memory model */
				(int)(sizeof(ptr) * 2), ptr
#endif
			);
			for (qPtr = szPtr; *qPtr; ++qPtr) {
				H__Consume(*qPtr);
			}
			H__ResetSpec;
		}
		else { goto case_default; }
		continue;

	case 'u': /* unsigned base 10 */
	case 'i': /* signed base 10 */
	case 'd': /* signed base 10 */
	case 'o': /* octal base 8 */
	case 'x': /* lowercase hex base 16 */
	case 'X': /* uppercase hex base 16 */
		if (flags & fSpecifier) {
			int const isSigned = (*p == 'd' || *p == 'i');
			int const radix = (*p == 'o') ? 8 :
				(*p == 'x' || *p == 'X') ? 16 : 10;
			rUIntMax theInt = 0;
			enum { MaxSzInt = 99 };
			rChar szInt[MaxSzInt] = { 0 };
			rChar signChar = 0, *q = NULL;
			int iIntStr = 0, nDigit = 0;
			/* get the int and convert to string */
			if ((longg & ~(lHalfHalf | lHalf)) == 0) {
				theInt = va_arg(ap, unsigned int);
				if (isSigned) {
					theInt = (int)theInt;
				}
				if (longg & lHalfHalf) {
					if (isSigned) { theInt = (signed char)theInt; }
					else { theInt = (unsigned char)theInt; }
				}
				else if (longg & lHalf) {
					if (isSigned) { theInt = (short)theInt; }
					else { theInt = (unsigned short)theInt; }
				}
			}
#if defined(RAYMAI_PRINTF_ENABLE_LONGLONG)
			else if (longg & lLongLong) {
				theInt = va_arg(ap, rULongLong);
				if (isSigned) {
					theInt = (rLongLong)theInt;
				}
			}
#endif
			else if (longg & lLong) {
				theInt = va_arg(ap, unsigned long);
				if (isSigned) {
					theInt = (long)theInt;
				}
			}
			else if (longg & l_size_t) {
				theInt = va_arg(ap, size_t);
			}
			else if (longg & l_ptrdiff_t) {
				theInt = va_arg(ap, ptrdiff_t);
			}
			else if (longg & l_intmax_t) {
				theInt = va_arg(ap, rUIntMax);
			}
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
			iIntStr = isSigned
				? raymai_printf__StrOfInt(szInt, MaxSzInt - 1, theInt, radix)
				: raymai_printf__StrOfUInt(szInt, MaxSzInt - 1, theInt, radix);
			if (iIntStr < 0) {
				err = RaymaiPrintf_Err_Unexpected; goto eof;
			}
			/* for signed type */
			if (isSigned) {
				if (szInt[0] == '-') {
					signChar = '-';
				}
				else if (flags & fPositiveSign) {
					signChar = '+';
				}
				else if (flags & fSpaceSign) {
					signChar = ' ';
				}
			}
			/* for lowercase hex */
			else if (*p == 'x') for (q = szInt; *q; ++q) {
				if (*q >= 'A' && *q <= 'Z') { *q += 'a' - 'A'; }
			}
			/* find number of digit */
			if (theInt == 0 && (flags & fMetPreciDot) && preci == 0) {
				nDigit = 0;
			}
			else {
				for (nDigit = 0; szInt[nDigit]; ++nDigit);
				if (szInt[0] == '-') { --nDigit; }
			}
			/* width = mininum strlen */
			width -= ((preci > nDigit) ? preci : nDigit) + (signChar ? 1 : 0);
			if ((flags & fLeftAlign) == 0) {
				if ((flags & fPadWithZero) && (flags & fMetPreciDot) == 0) {
					if (signChar) { H__Consume(signChar); signChar = 0; }
					while (width-- > 0) { H__Consume('0'); }
				}
				else {
					while (width-- > 0) { H__Consume(' '); }
				}
			}
			if (signChar) { H__Consume(signChar); }
			/* preci = minimum number of digit */
			preci -= nDigit;
			while (preci-- > 0) { H__Consume('0'); }
			if (nDigit > 0) {
				q = (szInt[0] == '-') ? &szInt[1] : szInt;
				for (; *q; ++q) { H__Consume(*q); }
			}
			while (width-- > 0) { H__Consume(' '); }
			H__ResetSpec;
		}
		else { goto case_default; }
		continue;

	case 'n': /* number of char written so far */
		if (flags & fSpecifier) {
			if (longg == 0) {
				*(va_arg(ap, int*)) = cch;
			}
			else if (longg & lLong) {
				*(va_arg(ap, long*)) = cch;
			}
#if defined(RAYMAI_PRINTF_ENABLE_LONGLONG)
			else if (longg & lLongLong) {
				*(va_arg(ap, rLongLong*)) = cch;
			}
#endif
			else if (longg & lHalf) {
				*(va_arg(ap, short*)) = (short)cch;
			}
			else if (longg & lHalfHalf) {
				*(va_arg(ap, signed char*)) = (signed char)cch;
			}
			else if (longg & l_size_t) {
				*(va_arg(ap, size_t*)) = cch;
			}
			else if (longg & l_intmax_t) {
				*(va_arg(ap, rIntMax*)) = cch;
			}
			else if (longg & l_ptrdiff_t) {
				*(va_arg(ap, ptrdiff_t*)) = cch;
			}
			else {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
			H__ResetSpec;
		}
		else { goto case_default; }
		continue;

	case 'c': /* char */
		if (flags & fSpecifier) {
			rChar chrArg = 0;
			if (width != 0) {
				err = RaymaiPrintf_Err_Width; goto eof;
			}
			if (flags & fMetPreciDot) {
				err = RaymaiPrintf_Err_Preci; goto eof;
			}
			if (longg != 0) {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
			/* va_arg(int) not (rChar) becuz default argument promotions. */
			chrArg = (rChar)va_arg(ap, int);
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
			if (flags & fColonEx) {
				int const mode = pfnPutc ? ColonExModePut : ColonExModeCount;
				rChar chrarg = chrArg;
				switch (pfnColonEx(*p, szColonEx, mode, &chrarg, pUser)) {
				case ColonExReturnAbort: err = RaymaiPrintf_Err_UsrAbort; goto eof;
				case ColonExReturnNoOp: break;
				case 0: H__ResetSpec; continue;
				case 1: chrArg = chrarg; break;
				default: err = RaymaiPrintf_Err_UsrRetVal; goto eof;
				}
			}
#endif
			H__Consume(chrArg);
			H__ResetSpec;
		}
		else { goto case_default; }
		continue;

	case 's': /* null-terminated string */
		if (flags & fSpecifier) {
			rPCStr pszArg = NULL;
			int i = 0, n = 0, already_strlen = 0;
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
			void *pColonExUsrRet = NULL;
#endif
			if (longg != 0) {
				err = RaymaiPrintf_Err_Longg; goto eof;
			}
			pszArg = va_arg(ap, rPCStr);
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
			if (flags & fColonEx) {
				int const mode = pfnPutc ? ColonExModePut : ColonExModeCount;
				rPCStr pszarg = pszArg;
				switch (n = pfnColonEx(*p, szColonEx, mode, (void*)&pszarg, pUser)) {
				case ColonExReturnAbort: err = RaymaiPrintf_Err_UsrAbort; goto eof;
				case ColonExReturnNoOp: break;
				case 0: H__ResetSpec; continue;
				default:
					if (n < 0) {
						err = RaymaiPrintf_Err_UsrRetVal; goto eof;
					}
					if (mode == ColonExModePut) {
						pszArg = pColonExUsrRet = (void*)pszarg;
					}
					already_strlen = 1;
				}
			}
#endif
			if (!pszArg) {
				static rChar szNull[9];
				rChar *q = szNull;
				char const *r = "(null)";
				for (; *q = *r, *r; ++q, ++r);
				pszArg = szNull;
				already_strlen = 0;
			}
			if (!already_strlen) {
				for (n = 0; pszArg[n]; ++n);
			}
			if ((width > 0) || (flags & fMetPreciDot)) {
				/* preci = maximum strlen */
				if ((flags & fMetPreciDot) && (n > preci)) { n = preci; }
				/* width = mininum strlen */
				width -= n;
				if ((flags & fLeftAlign) == 0) {
					while (width-- > 0) { H__Consume(' '); }
				}
				for (i = 0; i < n; ++i) { H__Consume(pszArg[i]); }
				while (width-- > 0) { H__Consume(' '); }
			}
			else {
				for (i = 0; i < n; ++i) { H__Consume(pszArg[i]); }
			}
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
			if (pColonExUsrRet) {
				pfnColonEx(*p, szColonEx, ColonExModeFree, &pColonExUsrRet, pUser);
			}
#endif
			H__ResetSpec;
		}
		else { goto case_default; }
		continue;

	case '-': /* FLAG */
		if (flags & fSpecifier) {
			if (H__BadTimingForFlag) {
				err = RaymaiPrintf_Err_Char; goto eof;
			}
			if (flags & fLeftAlign) {
				err = RaymaiPrintf_Err_Repeat; goto eof;
			}
			flags |= fLeftAlign;
		}
		else { goto case_default; }
		continue;

	case '+': /* FLAG */
		if (flags & fSpecifier) {
			if (H__BadTimingForFlag) {
				err = RaymaiPrintf_Err_Char; goto eof;
			}
			if (flags & fPositiveSign) {
				err = RaymaiPrintf_Err_Repeat; goto eof;
			}
			flags |= fPositiveSign;
		}
		else { goto case_default; }
		continue;

	case ' ': /* FLAG */
		if (flags & fSpecifier) {
			if (H__BadTimingForFlag) {
				err = RaymaiPrintf_Err_Char; goto eof;
			}
			if (flags & fSpaceSign) {
				err = RaymaiPrintf_Err_Repeat; goto eof;
			}
			flags |= fSpaceSign;
		}
		else { goto case_default; }
		continue;

	case '0': /* FLAG or WIDTH or PRECI */
		if (flags & fSpecifier) {
			if (flags & (fMetDigit | fMetPreciDot)) goto case_digit_0;
			if (longg != 0) {
				err = RaymaiPrintf_Err_Char; goto eof;
			}
			if (flags & fPadWithZero) {
				err = RaymaiPrintf_Err_Repeat; goto eof;
			}
			flags |= fPadWithZero;
		}
		else { goto case_default; }
		continue;

	case '.': /* PRECI start point */
		if (flags & fSpecifier) {
			if (longg != 0) {
				err = RaymaiPrintf_Err_Char; goto eof;
			}
			if (flags & fMetPreciDot) {
				err = RaymaiPrintf_Err_Repeat; goto eof;
			}
			flags |= fMetPreciDot;
		}
		else { goto case_default; }
		continue;

	case '*': /* dynamic WIDTH or PRECI */
		if (flags & fSpecifier) {
			if (longg != 0) {
				err = RaymaiPrintf_Err_Char; goto eof;
			}
			if (flags & fMetPreciDot) {
				if (flags & fPreciSet) {
					err = RaymaiPrintf_Err_Repeat; goto eof;
				}
				flags |= fPreciSet;
				preci = va_arg(ap, int);
			}
			else {
				if (flags & fWidthSet) {
					err = RaymaiPrintf_Err_Repeat; goto eof;
				}
				flags |= fWidthSet;
				width = va_arg(ap, int);
			}
		}
		else { goto case_default; }
		continue;

	case_digit_0: /* WIDTH or PRECI */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (flags & fSpecifier) {
			if (longg != 0) {
				err = RaymaiPrintf_Err_Char; goto eof;
			}
			flags |= fMetDigit;
			if (flags & fMetPreciDot) {
				preci = preci * 10 + (*p - '0');
			}
			else {
				width = width * 10 + (*p - '0');
			}
		}
		else { goto case_default; }
		continue;

	case_default:
	default:
		if (flags & fSpecifier) {
			err = RaymaiPrintf_Err_Char; goto eof;
		}
		H__Consume(*p);
	}
	if (flags & fSpecifier) {
		err = RaymaiPrintf_Err_End; goto eof;
	}
eof:
	return err ? err : cch;

#undef H__BadTimingForFlag
#undef H__Consume
#undef H__ResetSpec
}

static int raymai_printf__put(
	H__FmtstrPCStr pszFmt,
	va_list ap,
	int(*pfnPutc)(void *pImpl, RAYMAI_PRINTF_CHARTYPE theChar),
	void *pImpl)
{
	return raymai_printf__impl(pszFmt, ap, pfnPutc, pImpl);
}

static int raymai_printf__cch(
	H__FmtstrPCStr pszFmt,
	va_list ap)
{
	return raymai_printf__impl(pszFmt, ap, NULL, NULL);
}


#define H__Self_fprintf  struct { \
	FILE *pFile; }

static int raymai_fprintf__putc(
	void *pImpl, RAYMAI_PRINTF_CHARTYPE theChar)
{
	H__Self_fprintf *pSelf = pImpl;
	return fputc(theChar, pSelf->pFile) == EOF ? -1 : 1;
}

/* return number of written CHAR */
static int raymai_vfprintf(
	FILE *pFile,
	H__FmtstrPCStr pszFmt,
	va_list ap)
{
	H__Self_fprintf self = { 0 };
	self.pFile = pFile;
	return raymai_printf__put(pszFmt, ap,
		raymai_fprintf__putc, &self);
}

/* return number of written CHAR */
static int raymai_fprintf(
	FILE *pFile,
	H__FmtstrPCStr pszFmt, ...)
{
	va_list ap; int ret;
	va_start(ap, pszFmt);
	ret = raymai_vfprintf(pFile, pszFmt, ap);
	va_end(ap);
	return ret;
}

static int raymai_printf(
	H__FmtstrPCStr pszFmt, ...)
{
	va_list ap; int ret;
	va_start(ap, pszFmt);
	ret = raymai_vfprintf(stdout, pszFmt, ap);
	va_end(ap);
	return ret;
}


#define H__Self_snprintf  struct { \
	RAYMAI_PRINTF_PSTRTYPE pszBuf;  \
	size_t nBufMax, iBuf; }

static int raymai_snprintf__putc(
	void *pImpl, RAYMAI_PRINTF_CHARTYPE theChar)
{
	H__Self_snprintf *pSelf = pImpl;
	if (pSelf->iBuf < pSelf->nBufMax) {
		pSelf->pszBuf[pSelf->iBuf++] = theChar;
		return 1;
	}
	return -1;
}

/* return number of CHAR of formatted string (NOT written) */
static int raymai_vsnprintf(
	RAYMAI_PRINTF_PSTRTYPE pszBuf,
	size_t nBufMax,
	H__FmtstrPCStr pszFmt,
	va_list ap)
{
	int cch; va_list ap2;
	va_copy(ap2, ap);
	cch = raymai_printf__cch(pszFmt, ap);
	if (cch >= 0 && pszBuf && nBufMax > 0) {
		H__Self_snprintf self = { 0 };
		self.pszBuf = pszBuf;
		self.nBufMax = nBufMax;
		raymai_printf__put(pszFmt, ap2,
			raymai_snprintf__putc, &self);
		if (self.iBuf < self.nBufMax) {
			self.pszBuf[self.iBuf] = '\0';
		}
		else {
			self.pszBuf[self.nBufMax - 1] = '\0';
		}
	}
	va_end(ap2);
	return cch;
}

/* return number of CHAR of formatted string (NOT written) */
static int raymai_snprintf(
	RAYMAI_PRINTF_PSTRTYPE pszBuf,
	size_t nBufMax,
	H__FmtstrPCStr pszFmt, ...)
{
	va_list ap; int cch;
	va_start(ap, pszFmt);
	cch = raymai_vsnprintf(pszBuf, nBufMax, pszFmt, ap);
	va_end(ap);
	return cch;
}


#define H__Self_asprintf  struct { \
	RAYMAI_PRINTF_PSTRTYPE pszOut; \
	size_t iOut; }

static int raymai_asprintf__putc(
	void *pImpl, RAYMAI_PRINTF_CHARTYPE theChar)
{
	H__Self_asprintf *pSelf = pImpl;
	pSelf->pszOut[pSelf->iOut++] = theChar;
	return 1;
}

/* return number of CHAR of allocated formatted string */
static int raymai_vasprintf(
	RAYMAI_PRINTF_PPSTRTYPE ppszOut,
	H__FmtstrPCStr pszFmt,
	va_list ap)
{
	typedef RAYMAI_PRINTF_CHARTYPE rChar;
	typedef RAYMAI_PRINTF_PSTRTYPE rPStr;
	int cch; va_list ap2;
	va_copy(ap2, ap);
	cch = raymai_printf__cch(pszFmt, ap);
	if (cch >= 0) {
		rPStr pszOut = RAYMAI_PRINTF_MALLOC0((cch + 1) * sizeof(rChar));
		if (pszOut) {
			H__Self_asprintf self = { 0 };
			self.pszOut = pszOut;
			cch = raymai_printf__put(pszFmt, ap2,
				raymai_asprintf__putc, &self);
			if (cch >= 0) {
				*ppszOut = pszOut;
			}
		}
	}
	va_end(ap2);
	return cch;
}

/* return number of CHAR of allocated formatted string */
static int raymai_asprintf(
	RAYMAI_PRINTF_PPSTRTYPE ppszOut,
	H__FmtstrPCStr pszFmt, ...)
{
	va_list ap; int ret;
	va_start(ap, pszFmt);
	ret = raymai_vasprintf(ppszOut, pszFmt, ap);
	va_end(ap);
	return ret;
}

#if defined(H__MeDefined_VA_COPY)
# undef va_copy
#endif
#undef H__IntMaxType
#undef H__UIntMaxType
#undef H__FmtstrChar 
#undef H__FmtstrPCStr
#undef H__Feature_Callback
#undef H__Self_fprintf
#undef H__Self_snprintf
#undef H__Self_asprintf
