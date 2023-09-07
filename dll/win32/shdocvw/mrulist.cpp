/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement MRU List of shdocvw.dll
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <oleauto.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <strsafe.h>
#include "shdocvw.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

extern "C" void __cxa_pure_virtual(void)
{
    ::DebugBreak();
}

BOOL IEILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL bUnknown)
{
    UINT cb1 = ILGetSize(pidl1), cb2 = ILGetSize(pidl2);
    if (cb1 == cb2 && memcmp(pidl1, pidl2, cb1) == 0)
        return TRUE;

    FIXME("%p, %p\n", pidl1, pidl2);
    return FALSE;
}

// The flags for SLOTITEMDATA.dwFlags
#define SLOT_LOADED         0x1
#define SLOT_SET            0x2

// The flags for CMruBase.m_dwFlags
#define COMPARE_BY_MEMCMP       0x0
#define COMPARE_BY_STRCMPIW     0x1
#define COMPARE_BY_STRCMPW      0x2
#define COMPARE_BY_IEILISEQUAL  0x3
#define COMPARE_BY_MASK         0xF

class CMruBase
    : public IMruDataList
{
protected:
    LONG            m_cRefs         = 1;        // Reference count
    DWORD           m_dwFlags       = 0;        // The COMPARE_BY_... flags
    BOOL            m_bNeedSave     = FALSE;    // ???
    BOOL            m_bChecked      = FALSE;    // The checked flag
    HKEY            m_hKey          = NULL;     // A registry key
    DWORD           m_cSlotRooms    = 0;        // Rooms for slots
    DWORD           m_cSlots        = 0;        // The # of slots
    SLOTCOMPARE     m_fnCompare     = NULL;     // The comparison function
    SLOTITEMDATA *  m_pSlots        = NULL;     // Slot data

    HRESULT _LoadItem(UINT iSlot);
    HRESULT _AddItem(UINT iSlot, const BYTE *pbData, DWORD cbData);
    HRESULT _GetItem(UINT iSlot, SLOTITEMDATA **ppItem);
    void _DeleteItem(UINT iSlot);

    HRESULT _GetSlotItem(UINT iSlot, SLOTITEMDATA **ppItem);
    void _CheckUsedSlots();
    HRESULT _UseEmptySlot(UINT *piSlot);

public:
    CMruBase();
    virtual ~CMruBase();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ::InterlockedIncrement(&m_cRefs);
    }
    STDMETHODIMP_(ULONG) Release() override;

    // IMruDataList methods
    STDMETHODIMP InitData(UINT cCapacity, UINT flags, HKEY hKey,
                          LPCWSTR pszSubKey OPTIONAL,
                          SLOTCOMPARE fnCompare OPTIONAL) override;
    STDMETHODIMP AddData(const BYTE *pbData, DWORD cbData, UINT *piSlot) override;
    STDMETHODIMP FindData(const BYTE *pbData, DWORD cbData, UINT *piSlot) override;
    STDMETHODIMP GetData(UINT iSlot, BYTE *pbData, DWORD cbData) override;
    STDMETHODIMP QueryInfo(UINT iSlot, UINT *puSlot, DWORD *pcbData) override;
    STDMETHODIMP Delete(UINT iSlot) override;

    // Non-standard methods
    virtual BOOL _IsEqual(const SLOTITEMDATA *pItem, LPCVOID pvData, UINT cbData) const;
    virtual HRESULT _DeleteValue(LPCWSTR pszValue);
    virtual HRESULT _InitSlots() = 0;
    virtual void _SaveSlots() = 0;
    virtual UINT _UpdateSlots(UINT iSlot) = 0;
    virtual void _SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch) = 0;
    virtual HRESULT _GetSlot(UINT iSlot, UINT *puSlot) = 0;
    virtual HRESULT _RemoveSlot(UINT iSlot, UINT *puSlot) = 0;

    static void* operator new(size_t size)
    {
        return ::LocalAlloc(LPTR, size);
    }
    static void operator delete(void *ptr)
    {
        ::LocalFree(ptr);
    }
};

CMruBase::CMruBase()
{
    ::InterlockedIncrement(&SHDOCVW_refCount);
}

