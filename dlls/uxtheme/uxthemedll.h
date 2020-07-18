/*
 * Internal uxtheme defines & declarations
 *
 * Copyright (C) 2003 Kevin Koltzau
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

#ifndef __WINE_UXTHEMEDLL_H
#define __WINE_UXTHEMEDLL_H

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "uxtheme.h"

typedef HANDLE HTHEMEFILE;

/**********************************************************************
 *              EnumThemeProc
 *
 * Callback function for EnumThemes.
 *
 * RETURNS
 *     TRUE to continue enumeration, FALSE to stop
 *
 * PARAMS
 *     lpReserved          Always 0
 *     pszThemeFileName    Full path to theme msstyles file
 *     pszThemeName        Display name for theme
 *     pszToolTip          Tooltip name for theme
 *     lpReserved2         Always 0
 *     lpData              Value passed through lpData from EnumThemes
 */
typedef BOOL (CALLBACK *EnumThemeProc)(LPVOID lpReserved, LPCWSTR pszThemeFileName,
                                       LPCWSTR pszThemeName, LPCWSTR pszToolTip, LPVOID lpReserved2,
                                       LPVOID lpData);

/**********************************************************************
 *              ParseThemeIniFileProc
 *
 * Callback function for ParseThemeIniFile.
 *
 * RETURNS
 *     TRUE to continue enumeration, FALSE to stop
 *
 * PARAMS
 *     dwType              Entry type
 *     pszParam1           Use defined by entry type
 *     pszParam2           Use defined by entry type
 *     pszParam3           Use defined by entry type
 *     dwParam             Use defined by entry type
 *     lpData              Value passed through lpData from ParseThemeIniFile
 *
 * NOTES
 * I don't know what the valid entry types are
 */
typedef BOOL (CALLBACK*ParseThemeIniFileProc)(DWORD dwType, LPWSTR pszParam1,
                                              LPWSTR pszParam2, LPWSTR pszParam3,
                                              DWORD dwParam, LPVOID lpData);

/* Structure filled in by EnumThemeColors() and EnumeThemeSizes() with the
 * various strings for a theme color or size. */
typedef struct tagTHEMENAMES
{
    WCHAR szName[MAX_PATH+1];
    WCHAR szDisplayName[MAX_PATH+1];
    WCHAR szTooltip[MAX_PATH+1];
} THEMENAMES, *PTHEMENAMES;

/* Declarations for undocumented functions for use internally */
DWORD WINAPI QueryThemeServices(void) DECLSPEC_HIDDEN;
HRESULT WINAPI OpenThemeFile(LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
                             LPCWSTR pszSizeName, HTHEMEFILE *hThemeFile,
                             DWORD unknown) DECLSPEC_HIDDEN;
HRESULT WINAPI CloseThemeFile(HTHEMEFILE hThemeFile) DECLSPEC_HIDDEN;
HRESULT WINAPI ApplyTheme(HTHEMEFILE hThemeFile, char *unknown, HWND hWnd) DECLSPEC_HIDDEN;
HRESULT WINAPI GetThemeDefaults(LPCWSTR pszThemeFileName, LPWSTR pszColorName,
                                DWORD dwColorNameLen, LPWSTR pszSizeName,
                                DWORD dwSizeNameLen) DECLSPEC_HIDDEN;
HRESULT WINAPI EnumThemes(LPCWSTR pszThemePath, EnumThemeProc callback,
                          LPVOID lpData) DECLSPEC_HIDDEN;
HRESULT WINAPI EnumThemeColors(LPWSTR pszThemeFileName, LPWSTR pszSizeName,
                               DWORD dwColorNum, PTHEMENAMES pszColorNames) DECLSPEC_HIDDEN;
HRESULT WINAPI EnumThemeSizes(LPWSTR pszThemeFileName, LPWSTR pszColorName,
                              DWORD dwSizeNum, PTHEMENAMES pszColorNames) DECLSPEC_HIDDEN;
HRESULT WINAPI ParseThemeIniFile(LPCWSTR pszIniFileName, LPWSTR pszUnknown,
                                 ParseThemeIniFileProc callback, LPVOID lpData) DECLSPEC_HIDDEN;

extern void UXTHEME_InitSystem(HINSTANCE hInst) DECLSPEC_HIDDEN;

BOOL uxtheme_gtk_enabled(void) DECLSPEC_HIDDEN;

