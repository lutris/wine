/*
 *	IContextMenu for items in the shellview
 *
 * Copyright 1998-2000 Juergen Schmied <juergen.schmied@debitel.net>,
 *                                     <juergen.schmied@metronet.de>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "winerror.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "undocshell.h"
#include "shlobj.h"
#include "winreg.h"
#include "prsht.h"

#include "shell32_main.h"
#include "shellfolder.h"

#include "shresdef.h"
#include "shlwapi.h"
#include "aclui.h"
#include "aclapi.h"

#include "wine/heap.h"
#include "wine/debug.h"

/* Small hack: We need to remove DECLSPEC_HIDDEN from the aclui export. */
const GUID IID_ISecurityInformation = {0x965fc360, 0x16ff, 0x11d0, {0x91, 0xcb, 0x0, 0xaa, 0x0, 0xbb, 0xb7, 0x23}};

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* According to https://blogs.msdn.microsoft.com/oldnewthing/20070726-00/?p=25833 */
static const SI_ACCESS access_rights_files[] =
{
    /* General access rights */
    {
        &GUID_NULL,
        FILE_ALL_ACCESS, MAKEINTRESOURCEW(IDS_SECURITY_ALL_ACCESS),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE,
        MAKEINTRESOURCEW(IDS_SECURITY_MODIFY),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
        MAKEINTRESOURCEW(IDS_SECURITY_READ_EXEC),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_READ,
        MAKEINTRESOURCEW(IDS_SECURITY_READ),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_WRITE & ~READ_CONTROL,
        MAKEINTRESOURCEW(IDS_SECURITY_WRITE),
        SI_ACCESS_GENERAL
    },

    /* Advanced permissions */
    { &GUID_NULL, FILE_ALL_ACCESS,       MAKEINTRESOURCEW(IDS_SECURITY_ALL_ACCESS),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_EXECUTE,          MAKEINTRESOURCEW(IDS_SECURITY_EXECUTE),       SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_DATA,        MAKEINTRESOURCEW(IDS_SECURITY_READ_DATA),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_ATTRIBUTES,  MAKEINTRESOURCEW(IDS_SECURITY_READ_ATTR),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_EA,          MAKEINTRESOURCEW(IDS_SECURITY_READ_EX_ATTR),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_DATA,       MAKEINTRESOURCEW(IDS_SECURITY_WRITE_DATA),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_APPEND_DATA,      MAKEINTRESOURCEW(IDS_SECURITY_APPEND_DATA),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_ATTRIBUTES, MAKEINTRESOURCEW(IDS_SECURITY_WRITE_ATTR),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_EA,         MAKEINTRESOURCEW(IDS_SECURITY_WRITE_EX_ATTR), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DELETE,                MAKEINTRESOURCEW(IDS_SECURITY_DELETE),        SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,          MAKEINTRESOURCEW(IDS_SECURITY_READ_PERM),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,             MAKEINTRESOURCEW(IDS_SECURITY_CHANGE_PERM),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,           MAKEINTRESOURCEW(IDS_SECURITY_CHANGE_OWNER),  SI_ACCESS_SPECIFIC },
};

static const SI_ACCESS access_rights_directories[] =
{
    /* General access rights */
    {
        &GUID_NULL,
        FILE_ALL_ACCESS,
        MAKEINTRESOURCEW(IDS_SECURITY_ALL_ACCESS),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE,
        MAKEINTRESOURCEW(IDS_SECURITY_MODIFY),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
        MAKEINTRESOURCEW(IDS_SECURITY_DIR_LIST),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_READ,
        MAKEINTRESOURCEW(IDS_SECURITY_READ),
        SI_ACCESS_GENERAL
    },
    {
        &GUID_NULL,
        FILE_GENERIC_WRITE & ~READ_CONTROL,
        MAKEINTRESOURCEW(IDS_SECURITY_WRITE),
        SI_ACCESS_GENERAL
    },

    /* Advanced permissions */
    { &GUID_NULL, FILE_ALL_ACCESS,       MAKEINTRESOURCEW(IDS_SECURITY_ALL_ACCESS),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_TRAVERSE,         MAKEINTRESOURCEW(IDS_SECURITY_TRAVERSE),      SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_LIST_DIRECTORY,   MAKEINTRESOURCEW(IDS_SECURITY_LIST_FOLDER),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_ATTRIBUTES,  MAKEINTRESOURCEW(IDS_SECURITY_READ_ATTR),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_EA,          MAKEINTRESOURCEW(IDS_SECURITY_READ_EX_ATTR),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_ADD_FILE,         MAKEINTRESOURCEW(IDS_SECURITY_CREATE_FILES),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_ADD_SUBDIRECTORY, MAKEINTRESOURCEW(IDS_SEUCRITY_CREATE_FOLDER), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_ATTRIBUTES, MAKEINTRESOURCEW(IDS_SECURITY_WRITE_ATTR),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_EA,         MAKEINTRESOURCEW(IDS_SECURITY_WRITE_EX_ATTR), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_DELETE_CHILD,     MAKEINTRESOURCEW(IDS_SECURITY_DELETE_CHILD),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DELETE,                MAKEINTRESOURCEW(IDS_SECURITY_DELETE),        SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,          MAKEINTRESOURCEW(IDS_SECURITY_READ_PERM),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,             MAKEINTRESOURCEW(IDS_SECURITY_CHANGE_PERM),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,           MAKEINTRESOURCEW(IDS_SECURITY_CHANGE_OWNER),  SI_ACCESS_SPECIFIC },
};

struct FileSecurity
{
    ISecurityInformation ISecurityInformation_iface;
    LONG ref;
    WCHAR *path;
    BOOL directory;
};

static inline struct FileSecurity *impl_from_ISecurityInformation(ISecurityInformation *iface)
{
    return CONTAINING_RECORD(iface, struct FileSecurity, ISecurityInformation_iface);
}

typedef struct
{
    IContextMenu3 IContextMenu3_iface;
    IShellExtInit IShellExtInit_iface;
    IObjectWithSite IObjectWithSite_iface;
    LONG ref;

    IShellFolder* parent;

    /* item menu data */
    LPITEMIDLIST  pidl;  /* root pidl */
    LPITEMIDLIST *apidl; /* array of child pidls */
    UINT cidl;
    BOOL allvalues;

    /* background menu data */
    BOOL desktop;
} ContextMenu;

static BOOL DoPaste(ContextMenu *This);

static inline ContextMenu *impl_from_IContextMenu3(IContextMenu3 *iface)
{
    return CONTAINING_RECORD(iface, ContextMenu, IContextMenu3_iface);
}

static inline ContextMenu *impl_from_IShellExtInit(IShellExtInit *iface)
{
    return CONTAINING_RECORD(iface, ContextMenu, IShellExtInit_iface);
}

static inline ContextMenu *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, ContextMenu, IObjectWithSite_iface);
}

