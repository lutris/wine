/*
 * GTK uxtheme implementation
 *
 * Copyright (C) 2015 Ivan Akulinchev
 * Copyright (C) 2015 Michael Müller
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

#ifndef UXTHEMEGTK_H
#define UXTHEMEGTK_H

#include "windef.h"

#define GDK_DISABLE_DEPRECATION_WARNINGS
#include <gtk/gtk.h>

typedef struct _uxgtk_theme uxgtk_theme_t;
typedef struct _uxgtk_theme_vtable uxgtk_theme_vtable_t;

struct _uxgtk_theme_vtable
{
    HRESULT (*get_color)(uxgtk_theme_t *theme, int part_id, int state_id,
                         int prop_id, GdkRGBA *rgba);
    HRESULT (*draw_background)(uxgtk_theme_t *theme, cairo_t *cr, int part_id, int state_id,
                              int width, int height);
    HRESULT (*get_part_size)(uxgtk_theme_t *theme, int part_id, int state_id,
                             RECT *rect, SIZE *size);
    BOOL (*is_part_defined)(int part_id, int state_id);
};

struct _uxgtk_theme
{
    const uxgtk_theme_vtable_t *vtable;

    GtkWidget *window;
    GtkWidget *layout;
};

#define MAKE_FUNCPTR(f) extern typeof(f) * p##f DECLSPEC_HIDDEN
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

uxgtk_theme_t *uxgtk_button_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_combobox_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_edit_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_header_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_listbox_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_listview_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_menu_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_rebar_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_status_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_tab_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_toolbar_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_trackbar_theme_create(void) DECLSPEC_HIDDEN;
uxgtk_theme_t *uxgtk_window_theme_create(void) DECLSPEC_HIDDEN;

void uxgtk_theme_init(uxgtk_theme_t *theme, const uxgtk_theme_vtable_t *vtable) DECLSPEC_HIDDEN;

#endif /* UXTHEMEGTK_H */
