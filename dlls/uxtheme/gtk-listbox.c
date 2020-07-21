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

typedef struct _listbox_theme
{
    uxgtk_theme_t base;

    GtkWidget *scrolled_window;
} listbox_theme_t;

static inline listbox_theme_t *impl_from_uxgtk_theme_t(uxgtk_theme_t *theme)
{
    return CONTAINING_RECORD(theme, listbox_theme_t, base);
}

static HRESULT draw_border(listbox_theme_t *theme, cairo_t *cr, int width, int height)
{
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->scrolled_window);
    pgtk_style_context_save(context);

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_VIEW);
    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_FRAME);

    pgtk_render_background(context, cr, 0, 0, width, height);
    pgtk_render_frame(context, cr, 0, 0, width, height);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_background(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                               int width, int height)
{
    listbox_theme_t *listbox_theme = impl_from_uxgtk_theme_t(theme);

    switch (part_id)
    {
        case 0:
        case LBCP_BORDER_HSCROLL:
        case LBCP_BORDER_HVSCROLL:
        case LBCP_BORDER_NOSCROLL:
        case LBCP_BORDER_VSCROLL:
            return draw_border(listbox_theme, cr, width, height);
    }

    FIXME("Unsupported listbox part %d.\n", part_id);
    return E_NOTIMPL;
}

static BOOL is_part_defined(int part_id, int state_id)
{
    return (part_id >= 0 && part_id < LBCP_ITEM);
}

static const uxgtk_theme_vtable_t listbox_vtable =
{
    NULL, /* get_color */
    draw_background,
    NULL, /* get_part_size */
    is_part_defined
};

uxgtk_theme_t *uxgtk_listbox_theme_create(void)
{
    listbox_theme_t *theme;

    TRACE("()\n");

    theme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*theme));
    if (!theme) return NULL;

    uxgtk_theme_init(&theme->base, &listbox_vtable);

    theme->scrolled_window = pgtk_scrolled_window_new(NULL, NULL);

    pgtk_container_add((GtkContainer *)theme->base.layout, theme->scrolled_window);

    return &theme->base;
}

#endif /* HAVE_GTK_GTK_H */
