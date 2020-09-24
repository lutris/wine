/*
 * GTK uxtheme implementation
 *
 * Copyright (C) 2015 Ivan Akulinchev
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "uxtheme.h"
#include "vsstyle.h"
#include "vssym32.h"

#include "wine/debug.h"

#include "uxthemedll.h"

#if defined(HAVE_GTK_GTK_H) && defined(SONAME_LIBGTK_3)

#include "uxthemegtk.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

#define GDK_DISABLE_DEPRECATION_WARNINGS
#include <gtk/gtk.h>

static void *libgtk3 = NULL;
static void *libcairo = NULL;
static void *libgobject2 = NULL;

#define MAKE_FUNCPTR(f) typeof(f) * p##f DECLSPEC_HIDDEN
MAKE_FUNCPTR(cairo_create);
MAKE_FUNCPTR(cairo_destroy);
MAKE_FUNCPTR(cairo_image_surface_create);
MAKE_FUNCPTR(cairo_image_surface_get_data);
MAKE_FUNCPTR(cairo_image_surface_get_stride);
MAKE_FUNCPTR(cairo_surface_destroy);
MAKE_FUNCPTR(cairo_surface_flush);
MAKE_FUNCPTR(g_type_check_instance_is_a);
MAKE_FUNCPTR(gtk_bin_get_child);
MAKE_FUNCPTR(gtk_button_new);
MAKE_FUNCPTR(gtk_check_button_new);
MAKE_FUNCPTR(gtk_combo_box_new_with_entry);
MAKE_FUNCPTR(gtk_container_add);
MAKE_FUNCPTR(gtk_container_forall);
MAKE_FUNCPTR(gtk_entry_new);
MAKE_FUNCPTR(gtk_fixed_new);
MAKE_FUNCPTR(gtk_frame_new);
MAKE_FUNCPTR(gtk_init);
MAKE_FUNCPTR(gtk_label_new);
MAKE_FUNCPTR(gtk_menu_bar_new);
MAKE_FUNCPTR(gtk_menu_item_new);
MAKE_FUNCPTR(gtk_menu_item_set_submenu);
MAKE_FUNCPTR(gtk_menu_new);
MAKE_FUNCPTR(gtk_menu_shell_append);
MAKE_FUNCPTR(gtk_notebook_new);
MAKE_FUNCPTR(gtk_radio_button_new);
MAKE_FUNCPTR(gtk_render_arrow);
MAKE_FUNCPTR(gtk_render_background);
MAKE_FUNCPTR(gtk_render_check);
MAKE_FUNCPTR(gtk_render_frame);
MAKE_FUNCPTR(gtk_render_handle);
MAKE_FUNCPTR(gtk_render_line);
MAKE_FUNCPTR(gtk_render_option);
MAKE_FUNCPTR(gtk_render_slider);
MAKE_FUNCPTR(gtk_scale_new);
MAKE_FUNCPTR(gtk_scrolled_window_new);
MAKE_FUNCPTR(gtk_separator_tool_item_new);
MAKE_FUNCPTR(gtk_style_context_add_class);
MAKE_FUNCPTR(gtk_style_context_add_region);
MAKE_FUNCPTR(gtk_style_context_get_background_color);
MAKE_FUNCPTR(gtk_style_context_get_border_color);
MAKE_FUNCPTR(gtk_style_context_get_color);
MAKE_FUNCPTR(gtk_style_context_remove_class);
MAKE_FUNCPTR(gtk_style_context_restore);
MAKE_FUNCPTR(gtk_style_context_save);
MAKE_FUNCPTR(gtk_style_context_set_junction_sides);
MAKE_FUNCPTR(gtk_style_context_set_state);
MAKE_FUNCPTR(gtk_toggle_button_get_type);
MAKE_FUNCPTR(gtk_toolbar_new);
MAKE_FUNCPTR(gtk_tree_view_append_column);
MAKE_FUNCPTR(gtk_tree_view_column_get_button);
MAKE_FUNCPTR(gtk_tree_view_column_new);
MAKE_FUNCPTR(gtk_tree_view_get_column);
MAKE_FUNCPTR(gtk_tree_view_new);
MAKE_FUNCPTR(gtk_widget_destroy);
MAKE_FUNCPTR(gtk_widget_get_style_context);
MAKE_FUNCPTR(gtk_widget_style_get);
MAKE_FUNCPTR(gtk_window_new);
#undef MAKE_FUNCPTR

#define NUM_SYS_COLORS      (COLOR_MENUBAR + 1)
#define MENU_HEIGHT         20
#define CLASSLIST_MAXLEN    128

static inline WORD reset_fpu_flags(void)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    WORD default_cw = 0x37f, pre_cw;
    __asm__ __volatile__( "fwait" );
    __asm__ __volatile__( "fnstcw %0" : "=m" (pre_cw) );
    __asm__ __volatile__( "fldcw %0" : : "m" (default_cw) );
    return pre_cw;
#else
    return 0;
#endif
}

static inline void set_fpu_flags(WORD flags)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    __asm__ __volatile__( "fclex" );
    __asm__ __volatile__( "fldcw %0" : : "m" (flags) );
#endif
}

static void free_gtk3_libs(void)
{
    if (libgtk3) dlclose(libgtk3);
    if (libcairo) dlclose(libcairo);
    if (libgobject2) dlclose(libgobject2);
    libgtk3 = libcairo = libgobject2 = NULL;
}

#define LOAD_FUNCPTR(lib, f) \
    if(!(p##f = dlsym(lib, #f))) \
    { \
        WARN("Can't find symbol %s.\n", #f); \
        goto error; \
    }

static BOOL load_gtk3_libs(void)
{
    if (libgtk3 && libcairo && libgobject2)
        return TRUE;

    libgtk3 = dlopen(SONAME_LIBGTK_3, RTLD_NOW);
    if (!libgtk3)
    {
        FIXME("Wine cannot find the %s library.\n", SONAME_LIBGTK_3);
        goto error;
    }

    LOAD_FUNCPTR(libgtk3, gtk_bin_get_child)
    LOAD_FUNCPTR(libgtk3, gtk_button_new)
    LOAD_FUNCPTR(libgtk3, gtk_check_button_new)
    LOAD_FUNCPTR(libgtk3, gtk_combo_box_new_with_entry)
    LOAD_FUNCPTR(libgtk3, gtk_container_add)
    LOAD_FUNCPTR(libgtk3, gtk_container_forall)
    LOAD_FUNCPTR(libgtk3, gtk_entry_new)
    LOAD_FUNCPTR(libgtk3, gtk_fixed_new)
    LOAD_FUNCPTR(libgtk3, gtk_frame_new)
    LOAD_FUNCPTR(libgtk3, gtk_init)
    LOAD_FUNCPTR(libgtk3, gtk_label_new)
    LOAD_FUNCPTR(libgtk3, gtk_menu_bar_new)
    LOAD_FUNCPTR(libgtk3, gtk_menu_item_new)
    LOAD_FUNCPTR(libgtk3, gtk_menu_item_set_submenu)
    LOAD_FUNCPTR(libgtk3, gtk_menu_new)
    LOAD_FUNCPTR(libgtk3, gtk_menu_shell_append)
    LOAD_FUNCPTR(libgtk3, gtk_notebook_new)
    LOAD_FUNCPTR(libgtk3, gtk_radio_button_new)
    LOAD_FUNCPTR(libgtk3, gtk_render_arrow)
    LOAD_FUNCPTR(libgtk3, gtk_render_background)
    LOAD_FUNCPTR(libgtk3, gtk_render_check)
    LOAD_FUNCPTR(libgtk3, gtk_render_frame)
    LOAD_FUNCPTR(libgtk3, gtk_render_handle)
    LOAD_FUNCPTR(libgtk3, gtk_render_line)
    LOAD_FUNCPTR(libgtk3, gtk_render_option)
    LOAD_FUNCPTR(libgtk3, gtk_render_slider)
    LOAD_FUNCPTR(libgtk3, gtk_scale_new)
    LOAD_FUNCPTR(libgtk3, gtk_scrolled_window_new)
    LOAD_FUNCPTR(libgtk3, gtk_separator_tool_item_new)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_add_class)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_add_region)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_get_background_color)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_get_border_color)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_get_color)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_remove_class)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_restore)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_save)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_set_junction_sides)
    LOAD_FUNCPTR(libgtk3, gtk_style_context_set_state)
    LOAD_FUNCPTR(libgtk3, gtk_toggle_button_get_type)
    LOAD_FUNCPTR(libgtk3, gtk_toolbar_new)
    LOAD_FUNCPTR(libgtk3, gtk_tree_view_append_column)
    LOAD_FUNCPTR(libgtk3, gtk_tree_view_column_get_button)
    LOAD_FUNCPTR(libgtk3, gtk_tree_view_column_new)
    LOAD_FUNCPTR(libgtk3, gtk_tree_view_get_column)
    LOAD_FUNCPTR(libgtk3, gtk_tree_view_new)
    LOAD_FUNCPTR(libgtk3, gtk_widget_destroy)
    LOAD_FUNCPTR(libgtk3, gtk_widget_get_style_context)
    LOAD_FUNCPTR(libgtk3, gtk_widget_style_get)
    LOAD_FUNCPTR(libgtk3, gtk_window_new)

    libcairo = dlopen(SONAME_LIBCAIRO, RTLD_NOW);
    if (!libcairo)
    {
        FIXME("Wine cannot find the %s library.\n", SONAME_LIBCAIRO);
        goto error;
    }

    LOAD_FUNCPTR(libcairo, cairo_create)
    LOAD_FUNCPTR(libcairo, cairo_destroy)
    LOAD_FUNCPTR(libcairo, cairo_image_surface_create)
    LOAD_FUNCPTR(libcairo, cairo_image_surface_get_data)
    LOAD_FUNCPTR(libcairo, cairo_image_surface_get_stride)
    LOAD_FUNCPTR(libcairo, cairo_surface_destroy)
    LOAD_FUNCPTR(libcairo, cairo_surface_flush)

    libgobject2 = dlopen(SONAME_LIBGOBJECT_2_0, RTLD_NOW);
    if (!libgobject2)
    {
        FIXME("Wine cannot find the %s library.\n", SONAME_LIBGOBJECT_2_0);
        goto error;
    }

    LOAD_FUNCPTR(libgobject2, g_type_check_instance_is_a)
    return TRUE;

error:
    free_gtk3_libs();
    return FALSE;
}

static BOOL init_gtk3(void)
{
    int i, colors[NUM_SYS_COLORS];
    COLORREF refs[NUM_SYS_COLORS];
    NONCLIENTMETRICSW metrics;
    static BOOL initialized;

    if (!load_gtk3_libs())
        return FALSE;

    if (initialized)
        return TRUE;
    initialized = TRUE;

    pgtk_init(0, NULL); /* Otherwise every call to GTK will fail */

    /* apply colors */
    for (i = 0; i < NUM_SYS_COLORS; i++)
    {
        colors[i] = i;
        refs[i] = uxtheme_gtk_GetThemeSysColor(NULL, i);
    }
    SetSysColors(NUM_SYS_COLORS, colors, refs);

    /* fix sys params */
    memset(&metrics, 0, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
    metrics.iMenuHeight = MENU_HEIGHT;
    SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);

    SystemParametersInfoW(SPI_SETCLEARTYPE, 0, (LPVOID)TRUE, 0);
    SystemParametersInfoW(SPI_SETFONTSMOOTHING, 0, (LPVOID)TRUE, 0);
    SystemParametersInfoW(SPI_SETFLATMENU, 0, (LPVOID)TRUE, 0);

    return TRUE;
}

