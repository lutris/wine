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

typedef struct _status_theme
{
    uxgtk_theme_t base;

    int grip_width;
    int grip_height;
} status_theme_t;

static inline status_theme_t *impl_from_uxgtk_theme_t(uxgtk_theme_t *theme)
{
    return CONTAINING_RECORD(theme, status_theme_t, base);
}

static HRESULT draw_pane(uxgtk_theme_t *theme, cairo_t *cr, int width, int height)
{
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->window);

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_BACKGROUND);
    pgtk_render_background(context, cr, 0, 0, width, height);

    return S_OK;
}

static HRESULT draw_gripper(uxgtk_theme_t *theme, cairo_t *cr, int width, int height)
{
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->window);
    pgtk_style_context_save(context);

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_GRIP);
    pgtk_style_context_set_junction_sides(context, GTK_JUNCTION_CORNER_BOTTOMRIGHT);
    pgtk_render_handle(context, cr, 0, 0, width, height);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_background(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                               int width, int height)
{
    TRACE("(%p, %p, %d, %d, %d, %d)\n", theme, cr, part_id, state_id, width, height);

    switch (part_id)
    {
        case 0:
        case SP_PANE:
        case SP_GRIPPERPANE:
            return draw_pane(theme, cr, width, height);

        case SP_GRIPPER:
            return draw_gripper(theme, cr, width, height);
    }

    ERR("Unknown status part %d.\n", part_id);
    return E_FAIL;
}

static HRESULT get_part_size(uxgtk_theme_t *theme, int part_id, int state_id,
                             RECT *rect, SIZE *size)
{
    status_theme_t *status_theme = impl_from_uxgtk_theme_t(theme);

    assert(theme != NULL);
    assert(size != NULL);

    switch (part_id)
    {
        case SP_GRIPPER:
            size->cx = status_theme->grip_width;
            size->cy = status_theme->grip_height;
            return S_OK;
    }

    FIXME("Unsupported status part %d.\n", part_id);
    return E_NOTIMPL;
}

static BOOL is_part_defined(int part_id, int state_id)
{
    return (part_id >= 0 && part_id <= SP_GRIPPER);
}

static const uxgtk_theme_vtable_t status_vtable =
{
    NULL, /* get_color */
    draw_background,
    get_part_size,
    is_part_defined
};

uxgtk_theme_t *uxgtk_status_theme_create(void)
{
    status_theme_t *theme;

    TRACE("()\n");

    theme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*theme));
    if (!theme) return NULL;

    uxgtk_theme_init(&theme->base, &status_vtable);

    pgtk_widget_style_get(theme->base.window,
                          "resize-grip-width", &theme->grip_width,
                          "resize-grip-height", &theme->grip_height,
                          NULL);

    TRACE("-GtkWindow-resize-grip-width: %d\n", theme->grip_width);
    TRACE("-GtkWindow-resize-grip-height: %d\n", theme->grip_height);

    return &theme->base;
}

#endif /* HAVE_GTK_GTK_H */
