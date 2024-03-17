/*
 * PROJECT:    PAINT for Windivs
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Initializing everything
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

#include <mapi.h>

POINT g_ptStart, g_ptEnd;
BOOL g_askBeforeEnlarging = FALSE;  // TODO: initialize from registry
HINSTANCE g_hinstExe = NULL;
TCHAR g_szFileName[MAX_LONG_PATH] = { 0 };
WCHAR g_szMailTempFile[MAX_LONG_PATH] = { 0 };
BOOL g_isAFile = FALSE;
BOOL g_imageSaved = FALSE;
BOOL g_showGrid = FALSE;

CMainWindow mainWindow;

/* FUNCTIONS ********************************************************/

// get file name extension from filter string
static BOOL
FileExtFromFilter(LPTSTR pExt, OPENFILENAME *pOFN)
{
    LPTSTR pchExt = pExt;
    *pchExt = 0;

    DWORD nIndex = 1;
    for (LPCTSTR pch = pOFN->lpstrFilter; *pch; ++nIndex)
    {
        pch += lstrlen(pch) + 1;
        if (pOFN->nFilterIndex == nIndex)
        {
            for (++pch; *pch && *pch != _T(';'); ++pch)
            {
                *pchExt++ = *pch;
            }
            *pchExt = 0;
            CharLower(pExt);
            return TRUE;
        }
        pch += lstrlen(pch) + 1;
    }
    return FALSE;
}

// Hook procedure for OPENFILENAME to change the file name extension
static UINT_PTR APIENTRY
OFNHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hParent;
    OFNOTIFY *pon;
    switch (uMsg)
    {
    case WM_NOTIFY:
        pon = (OFNOTIFY *)lParam;
        if (pon->hdr.code == CDN_TYPECHANGE)
        {
            hParent = GetParent(hwnd);
            TCHAR Path[MAX_PATH];
            SendMessage(hParent, CDM_GETFILEPATH, _countof(Path), (LPARAM)Path);
            FileExtFromFilter(PathFindExtension(Path), pon->lpOFN);
            SendMessage(hParent, CDM_SETCONTROLTEXT, 0x047c, (LPARAM)PathFindFileName(Path));
            lstrcpyn(pon->lpOFN->lpstrFile, Path, pon->lpOFN->nMaxFile);
        }
        break;
    }
    return 0;
}

typedef ULONG (WINAPI *FN_MAPISendMail)(LHANDLE, ULONG_PTR, lpMapiMessage, FLAGS, ULONG);
typedef ULONG (WINAPI *FN_MAPISendMailW)(LHANDLE, ULONG_PTR, lpMapiMessageW, FLAGS, ULONG);