void uxgtk_theme_init(uxgtk_theme_t *theme, const uxgtk_theme_vtable_t *vtable)
{
    theme->vtable = vtable;
    theme->window = pgtk_window_new(GTK_WINDOW_TOPLEVEL);
    theme->layout = pgtk_fixed_new();
    pgtk_container_add((GtkContainer *)theme->window, theme->layout);
}

static const struct
{
    const WCHAR *classname;
    uxgtk_theme_t *(*create)(void);
}
classes[] =
{
    { VSCLASS_BUTTON,   uxgtk_button_theme_create },
    { VSCLASS_COMBOBOX, uxgtk_combobox_theme_create },
    { VSCLASS_EDIT,     uxgtk_edit_theme_create },
    { VSCLASS_HEADER,   uxgtk_header_theme_create },
    { VSCLASS_LISTBOX,  uxgtk_listbox_theme_create },
    { VSCLASS_LISTVIEW, uxgtk_listview_theme_create },
    { VSCLASS_MENU,     uxgtk_menu_theme_create },
    { VSCLASS_REBAR,    uxgtk_rebar_theme_create },
    { VSCLASS_STATUS,   uxgtk_status_theme_create },
    { VSCLASS_TAB,      uxgtk_tab_theme_create },
    { VSCLASS_TOOLBAR,  uxgtk_toolbar_theme_create },
    { VSCLASS_TRACKBAR, uxgtk_trackbar_theme_create },
    { VSCLASS_WINDOW,   uxgtk_window_theme_create }
};