HRESULT uxtheme_gtk_CloseThemeData(HTHEME theme) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_EnableThemeDialogTexture(HWND hwnd, DWORD flags) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_EnableTheming(BOOL enable) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetCurrentThemeName(LPWSTR filename, int filename_maxlen,
                                        LPWSTR color, int color_maxlen,
                                        LPWSTR size, int size_maxlen) DECLSPEC_HIDDEN;
DWORD uxtheme_gtk_GetThemeAppProperties(void) DECLSPEC_HIDDEN;
BOOL uxtheme_gtk_IsThemeDialogTextureEnabled(HWND hwnd) DECLSPEC_HIDDEN;
HTHEME uxtheme_gtk_OpenThemeDataEx(HWND hwnd, LPCWSTR classlist, DWORD flags) DECLSPEC_HIDDEN;
void uxtheme_gtk_SetThemeAppProperties(DWORD flags) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_SetWindowTheme(HWND hwnd, LPCWSTR sub_app_name, LPCWSTR sub_id_list) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeBool(HTHEME htheme, int part_id, int state_id,
                                 int prop_id, BOOL *value) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeColor(HTHEME htheme, int part_id, int state_id,
                                  int prop_id, COLORREF *color) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeEnumValue(HTHEME htheme, int part_id, int state_id,
                                      int prop_id, int *value) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeFilename(HTHEME htheme, int part_id, int state_id,
                                     int prop_id, LPWSTR filename, int maxlen) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeFont(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                 int prop_id, LOGFONTW *font) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeInt(HTHEME htheme, int part_id, int state_id,
                                int prop_id, int *value) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeIntList(HTHEME htheme, int part_id, int state_id,
                                    int prop_id, INTLIST *intlist) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeMargins(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                    int prop_id, LPRECT rect, MARGINS *margins) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeMetric(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                   int prop_id, int *value) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemePosition(HTHEME htheme, int part_id, int state_id,
                                     int prop_id, POINT *point) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemePropertyOrigin(HTHEME htheme, int part_id, int state_id,
                                           int prop_id, PROPERTYORIGIN *origin) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeRect(HTHEME htheme, int part_id, int state_id,
                                 int prop_id, RECT *rect) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeString(HTHEME htheme, int part_id, int state_id,
                                   int prop_id, LPWSTR buffer, int maxlen) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeTransitionDuration(HTHEME htheme, int part_id, int state_id_from,
                                               int state_id_to, int prop_id, DWORD *duration) DECLSPEC_HIDDEN;
BOOL uxtheme_gtk_GetThemeSysBool(HTHEME htheme, int bool_id) DECLSPEC_HIDDEN;
COLORREF uxtheme_gtk_GetThemeSysColor(HTHEME htheme, int color_id) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeSysFont(HTHEME htheme, int font_id, LOGFONTW *font) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeSysInt(HTHEME htheme, int int_id, int *value) DECLSPEC_HIDDEN;
int uxtheme_gtk_GetThemeSysSize(HTHEME htheme, int size_id) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeSysString(HTHEME htheme, int string_id, LPWSTR buffer, int maxlen) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_DrawThemeBackgroundEx(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                          LPCRECT rect, const DTBGOPTS *options) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_DrawThemeTextEx(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                    LPCWSTR text, int length, DWORD flags, RECT *rect,
                                    const DTTOPTS *options) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeBackgroundRegion(HTHEME htheme, HDC hdc, int part_id,
                                             int state_id, LPCRECT rect, HRGN *region) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemePartSize(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                     RECT *rect, THEMESIZE type, SIZE *size) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeTextExtent(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                       LPCWSTR text, int length, DWORD flags,
                                       LPCRECT bounding_rect, LPRECT extent_rect) DECLSPEC_HIDDEN;
HRESULT uxtheme_gtk_GetThemeTextMetrics(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                        TEXTMETRICW *metric) DECLSPEC_HIDDEN;
BOOL uxtheme_gtk_IsThemeBackgroundPartiallyTransparent(HTHEME htheme, int part_id, int state_id) DECLSPEC_HIDDEN;
BOOL uxtheme_gtk_IsThemePartDefined(HTHEME htheme, int part_id, int state_id) DECLSPEC_HIDDEN;

/* No alpha blending */
#define ALPHABLEND_NONE             0
/* "Cheap" binary alpha blending - but possibly faster */
#define ALPHABLEND_BINARY           1
/* Full alpha blending */
#define ALPHABLEND_FULL             2

#endif /* __WINE_UXTHEMEDLL_H */
