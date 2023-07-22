/*
 * PROJECT:         Windivs api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for PrivateExtractIcons
 * PROGRAMMER:      Hermes Belusca-Maito
 *                  Doug Lyons <douglyons@douglyons.com>
 */

#include "precomp.h"
#include <stdio.h>

BOOL FileExists(PCWSTR FileName)
{
    DWORD Attribute = GetFileAttributesW(FileName);

    return (Attribute != INVALID_FILE_ATTRIBUTES &&
            !(Attribute & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL ResourceToFile(INT i, PCWSTR FileName)
{
    FILE *fout;
    HGLOBAL hData;
    HRSRC hRes;
    PVOID pResLock;
    UINT iSize;

    if (FileExists(FileName))
    {
        skip("'%S' already exists. Exiting now\n", FileName);
        return FALSE;
    }

    hRes = FindResourceW(NULL, MAKEINTRESOURCEW(i), MAKEINTRESOURCEW(RT_RCDATA));
    if (hRes == NULL)
    {
        skip("Could not locate resource (%d). Exiting now\n", i);
        return FALSE;
    }

    iSize = SizeofResource(NULL, hRes);

    hData = LoadResource(NULL, hRes);
    if (hData == NULL)
    {
        skip("Could not load resource (%d). Exiting now\n", i);
        return FALSE;
    }

    // Lock the resource into global memory.
    pResLock = LockResource(hData);
    if (pResLock == NULL)
    {
        skip("Could not lock resource (%d). Exiting now\n", i);
        return FALSE;
    }

    fout = _wfopen(FileName, L"wb");
    fwrite(pResLock, iSize, 1, fout);
    fclose(fout);
    return TRUE;
}

static struct
{
    PCWSTR FilePath;
    UINT cIcons;        // Return value of the first icon group extracted (should be 1 if no error)
    UINT cTotalIcons;   // Return value of total icon groups in file
    BOOL bhIconValid;   // Whether or not the returned icon handle is not NULL.
} IconTests[] =
{
    /* Executables with just one icon group */
    {L"notepad.exe", 1, 1, TRUE},
    {L"%SystemRoot%\\System32\\cmd.exe", 1, 1, TRUE},

    /* Executable without icon groups */
    {L"%SystemRoot%\\System32\\autochk.exe", 0, 0, FALSE},

    /* Existing file (shell32 has 233 icon groups in Windivs only) */
    {L"%SystemRoot%\\System32\\shell32.dll", 1, 233, TRUE},

    /* Non-existing files */
    {L"%SystemRoot%\\non-existent-file.sdf", 0xFFFFFFFF, 0, FALSE},

    /* Executable with 18 icon groups */
    {L"%SystemRoot%\\explorer.exe", 1, 18, TRUE},

    /* Icon group file containing 6 icons */
    {L"%temp%\\sysicon.ico", 1, 1, TRUE},

    /* Icon group file containing one PNG icon and one normal icon */
    {L"%temp%\\ROS.ico", 1, 1, TRUE},
};

static struct
{
    PCWSTR FileName;
    INT ResourceId;
} IconFiles[] =
{
    {L"%temp%\\ROS.ico", IDR_ICONS_PNG},
    {L"%temp%\\sysicon.ico", IDR_ICONS_NORMAL},
};

START_TEST(PrivateExtractIcons)
{
    HICON ahIcon;
    UINT i, aIconId, cIcons, cIcoTotal;
    WCHAR PathBuffer[MAX_PATH];

    /* Extract icons */
    for (i = 0; i < _countof(IconFiles); ++i)
    {
        ExpandEnvironmentStringsW(IconFiles[i].FileName, PathBuffer, _countof(PathBuffer));

        if (!ResourceToFile(IconFiles[i].ResourceId, PathBuffer))
            goto Cleanup;
    }

    for (i = 0; i < _countof(IconTests); ++i)
    {
        /* Get total number of icon groups in file.
         * None of the hard numbers in the function matter since we have
         * two NULLs for the Icon Handle and Count to be set. */
        cIcoTotal = PrivateExtractIconsW(IconTests[i].FilePath, 0, 16, 16, NULL, NULL, 0, 0);
        ok((i == 3 ?
              cIcoTotal > 232 && cIcoTotal < 240 :    /* shell32 case: ROS has 233, W2K2SP2 has 239 icon groups. */
              cIcoTotal == IconTests[i].cTotalIcons),
           "PrivateExtractIconsW(%u): "
           "got %u, expected %u\n", i, cIcoTotal, IconTests[i].cTotalIcons);

        /* Always test extraction of the FIRST icon (index 0) */
        ahIcon = (HICON)UlongToHandle(0xdeadbeef);
        aIconId = 0xdeadbeef;
        cIcons = PrivateExtractIconsW(IconTests[i].FilePath, 0, 16, 16, &ahIcon, &aIconId, 1, 0);
        ok(cIcons == IconTests[i].cIcons, "PrivateExtractIconsW(%u): got %u, expected %u\n", i, cIcons, IconTests[i].cIcons);
        ok(ahIcon != (HICON)UlongToHandle(0xdeadbeef), "PrivateExtractIconsW(%u): icon not set\n", i);
        ok((IconTests[i].bhIconValid && ahIcon) || (!IconTests[i].bhIconValid && !ahIcon),
            "PrivateExtractIconsW(%u): icon expected to be %s, but got 0x%p\n",
            i, IconTests[i].bhIconValid ? "valid" : "not valid", ahIcon);
        if (cIcons == 0xFFFFFFFF)
        {
            ok(aIconId == 0xdeadbeef,
               "PrivateExtractIconsW(%u): id should not be set to 0x%x\n",
               i, aIconId);
        }
        else
        {
            ok(aIconId != 0xdeadbeef, "PrivateExtractIconsW(%u): id not set\n", i);
        }
        if (ahIcon && ahIcon != (HICON)UlongToHandle(0xdeadbeef))
            DestroyIcon(ahIcon);
    }

Cleanup:
    for (i = 0; i < _countof(IconFiles); ++i)
    {
        ExpandEnvironmentStringsW(IconFiles[i].FileName, PathBuffer, _countof(PathBuffer));
        DeleteFileW(PathBuffer);
    }
}
