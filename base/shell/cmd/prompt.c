/*
 *  PROMPT.C - prompt handling.
 *
 *
 *  History:
 *
 *    14/01/95 (Tim Normal)
 *        started.
 *
 *    08/08/95 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *    01/06/96 (Tim Norman)
 *        added day of the week printing (oops, forgot about that!)
 *
 *    08/07/96 (Steffan Kaiser)
 *        small changes for speed
 *
 *    20-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        removed redundant day strings. Use ones defined in date.c.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    28-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        moved cmd_prompt from internal.c to here
 *
 *    09-Dec-1998 (Eric Kohl)
 *        Added help text ("/?").
 *
 *    14-Dec-1998 (Eric Kohl)
 *        Added "$+" option.
 *
 *    09-Jan-1999 (Eric Kohl)
 *        Added "$A", "$C" and "$F" option.
 *        Added locale support.
 *        Fixed "$V" option.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    24-Jan-1999 (Eric Kohl)
 *        Fixed Win32 environment handling.
 *
 *    30-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */
#include "precomp.h"

/* The default prompt */
static TCHAR DefaultPrompt[] = _T("$P$G");

/*
 * Initialize prompt support.
 */
VOID InitPrompt(VOID)
{
    TCHAR Buffer[2];

    /*
     * Set the PROMPT environment variable if it doesn't exist already.
     * You can change the PROMPT environment variable before cmd starts.
     */
    if (GetEnvironmentVariable(_T("PROMPT"), Buffer, _countof(Buffer)) == 0)
        SetEnvironmentVariable(_T("PROMPT"), DefaultPrompt);
}

/*
 * Print the command-line prompt.
 */
VOID PrintPrompt(VOID)
{
    LPTSTR pr, Prompt;
    TCHAR szPrompt[256];
    TCHAR szPath[MAX_PATH];

    if (GetEnvironmentVariable(_T("PROMPT"), szPrompt, _countof(szPrompt)))
        Prompt = szPrompt;
    else
        Prompt = DefaultPrompt;

    /* Parse the prompt string */
    for (pr = Prompt; *pr; ++pr)
    {
        if (*pr != _T('$'))
        {
            ConOutChar(*pr);
        }
        else
        {
            ++pr;
            if (!*pr) break;
            switch (_totupper(*pr))
            {
                case _T('A'):
                    ConOutChar(_T('&'));
                    break;

                case _T('B'):
                    ConOutChar(_T('|'));
                    break;

                case _T('C'):
                    ConOutChar(_T('('));
                    break;

                case _T('D'):
                    ConOutPrintf(_T("%s"), GetDateString());
                    break;

                case _T('E'):
                    ConOutChar(_T('\x1B'));
                    break;

                case _T('F'):
                    ConOutChar(_T(')'));
                    break;

                case _T('G'):
                    ConOutChar(_T('>'));
                    break;

                case _T('H'):
                    ConOutPuts(_T("\x08 \x08"));
                    break;

                case _T('L'):
                    ConOutChar(_T('<'));
                    break;

                case _T('N'):
                {
                    GetCurrentDirectory(_countof(szPath), szPath);
                    ConOutChar(szPath[0]);
                    break;
                }

                case _T('P'):
                {
                    GetCurrentDirectory(_countof(szPath), szPath);
                    ConOutPrintf(_T("%s"), szPath);
                    break;
                }

                case _T('Q'):
                    ConOutChar(_T('='));
                    break;

                case _T('S'):
                    ConOutChar(_T(' '));
                    break;

                case _T('T'):
                    ConOutPrintf(_T("%s"), GetTimeString());
                    break;

                case _T('V'):
                    PrintOSVersion();
                    break;

                case _T('_'):
                    ConOutChar(_T('\n'));
                    break;

                case _T('$'):
                    ConOutChar(_T('$'));
                    break;

#ifdef FEATURE_DIRECTORY_STACK
                case _T('+'):
                {
                    INT i;
                    for (i = 0; i < GetDirectoryStackDepth(); i++)
                        ConOutChar(_T('+'));
                    break;
                }
#endif
            }
        }
    }
}


#ifdef INCLUDE_CMD_PROMPT

INT cmd_prompt(LPTSTR param)
{
    INT retval = 0;

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_PROMPT_HELP1);
#ifdef FEATURE_DIRECTORY_STACK
        ConOutResPaging(FALSE, STRING_PROMPT_HELP2);
#endif
        ConOutResPaging(FALSE, STRING_PROMPT_HELP3);
        return 0;
    }

    /*
     * Set the PROMPT environment variable. If 'param' is NULL or is
     * an empty string (the user entered "prompt" only), then remove
     * the environment variable and therefore use the default prompt.
     * Otherwise, use the new prompt.
     */
    if (!SetEnvironmentVariable(_T("PROMPT"),
                                (param && param[0] != _T('\0') ? param : NULL)))
    {
        retval = 1;
    }

    if (BatType != CMD_TYPE)
    {
        if (retval != 0)
            nErrorLevel = retval;
    }
    else
    {
        nErrorLevel = retval;
    }

    return retval;
}
#endif

/* EOF */
