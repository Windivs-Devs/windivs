/*
 * PROJECT:     Windivs Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NT 5.x family (MS Windows <= 2003, and Windivs)
 *              operating systems detection code.
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "ntverrsrc.h"
// #include "arcname.h"
#include "bldrsup.h"
#include "filesup.h"
#include "genlist.h"
#include "partlist.h"
#include "arcname.h"
#include "osdetect.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

/* Language-independent Vendor strings */
static const PCWSTR KnownVendors[] = { VENDOR_REACTOS, VENDOR_MICROSOFT };


/* FUNCTIONS ****************************************************************/

static BOOLEAN
IsValidNTOSInstallation(
    IN PUNICODE_STRING SystemRootPath,
    OUT PUSHORT Machine OPTIONAL,
    OUT PUNICODE_STRING VendorName OPTIONAL);

static PNTOS_INSTALLATION
FindExistingNTOSInstall(
    IN PGENERIC_LIST List,
    IN PCWSTR SystemRootArcPath OPTIONAL,
    IN PUNICODE_STRING SystemRootNtPath OPTIONAL // or PCWSTR ?
    );

static PNTOS_INSTALLATION
AddNTOSInstallation(
    IN PGENERIC_LIST List,
    IN PCWSTR InstallationName,
    IN USHORT Machine,
    IN PCWSTR VendorName,
    IN PCWSTR SystemRootArcPath,
    IN PUNICODE_STRING SystemRootNtPath, // or PCWSTR ?
    IN PCWSTR PathComponent,    // Pointer inside SystemRootNtPath buffer
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber,
    IN PPARTENTRY PartEntry OPTIONAL);

typedef struct _ENUM_INSTALLS_DATA
{
    IN OUT PGENERIC_LIST List;
    IN PPARTLIST PartList;
    // IN PPARTENTRY PartEntry;
} ENUM_INSTALLS_DATA, *PENUM_INSTALLS_DATA;

