/*
 * Copyright (c) 2014 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author:
 *      Ikey Doherty <michael.i.doherty@intel.com>
 */

#include "config.h"

#include "gm-sidebar.h"

#include <gtk/gtk.h>

/**
 * SECTION:gtkstack_sidebar
 * @Title: GtkStackSidebar
 * @Short_description: An automatic stack_sidebar widget
 *
 * A GtkStackSidebar enables you to quickly and easily provide a consistent
 * "stack_sidebar" object for your user interface.
 *
 * In order to use a GtkStackSidebar, you simply use a GtkStack to organize
 * your UI flow, and add the stack_sidebar to your stack_sidebar area. You can use
 * gtk_stack_sidebar_set_stack() to connect the #GtkStackSidebar to the #GtkStack.
 *
 * Since: 3.16
 */
 
struct _GtkStackSidebarPrivate
{
  GtkListBox *list;
  GtkStack *stack;
  GHashTable *rows;
  gboolean in_child_changed;
};

G_DEFINE_TYPE_WITH_PRIVATE (GtkStackSidebar, gtk_stack_sidebar, GTK_TYPE_BIN)

enum
{
  PROP_0,
  PROP_STACK,
  N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gtk_stack_sidebar_set_property (GObject    *object,
                          guint       prop_id,
                          const       GValue *value,
                          GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_STACK:
      gtk_stack_sidebar_set_stack (GTK_STACK_SIDEBAR (object), g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_stack_sidebar_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (GTK_STACK_SIDEBAR (object));

  switch (prop_id)
    {
    case PROP_STACK:
      g_value_set_object (value, priv->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
update_header (GtkListBoxRow         *row,
               GtkListBoxRow         *before,
               G_GNUC_UNUSED gpointer userdata)
{
  GtkWidget *ret = NULL;

  if (before && !gtk_list_box_row_get_header (row))
    {
      ret = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
      gtk_list_box_row_set_header (row, ret);
    }
}

static gint
sort_list (GtkListBoxRow *row1,
           GtkListBoxRow *row2,
           gpointer       userdata)
{
  GtkStackSidebar *stack_sidebar = GTK_STACK_SIDEBAR (userdata);
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkWidget *item;
  GtkWidget *widget;
  gint left = 0; gint right = 0;


  if (row1)
    {
      item = gtk_bin_get_child (GTK_BIN (row1));
      widget = g_object_get_data (G_OBJECT (item), "stack-child");
      gtk_container_child_get (GTK_CONTAINER (priv->stack), widget,
                               "position", &left,
                               NULL);
    }

  if (row2)
    {
      item = gtk_bin_get_child (GTK_BIN (row2));
      widget = g_object_get_data (G_OBJECT (item), "stack-child");
      gtk_container_child_get (GTK_CONTAINER (priv->stack), widget,
                               "position", &right,
                               NULL);
    }

  if (left < right)
    return  -1;

  if (left == right)
    return 0;

  return 1;
}

static void
gtk_stack_sidebar_row_selected (G_GNUC_UNUSED GtkListBox   *box,
                          GtkListBoxRow              *row,
                          gpointer                    userdata)
{
  GtkStackSidebar *stack_sidebar = GTK_STACK_SIDEBAR (userdata);
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkWidget *item;
  GtkWidget *widget;

  if (priv->in_child_changed)
    return;

  if (!row)
    return;

  item = gtk_bin_get_child (GTK_BIN (row));
  widget = g_object_get_data (G_OBJECT (item), "stack-child");
  gtk_stack_set_visible_child (priv->stack, widget);
}

static void
gtk_stack_sidebar_init (GtkStackSidebar *stack_sidebar)
{
  GtkStyleContext *style;
  GtkStackSidebarPrivate *priv;
  GtkWidget *sw;

  priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (sw);
  gtk_widget_set_no_show_all (sw, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (stack_sidebar), sw);

  priv->list = GTK_LIST_BOX (gtk_list_box_new ());
  gtk_widget_show (GTK_WIDGET (priv->list));

  gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (priv->list));

  gtk_list_box_set_header_func (priv->list, update_header, stack_sidebar, NULL);
  gtk_list_box_set_sort_func (priv->list, sort_list, stack_sidebar, NULL);

  g_signal_connect (priv->list, "row-selected",
                    G_CALLBACK (gtk_stack_sidebar_row_selected), stack_sidebar);

  style = gtk_widget_get_style_context (GTK_WIDGET (stack_sidebar));
  gtk_style_context_add_class (style, "stack_sidebar");

  priv->rows = g_hash_table_new (NULL, NULL);
}

static void
update_row (GtkStackSidebar *stack_sidebar,
            GtkWidget  *widget,
            GtkWidget  *row)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkWidget *item;
  gchar *title;
  gboolean needs_attention;
  GtkStyleContext *context;

  gtk_container_child_get (GTK_CONTAINER (priv->stack), widget,
                           "title", &title,
                           "needs-attention", &needs_attention,
                           NULL);

  item = gtk_bin_get_child (GTK_BIN (row));
  gtk_label_set_text (GTK_LABEL (item), title);

  gtk_widget_set_visible (row, gtk_widget_get_visible (widget) && title != NULL);

  context = gtk_widget_get_style_context (row);
  if (needs_attention)
     gtk_style_context_add_class (context, GTK_STYLE_CLASS_NEEDS_ATTENTION);
  else
    gtk_style_context_remove_class (context, GTK_STYLE_CLASS_NEEDS_ATTENTION);

  g_free (title);
}

static void
on_position_updated (G_GNUC_UNUSED GtkWidget  *widget,
                     G_GNUC_UNUSED GParamSpec *pspec,
                     GtkStackSidebar               *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  gtk_list_box_invalidate_sort (priv->list);
}

static void
on_child_updated (GtkWidget                *widget,
                  G_GNUC_UNUSED GParamSpec *pspec,
                  GtkStackSidebar               *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkWidget *row;

  row = g_hash_table_lookup (priv->rows, widget);
  update_row (stack_sidebar, widget, row);
}

static void
add_child (GtkWidget  *widget,
           GtkStackSidebar *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkStyleContext *style;
  GtkWidget *item;
  GtkWidget *row;

  /* Check we don't actually already know about this widget */
  if (g_hash_table_lookup (priv->rows, widget))
    return;

  /* Make a pretty item when we add kids */
  item = gtk_label_new ("");
  gtk_widget_set_halign (item, GTK_ALIGN_START);
  gtk_widget_set_valign (item, GTK_ALIGN_CENTER);
  row = gtk_list_box_row_new ();
  gtk_container_add (GTK_CONTAINER (row), item);
  gtk_widget_show (item);

  update_row (stack_sidebar, widget, row);

  /* Fix up styling */
  style = gtk_widget_get_style_context (row);
  gtk_style_context_add_class (style, "stack_sidebar-item");

  /* Hook up for events */
  g_signal_connect (widget, "child-notify::title",
                    G_CALLBACK (on_child_updated), stack_sidebar);
  g_signal_connect (widget, "child-notify::needs-attention",
                    G_CALLBACK (on_child_updated), stack_sidebar);
  g_signal_connect (widget, "notify::visible",
                    G_CALLBACK (on_child_updated), stack_sidebar);
  g_signal_connect (widget, "child-notify::position",
                    G_CALLBACK (on_position_updated), stack_sidebar);

  g_object_set_data (G_OBJECT (item), "stack-child", widget);
  g_hash_table_insert (priv->rows, widget, row);
  gtk_container_add (GTK_CONTAINER (priv->list), row);
}

static void
remove_child (GtkWidget  *widget,
              GtkStackSidebar *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkWidget *row;

  row = g_hash_table_lookup (priv->rows, widget);
  if (!row)
    return;

  g_signal_handlers_disconnect_by_func (widget, on_child_updated, stack_sidebar);
  g_signal_handlers_disconnect_by_func (widget, on_position_updated, stack_sidebar);

  gtk_container_remove (GTK_CONTAINER (priv->list), row);
  g_hash_table_remove (priv->rows, widget);
}

static void
populate_stack_sidebar (GtkStackSidebar *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkWidget *widget, *row;

  gtk_container_foreach (GTK_CONTAINER (priv->stack), (GtkCallback)add_child, stack_sidebar);

  widget = gtk_stack_get_visible_child (priv->stack);
  if (widget)
    {
      row = g_hash_table_lookup (priv->rows, widget);
      gtk_list_box_select_row (priv->list, GTK_LIST_BOX_ROW (row));
    }
}

static void
clear_stack_sidebar (GtkStackSidebar *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  gtk_container_foreach (GTK_CONTAINER (priv->stack), (GtkCallback)remove_child, stack_sidebar);
}

static void
on_child_changed (GtkWidget                *widget,
                  G_GNUC_UNUSED GParamSpec *pspec,
                  GtkStackSidebar               *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);
  GtkWidget *child;
  GtkWidget *row;

  child = gtk_stack_get_visible_child (GTK_STACK (widget));
  row = g_hash_table_lookup (priv->rows, child);
  if (row != NULL)
    {
      priv->in_child_changed = TRUE;
      gtk_list_box_select_row (priv->list, GTK_LIST_BOX_ROW (row));
      priv->in_child_changed = FALSE;
    }
}

static void
on_stack_child_added (G_GNUC_UNUSED GtkContainer *container,
                      GtkWidget                  *widget,
                      GtkStackSidebar                 *stack_sidebar)
{
  add_child (widget, stack_sidebar);
}

static void
on_stack_child_removed (G_GNUC_UNUSED GtkContainer *container,
                        GtkWidget                  *widget,
                        GtkStackSidebar                 *stack_sidebar)
{
  remove_child (widget, stack_sidebar);
}

static void
disconnect_stack_signals (GtkStackSidebar *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_added, stack_sidebar);
  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_removed, stack_sidebar);
  g_signal_handlers_disconnect_by_func (priv->stack, on_child_changed, stack_sidebar);
  g_signal_handlers_disconnect_by_func (priv->stack, disconnect_stack_signals, stack_sidebar);
}