static HRESULT WINAPI ContextMenu_QueryInterface(IContextMenu3 *iface, REFIID riid, LPVOID *ppvObj)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)      ||
        IsEqualIID(riid, &IID_IContextMenu)  ||
        IsEqualIID(riid, &IID_IContextMenu2) ||
        IsEqualIID(riid, &IID_IContextMenu3))
    {
        *ppvObj = &This->IContextMenu3_iface;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppvObj = &This->IShellExtInit_iface;
    }
    else if (IsEqualIID(riid, &IID_IObjectWithSite))
    {
        *ppvObj = &This->IObjectWithSite_iface;
    }

    if(*ppvObj)
    {
        IContextMenu3_AddRef(iface);
        return S_OK;
    }

    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ContextMenu_AddRef(IContextMenu3 *iface)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI ContextMenu_Release(IContextMenu3 *iface)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (!ref)
    {
        if(This->parent)
            IShellFolder_Release(This->parent);

        SHFree(This->pidl);
        _ILFreeaPidl(This->apidl, This->cidl);

        heap_free(This);
    }

    return ref;
}

static BOOL CheckClipboard(void)
{
    IDataObject *pda;
    BOOL ret = FALSE;

    if (SUCCEEDED(OleGetClipboard(&pda)))
    {
        STGMEDIUM medium;
        FORMATETC formatetc;

        /* Set the FORMATETC structure*/
        InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLISTW), TYMED_HGLOBAL);

        /* Get the pidls from IDataObject */
        if (SUCCEEDED(IDataObject_GetData(pda, &formatetc, &medium)))
        {
            ReleaseStgMedium(&medium);
            ret = TRUE;
        }
        IDataObject_Release(pda);
    }
    return ret;
}

static HRESULT WINAPI ItemMenu_QueryContextMenu(
	IContextMenu3 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    INT uIDMax;
    DWORD attr = SFGAO_CANRENAME;

    TRACE("(%p)->(%p %d 0x%x 0x%x 0x%x )\n", This, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if(!(CMF_DEFAULTONLY & uFlags) && This->cidl > 0)
    {
        HMENU hmenures = LoadMenuW(shell32_hInstance, MAKEINTRESOURCEW(MENU_SHV_FILE));

        if(uFlags & CMF_EXPLORE)
            RemoveMenu(hmenures, FCIDM_SHVIEW_OPEN, MF_BYCOMMAND);

        uIDMax = Shell_MergeMenus(hmenu, GetSubMenu(hmenures, 0), indexMenu, idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);

        DestroyMenu(hmenures);

        if(This->allvalues)
        {
            MENUITEMINFOW mi;
            WCHAR str[255];
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
            mi.dwTypeData = str;
            mi.cch = 255;
            GetMenuItemInfoW(hmenu, FCIDM_SHVIEW_EXPLORE, MF_BYCOMMAND, &mi);
            RemoveMenu(hmenu, FCIDM_SHVIEW_EXPLORE + idCmdFirst, MF_BYCOMMAND);

            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_STRING;
            mi.dwTypeData = str;
            mi.fState = MFS_ENABLED;
            mi.wID = FCIDM_SHVIEW_EXPLORE;
            mi.fType = MFT_STRING;
            InsertMenuItemW(hmenu, (uFlags & CMF_EXPLORE) ? 1 : 2, MF_BYPOSITION, &mi);
        }

        SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);

        if (This->apidl && This->cidl == 1)
            IShellFolder_GetAttributesOf(This->parent, 1, (LPCITEMIDLIST*)This->apidl, &attr);

        if(uFlags & ~CMF_CANRENAME)
            RemoveMenu(hmenu, FCIDM_SHVIEW_RENAME, MF_BYCOMMAND);
        else
        {
            UINT enable = MF_BYCOMMAND;

            /* can't rename more than one item at a time*/
            if (!This->apidl || This->cidl > 1)
                enable |= MFS_DISABLED;
            else
                enable |= (attr & SFGAO_CANRENAME) ? MFS_ENABLED : MFS_DISABLED;

            EnableMenuItem(hmenu, FCIDM_SHVIEW_RENAME, enable);
        }

        if ((attr & (SFGAO_FILESYSTEM|SFGAO_FOLDER)) != (SFGAO_FILESYSTEM|SFGAO_FOLDER) || !CheckClipboard())
            RemoveMenu(hmenu, FCIDM_SHVIEW_INSERT + idCmdFirst, MF_BYCOMMAND);

        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, uIDMax-idCmdFirst);
    }
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}

/**************************************************************************
* DoOpenExplore
*
*  for folders only
*/

static void DoOpenExplore(ContextMenu *This, HWND hwnd, LPCSTR verb)
{
        UINT i;
        BOOL bFolderFound = FALSE;
	LPITEMIDLIST	pidlFQ;
	SHELLEXECUTEINFOA	sei;

	/* Find the first item in the list that is not a value. These commands
	    should never be invoked if there isn't at least one folder item in the list.*/

	for(i = 0; i<This->cidl; i++)
	{
	  if(!_ILIsValue(This->apidl[i]))
	  {
	    bFolderFound = TRUE;
	    break;
	  }
	}

	if (!bFolderFound) return;

	pidlFQ = ILCombine(This->pidl, This->apidl[i]);

	ZeroMemory(&sei, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
	sei.lpIDList = pidlFQ;
	sei.lpClass = "Folder";
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = verb;
	ShellExecuteExA(&sei);
	SHFree(pidlFQ);
}

/**************************************************************************
 * DoDelete
 *
 * deletes the currently selected items
 */
static void DoDelete(ContextMenu *This)
{
    ISFHelper *helper;

    IShellFolder_QueryInterface(This->parent, &IID_ISFHelper, (void**)&helper);
    if (helper)
    {
        ISFHelper_DeleteItems(helper, This->cidl, (LPCITEMIDLIST *)This->apidl, TRUE);
        ISFHelper_Release(helper);
    }
}

/**************************************************************************
 * SetDropEffect
 *
 * Set the drop effect in a IDataObject object
 */
static void SetDropEffect(IDataObject *dataobject, DWORD value)
{
    FORMATETC formatetc;
    STGMEDIUM medium;
    DWORD *effect;

    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW), TYMED_HGLOBAL);

    medium.tymed = TYMED_HGLOBAL;
    medium.pUnkForRelease = NULL;
    medium.u.hGlobal = GlobalAlloc(GHND|GMEM_SHARE, sizeof(DWORD));
    if (!medium.u.hGlobal) return;

    effect = GlobalLock(medium.u.hGlobal);
    if (!effect)
    {
        ReleaseStgMedium(&medium);
        return;
    }
    *effect = value;
    GlobalUnlock(effect);

    IDataObject_SetData(dataobject, &formatetc, &medium, FALSE);
    ReleaseStgMedium(&medium);
}

/**************************************************************************
 * GetDropEffect
 *
 * Get the drop effect from a IDataObject object
 */
static void GetDropEffect(IDataObject *dataobject, DWORD *value)
{
    FORMATETC formatetc;
    STGMEDIUM medium;
    DWORD *effect;

    *value = DROPEFFECT_NONE;

    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW), TYMED_HGLOBAL);

    if (SUCCEEDED(IDataObject_GetData(dataobject, &formatetc, &medium)))
    {
        effect = GlobalLock(medium.u.hGlobal);
        if (effect)
        {
            *value = *effect;
            GlobalUnlock(effect);
        }
        ReleaseStgMedium(&medium);
    }
}

/**************************************************************************
 * DoCopyOrCut
 *
 * copies the currently selected items into the clipboard
 */
