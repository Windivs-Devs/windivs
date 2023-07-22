/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         Windivs crt library
 * FILE:            lib/sdk/crt/printf/fprintf_s.c
 * PURPOSE:         Implementation of fprintf_s
 * PROGRAMMER:      Samuel Serapi�n
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
fprintf_s(FILE* file, const char *format, ...)
{
    va_list argptr;
    int result;
    va_start(argptr, format);
    result = vfprintf_s(file, format, argptr);
    va_end(argptr);
    return result;
}
