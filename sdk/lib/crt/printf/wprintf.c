/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         Windivs crt library
 * FILE:            lib/sdk/crt/printf/wprintf.c
 * PURPOSE:         Implementation of wprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapi�n
 */

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
wprintf(const wchar_t *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = vwprintf(format, argptr);
    va_end(argptr);
    return result;
}
