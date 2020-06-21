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

typedef struct _tab_theme
{
    uxgtk_theme_t base;

    int tab_overlap;

    GtkWidget *notebook;
} tab_theme_t;

static inline tab_theme_t *impl_from_uxgtk_theme_t(uxgtk_theme_t *theme)
{
    return CONTAINING_RECORD(theme, tab_theme_t, base);
}

static HRESULT draw_tab_item(tab_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                             int width, int height)
{
    int x = 0, new_width = width, new_height = height;
    GtkRegionFlags region = 0;
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->notebook);
    pgtk_style_context_save(context);

    /* Emulate the "-GtkNotebook-tab-overlap" style property */
    if (part_id == TABP_TABITEM || part_id == TABP_TABITEMRIGHTEDGE)
    {
        x = -theme->tab_overlap;
        new_width += theme->tab_overlap;
    }

    /* Provide GTK a little bit more information about the tab position */
    if (part_id == TABP_TABITEMLEFTEDGE || part_id == TABP_TOPTABITEMLEFTEDGE)
        region = GTK_REGION_FIRST;
    else if (part_id == TABP_TABITEMRIGHTEDGE || part_id == TABP_TOPTABITEMRIGHTEDGE)
        region = GTK_REGION_LAST;
    else if (part_id == TABP_TABITEMBOTHEDGE || part_id == TABP_TOPTABITEMBOTHEDGE)
        region = GTK_REGION_ONLY;
    pgtk_style_context_add_region(context, GTK_STYLE_REGION_TAB, region);

    /* Some themes are not friendly with the TCS_MULTILINE tabs */
    pgtk_style_context_set_junction_sides(context, GTK_JUNCTION_BOTTOM);

    /* Active tabs have their own parts */
    if (part_id > TABP_TABITEMBOTHEDGE && part_id < TABP_PANE) {
        new_height--; /* Fix the active tab height */
        pgtk_style_context_set_state(context, GTK_STATE_FLAG_ACTIVE);
    }

    pgtk_render_background(context, cr, x, 0, new_width, new_height);
    pgtk_render_frame(context, cr, x, 0, new_width, new_height);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_tab_pane(tab_theme_t *theme, cairo_t *cr, int width, int height)
{
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->notebook);
    pgtk_style_context_save(context);

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_FRAME);
    pgtk_style_context_set_junction_sides(context, GTK_JUNCTION_TOP);

    pgtk_render_background(context, cr, 0, 0, width, height);
    pgtk_render_frame(context, cr, 0, 0, width, height);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_tab_body(tab_theme_t *theme, cairo_t *cr, int width, int height)
{
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->notebook);

    /* Some borders are already drawned by draw_tab_pane */
    pgtk_render_background(context, cr, -4, -4, width + 4, height + 4);

    return S_OK;
}

static HRESULT draw_background(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                               int width, int height)
{
    tab_theme_t *tab_theme = impl_from_uxgtk_theme_t(theme);
    GtkStyleContext *context;

    assert(theme != NULL);

    /* Draw a dialog background to fix some themes like Ambiance */
    context = pgtk_widget_get_style_context(theme->window);
    pgtk_render_background(context, cr, 0, 0, width, height - 1);

    switch (part_id)
    {
        case TABP_TABITEM:
        case TABP_TABITEMLEFTEDGE:
        case TABP_TABITEMRIGHTEDGE:
        case TABP_TABITEMBOTHEDGE:
        case TABP_TOPTABITEM:
        case TABP_TOPTABITEMLEFTEDGE:
        case TABP_TOPTABITEMRIGHTEDGE:
        case TABP_TOPTABITEMBOTHEDGE:
            return draw_tab_item(tab_theme, cr, part_id, state_id, width, height);

        case TABP_PANE:
            return draw_tab_pane(tab_theme, cr, width, height);

        case TABP_BODY:
        case TABP_AEROWIZARDBODY:
            return draw_tab_body(tab_theme, cr, width, height);
    }

    ERR("Unknown tab part %d.\n", part_id);
    return E_FAIL;
}

static BOOL is_part_defined(int part_id, int state_id)
{
    return (part_id > 0 && part_id <= TABP_AEROWIZARDBODY);
}

static const uxgtk_theme_vtable_t tab_vtable =
{
    NULL, /* get_color */
    draw_background,
    NULL, /* get_part_size */
    is_part_defined
};

uxgtk_theme_t *uxgtk_tab_theme_create(void)
{
    tab_theme_t *theme;
    GtkStyleContext *context;

    TRACE("()\n");

    theme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*theme));
    if (!theme) return NULL;

    uxgtk_theme_init(&theme->base, &tab_vtable);

    theme->notebook = pgtk_notebook_new();

    pgtk_container_add((GtkContainer *)theme->base.layout, theme->notebook);

    context = pgtk_widget_get_style_context(theme->notebook);

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_NOTEBOOK);
    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_TOP);

    pgtk_widget_style_get(theme->notebook, "tab-overlap", &theme->tab_overlap, NULL);

    TRACE("-GtkNotebook-tab-overlap: %d\n", theme->tab_overlap);

    return &theme->base;
}

#endif /* HAVE_GTK_GTK_H */
