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

typedef struct _trackbar_theme
{
    uxgtk_theme_t base;

    int slider_length;
    int slider_width;

    GtkWidget *scale;
} trackbar_theme_t;

static inline trackbar_theme_t *impl_from_uxgtk_theme_t(uxgtk_theme_t *theme)
{
    return CONTAINING_RECORD(theme, trackbar_theme_t, base);
}

static HRESULT draw_track(trackbar_theme_t *theme, cairo_t *cr, int part_id, int width, int height)
{
    int x1, x2, y1, y2;
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->scale);
    pgtk_style_context_save(context);

    if (part_id == TKP_TRACKVERT)
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

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_SEPARATOR);

    pgtk_render_line(context, cr, x1, y1, x2, y2);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_thumb(trackbar_theme_t *theme, cairo_t *cr, int state_id, int width, int height)
{
    GtkStateFlags state = GTK_STATE_FLAG_NORMAL;
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->scale);
    pgtk_style_context_save(context);

    if (state_id == TUS_HOT)
        state = GTK_STATE_FLAG_PRELIGHT;
    else if (state_id == TUBS_PRESSED)
        state = GTK_STATE_FLAG_ACTIVE;

    pgtk_style_context_set_state(context, state);

    if (width > height)
        if (theme->slider_length > theme->slider_width)
            pgtk_style_context_add_class(context, GTK_STYLE_CLASS_HORIZONTAL);
        else
            pgtk_style_context_add_class(context, GTK_STYLE_CLASS_VERTICAL);
    else
        if (theme->slider_length > theme->slider_width)
            pgtk_style_context_add_class(context, GTK_STYLE_CLASS_VERTICAL);
        else
            pgtk_style_context_add_class(context, GTK_STYLE_CLASS_HORIZONTAL);

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_SCALE);
    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_SLIDER);

    pgtk_render_slider(context, cr, 0, 0, theme->slider_length, theme->slider_width,
                       GTK_ORIENTATION_HORIZONTAL);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_background(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                               int width, int height)
{
    trackbar_theme_t *trackbar_theme = impl_from_uxgtk_theme_t(theme);

    TRACE("(%p, %p, %d, %d, %d, %d)\n", theme, cr, part_id, state_id, width, height);

    switch (part_id)
    {
        case TKP_THUMB:
        case TKP_THUMBBOTTOM:
        case TKP_THUMBTOP:
        case TKP_THUMBVERT:
        case TKP_THUMBLEFT:
        case TKP_THUMBRIGHT:
            return draw_thumb(trackbar_theme, cr, state_id, width, height);

        case TKP_TRACK:
        case TKP_TRACKVERT:
            return draw_track(trackbar_theme, cr, part_id, width, height);
    }

    FIXME("Unsupported trackbar part %d.\n", part_id);
    return E_NOTIMPL;
}

static BOOL is_part_defined(int part_id, int state_id)
{
    return (part_id > 0 && part_id < TKP_TICS);
}

static const uxgtk_theme_vtable_t trackbar_vtable =
{
    NULL, /* get_color */
    draw_background,
    NULL, /* get_part_size */
    is_part_defined
};

uxgtk_theme_t *uxgtk_trackbar_theme_create(void)
{
    trackbar_theme_t *theme;

    TRACE("()\n");

    theme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*theme));
    if (!theme) return NULL;

    uxgtk_theme_init(&theme->base, &trackbar_vtable);

    theme->scale = pgtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);

    pgtk_container_add((GtkContainer *)theme->base.layout, theme->scale);

    pgtk_widget_style_get(theme->scale,
                          "slider-length", &theme->slider_length,
                          "slider-width", &theme->slider_width,
                          NULL);

    TRACE("-GtkScale-slider-length: %d\n", theme->slider_length);
    TRACE("-GtkScale-slider-width: %d\n", theme->slider_width);

    return &theme->base;
}

#endif /* HAVE_GTK_GTK_H */