// PENUM_BOOT_ENTRIES_ROUTINE
static NTSTATUS
NTAPI
EnumerateInstallations(
    IN BOOT_STORE_TYPE Type,
    IN PBOOT_STORE_ENTRY BootEntry,
    IN PVOID Parameter OPTIONAL)
{
    PENUM_INSTALLS_DATA Data = (PENUM_INSTALLS_DATA)Parameter;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    PNTOS_INSTALLATION NtOsInstall;

    ULONG DiskNumber = 0, PartitionNumber = 0;
    PCWSTR PathComponent = NULL;
    PDISKENTRY DiskEntry = NULL;
    PPARTENTRY PartEntry = NULL;

    UNICODE_STRING SystemRootPath;
    WCHAR SystemRoot[MAX_PATH];

    USHORT Machine;
    UNICODE_STRING VendorName;
    WCHAR VendorNameBuffer[MAX_PATH];


    /* We have a boot entry */

    /* Check for supported boot type "Windows2003" */
    if (BootEntry->OsOptionsLength < sizeof(NTOS_OPTIONS) ||
        RtlCompareMemory(&BootEntry->OsOptions /* Signature */,
                         NTOS_OPTIONS_SIGNATURE,
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature)) !=
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature))
    {
        /* This is not a Windivs entry */
        // DPRINT("    An installation '%S' of unsupported type '%S'\n",
               // BootEntry->FriendlyName, BootEntry->Version ? BootEntry->Version : L"n/a");
        DPRINT("    An installation '%S' of unsupported type %lu\n",
               BootEntry->FriendlyName, BootEntry->OsOptionsLength);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    /* BootType is Windows2003, now check OsLoadPath */
    if (!Options->OsLoadPath || !*Options->OsLoadPath)
    {
        /* Certainly not a Windivs installation */
        DPRINT1("    A Win2k3 install '%S' without an ARC path?!\n", BootEntry->FriendlyName);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT("    Found a candidate Win2k3 install '%S' with ARC path '%S'\n",
           BootEntry->FriendlyName, Options->OsLoadPath);
    // DPRINT("    Found a Win2k3 install '%S' with ARC path '%S'\n",
           // BootEntry->FriendlyName, Options->OsLoadPath);

    // TODO: Normalize the ARC path.

    /*
     * Check whether we already have an installation with this ARC path.
     * If this is the case, stop there.
     */
    NtOsInstall = FindExistingNTOSInstall(Data->List, Options->OsLoadPath, NULL);
    if (NtOsInstall)
    {
        DPRINT("    An NTOS installation with name \"%S\" from vendor \"%S\" already exists in SystemRoot '%wZ'\n",
               NtOsInstall->InstallationName, NtOsInstall->VendorName, &NtOsInstall->SystemArcPath);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    /*
     * Convert the ARC path into an NT path, from which we will deduce
     * the real disk drive & partition on which the candidate installation
     * resides, as well verifying whether it is indeed an NTOS installation.
     */
    RtlInitEmptyUnicodeString(&SystemRootPath, SystemRoot, sizeof(SystemRoot));
    if (!ArcPathToNtPath(&SystemRootPath, Options->OsLoadPath, Data->PartList))
    {
        DPRINT1("ArcPathToNtPath(%S) failed, skip the installation.\n", Options->OsLoadPath);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT("ArcPathToNtPath() succeeded: '%S' --> '%wZ'\n",
           Options->OsLoadPath, &SystemRootPath);

    /*
     * Check whether we already have an installation with this NT path.
     * If this is the case, stop there.
     */
    NtOsInstall = FindExistingNTOSInstall(Data->List, NULL /*Options->OsLoadPath*/, &SystemRootPath);
    if (NtOsInstall)
    {
        DPRINT1("    An NTOS installation with name \"%S\" from vendor \"%S\" already exists in SystemRoot '%wZ'\n",
                NtOsInstall->InstallationName, NtOsInstall->VendorName, &NtOsInstall->SystemNtPath);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT("EnumerateInstallations: SystemRootPath: '%wZ'\n", &SystemRootPath);

    /* Check if this is a valid NTOS installation; stop there if it isn't one */
    RtlInitEmptyUnicodeString(&VendorName, VendorNameBuffer, sizeof(VendorNameBuffer));
    if (!IsValidNTOSInstallation(&SystemRootPath, &Machine, &VendorName))
    {
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT("Found a valid NTOS installation in SystemRoot ARC path '%S', NT path '%wZ'\n",
           Options->OsLoadPath, &SystemRootPath);

    /* From the NT path, compute the disk, partition and path components */
    if (NtPathToDiskPartComponents(SystemRootPath.Buffer, &DiskNumber, &PartitionNumber, &PathComponent))
    {
        DPRINT("SystemRootPath = '%wZ' points to disk #%d, partition #%d, path '%S'\n",
               &SystemRootPath, DiskNumber, PartitionNumber, PathComponent);

        /* Retrieve the corresponding disk and partition */
        if (!GetDiskOrPartition(Data->PartList, DiskNumber, PartitionNumber, &DiskEntry, &PartEntry))
        {
            DPRINT1("GetDiskOrPartition(disk #%d, partition #%d) failed\n",
                    DiskNumber, PartitionNumber);
        }
    }
    else
    {
        DPRINT1("NtPathToDiskPartComponents(%wZ) failed\n", &SystemRootPath);
    }

    /* Add the discovered NTOS installation into the list */
    AddNTOSInstallation(Data->List,
                        BootEntry->FriendlyName,
                        Machine,
                        VendorName.Buffer, // FIXME: What if it's not NULL-terminated?
                        Options->OsLoadPath,
                        &SystemRootPath, PathComponent,
                        DiskNumber, PartitionNumber, PartEntry);

    /* Continue the enumeration */
    return STATUS_SUCCESS;
}

/*
 * FindSubStrI(PCWSTR str, PCWSTR strSearch) :
 *    Searches for a sub-string 'strSearch' inside 'str', similarly to what
 *    wcsstr(str, strSearch) does, but ignores the case during the comparisons.
 */
PCWSTR FindSubStrI(PCWSTR str, PCWSTR strSearch)
{
    PCWSTR cp = str;
    PCWSTR s1, s2;

    if (!*strSearch)
        return str;

    while (*cp)
    {
        s1 = cp;
        s2 = strSearch;

        while (*s1 && *s2 && (towupper(*s1) == towupper(*s2)))
            ++s1, ++s2;

        if (!*s2)
            return cp;

        ++cp;
    }

    return NULL;
}

static BOOLEAN
CheckForValidPEAndVendor(
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathNameToFile,
    OUT PUSHORT Machine,
    OUT PUNICODE_STRING VendorName)
{
    BOOLEAN Success = FALSE;
    NTSTATUS Status;
    HANDLE FileHandle, SectionHandle;
    // SIZE_T ViewSize;
    PVOID ViewBase;
    PIMAGE_NT_HEADERS NtHeader;
    PVOID VersionBuffer = NULL; // Read-only
    PVOID pvData = NULL;
    UINT BufLen = 0;

    if (VendorName->MaximumLength < sizeof(UNICODE_NULL))
        return FALSE;

    *VendorName->Buffer = UNICODE_NULL;
    VendorName->Length = 0;

    Status = OpenAndMapFile(RootDirectory, PathNameToFile,
                            &FileHandle, &SectionHandle, &ViewBase,
                            NULL, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open and map file '%S', Status 0x%08lx\n", PathNameToFile, Status);
        return FALSE; // Status;
    }

    /* Make sure it's a valid NT PE file */
    NtHeader = RtlImageNtHeader(ViewBase);
    if (!NtHeader)
    {
        DPRINT1("File '%S' does not seem to be a valid NT PE file, bail out\n", PathNameToFile);
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto UnmapCloseFile;
    }

    /* Retrieve the target architecture of this PE module */
    *Machine = NtHeader->FileHeader.Machine;

    /*
     * Search for a valid executable version and vendor.
     * NOTE: The module is loaded as a data file, it should be marked as such.
     */
    Status = NtGetVersionResource((PVOID)((ULONG_PTR)ViewBase | 1), &VersionBuffer, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get version resource for file '%S', Status 0x%08lx\n", PathNameToFile, Status);
        goto UnmapCloseFile;
    }

    Status = NtVerQueryValue(VersionBuffer, L"\\VarFileInfo\\Translation", &pvData, &BufLen);
    if (NT_SUCCESS(Status))
    {
        USHORT wCodePage = 0, wLangID = 0;
        WCHAR FileInfo[MAX_PATH];

        wCodePage = LOWORD(*(ULONG*)pvData);
        wLangID   = HIWORD(*(ULONG*)pvData);

        RtlStringCchPrintfW(FileInfo, ARRAYSIZE(FileInfo),
                            L"StringFileInfo\\%04X%04X\\CompanyName",
                            wCodePage, wLangID);

        Status = NtVerQueryValue(VersionBuffer, FileInfo, &pvData, &BufLen);

        /* Fixup the Status in case pvData is NULL */
        if (NT_SUCCESS(Status) && !pvData)
            Status = STATUS_NOT_FOUND;

        if (NT_SUCCESS(Status) /*&& pvData*/)
        {
            /* BufLen includes the NULL terminator count */
            DPRINT("Found version vendor: \"%S\" for file '%S'\n", pvData, PathNameToFile);

            RtlStringCbCopyNW(VendorName->Buffer, VendorName->MaximumLength,
                              pvData, BufLen * sizeof(WCHAR));
            VendorName->Length = (USHORT)wcslen(VendorName->Buffer) * sizeof(WCHAR);

            Success = TRUE;
        }
    }

    if (!NT_SUCCESS(Status))
        DPRINT("No version vendor found for file '%S'\n", PathNameToFile);

UnmapCloseFile:
    /* Finally, unmap and close the file */
    UnMapAndCloseFile(FileHandle, SectionHandle, ViewBase);

    return Success;
}

//
// TODO: Instead of returning TRUE/FALSE, it would be nice to return
// a flag indicating:
// - whether the installation is actually valid;
// - if it's broken or not (aka. needs for repair, or just upgrading).
//
static BOOLEAN
IsValidNTOSInstallationByHandle(
    IN HANDLE SystemRootDirectory,
    OUT PUSHORT Machine OPTIONAL,
    OUT PUNICODE_STRING VendorName OPTIONAL)
{
    BOOLEAN Success = FALSE;
    PCWSTR PathName;
    USHORT i;
    USHORT LocalMachine;
    UNICODE_STRING LocalVendorName;
    WCHAR VendorNameBuffer[MAX_PATH];

    /* Check for VendorName validity */
    if (VendorName->MaximumLength < sizeof(UNICODE_NULL))
    {
        /* Don't use it, invalidate the pointer */
        VendorName = NULL;
    }
    else
    {
        /* Zero it out */
        *VendorName->Buffer = UNICODE_NULL;
        VendorName->Length = 0;
    }

    /* Check for the existence of \SystemRoot\System32 */
    PathName = L"System32\\";
    if (!DoesDirExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open directory '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\drivers */
    PathName = L"System32\\drivers\\";
    if (!DoesDirExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open directory '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\config */
    PathName = L"System32\\config\\";
    if (!DoesDirExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open directory '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }

#if 0
    /*
     * Check for the existence of SYSTEM and SOFTWARE hives in \SystemRoot\System32\config
     * (but we don't check here whether they are actually valid).
     */
    PathName = L"System32\\config\\SYSTEM";
    if (!DoesFileExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open file '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }
    PathName = L"System32\\config\\SOFTWARE";
    if (!DoesFileExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open file '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }
#endif

    RtlInitEmptyUnicodeString(&LocalVendorName, VendorNameBuffer, sizeof(VendorNameBuffer));

    /* Check for the existence of \SystemRoot\System32\ntoskrnl.exe and retrieves its vendor name */
    PathName = L"System32\\ntoskrnl.exe";
    Success = CheckForValidPEAndVendor(SystemRootDirectory, PathName, &LocalMachine, &LocalVendorName);
    if (!Success)
        DPRINT1("Kernel executable '%S' is either not a PE file, or does not have any vendor?\n", PathName);

    /*
     * The kernel gives the OS its flavour. If we failed due to the absence of
     * ntoskrnl.exe this might be due to the fact this particular installation
     * uses a custom kernel that has a different name, overridden in the boot
     * parameters. We then rely on the existence of ntdll.dll, which cannot be
     * renamed on a valid NT system.
     */
    if (Success)
    {
        for (i = 0; i < ARRAYSIZE(KnownVendors); ++i)
        {
            Success = !!FindSubStrI(LocalVendorName.Buffer, KnownVendors[i]);
            if (Success)
            {
                /* We have found a correct vendor combination */
                DPRINT("IsValidNTOSInstallation: We've got an NTOS installation from %S !\n", KnownVendors[i]);
                break;
            }
        }

        /* Return the target architecture */
        if (Machine)
        {
            /* Copy the value and invalidate the pointer */
            *Machine = LocalMachine;
            Machine = NULL;
        }

        /* Return the vendor name */
        if (VendorName)
        {
            /* Copy the string and invalidate the pointer */
            RtlCopyUnicodeString(VendorName, &LocalVendorName);
            VendorName = NULL;
        }
    }

    /* OPTIONAL: Check for the existence of \SystemRoot\System32\ntkrnlpa.exe */

    /* Check for the existence of \SystemRoot\System32\ntdll.dll and retrieves its vendor name */
    PathName = L"System32\\ntdll.dll";
    Success = CheckForValidPEAndVendor(SystemRootDirectory, PathName, &LocalMachine, &LocalVendorName);
    if (!Success)
        DPRINT1("User-mode DLL '%S' is either not a PE file, or does not have any vendor?\n", PathName);

    if (Success)
    {
        for (i = 0; i < ARRAYSIZE(KnownVendors); ++i)
        {
            if (!!FindSubStrI(LocalVendorName.Buffer, KnownVendors[i]))
            {
                /* We have found a correct vendor combination */
                DPRINT("IsValidNTOSInstallation: The user-mode DLL '%S' is from %S\n", PathName, KnownVendors[i]);
                break;
            }
        }

        /* Return the target architecture if not already obtained */
        if (Machine)
        {
            /* Copy the value and invalidate the pointer */
            *Machine = LocalMachine;
            Machine = NULL;
        }

        /* Return the vendor name if not already obtained */
        if (VendorName)
        {
            /* Copy the string and invalidate the pointer */
            RtlCopyUnicodeString(VendorName, &LocalVendorName);
            VendorName = NULL;
        }
    }

    return Success;
}

static BOOLEAN
IsValidNTOSInstallation(
    IN PUNICODE_STRING SystemRootPath,
    OUT PUSHORT Machine,
    OUT PUNICODE_STRING VendorName OPTIONAL)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE SystemRootDirectory;
    BOOLEAN Success;

    /* Open SystemRootPath */
    InitializeObjectAttributes(&ObjectAttributes,
                               SystemRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&SystemRootDirectory,
                        FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open SystemRoot '%wZ', Status 0x%08lx\n", SystemRootPath, Status);
        return FALSE;
    }

    Success = IsValidNTOSInstallationByHandle(SystemRootDirectory,
                                              Machine, VendorName);

    /* Done! */
    NtClose(SystemRootDirectory);
    return Success;
}

#ifndef NDEBUG
static VOID
DumpNTOSInstalls(
    IN PGENERIC_LIST List)
{
    PGENERIC_LIST_ENTRY Entry;
    PNTOS_INSTALLATION NtOsInstall;
    ULONG NtOsInstallsCount = GetNumberOfListEntries(List);

    DPRINT("There %s %d installation%s detected:\n",
           NtOsInstallsCount >= 2 ? "are" : "is",
           NtOsInstallsCount,
           NtOsInstallsCount >= 2 ? "s" : "");

    for (Entry = GetFirstListEntry(List); Entry; Entry = GetNextListEntry(Entry))
    {
        NtOsInstall = (PNTOS_INSTALLATION)GetListEntryData(Entry);

        DPRINT("    On disk #%d, partition #%d: Installation \"%S\" in SystemRoot '%wZ'\n",
               NtOsInstall->DiskNumber, NtOsInstall->PartitionNumber,
               NtOsInstall->InstallationName, &NtOsInstall->SystemNtPath);
    }

    DPRINT("Done.\n");
}
#endif

static PNTOS_INSTALLATION
FindExistingNTOSInstall(
    IN PGENERIC_LIST List,
    IN PCWSTR SystemRootArcPath OPTIONAL,
    IN PUNICODE_STRING SystemRootNtPath OPTIONAL // or PCWSTR ?
    )
{
    PGENERIC_LIST_ENTRY Entry;
    PNTOS_INSTALLATION NtOsInstall;
    UNICODE_STRING SystemArcPath;

    /*
     * We search either via ARC path or NT path.
     * If both pointers are NULL then we fail straight away.
     */
    if (!SystemRootArcPath && !SystemRootNtPath)
        return NULL;

    RtlInitUnicodeString(&SystemArcPath, SystemRootArcPath);

    for (Entry = GetFirstListEntry(List); Entry; Entry = GetNextListEntry(Entry))
    {
        NtOsInstall = (PNTOS_INSTALLATION)GetListEntryData(Entry);

        /*
         * Note that if both ARC paths are equal, then the corresponding
         * NT paths must be the same. However, two ARC paths may be different
         * but resolve into the same NT path.
         */
        if ( (SystemRootArcPath &&
              RtlEqualUnicodeString(&NtOsInstall->SystemArcPath,
                                    &SystemArcPath, TRUE)) ||
             (SystemRootNtPath  &&
              RtlEqualUnicodeString(&NtOsInstall->SystemNtPath,
                                    SystemRootNtPath, TRUE)) )
        {
            /* Found it! */
            return NtOsInstall;
        }
    }

    return NULL;
}

static PNTOS_INSTALLATION
AddNTOSInstallation(
    IN PGENERIC_LIST List,
    IN PCWSTR InstallationName,
    IN USHORT Machine,
    IN PCWSTR VendorName,
    IN PCWSTR SystemRootArcPath,
    IN PUNICODE_STRING SystemRootNtPath, // or PCWSTR ?
    IN PCWSTR PathComponent,    // Pointer inside SystemRootNtPath buffer
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber,
    IN PPARTENTRY PartEntry OPTIONAL)
{
    PNTOS_INSTALLATION NtOsInstall;
    SIZE_T ArcPathLength, NtPathLength;

    /* Is there already any installation with these settings? */
    NtOsInstall = FindExistingNTOSInstall(List, SystemRootArcPath, SystemRootNtPath);
    if (NtOsInstall)
    {
        DPRINT1("An NTOS installation with name \"%S\" from vendor \"%S\" already exists on disk #%d, partition #%d, in SystemRoot '%wZ'\n",
                NtOsInstall->InstallationName, NtOsInstall->VendorName,
                NtOsInstall->DiskNumber, NtOsInstall->PartitionNumber, &NtOsInstall->SystemNtPath);
        //
        // NOTE: We may use its "IsDefault" attribute, and only keep the entries that have IsDefault == TRUE...
        // Setting IsDefault to TRUE would imply searching for the "Default" entry in the loader configuration file.
        //
        return NtOsInstall;
    }

    ArcPathLength = (wcslen(SystemRootArcPath) + 1) * sizeof(WCHAR);
    // NtPathLength  = ROUND_UP(SystemRootNtPath->Length + sizeof(UNICODE_NULL), sizeof(WCHAR));
    NtPathLength  = SystemRootNtPath->Length + sizeof(UNICODE_NULL);

    /* None was found, so add a new one */
    NtOsInstall = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY,
                                  sizeof(*NtOsInstall) +
                                  ArcPathLength + NtPathLength);
    if (!NtOsInstall)
        return NULL;

    NtOsInstall->DiskNumber = DiskNumber;
    NtOsInstall->PartitionNumber = PartitionNumber;
    NtOsInstall->PartEntry = PartEntry;

    NtOsInstall->Machine = Machine;

    RtlInitEmptyUnicodeString(&NtOsInstall->SystemArcPath,
                              (PWCHAR)(NtOsInstall + 1),
                              ArcPathLength);
    RtlCopyMemory(NtOsInstall->SystemArcPath.Buffer, SystemRootArcPath, ArcPathLength);
    NtOsInstall->SystemArcPath.Length = ArcPathLength - sizeof(UNICODE_NULL);

    RtlInitEmptyUnicodeString(&NtOsInstall->SystemNtPath,
                              (PWCHAR)((ULONG_PTR)(NtOsInstall + 1) + ArcPathLength),
                              NtPathLength);
    RtlCopyUnicodeString(&NtOsInstall->SystemNtPath, SystemRootNtPath);
    NtOsInstall->PathComponent = NtOsInstall->SystemNtPath.Buffer +
                                    (PathComponent - SystemRootNtPath->Buffer);

    RtlStringCchCopyW(NtOsInstall->InstallationName,
                      ARRAYSIZE(NtOsInstall->InstallationName),
                      InstallationName);

    RtlStringCchCopyW(NtOsInstall->VendorName,
                      ARRAYSIZE(NtOsInstall->VendorName),
                      VendorName);

    AppendGenericListEntry(List, NtOsInstall, FALSE);

    return NtOsInstall;
}

static VOID
FindNTOSInstallations(
    IN OUT PGENERIC_LIST List,
    IN PPARTLIST PartList,
    IN PPARTENTRY PartEntry)
{
    NTSTATUS Status;
    ULONG DiskNumber = PartEntry->DiskEntry->DiskNumber;
    ULONG PartitionNumber = PartEntry->PartitionNumber;
    HANDLE PartitionDirectoryHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PartitionRootPath;
    BOOT_STORE_TYPE Type;
    PVOID BootStoreHandle;
    ENUM_INSTALLS_DATA Data;
    ULONG Version;
    WCHAR PathBuffer[MAX_PATH];

    ASSERT(PartEntry->IsPartitioned && PartEntry->PartitionNumber != 0);

    /* Set PartitionRootPath */
    RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                        L"\\Device\\Harddisk%lu\\Partition%lu\\",
                        DiskNumber, PartitionNumber);
    RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
    DPRINT("FindNTOSInstallations: PartitionRootPath: '%wZ'\n", &PartitionRootPath);

    /* Open the partition */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PartitionRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionDirectoryHandle,
                        FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n", &PartitionRootPath, Status);
        return;
    }

    Data.List = List;
    Data.PartList = PartList;

    /* Try to see whether we recognize some NT boot loaders */
    for (Type = FreeLdr; Type < BldrTypeMax; ++Type)
    {
        Status = FindBootStore(PartitionDirectoryHandle, Type, &Version);
        if (!NT_SUCCESS(Status))
        {
            /* The loader does not exist, continue with another one */
            DPRINT("Loader type '%d' does not exist, or an error happened (Status 0x%08lx), continue with another one...\n",
                   Type, Status);
            continue;
        }

        /* The loader exists, try to enumerate its boot entries */
        DPRINT("Analyze the OS installations for loader type '%d' in disk #%d, partition #%d\n",
               Type, DiskNumber, PartitionNumber);

        Status = OpenBootStoreByHandle(&BootStoreHandle, PartitionDirectoryHandle, Type, FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not open the NTOS boot store of type '%d' (Status 0x%08lx), continue with another one...\n",
                    Type, Status);
            continue;
        }
        EnumerateBootStoreEntries(BootStoreHandle, EnumerateInstallations, &Data);
        CloseBootStore(BootStoreHandle);
    }

    /* Close the partition */
    NtClose(PartitionDirectoryHandle);
}

// static
FORCEINLINE BOOLEAN
ShouldICheckThisPartition(
    IN PPARTENTRY PartEntry)
{
    if (!PartEntry)
        return FALSE;

    return PartEntry->IsPartitioned &&
           !IsContainerPartition(PartEntry->PartitionType) /* alternatively: PartEntry->PartitionNumber != 0 */ &&
           !PartEntry->New &&
           (PartEntry->FormatState == Preformatted /* || PartEntry->FormatState == Formatted */);
}

// EnumerateNTOSInstallations
PGENERIC_LIST
CreateNTOSInstallationsList(
    IN PPARTLIST PartList)
{
    PGENERIC_LIST List;
    PLIST_ENTRY Entry, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    /* Loop each available disk ... */
    Entry = PartList->DiskListHead.Flink;
    while (Entry != &PartList->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
        Entry = Entry->Flink;

        DPRINT("Disk #%d\n", DiskEntry->DiskNumber);

        /* ... and for each disk, loop each available partition */

        /* First, the primary partitions */
        Entry2 = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry2 != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            Entry2 = Entry2->Flink;

            ASSERT(PartEntry->DiskEntry == DiskEntry);

            DPRINT("   Primary Partition #%d, index %d - Type 0x%02x, IsLogical = %s, IsPartitioned = %s, IsNew = %s, FormatState = %lu -- Should I check it? %s\n",
                   PartEntry->PartitionNumber, PartEntry->PartitionIndex,
                   PartEntry->PartitionType, PartEntry->LogicalPartition ? "TRUE" : "FALSE",
                   PartEntry->IsPartitioned ? "TRUE" : "FALSE",
                   PartEntry->New ? "Yes" : "No",
                   PartEntry->FormatState,
                   ShouldICheckThisPartition(PartEntry) ? "YES!" : "NO!");

            if (ShouldICheckThisPartition(PartEntry))
                FindNTOSInstallations(List, PartList, PartEntry);
        }

        /* Then, the logical partitions (present in the extended partition) */
        Entry2 = DiskEntry->LogicalPartListHead.Flink;
        while (Entry2 != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            Entry2 = Entry2->Flink;

            ASSERT(PartEntry->DiskEntry == DiskEntry);

            DPRINT("   Logical Partition #%d, index %d - Type 0x%02x, IsLogical = %s, IsPartitioned = %s, IsNew = %s, FormatState = %lu -- Should I check it? %s\n",
                   PartEntry->PartitionNumber, PartEntry->PartitionIndex,
                   PartEntry->PartitionType, PartEntry->LogicalPartition ? "TRUE" : "FALSE",
                   PartEntry->IsPartitioned ? "TRUE" : "FALSE",
                   PartEntry->New ? "Yes" : "No",
                   PartEntry->FormatState,
                   ShouldICheckThisPartition(PartEntry) ? "YES!" : "NO!");

            if (ShouldICheckThisPartition(PartEntry))
                FindNTOSInstallations(List, PartList, PartEntry);
        }
    }

#ifndef NDEBUG
    /**** Debugging: List all the collected installations ****/
    DumpNTOSInstalls(List);
#endif

    return List;
}

/* EOF */