CMruBase::~CMruBase()
{
    if (m_hKey)
    {
        ::RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

    if (m_pSlots)
    {
        for (UINT iSlot = 0; iSlot < m_cSlots; ++iSlot)
        {
            m_pSlots[iSlot].pvData = ::LocalFree(m_pSlots[iSlot].pvData);
        }

        ::LocalFree(m_pSlots);
        m_pSlots = NULL;
    }

    ::InterlockedDecrement(&SHDOCVW_refCount);
}

STDMETHODIMP CMruBase::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (IsEqualGUID(riid, IID_IMruDataList) || IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IMruDataList*>(this);
        AddRef();
        return S_OK;
    }
    ERR("%s: E_NOINTERFACE\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CMruBase::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        _SaveSlots();
        delete this;
        return 0;
    }
    return m_cRefs;
}

HRESULT CMruBase::_LoadItem(UINT iSlot)
{
    DWORD cbData;
    WCHAR szValue[12];

    SLOTITEMDATA *pItem = &m_pSlots[iSlot];
    _SlotString(iSlot, szValue, _countof(szValue));

    if (SHGetValueW(m_hKey, NULL, szValue, NULL, NULL, &cbData) == ERROR_SUCCESS &&
        cbData > 0)
    {
        pItem->pvData = ::LocalAlloc(LPTR, cbData);
        if (pItem->pvData)
        {
            pItem->cbData = cbData;
            if (SHGetValueW(m_hKey, NULL, szValue, NULL, pItem->pvData, &cbData) != ERROR_SUCCESS)
                pItem->pvData = ::LocalFree(pItem->pvData);
        }
    }

    pItem->dwFlags |= SLOT_LOADED;
    if (!pItem->pvData)
        return E_FAIL;

    return S_OK;
}

HRESULT CMruBase::_GetSlotItem(UINT iSlot, SLOTITEMDATA **ppItem)
{
    if (!(m_pSlots[iSlot].dwFlags & SLOT_LOADED))
        _LoadItem(iSlot);

    SLOTITEMDATA *pItem = &m_pSlots[iSlot];
    if (!pItem->pvData)
        return E_OUTOFMEMORY;

    *ppItem = pItem;
    return S_OK;
}

HRESULT CMruBase::_GetItem(UINT iSlot, SLOTITEMDATA **ppItem)
{
    HRESULT hr = _GetSlot(iSlot, &iSlot);
    if (FAILED(hr))
        return hr;
    return _GetSlotItem(iSlot, ppItem);
}

void CMruBase::_DeleteItem(UINT iSlot)
{
    WCHAR szBuff[12];

    _SlotString(iSlot, szBuff, _countof(szBuff));
    _DeleteValue(szBuff);

    m_pSlots[iSlot].pvData = ::LocalFree(m_pSlots[iSlot].pvData);
}

void CMruBase::_CheckUsedSlots()
{
    UINT iGotSlot;
    for (UINT iSlot = 0; iSlot < m_cSlots; ++iSlot)
        _GetSlot(iSlot, &iGotSlot);

    m_bChecked = TRUE;
}

HRESULT CMruBase::_AddItem(UINT iSlot, const BYTE *pbData, DWORD cbData)
{
    SLOTITEMDATA *pItem = &m_pSlots[iSlot];

    WCHAR szBuff[12];
    _SlotString(iSlot, szBuff, _countof(szBuff));

    if (SHSetValueW(m_hKey, NULL, szBuff, REG_BINARY, pbData, cbData) != ERROR_SUCCESS)
        return E_OUTOFMEMORY;

    if (cbData >= pItem->cbData || !pItem->pvData)
    {
        ::LocalFree(pItem->pvData);
        pItem->pvData = ::LocalAlloc(LPTR, cbData);
    }

    if (!pItem->pvData)
        return E_FAIL;

    pItem->cbData = cbData;
    pItem->dwFlags = (SLOT_LOADED | SLOT_SET);
    CopyMemory(pItem->pvData, pbData, cbData);
    return S_OK;
}

STDMETHODIMP
CMruBase::InitData(
    UINT cCapacity,
    UINT flags,
    HKEY hKey,
    LPCWSTR pszSubKey OPTIONAL,
    SLOTCOMPARE fnCompare OPTIONAL)
{
    m_dwFlags = flags;
    m_fnCompare = fnCompare;
    m_cSlotRooms = cCapacity;

    if (pszSubKey)
        ::RegCreateKeyExWrapW(hKey, pszSubKey, 0, NULL, 0, MAXIMUM_ALLOWED, NULL, &m_hKey, NULL);
    else
        m_hKey = SHRegDuplicateHKey(hKey);

    if (!m_hKey)
        return E_FAIL;

    m_pSlots = (SLOTITEMDATA*)::LocalAlloc(LPTR, m_cSlotRooms * sizeof(SLOTITEMDATA));
    if (!m_pSlots)
        return E_OUTOFMEMORY;

    return _InitSlots();
}

