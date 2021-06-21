/*
 * WineCfg Staging panel
 *
 * Copyright 2014 Michael Müller
 * Copyright 2015 Sebastian Lackner
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
 *
 */

#define COBJMACROS

#include "config.h"

#include <windows.h>
#include <wine/unicode.h>

#include "resource.h"
#include "winecfg.h"

/*
 * Command stream multithreading
 */
static BOOL csmt_get(void)
{
    char *buf = get_reg_key(config_key, "Direct3D", "csmt", NULL);
    BOOL ret = buf ? !!*buf : TRUE;
    HeapFree(GetProcessHeap(), 0, buf);
    return ret;
}
static void csmt_set(BOOL status)
{
    set_reg_key_dword(config_key, "Direct3D", "csmt", status);
}

/*
 * DXVA2
 */
static BOOL vaapi_get(void)
{
#ifdef HAVE_VAAPI
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("DXVA2"), "backend", NULL);
    ret = (value && !strcmp(value, "va"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
#else
    return FALSE;
#endif
}
static void vaapi_set(BOOL status)
{
#ifdef HAVE_VAAPI
    set_reg_key(config_key, keypath("DXVA2"), "backend", status ? "va" : NULL);
#endif
}

/*
 * EAX
 */
static BOOL eax_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath("DirectSound"), "EAXEnabled", "N");
    ret = IS_OPTION_TRUE(*value);
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void eax_set(BOOL status)
{
    set_reg_key(config_key, keypath("DirectSound"), "EAXEnabled", status ? "Y" : "N");
}

/*
 * Hide Wine exports from applications
 */
static BOOL hidewine_get(void)
{
    BOOL ret;
    char *value = get_reg_key(config_key, keypath(""), "HideWineExports", "N");
    ret = IS_OPTION_TRUE(*value);
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
}
static void hidewine_set(BOOL status)
{
    set_reg_key(config_key, keypath(""), "HideWineExports", status ? "Y" : "N");
}

/*
 * GTK3
 */
static BOOL gtk3_get(void)
{
#if defined(HAVE_GTK_GTK_H) && defined(SONAME_LIBGTK_3)
    BOOL ret;
    char *value = get_reg_key(config_key, keypath(""), "ThemeEngine", NULL);
    ret = (value && !strcasecmp(value, "GTK"));
    HeapFree(GetProcessHeap(), 0, value);
    return ret;
#else
    return FALSE;
#endif
}
static void gtk3_set(BOOL status)
{
#if defined(HAVE_GTK_GTK_H) && defined(SONAME_LIBGTK_3)
    set_reg_key(config_key, keypath(""), "ThemeEngine", status ? "GTK" : NULL);
#endif
}

static void load_staging_settings(HWND dialog)
{
    CheckDlgButton(dialog, IDC_ENABLE_CSMT, csmt_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_VAAPI, vaapi_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_EAX, eax_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_HIDEWINE, hidewine_get() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ENABLE_GTK3, gtk3_get() ? BST_CHECKED : BST_UNCHECKED);

#ifndef HAVE_VAAPI
    disable(IDC_ENABLE_VAAPI);
#endif
#if !defined(HAVE_GTK_GTK_H) || !defined(SONAME_LIBGTK_3)
    disable(IDC_ENABLE_GTK3);
#endif
}

INT_PTR CALLBACK StagingDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        break;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->code == PSN_SETACTIVE)
            load_staging_settings(hDlg);
        break;

    case WM_SHOWWINDOW:
        set_window_title(hDlg);
        break;

    case WM_DESTROY:
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) != BN_CLICKED) break;
        switch (LOWORD(wParam))
        {
        case IDC_ENABLE_CSMT:
            csmt_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_CSMT) == BST_CHECKED);
            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
            return TRUE;
        case IDC_ENABLE_VAAPI:
            vaapi_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_VAAPI) == BST_CHECKED);
            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
            return TRUE;
        case IDC_ENABLE_EAX:
            eax_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_EAX) == BST_CHECKED);
            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
            return TRUE;
        case IDC_ENABLE_HIDEWINE:
            hidewine_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_HIDEWINE) == BST_CHECKED);
            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
            return TRUE;
        case IDC_ENABLE_GTK3:
            gtk3_set(IsDlgButtonChecked(hDlg, IDC_ENABLE_GTK3) == BST_CHECKED);
            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