BOOL uxtheme_gtk_enabled(void)
{
    static const WCHAR keypath[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e',0};
    static const WCHAR value[] = {'T','h','e','m','e','E','n','g','i','n','e',0};
    static const WCHAR gtk[] = {'G','T','K',0};
    WCHAR buffer[4];
    DWORD size = sizeof(buffer);
    HKEY key;
    LONG l;

    /* @@ Wine registry key: HKCU\Software\Wine */
    if (RegOpenKeyExW(HKEY_CURRENT_USER, keypath, 0, KEY_READ, &key))
        return FALSE;

    l = RegQueryValueExW(key, value, NULL, NULL, (BYTE *)buffer, &size);
    RegCloseKey(key);

    if (l || lstrcmpiW(buffer, gtk))
        return FALSE;

    return init_gtk3();
}

HRESULT uxtheme_gtk_CloseThemeData(HTHEME htheme)
{
    uxgtk_theme_t *theme = (uxgtk_theme_t *)htheme;
    WORD fpu_flags;

    TRACE("(%p)\n", htheme);

    /* Destroy the toplevel widget */
    fpu_flags = reset_fpu_flags();
    pgtk_widget_destroy(theme->window);
    set_fpu_flags(fpu_flags);

    HeapFree(GetProcessHeap(), 0, theme);
    return S_OK;
}

HRESULT uxtheme_gtk_EnableThemeDialogTexture(HWND hwnd, DWORD flags)
{
    HTHEME htheme;

    TRACE("(%p, %u)\n", hwnd, flags);

    if (libgtk3 == NULL)
        return E_NOTIMPL;

    if (flags & ETDT_USETABTEXTURE)
    {
        htheme = GetWindowTheme(hwnd);
        OpenThemeData(hwnd, VSCLASS_TAB);
        CloseThemeData(htheme);
    }

    return S_OK; /* Always enabled */
}

HRESULT uxtheme_gtk_EnableTheming(BOOL enable)
{
    TRACE("(%u)\n", enable);

    return S_OK; /* Always enabled */
}

HRESULT uxtheme_gtk_GetCurrentThemeName(LPWSTR filename, int filename_maxlen,
                                        LPWSTR color, int color_maxlen,
                                        LPWSTR size, int size_maxlen)
{
    TRACE("(%p, %d, %p, %d, %p, %d)\n", filename, filename_maxlen,
          color, color_maxlen, size, size_maxlen);

    return E_FAIL; /* To prevent calling EnumThemeColors and so on */
}

DWORD uxtheme_gtk_GetThemeAppProperties(void)
{
    TRACE("()\n");

    return STAP_ALLOW_CONTROLS; /* Non-client drawing is not supported */
}

BOOL uxtheme_gtk_IsThemeDialogTextureEnabled(HWND hwnd)
{
    TRACE("(%p)\n", hwnd);

    return TRUE; /* Always enabled */
}

HTHEME uxtheme_gtk_OpenThemeDataEx(HWND hwnd, LPCWSTR classlist, DWORD flags)
{
    WCHAR *start, *tok, buf[CLASSLIST_MAXLEN];
    uxgtk_theme_t *theme;
    WORD fpu_flags;
    int i;

    TRACE("(%p, %s, %#x)\n", hwnd, debugstr_w(classlist), flags);

    lstrcpynW(buf, classlist, CLASSLIST_MAXLEN - 1);
    buf[CLASSLIST_MAXLEN - 1] = L'\0';

    /* search for the first match */
    start = buf;
    for (tok = buf; *tok; tok++)
    {
        if (*tok != ';') continue;
        *tok = 0;

        for (i = 0; i < sizeof(classes) / sizeof(classes[0]); i++)
            if (lstrcmpiW(start, classes[i].classname) == 0) goto found;

        start = tok + 1;
    }
    if (start != tok)
    {
        for (i = 0; i < sizeof(classes) / sizeof(classes[0]); i++)
            if (lstrcmpiW(start, classes[i].classname) == 0) goto found;
    }

    FIXME("No matching theme for %s.\n", debugstr_w(classlist));
    SetLastError(ERROR_NOT_FOUND);
    return NULL;

found:
    TRACE("Using %s for %s.\n", debugstr_w(classes[i].classname),
          debugstr_w(classlist));

    fpu_flags = reset_fpu_flags();
    theme = classes[i].create();
    set_fpu_flags(fpu_flags);

    return theme;
}

void uxtheme_gtk_SetThemeAppProperties(DWORD flags)
{
    TRACE("(%u)\n", flags);
}

HRESULT uxtheme_gtk_SetWindowTheme(HWND hwnd, LPCWSTR sub_app_name, LPCWSTR sub_id_list)
{
    FIXME("(%p, %s, %s)\n", hwnd, debugstr_w(sub_app_name),
          debugstr_w(sub_id_list));

    return S_OK;
}

HRESULT uxtheme_gtk_GetThemeBool(HTHEME htheme, int part_id, int state_id,
                                 int prop_id, BOOL *value)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, value);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeColor(HTHEME htheme, int part_id, int state_id,
                                  int prop_id, COLORREF *color)
{
    HRESULT hr;
    GdkRGBA rgba = {0, 0, 0, 0};
    uxgtk_theme_t *theme = (uxgtk_theme_t *)htheme;
    WORD fpu_flags;

    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, color);

    if (theme->vtable == NULL)
        return E_HANDLE;

    if (theme->vtable->get_color == NULL)
        return E_NOTIMPL;

    if (color == NULL)
        return E_INVALIDARG;

    fpu_flags = reset_fpu_flags();
    hr = theme->vtable->get_color(theme, part_id, state_id, prop_id, &rgba);
    set_fpu_flags(fpu_flags);

    if (SUCCEEDED(hr) && rgba.alpha > 0)
    {
        *color = RGB((int)(0.5 + CLAMP(rgba.red, 0.0, 1.0) * 255.0),
                     (int)(0.5 + CLAMP(rgba.green, 0.0, 1.0) * 255.0),
                     (int)(0.5 + CLAMP(rgba.blue, 0.0, 1.0) * 255.0));
        return S_OK;
    }

    return E_FAIL;
}

