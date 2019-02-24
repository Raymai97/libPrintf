/* #include <raymai/printf.h> before this */

static char const * RaymaiPrintf_ErrText(int err)
{
	return
		err == RaymaiPrintf_Err_Put ?
		"Failed to output to buffer or file" :
		err == RaymaiPrintf_Err_Char ?
		"Unexpected char in format string" :
		err == RaymaiPrintf_Err_End ?
		"Unexpected end of format string" :
		err == RaymaiPrintf_Err_Preci ?
		"The type does not support precision modifier" :
		err == RaymaiPrintf_Err_Width ?
		"The type does not support width modifier" :
		err == RaymaiPrintf_Err_Longg ?
		"Unsupported or invalid length sub-specifier" :
		err == RaymaiPrintf_Err_Repeat ?
		"The char was not meant to be repeated here" :
#if defined(RAYMAI_PRINTF_ENABLE_COLONEX)
		err == RaymaiPrintf_Err_UsrIdBomb ?
		"User identifier in format string exceed buffer limit "
		"such as RAYMAI_PRINTF_COLONEX_USRID_MAX" :
#endif
#if defined(H__Feature_Callback)
		err == RaymaiPrintf_Err_UsrAbort ?
		"User callback want to abort" :
		err == RaymaiPrintf_Err_UsrRetVal ?
		"User callback returned unacceptable value" :
#endif
		RaymaiPrintf_Err_Unexpected ?
		"Unexpected error" :
		NULL;
}
