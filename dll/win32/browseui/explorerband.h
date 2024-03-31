/*
 * ReactOS Explorer
 *
 * Copyright 2016 Sylvain Deverre <deverre dot sylv at gmail dot com>
 * Copyright 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#define WM_USER_SHELLEVENT WM_USER+88
#define WM_USER_FOLDEREVENT WM_USER+88

class CExplorerBand :
    public CComCoClass<CExplorerBand, &CLSID_ExplorerBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IBandNavigate,
    public IWinEventHandler,
    public INamespaceProxy,
    public IDispatch,
    public IDropSource,
    public IDropTarget,
    public CWindowImpl<CExplorerBand, CWindow, CControlWinTraits>
{

private:
    class NodeInfo
    {
    public:
        LPITEMIDLIST absolutePidl;
        LPITEMIDLIST relativePidl;
        BOOL expanded;
    };

    // *** BaseBarSite information ***
    CComPtr<IUnknown> m_pSite;
    CComPtr<IShellFolder> m_pDesktop;

    // *** tree explorer band stuff ***
    BOOL m_fVisible;
    BOOL m_bNavigating;
    BOOL m_bFocused;
    DWORD m_dwBandID;
    BOOL m_isEditing;
    HIMAGELIST m_hImageList;
    HTREEITEM  m_hRoot;
    HTREEITEM  m_oldSelected;
    LPITEMIDLIST m_pidlCurrent;

    // *** notification cookies ***
    DWORD m_adviseCookie;
    ULONG m_shellRegID;

    // *** Drop target information ***
    CComPtr<IDropTarget> m_pDropTarget;
    HTREEITEM m_childTargetNode;
    CComPtr<IDataObject> m_pCurObject;

    void InitializeExplorerBand();
    void DestroyExplorerBand();
    HRESULT ExecuteCommand(CComPtr<IContextMenu>& menu, UINT nCmd);

    // *** notifications handling ***
    BOOL OnTreeItemExpanding(LPNMTREEVIEW pnmtv);
    void OnSelectionChanged(LPNMTREEVIEW pnmtv);
    BOOL OnTreeItemDeleted(LPNMTREEVIEW pnmtv);
    void OnTreeItemDragging(LPNMTREEVIEW pnmtv, BOOL isRightClick);

    // *** ATL event handlers ***
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT ContextMenuHack(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnShellEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    // *** Helper functions ***
    NodeInfo* GetNodeInfo(HTREEITEM hItem);
    HRESULT UpdateBrowser(LPITEMIDLIST pidlGoto);
    HTREEITEM InsertItem(HTREEITEM hParent, IShellFolder *psfParent, LPITEMIDLIST pElt, LPITEMIDLIST pEltRelative, BOOL bSort);
    HTREEITEM InsertItem(HTREEITEM hParent, LPITEMIDLIST pElt, LPITEMIDLIST pEltRelative, BOOL bSort);
    BOOL InsertSubitems(HTREEITEM hItem, NodeInfo *pNodeInfo);
    BOOL NavigateToPIDL(LPITEMIDLIST dest, HTREEITEM *item, BOOL bExpand, BOOL bInsert, BOOL bSelect);
    BOOL DeleteItem(LPITEMIDLIST toDelete);
    BOOL RenameItem(HTREEITEM toRename, LPITEMIDLIST newPidl);
    BOOL RefreshTreePidl(HTREEITEM tree, LPITEMIDLIST pidlParent);
    BOOL NavigateToCurrentFolder();

    // *** Tree item sorting callback ***
    static int CALLBACK CompareTreeItems(LPARAM p1, LPARAM p2, LPARAM p3);

public:
    CExplorerBand();
    virtual ~CExplorerBand();

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IDockingWindow methods ***
    STDMETHOD(CloseDW)(DWORD dwReserved) override;
    STDMETHOD(ResizeBorderDW)(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;
    STDMETHOD(ShowDW)(BOOL fShow) override;

    // *** IDeskBand methods ***
    STDMETHOD(GetBandInfo)(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IInputObject methods ***
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // *** IWinEventHandler methods ***
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

    // *** IBandNavigate methods ***
    STDMETHOD(Select)(long paramC) override;

    // *** INamespaceProxy ***
    STDMETHOD(GetNavigateTarget)(long paramC, long param10, long param14) override;
    STDMETHOD(Invoke)(long paramC) override;
    STDMETHOD(OnSelectionChanged)(long paramC) override;
    STDMETHOD(RefreshFlags)(long paramC, long param10, long param14) override;
    STDMETHOD(CacheItem)(long paramC) override;

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) override;
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) override;
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override;

    // *** IDropTarget methods ***
    STDMETHOD(DragEnter)(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragOver)(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect) override;

    // *** IDropSource methods ***
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState) override;
    STDMETHOD(GiveFeedback)(DWORD dwEffect) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_EXPLORERBAND)
    DECLARE_NOT_AGGREGATABLE(CExplorerBand)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CExplorerBand)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IBandNavigate, IBandNavigate)
        COM_INTERFACE_ENTRY_IID(IID_INamespaceProxy, INamespaceProxy)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
    END_COM_MAP()

    BEGIN_MSG_MAP(CExplorerBand)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_USER_SHELLEVENT, OnShellEvent)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, ContextMenuHack)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        // MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    END_MSG_MAP()
};
