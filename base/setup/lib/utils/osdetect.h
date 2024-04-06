/*
 * PROJECT:     Windivs Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NT 5.x family (MS Windows <= 2003, and Windivs)
 *              operating systems detection code.
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

#pragma once

/* Language-independent Vendor strings */
#define VENDOR_REACTOS      L"Windivs"
#define VENDOR_MICROSOFT    L"Microsoft"

typedef struct _NTOS_INSTALLATION
{
    LIST_ENTRY ListEntry;
// BOOLEAN IsDefault;   // TRUE / FALSE whether this installation is marked as "default" in its corresponding loader configuration file.
    USHORT Machine;                 // Target architecture of the NTOS installation
    UNICODE_STRING SystemArcPath;   // Normalized ARC path ("ArcSystemRoot")
    UNICODE_STRING SystemNtPath;    // Corresponding NT path ("NtSystemRoot")
    PCWSTR PathComponent;           // Pointer inside SystemNtPath.Buffer
    ULONG DiskNumber;
    ULONG PartitionNumber;
    PPARTENTRY PartEntry;
    WCHAR InstallationName[MAX_PATH];
    WCHAR VendorName[MAX_PATH];
    // CHAR Data[ANYSIZE_ARRAY];
} NTOS_INSTALLATION, *PNTOS_INSTALLATION;

// EnumerateNTOSInstallations
PGENERIC_LIST
CreateNTOSInstallationsList(
    IN PPARTLIST List);

/*
 * FindSubStrI(PCWSTR str, PCWSTR strSearch) :
 *    Searches for a sub-string 'strSearch' inside 'str', similarly to what
 *    wcsstr(str, strSearch) does, but ignores the case during the comparisons.
 */
PCWSTR FindSubStrI(PCWSTR str, PCWSTR strSearch);

/* EOF */