static void DoCopyOrCut(ContextMenu *This, HWND hwnd, BOOL cut)
{
    IDataObject *dataobject;

    TRACE("(%p)->(wnd=%p, cut=%d)\n", This, hwnd, cut);

    if (SUCCEEDED(IShellFolder_GetUIObjectOf(This->parent, hwnd, This->cidl, (LPCITEMIDLIST*)This->apidl, &IID_IDataObject, 0, (void**)&dataobject)))
    {
        SetDropEffect(dataobject, cut ? DROPEFFECT_MOVE : DROPEFFECT_COPY);
        OleSetClipboard(dataobject);
        IDataObject_Release(dataobject);
    }
}

/**************************************************************************
 * Properties_AddPropSheetCallback
 *
 * Used by DoOpenProperties through SHCreatePropSheetExtArrayEx to add
 * propertysheet pages from shell extensions.
 */
static BOOL CALLBACK Properties_AddPropSheetCallback(HPROPSHEETPAGE hpage, LPARAM lparam)
{
	LPPROPSHEETHEADERW psh = (LPPROPSHEETHEADERW) lparam;
	psh->u3.phpage[psh->nPages++] = hpage;

	return TRUE;
}

static BOOL format_date(FILETIME *time, WCHAR *buffer, DWORD size)
{
    FILETIME ft;
    SYSTEMTIME st;
    int ret;

    if (!FileTimeToLocalFileTime(time, &ft))
        return FALSE;

    if (!FileTimeToSystemTime(&ft, &st))
        return FALSE;

    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, buffer, size);
    if (ret)
    {
        buffer[ret - 1] = ' ';
        ret = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, buffer + ret , size - ret);
    }
    return ret != 0;
}

static BOOL get_program_description(WCHAR *path, WCHAR *buffer, DWORD size)
{
    static const WCHAR translationW[] = {
        '\\','V','a','r','F','i','l','e','I','n','f','o',
        '\\','T','r','a','n','s','l','a','t','i','o','n',0
    };
    static const WCHAR fileDescFmtW[] = {
        '\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
        '\\','%','0','4','x','%','0','4','x',
        '\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0
    };
    WCHAR fileDescW[41], *desc;
    DWORD versize, *lang;
    UINT dlen, llen, i;
    BOOL ret = FALSE;
    PVOID data;

    versize = GetFileVersionInfoSizeW(path, NULL);
    if (!versize) return FALSE;

    data = heap_alloc(versize);
    if (!data) return FALSE;

    if (!GetFileVersionInfoW(path, 0, versize, data))
        goto out;

    if (!VerQueryValueW(data, translationW, (LPVOID *)&lang, &llen))
        goto out;

    for (i = 0; i < llen / sizeof(DWORD); i++)
    {
        sprintfW(fileDescW, fileDescFmtW, LOWORD(lang[i]), HIWORD(lang[i]));
        if (VerQueryValueW(data, fileDescW, (LPVOID *)&desc, &dlen))
        {
            if (dlen > size - 1) dlen = size - 1;
            memcpy(buffer, desc, dlen * sizeof(WCHAR));
            buffer[dlen] = 0;
            ret = TRUE;
            break;
        }
    }

out:
    heap_free(data);
    return ret;
}

struct file_properties_info
{
    WCHAR path[MAX_PATH];
    WCHAR dir[MAX_PATH];
    WCHAR *filename;
    DWORD attrib;
};

