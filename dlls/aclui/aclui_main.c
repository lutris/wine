/*
 * Implementation of the AclUI
 *
 * Copyright (C) 2009 Nikolay Sivov
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

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnt.h"
#include "aclui.h"
#include "resource.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aclui);

/* the aclui.h files does not contain the necessary COBJMACROS */
#define ISecurityInformation_AddRef(This) (This)->lpVtbl->AddRef(This)
#define ISecurityInformation_Release(This) (This)->lpVtbl->Release(This)
#define ISecurityInformation_GetObjectInformation(This, obj) (This)->lpVtbl->GetObjectInformation(This, obj)
#define ISecurityInformation_GetSecurity(This, info, sd, def) (This)->lpVtbl->GetSecurity(This, info, sd, def)
#define ISecurityInformation_GetAccessRights(This, type, flags, access, count, def) (This)->lpVtbl->GetAccessRights(This, type, flags, access, count, def)

struct user
{
    struct list entry;
    WCHAR *name;

    /* must be the last entry */
    SID sid;
};

struct security_page
{
    ISecurityInformation *security;
    SI_OBJECT_INFO info;
    PSECURITY_DESCRIPTOR sd;
    SI_ACCESS *access;
    ULONG access_count;
    struct list users;

    HWND dialog;
    HIMAGELIST image_list_user;
};

static HINSTANCE aclui_instance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;    /* prefer native version */
    case DLL_PROCESS_ATTACH:
        aclui_instance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;
}

static WCHAR* WINAPIV load_formatstr(UINT resource, ...)
{
    __ms_va_list valist;
    WCHAR *str, fmtstr[256];
    DWORD ret;

    if (!LoadStringW(aclui_instance, resource, fmtstr, 256))
        return NULL;

    __ms_va_start( valist, resource );
    ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         fmtstr, 0, 0, (WCHAR*)&str, 0, &valist);
    __ms_va_end( valist );
    return ret ? str : NULL;
}

static void security_page_free(struct security_page *page)
{
    struct user *user, *user2;

    LIST_FOR_EACH_ENTRY_SAFE(user, user2, &page->users, struct user, entry)
    {
        list_remove(&user->entry);
        HeapFree(GetProcessHeap(), 0, user->name);
        HeapFree(GetProcessHeap(), 0, user);
    }

    if (page->image_list_user) ImageList_Destroy(page->image_list_user);
    if (page->security) ISecurityInformation_Release(page->security);
    HeapFree(GetProcessHeap(), 0, page);
}

static void users_clear(struct security_page *page)
{
    struct user *user, *user2;

    LIST_FOR_EACH_ENTRY_SAFE(user, user2, &page->users, struct user, entry)
    {
        /* TODO: Remove from GUI */

        list_remove(&user->entry);
        HeapFree(GetProcessHeap(), 0, user->name);
        HeapFree(GetProcessHeap(), 0, user);
    }
}

static WCHAR *get_sid_name(PSID sid, SID_NAME_USE *sid_type)
{
    WCHAR *name, *domain;
    DWORD domain_len = 0;
    DWORD name_len = 0;
    BOOL ret;

    LookupAccountSidW(NULL, sid, NULL, &name_len, NULL, &domain_len, sid_type);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return NULL;
    if (!(name = HeapAlloc(GetProcessHeap(), 0, name_len * sizeof(WCHAR))))
        return NULL;
    if (!(domain = HeapAlloc(GetProcessHeap(), 0, domain_len * sizeof(WCHAR))))
        goto error;

    ret = LookupAccountSidW(NULL, sid, name, &name_len, domain, &domain_len, sid_type);
    HeapFree(GetProcessHeap(), 0, domain);
    if (ret) return name;

error:
    HeapFree(GetProcessHeap(), 0, name);
    return NULL;
}

