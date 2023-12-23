/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         Windivs crt library
 * FILE:            lib/sdk/crt/printf/_cwprintf.c
 * PURPOSE:         Implementation of _cwprintf
 * PROGRAMMER:      Samuel Serapi�n
 */

#include <conio.h>
#include <stdarg.h>

int
__cdecl
_cwprintf(const wchar_t* format, ...)
{
  int retval;
  va_list valist;

  va_start( valist, format );
  retval = _vcwprintf(format, valist);
  va_end(valist);

  return retval;
}