static void init_file_properties_dlg(HWND hwndDlg, struct file_properties_info *props)
{
    WCHAR buffer[MAX_PATH], buffer2[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA exinfo;
    SHFILEINFOW shinfo;

    SetDlgItemTextW(hwndDlg, IDC_FPROP_PATH, props->filename);
    SetDlgItemTextW(hwndDlg, IDC_FPROP_LOCATION, props->dir);

    if (SHGetFileInfoW(props->path, 0, &shinfo, sizeof(shinfo), SHGFI_TYPENAME|SHGFI_ICON))
    {
        if (shinfo.hIcon)
        {
            SendDlgItemMessageW(hwndDlg, IDC_FPROP_ICON, STM_SETICON, (WPARAM)shinfo.hIcon, 0);
            DestroyIcon(shinfo.hIcon);
        }
        if (shinfo.szTypeName[0])
            SetDlgItemTextW(hwndDlg, IDC_FPROP_TYPE, shinfo.szTypeName);
    }

    if (!GetFileAttributesExW(props->path, GetFileExInfoStandard, &exinfo))
        return;

    if (format_date(&exinfo.ftCreationTime, buffer, ARRAY_SIZE(buffer)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_CREATED, buffer);

    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_READONLY, BM_SETCHECK, BST_CHECKED, 0);
    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_HIDDEN, BM_SETCHECK, BST_CHECKED, 0);
    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_ARCHIVE, BM_SETCHECK, BST_CHECKED, 0);

    if (exinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        static const WCHAR unknownW[] = {'(','u','n','k','n','o','w','n',')',0};
        SetDlgItemTextW(hwndDlg, IDC_FPROP_SIZE, unknownW);

        /* TODO: Implement counting for directories */
        return;
    }

    /* Information about files only */
    StrFormatByteSizeW(((LONGLONG)exinfo.nFileSizeHigh << 32) | exinfo.nFileSizeLow,
                       buffer, ARRAY_SIZE(buffer));
    SetDlgItemTextW(hwndDlg, IDC_FPROP_SIZE, buffer);

    if (format_date(&exinfo.ftLastWriteTime, buffer, ARRAY_SIZE(buffer)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_MODIFIED, buffer);
    if (format_date(&exinfo.ftLastAccessTime, buffer, ARRAY_SIZE(buffer)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_ACCESSED, buffer);

    if (FindExecutableW(props->path, NULL, buffer) <= (HINSTANCE)32)
        return;

    /* Information about executables */
    if (SHGetFileInfoW(buffer, 0, &shinfo, sizeof(shinfo), SHGFI_ICON | SHGFI_SMALLICON) && shinfo.hIcon)
        SendDlgItemMessageW(hwndDlg, IDC_FPROP_PROG_ICON, STM_SETICON, (WPARAM)shinfo.hIcon, 0);

    if (get_program_description(buffer, buffer2, ARRAY_SIZE(buffer2)))
        SetDlgItemTextW(hwndDlg, IDC_FPROP_PROG_NAME, buffer2);
    else
    {
        WCHAR *p = strrchrW(buffer, '\\');
        SetDlgItemTextW(hwndDlg, IDC_FPROP_PROG_NAME, p ? ++p : buffer);
    }
}

static INT_PTR CALLBACK file_properties_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW page = (LPPROPSHEETPAGEW)lParam;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)page->lParam);
            init_file_properties_dlg(hwndDlg, (struct file_properties_info *)page->lParam);
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_FPROP_PROG_CHANGE)
            {
                /* TODO: Implement file association dialog */
                MessageBoxA(hwndDlg, "Not implemented yet.", "Error", MB_OK | MB_ICONEXCLAMATION);
            }
            else if (LOWORD(wParam) == IDC_FPROP_READONLY ||
                     LOWORD(wParam) == IDC_FPROP_HIDDEN ||
                     LOWORD(wParam) == IDC_FPROP_ARCHIVE)
            {
                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            }
            else if (LOWORD(wParam) == IDC_FPROP_PATH && HIWORD(wParam) == EN_CHANGE)
            {
                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            }
            break;

        case WM_NOTIFY:
            {
                LPPSHNOTIFY notify = (LPPSHNOTIFY)lParam;
                if (notify->hdr.code == PSN_APPLY)
                {
                    struct file_properties_info *props = (struct file_properties_info *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                    WCHAR newname[MAX_PATH], newpath[MAX_PATH];
                    DWORD attributes;

                    attributes = GetFileAttributesW(props->path);
                    if (attributes != INVALID_FILE_ATTRIBUTES)
                    {
                        attributes &= ~(FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_ARCHIVE);

                        if (SendDlgItemMessageW(hwndDlg, IDC_FPROP_READONLY, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            attributes |= FILE_ATTRIBUTE_READONLY;
                        if (SendDlgItemMessageW(hwndDlg, IDC_FPROP_HIDDEN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            attributes |= FILE_ATTRIBUTE_HIDDEN;
                        if (SendDlgItemMessageW(hwndDlg, IDC_FPROP_ARCHIVE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            attributes |= FILE_ATTRIBUTE_ARCHIVE;

                        if (!SetFileAttributesW(props->path, attributes))
                            ERR("failed to update file attributes of %s\n", debugstr_w(props->path));
                    }

                    /* Update filename if it was changed */
                    if (GetDlgItemTextW(hwndDlg, IDC_FPROP_PATH, newname, ARRAY_SIZE(newname)) &&
                        strcmpW(props->filename, newname) &&
                        strlenW(props->dir) + strlenW(newname) + 2 < ARRAY_SIZE(newpath))
                    {
                        static const WCHAR slash[] = {'\\', 0};
                        strcpyW(newpath, props->dir);
                        strcatW(newpath, slash);
                        strcatW(newpath, newname);

                        if (!MoveFileW(props->path, newpath))
                            ERR("failed to move file %s to %s\n", debugstr_w(props->path), debugstr_w(newpath));
                        else
                        {
                            WCHAR *p;
                            strcpyW(props->path, newpath);
                            strcpyW(props->dir, newpath);
                            if ((p = strrchrW(props->dir, '\\')))
                            {
                                *p = 0;
                                props->filename = p + 1;
                            }
                            else
                                props->filename = props->dir;
                            SetDlgItemTextW(hwndDlg, IDC_FPROP_LOCATION, props->dir);
                        }
                    }

                    return TRUE;
                }
            }
            break;

        default:
            break;
    }
    return FALSE;
}

static UINT CALLBACK file_properties_callback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGEW page)
{
    struct file_properties_info *props = (struct file_properties_info *)page->lParam;
    if (uMsg == PSPCB_RELEASE)
    {
        heap_free(props);
    }
    return 1;
}

static void init_file_properties_pages(IDataObject *dataobject, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    struct file_properties_info *props;
    HPROPSHEETPAGE general_page;
    PROPSHEETPAGEW propsheet;
    FORMATETC format;
    STGMEDIUM stgm;
    HRESULT hr;
    WCHAR *p;

    props = heap_alloc(sizeof(*props));
    if (!props) return;

    format.cfFormat = CF_HDROP;
    format.ptd      = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex   = -1;
    format.tymed    = TYMED_HGLOBAL;

    hr = IDataObject_GetData(dataobject, &format, &stgm);
    if (FAILED(hr)) goto error;

    if (!DragQueryFileW((HDROP)stgm.u.hGlobal, 0, props->path, ARRAY_SIZE(props->path)))
    {
        ReleaseStgMedium(&stgm);
        goto error;
    }

    ReleaseStgMedium(&stgm);

    props->attrib = GetFileAttributesW(props->path);
    if (props->attrib == INVALID_FILE_ATTRIBUTES)
        goto error;

    strcpyW(props->dir, props->path);
    if ((p = strrchrW(props->dir, '\\')))
    {
        *p = 0;
        props->filename = p + 1;
    }
    else
        props->filename = props->dir;

    memset(&propsheet, 0, sizeof(propsheet));
    propsheet.dwSize        = sizeof(propsheet);
    propsheet.dwFlags       = PSP_DEFAULT | PSP_USECALLBACK;
    propsheet.hInstance     = shell32_hInstance;
    if (props->attrib & FILE_ATTRIBUTE_DIRECTORY)
        propsheet.u.pszTemplate = (LPWSTR)MAKEINTRESOURCE(IDD_FOLDER_PROPERTIES);
    else
        propsheet.u.pszTemplate = (LPWSTR)MAKEINTRESOURCE(IDD_FILE_PROPERTIES);
    propsheet.pfnDlgProc    = file_properties_proc;
    propsheet.pfnCallback   = file_properties_callback;
    propsheet.lParam        = (LPARAM)props;

    general_page = CreatePropertySheetPageW(&propsheet);
    if (general_page)
        lpfnAddPage(general_page, lParam);
    return;

error:
    heap_free(props);
}

static HRESULT WINAPI filesecurity_QueryInterface(ISecurityInformation *iface, REFIID riid, void **ppv)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->ISecurityInformation_iface;
    }
    else if (IsEqualGUID(&IID_ISecurityInformation, riid))
    {
        TRACE("(%p)->(IID_ISecurityInformation %p)\n", This, ppv);
        *ppv = &This->ISecurityInformation_iface;
    }
    else
    {
        *ppv = NULL;
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI filesecurity_AddRef(ISecurityInformation *iface)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI filesecurity_Release(ISecurityInformation *iface)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);
    ULONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->path);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI filesecurity_GetObjectInformation(ISecurityInformation *iface, SI_OBJECT_INFO *info)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    TRACE("(%p, %p)\n", This, info);

    info->dwFlags = SI_ADVANCED;
    info->hInstance = shell32_hInstance;
    info->pszServerName = NULL;
    info->pszObjectName = This->path;
    info->pszPageTitle = NULL;
    memcpy(&info->guidObjectType, &GUID_NULL, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI filesecurity_GetSecurity(ISecurityInformation *iface, SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR *sd, BOOL default_sd)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    TRACE("(%p, %u, %p, %u)\n", This, info, sd, default_sd);

    if (default_sd)
        FIXME("Returning a default sd is not implemented\n");

    if (GetNamedSecurityInfoW(This->path, SE_FILE_OBJECT, info, NULL, NULL, NULL, NULL, sd) != ERROR_SUCCESS)
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI filesecurity_SetSecurity(ISecurityInformation *iface, SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR sd)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);
    BOOL present, defaulted;
    PSID owner, group;
    ACL *dacl, *sacl;

    TRACE("(%p, %u, %p)\n", This, info, sd);

    if (!GetSecurityDescriptorOwner(sd, &owner, &defaulted))
        return E_FAIL;

    if (!GetSecurityDescriptorGroup(sd, &group, &defaulted))
        return E_FAIL;

    if (!GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted))
        return E_FAIL;
    if (!present) dacl = NULL;

    if (!GetSecurityDescriptorSacl(sd, &present, &sacl, &defaulted))
        return E_FAIL;
    if (!present) sacl = NULL;

    if (SetNamedSecurityInfoW(This->path, SE_FILE_OBJECT, info, owner, group, dacl, sacl) != ERROR_SUCCESS)
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI filesecurity_GetAccessRights(ISecurityInformation *iface, const GUID* type, DWORD flags, SI_ACCESS **access,
                                                   ULONG *count, ULONG *default_access )
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    TRACE("(%p, %s, %x, %p, %p, %p)\n", This, debugstr_guid(type), flags, access, count, default_access);

    if (This->directory)
    {
        *access = (SI_ACCESS *)access_rights_directories;
        *count = sizeof(access_rights_directories) / sizeof(access_rights_directories[0]);
    }
    else
    {
        *access = (SI_ACCESS *)access_rights_files;
        *count = sizeof(access_rights_files) / sizeof(access_rights_files[0]);
    }

    *default_access = 0;
    return S_OK;
}