static BOOL users_add(struct security_page *page, PSID sid)
{
    SID_NAME_USE sid_type;
    struct user *user;
    DWORD sid_len;
    LVITEMW item;
    WCHAR *name;

    /* check if we already processed this user or group */
    LIST_FOR_EACH_ENTRY( user, &page->users, struct user, entry )
    {
        if (EqualSid(sid, &user->sid))
            return TRUE;
    }

    if (!(name = get_sid_name(sid, &sid_type)))
        return FALSE;

    sid_len = GetLengthSid(sid);
    user = HeapAlloc(GetProcessHeap(), 0, offsetof(struct user, sid) + sid_len);
    if (!user)
    {
        HeapFree(GetProcessHeap(), 0, name);
        return FALSE;
    }

    user->name = name;
    CopySid(sid_len, &user->sid, sid);
    list_add_tail(&page->users, &user->entry);

    /* Add in GUI */
    item.mask     = LVIF_PARAM | LVIF_TEXT;
    item.iItem    = -1;
    item.iSubItem = 0;
    item.pszText  = name;
    item.lParam   = (LPARAM)user;

    if (page->image_list_user)
    {
        item.mask |= LVIF_IMAGE;
        item.iImage = (sid_type == SidTypeGroup || sid_type == SidTypeWellKnownGroup) ? 0 : 1;
    }

    ListView_InsertItemW(GetDlgItem(page->dialog, IDC_USERS), &item);
    return TRUE;
}

static PSID get_sid_from_ace(ACE_HEADER *ace)
{
    switch (ace->AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            return (SID *)&((ACCESS_ALLOWED_ACE *)ace)->SidStart;
        case ACCESS_DENIED_ACE_TYPE:
            return (SID *)&((ACCESS_DENIED_ACE *)ace)->SidStart;
        default:
            FIXME("Don't know how to extract SID from ace type %d\n", ace->AceType);
            return NULL;
    }
}

static void users_refresh(struct security_page *page)
{
    BOOL defaulted, present;
    ACE_HEADER *ace;
    DWORD index;
    ACL *dacl;
    PSID sid;

    users_clear(page);

    /* Add Owner to list */
    if (GetSecurityDescriptorOwner(page->sd, &sid, &defaulted))
        users_add(page, sid);

    /* Everyone else who appears in the DACL */
    if (GetSecurityDescriptorDacl(page->sd, &present, &dacl, &defaulted) && present)
    {
        for (index = 0; index < dacl->AceCount; index++)
        {
            if (!GetAce(dacl, index, (void**)&ace))
                break;
            if (!(sid = get_sid_from_ace(ace)))
                continue;
            users_add(page, sid);
        }
    }
}

static HIMAGELIST create_image_list(UINT resource, UINT width, UINT height, UINT count, COLORREF mask_color)
{
    HIMAGELIST image_list;
    HBITMAP image;
    INT ret;

    if (!(image_list = ImageList_Create(width, height, ILC_COLOR32 | ILC_MASK, 0, count)))
        return NULL;
    if (!(image = LoadBitmapW(aclui_instance, MAKEINTRESOURCEW(resource))))
        goto error;

    ret = ImageList_AddMasked(image_list, image, mask_color);
    DeleteObject(image);
    if (ret != -1) return image_list;

error:
    ImageList_Destroy(image_list);
    return NULL;
}

static void compute_access_masks(PSECURITY_DESCRIPTOR sd, PSID sid, ACCESS_MASK *allowed, ACCESS_MASK *denied)
{
    BOOL defaulted, present;
    ACE_HEADER *ace;
    PSID ace_sid;
    DWORD index;
    ACL *dacl;

    *allowed = 0;
    *denied  = 0;

    if (!GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted) || !present)
        return;

    for (index = 0; index < dacl->AceCount; index++)
    {
        if (!GetAce(dacl, index, (void**)&ace))
            break;

        ace_sid = get_sid_from_ace(ace);
        if (!ace_sid || !EqualSid(ace_sid, sid))
            continue;

        if (ace->AceType == ACCESS_ALLOWED_ACE_TYPE)
            *allowed |= ((ACCESS_ALLOWED_ACE*)ace)->Mask;
        else if (ace->AceType == ACCESS_DENIED_ACE_TYPE)
            *denied |= ((ACCESS_DENIED_ACE*)ace)->Mask;
    }
}