HRESULT uxtheme_gtk_GetThemeEnumValue(HTHEME htheme, int part_id, int state_id,
                                      int prop_id, int *value)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, value);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeFilename(HTHEME htheme, int part_id, int state_id,
                                     int prop_id, LPWSTR filename, int maxlen)
{
    TRACE("(%p, %d, %d, %d, %p, %d)\n", htheme, part_id, state_id, prop_id,
          filename, maxlen);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeFont(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                 int prop_id, LOGFONTW *font)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, font);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeInt(HTHEME htheme, int part_id, int state_id,
                                int prop_id, int *value)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, value);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeIntList(HTHEME htheme, int part_id, int state_id,
                                    int prop_id, INTLIST *intlist)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, intlist);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeMargins(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                    int prop_id, LPRECT rect, MARGINS *margins)
{
    TRACE("(%p, %d, %d, %d, %p, %p)\n", htheme, part_id, state_id, prop_id, rect, margins);

    memset(margins, 0, sizeof(MARGINS)); /* Set all margins to 0 */

    return S_OK;
}

HRESULT uxtheme_gtk_GetThemeMetric(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                   int prop_id, int *value)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, value);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemePosition(HTHEME htheme, int part_id, int state_id,
                                     int prop_id, POINT *point)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, point);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemePropertyOrigin(HTHEME htheme, int part_id, int state_id,
                                           int prop_id, PROPERTYORIGIN *origin)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, origin);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeRect(HTHEME htheme, int part_id, int state_id,
                                 int prop_id, RECT *rect)
{
    TRACE("(%p, %d, %d, %d, %p)\n", htheme, part_id, state_id, prop_id, rect);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeString(HTHEME htheme, int part_id, int state_id,
                                   int prop_id, LPWSTR buffer, int maxlen)
{
    TRACE("(%p, %d, %d, %d, %p, %d)\n", htheme, part_id, state_id, prop_id, buffer,
          maxlen);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeTransitionDuration(HTHEME htheme, int part_id, int state_id_from,
                                               int state_id_to, int prop_id, DWORD *duration)
{
    TRACE("(%p, %d, %d, %d, %d, %p)\n", htheme, part_id, state_id_from, state_id_to, prop_id,
          duration);

    return E_NOTIMPL;
}

BOOL uxtheme_gtk_GetThemeSysBool(HTHEME htheme, int bool_id)
{
    TRACE("(%p, %d)\n", htheme, bool_id);

    SetLastError(ERROR_NOT_SUPPORTED);

    return FALSE;
}

COLORREF uxtheme_gtk_GetThemeSysColor(HTHEME htheme, int color_id)
{
    HRESULT hr = S_OK;
    COLORREF color = 0;

    static HTHEME window_htheme = NULL;
    static HTHEME button_htheme = NULL;
    static HTHEME edit_htheme = NULL;
    static HTHEME menu_htheme = NULL;

    TRACE("(%p, %d)\n", htheme, color_id);

    if (window_htheme == NULL)
    {
        window_htheme = OpenThemeData(NULL, VSCLASS_WINDOW);
        button_htheme = OpenThemeData(NULL, VSCLASS_BUTTON);
        edit_htheme = OpenThemeData(NULL, VSCLASS_EDIT);
        menu_htheme = OpenThemeData(NULL, VSCLASS_MENU);
    }

    switch (color_id)
    {
        case COLOR_BTNFACE:
        case COLOR_SCROLLBAR:
        case COLOR_WINDOWFRAME:
        case COLOR_INACTIVECAPTION:
        case COLOR_GRADIENTINACTIVECAPTION:
        case COLOR_3DDKSHADOW:
        case COLOR_BTNHIGHLIGHT:
        case COLOR_ACTIVEBORDER:
        case COLOR_INACTIVEBORDER:
        case COLOR_APPWORKSPACE:
        case COLOR_BACKGROUND:
        case COLOR_ACTIVECAPTION:
        case COLOR_GRADIENTACTIVECAPTION:
        case COLOR_ALTERNATEBTNFACE:
        case COLOR_INFOBK: /* FIXME */
            hr = GetThemeColor(window_htheme, WP_DIALOG, 0, TMT_FILLCOLOR, &color);
            break;

        case COLOR_3DLIGHT:
        case COLOR_BTNSHADOW:
            hr = GetThemeColor(button_htheme, BP_PUSHBUTTON, PBS_NORMAL, TMT_BORDERCOLOR, &color);
            break;

        case COLOR_BTNTEXT:
        case COLOR_INFOTEXT:
        case COLOR_WINDOWTEXT:
        case COLOR_CAPTIONTEXT:
            hr = GetThemeColor(window_htheme, WP_DIALOG, 0, TMT_TEXTCOLOR, &color);
            break;

        case COLOR_HIGHLIGHTTEXT:
            hr = GetThemeColor(edit_htheme, EP_EDITTEXT, ETS_SELECTED, TMT_TEXTCOLOR, &color);
            break;

        case COLOR_GRAYTEXT:
        case COLOR_INACTIVECAPTIONTEXT:
            hr = GetThemeColor(button_htheme, BP_PUSHBUTTON, PBS_DISABLED, TMT_TEXTCOLOR, &color);
            break;

        case COLOR_HIGHLIGHT:
        case COLOR_MENUHILIGHT:
        case COLOR_HOTLIGHT:
            hr = GetThemeColor(edit_htheme, EP_EDITTEXT, ETS_SELECTED, TMT_FILLCOLOR, &color);
            break;

        case COLOR_MENUBAR:
            hr = GetThemeColor(menu_htheme, MENU_BARBACKGROUND, MB_ACTIVE, TMT_FILLCOLOR, &color);
            break;

        case COLOR_MENU:
            hr = GetThemeColor(menu_htheme, MENU_POPUPBACKGROUND, 0, TMT_FILLCOLOR, &color);
            break;

        case COLOR_MENUTEXT:
            hr = GetThemeColor(menu_htheme, MENU_POPUPITEM, MPI_NORMAL, TMT_TEXTCOLOR, &color);
            break;

        case COLOR_WINDOW:
            hr = GetThemeColor(edit_htheme, EP_EDITTEXT, ETS_NORMAL, TMT_FILLCOLOR, &color);
            break;

        default:
            FIXME("Unknown color %d.\n", color_id);
            return GetSysColor(color_id);
    }

    if (FAILED(hr))
        return GetSysColor(color_id);

    return color;
}

HRESULT uxtheme_gtk_GetThemeSysFont(HTHEME htheme, int font_id, LOGFONTW *font)
{
    TRACE("(%p, %d, %p)\n", htheme, font_id, font);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeSysInt(HTHEME htheme, int int_id, int *value)
{
    TRACE("(%p, %d, %p)\n", htheme, int_id, value);

    return E_NOTIMPL;
}

int uxtheme_gtk_GetThemeSysSize(HTHEME htheme, int size_id)
{
    TRACE("(%p, %d)\n", htheme, size_id);

    SetLastError(ERROR_NOT_SUPPORTED);

    return -1;
}

HRESULT uxtheme_gtk_GetThemeSysString(HTHEME htheme, int string_id, LPWSTR buffer, int maxlen)
{
    TRACE("(%p, %d, %p, %d)\n", htheme, string_id, buffer, maxlen);

    return E_NOTIMPL;
}

static void paint_cairo_surface(cairo_surface_t *surface, HDC target_hdc,
                                int x, int y, int width, int height)
{
    unsigned char *bitmap_data, *surface_data;
    int i, dib_stride, cairo_stride;
    HDC bitmap_hdc;
    HBITMAP bitmap;
    BITMAPINFO info;
    BLENDFUNCTION bf;

    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height; /* top-down, see MSDN */
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB; /* no compression */
    info.bmiHeader.biSizeImage = 0;
    info.bmiHeader.biXPelsPerMeter = 1;
    info.bmiHeader.biYPelsPerMeter = 1;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;

    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 0xff;
    bf.AlphaFormat = AC_SRC_ALPHA;

    bitmap_hdc = CreateCompatibleDC(target_hdc);
    bitmap = CreateDIBSection(bitmap_hdc, &info, DIB_RGB_COLORS,
                              (void **)&bitmap_data, NULL, 0);

    pcairo_surface_flush(surface);

    surface_data = pcairo_image_surface_get_data(surface);
    cairo_stride = pcairo_image_surface_get_stride(surface);
    dib_stride = width * 4;

    for (i = 0; i < height; i++)
        memcpy(bitmap_data + i * dib_stride, surface_data + i * cairo_stride, width * 4);

    SelectObject(bitmap_hdc, bitmap);

    GdiAlphaBlend(target_hdc, x, y, width, height,
                  bitmap_hdc, 0, 0, width, height, bf);

    DeleteObject(bitmap);
    DeleteDC(bitmap_hdc);
}

HRESULT uxtheme_gtk_DrawThemeBackgroundEx(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                          LPCRECT rect, const DTBGOPTS *options)
{
    uxgtk_theme_t *theme = (uxgtk_theme_t *)htheme;
    cairo_surface_t *surface;
    int width, height;
    WORD fpu_flags;
    cairo_t *cr;
    HRESULT hr;

    TRACE("(%p, %p, %d, %d, %p, %p)\n", htheme, hdc, part_id, state_id, rect, options);

    if (theme->vtable == NULL)
        return E_HANDLE;

    if (theme->vtable->draw_background == NULL)
        return E_NOTIMPL;

    width = rect->right - rect->left;
    height = rect->bottom - rect->top;

    fpu_flags = reset_fpu_flags();

    surface = pcairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr = pcairo_create(surface);

    hr = theme->vtable->draw_background(theme, cr, part_id, state_id, width, height);
    if (SUCCEEDED(hr))
        paint_cairo_surface(surface, hdc, rect->left, rect->top, width, height);

    pcairo_destroy(cr);
    pcairo_surface_destroy(surface);

    set_fpu_flags(fpu_flags);

    return hr;
}

HRESULT uxtheme_gtk_DrawThemeTextEx(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                    LPCWSTR text, int length, DWORD flags, RECT *rect,
                                    const DTTOPTS *options)
{
    RECT rt;
    HRESULT hr;
    COLORREF color = RGB(0, 0, 0), oldcolor;

    TRACE("(%p, %p, %d, %d, %s, %#x, %s, %p)\n", htheme, hdc, part_id, state_id,
          wine_dbgstr_wn(text, length), flags, wine_dbgstr_rect(rect), options);

    hr = GetThemeColor(htheme, part_id, state_id, TMT_TEXTCOLOR, &color);
    if (FAILED(hr))
    {
        FIXME("No color.\n");
        /*return hr;*/
    }

    oldcolor = SetTextColor(hdc, color);

    CopyRect(&rt, rect);

    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text, length, &rt, flags);

    SetTextColor(hdc, oldcolor);

    return S_OK;
}

HRESULT uxtheme_gtk_GetThemeBackgroundRegion(HTHEME htheme, HDC hdc, int part_id,
                                             int state_id, LPCRECT rect, HRGN *region)
{
    TRACE("(%p, %p, %d, %d, %p, %p)\n", htheme, hdc, part_id, state_id, rect, region);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemePartSize(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                     RECT *rect, THEMESIZE type, SIZE *size)
{
    uxgtk_theme_t *theme = (uxgtk_theme_t *)htheme;
    HRESULT result;
    WORD fpu_flags;

    TRACE("(%p, %p, %d, %d, %p, %d, %p)\n", htheme, hdc, part_id, state_id, rect, type, size);

    if (theme->vtable == NULL)
        return E_HANDLE;

    if (theme->vtable->get_part_size == NULL)
        return E_NOTIMPL;

    if (rect == NULL || size == NULL)
        return E_INVALIDARG;

    fpu_flags = reset_fpu_flags();
    result = theme->vtable->get_part_size(theme, part_id, state_id, rect, size);
    set_fpu_flags(fpu_flags);

    return result;
}

HRESULT uxtheme_gtk_GetThemeTextExtent(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                       LPCWSTR text, int length, DWORD flags,
                                       LPCRECT bounding_rect, LPRECT extent_rect)
{
    TRACE("(%p, %p, %d, %d, %s, %u, %p, %p)\n", htheme, hdc, part_id, state_id,
          wine_dbgstr_wn(text, length), flags, bounding_rect, extent_rect);

    return E_NOTIMPL;
}

HRESULT uxtheme_gtk_GetThemeTextMetrics(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                        TEXTMETRICW *metric)
{
    TRACE("(%p, %p, %d, %d, %p)\n", htheme, hdc, part_id, state_id, metric);

    if (!GetTextMetricsW(hdc, metric))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

BOOL uxtheme_gtk_IsThemeBackgroundPartiallyTransparent(HTHEME htheme, int part_id, int state_id)
{
    TRACE("(%p, %d, %d)\n", htheme, part_id, state_id);

    return TRUE; /* The most widgets are partially transparent */
}

BOOL uxtheme_gtk_IsThemePartDefined(HTHEME htheme, int part_id, int state_id)
{
    uxgtk_theme_t *theme = (uxgtk_theme_t *)htheme;
    WORD fpu_flags;
    BOOL result;

    TRACE("(%p, %d, %d)\n", htheme, part_id, state_id);

    if (theme->vtable == NULL)
    {
        SetLastError(E_HANDLE);
        return FALSE;
    }

    if (theme->vtable->is_part_defined == NULL)
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    fpu_flags = reset_fpu_flags();
    result = theme->vtable->is_part_defined(part_id, state_id);
    set_fpu_flags(fpu_flags);

    return result;
}

#else

BOOL uxtheme_gtk_enabled(void)
{
    return FALSE;
}

HRESULT uxtheme_gtk_CloseThemeData(HTHEME theme) { return 0; };
HRESULT uxtheme_gtk_EnableThemeDialogTexture(HWND hwnd, DWORD flags) { return 0; };
HRESULT uxtheme_gtk_EnableTheming(BOOL enable) { return 0; };
HRESULT uxtheme_gtk_GetCurrentThemeName(LPWSTR filename, int filename_maxlen,
                                        LPWSTR color, int color_maxlen,
                                        LPWSTR size, int size_maxlen) { return 0; };
DWORD uxtheme_gtk_GetThemeAppProperties(void) { return 0; };
BOOL uxtheme_gtk_IsThemeDialogTextureEnabled(HWND hwnd) { return 0; };
HTHEME uxtheme_gtk_OpenThemeDataEx(HWND hwnd, LPCWSTR classlist, DWORD flags) { return 0; };
void uxtheme_gtk_SetThemeAppProperties(DWORD flags) { };
HRESULT uxtheme_gtk_SetWindowTheme(HWND hwnd, LPCWSTR sub_app_name, LPCWSTR sub_id_list) { return 0; };
HRESULT uxtheme_gtk_GetThemeBool(HTHEME htheme, int part_id, int state_id,
                                 int prop_id, BOOL *value) { return 0; };
HRESULT uxtheme_gtk_GetThemeColor(HTHEME htheme, int part_id, int state_id,
                                  int prop_id, COLORREF *color) { return 0; };
HRESULT uxtheme_gtk_GetThemeEnumValue(HTHEME htheme, int part_id, int state_id,
                                      int prop_id, int *value) { return 0; };
HRESULT uxtheme_gtk_GetThemeFilename(HTHEME htheme, int part_id, int state_id,
                                     int prop_id, LPWSTR filename, int maxlen) { return 0; };
HRESULT uxtheme_gtk_GetThemeFont(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                 int prop_id, LOGFONTW *font) { return 0; };
HRESULT uxtheme_gtk_GetThemeInt(HTHEME htheme, int part_id, int state_id,
                                int prop_id, int *value) { return 0; };
HRESULT uxtheme_gtk_GetThemeIntList(HTHEME htheme, int part_id, int state_id,
                                    int prop_id, INTLIST *intlist) { return 0; };
HRESULT uxtheme_gtk_GetThemeMargins(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                    int prop_id, LPRECT rect, MARGINS *margins) { return 0; };
HRESULT uxtheme_gtk_GetThemeMetric(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                   int prop_id, int *value) { return 0; };
HRESULT uxtheme_gtk_GetThemePosition(HTHEME htheme, int part_id, int state_id,
                                     int prop_id, POINT *point) { return 0; };
HRESULT uxtheme_gtk_GetThemePropertyOrigin(HTHEME htheme, int part_id, int state_id,
                                           int prop_id, PROPERTYORIGIN *origin) { return 0; };
HRESULT uxtheme_gtk_GetThemeRect(HTHEME htheme, int part_id, int state_id,
                                 int prop_id, RECT *rect) { return 0; };
HRESULT uxtheme_gtk_GetThemeString(HTHEME htheme, int part_id, int state_id,
                                   int prop_id, LPWSTR buffer, int maxlen) { return 0; };
HRESULT uxtheme_gtk_GetThemeTransitionDuration(HTHEME htheme, int part_id, int state_id_from,
                                               int state_id_to, int prop_id, DWORD *duration) { return 0; };
BOOL uxtheme_gtk_GetThemeSysBool(HTHEME htheme, int bool_id) { return 0; };
COLORREF uxtheme_gtk_GetThemeSysColor(HTHEME htheme, int color_id) { return 0; };
HRESULT uxtheme_gtk_GetThemeSysFont(HTHEME htheme, int font_id, LOGFONTW *font) { return 0; };
HRESULT uxtheme_gtk_GetThemeSysInt(HTHEME htheme, int int_id, int *value) { return 0; };
int uxtheme_gtk_GetThemeSysSize(HTHEME htheme, int size_id) { return 0; };
HRESULT uxtheme_gtk_GetThemeSysString(HTHEME htheme, int string_id, LPWSTR buffer, int maxlen) { return 0; };
HRESULT uxtheme_gtk_DrawThemeBackgroundEx(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                          LPCRECT rect, const DTBGOPTS *options) { return 0; };
HRESULT uxtheme_gtk_DrawThemeTextEx(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                    LPCWSTR text, int length, DWORD flags, RECT *rect,
                                    const DTTOPTS *options) { return 0; };
HRESULT uxtheme_gtk_GetThemeBackgroundRegion(HTHEME htheme, HDC hdc, int part_id,
                                             int state_id, LPCRECT rect, HRGN *region) { return 0; };
HRESULT uxtheme_gtk_GetThemePartSize(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                     RECT *rect, THEMESIZE type, SIZE *size) { return 0; };
HRESULT uxtheme_gtk_GetThemeTextExtent(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                       LPCWSTR text, int length, DWORD flags,
                                       LPCRECT bounding_rect, LPRECT extent_rect) { return 0; };
HRESULT uxtheme_gtk_GetThemeTextMetrics(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                        TEXTMETRICW *metric) { return 0; };
BOOL uxtheme_gtk_IsThemeBackgroundPartiallyTransparent(HTHEME htheme, int part_id, int state_id) { return 0; };
BOOL uxtheme_gtk_IsThemePartDefined(HTHEME htheme, int part_id, int state_id) { return 0; };

#endif /* HAVE_GTK_GTK_H */
