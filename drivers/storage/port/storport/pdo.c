/*
 * PROJECT:     Windivs Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport PDO code
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

NTSTATUS
PortCreatePdo(
    _In_ PFDO_DEVICE_EXTENSION FdoDeviceExtension,
    _In_ ULONG Bus,
    _In_ ULONG Target,
    _In_ ULONG Lun,
    _Out_ PPDO_DEVICE_EXTENSION *PdoDeviceExtension)
{
    PPDO_DEVICE_EXTENSION DeviceExtension = NULL;
    PDEVICE_OBJECT Pdo = NULL;
    KLOCK_QUEUE_HANDLE LockHandle;
    NTSTATUS Status;

    DPRINT("PortCreatePdo(%p %p)\n",
           FdoDeviceExtension, PdoDeviceExtension);

    /* Create the port device */
    Status = IoCreateDevice(FdoDeviceExtension->Device->DriverObject,
                            sizeof(PDO_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_MASS_STORAGE,
                            FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice() failed (Status 0x%lX)\n", Status);
        return Status;
    }

    /* Initialize the device */
    Pdo->Flags |= DO_DIRECT_IO;
    Pdo->Flags |= DO_POWER_PAGABLE;

    DeviceExtension = (PPDO_DEVICE_EXTENSION)Pdo->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

    DeviceExtension->ExtensionType = PdoExtension;

    DeviceExtension->Device = Pdo;
    DeviceExtension->FdoExtension = FdoDeviceExtension;
    DeviceExtension->PnpState = dsStopped;

    /* Add the PDO to the PDO list*/
    KeAcquireInStackQueuedSpinLock(&FdoDeviceExtension->PdoListLock,
                                   &LockHandle);
    InsertHeadList(&FdoDeviceExtension->PdoListHead,
                   &DeviceExtension->PdoListEntry);
    FdoDeviceExtension->PdoCount++;
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    DeviceExtension->Bus = Bus;
    DeviceExtension->Target = Target;
    DeviceExtension->Lun = Lun;


    // FIXME: More initialization


    /* The device has been initialized */
    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

    *PdoDeviceExtension = DeviceExtension;

    return STATUS_SUCCESS;
}


NTSTATUS
PortDeletePdo(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    KLOCK_QUEUE_HANDLE LockHandle;

    DPRINT("PortDeletePdo(%p)\n", PdoExtension);

    /* Remove the PDO from the PDO list*/
    KeAcquireInStackQueuedSpinLock(&PdoExtension->FdoExtension->PdoListLock,
                                   &LockHandle);
    RemoveEntryList(&PdoExtension->PdoListEntry);
    PdoExtension->FdoExtension->PdoCount--;
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    if (PdoExtension->InquiryBuffer)
    {
        ExFreePoolWithTag(PdoExtension->InquiryBuffer, TAG_INQUIRY_DATA);
        PdoExtension->InquiryBuffer = NULL;
    }


    // FIXME: More uninitialization


    /* Delete the PDO */
    IoDeleteDevice(PdoExtension->Device);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PortPdoScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    DPRINT1("PortPdoScsi(%p %p)\n", DeviceObject, Irp);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PortPdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    DPRINT1("PortPdoPnp(%p %p)\n", DeviceObject, Irp);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/* EOF */
