
#ifndef H_PriType
#define H_PriType  "d"
#endif
#ifndef H_IntType
#define H_IntType  int
#endif
#ifndef H_FunName
#define H_FunName  my_test_int_d
#endif

static void H_FunName(void)
{
	typedef H_IntType  T;
	enum { nBit = sizeof(T) * CHAR_BIT };
	char const * const pFmtSimple = "%%%s" H_PriType;
	char const * const pFmtWidth = "%%%s" "%d" H_PriType;
	char const * const pFmtPreci = "%%%s" ".%d" H_PriType;
	char const * const pFmtWidthPreci = "%%%s" "%d" ".%d" H_PriType;
	int i, j, k; char szFmt[99];

	for (i = 0; i < ARR_SIZE(s_arrSignedIntFlag); ++i) {
		char const *pszFlag = s_arrSignedIntFlag[i];

		for (j = 0; j < nBit; ++j) {
			T const dPos = INT_MAX / ((T)1 << (nBit - 1 - j));
			T const dNeg = INT_MIN / ((T)1 << (nBit - 1 - j));

			sprintf(szFmt, pFmtSimple, pszFlag);
			my_printf(szFmt, dPos);
			my_printf(szFmt, dNeg);

			for (k = 0; k < nBit; ++k) {
				int const width = k + 1;
				int const preci = k;
				/* Explicit width of zero is invalid because */
				/* '0' refers to fPadWithZero flag. */
				assert(width != 0);

				sprintf(szFmt, pFmtWidth, pszFlag, width);
				my_printf(szFmt, dPos);
				my_printf(szFmt, dNeg);

				sprintf(szFmt, pFmtPreci, pszFlag, preci);
				my_printf(szFmt, dPos);
				my_printf(szFmt, dNeg);

				sprintf(szFmt, pFmtWidthPreci, pszFlag, width, preci);
				my_printf(szFmt, dPos);
				my_printf(szFmt, dNeg);

				sprintf(szFmt, pFmtWidthPreci, pszFlag, width, preci + 2);
				my_printf(szFmt, dPos);
				my_printf(szFmt, dNeg);
			}
		}
	}
}

#undef H_PriType
#undef H_IntType
#undef H_FunName
