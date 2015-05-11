#include "D3Types.h"

#ifdef _MSC_VER 
#include <windows.h>
  // Microsoft had to implement the secure version of sprintf in their
  // own strange way... 
  // This snprintf works like the unix version.
  int snprintf(char* pszBuffer, size_t size, const char* pszFormat, ...)
  {
    size_t    nCount;
    va_list   pArgs;

    va_start(pArgs, pszFormat);
    nCount = _vscprintf(pszFormat, pArgs);
    _vsnprintf_s(pszBuffer, size, _TRUNCATE, pszFormat, pArgs);
    va_end(pArgs);

    return nCount;
  }

	// Call Sleep
  void sleep(long millisecs)
  {
		Sleep(millisecs);
  }
#endif