BOOL OpenMailer(HWND hWnd, LPCWSTR pszPathName)
{
    // Delete the temporary file if any
    if (g_szMailTempFile[0])
    {
        ::DeleteFileW(g_szMailTempFile);
        g_szMailTempFile[0] = UNICODE_NULL;
    }

    CStringW strFileTitle;
    if (PathFileExistsW(pszPathName) && imageModel.IsImageSaved())
    {
        strFileTitle = PathFindFileNameW(pszPathName);
    }
    else // Not existing or not saved
    {
        // Get the name of a temporary file
        WCHAR szTempDir[MAX_PATH];
        ::GetTempPathW(_countof(szTempDir), szTempDir);
        if (!::GetTempFileNameW(szTempDir, L"afx", 0, g_szMailTempFile))
            return FALSE; // Failure

        if (PathFileExistsW(g_szFileName))
        {
            // Set file title
            strFileTitle = PathFindFileNameW(g_szFileName);

            // Copy to the temporary file
            if (!::CopyFileW(g_szFileName, g_szMailTempFile, FALSE))
            {
                g_szMailTempFile[0] = UNICODE_NULL;
                return FALSE; // Failure
            }
        }
        else
        {
            // Set file title
            strFileTitle.LoadString(IDS_DEFAULTFILENAME);
            strFileTitle += L".png";

            // Save it to the temporary file
            HBITMAP hbmLocked = imageModel.LockBitmap();
            BOOL ret = SaveDIBToFile(hbmLocked, g_szMailTempFile, FALSE, Gdiplus::ImageFormatPNG);
            imageModel.UnlockBitmap(hbmLocked);
            if (!ret)
            {
                g_szMailTempFile[0] = UNICODE_NULL;
                return FALSE; // Failure
            }
        }

        // Use the temporary file 
        pszPathName = g_szMailTempFile;
    }

    // Load "mapi32.dll"
    HINSTANCE hMAPI = LoadLibraryW(L"mapi32.dll");
    if (!hMAPI)
        return FALSE; // Failure

    // Attachment
    MapiFileDescW attachmentW = { 0 };
    attachmentW.nPosition = (ULONG)-1;
    attachmentW.lpszPathName = (LPWSTR)pszPathName;
    attachmentW.lpszFileName = (LPWSTR)(LPCWSTR)strFileTitle;

    // Message with attachment
    MapiMessageW messageW = { 0 };
    messageW.lpszSubject = NULL;
    messageW.nFileCount = 1;
    messageW.lpFiles = &attachmentW;

    // First, try to open the mailer by the function of Unicode version
    FN_MAPISendMailW pMAPISendMailW = (FN_MAPISendMailW)::GetProcAddress(hMAPI, "MAPISendMailW");
    if (pMAPISendMailW)
    {
        pMAPISendMailW(0, (ULONG_PTR)hWnd, &messageW, MAPI_DIALOG | MAPI_LOGON_UI, 0);
        ::FreeLibrary(hMAPI);
        return TRUE; // MAPISendMailW will show an error message on failure
    }

    // Convert to ANSI strings
    CStringA szPathNameA(pszPathName), szFileTitleA(strFileTitle);

    MapiFileDesc attachment = { 0 };
    attachment.nPosition = (ULONG)-1;
    attachment.lpszPathName = (LPSTR)(LPCSTR)szPathNameA;
    attachment.lpszFileName = (LPSTR)(LPCSTR)szFileTitleA;

    MapiMessage message = { 0 };
    message.lpszSubject = NULL;
    message.nFileCount = 1;
    message.lpFiles = &attachment;

    // Try again but in ANSI version
    FN_MAPISendMail pMAPISendMail = (FN_MAPISendMail)::GetProcAddress(hMAPI, "MAPISendMail");
    if (pMAPISendMail)
    {
        pMAPISendMail(0, (ULONG_PTR)hWnd, &message, MAPI_DIALOG | MAPI_LOGON_UI, 0);
        ::FreeLibrary(hMAPI);
        return TRUE; // MAPISendMail will show an error message on failure
    }

    ::FreeLibrary(hMAPI);
    return FALSE; // Failure
}

BOOL CMainWindow::GetOpenFileName(IN OUT LPTSTR pszFile, INT cchMaxFile)
{
    static OPENFILENAME ofn = { 0 };
    static CString strFilter;

    if (ofn.lStructSize == 0)
    {
        // The "All Files" item text
        CString strAllPictureFiles;
        strAllPictureFiles.LoadString(g_hinstExe, IDS_ALLPICTUREFILES);

        // Get the import filter
        CSimpleArray<GUID> aguidFileTypesI;
        CImage::GetImporterFilterString(strFilter, aguidFileTypesI, strAllPictureFiles,
                                        CImage::excludeDefaultLoad, _T('\0'));

        // Initializing the OPENFILENAME structure for GetOpenFileName
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = m_hWnd;
        ofn.hInstance   = g_hinstExe;
        ofn.lpstrFilter = strFilter;
        ofn.Flags       = OFN_EXPLORER | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = L"png";
    }

    ofn.lpstrFile = pszFile;
    ofn.nMaxFile  = cchMaxFile;
    return ::GetOpenFileName(&ofn);
}