static void show_ace_entries(struct security_page *page, struct user *user)
{
    static const WCHAR yesW[] = {'X',0};
    static const WCHAR noW[] = {'-',0};

    ACCESS_MASK allowed, denied;
    WCHAR *infotext;
    ULONG i, index;
    LVITEMW item;
    HWND control;

    compute_access_masks(page->sd, &user->sid, &allowed, &denied);

    if ((infotext = load_formatstr(IDS_PERMISSION_FOR, user->name)))
    {
        SetDlgItemTextW(page->dialog, IDC_ACE_USER, infotext);
        LocalFree(infotext);
    }

    control = GetDlgItem(page->dialog, IDC_ACE);
    index = 0;
    for (i = 0; i < page->access_count; i++)
    {
        if (!(page->access[i].dwFlags & SI_ACCESS_GENERAL))
            continue;

        item.mask = LVIF_TEXT;
        item.iItem = index;

        item.iSubItem = 1;
        if ((page->access[i].mask & allowed) == page->access[i].mask)
            item.pszText = (WCHAR *)yesW;
        else
            item.pszText = (WCHAR *)noW;
        ListView_SetItemW(control, &item);

        item.iSubItem = 2;
        if ((page->access[i].mask & denied) == page->access[i].mask)
            item.pszText = (WCHAR *)yesW;
        else
            item.pszText = (WCHAR *)noW;
        ListView_SetItemW(control, &item);

        index++;
    }
}

static void create_ace_entries(struct security_page *page)
{
    WCHAR str[256];
    HWND control;
    LVITEMW item;
    ULONG i, index;

    control = GetDlgItem(page->dialog, IDC_ACE);
    index = 0;
    for (i = 0; i < page->access_count; i++)
    {
        if (!(page->access[i].dwFlags & SI_ACCESS_GENERAL))
            continue;

        item.mask     = LVIF_TEXT;
        item.iItem    = index;
        item.iSubItem = 0;
        if (IS_INTRESOURCE(page->access[i].pszName))
        {
            str[0] = 0;
            LoadStringW(page->info.hInstance, (UINT)(DWORD_PTR)page->access[i].pszName, str, 256);
            item.pszText = str;
        }
        else
            item.pszText = (WCHAR *)page->access[i].pszName;
        ListView_InsertItemW(control, &item);

        index++;
    }
}

static void security_page_init_dlg(HWND hwnd, struct security_page *page)
{
    LVCOLUMNW column;
    HWND control;
    RECT rect;
    ULONG def = 0;

    page->dialog = hwnd;

    if (FAILED(ISecurityInformation_GetSecurity(page->security, DACL_SECURITY_INFORMATION |
                                                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
                                                &page->sd, FALSE)))
        return;

    if (FAILED(ISecurityInformation_GetAccessRights(page->security, NULL, 0, &page->access,
                                                    &page->access_count, &def)))
        return;

    /* Prepare user list */
    control = GetDlgItem(hwnd, IDC_USERS);
    SendMessageW(control, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    GetClientRect(control, &rect);
    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    column.cx = rect.right - rect.left;
    ListView_InsertColumnW(control, 0, &column);

    if ((page->image_list_user = create_image_list(IDB_USER_ICONS, 18, 18, 2, RGB(255, 0, 255))))
        SendMessageW(control, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)page->image_list_user);

    /* Prepare ACE list */
    control = GetDlgItem(hwnd, IDC_ACE);
    SendMessageW(control, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    column.cx = 170;
    ListView_InsertColumnW(control, 0, &column);

    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.fmt = LVCFMT_CENTER;
    column.cx = 85;
    ListView_InsertColumnW(control, 1, &column);

    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.fmt = LVCFMT_CENTER;
    column.cx = 85;
    ListView_InsertColumnW(control, 2, &column);

    users_refresh(page);
    create_ace_entries(page);

    if (!list_empty(&page->users))
    {
        LVITEMW item;
        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = LVIS_FOCUSED | LVIS_SELECTED;
        item.stateMask = item.state;
        ListView_SetItemW(GetDlgItem(hwnd, IDC_USERS), &item);
    }
}

static INT_PTR CALLBACK security_page_proc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW ppsp = (LPPROPSHEETPAGEW)lParam;
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)ppsp->lParam);
            security_page_init_dlg(hwndDlg, (struct security_page *)ppsp->lParam);
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_USER_ADD ||
                LOWORD(wParam) == IDC_USER_REMOVE)
            {
                /* TODO: Adding & removing users */
                MessageBoxA(hwndDlg, "Not implemented yet.", "Error", MB_OK | MB_ICONEXCLAMATION);
            }
            break;

        case WM_NOTIFY:
        {
            struct security_page *page = (struct security_page *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
            NMHDR *hdr = (NMHDR *)lParam;

            if (hdr->hwndFrom == GetDlgItem(hwndDlg, IDC_USERS) && hdr->code == LVN_ITEMCHANGED)
            {
                NMLISTVIEW *nmv = (NMLISTVIEW *)lParam;
                if (!(nmv->uOldState & LVIS_SELECTED) && (nmv->uNewState & LVIS_SELECTED))
                    show_ace_entries(page, (struct user *)nmv->lParam);
                return TRUE;
            }
            break;
        }

        default:
            break;
    }
    return FALSE;
}