static void
connect_stack_signals (GtkStackSidebar *stack_sidebar)
{
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  g_signal_connect_after (priv->stack, "add",
                          G_CALLBACK (on_stack_child_added), stack_sidebar);
  g_signal_connect_after (priv->stack, "remove",
                          G_CALLBACK (on_stack_child_removed), stack_sidebar);
  g_signal_connect (priv->stack, "notify::visible-child",
                    G_CALLBACK (on_child_changed), stack_sidebar);
  g_signal_connect_swapped (priv->stack, "destroy",
                            G_CALLBACK (disconnect_stack_signals), stack_sidebar);
}

static void
gtk_stack_sidebar_dispose (GObject *object)
{
  GtkStackSidebar *stack_sidebar = GTK_STACK_SIDEBAR (object);

  gtk_stack_sidebar_set_stack (stack_sidebar, NULL);

  G_OBJECT_CLASS (gtk_stack_sidebar_parent_class)->dispose (object);
}

static void
gtk_stack_sidebar_finalize (GObject *object)
{
  GtkStackSidebar *stack_sidebar = GTK_STACK_SIDEBAR (object);
  GtkStackSidebarPrivate *priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  g_hash_table_destroy (priv->rows);

  G_OBJECT_CLASS (gtk_stack_sidebar_parent_class)->finalize (object);
}

