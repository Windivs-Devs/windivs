/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
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

class CGlobalFolderSettings :
    public CComCoClass<CGlobalFolderSettings, &CLSID_GlobalFolderSettings>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IGlobalFolderSettings
{
private:
public:
    CGlobalFolderSettings();
    ~CGlobalFolderSettings();

    // *** IGlobalFolderSettings methods ***
    STDMETHOD(Get)(DEFFOLDERSETTINGS *paramC, int param10) override;
    STDMETHOD(Set)(const DEFFOLDERSETTINGS *paramC, int param10, unsigned int param14) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_GLOBALFOLDERSETTINGS)
    DECLARE_NOT_AGGREGATABLE(CGlobalFolderSettings)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CGlobalFolderSettings)
        COM_INTERFACE_ENTRY_IID(IID_IGlobalFolderSettings, IGlobalFolderSettings)
    END_COM_MAP()
};