STDMETHODIMP CMruBase::AddData(const BYTE *pbData, DWORD cbData, UINT *piSlot)
{
    UINT iSlot;
    HRESULT hr = FindData(pbData, cbData, &iSlot);
    if (FAILED(hr))
    {
        iSlot = _UpdateSlots(m_cSlots);
        hr = _AddItem(iSlot, pbData, cbData);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        iSlot = _UpdateSlots(iSlot);
        hr = S_OK;
    }

    if (piSlot)
        *piSlot = iSlot;

    return hr;
}

STDMETHODIMP CMruBase::FindData(const BYTE *pbData, DWORD cbData, UINT *piSlot)
{
    if (m_cSlots <= 0)
        return E_FAIL;

    UINT iSlot = 0;
    SLOTITEMDATA *pItem;
    while (FAILED(_GetItem(iSlot, &pItem)) || !_IsEqual(pItem, pbData, cbData))
    {
        if (++iSlot >= m_cSlots)
            return E_FAIL;
    }

    *piSlot = iSlot;
    return S_OK;
}

STDMETHODIMP CMruBase::GetData(UINT iSlot, BYTE *pbData, DWORD cbData)
{
    SLOTITEMDATA *pItem;
    HRESULT hr = _GetItem(iSlot, &pItem);
    if (FAILED(hr))
        return hr;

    if (cbData < pItem->cbData)
        return 0x8007007A; // FIXME: Magic number

    CopyMemory(pbData, pItem->pvData, pItem->cbData);
    return hr;
}

STDMETHODIMP CMruBase::QueryInfo(UINT iSlot, UINT *puSlot, DWORD *pcbData)
{
    UINT iGotSlot;
    HRESULT hr = _GetSlot(iSlot, &iGotSlot);
    if (FAILED(hr))
        return hr;

    if (puSlot)
        *puSlot = iGotSlot;

    if (pcbData)
    {
        SLOTITEMDATA *pItem;
        hr = _GetSlotItem(iGotSlot, &pItem);
        if (SUCCEEDED(hr))
            *pcbData = pItem->cbData;
    }

    return hr;
}

STDMETHODIMP CMruBase::Delete(UINT iSlot)
{
    UINT uSlot;
    HRESULT hr = _RemoveSlot(iSlot, &uSlot);
    if (FAILED(hr))
        return hr;

    _DeleteItem(uSlot);
    return hr;
}

BOOL CMruBase::_IsEqual(const SLOTITEMDATA *pItem, LPCVOID pvData, UINT cbData) const
{
    if (m_fnCompare)
        return m_fnCompare(pvData, pItem->pvData, cbData) == 0;

    switch (m_dwFlags & COMPARE_BY_MASK)
    {
        case COMPARE_BY_MEMCMP:
            if (pItem->cbData != cbData)
                return FALSE;
            return memcmp(pvData, pItem->pvData, cbData) == 0;

        case COMPARE_BY_STRCMPIW:
            return StrCmpIW((LPCWSTR)pvData, (LPCWSTR)pItem->pvData) == 0;

        case COMPARE_BY_STRCMPW:
            return StrCmpW((LPCWSTR)pvData, (LPCWSTR)pItem->pvData) == 0;

        case COMPARE_BY_IEILISEQUAL:
            return IEILIsEqual((LPCITEMIDLIST)pvData, (LPCITEMIDLIST)pItem->pvData, FALSE);

        default:
            ERR("0x%08X\n", m_dwFlags);
            return FALSE;
    }
}

HRESULT CMruBase::_DeleteValue(LPCWSTR pszValue)
{
    return SHDeleteValueW(m_hKey, NULL, pszValue);
}