static HRESULT WINAPI filesecurity_MapGeneric(ISecurityInformation *iface, const GUID *type, UCHAR *ace_flags, ACCESS_MASK *mask)
{
    static GENERIC_MAPPING file_access_map =
    {
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
    };
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    FIXME("(%p, %s, %p, %p): semi-stub!\n", This, debugstr_guid(type), ace_flags, mask);

    MapGenericMask((DWORD*)mask, &file_access_map);
    return S_OK;
}

static HRESULT WINAPI filesecurity_GetInheritTypes(ISecurityInformation *iface, PSI_INHERIT_TYPE *types, ULONG *count)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    FIXME("(%p, %p, %p): stub!\n", This, types, count);

    *types = NULL;
    *count = 0;

    return S_OK;
}

static HRESULT WINAPI filesecurity_PropertySheetPageCallback(ISecurityInformation *iface, HWND hwnd, UINT msg, SI_PAGE_TYPE page)
{
    struct FileSecurity *This = impl_from_ISecurityInformation(iface);

    TRACE("(%p, %p, %u, %u)\n", This, hwnd, msg, page);
    return S_OK;
}

static const struct ISecurityInformationVtbl filesecurity_vtbl =
{
    /* IUnknown */
    filesecurity_QueryInterface,
    filesecurity_AddRef,
    filesecurity_Release,
    /* ISecurityInformation */
    filesecurity_GetObjectInformation,
    filesecurity_GetSecurity,
    filesecurity_SetSecurity,
    filesecurity_GetAccessRights,
    filesecurity_MapGeneric,
    filesecurity_GetInheritTypes,
    filesecurity_PropertySheetPageCallback,
};

static ISecurityInformation *create_filesecurity_information(WCHAR *path, BOOL directory)
{
    struct FileSecurity *security;
    DWORD len;

    security = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*security));
    if (!security) return NULL;

    security->ISecurityInformation_iface.lpVtbl = &filesecurity_vtbl;
    security->ref = 1;
    security->directory = directory;

    len = (strlenW(path) + 1) * sizeof(WCHAR);
    security->path = HeapAlloc(GetProcessHeap(), 0, len);
    if (!security->path) goto error;

    memcpy(security->path, path, len);
    return &security->ISecurityInformation_iface;

error:
    HeapFree(GetProcessHeap(), 0, security);
    return NULL;
}

static void init_security_properties_pages(IDataObject *pDo, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    ISecurityInformation *security;
    HPROPSHEETPAGE security_page;
    FORMATETC format;
    STGMEDIUM stgm;
    DWORD attrib;
    WCHAR *path;
    UINT len;

    format.cfFormat = CF_HDROP;
    format.ptd      = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex   = -1;
    format.tymed    = TYMED_HGLOBAL;

    if (FAILED(IDataObject_GetData(pDo, &format, &stgm)))
        return;

    if (!(len = DragQueryFileW((HDROP)stgm.DUMMYUNIONNAME.hGlobal, 0, NULL, 0)))
    {
        ReleaseStgMedium(&stgm);
        return;
    }

    if (!(path = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR))))
    {
        ReleaseStgMedium(&stgm);
        return;
    }

    len = DragQueryFileW((HDROP)stgm.DUMMYUNIONNAME.hGlobal, 0, path, len + 1);
    ReleaseStgMedium(&stgm);
    if (!len) goto done;

    attrib = GetFileAttributesW(path);
    if (attrib == INVALID_FILE_ATTRIBUTES)
        goto done;

    if (!(security = create_filesecurity_information(path, attrib & FILE_ATTRIBUTE_DIRECTORY)))
        goto done;

    security_page = CreateSecurityPage(security);
    IUnknown_Release((IUnknown *)security);
    if (!security_page) goto done;

    lpfnAddPage(security_page, lParam);

done:
    HeapFree(GetProcessHeap(), 0, path);
}


#define MAX_PROP_PAGES 99

static void DoOpenProperties(ContextMenu *This, HWND hwnd)
{
	static const WCHAR wszFolder[] = {'F','o','l','d','e','r', 0};
	static const WCHAR wszFiletypeAll[] = {'*',0};
	LPSHELLFOLDER lpDesktopSF;
	LPSHELLFOLDER lpSF;
	LPDATAOBJECT lpDo;
	WCHAR wszFiletype[MAX_PATH];
	WCHAR wszFilename[MAX_PATH];
	PROPSHEETHEADERW psh;
	HPROPSHEETPAGE hpages[MAX_PROP_PAGES];
	HPSXA hpsxa;
	UINT ret;

	TRACE("(%p)->(wnd=%p)\n", This, hwnd);

	ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
	psh.dwSize = sizeof (PROPSHEETHEADERW);
	psh.hwndParent = hwnd;
	psh.dwFlags = PSH_PROPTITLE;
	psh.nPages = 0;
	psh.u3.phpage = hpages;
	psh.u2.nStartPage = 0;

	_ILSimpleGetTextW(This->apidl[0], (LPVOID)wszFilename, MAX_PATH);
	psh.pszCaption = (LPCWSTR)wszFilename;

	/* Find out where to look for the shell extensions */
	if (_ILIsValue(This->apidl[0]))
	{
	    char sTemp[64];
	    sTemp[0] = 0;
	    if (_ILGetExtension(This->apidl[0], sTemp, 64))
	    {
		HCR_MapTypeToValueA(sTemp, sTemp, 64, TRUE);
		MultiByteToWideChar(CP_ACP, 0, sTemp, -1, wszFiletype, MAX_PATH);
	    }
	    else
	    {
		wszFiletype[0] = 0;
	    }
	}
	else if (_ILIsFolder(This->apidl[0]))
	{
	    lstrcpynW(wszFiletype, wszFolder, 64);
	}
	else if (_ILIsSpecialFolder(This->apidl[0]))
	{
	    LPGUID folderGUID;
	    static const WCHAR wszclsid[] = {'C','L','S','I','D','\\', 0};
	    folderGUID = _ILGetGUIDPointer(This->apidl[0]);
	    lstrcpyW(wszFiletype, wszclsid);
	    StringFromGUID2(folderGUID, &wszFiletype[6], MAX_PATH - 6);
	}
	else
	{
	    FIXME("Requested properties for unknown type.\n");
	    return;
	}

	/* Get a suitable DataObject for accessing the files */
	SHGetDesktopFolder(&lpDesktopSF);
	if (_ILIsPidlSimple(This->pidl))
	{
	    ret = IShellFolder_GetUIObjectOf(lpDesktopSF, hwnd, This->cidl, (LPCITEMIDLIST*)This->apidl,
					     &IID_IDataObject, NULL, (LPVOID *)&lpDo);
	    IShellFolder_Release(lpDesktopSF);
	}
	else
	{
	    IShellFolder_BindToObject(lpDesktopSF, This->pidl, NULL, &IID_IShellFolder, (LPVOID*) &lpSF);
	    ret = IShellFolder_GetUIObjectOf(lpSF, hwnd, This->cidl, (LPCITEMIDLIST*)This->apidl,
					     &IID_IDataObject, NULL, (LPVOID *)&lpDo);
	    IShellFolder_Release(lpSF);
	    IShellFolder_Release(lpDesktopSF);
	}

	if (SUCCEEDED(ret))
	{
            init_file_properties_pages(lpDo, Properties_AddPropSheetCallback, (LPARAM)&psh);
            init_security_properties_pages(lpDo, Properties_AddPropSheetCallback, (LPARAM)&psh);

	    hpsxa = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszFiletype, MAX_PROP_PAGES - psh.nPages, lpDo);
	    if (hpsxa != NULL)
	    {
		SHAddFromPropSheetExtArray(hpsxa, Properties_AddPropSheetCallback, (LPARAM)&psh);
		SHDestroyPropSheetExtArray(hpsxa);
	    }
	    hpsxa = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszFiletypeAll, MAX_PROP_PAGES - psh.nPages, lpDo);
	    if (hpsxa != NULL)
	    {
		SHAddFromPropSheetExtArray(hpsxa, Properties_AddPropSheetCallback, (LPARAM)&psh);
		SHDestroyPropSheetExtArray(hpsxa);
	    }
	    IDataObject_Release(lpDo);
	}

	if (psh.nPages)
	    PropertySheetW(&psh);
	else
	    FIXME("No property pages found.\n");
}

