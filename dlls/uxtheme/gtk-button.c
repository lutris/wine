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

#if !GTK_CHECK_VERSION(3, 14, 0)
# define GTK_STATE_FLAG_CHECKED (1 << 11)
#endif

WINE_DEFAULT_DEBUG_CHANNEL(uxthemegtk);

typedef struct _button_theme
{
    uxgtk_theme_t base;

    int indicator_size;

    GtkWidget *button;
    GtkWidget *check;
    GtkWidget *radio;
    GtkWidget *frame;
    GtkWidget *label;
    GtkWidget *button_label;
    GtkWidget *check_label;
    GtkWidget *radio_label;
} button_theme_t;

static inline button_theme_t *impl_from_uxgtk_theme_t(uxgtk_theme_t *theme)
{
    return CONTAINING_RECORD(theme, button_theme_t, base);
}

static GtkWidget *get_button(button_theme_t *theme)
{
    assert(theme != NULL);

    if (theme->button == NULL)
    {
        theme->button = pgtk_button_new();
        pgtk_container_add((GtkContainer *)theme->base.layout, theme->button);
    }

    return theme->button;
}

static GtkWidget *get_radio(button_theme_t *theme)
{
    assert(theme != NULL);

    if (theme->radio == NULL)
    {
        theme->radio = pgtk_radio_button_new(NULL);
        pgtk_container_add((GtkContainer *)theme->base.layout, theme->radio);
    }

    return theme->radio;
}

static GtkWidget *get_frame(button_theme_t *theme)
{
    assert(theme != NULL);

    if (theme->frame == NULL)
    {
        theme->frame = pgtk_frame_new(NULL);
        pgtk_container_add((GtkContainer *)theme->base.layout, theme->frame);
    }

    return theme->frame;
}

static GtkWidget *get_label(button_theme_t *theme)
{
    assert(theme != NULL);

    if (theme->label == NULL)
    {
        theme->label = pgtk_label_new(NULL);
        pgtk_container_add((GtkContainer *)theme->base.layout, theme->label);
    }

    return theme->label;
}

static GtkWidget *get_button_label(button_theme_t *theme)
{
    assert(theme != NULL);

    if (theme->button_label == NULL)
    {
        GtkWidget *button = get_button(theme);
        theme->button_label = pgtk_label_new(NULL);
        pgtk_container_add((GtkContainer *)button, theme->button_label);
    }

    return theme->button_label;
}

static GtkWidget *get_check_label(button_theme_t *theme)
{
    assert(theme != NULL);

    if (theme->check_label == NULL)
    {
        theme->check_label = pgtk_label_new(NULL);
        pgtk_container_add((GtkContainer *)theme->check, theme->check_label);
    }

    return theme->check_label;
}

static GtkWidget *get_radio_label(button_theme_t *theme)
{
    assert(theme != NULL);

    if (theme->radio_label == NULL)
    {
        GtkWidget *radio = get_radio(theme);
        theme->radio_label = pgtk_label_new(NULL);
        pgtk_container_add((GtkContainer *)radio, theme->radio_label);
    }

    return theme->radio_label;
}

static GtkStateFlags get_push_button_state_flags(int state_id)
{
    switch (state_id)
    {
        case PBS_NORMAL:
            return GTK_STATE_FLAG_NORMAL;

        case PBS_HOT:
            return GTK_STATE_FLAG_PRELIGHT;

        case PBS_PRESSED:
            return GTK_STATE_FLAG_ACTIVE;

        case PBS_DISABLED:
            return GTK_STATE_FLAG_INSENSITIVE;

        case PBS_DEFAULTED:
            return GTK_STATE_FLAG_FOCUSED;
    }

    FIXME("Unsupported push button state %d.\n", state_id);
    return GTK_STATE_FLAG_NORMAL;
}

static GtkStateFlags get_radio_button_state_flags(int state_id)
{
    switch (state_id)
    {
        case RBS_UNCHECKEDNORMAL:
            return GTK_STATE_FLAG_NORMAL;

        case RBS_UNCHECKEDHOT:
            return GTK_STATE_FLAG_PRELIGHT;

        case RBS_UNCHECKEDPRESSED:
            return GTK_STATE_FLAG_ACTIVE;

        case RBS_UNCHECKEDDISABLED:
            return GTK_STATE_FLAG_INSENSITIVE;

        case RBS_CHECKEDNORMAL:
            return GTK_STATE_FLAG_NORMAL | GTK_STATE_FLAG_ACTIVE;

        case RBS_CHECKEDHOT:
            return GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE;

        case RBS_CHECKEDPRESSED:
            return GTK_STATE_FLAG_ACTIVE;

        case RBS_CHECKEDDISABLED:
            return GTK_STATE_FLAG_INSENSITIVE | GTK_STATE_FLAG_ACTIVE;
    }

    ERR("Unknown radio button state %d.\n", state_id);
    return GTK_STATE_FLAG_NORMAL;
}

