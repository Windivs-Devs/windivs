/*
 * PROJECT:     ReactOS Safely Remove Hardware Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware safe removal support
 * COPYRIGHT:   Copyright 2023 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#include "hotplug.h"

static
VOID
SafeRemoveDevice(
    _In_ DEVINST DevInst)
{
    PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown;
    CONFIGRET cr;

    cr = CM_Request_Device_EjectW(DevInst, &VetoType, NULL, 0, 0);
    if (cr != CR_SUCCESS && VetoType == PNP_VetoTypeUnknown)
    {
        WCHAR szError[64];
        swprintf(szError, L"Failed to remove device (0x%x)", cr);
        MessageBoxW(NULL, szError, NULL, MB_ICONEXCLAMATION | MB_OK);
    }
}

INT_PTR
CALLBACK
ConfirmRemovalDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    static DEVINST selectedDev;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            selectedDev = lParam;
            return TRUE;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                    SafeRemoveDevice(selectedDev);
                    EndDialog(hwndDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, TRUE);
                    break;
            }

            break;
        }
    }

    return FALSE;
}