HRESULT CMruBase::_UseEmptySlot(UINT *piSlot)
{
    if (!m_bChecked)
        _CheckUsedSlots();

    if (!m_cSlotRooms)
        return E_FAIL;

    UINT iSlot = 0;
    for (SLOTITEMDATA *pItem = m_pSlots; (pItem->dwFlags & SLOT_SET); ++pItem)
    {
        if (++iSlot >= m_cSlotRooms)
            return E_FAIL;
    }

    m_pSlots[iSlot].dwFlags |= SLOT_SET;
    *piSlot = iSlot;
    ++m_cSlots;

    return S_OK;
}

class CMruShortList
    : public CMruBase
{
protected:
    LPWSTR m_pszSlotData = NULL;

    HRESULT _InitSlots() override;
    void _SaveSlots() override;
    UINT _UpdateSlots(UINT iSlot) override;
    void _SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch) override;
    HRESULT _GetSlot(UINT iSlot, UINT *puSlot) override;
    HRESULT _RemoveSlot(UINT iSlot, UINT *puSlot) override;
    friend class CMruLongList;

public:
    CMruShortList()
    {
    }

    ~CMruShortList() override
    {
        m_pszSlotData = (LPWSTR)::LocalFree(m_pszSlotData);
    }
};

HRESULT CMruShortList::_InitSlots()
{
    DWORD cbData = (m_cSlotRooms + 1) * sizeof(WCHAR);
    m_pszSlotData = (LPWSTR)LocalAlloc(LPTR, cbData);
    if (!m_pszSlotData)
        return E_OUTOFMEMORY;

    if (SHGetValueW(m_hKey, NULL, L"MRUList", NULL, m_pszSlotData, &cbData) == ERROR_SUCCESS)
        m_cSlots = (cbData / sizeof(WCHAR)) - 1;

    m_pszSlotData[m_cSlots] = 0;
    return S_OK;
}

void CMruShortList::_SaveSlots()
{
    if (m_bNeedSave)
    {
        DWORD cbData = (m_cSlots + 1) * sizeof(WCHAR);
        SHSetValueW(m_hKey, NULL, L"MRUList", REG_SZ, m_pszSlotData, cbData);
        m_bNeedSave = FALSE;
    }
}

UINT CMruShortList::_UpdateSlots(UINT iSlot)
{
    UINT iData, cDataToMove = iSlot;

    if (iSlot == m_cSlots)
    {
        if (SUCCEEDED(_UseEmptySlot(&iData)))
        {
            ++cDataToMove;
        }
        else
        {
            iData = m_pszSlotData[m_cSlots - 1] - L'a';
            --cDataToMove;
        }
    }
    else
    {
        iData = m_pszSlotData[iSlot] - L'a';
    }

    if (cDataToMove)
    {
        MoveMemory(m_pszSlotData + 1, m_pszSlotData, cDataToMove * sizeof(WCHAR));
        m_pszSlotData[0] = (WCHAR)(L'a' + iData);
        m_bNeedSave = TRUE;
    }

    return iData;
}

void CMruShortList::_SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch)
{
    if (cch >= 2)
    {
        psz[0] = (WCHAR)(L'a' + dwSlot);
        psz[1] = UNICODE_NULL;
    }
}

HRESULT CMruShortList::_GetSlot(UINT iSlot, UINT *puSlot)
{
    if (iSlot >= m_cSlots)
        return E_FAIL;

    UINT iData = m_pszSlotData[iSlot] - L'a';
    if (iData >= m_cSlotRooms)
        return E_FAIL;

    *puSlot = iData;
    m_pSlots[iData].dwFlags |= SLOT_SET;
    return S_OK;
}

HRESULT CMruShortList::_RemoveSlot(UINT iSlot, UINT *puSlot)
{
    HRESULT hr = _GetSlot(iSlot, puSlot);
    if (FAILED(hr))
        return hr;

    MoveMemory(&m_pszSlotData[iSlot], &m_pszSlotData[iSlot + 1], (m_cSlots - iSlot) * sizeof(WCHAR));
    --m_cSlots;
    m_pSlots->dwFlags &= ~SLOT_SET;
    m_bNeedSave = TRUE;

    return hr;
}

class CMruLongList
    : public CMruBase
{
protected:
    UINT *m_puSlotData = NULL;      // The slot data

    void _ImportShortList();

    HRESULT _InitSlots() override;
    void _SaveSlots() override;
    UINT _UpdateSlots(UINT iSlot) override;
    void _SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch) override;
    HRESULT _GetSlot(UINT iSlot, UINT *puSlot) override;
    HRESULT _RemoveSlot(UINT iSlot, UINT *puSlot) override;