static GtkStateFlags get_checkbox_state_flags(int state_id)
{
    switch (state_id)
    {
        case CBS_UNCHECKEDNORMAL:
            return GTK_STATE_FLAG_NORMAL;

        case CBS_UNCHECKEDHOT:
            return GTK_STATE_FLAG_PRELIGHT;

        case CBS_UNCHECKEDPRESSED:
            return GTK_STATE_FLAG_SELECTED;

        case CBS_UNCHECKEDDISABLED:
            return GTK_STATE_FLAG_INSENSITIVE;

        case CBS_CHECKEDNORMAL:
            return GTK_STATE_FLAG_NORMAL | GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_CHECKED;

        case CBS_CHECKEDHOT:
            return GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_CHECKED;

        case CBS_CHECKEDPRESSED:
            return GTK_STATE_FLAG_SELECTED | GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_CHECKED;

        case CBS_CHECKEDDISABLED:
            return GTK_STATE_FLAG_INSENSITIVE | GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_CHECKED;

        case CBS_MIXEDNORMAL:
            return GTK_STATE_FLAG_NORMAL | GTK_STATE_FLAG_INCONSISTENT;

        case CBS_MIXEDHOT:
            return GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_INCONSISTENT;

        case CBS_MIXEDPRESSED:
            return GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_CHECKED | GTK_STATE_FLAG_INCONSISTENT;

        case CBS_MIXEDDISABLED:
            return GTK_STATE_FLAG_INSENSITIVE | GTK_STATE_FLAG_INCONSISTENT;
    }

    FIXME("Unsupported checkbox state %d.\n", state_id);
    return GTK_STATE_FLAG_NORMAL;
}

static GtkStateFlags get_groupbox_state_flags(int state_id)
{
    switch (state_id)
    {
        case GBS_NORMAL:
            return GTK_STATE_FLAG_NORMAL;

        case GBS_DISABLED:
            return GTK_STATE_FLAG_INSENSITIVE;
    }

    ERR("Unknown groupbox state %d.\n", state_id);
    return GTK_STATE_FLAG_NORMAL;
}

static HRESULT get_border_color(button_theme_t *theme, int part_id, int state_id, GdkRGBA *rgba)
{
    GtkStateFlags state;
    GtkStyleContext *context;

    switch (part_id)
    {
        case BP_PUSHBUTTON:
            state = get_push_button_state_flags(state_id);
            break;

        case BP_RADIOBUTTON:
            state = get_radio_button_state_flags(state_id);
            break;

        case BP_CHECKBOX:
            state = get_checkbox_state_flags(state_id);
            break;

        case BP_GROUPBOX:
            state = get_groupbox_state_flags(state_id);
            break;

        default:
            FIXME("Unsupported button part %d.\n", part_id);
            return E_NOTIMPL;
    }

    context = pgtk_widget_get_style_context(get_frame(theme));

    pgtk_style_context_save(context);

    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_FRAME);
    pgtk_style_context_get_border_color(context, state, rgba);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT get_text_color(button_theme_t *theme, int part_id, int state_id, GdkRGBA *rgba)
{
    GtkWidget *widget;
    GtkStateFlags state;
    GtkStyleContext *context;

    switch (part_id)
    {
        case BP_PUSHBUTTON:
            widget = get_button_label(theme);
            state = get_push_button_state_flags(state_id);
            break;

        case BP_RADIOBUTTON:
            widget = get_radio_label(theme);
            state = get_radio_button_state_flags(state_id);
            break;

        case BP_CHECKBOX:
            widget = get_check_label(theme);
            state = get_checkbox_state_flags(state_id);
            break;

        case BP_GROUPBOX:
            widget = get_label(theme);
            state = get_groupbox_state_flags(state_id);
            break;

        default:
            FIXME("Unsupported button part %d.\n", part_id);
            return E_NOTIMPL;
    }

    context = pgtk_widget_get_style_context(widget);
    pgtk_style_context_get_color(context, state, rgba);

    return S_OK;
}