BOOL CMainWindow::GetSaveFileName(IN OUT LPTSTR pszFile, INT cchMaxFile)
{
    static OPENFILENAME sfn = { 0 };
    static CString strFilter;

    if (sfn.lStructSize == 0)
    {
        // Get the export filter
        CSimpleArray<GUID> aguidFileTypesE;
        CImage::GetExporterFilterString(strFilter, aguidFileTypesE, NULL,
                                        CImage::excludeDefaultSave, _T('\0'));

        // Initializing the OPENFILENAME structure for GetSaveFileName
        ZeroMemory(&sfn, sizeof(sfn));
        sfn.lStructSize = sizeof(sfn);
        sfn.hwndOwner   = m_hWnd;
        sfn.hInstance   = g_hinstExe;
        sfn.lpstrFilter = strFilter;
        sfn.Flags       = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK;
        sfn.lpfnHook    = OFNHookProc;
        sfn.lpstrDefExt = L"png";

        LPWSTR pchDotExt = PathFindExtensionW(pszFile);
        if (*pchDotExt == UNICODE_NULL)
        {
            // Choose PNG
            wcscat(pszFile, L".png");
            for (INT i = 0; i < aguidFileTypesE.GetSize(); ++i)
            {
                if (aguidFileTypesE[i] == Gdiplus::ImageFormatPNG)
                {
                    sfn.nFilterIndex = i + 1;
                    break;
                }
            }
        }
    }

    sfn.lpstrFile = pszFile;
    sfn.nMaxFile  = cchMaxFile;
    return ::GetSaveFileName(&sfn);
}

BOOL CMainWindow::ChooseColor(IN OUT COLORREF *prgbColor)
{
    static CHOOSECOLOR choosecolor = { 0 };
    static COLORREF custColors[16] =
    {
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff
    };

    if (choosecolor.lStructSize == 0)
    {
        // Initializing the CHOOSECOLOR structure for ChooseColor
        ZeroMemory(&choosecolor, sizeof(choosecolor));
        choosecolor.lStructSize  = sizeof(choosecolor);
        choosecolor.hwndOwner    = m_hWnd;
        choosecolor.lpCustColors = custColors;
    }

    choosecolor.Flags = CC_RGBINIT;
    choosecolor.rgbResult = *prgbColor;
    if (!::ChooseColor(&choosecolor))
        return FALSE;

    *prgbColor = choosecolor.rgbResult;
    return TRUE;
}

HWND CMainWindow::DoCreate()
{
    ::LoadString(g_hinstExe, IDS_DEFAULTFILENAME, g_szFileName, _countof(g_szFileName));

    CString strTitle;
    strTitle.Format(IDS_WINDOWTITLE, PathFindFileName(g_szFileName));

    RECT& rc = registrySettings.WindowPlacement.rcNormalPosition;
    return Create(HWND_DESKTOP, rc, strTitle, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES);
}

// entry point
INT WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
#ifdef _DEBUG
    // Report any memory leaks on exit
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
            SystemParametersInfoW(SPI_GETWHEELSCROLLCHARS, 0, &nCount, 0);
            for (UINT i = 0; i < nCount; ++i)
            {
                if (zDelta < 0)
                    ::PostMessageW(canvasWindow, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
                else if (zDelta > 0)
                    ::PostMessageW(canvasWindow, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            }
        }
        else
        {
            SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &nCount, 0);
            for (UINT i = 0; i < nCount; ++i)
            {
                if (zDelta < 0)
                    ::PostMessageW(canvasWindow, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
                else if (zDelta > 0)
                    ::PostMessageW(canvasWindow, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            }
        }
    }

    return 0;
}

LRESULT CMainWindow::OnDropFiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WCHAR droppedfile[MAX_PATH];

    HDROP hDrop = (HDROP)wParam;
    DragQueryFile(hDrop, 0, droppedfile, _countof(droppedfile));
    DragFinish(hDrop);

    ConfirmSave() && DoLoadImageFile(m_hWnd, droppedfile, TRUE);

    return 0;
}