public:
    CMruLongList()
    {
    }

    ~CMruLongList() override
    {
        if (m_puSlotData)
        {
            ::LocalFree(m_puSlotData);
            m_puSlotData = NULL;
        }
    }
};

HRESULT CMruLongList::_InitSlots()
{
    DWORD cbData = (m_cSlotRooms + 1) * sizeof(UINT);
    m_puSlotData = (UINT*)LocalAlloc(LPTR, cbData);
    if (!m_puSlotData)
        return E_OUTOFMEMORY;

    if (SHGetValueW(m_hKey, NULL, L"MRUListEx", NULL, m_puSlotData, &cbData) == ERROR_SUCCESS)
        m_cSlots = (cbData / sizeof(UINT)) - 1;
    else
        _ImportShortList();

    m_puSlotData[m_cSlots] = 0xFFFFFFFF;
    return S_OK;
}

void CMruLongList::_SaveSlots()
{
    if (m_bNeedSave)
    {
        SHSetValueW(m_hKey, NULL, L"MRUListEx", REG_BINARY, m_puSlotData,
                    (m_cSlots + 1) * sizeof(UINT));
        m_bNeedSave = FALSE;
    }
}

UINT CMruLongList::_UpdateSlots(UINT iSlot)
{
    UINT cSlotsToMove, uSlotData;

    cSlotsToMove = iSlot;
    if (iSlot == m_cSlots)
    {
        if (SUCCEEDED(_UseEmptySlot(&uSlotData)))
        {
            ++cSlotsToMove;
        }
        else
        {
            uSlotData = m_puSlotData[m_cSlots - 1];
            --cSlotsToMove;
        }
    }
    else
    {
        uSlotData = m_puSlotData[iSlot];
    }

    if (cSlotsToMove > 0)
    {
        MoveMemory(m_puSlotData + 1, m_puSlotData, cSlotsToMove * sizeof(UINT));
        m_puSlotData[0] = uSlotData;
        m_bNeedSave = TRUE;
    }

    return uSlotData;
}

void CMruLongList::_SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch)
{
    StringCchPrintfW(psz, cch, L"%d", dwSlot);
}

HRESULT CMruLongList::_GetSlot(UINT iSlot, UINT *puSlot)
{
    if (iSlot >= m_cSlots)
        return E_FAIL;

    UINT uSlotData = m_puSlotData[iSlot];
    if (uSlotData >= m_cSlotRooms)
        return E_FAIL;

    *puSlot = uSlotData;
    m_pSlots[uSlotData].dwFlags |= SLOT_SET;
    return S_OK;
}

HRESULT CMruLongList::_RemoveSlot(UINT iSlot, UINT *puSlot)
{
    HRESULT hr = _GetSlot(iSlot, puSlot);
    if (FAILED(hr))
        return hr;

    MoveMemory(&m_puSlotData[iSlot], &m_puSlotData[iSlot + 1], (m_cSlots - iSlot) * sizeof(UINT));
    --m_cSlots;
    m_pSlots[0].dwFlags &= ~SLOT_SET;
    m_bNeedSave = TRUE;

    return hr;
}

void CMruLongList::_ImportShortList()
{
    CMruShortList *pShortList = new CMruShortList();
    if (!pShortList)
        return;

    HRESULT hr = pShortList->InitData(m_cSlotRooms, 0, m_hKey, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        for (;;)
        {
            UINT uSlot;
            hr = pShortList->_GetSlot(m_cSlots, &uSlot);
            if (FAILED(hr))
                break;

            SLOTITEMDATA *pItem;
            hr = pShortList->_GetSlotItem(uSlot, &pItem);
            if (FAILED(hr))
                break;

            _AddItem(uSlot, (const BYTE*)pItem->pvData, pItem->cbData);
            pShortList->_DeleteItem(uSlot);

            m_puSlotData[m_cSlots++] = uSlot;
        }

        m_bNeedSave = TRUE;
    }

    SHDeleteValueW(m_hKey, NULL, L"MRUList");
    pShortList->Release();
}