static HRESULT WINAPI ItemMenu_InvokeCommand(
	IContextMenu3 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);

    if (lpcmi->cbSize != sizeof(CMINVOKECOMMANDINFO))
        FIXME("Is an EX structure\n");

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

    if (IS_INTRESOURCE(lpcmi->lpVerb) && LOWORD(lpcmi->lpVerb) > FCIDM_SHVIEWLAST)
    {
        TRACE("Invalid Verb %x\n", LOWORD(lpcmi->lpVerb));
        return E_INVALIDARG;
    }

    if (IS_INTRESOURCE(lpcmi->lpVerb))
    {
        switch(LOWORD(lpcmi->lpVerb))
        {
        case FCIDM_SHVIEW_EXPLORE:
            TRACE("Verb FCIDM_SHVIEW_EXPLORE\n");
            DoOpenExplore(This, lpcmi->hwnd, "explore");
            break;
        case FCIDM_SHVIEW_OPEN:
            TRACE("Verb FCIDM_SHVIEW_OPEN\n");
            DoOpenExplore(This, lpcmi->hwnd, "open");
            break;
        case FCIDM_SHVIEW_RENAME:
        {
            IShellBrowser *browser;

            /* get the active IShellView */
            browser = (IShellBrowser*)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
            if (browser)
            {
                IShellView *view;

                if(SUCCEEDED(IShellBrowser_QueryActiveShellView(browser, &view)))
                {
                    TRACE("(shellview=%p)\n", view);
                    IShellView_SelectItem(view, This->apidl[0],
                         SVSI_DESELECTOTHERS|SVSI_EDIT|SVSI_ENSUREVISIBLE|SVSI_FOCUSED|SVSI_SELECT);
                    IShellView_Release(view);
                }
            }
            break;
        }
        case FCIDM_SHVIEW_DELETE:
            TRACE("Verb FCIDM_SHVIEW_DELETE\n");
            DoDelete(This);
            break;
        case FCIDM_SHVIEW_COPY:
            TRACE("Verb FCIDM_SHVIEW_COPY\n");
            DoCopyOrCut(This, lpcmi->hwnd, FALSE);
            break;
        case FCIDM_SHVIEW_CUT:
            TRACE("Verb FCIDM_SHVIEW_CUT\n");
            DoCopyOrCut(This, lpcmi->hwnd, TRUE);
            break;
        case FCIDM_SHVIEW_INSERT:
            TRACE("Verb FCIDM_SHVIEW_INSERT\n");
            DoPaste(This);
            break;
        case FCIDM_SHVIEW_PROPERTIES:
            TRACE("Verb FCIDM_SHVIEW_PROPERTIES\n");
            DoOpenProperties(This, lpcmi->hwnd);
            break;
        default:
            FIXME("Unhandled Verb %xl\n",LOWORD(lpcmi->lpVerb));
            return E_INVALIDARG;
        }
    }
    else
    {
        TRACE("Verb is %s\n",debugstr_a(lpcmi->lpVerb));
        if (strcmp(lpcmi->lpVerb,"delete")==0)
            DoDelete(This);
        else if (strcmp(lpcmi->lpVerb,"properties")==0)
            DoOpenProperties(This, lpcmi->hwnd);
        else if (strcmp(lpcmi->lpVerb,"cut")==0)
            DoCopyOrCut(This, lpcmi->hwnd, TRUE);
        else if (strcmp(lpcmi->lpVerb,"copy")==0)
            DoCopyOrCut(This, lpcmi->hwnd, FALSE);
        else if (strcmp(lpcmi->lpVerb,"paste")==0)
            DoPaste(This);
        else {
            FIXME("Unhandled string verb %s\n",debugstr_a(lpcmi->lpVerb));
            return E_FAIL;
        }
    }
    return S_OK;
}

static HRESULT WINAPI ItemMenu_GetCommandString(IContextMenu3 *iface, UINT_PTR cmdid, UINT flags,
    UINT *reserved, LPSTR name, UINT maxlen)
{
    static const WCHAR openW[] = {'o','p','e','n',0};
    static const WCHAR exploreW[] = {'e','x','p','l','o','r','e',0};
    static const WCHAR cutW[] = {'c','u','t',0};
    static const WCHAR copyW[] = {'c','o','p','y',0};
    static const WCHAR pasteW[] = {'p','a','s','t','e',0};
    static const WCHAR linkW[] = {'l','i','n','k',0};
    static const WCHAR deleteW[] = {'d','e','l','e','t','e',0};
    static const WCHAR propertiesW[] = {'p','r','o','p','e','r','t','i','e','s',0};
    static const WCHAR renameW[] = {'r','e','n','a','m','e',0};
    ContextMenu *This = impl_from_IContextMenu3(iface);
    const WCHAR *cmdW = NULL;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%lx, %#x, %p, %p, %u)\n", This, cmdid, flags, reserved, name, maxlen);

    switch (flags)
    {
    case GCS_HELPTEXTA:
    case GCS_HELPTEXTW:
        hr = E_NOTIMPL;
        break;

    case GCS_VERBA:
    case GCS_VERBW:
        switch (cmdid)
        {
        case FCIDM_SHVIEW_OPEN:
            cmdW = openW;
            break;
        case FCIDM_SHVIEW_EXPLORE:
            cmdW = exploreW;
            break;
        case FCIDM_SHVIEW_CUT:
            cmdW = cutW;
            break;
        case FCIDM_SHVIEW_COPY:
            cmdW = copyW;
            break;
        case FCIDM_SHVIEW_INSERT:
            cmdW = pasteW;
            break;
        case FCIDM_SHVIEW_CREATELINK:
            cmdW = linkW;
            break;
        case FCIDM_SHVIEW_DELETE:
            cmdW = deleteW;
            break;
        case FCIDM_SHVIEW_PROPERTIES:
            cmdW = propertiesW;
            break;
        case FCIDM_SHVIEW_RENAME:
            cmdW = renameW;
            break;
        }

        if (!cmdW)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (flags == GCS_VERBA)
            WideCharToMultiByte(CP_ACP, 0, cmdW, -1, name, maxlen, NULL, NULL);
        else
            lstrcpynW((WCHAR *)name, cmdW, maxlen);

        TRACE("name %s\n", flags == GCS_VERBA ? debugstr_a(name) : debugstr_w((WCHAR *)name));
        break;

    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
        break;
    }

    return hr;
}