LRESULT CMainWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Loading and setting the window menu from resource
    m_hMenu = ::LoadMenuW(g_hinstExe, MAKEINTRESOURCEW(ID_MENU));
    SetMenu(m_hMenu);

    // Create the status bar
    DWORD style = SBARS_SIZEGRIP | WS_CHILD | (registrySettings.ShowStatusBar ? WS_VISIBLE : 0);
    g_hStatusBar = ::CreateWindowExW(0, STATUSCLASSNAME, NULL, style, 0, 0, 0, 0, m_hWnd,
                                     NULL, g_hinstExe, NULL);
    ::SendMessageW(g_hStatusBar, SB_SETMINHEIGHT, 21, 0);

    // Create the tool box
    toolBoxContainer.DoCreate(m_hWnd);

    // Create the palette window
    RECT rcEmpty = { 0, 0, 0, 0 }; // Rely on WM_SIZE
    style = WS_CHILD | (registrySettings.ShowPalette ? WS_VISIBLE : 0);
    paletteWindow.Create(m_hWnd, rcEmpty, NULL, style, WS_EX_STATICEDGE);

    // Create the canvas
    style = WS_CHILD | WS_GROUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE;
    canvasWindow.Create(m_hWnd, rcEmpty, NULL, style, WS_EX_CLIENTEDGE);

    // Create and show the miniature if necessary
    if (registrySettings.ShowThumbnail)
    {
        miniature.DoCreate(m_hWnd);
        miniature.ShowWindow(SW_SHOWNOACTIVATE);
    }

    // Set icon
    SendMessage(WM_SETICON, ICON_BIG, (LPARAM)::LoadIconW(g_hinstExe, MAKEINTRESOURCEW(IDI_APPICON)));
    SendMessage(WM_SETICON, ICON_SMALL, (LPARAM)::LoadIconW(g_hinstExe, MAKEINTRESOURCEW(IDI_APPICON)));

    return 0;
}

LRESULT CMainWindow::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    registrySettings.WindowPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&(registrySettings.WindowPlacement));

    DoHtmlHelpW(NULL, NULL, HH_CLOSE_ALL, 0);

    if (s_hHHCTRL_OCX)
    {
        FreeLibrary(s_hHHCTRL_OCX);
        s_hHHCTRL_OCX = NULL;
        s_pHtmlHelpW = NULL;
    }

    SetMenu(NULL);
    if (m_hMenu)
    {
        ::DestroyMenu(m_hMenu);
        m_hMenu = NULL;
    }

    PostQuitMessage(0); /* send a WM_QUIT to the message queue */
    return 0;
}

BOOL CMainWindow::ConfirmSave()
{
    canvasWindow.OnEndDraw(FALSE);

    if (imageModel.IsImageSaved())
        return TRUE;

    CStringW strProgramName;
    strProgramName.LoadString(IDS_PROGRAMNAME);

    CStringW strSavePromptText;
    strSavePromptText.Format(IDS_SAVEPROMPTTEXT, PathFindFileName(g_szFileName));

    switch (MessageBox(strSavePromptText, strProgramName, MB_YESNOCANCEL | MB_ICONQUESTION))
    {
        case IDYES:
            saveImage(TRUE);
            return imageModel.IsImageSaved();
        case IDNO:
            return TRUE;
        case IDCANCEL:
            return FALSE;
    }

    return TRUE;
}

LRESULT CMainWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (ConfirmSave())
    {
        DestroyWindow();
    }
    return 0;
}

