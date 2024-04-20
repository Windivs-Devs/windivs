#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <commdlg.h>
#include <ddeml.h>
#include <userenv.h>

#include <shlwapi.h>
#include <wininet.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <emptyvc.h>
#include <ndk/rtlfuncs.h>
#include <fmifs/fmifs.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <atlcoll.h>
#include <powrprof.h>
#include <winnetwk.h>
#include <objsafe.h>

#include <comctl32_undoc.h>
#include <shlguid_undoc.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>

#include <shellapi.h>
#undef ShellExecute
#include <undocshell.h>

/*
 * For versions < Vista+, redefine ShellMessageBoxW to ShellMessageBoxWrapW
 * (this is needed to avoid a linker error). On Vista+ onwards, shell32.ShellMessageBoxW
 * redirects to shlwapi.ShellMessageBoxW so the #define should not be needed.
 *
 * However our shell32 is built with _WIN32_WINNT set to 0x600 (Vista+),
 * yet its exports (especially regarding ShellMessageBoxA/W) are Win2003
 * compatible. So the #define is still needed, and the check be disabled.
 */
// #if (_WIN32_WINNT < 0x0600)
#define ShellMessageBoxW ShellMessageBoxWrapW
// #endif

#include <browseui_undoc.h>

#include <shellutils.h>

#include "shellrecyclebin/recyclebin.h"
#include "wine/pidl.h"
#include "debughlp.h"
#include "wine/shell32_main.h"
#include "shresdef.h"
#include "wine/cpanel.h"
#include "CActiveDesktop.h"
#include "CEnumIDListBase.h"
#include "shfldr.h"
#include "CShellItem.h"
#include "CShellLink.h"
#include "CFolderItemVerbs.h"
#include "CFolderItems.h"
#include "CFolder.h"
#include "CShellDispatch.h"
#include "CDropTargetHelper.h"
#include "CFolderOptions.h"
#include "folders/CFSFolder.h"
#include "folders/CDrivesFolder.h"
#include "folders/CDesktopFolder.h"
#include "folders/CControlPanelFolder.h"
#include "folders/CMyDocsFolder.h"
#include "folders/CNetFolder.h"
#include "folders/CFontsFolder.h"
#include "folders/CPrinterFolder.h"
#include "folders/CAdminToolsFolder.h"
#include "folders/CRecycleBin.h"
#include "droptargets/CexeDropHandler.h"
#include "droptargets/CFSDropTarget.h"
#include "COpenWithMenu.h"
#include "CNewMenu.h"
#include "CSendToMenu.h"
#include "CCopyMoveToMenu.h"
#include "dialogs/filedefext.h"
#include "dialogs/drvdefext.h"
#include "CQueryAssociations.h"
#include "shellmenu/CMenuBand.h"
#include "shellmenu/CMenuDeskBar.h"
#include "shellmenu/CMenuSite.h"
#include "shellmenu/CMergedFolder.h"
#include "shellmenu/shellmenu.h"
#include "CUserNotification.h"
#include "dialogs/folder_options.h"
#include "shelldesktop/CChangeNotifyServer.h"

#include <wine/debug.h>
#include <wine/unicode.h>

extern const GUID CLSID_AdminFolderShortcut;
extern const GUID CLSID_FontsFolderShortcut;
extern const GUID CLSID_StartMenu;
extern const GUID CLSID_MenuBandSite;
extern const GUID CLSID_OpenWith;
extern const GUID CLSID_UnixFolder;
extern const GUID CLSID_UnixDosFolder;
extern const GUID SHELL32_AdvtShortcutProduct;
extern const GUID SHELL32_AdvtShortcutComponent;

#define MAX_PROPERTY_SHEET_PAGE 32

extern inline
BOOL
CALLBACK
AddPropSheetPageCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    PROPSHEETHEADERW *pHeader = (PROPSHEETHEADERW *)lParam;

    if (pHeader->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        pHeader->phpage[pHeader->nPages++] = hPage;
        return TRUE;
    }

    return FALSE;
}

HRESULT WINAPI
Shell_DefaultContextMenuCallBack(IShellFolder *psf, IDataObject *pdtobj);

// CStubWindow32 --- The owner window of file property sheets.
// This window hides taskbar button of property sheet.
class CStubWindow32 : public CWindowImpl<CStubWindow32>
{
public:
    DECLARE_WND_CLASS_EX(_T("StubWindow32"), 0, COLOR_WINDOWTEXT)

    BEGIN_MSG_MAP(CStubWindow32)
    END_MSG_MAP()
};

HRESULT
Shell_TranslateIDListAlias(
    _In_ LPCITEMIDLIST pidl,
    _In_ HANDLE hToken,
    _Out_ LPITEMIDLIST *ppidlAlias,
    _In_ DWORD dwFlags);

BOOL BindCtx_ContainsObject(_In_ IBindCtx *pBindCtx, _In_ LPCWSTR pszName);
DWORD BindCtx_GetMode(_In_ IBindCtx *pbc, _In_ DWORD dwDefault);
BOOL SHSkipJunctionBinding(_In_ IBindCtx *pbc, _In_ CLSID *pclsid);
HRESULT SHIsFileSysBindCtx(_In_ IBindCtx *pBindCtx, _Out_opt_ WIN32_FIND_DATAW *pFindData);
BOOL Shell_FailForceReturn(_In_ HRESULT hr);

EXTERN_C INT
Shell_ParseSpecialFolder(_In_ LPCWSTR pszStart, _Out_ LPWSTR *ppch, _Out_ INT *pcch);

HRESULT
Shell_DisplayNameOf(
    _In_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwFlags,
    _Out_ LPWSTR pszBuf,
    _In_ UINT cchBuf);

HRESULT SHBindToObject(
    _In_opt_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ REFIID riid,
    _Out_ void **ppvObj);

HRESULT
SHBindToObjectEx(
    _In_opt_ IShellFolder *pShellFolder,
    _In_ LPCITEMIDLIST pidl,
    _In_opt_ IBindCtx *pBindCtx,
    _In_ REFIID riid,
    _Out_ void **ppvObj);

DWORD
SHGetAttributes(_In_ IShellFolder *psf, _In_ LPCITEMIDLIST pidl, _In_ DWORD dwAttributes);
HRESULT SHCoInitializeAnyApartment(VOID);

HRESULT
SHGetNameAndFlagsW(
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwFlags,
    _Out_opt_ LPWSTR pszText,
    _In_ UINT cchBuf,
    _Inout_opt_ DWORD *pdwAttributes);

EXTERN_C HWND BindCtx_GetUIWindow(_In_ IBindCtx *pBindCtx);

EXTERN_C HRESULT
BindCtx_RegisterObjectParam(
    _In_ IBindCtx *pBindCtx,
    _In_ LPOLESTR pszKey,
    _In_opt_ IUnknown *punk,
    _Out_ LPBC *ppbc);

BOOL PathIsDotOrDotDotW(_In_ LPCWSTR pszPath);
BOOL PathIsValidElement(_In_ LPCWSTR pszPath);
BOOL PathIsDosDevice(_In_ LPCWSTR pszName);
HRESULT SHILAppend(_Inout_ LPITEMIDLIST pidl, _Inout_ LPITEMIDLIST *ppidl);

#endif /* _PRECOMP_H__ */
