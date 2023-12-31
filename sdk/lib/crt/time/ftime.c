/*
 * COPYRIGHT:   LGPL, See LGPL.txt in the top level directory
 * PROJECT:     Windivs CRT library
 * FILE:        lib/sdk/crt/time/ftime.c
 * PURPOSE:     Deprecated BSD library call
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>
#include <sys/timeb.h>
#include "bitsfixup.h"

/******************************************************************************
 * \name _ftime_s
 * \brief Get the current time.
 * \param [out] ptimeb Pointer to a structure of type struct _timeb that
 *        receives the current time.
 * \sa http://msdn.microsoft.com/en-us/library/95e68951.aspx
 */
errno_t
CDECL
_ftime_s(struct _timeb *ptimeb)
{
    DWORD ret;
    TIME_ZONE_INFORMATION TimeZoneInformation;
    FILETIME SystemTime;

    /* Validate parameters */
    if (!MSVCRT_CHECK_PMT( ptimeb != NULL ))
    {
        *_errno() = EINVAL;
        return EINVAL;
    }

    ret = GetTimeZoneInformation(&TimeZoneInformation);
    ptimeb->dstflag = (ret == TIME_ZONE_ID_DAYLIGHT) ? 1 : 0;
    ptimeb->timezone = (short)TimeZoneInformation.Bias;

    GetSystemTimeAsFileTime(&SystemTime);
    ptimeb->time = (time_t)FileTimeToUnixTime(&SystemTime, &ptimeb->millitm);

    return 0;
}

/******************************************************************************
 * \name _ftime
 * \brief Get the current time.
 * \param [out] ptimeb Pointer to a structure of type struct _timeb that
 *        receives the current time.
 * \note This function is for compatability and simply calls the secure
 *       version _ftime_s().
 * \sa http://msdn.microsoft.com/en-us/library/z54t9z5f.aspx
 */
void
CDECL
_ftime(struct _timeb *ptimeb)
{
    _ftime_s(ptimeb);
}
