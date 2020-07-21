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

#if defined(HAVE_GTK_GTK_H) && defined(SONAME_LIBGTK_3)

#include "uxthemegtk.h"

#include <assert.h>

#include "winbase.h"
#include "vsstyle.h"
#include "vssym32.h"
#include "winerror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxthemegtk);

typedef struct _window_theme
{
    uxgtk_theme_t base;
} window_theme_t;

static inline window_theme_t *impl_from_uxgtk_theme_t(uxgtk_theme_t *theme)
{
    return CONTAINING_RECORD(theme, window_theme_t, base);
}

static HRESULT get_fill_color(uxgtk_theme_t *theme, int part_id, int state_id, GdkRGBA *rgba)
{
    GtkStyleContext *context;
    GtkStateFlags state;

    assert(theme != NULL);

    switch (part_id)
    {
        case WP_DIALOG:
            state = GTK_STATE_FLAG_NORMAL;
            context = pgtk_widget_get_style_context(theme->window);
            break;

        default:
            FIXME("Unsupported window part %d.\n", part_id);
            return E_NOTIMPL;
    }

    pgtk_style_context_get_background_color(context, state, rgba);

    return S_OK;
}

static HRESULT get_text_color(uxgtk_theme_t *theme, int part_id, int state_id, GdkRGBA *rgba)
{
    GtkStyleContext *context;
    GtkStateFlags state;

    assert(theme != NULL);

    switch (part_id)
    {
        case WP_DIALOG:
            state = GTK_STATE_FLAG_NORMAL;
            context = pgtk_widget_get_style_context(theme->window);
            break;

        default:
            FIXME("Unsupported window part %d.\n", part_id);
            return E_NOTIMPL;
    }

    pgtk_style_context_get_color(context, state, rgba);

    return S_OK;
}

static HRESULT get_color(uxgtk_theme_t *theme, int part_id, int state_id,
                         int prop_id, GdkRGBA *rgba)
{
    switch (prop_id)
    {
        case TMT_FILLCOLOR:
            return get_fill_color(theme, part_id, state_id, rgba);

        case TMT_TEXTCOLOR:
            return get_text_color(theme, part_id, state_id, rgba);

        default:
            FIXME("Unsupported property %d.\n", prop_id);
            return E_FAIL;
    }

    return S_OK;
}

static HRESULT draw_dialog(uxgtk_theme_t *theme, cairo_t *cr, int state_id, int width, int height)
{
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->window);

    pgtk_render_background(context, cr, 0, 0, width, height);

    return S_OK;
}

static HRESULT draw_background(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                               int width, int height)
{
    switch (part_id)
    {
        case WP_DIALOG:
            return draw_dialog(theme, cr, state_id, width, height);
    }

    FIXME("Unsupported window part %d.\n", part_id);
    return E_NOTIMPL;
}

static BOOL is_part_defined(int part_id, int state_id)
{
    return (part_id == WP_DIALOG);
}

static const uxgtk_theme_vtable_t window_vtable =
{
    get_color,
    draw_background,
    NULL, /* get_part_size */
    is_part_defined
};

uxgtk_theme_t *uxgtk_window_theme_create(void)
{
    window_theme_t *theme;
    GtkStyleContext *context;

    TRACE("()\n");

    theme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*theme));
    if (!theme) return NULL;

    uxgtk_theme_init(&theme->base, &window_vtable);

    context = pgtk_widget_get_style_context(theme->base.window);
    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_BACKGROUND);

    return &theme->base;
}

#endif /* HAVE_GTK_GTK_H */