/**************************************************************************
* NOTES
*  should be only in IContextMenu2 and IContextMenu3
*  is nevertheless called from word95
*/
static HRESULT WINAPI ContextMenu_HandleMenuMsg(IContextMenu3 *iface, UINT msg,
    WPARAM wParam, LPARAM lParam)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    FIXME("(%p)->(0x%x 0x%lx 0x%lx): stub\n", This, msg, wParam, lParam);
    return E_NOTIMPL;
}

static HRESULT WINAPI ContextMenu_HandleMenuMsg2(IContextMenu3 *iface, UINT msg,
    WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    FIXME("(%p)->(0x%x 0x%lx 0x%lx %p): stub\n", This, msg, wParam, lParam, result);
    return E_NOTIMPL;
}

static const IContextMenu3Vtbl ItemContextMenuVtbl =
{
    ContextMenu_QueryInterface,
    ContextMenu_AddRef,
    ContextMenu_Release,
    ItemMenu_QueryContextMenu,
    ItemMenu_InvokeCommand,
    ItemMenu_GetCommandString,
    ContextMenu_HandleMenuMsg,
    ContextMenu_HandleMenuMsg2
};

static HRESULT WINAPI ShellExtInit_QueryInterface(IShellExtInit *iface, REFIID riid, void **obj)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);
    return IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, obj);
}

static ULONG WINAPI ShellExtInit_AddRef(IShellExtInit *iface)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);
    return IContextMenu3_AddRef(&This->IContextMenu3_iface);
}

static ULONG WINAPI ShellExtInit_Release(IShellExtInit *iface)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);
    return IContextMenu3_Release(&This->IContextMenu3_iface);
}

static HRESULT WINAPI ShellExtInit_Initialize(IShellExtInit *iface, LPCITEMIDLIST folder,
    IDataObject *dataobj, HKEY progidkey)
{
    ContextMenu *This = impl_from_IShellExtInit(iface);

    FIXME("(%p)->(%p %p %p): stub\n", This, folder, dataobj, progidkey);

    return E_NOTIMPL;
}

static const IShellExtInitVtbl ShellExtInitVtbl =
{
    ShellExtInit_QueryInterface,
    ShellExtInit_AddRef,
    ShellExtInit_Release,
    ShellExtInit_Initialize
};

static HRESULT WINAPI ObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **obj)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);
    return IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, obj);
}

static ULONG WINAPI ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);
    return IContextMenu3_AddRef(&This->IContextMenu3_iface);
}

static ULONG WINAPI ObjectWithSite_Release(IObjectWithSite *iface)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);
    return IContextMenu3_Release(&This->IContextMenu3_iface);
}

static HRESULT WINAPI ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *site)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);

    FIXME("(%p)->(%p): stub\n", This, site);

    return E_NOTIMPL;
}

static HRESULT WINAPI ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, void **site)
{
    ContextMenu *This = impl_from_IObjectWithSite(iface);

    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), site);

    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl ObjectWithSiteVtbl =
{
    ObjectWithSite_QueryInterface,
    ObjectWithSite_AddRef,
    ObjectWithSite_Release,
    ObjectWithSite_SetSite,
    ObjectWithSite_GetSite,
};

HRESULT ItemMenu_Constructor(IShellFolder *parent, LPCITEMIDLIST pidl, const LPCITEMIDLIST *apidl, UINT cidl,
    REFIID riid, void **pObj)
{
    ContextMenu* This;
    HRESULT hr;
    UINT i;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IContextMenu3_iface.lpVtbl = &ItemContextMenuVtbl;
    This->IShellExtInit_iface.lpVtbl = &ShellExtInitVtbl;
    This->IObjectWithSite_iface.lpVtbl = &ObjectWithSiteVtbl;
    This->ref = 1;
    This->parent = parent;
    if (parent) IShellFolder_AddRef(parent);

    This->pidl = ILClone(pidl);
    This->apidl = _ILCopyaPidl(apidl, cidl);
    This->cidl = cidl;
    This->allvalues = TRUE;

    This->desktop = FALSE;

    for (i = 0; i < cidl; i++)
       This->allvalues &= (_ILIsValue(apidl[i]) ? 1 : 0);

    hr = IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, pObj);
    IContextMenu3_Release(&This->IContextMenu3_iface);

    return hr;
}

/* Background menu implementation */
static HRESULT WINAPI BackgroundMenu_QueryContextMenu(
	IContextMenu3 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    HMENU hMyMenu;
    UINT idMax;
    HRESULT hr;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          This, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    hMyMenu = LoadMenuA(shell32_hInstance, "MENU_002");
    if (uFlags & CMF_DEFAULTONLY)
    {
        HMENU ourMenu = GetSubMenu(hMyMenu,0);
        UINT oldDef = GetMenuDefaultItem(hMenu,TRUE,GMDI_USEDISABLED);
        UINT newDef = GetMenuDefaultItem(ourMenu,TRUE,GMDI_USEDISABLED);
        if (newDef != oldDef)
            SetMenuDefaultItem(hMenu,newDef,TRUE);
        if (newDef!=0xFFFFFFFF)
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, newDef+1);
        else
            hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }
    else
    {
        idMax = Shell_MergeMenus (hMenu, GetSubMenu(hMyMenu,0), indexMenu,
                                  idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);
        hr =  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, idMax-idCmdFirst);
    }
    DestroyMenu(hMyMenu);

    TRACE("(%p)->returning 0x%x\n",This,hr);
    return hr;
}

static void DoNewFolder(ContextMenu *This, IShellView *view)
{
    ISFHelper *helper;

    IShellFolder_QueryInterface(This->parent, &IID_ISFHelper, (void**)&helper);
    if (helper)
    {
        WCHAR nameW[MAX_PATH];
        LPITEMIDLIST pidl;

        ISFHelper_GetUniqueName(helper, nameW, MAX_PATH);
        ISFHelper_AddFolder(helper, 0, nameW, &pidl);

        if (view)
        {
	    /* if we are in a shellview do labeledit */
	    IShellView_SelectItem(view,
                    pidl,(SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE
                    |SVSI_FOCUSED|SVSI_SELECT));
        }

        SHFree(pidl);
        ISFHelper_Release(helper);
    }
}