static HRESULT get_color(uxgtk_theme_t *theme, int part_id, int state_id,
                         int prop_id, GdkRGBA *rgba)
{
    button_theme_t *button_theme = impl_from_uxgtk_theme_t(theme);

    switch (prop_id)
    {
        case TMT_BORDERCOLOR:
            return get_border_color(button_theme, part_id, state_id, rgba);

        case TMT_TEXTCOLOR:
            return get_text_color(button_theme, part_id, state_id, rgba);
    }

    FIXME("Unsupported button color %d.\n", prop_id);
    return E_NOTIMPL;
}

static HRESULT draw_button(button_theme_t *theme, cairo_t *cr, int state_id, int width, int height)
{
    GtkStateFlags state = get_push_button_state_flags(state_id);
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(get_button(theme));
    pgtk_style_context_save(context);

    pgtk_style_context_set_state(context, state);

    if (state_id == PBS_DEFAULTED)
        pgtk_style_context_add_class(context, GTK_STYLE_CLASS_DEFAULT);

    pgtk_render_background(context, cr, 0, 0, width, height);
    pgtk_render_frame(context, cr, 0, 0, width, height);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_radio(button_theme_t *theme, cairo_t *cr, int state_id)
{
    GtkStateFlags state = get_radio_button_state_flags(state_id);
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(get_radio(theme));
    pgtk_style_context_save(context);

    pgtk_style_context_set_state(context, state);
    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_RADIO);

    pgtk_render_option(context, cr, 0, 0, theme->indicator_size, theme->indicator_size);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_checkbox(button_theme_t *theme, cairo_t *cr, int state_id)
{
    GtkStateFlags state = get_checkbox_state_flags(state_id);
    GtkStyleContext *context;

    assert(theme != NULL);

    context = pgtk_widget_get_style_context(theme->check);
    pgtk_style_context_save(context);

    pgtk_style_context_set_state(context, state);
    pgtk_style_context_add_class(context, GTK_STYLE_CLASS_CHECK);

    pgtk_render_check(context, cr, 0, 0, theme->indicator_size, theme->indicator_size);

    pgtk_style_context_restore(context);

    return S_OK;
}

static HRESULT draw_background(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                               int width, int height)
{
    button_theme_t *button_theme = impl_from_uxgtk_theme_t(theme);

    switch (part_id)
    {
        case BP_PUSHBUTTON:
            return draw_button(button_theme, cr, state_id, width, height);

        case BP_RADIOBUTTON:
            return draw_radio(button_theme, cr, state_id);

        case BP_CHECKBOX:
            return draw_checkbox(button_theme, cr, state_id);

        case BP_GROUPBOX:
            /* GNOME applications don't draw a group box. Return some error code
             * to avoid useless painting operations. */
            return E_ABORT;
    }

    FIXME("Unsupported button part %d.\n", part_id);
    return E_NOTIMPL;
}

static HRESULT get_part_size(uxgtk_theme_t *theme, int part_id, int state_id,
                             RECT *rect, SIZE *size)
{
    button_theme_t *button_theme = impl_from_uxgtk_theme_t(theme);

    assert(theme != NULL);
    assert(size != NULL);

    switch (part_id)
    {
        case BP_CHECKBOX:
        case BP_RADIOBUTTON:
            size->cx = button_theme->indicator_size;
            size->cy = button_theme->indicator_size;
            return S_OK;
    }

    FIXME("Unsupported button part %d.\n", part_id);
    return E_NOTIMPL;
}

static BOOL is_part_defined(int part_id, int state_id)
{
    if (part_id == BP_CHECKBOX)
        return (state_id < CBS_IMPLICITNORMAL);

    return (part_id > 0 && part_id < BP_COMMANDLINK);
}

static const uxgtk_theme_vtable_t button_vtable =
{
    get_color,
    draw_background,
    get_part_size,
    is_part_defined
};

uxgtk_theme_t *uxgtk_button_theme_create(void)
{
    button_theme_t *theme;

    TRACE("()\n");

    theme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*theme));
    if (!theme) return NULL;

    uxgtk_theme_init(&theme->base, &button_vtable);

    /* Other widgets will be created on demand */
    theme->check = pgtk_check_button_new();

    pgtk_container_add((GtkContainer *)theme->base.layout, theme->check);

    /* Used for both check- and radiobuttons */
    pgtk_widget_style_get(theme->check, "indicator-size", &theme->indicator_size, NULL);

    TRACE("-GtkCheckButton-indicator-size: %d\n", theme->indicator_size);

    return &theme->base;
}

#endif /* HAVE_GTK_GTK_H */