static void
gtk_stack_sidebar_class_init (GtkStackSidebarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gtk_stack_sidebar_dispose;
  object_class->finalize = gtk_stack_sidebar_finalize;
  object_class->set_property = gtk_stack_sidebar_set_property;
  object_class->get_property = gtk_stack_sidebar_get_property;

  obj_properties[PROP_STACK] =
      g_param_spec_object ("stack", "Stack",
                           "Associated stack for this GtkStackSidebar",
                           GTK_TYPE_STACK,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}

/**
 * gtk_stack_sidebar_new:
 *
 * Creates a new stack_sidebar.
 *
 * Returns: the new #GtkStackSidebar
 *
 * Since: 3.16
 */
GtkWidget *
gtk_stack_sidebar_new (void)
{
  return GTK_WIDGET (g_object_new (GTK_TYPE_STACK_SIDEBAR, NULL));
}

/**
 * gtk_stack_sidebar_set_stack:
 * @stack_sidebar: a #GtkStackSidebar
 * @stack: a #GtkStack
 *
 * Set the #GtkStack associated with this #GtkStackSidebar.
 *
 * The stack_sidebar widget will automatically update according to the order
 * (packing) and items within the given #GtkStack.
 *
 * Since: 3.16
 */
void
gtk_stack_sidebar_set_stack (GtkStackSidebar *stack_sidebar,
                       GtkStack   *stack)
{
  GtkStackSidebarPrivate *priv;

  g_return_if_fail (GTK_IS_STACK_SIDEBAR (stack_sidebar));
  g_return_if_fail (GTK_IS_STACK (stack) || stack == NULL);

  priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  if (priv->stack == stack)
    return;

  if (priv->stack)
    {
      disconnect_stack_signals (stack_sidebar);
      clear_stack_sidebar (stack_sidebar);
      g_clear_object (&priv->stack);
    }
  if (stack)
    {
      priv->stack = g_object_ref (stack);
      populate_stack_sidebar (stack_sidebar);
      connect_stack_signals (stack_sidebar);
    }

  gtk_widget_queue_resize (GTK_WIDGET (stack_sidebar));

  g_object_notify (G_OBJECT (stack_sidebar), "stack");
}

/**
 * gtk_stack_sidebar_get_stack:
 * @stack_sidebar: a #GtkStackSidebar
 *
 * Retrieves the stack.
 * See gtk_stack_sidebar_set_stack().
 *
 * Returns: (transfer full): the associated #GtkStack or
 *     %NULL if none has been set explicitly
 *
 * Since: 3.16
 */
GtkStack *
gtk_stack_sidebar_get_stack (GtkStackSidebar *stack_sidebar)
{
  GtkStackSidebarPrivate *priv;

  g_return_val_if_fail (GTK_IS_STACK_SIDEBAR (stack_sidebar), NULL);

  priv = gtk_stack_sidebar_get_instance_private (stack_sidebar);

  return GTK_STACK (priv->stack);
}