void CMainWindow::ProcessFileMenu(HMENU hPopupMenu)
{
    for (INT iItem = 0; iItem < MAX_RECENT_FILES; ++iItem)
        RemoveMenu(hPopupMenu, IDM_FILE1 + iItem, MF_BYCOMMAND);

    if (registrySettings.strFiles[0].IsEmpty())
        return;

    RemoveMenu(hPopupMenu, IDM_FILEMOSTRECENTLYUSEDFILE, MF_BYCOMMAND);

    INT cMenuItems = GetMenuItemCount(hPopupMenu);

    for (INT iItem = 0; iItem < MAX_RECENT_FILES; ++iItem)
    {
        CStringW& strFile = registrySettings.strFiles[iItem];
        if (strFile.IsEmpty())
            break;

        // Condense the lengthy pathname by using '...'
#define MAX_RECENT_PATHNAME_DISPLAY 30
        CPath pathFile(strFile);
        pathFile.CompactPathEx(MAX_RECENT_PATHNAME_DISPLAY);
        assert(wcslen((LPCWSTR)pathFile) <= MAX_RECENT_PATHNAME_DISPLAY);

        // Add an accelerator (by '&') to the item number for quick access
        WCHAR szText[4 + MAX_RECENT_PATHNAME_DISPLAY + 1];
        StringCchPrintfW(szText, _countof(szText), L"&%u %s", iItem + 1, (LPCWSTR)pathFile);

        INT iMenuItem = (cMenuItems - 2) + iItem;
        InsertMenu(hPopupMenu, iMenuItem, MF_BYPOSITION | MF_STRING, IDM_FILE1 + iItem, szText);
    }
}

BOOL CMainWindow::CanUndo() const
{
    if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
        return (BOOL)textEditWindow.SendMessage(EM_CANUNDO);
    if (selectionModel.m_bShow && toolsModel.IsSelection())
        return TRUE;
    return imageModel.CanUndo();
}

BOOL CMainWindow::CanRedo() const
{
    if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
        return FALSE; // There is no "WM_REDO" in EDIT control
    return imageModel.CanRedo();
}

BOOL CMainWindow::CanPaste() const
{
    if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
        return ::IsClipboardFormatAvailable(CF_UNICODETEXT);

    return (::IsClipboardFormatAvailable(CF_ENHMETAFILE) ||
            ::IsClipboardFormatAvailable(CF_DIB) ||
            ::IsClipboardFormatAvailable(CF_BITMAP));
}