static UINT CALLBACK security_page_callback(HWND hwnd, UINT msg, LPPROPSHEETPAGEW ppsp)
{
    struct security_page *page = (struct security_page *)ppsp->lParam;

    if (msg == PSPCB_RELEASE)
        security_page_free(page);

    return 1;
}

static HPROPSHEETPAGE create_security_property_page(ISecurityInformation *security)
{
    struct security_page *page;
    PROPSHEETPAGEW propsheet;
    HPROPSHEETPAGE ret;

    if (!(page = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*page))))
        return NULL;

    list_init(&page->users);
    page->security = security;
    ISecurityInformation_AddRef(security);

    if (FAILED(ISecurityInformation_GetObjectInformation(security, &page->info)))
        goto error;

    memset(&propsheet, 0, sizeof(propsheet));
    propsheet.dwSize        = sizeof(propsheet);
    propsheet.dwFlags       = PSP_DEFAULT | PSP_USECALLBACK;
    propsheet.hInstance     = aclui_instance;
    propsheet.u.pszTemplate = (LPWSTR)MAKEINTRESOURCE(IDD_SECURITY_PROPERTIES);
    propsheet.pfnDlgProc    = security_page_proc;
    propsheet.pfnCallback   = security_page_callback;
    propsheet.lParam        = (LPARAM)page;

    if (page->info.dwFlags & SI_PAGE_TITLE)
    {
        propsheet.pszTitle = page->info.pszPageTitle;
        propsheet.dwFlags |= PSP_USETITLE;
    }

    if ((ret = CreatePropertySheetPageW(&propsheet)))
        return ret;

error:
    security_page_free(page);
    return NULL;
}

HPROPSHEETPAGE WINAPI CreateSecurityPage(LPSECURITYINFO psi)
{
    FIXME("(%p): semi-stub\n", psi);

    InitCommonControls();
    return create_security_property_page(psi);
}

BOOL WINAPI EditSecurity(HWND owner, LPSECURITYINFO psi)
{
    PROPSHEETHEADERW prop;
    HPROPSHEETPAGE pages[1];
    SI_OBJECT_INFO info;
    BOOL ret;

    TRACE("(%p, %p)\n", owner, psi);

    if (FAILED(ISecurityInformation_GetObjectInformation(psi, &info)))
        return FALSE;
    if (!(pages[0] = CreateSecurityPage(psi)))
        return FALSE;

    memset(&prop, 0, sizeof(prop));
    prop.dwSize = sizeof(prop);
    prop.dwFlags = PSH_DEFAULT;
    prop.hwndParent = owner;
    prop.hInstance = aclui_instance;
    prop.pszCaption = load_formatstr(IDS_PERMISSION_FOR, info.pszObjectName);
    prop.nPages = 1;
    prop.u2.nStartPage = 0;
    prop.u3.phpage = pages;

    ret = PropertySheetW(&prop) != -1;
    LocalFree((void *)prop.pszCaption);
    return ret;
}