EXTERN_C HRESULT
CMruLongList_CreateInstance(DWORD_PTR dwUnused1, void **ppv, DWORD_PTR dwUnused3)
{
    UNREFERENCED_PARAMETER(dwUnused1);
    UNREFERENCED_PARAMETER(dwUnused3);

    TRACE("%p %p %p\n", dwUnused1, ppv, dwUnused3);

    if (!ppv)
        return E_POINTER;

    CMruLongList *pMruList = new CMruLongList();
    *ppv = static_cast<IMruDataList*>(pMruList);
    TRACE("%p\n", *ppv);

    return S_OK;
}

class CMruNode
    : public CMruLongList
{
protected:
    UINT m_uSlotData = 0;                   // The slot data
    CMruNode *m_pParent = NULL;             // The parent
    IShellFolder *m_pShellFolder = NULL;    // The shell folder

public:
    CMruNode() { }
    CMruNode(CMruNode *pParent, UINT uSlotData);
    ~CMruNode() override;

    CMruNode *GetParent();
};

CMruNode::CMruNode(CMruNode *pParent, UINT uSlotData)
{
    m_uSlotData = uSlotData;
    m_pParent = pParent;
    pParent->AddRef();
}

CMruNode::~CMruNode()
{
    if (m_pParent)
    {
        m_pParent->Release();
        m_pParent = NULL;
    }

    if (m_pShellFolder)
    {
        m_pShellFolder->Release();
        m_pShellFolder = NULL;
    }
}

CMruNode *CMruNode::GetParent()
{
    if (m_pParent)
        m_pParent->AddRef();
    return m_pParent;
}

class CMruPidlList
    : public IMruPidlList
    , public CMruNode
{
protected:
    LPBYTE m_pbSlots = NULL;        // The data
    DWORD m_cbSlots = 0;            // The data size
    HANDLE m_hMutex = NULL;         // The mutex (for sync)

    BOOL _LoadNodeSlots()
    {
        DWORD cbSlots = m_cbSlots;
        if (SHGetValueW(m_hKey, NULL, L"NodeSlots", NULL, m_pbSlots, &cbSlots) != ERROR_SUCCESS)
            return FALSE;
        m_cbSlots = cbSlots;
        return TRUE;
    }

    void _SaveNodeSlots()
    {
        SHSetValueW(m_hKey, NULL, L"NodeSlots", REG_BINARY, m_pbSlots, m_cbSlots);
    }

public:
    CMruPidlList()
    {
    }

    virtual ~CMruPidlList()
    {
        m_pbSlots = (LPBYTE)::LocalFree(m_pbSlots);
        if (m_hMutex)
        {
            ::CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
    }

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return CMruBase::AddRef();
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        return CMruBase::Release();
    }

    // IMruPidlList methods
    STDMETHODIMP InitList(UINT cMRUSize, HKEY hKey, LPCWSTR pszName) override;
    STDMETHODIMP UsePidl(LPCITEMIDLIST pidl, UINT *puSlots) override;
    STDMETHODIMP QueryPidl(
        LPCITEMIDLIST pidl,
        UINT cSlots,
        UINT *puSlots,
        UINT *pcSlots) override;
    STDMETHODIMP PruneKids(LPCITEMIDLIST pidl) override;
};

STDMETHODIMP CMruPidlList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_POINTER;

    if (::IsEqualGUID(riid, IID_IMruPidlList) || ::IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IMruPidlList*>(this);
        AddRef();
        return S_OK;
    }

    ERR("%s: E_NOINTERFACE\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP CMruPidlList::InitList(UINT cMRUSize, HKEY hKey, LPCWSTR pszName)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruPidlList::UsePidl(LPCITEMIDLIST pidl, UINT *puSlots)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruPidlList::QueryPidl(
    LPCITEMIDLIST pidl,
    UINT cSlots,
    UINT *puSlots,
    UINT *pcSlots)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruPidlList::PruneKids(LPCITEMIDLIST pidl)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

EXTERN_C HRESULT CMruPidlList_CreateInstance(DWORD_PTR dwUnused1, void **ppv, DWORD_PTR dwUnused3)
{
    UNREFERENCED_PARAMETER(dwUnused1);
    UNREFERENCED_PARAMETER(dwUnused3);

    TRACE("%p %p %p\n", dwUnused1, ppv, dwUnused3);

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    CMruPidlList *pMruList = new CMruPidlList();
    if (pMruList == NULL)
        return E_OUTOFMEMORY;

    *ppv = static_cast<IMruPidlList*>(pMruList);
    TRACE("%p\n", *ppv);
    return S_OK;
}