LRESULT CMainWindow::OnInitMenuPopup(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HMENU menu = (HMENU)wParam;
    BOOL trueSelection = (selectionModel.m_bShow && toolsModel.IsSelection());
    BOOL textShown = (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow));
    DWORD dwStart = 0, dwEnd = 0;
    if (textShown)
        textEditWindow.SendMessage(EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    BOOL hasTextSel = (dwStart < dwEnd);

    //
    // File menu
    //
    if (::GetSubMenu(GetMenu(), 0) == menu)
    {
        ProcessFileMenu(menu);
    }

    //
    // Edit menu
    //
    EnableMenuItem(menu, IDM_EDITUNDO, ENABLED_IF(CanUndo()));
    EnableMenuItem(menu, IDM_EDITREDO, ENABLED_IF(CanRedo()));
    EnableMenuItem(menu, IDM_EDITCUT, ENABLED_IF(textShown ? hasTextSel : trueSelection));
    EnableMenuItem(menu, IDM_EDITCOPY, ENABLED_IF(textShown ? hasTextSel : trueSelection));
    EnableMenuItem(menu, IDM_EDITDELETESELECTION,
                   ENABLED_IF(textShown ? hasTextSel : trueSelection));
    EnableMenuItem(menu, IDM_EDITINVERTSELECTION, ENABLED_IF(trueSelection));
    EnableMenuItem(menu, IDM_EDITCOPYTO, ENABLED_IF(trueSelection));
    EnableMenuItem(menu, IDM_EDITPASTE, ENABLED_IF(CanPaste()));
    EnableMenuItem(menu, IDM_CROPSELECTION, ENABLED_IF(trueSelection));

    //
    // View menu
    //
    CheckMenuItem(menu, IDM_VIEWTOOLBOX, CHECKED_IF(::IsWindowVisible(toolBoxContainer)));
    CheckMenuItem(menu, IDM_VIEWCOLORPALETTE, CHECKED_IF(::IsWindowVisible(paletteWindow)));
    CheckMenuItem(menu, IDM_VIEWSTATUSBAR,    CHECKED_IF(::IsWindowVisible(g_hStatusBar)));
    CheckMenuItem(menu, IDM_FORMATICONBAR, CHECKED_IF(::IsWindowVisible(fontsDialog)));
    EnableMenuItem(menu, IDM_FORMATICONBAR, ENABLED_IF(toolsModel.GetActiveTool() == TOOL_TEXT));
    CheckMenuItem(menu, IDM_VIEWZOOM125, CHECKED_IF(toolsModel.GetZoom() == 125));
    CheckMenuItem(menu, IDM_VIEWZOOM25,  CHECKED_IF(toolsModel.GetZoom() == 250));
    CheckMenuItem(menu, IDM_VIEWZOOM50,  CHECKED_IF(toolsModel.GetZoom() == 500));
    CheckMenuItem(menu, IDM_VIEWZOOM100, CHECKED_IF(toolsModel.GetZoom() == 1000));
    CheckMenuItem(menu, IDM_VIEWZOOM200, CHECKED_IF(toolsModel.GetZoom() == 2000));
    CheckMenuItem(menu, IDM_VIEWZOOM400, CHECKED_IF(toolsModel.GetZoom() == 4000));
    CheckMenuItem(menu, IDM_VIEWZOOM800, CHECKED_IF(toolsModel.GetZoom() == 8000));
    CheckMenuItem(menu, IDM_VIEWSHOWGRID,      CHECKED_IF(g_showGrid));
    CheckMenuItem(menu, IDM_VIEWSHOWMINIATURE, CHECKED_IF(registrySettings.ShowThumbnail));

    //
    // Image menu
    //
    EnableMenuItem(menu, IDM_IMAGECROP, ENABLED_IF(selectionModel.m_bShow));
    EnableMenuItem(menu, IDM_IMAGEDELETEIMAGE, ENABLED_IF(!selectionModel.m_bShow));
    CheckMenuItem(menu, IDM_IMAGEDRAWOPAQUE, CHECKED_IF(!toolsModel.IsBackgroundTransparent()));

    //
    // Palette menu
    //
    CheckMenuItem(menu, IDM_COLORSMODERNPALETTE, CHECKED_IF(paletteModel.SelectedPalette() == PAL_MODERN));
    CheckMenuItem(menu, IDM_COLORSOLDPALETTE,    CHECKED_IF(paletteModel.SelectedPalette() == PAL_OLDTYPE));
    return 0;
}

LRESULT CMainWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int test[] = { LOWORD(lParam) - 260, LOWORD(lParam) - 140, LOWORD(lParam) - 20 };
    if (::IsWindow(g_hStatusBar))
    {
        ::SendMessageW(g_hStatusBar, WM_SIZE, 0, 0);
        ::SendMessageW(g_hStatusBar, SB_SETPARTS, 3, (LPARAM)&test);
    }
    alignChildrenToMainWindow();
    return 0;
}

    g_hinstExe = hInstance;

    // Initialize common controls library
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&iccx);

    // Load settings from registry
    registrySettings.Load(nCmdShow);

    // Create the main window
    if (!mainWindow.DoCreate())
    {
        MessageBox(NULL, TEXT("Failed to create main window."), NULL, MB_ICONERROR);
        return 1;
    }

    // Initialize imageModel
    if (__argc < 2 || !DoLoadImageFile(mainWindow, __targv[1], TRUE))
        InitializeImage(NULL, NULL, FALSE);

    // Make the window visible on the screen
    mainWindow.ShowWindow(registrySettings.WindowPlacement.showCmd);

    // Load the access keys
    HACCEL hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(800));

    // The message loop
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
    {
        if (fontsDialog.IsWindow() && fontsDialog.IsDialogMessage(&msg))
            continue;

        if (::TranslateAccelerator(mainWindow, hAccel, &msg))
            continue;

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    // Unload the access keys
    ::DestroyAcceleratorTable(hAccel);

    // Write back settings to registry
    registrySettings.Store();

    if (g_szMailTempFile[0])
        ::DeleteFileW(g_szMailTempFile);

    // Return the value that PostQuitMessage() gave
    return (INT)msg.wParam;
}
