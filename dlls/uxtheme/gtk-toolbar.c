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
#include "winerror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxthemegtk);

typedef struct _toolbar_theme
{
    uxgtk_theme_t base;

    GtkWidget *button;
    GtkWidget *separator;
} toolbar_theme_t;

static inline toolbar_theme_t *impl_from_uxgtk_theme_t(uxgtk_theme_t *theme)
{
    return CONTAINING_RECORD(theme, toolbar_theme_t, base);
}

static GtkStateFlags get_state_flags(int state_id)
{
    switch (state_id)
    {
        case TS_NORMAL:
            return GTK_STATE_FLAG_NORMAL;

        case TS_HOT:
            return GTK_STATE_FLAG_PRELIGHT;

        case TS_PRESSED:
            return GTK_STATE_FLAG_ACTIVE;

        case TS_DISABLED:
            return GTK_STATE_FLAG_INSENSITIVE;
    }

    FIXME("Unsupported toolbar state %d.\n", state_id);
    return GTK_STATE_FLAG_NORMAL;
}

static HRESULT draw_button(toolbar_theme_t *theme, cairo_t *cr, int state_id, int width, int height)
{
    GtkStateFlags state = get_state_flags(state_id);
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->button);
    pgtk_style_context_save(context);

    pgtk_style_context_set_state(context, state);

    pgtk_render_background(context, cr, 0, 0, width, height);
    pgtk_render_frame(context, cr, 0, 0, width, height);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_separator(toolbar_theme_t *theme, cairo_t *cr, int part_id,
                              int width, int height)
{
    int x1, x2, y1, y2;
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->separator);

    if (part_id == TP_SEPARATOR) /* TP_SEPARATORVERT ? */
    {
        y1 = 0;
        y2 = height;
        x1 = x2 = width/2;
    }
    else
    {
        x1 = 0;
        x2 = width;
        y1 = y2 = height/2;
    }

    pgtk_render_line(context, cr, x1, y1, x2, y2);

    return S_OK;
}

static HRESULT draw_background(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                               int width, int height)
{
    toolbar_theme_t *toolbar_theme = impl_from_uxgtk_theme_t(theme);

    switch (part_id)
    {
        case TP_BUTTON:
            return draw_button(toolbar_theme, cr, state_id, width, height);

        case TP_SEPARATOR:
        case TP_SEPARATORVERT:
            return draw_separator(toolbar_theme, cr, part_id, width, height);
    }

    FIXME("Unsupported toolbar part %d.\n", part_id);
    return E_NOTIMPL;
}

static BOOL is_part_defined(int part_id, int state_id)
{
    return (part_id == TP_BUTTON || part_id == TP_SEPARATOR || part_id == TP_SEPARATORVERT);
}

static const uxgtk_theme_vtable_t toolbar_vtable =
{
    NULL, /* get_color */
    draw_background,
    NULL, /* get_part_size */
    is_part_defined
};

uxgtk_theme_t *uxgtk_toolbar_theme_create(void)
{
    toolbar_theme_t *theme;

    TRACE("()\n");

    theme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*theme));
    if (!theme) return NULL;

    uxgtk_theme_init(&theme->base, &toolbar_vtable);

    theme->button = pgtk_button_new();
    theme->separator = (GtkWidget *)pgtk_separator_tool_item_new();

    pgtk_container_add((GtkContainer *)theme->base.layout, theme->button);
    pgtk_container_add((GtkContainer *)theme->base.layout, theme->separator);

    return &theme->base;
}

#endif /* HAVE_GTK_GTK_H */