static BOOL DoPaste(ContextMenu *This)
{
	BOOL bSuccess = FALSE;
	IDataObject * pda;

	TRACE("\n");

	if(SUCCEEDED(OleGetClipboard(&pda)))
	{
	  STGMEDIUM medium;
	  FORMATETC formatetc;

	  TRACE("pda=%p\n", pda);

	  /* Set the FORMATETC structure*/
	  InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLISTW), TYMED_HGLOBAL);

	  /* Get the pidls from IDataObject */
	  if(SUCCEEDED(IDataObject_GetData(pda,&formatetc,&medium)))
          {
	    LPITEMIDLIST * apidl;
	    LPITEMIDLIST pidl;
	    IShellFolder *psfFrom = NULL, *psfDesktop;

	    LPIDA lpcida = GlobalLock(medium.u.hGlobal);
	    TRACE("cida=%p\n", lpcida);

	    apidl = _ILCopyCidaToaPidl(&pidl, lpcida);

            /*
             * In case source is a file we need to remove the last component
             * to obtain a IShellFolder of the parent.
             */
            if (_ILIsValue(pidl))
                ILRemoveLastID(pidl);

	    /* bind to the source shellfolder */
	    SHGetDesktopFolder(&psfDesktop);
	    if(psfDesktop)
	    {
	      IShellFolder_BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, (LPVOID*)&psfFrom);
	      IShellFolder_Release(psfDesktop);
	    }

	    if (psfFrom)
	    {
	      /* get source and destination shellfolder */
            ISFHelper *psfhlpdst = NULL, *psfhlpsrc = NULL;

            /* when using an item context menu we first need to bind to the selected folder */
            if (This->apidl)
            {
                IShellFolder *folder;

                if (SUCCEEDED(IShellFolder_BindToObject(This->parent, This->apidl[0], NULL, &IID_IShellFolder, (LPVOID*)&folder)))
                {
                    IShellFolder_QueryInterface(folder, &IID_ISFHelper, (void**)&psfhlpdst);
                    IShellFolder_Release(folder);
                }
            }
            else
                IShellFolder_QueryInterface(This->parent, &IID_ISFHelper, (void**)&psfhlpdst);

	      IShellFolder_QueryInterface(psfFrom, &IID_ISFHelper, (void**)&psfhlpsrc);

	      /* do the copy/move */
	      if (psfhlpdst && psfhlpsrc)
	      {
                DWORD dropEffect;
                GetDropEffect(pda, &dropEffect);

                if (SUCCEEDED(ISFHelper_CopyItems(psfhlpdst, psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl)))
                {
                    if (dropEffect == DROPEFFECT_MOVE)
                    {
                        if (SUCCEEDED(ISFHelper_DeleteItems(psfhlpsrc, lpcida->cidl, (LPCITEMIDLIST*)apidl, FALSE)))
                            bSuccess = TRUE;
                    }
                    else
                        bSuccess = TRUE;
                }
	      }
	      if(psfhlpdst) ISFHelper_Release(psfhlpdst);
	      if(psfhlpsrc) ISFHelper_Release(psfhlpsrc);
	      IShellFolder_Release(psfFrom);
	    }

	    _ILFreeaPidl(apidl, lpcida->cidl);
	    SHFree(pidl);

	    /* release the medium*/
	    ReleaseStgMedium(&medium);
	  }
	  IDataObject_Release(pda);
	}
#if 0
	HGLOBAL  hMem;

	OpenClipboard(NULL);
	hMem = GetClipboardData(CF_HDROP);

	if(hMem)
	{
          char * pDropFiles = GlobalLock(hMem);
	  if(pDropFiles)
	  {
	    int len, offset = sizeof(DROPFILESTRUCT);

	    while( pDropFiles[offset] != 0)
	    {
	      len = strlen(pDropFiles + offset);
	      TRACE("%s\n", pDropFiles + offset);
	      offset += len+1;
	    }
	  }
	  GlobalUnlock(hMem);
	}
	CloseClipboard();
#endif
	return bSuccess;
}

static HRESULT WINAPI BackgroundMenu_InvokeCommand(
	IContextMenu3 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    ContextMenu *This = impl_from_IContextMenu3(iface);
    IShellBrowser *browser;
    IShellView *view = NULL;
    HWND hWnd = NULL;

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n", This, lpcmi, lpcmi->lpVerb, lpcmi->hwnd);

    /* get the active IShellView */
    if ((browser = (IShellBrowser*)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0)))
    {
        if (SUCCEEDED(IShellBrowser_QueryActiveShellView(browser, &view)))
	    IShellView_GetWindow(view, &hWnd);
    }

    if(HIWORD(lpcmi->lpVerb))
    {
        TRACE("%s\n", debugstr_a(lpcmi->lpVerb));

        if (!strcmp(lpcmi->lpVerb, CMDSTR_NEWFOLDERA))
        {
            DoNewFolder(This, view);
        }
        else if (!strcmp(lpcmi->lpVerb, CMDSTR_VIEWLISTA))
        {
            if (hWnd) SendMessageA(hWnd, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_LISTVIEW, 0), 0);
        }
        else if (!strcmp(lpcmi->lpVerb, CMDSTR_VIEWDETAILSA))
        {
	    if (hWnd) SendMessageA(hWnd, WM_COMMAND, MAKEWPARAM(FCIDM_SHVIEW_REPORTVIEW, 0), 0);
        }
        else
        {
            FIXME("please report: unknown verb %s\n", debugstr_a(lpcmi->lpVerb));
        }
    }
    else
    {
        switch (LOWORD(lpcmi->lpVerb))
        {
	    case FCIDM_SHVIEW_REFRESH:
	        if (view) IShellView_Refresh(view);
                break;

            case FCIDM_SHVIEW_NEWFOLDER:
                DoNewFolder(This, view);
                break;

            case FCIDM_SHVIEW_INSERT:
                DoPaste(This);
                break;

            case FCIDM_SHVIEW_PROPERTIES:
                if (This->desktop) {
		    ShellExecuteA(lpcmi->hwnd, "open", "rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, NULL, SW_SHOWNORMAL);
		} else {
		    FIXME("launch item properties dialog\n");
		}
		break;

            default:
                /* if it's an id just pass it to the parent shv */
                if (hWnd) SendMessageA(hWnd, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0), 0);
                break;
         }
    }

    if (view)
        IShellView_Release(view);

    return S_OK;
}

static HRESULT WINAPI BackgroundMenu_GetCommandString(
	IContextMenu3 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
        ContextMenu *This = impl_from_IContextMenu3(iface);

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

	/* test the existence of the menu items, the file dialog enables
	   the buttons according to this */
	if (uFlags == GCS_VALIDATEA)
	{
	  if(HIWORD(idCommand))
	  {
	    if (!strcmp((LPSTR)idCommand, CMDSTR_VIEWLISTA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_VIEWDETAILSA) ||
	        !strcmp((LPSTR)idCommand, CMDSTR_NEWFOLDERA))
	    {
	      return S_OK;
	    }
	  }
	}

	FIXME("unknown command string\n");
	return E_FAIL;
}

static const IContextMenu3Vtbl BackgroundContextMenuVtbl =
{
    ContextMenu_QueryInterface,
    ContextMenu_AddRef,
    ContextMenu_Release,
    BackgroundMenu_QueryContextMenu,
    BackgroundMenu_InvokeCommand,
    BackgroundMenu_GetCommandString,
    ContextMenu_HandleMenuMsg,
    ContextMenu_HandleMenuMsg2
};

HRESULT BackgroundMenu_Constructor(IShellFolder *parent, BOOL desktop, REFIID riid, void **pObj)
{
    ContextMenu *This;
    HRESULT hr;

    This = heap_alloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IContextMenu3_iface.lpVtbl = &BackgroundContextMenuVtbl;
    This->IShellExtInit_iface.lpVtbl = &ShellExtInitVtbl;
    This->IObjectWithSite_iface.lpVtbl = &ObjectWithSiteVtbl;
    This->ref = 1;
    This->parent = parent;

    This->pidl = NULL;
    This->apidl = NULL;
    This->cidl = 0;
    This->allvalues = FALSE;

    This->desktop = desktop;
    if (parent) IShellFolder_AddRef(parent);

    hr = IContextMenu3_QueryInterface(&This->IContextMenu3_iface, riid, pObj);
    IContextMenu3_Release(&This->IContextMenu3_iface);

    return hr;
}
