/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball, Josh MacDonald, 
 * Copyright (C) 1997-1998 Jay Painter <jpaint@serv.net><jpaint@gimp.org>  
 *
 * GtkDTree widget for GTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <stdlib.h>
#include "gtkdtree.h"
#include <gtk/gtkbindings.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkdnd.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#define PM_SIZE                    8
#define TAB_SIZE                   (PM_SIZE + 6)
#define CELL_SPACING               1
#define CLIST_OPTIMUM_SIZE         64
#define COLUMN_INSET               3
#define DRAG_WIDTH                 6

#define ROW_TOP_YPIXEL(clist, row) (((clist)->row_height * (row)) + \
				    (((row) + 1) * CELL_SPACING) + \
				    (clist)->voffset)
#define ROW_FROM_YPIXEL(clist, y)  (((y) - (clist)->voffset) / \
                                    ((clist)->row_height + CELL_SPACING))
#define COLUMN_LEFT_XPIXEL(clist, col)  ((clist)->column[(col)].area.x \
                                    + (clist)->hoffset)
#define COLUMN_LEFT(clist, column) ((clist)->column[(column)].area.x)

static GtkType	GTK_TYPE_DTREE_NODE = 0;

static inline gint
COLUMN_FROM_XPIXEL (GtkCList * clist,
		    gint x)
{
  gint i, cx;

  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].visible)
      {
	cx = clist->column[i].area.x + clist->hoffset;

	if (x >= (cx - (COLUMN_INSET + CELL_SPACING)) &&
	    x <= (cx + clist->column[i].area.width + COLUMN_INSET))
	  return i;
      }

  /* no match */
  return -1;
}

#define GTK_CLIST_CLASS_FW(_widget_) GTK_CLIST_CLASS (((GtkObject*) (_widget_))->klass)
#define CLIST_UNFROZEN(clist)     (((GtkCList*) (clist))->freeze_count == 0)
#define CLIST_REFRESH(clist)    G_STMT_START { \
  if (CLIST_UNFROZEN (clist)) \
    GTK_CLIST_CLASS_FW (clist)->refresh ((GtkCList*) (clist)); \
} G_STMT_END


enum {
  ARG_0,
  ARG_N_COLUMNS,
  ARG_TREE_COLUMN,
  ARG_INDENT,
  ARG_SPACING,
  ARG_SHOW_STUB,
  ARG_LINE_STYLE,
  ARG_EXPANDER_STYLE
};


static void gtk_dtree_class_init        (GtkDTreeClass  *klass);
static void gtk_dtree_init              (GtkDTree       *dtree);
static void gtk_dtree_set_arg		(GtkObject      *object,
					 GtkArg         *arg,
					 guint           arg_id);
static void gtk_dtree_get_arg      	(GtkObject      *object,
					 GtkArg         *arg,
					 guint           arg_id);
static void gtk_dtree_realize           (GtkWidget      *widget);
static void gtk_dtree_unrealize         (GtkWidget      *widget);
static gint gtk_dtree_button_press      (GtkWidget      *widget,
					 GdkEventButton *event);
static void dtree_attach_styles         (GtkDTree       *dtree,
					 GtkDTreeNode   *node,
					 gpointer        data);
static void dtree_detach_styles         (GtkDTree       *dtree,
					 GtkDTreeNode   *node, 
					 gpointer        data);
static gint draw_cell_pixmap            (GdkWindow      *window,
					 GdkRectangle   *clip_rectangle,
					 GdkGC          *fg_gc,
					 GdkPixmap      *pixmap,
					 GdkBitmap      *mask,
					 gint            x,
					 gint            y,
					 gint            width,
					 gint            height);
static void get_cell_style              (GtkCList       *clist,
					 GtkCListRow    *clist_row,
					 gint            state,
					 gint            column,
					 GtkStyle      **style,
					 GdkGC         **fg_gc,
					 GdkGC         **bg_gc);
static gint gtk_dtree_draw_expander     (GtkDTree       *dtree,
					 GtkDTreeRow    *dtree_row,
					 GtkStyle       *style,
					 GdkRectangle   *clip_rectangle,
					 gint            x);
static gint gtk_dtree_draw_lines        (GtkDTree       *dtree,
					 GtkDTreeRow    *dtree_row,
					 gint            row,
					 gint            column,
					 gint            state,
					 GdkRectangle   *clip_rectangle,
					 GdkRectangle   *cell_rectangle,
					 GdkRectangle   *crect,
					 GdkRectangle   *area,
					 GtkStyle       *style);
static void draw_row                    (GtkCList       *clist,
					 GdkRectangle   *area,
					 gint            row,
					 GtkCListRow    *clist_row);
static void draw_drag_highlight         (GtkCList        *clist,
					 GtkCListRow     *dest_row,
					 gint             dest_row_number,
					 GtkCListDragPos  drag_pos);
static void tree_draw_node              (GtkDTree      *dtree,
					 GtkDTreeNode  *node);
static void set_cell_contents           (GtkCList      *clist,
					 GtkCListRow   *clist_row,
					 gint           column,
					 GtkCellType    type,
					 const gchar   *text,
					 guint8         spacing,
					 GdkPixmap     *pixmap,
					 GdkBitmap     *mask);
static void set_node_info               (GtkDTree      *dtree,
					 GtkDTreeNode  *node,
					 const gchar   *text,
					 guint8         spacing,
					 GdkPixmap     *pixmap_closed,
					 GdkBitmap     *mask_closed,
					 GdkPixmap     *pixmap_opened,
					 GdkBitmap     *mask_opened,
					 gboolean       is_leaf,
					 gboolean       expanded);
static GtkDTreeRow *row_new             (GtkDTree      *dtree);
static void row_delete                  (GtkDTree      *dtree,
				 	 GtkDTreeRow   *dtree_row);
static void tree_delete                 (GtkDTree      *dtree, 
					 GtkDTreeNode  *node, 
					 gpointer       data);
static void tree_delete_row             (GtkDTree      *dtree, 
					 GtkDTreeNode  *node, 
					 gpointer       data);
static void real_clear                  (GtkCList      *clist);
static void tree_update_level           (GtkDTree      *dtree, 
					 GtkDTreeNode  *node, 
					 gpointer       data);
static void tree_select                 (GtkDTree      *dtree, 
					 GtkDTreeNode  *node, 
					 gpointer       data);
static void tree_unselect               (GtkDTree      *dtree, 
					 GtkDTreeNode  *node, 
				         gpointer       data);
static void real_select_all             (GtkCList      *clist);
static void real_unselect_all           (GtkCList      *clist);
static void tree_expand                 (GtkDTree      *dtree, 
					 GtkDTreeNode  *node,
					 gpointer       data);
static void tree_collapse               (GtkDTree      *dtree, 
					 GtkDTreeNode  *node,
					 gpointer       data);
static void tree_collapse_to_depth      (GtkDTree      *dtree, 
					 GtkDTreeNode  *node, 
					 gint           depth);
static void tree_toggle_expansion       (GtkDTree      *dtree,
					 GtkDTreeNode  *node,
					 gpointer       data);
static void change_focus_row_expansion  (GtkDTree      *dtree,
				         GtkCTreeExpansionType expansion);
static void real_select_row             (GtkCList      *clist,
					 gint           row,
					 gint           column,
					 GdkEvent      *event);
static void real_unselect_row           (GtkCList      *clist,
					 gint           row,
					 gint           column,
					 GdkEvent      *event);
static void real_tree_select            (GtkDTree      *dtree,
					 GtkDTreeNode  *node,
					 gint           column);
static void real_tree_unselect          (GtkDTree      *dtree,
					 GtkDTreeNode  *node,
					 gint           column);
static void real_tree_expand            (GtkDTree      *dtree,
					 GtkDTreeNode  *node);
static void real_tree_collapse          (GtkDTree      *dtree,
					 GtkDTreeNode  *node);
static void real_tree_move              (GtkDTree      *dtree,
					 GtkDTreeNode  *node,
					 GtkDTreeNode  *new_parent, 
					 GtkDTreeNode  *new_sibling);
static void real_row_move               (GtkCList      *clist,
					 gint           source_row,
					 gint           dest_row);
static void gtk_dtree_link              (GtkDTree      *dtree,
					 GtkDTreeNode  *node,
					 GtkDTreeNode  *parent,
					 GtkDTreeNode  *sibling,
					 gboolean       update_focus_row);
static void gtk_dtree_unlink            (GtkDTree      *dtree, 
					 GtkDTreeNode  *node,
					 gboolean       update_focus_row);
static GtkDTreeNode * gtk_dtree_last_visible (GtkDTree     *dtree,
					      GtkDTreeNode *node);
static gboolean dtree_is_hot_spot       (GtkDTree      *dtree, 
					 GtkDTreeNode  *node,
					 gint           row, 
					 gint           x, 
					 gint           y);
static void tree_sort                   (GtkDTree      *dtree,
					 GtkDTreeNode  *node,
					 gpointer       data);
static void fake_unselect_all           (GtkCList      *clist,
					 gint           row);
static GList * selection_find           (GtkCList      *clist,
					 gint           row_number,
					 GList         *row_list_element);
static void resync_selection            (GtkCList      *clist,
					 GdkEvent      *event);
static void real_undo_selection         (GtkCList      *clist);
static void select_row_recursive        (GtkDTree      *dtree, 
					 GtkDTreeNode  *node, 
					 gpointer       data);
static gint real_insert_row             (GtkCList      *clist,
					 gint           row,
					 gchar         *text[]);
static void real_remove_row             (GtkCList      *clist,
					 gint           row);
static void real_sort_list              (GtkCList      *clist);
static void cell_size_request           (GtkCList       *clist,
					 GtkCListRow    *clist_row,
					 gint            column,
					 GtkRequisition *requisition);
static void column_auto_resize          (GtkCList       *clist,
					 GtkCListRow    *clist_row,
					 gint            column,
					 gint            old_width);
static void auto_resize_columns         (GtkCList       *clist);


static gboolean check_drag               (GtkDTree         *dtree,
			                  GtkDTreeNode     *drag_source,
					  GtkDTreeNode     *drag_target,
					  GtkCListDragPos   insert_pos);
static void gtk_dtree_drag_begin         (GtkWidget        *widget,
					  GdkDragContext   *context);
static gint gtk_dtree_drag_motion        (GtkWidget        *widget,
					  GdkDragContext   *context,
					  gint              x,
					  gint              y,
					  guint             time);
static void gtk_dtree_drag_data_received (GtkWidget        *widget,
					  GdkDragContext   *context,
					  gint              x,
					  gint              y,
					  GtkSelectionData *selection_data,
					  guint             info,
					  guint32           time);
static void remove_grab                  (GtkCList         *clist);
static void drag_dest_cell               (GtkCList         *clist,
					  gint              x,
					  gint              y,
					  GtkCListDestInfo *dest_info);


enum
{
  TREE_SELECT_ROW,
  TREE_UNSELECT_ROW,
  TREE_EXPAND,
  TREE_COLLAPSE,
  TREE_MOVE,
  CHANGE_FOCUS_ROW_EXPANSION,
  LAST_SIGNAL
};

static GtkCListClass *parent_class = NULL;
static GtkContainerClass *container_class = NULL;
static guint dtree_signals[LAST_SIGNAL] = {0};


GtkType
gtk_dtree_get_type (void)
{
  static GtkType dtree_type = 0;

  if (!dtree_type)
    {
      static const GtkTypeInfo dtree_info =
      {
	"GtkDTree",
	sizeof (GtkDTree),
	sizeof (GtkDTreeClass),
	(GtkClassInitFunc) gtk_dtree_class_init,
	(GtkObjectInitFunc) gtk_dtree_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      static const GtkTypeInfo node_info =
      {
	"GtkDTreeNode",
	sizeof (GtkDTreeNode),
	/*class_size*/0,
	(GtkClassInitFunc)NULL,
	(GtkObjectInitFunc)NULL,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc)NULL,
      };

      dtree_type = gtk_type_unique (GTK_TYPE_CLIST, &dtree_info);
      
      GTK_TYPE_DTREE_NODE = gtk_type_unique (GTK_TYPE_BOXED, &node_info);
    }

  return dtree_type;
}

static void
gtk_dtree_class_init (GtkDTreeClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkCListClass *clist_class;
  GtkBindingSet *binding_set;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;
  container_class = (GtkContainerClass *) klass;
  clist_class = (GtkCListClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_CLIST);
  container_class = gtk_type_class (GTK_TYPE_CONTAINER);

#if 0
  gtk_object_add_arg_type ("GtkDTree::n_columns",
			   GTK_TYPE_UINT,
			   GTK_ARG_READWRITE | GTK_ARG_CONSTRUCT_ONLY,
			   ARG_N_COLUMNS);
  gtk_object_add_arg_type ("GtkDTree::tree_column",
			   GTK_TYPE_UINT,
			   GTK_ARG_READWRITE | GTK_ARG_CONSTRUCT_ONLY,
			   ARG_TREE_COLUMN);
  gtk_object_add_arg_type ("GtkDTree::indent",
			   GTK_TYPE_UINT,
			   GTK_ARG_READWRITE,
			   ARG_INDENT);
  gtk_object_add_arg_type ("GtkDTree::spacing",
			   GTK_TYPE_UINT,
			   GTK_ARG_READWRITE,
			   ARG_SPACING);
  gtk_object_add_arg_type ("GtkDTree::show_stub",
			   GTK_TYPE_BOOL,
			   GTK_ARG_READWRITE,
			   ARG_SHOW_STUB);
  gtk_object_add_arg_type ("GtkDTree::line_style",
			   GTK_TYPE_DTREE_LINE_STYLE,
			   GTK_ARG_READWRITE,
			   ARG_LINE_STYLE);
  gtk_object_add_arg_type ("GtkDTree::expander_style",
			   GTK_TYPE_DTREE_EXPANDER_STYLE,
			   GTK_ARG_READWRITE,
			   ARG_EXPANDER_STYLE);
#endif
  object_class->set_arg = gtk_dtree_set_arg;
  object_class->get_arg = gtk_dtree_get_arg;

  dtree_signals[TREE_SELECT_ROW] =
    gtk_signal_new ("tree_select_row",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkDTreeClass, tree_select_row),
		    gtk_marshal_NONE__POINTER_INT,
		    GTK_TYPE_NONE, 2, GTK_TYPE_DTREE_NODE, GTK_TYPE_INT);
  dtree_signals[TREE_UNSELECT_ROW] =
    gtk_signal_new ("tree_unselect_row",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkDTreeClass, tree_unselect_row),
		    gtk_marshal_NONE__POINTER_INT,
		    GTK_TYPE_NONE, 2, GTK_TYPE_DTREE_NODE, GTK_TYPE_INT);
  dtree_signals[TREE_EXPAND] =
    gtk_signal_new ("tree_expand",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkDTreeClass, tree_expand),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1, GTK_TYPE_DTREE_NODE);
  dtree_signals[TREE_COLLAPSE] =
    gtk_signal_new ("tree_collapse",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkDTreeClass, tree_collapse),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1, GTK_TYPE_DTREE_NODE);
  dtree_signals[TREE_MOVE] =
    gtk_signal_new ("tree_move",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkDTreeClass, tree_move),
		    gtk_marshal_NONE__POINTER_POINTER_POINTER,
		    GTK_TYPE_NONE, 3, GTK_TYPE_DTREE_NODE,
		    GTK_TYPE_DTREE_NODE, GTK_TYPE_DTREE_NODE);
  dtree_signals[CHANGE_FOCUS_ROW_EXPANSION] =
    gtk_signal_new ("change_focus_row_expansion",
		    GTK_RUN_LAST | GTK_RUN_ACTION,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkDTreeClass,
				       change_focus_row_expansion),
		    gtk_marshal_NONE__ENUM,
		    GTK_TYPE_NONE, 1, GTK_TYPE_CTREE_EXPANSION_TYPE);
  gtk_object_class_add_signals (object_class, dtree_signals, LAST_SIGNAL);

  widget_class->realize = gtk_dtree_realize;
  widget_class->unrealize = gtk_dtree_unrealize;
  widget_class->button_press_event = gtk_dtree_button_press;

  widget_class->drag_begin = gtk_dtree_drag_begin;
  widget_class->drag_motion = gtk_dtree_drag_motion;
  widget_class->drag_data_received = gtk_dtree_drag_data_received;

  clist_class->select_row = real_select_row;
  clist_class->unselect_row = real_unselect_row;
  clist_class->row_move = real_row_move;
  clist_class->undo_selection = real_undo_selection;
  clist_class->resync_selection = resync_selection;
  clist_class->selection_find = selection_find;
  clist_class->click_column = NULL;
  clist_class->draw_row = draw_row;
  clist_class->draw_drag_highlight = draw_drag_highlight;
  clist_class->clear = real_clear;
  clist_class->select_all = real_select_all;
  clist_class->unselect_all = real_unselect_all;
  clist_class->fake_unselect_all = fake_unselect_all;
  clist_class->insert_row = real_insert_row;
  clist_class->remove_row = real_remove_row;
  clist_class->sort_list = real_sort_list;
  clist_class->set_cell_contents = set_cell_contents;
  clist_class->cell_size_request = cell_size_request;

  klass->tree_select_row = real_tree_select;
  klass->tree_unselect_row = real_tree_unselect;
  klass->tree_expand = real_tree_expand;
  klass->tree_collapse = real_tree_collapse;
  klass->tree_move = real_tree_move;
  klass->change_focus_row_expansion = change_focus_row_expansion;

  binding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (binding_set,
				'+', GDK_SHIFT_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_EXPAND);
  gtk_binding_entry_add_signal (binding_set,
				'+', 0,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_EXPAND);
  gtk_binding_entry_add_signal (binding_set,
				'+', GDK_CONTROL_MASK | GDK_SHIFT_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_EXPAND_RECURSIVE);
  gtk_binding_entry_add_signal (binding_set,
				'+', GDK_CONTROL_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_EXPAND_RECURSIVE);
  gtk_binding_entry_add_signal (binding_set,
				GDK_KP_Add, 0,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_EXPAND);
  gtk_binding_entry_add_signal (binding_set,
				GDK_KP_Add, GDK_CONTROL_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM,
				GTK_CTREE_EXPANSION_EXPAND_RECURSIVE);
  gtk_binding_entry_add_signal (binding_set,
				'-', 0,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_COLLAPSE);
  gtk_binding_entry_add_signal (binding_set,
				'-', GDK_CONTROL_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM,
				GTK_CTREE_EXPANSION_COLLAPSE_RECURSIVE);
  gtk_binding_entry_add_signal (binding_set,
				GDK_KP_Subtract, 0,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_COLLAPSE);
  gtk_binding_entry_add_signal (binding_set,
				GDK_KP_Subtract, GDK_CONTROL_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM,
				GTK_CTREE_EXPANSION_COLLAPSE_RECURSIVE);
  gtk_binding_entry_add_signal (binding_set,
				'=', 0,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_TOGGLE);
  gtk_binding_entry_add_signal (binding_set,
				'=', GDK_SHIFT_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_TOGGLE);
  gtk_binding_entry_add_signal (binding_set,
				GDK_KP_Multiply, 0,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM, GTK_CTREE_EXPANSION_TOGGLE);
  gtk_binding_entry_add_signal (binding_set,
				GDK_KP_Multiply, GDK_CONTROL_MASK,
				"change_focus_row_expansion", 1,
				GTK_TYPE_ENUM,
				GTK_CTREE_EXPANSION_TOGGLE_RECURSIVE);
}

static void
gtk_dtree_set_arg (GtkObject      *object,
		   GtkArg         *arg,
		   guint           arg_id)
{
  GtkDTree *dtree;

  dtree = GTK_DTREE (object);

  switch (arg_id)
    {
    case ARG_N_COLUMNS: /* construct-only arg, only set when !GTK_CONSTRUCTED */
      if (dtree->tree_column)
	gtk_dtree_construct (dtree,
			     MAX (1, GTK_VALUE_UINT (*arg)),
			     dtree->tree_column, NULL);
      else
	GTK_CLIST (dtree)->columns = MAX (1, GTK_VALUE_UINT (*arg));
      break;
    case ARG_TREE_COLUMN: /* construct-only arg, only set when !GTK_CONSTRUCTED */
      if (GTK_CLIST (dtree)->columns)
	gtk_dtree_construct (dtree,
			     GTK_CLIST (dtree)->columns,
			     MAX (1, GTK_VALUE_UINT (*arg)),
			     NULL);
      else
	dtree->tree_column = MAX (1, GTK_VALUE_UINT (*arg));
      break;
    case ARG_INDENT:
      gtk_dtree_set_indent (dtree, GTK_VALUE_UINT (*arg));
      break;
    case ARG_SPACING:
      gtk_dtree_set_spacing (dtree, GTK_VALUE_UINT (*arg));
      break;
    case ARG_SHOW_STUB:
      gtk_dtree_set_show_stub (dtree, GTK_VALUE_BOOL (*arg));
      break;
    case ARG_LINE_STYLE:
      gtk_dtree_set_line_style (dtree, GTK_VALUE_ENUM (*arg));
      break;
    case ARG_EXPANDER_STYLE:
      gtk_dtree_set_expander_style (dtree, GTK_VALUE_ENUM (*arg));
      break;
    default:
      break;
    }
}

static void
gtk_dtree_get_arg (GtkObject      *object,
		   GtkArg         *arg,
		   guint           arg_id)
{
  GtkDTree *dtree;

  dtree = GTK_DTREE (object);

  switch (arg_id)
    {
    case ARG_N_COLUMNS:
      GTK_VALUE_UINT (*arg) = GTK_CLIST (dtree)->columns;
      break;
    case ARG_TREE_COLUMN:
      GTK_VALUE_UINT (*arg) = dtree->tree_column;
      break;
    case ARG_INDENT:
      GTK_VALUE_UINT (*arg) = dtree->tree_indent;
      break;
    case ARG_SPACING:
      GTK_VALUE_UINT (*arg) = dtree->tree_spacing;
      break;
    case ARG_SHOW_STUB:
      GTK_VALUE_BOOL (*arg) = dtree->show_stub;
      break;
    case ARG_LINE_STYLE:
      GTK_VALUE_ENUM (*arg) = dtree->line_style;
      break;
    case ARG_EXPANDER_STYLE:
      GTK_VALUE_ENUM (*arg) = dtree->expander_style;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
gtk_dtree_init (GtkDTree *dtree)
{
  GtkCList *clist;

  GTK_CLIST_SET_FLAG (dtree, CLIST_DRAW_DRAG_RECT);
  GTK_CLIST_SET_FLAG (dtree, CLIST_DRAW_DRAG_LINE);

  clist = GTK_CLIST (dtree);

  dtree->tree_indent    = 20;
  dtree->tree_spacing   = 5;
  dtree->tree_column    = 0;
  dtree->line_style     = GTK_DTREE_LINES_SOLID;
  dtree->expander_style = GTK_DTREE_EXPANDER_SQUARE;
  dtree->drag_compare   = NULL;
  dtree->show_stub      = TRUE;

  clist->button_actions[0] |= GTK_BUTTON_EXPANDS;
}

static void
dtree_attach_styles (GtkDTree     *dtree,
		     GtkDTreeNode *node,
		     gpointer      data)
{
  GtkCList *clist;
  gint i;

  clist = GTK_CLIST (dtree);

  if (GTK_DTREE_ROW (node)->row.style)
    GTK_DTREE_ROW (node)->row.style =
      gtk_style_attach (GTK_DTREE_ROW (node)->row.style, clist->clist_window);

  if (GTK_DTREE_ROW (node)->row.fg_set || GTK_DTREE_ROW (node)->row.bg_set)
    {
      GdkColormap *colormap;

      colormap = gtk_widget_get_colormap (GTK_WIDGET (dtree));
      if (GTK_DTREE_ROW (node)->row.fg_set)
	gdk_color_alloc (colormap, &(GTK_DTREE_ROW (node)->row.foreground));
      if (GTK_DTREE_ROW (node)->row.bg_set)
	gdk_color_alloc (colormap, &(GTK_DTREE_ROW (node)->row.background));
    }

  for (i = 0; i < clist->columns; i++)
    if  (GTK_DTREE_ROW (node)->row.cell[i].style)
      GTK_DTREE_ROW (node)->row.cell[i].style =
	gtk_style_attach (GTK_DTREE_ROW (node)->row.cell[i].style,
			  clist->clist_window);
}

static void
dtree_detach_styles (GtkDTree     *dtree,
		     GtkDTreeNode *node,
		     gpointer      data)
{
  GtkCList *clist;
  gint i;

  clist = GTK_CLIST (dtree);

  if (GTK_DTREE_ROW (node)->row.style)
    gtk_style_detach (GTK_DTREE_ROW (node)->row.style);
  for (i = 0; i < clist->columns; i++)
    if  (GTK_DTREE_ROW (node)->row.cell[i].style)
      gtk_style_detach (GTK_DTREE_ROW (node)->row.cell[i].style);
}

static void
gtk_dtree_realize (GtkWidget *widget)
{
  GtkDTree *dtree;
  GtkCList *clist;
  GdkGCValues values;
  GtkDTreeNode *node;
  GtkDTreeNode *child;
  gint i;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DTREE (widget));

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  dtree = GTK_DTREE (widget);
  clist = GTK_CLIST (widget);

  node = GTK_DTREE_NODE (clist->row_list);
  for (i = 0; i < clist->rows; i++)
    {
      if (GTK_DTREE_ROW (node)->children.head && !GTK_DTREE_ROW (node)->expanded)
	for (child = GTK_DTREE_ROW (node)->children.head ; child ;
	     child = GTK_DTREE_ROW (child)->sibling.next)
	  gtk_dtree_pre_recursive (dtree, child, dtree_attach_styles, NULL);
      node = GTK_DTREE_NODE_NEXT (node);
    }

  values.foreground = widget->style->fg[GTK_STATE_NORMAL];
  values.background = widget->style->base[GTK_STATE_NORMAL];
  values.subwindow_mode = GDK_INCLUDE_INFERIORS;
  values.line_style = GDK_LINE_SOLID;
  dtree->lines_gc = gdk_gc_new_with_values (GTK_CLIST(widget)->clist_window, 
					    &values,
					    GDK_GC_FOREGROUND |
					    GDK_GC_BACKGROUND |
					    GDK_GC_SUBWINDOW |
					    GDK_GC_LINE_STYLE);

  if (dtree->line_style == GTK_DTREE_LINES_DOTTED)
    {
      gdk_gc_set_line_attributes (dtree->lines_gc, 1, 
				  GDK_LINE_ON_OFF_DASH, None, None);
      gdk_gc_set_dashes (dtree->lines_gc, 0, "\1\1", 2);
    }
}

static void
gtk_dtree_unrealize (GtkWidget *widget)
{
  GtkDTree *dtree;
  GtkCList *clist;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DTREE (widget));

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);

  dtree = GTK_DTREE (widget);
  clist = GTK_CLIST (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {
      GtkDTreeNode *node;
      GtkDTreeNode *child;
      gint i;

      node = GTK_DTREE_NODE (clist->row_list);
      for (i = 0; i < clist->rows; i++)
	{
	  if (GTK_DTREE_ROW (node)->children.head &&
	      !GTK_DTREE_ROW (node)->expanded)
	    for (child = GTK_DTREE_ROW (node)->children.head ; child ;
		 child = GTK_DTREE_ROW (child)->sibling.next)
	      gtk_dtree_pre_recursive(dtree, child, dtree_detach_styles, NULL);
	  node = GTK_DTREE_NODE_NEXT (node);
	}
    }

  gdk_gc_destroy (dtree->lines_gc);
}

static gint
gtk_dtree_button_press (GtkWidget      *widget,
			GdkEventButton *event)
{
  GtkDTree *dtree;
  GtkCList *clist;
  gint button_actions;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DTREE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dtree = GTK_DTREE (widget);
  clist = GTK_CLIST (widget);

  button_actions = clist->button_actions[event->button - 1];

  if (button_actions == GTK_BUTTON_IGNORED)
    return FALSE;

  if (event->window == clist->clist_window)
    {
      GtkDTreeNode *work;
      gint x;
      gint y;
      gint row;
      gint column;

      x = event->x;
      y = event->y;

      if (!gtk_clist_get_selection_info (clist, x, y, &row, &column))
	return FALSE;

      work = GTK_DTREE_NODE (g_list_nth (clist->row_list, row));
	  
      if (button_actions & GTK_BUTTON_EXPANDS &&
	  (GTK_DTREE_ROW (work)->children.head && !GTK_DTREE_ROW (work)->is_leaf  &&
	   (event->type == GDK_2BUTTON_PRESS ||
	    dtree_is_hot_spot (dtree, work, row, x, y))))
	{
	  if (GTK_DTREE_ROW (work)->expanded)
	    gtk_dtree_collapse (dtree, work);
	  else
	    gtk_dtree_expand (dtree, work);

	  return FALSE;
	}
    }
  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static void
draw_drag_highlight (GtkCList        *clist,
		     GtkCListRow     *dest_row,
		     gint             dest_row_number,
		     GtkCListDragPos  drag_pos)
{
  GtkDTree *dtree;
  GdkPoint points[4];
  gint level;
  gint i;
  gint y = 0;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  dtree = GTK_DTREE (clist);

  level = ((GtkDTreeRow *)(dest_row))->level;

  y = ROW_TOP_YPIXEL (clist, dest_row_number) - 1;

  switch (drag_pos)
    {
    case GTK_CLIST_DRAG_NONE:
      break;
    case GTK_CLIST_DRAG_AFTER:
      y += clist->row_height + 1;
    case GTK_CLIST_DRAG_BEFORE:
      
      if (clist->column[dtree->tree_column].visible)
	switch (clist->column[dtree->tree_column].justification)
	  {
	  case GTK_JUSTIFY_CENTER:
	  case GTK_JUSTIFY_FILL:
	  case GTK_JUSTIFY_LEFT:
	    if (dtree->tree_column > 0)
	      gdk_draw_line (clist->clist_window, clist->xor_gc, 
			     COLUMN_LEFT_XPIXEL(clist, 0), y,
			     COLUMN_LEFT_XPIXEL(clist, dtree->tree_column - 1)+
			     clist->column[dtree->tree_column - 1].area.width,
			     y);

	    gdk_draw_line (clist->clist_window, clist->xor_gc, 
			   COLUMN_LEFT_XPIXEL(clist, dtree->tree_column) + 
			   dtree->tree_indent * level -
			   (dtree->tree_indent - PM_SIZE) / 2, y,
			   GTK_WIDGET (dtree)->allocation.width, y);
	    break;
	  case GTK_JUSTIFY_RIGHT:
	    if (dtree->tree_column < clist->columns - 1)
	      gdk_draw_line (clist->clist_window, clist->xor_gc, 
			     COLUMN_LEFT_XPIXEL(clist, dtree->tree_column + 1),
			     y,
			     COLUMN_LEFT_XPIXEL(clist, clist->columns - 1) +
			     clist->column[clist->columns - 1].area.width, y);
      
	    gdk_draw_line (clist->clist_window, clist->xor_gc, 
			   0, y, COLUMN_LEFT_XPIXEL(clist, dtree->tree_column)
			   + clist->column[dtree->tree_column].area.width -
			   dtree->tree_indent * level +
			   (dtree->tree_indent - PM_SIZE) / 2, y);
	    break;
	  }
      else
	gdk_draw_line (clist->clist_window, clist->xor_gc, 
		       0, y, clist->clist_window_width, y);
      break;
    case GTK_CLIST_DRAG_INTO:
      y = ROW_TOP_YPIXEL (clist, dest_row_number) + clist->row_height;

      if (clist->column[dtree->tree_column].visible)
	switch (clist->column[dtree->tree_column].justification)
	  {
	  case GTK_JUSTIFY_CENTER:
	  case GTK_JUSTIFY_FILL:
	  case GTK_JUSTIFY_LEFT:
	    points[0].x =  COLUMN_LEFT_XPIXEL(clist, dtree->tree_column) + 
	      dtree->tree_indent * level - (dtree->tree_indent - PM_SIZE) / 2;
	    points[0].y = y;
	    points[3].x = points[0].x;
	    points[3].y = y - clist->row_height - 1;
	    points[1].x = clist->clist_window_width - 1;
	    points[1].y = points[0].y;
	    points[2].x = points[1].x;
	    points[2].y = points[3].y;

	    for (i = 0; i < 3; i++)
	      gdk_draw_line (clist->clist_window, clist->xor_gc,
			     points[i].x, points[i].y,
			     points[i+1].x, points[i+1].y);

	    if (dtree->tree_column > 0)
	      {
		points[0].x = COLUMN_LEFT_XPIXEL(clist,
						 dtree->tree_column - 1) +
		  clist->column[dtree->tree_column - 1].area.width ;
		points[0].y = y;
		points[3].x = points[0].x;
		points[3].y = y - clist->row_height - 1;
		points[1].x = 0;
		points[1].y = points[0].y;
		points[2].x = 0;
		points[2].y = points[3].y;

		for (i = 0; i < 3; i++)
		  gdk_draw_line (clist->clist_window, clist->xor_gc,
				 points[i].x, points[i].y, points[i+1].x, 
				 points[i+1].y);
	      }
	    break;
	  case GTK_JUSTIFY_RIGHT:
	    points[0].x =  COLUMN_LEFT_XPIXEL(clist, dtree->tree_column) - 
	      dtree->tree_indent * level + (dtree->tree_indent - PM_SIZE) / 2 +
	      clist->column[dtree->tree_column].area.width;
	    points[0].y = y;
	    points[3].x = points[0].x;
	    points[3].y = y - clist->row_height - 1;
	    points[1].x = 0;
	    points[1].y = points[0].y;
	    points[2].x = 0;
	    points[2].y = points[3].y;

	    for (i = 0; i < 3; i++)
	      gdk_draw_line (clist->clist_window, clist->xor_gc,
			     points[i].x, points[i].y,
			     points[i+1].x, points[i+1].y);

	    if (dtree->tree_column < clist->columns - 1)
	      {
		points[0].x = COLUMN_LEFT_XPIXEL(clist, dtree->tree_column +1);
		points[0].y = y;
		points[3].x = points[0].x;
		points[3].y = y - clist->row_height - 1;
		points[1].x = clist->clist_window_width - 1;
		points[1].y = points[0].y;
		points[2].x = points[1].x;
		points[2].y = points[3].y;

		for (i = 0; i < 3; i++)
		  gdk_draw_line (clist->clist_window, clist->xor_gc,
				 points[i].x, points[i].y,
				 points[i+1].x, points[i+1].y);
	      }
	    break;
	  }
      else
	gdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
			    0, y - clist->row_height,
			    clist->clist_window_width - 1, clist->row_height);
      break;
    }
}

static gint
draw_cell_pixmap (GdkWindow    *window,
		  GdkRectangle *clip_rectangle,
		  GdkGC        *fg_gc,
		  GdkPixmap    *pixmap,
		  GdkBitmap    *mask,
		  gint          x,
		  gint          y,
		  gint          width,
		  gint          height)
{
  gint xsrc = 0;
  gint ysrc = 0;

  if (mask)
    {
      gdk_gc_set_clip_mask (fg_gc, mask);
      gdk_gc_set_clip_origin (fg_gc, x, y);
    }
  if (x < clip_rectangle->x)
    {
      xsrc = clip_rectangle->x - x;
      width -= xsrc;
      x = clip_rectangle->x;
    }
  if (x + width > clip_rectangle->x + clip_rectangle->width)
    width = clip_rectangle->x + clip_rectangle->width - x;

  if (y < clip_rectangle->y)
    {
      ysrc = clip_rectangle->y - y;
      height -= ysrc;
      y = clip_rectangle->y;
    }
  if (y + height > clip_rectangle->y + clip_rectangle->height)
    height = clip_rectangle->y + clip_rectangle->height - y;

  if (width > 0 && height > 0)
    gdk_draw_pixmap (window, fg_gc, pixmap, xsrc, ysrc, x, y, width, height);

  if (mask)
    {
      gdk_gc_set_clip_rectangle (fg_gc, NULL);
      gdk_gc_set_clip_origin (fg_gc, 0, 0);
    }

  return x + MAX (width, 0);
}

static void
get_cell_style (GtkCList     *clist,
		GtkCListRow  *clist_row,
		gint          state,
		gint          column,
		GtkStyle    **style,
		GdkGC       **fg_gc,
		GdkGC       **bg_gc)
{
  gint fg_state;

  if ((state == GTK_STATE_NORMAL) &&
      (GTK_WIDGET (clist)->state == GTK_STATE_INSENSITIVE))
    fg_state = GTK_STATE_INSENSITIVE;
  else
    fg_state = state;

  if (clist_row->cell[column].style)
    {
      if (style)
	*style = clist_row->cell[column].style;
      if (fg_gc)
	*fg_gc = clist_row->cell[column].style->fg_gc[fg_state];
      if (bg_gc) {
	if (state == GTK_STATE_SELECTED)
	  *bg_gc = clist_row->cell[column].style->bg_gc[state];
	else
	  *bg_gc = clist_row->cell[column].style->base_gc[state];
      }
    }
  else if (clist_row->style)
    {
      if (style)
	*style = clist_row->style;
      if (fg_gc)
	*fg_gc = clist_row->style->fg_gc[fg_state];
      if (bg_gc) {
	if (state == GTK_STATE_SELECTED)
	  *bg_gc = clist_row->style->bg_gc[state];
	else
	  *bg_gc = clist_row->style->base_gc[state];
      }
    }
  else
    {
      if (style)
	*style = GTK_WIDGET (clist)->style;
      if (fg_gc)
	*fg_gc = GTK_WIDGET (clist)->style->fg_gc[fg_state];
      if (bg_gc) {
	if (state == GTK_STATE_SELECTED)
	  *bg_gc = GTK_WIDGET (clist)->style->bg_gc[state];
	else
	  *bg_gc = GTK_WIDGET (clist)->style->base_gc[state];
      }

      if (state != GTK_STATE_SELECTED)
	{
	  if (fg_gc && clist_row->fg_set)
	    *fg_gc = clist->fg_gc;
	  if (bg_gc && clist_row->bg_set)
	    *bg_gc = clist->bg_gc;
	}
    }
}

static gint
gtk_dtree_draw_expander (GtkDTree     *dtree,
			 GtkDTreeRow  *dtree_row,
			 GtkStyle     *style,
			 GdkRectangle *clip_rectangle,
			 gint          x)
{
  GtkCList *clist;
  GdkPoint points[3];
  gint justification_factor;
  gint y;

 if (dtree->expander_style == GTK_DTREE_EXPANDER_NONE)
   return x;

  clist = GTK_CLIST (dtree);
  if (clist->column[dtree->tree_column].justification == GTK_JUSTIFY_RIGHT)
    justification_factor = -1;
  else
    justification_factor = 1;
  y = (clip_rectangle->y + (clip_rectangle->height - PM_SIZE) / 2 -
       (clip_rectangle->height + 1) % 2);

  if (!dtree_row->children.head)
    {
      switch (dtree->expander_style)
	{
	case GTK_DTREE_EXPANDER_NONE:
	  return x;
	case GTK_DTREE_EXPANDER_TRIANGLE:
	  return x + justification_factor * (PM_SIZE + 3);
	case GTK_DTREE_EXPANDER_SQUARE:
	case GTK_DTREE_EXPANDER_CIRCULAR:
	  return x + justification_factor * (PM_SIZE + 1);
	}
    }

  gdk_gc_set_clip_rectangle (style->fg_gc[GTK_STATE_NORMAL], clip_rectangle);
  gdk_gc_set_clip_rectangle (style->base_gc[GTK_STATE_NORMAL], clip_rectangle);

  switch (dtree->expander_style)
    {
    case GTK_DTREE_EXPANDER_NONE:
      break;
    case GTK_DTREE_EXPANDER_TRIANGLE:
      if (dtree_row->expanded)
	{
	  points[0].x = x;
	  points[0].y = y + (PM_SIZE + 2) / 6;
	  points[1].x = points[0].x + justification_factor * (PM_SIZE + 2);
	  points[1].y = points[0].y;
	  points[2].x = (points[0].x +
			 justification_factor * (PM_SIZE + 2) / 2);
	  points[2].y = y + 2 * (PM_SIZE + 2) / 3;
	}
      else
	{
	  points[0].x = x + justification_factor * ((PM_SIZE + 2) / 6 + 2);
	  points[0].y = y - 1;
	  points[1].x = points[0].x;
	  points[1].y = points[0].y + (PM_SIZE + 2);
	  points[2].x = (points[0].x +
			 justification_factor * (2 * (PM_SIZE + 2) / 3 - 1));
	  points[2].y = points[0].y + (PM_SIZE + 2) / 2;
	}

      gdk_draw_polygon (clist->clist_window, style->base_gc[GTK_STATE_NORMAL],
			TRUE, points, 3);
      gdk_draw_polygon (clist->clist_window, style->fg_gc[GTK_STATE_NORMAL],
			FALSE, points, 3);

      x += justification_factor * (PM_SIZE + 3);
      break;
    case GTK_DTREE_EXPANDER_SQUARE:
    case GTK_DTREE_EXPANDER_CIRCULAR:
      if (justification_factor == -1)
	x += justification_factor * (PM_SIZE + 1);

      if (dtree->expander_style == GTK_DTREE_EXPANDER_CIRCULAR)
	{
	  gdk_draw_arc (clist->clist_window, style->base_gc[GTK_STATE_NORMAL],
			TRUE, x, y, PM_SIZE, PM_SIZE, 0, 360 * 64);
	  gdk_draw_arc (clist->clist_window, style->fg_gc[GTK_STATE_NORMAL],
			FALSE, x, y, PM_SIZE, PM_SIZE, 0, 360 * 64);
	}
      else
	{
	  gdk_draw_rectangle (clist->clist_window,
			      style->base_gc[GTK_STATE_NORMAL], TRUE,
			      x, y, PM_SIZE, PM_SIZE);
	  gdk_draw_rectangle (clist->clist_window,
			      style->fg_gc[GTK_STATE_NORMAL], FALSE,
			      x, y, PM_SIZE, PM_SIZE);
	}

      gdk_draw_line (clist->clist_window, style->fg_gc[GTK_STATE_NORMAL], 
		     x + 2, y + PM_SIZE / 2, x + PM_SIZE - 2, y + PM_SIZE / 2);

      if (!dtree_row->expanded)
	gdk_draw_line (clist->clist_window, style->fg_gc[GTK_STATE_NORMAL],
		       x + PM_SIZE / 2, y + 2,
		       x + PM_SIZE / 2, y + PM_SIZE - 2);

      if (justification_factor == 1)
	x += justification_factor * (PM_SIZE + 1);
      break;
    }

  gdk_gc_set_clip_rectangle (style->fg_gc[GTK_STATE_NORMAL], NULL);
  gdk_gc_set_clip_rectangle (style->base_gc[GTK_STATE_NORMAL], NULL);

  return x;
}


static gint
gtk_dtree_draw_lines (GtkDTree     *dtree,
		      GtkDTreeRow  *dtree_row,
		      gint          row,
		      gint          column,
		      gint          state,
		      GdkRectangle *clip_rectangle,
		      GdkRectangle *cell_rectangle,
		      GdkRectangle *crect,
		      GdkRectangle *area,
		      GtkStyle     *style)
{
  GtkCList *clist;
  GtkDTreeNode *node;
  GtkDTreeNode *parent;
  GdkRectangle tree_rectangle;
  GdkRectangle tc_rectangle;
  GdkGC *bg_gc;
  gint offset;
  gint offset_x;
  gint offset_y;
  gint xcenter;
  gint ycenter;
  gint next_level;
  gint column_right;
  gint column_left;
  gint justify_right;
  gint justification_factor;
  
  clist = GTK_CLIST (dtree);
  ycenter = clip_rectangle->y + (clip_rectangle->height / 2);
  justify_right = (clist->column[column].justification == GTK_JUSTIFY_RIGHT);

  if (justify_right)
    {
      offset = (clip_rectangle->x + clip_rectangle->width - 1 -
		dtree->tree_indent * (dtree_row->level - 1));
      justification_factor = -1;
    }
  else
    {
      offset = clip_rectangle->x + dtree->tree_indent * (dtree_row->level - 1);
      justification_factor = 1;
    }

  switch (dtree->line_style)
    {
    case GTK_DTREE_LINES_NONE:
      break;
    case GTK_DTREE_LINES_TABBED:
      xcenter = offset + justification_factor * TAB_SIZE;

      column_right = (COLUMN_LEFT_XPIXEL (clist, dtree->tree_column) +
		      clist->column[dtree->tree_column].area.width +
		      COLUMN_INSET);
      column_left = (COLUMN_LEFT_XPIXEL (clist, dtree->tree_column) -
		     COLUMN_INSET - CELL_SPACING);

      if (area)
	{
	  tree_rectangle.y = crect->y;
	  tree_rectangle.height = crect->height;

	  if (justify_right)
	    {
	      tree_rectangle.x = xcenter;
	      tree_rectangle.width = column_right - xcenter;
	    }
	  else
	    {
	      tree_rectangle.x = column_left;
	      tree_rectangle.width = xcenter - column_left;
	    }

	  if (!gdk_rectangle_intersect (area, &tree_rectangle, &tc_rectangle))
	    {
	      offset += justification_factor * 3;
	      break;
	    }
	}

      gdk_gc_set_clip_rectangle (dtree->lines_gc, crect);

      next_level = dtree_row->level;

      if (!dtree_row->sibling.next || (dtree_row->children.head && dtree_row->expanded))
	{
	  node = gtk_dtree_find_node_ptr (dtree, dtree_row);
	  if (GTK_DTREE_NODE_NEXT (node))
	    next_level = GTK_DTREE_ROW (GTK_DTREE_NODE_NEXT (node))->level;
	  else
	    next_level = 0;
	}

      if (dtree->tree_indent > 0)
	{
	  node = dtree_row->parent;
	  while (node)
	    {
	      xcenter -= (justification_factor * dtree->tree_indent);

	      if ((justify_right && xcenter < column_left) ||
		  (!justify_right && xcenter > column_right))
		{
		  node = GTK_DTREE_ROW (node)->parent;
		  continue;
		}

	      tree_rectangle.y = cell_rectangle->y;
	      tree_rectangle.height = cell_rectangle->height;
	      if (justify_right)
		{
		  tree_rectangle.x = MAX (xcenter - dtree->tree_indent + 1,
					  column_left);
		  tree_rectangle.width = MIN (xcenter - column_left,
					      dtree->tree_indent);
		}
	      else
		{
		  tree_rectangle.x = xcenter;
		  tree_rectangle.width = MIN (column_right - xcenter,
					      dtree->tree_indent);
		}

	      if (!area || gdk_rectangle_intersect (area, &tree_rectangle,
						    &tc_rectangle))
		{
		  get_cell_style (clist, &GTK_DTREE_ROW (node)->row,
				  state, column, NULL, NULL, &bg_gc);

		  if (bg_gc == clist->bg_gc)
		    gdk_gc_set_foreground
		      (clist->bg_gc, &GTK_DTREE_ROW (node)->row.background);

		  if (!area)
		    gdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
					tree_rectangle.x,
					tree_rectangle.y,
					tree_rectangle.width,
					tree_rectangle.height);
		  else 
		    gdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
					tc_rectangle.x,
					tc_rectangle.y,
					tc_rectangle.width,
					tc_rectangle.height);
		}
	      if (next_level > GTK_DTREE_ROW (node)->level)
		gdk_draw_line (clist->clist_window, dtree->lines_gc,
			       xcenter, crect->y,
			       xcenter, crect->y + crect->height);
	      else
		{
		  gint width;

		  offset_x = MIN (dtree->tree_indent, 2 * TAB_SIZE);
		  width = offset_x / 2 + offset_x % 2;

		  parent = GTK_DTREE_ROW (node)->parent;

		  tree_rectangle.y = ycenter;
		  tree_rectangle.height = (cell_rectangle->y - ycenter +
					   cell_rectangle->height);

		  if (justify_right)
		    {
		      tree_rectangle.x = MAX(xcenter + 1 - width, column_left);
		      tree_rectangle.width = MIN (xcenter + 1 - column_left,
						  width);
		    }
		  else
		    {
		      tree_rectangle.x = xcenter;
		      tree_rectangle.width = MIN (column_right - xcenter,
						  width);
		    }

		  if (!area ||
		      gdk_rectangle_intersect (area, &tree_rectangle,
					       &tc_rectangle))
		    {
		      if (parent)
			{
			  get_cell_style (clist, &GTK_DTREE_ROW (parent)->row,
					  state, column, NULL, NULL, &bg_gc);
			  if (bg_gc == clist->bg_gc)
			    gdk_gc_set_foreground
			      (clist->bg_gc,
			       &GTK_DTREE_ROW (parent)->row.background);
			}
		      else if (state == GTK_STATE_SELECTED)
			bg_gc = style->base_gc[state];
		      else
			bg_gc = GTK_WIDGET (clist)->style->base_gc[state];

		      if (!area)
			gdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
					    tree_rectangle.x,
					    tree_rectangle.y,
					    tree_rectangle.width,
					    tree_rectangle.height);
		      else
			gdk_draw_rectangle (clist->clist_window,
					    bg_gc, TRUE,
					    tc_rectangle.x,
					    tc_rectangle.y,
					    tc_rectangle.width,
					    tc_rectangle.height);
		    }

		  get_cell_style (clist, &GTK_DTREE_ROW (node)->row,
				  state, column, NULL, NULL, &bg_gc);
		  if (bg_gc == clist->bg_gc)
		    gdk_gc_set_foreground
		      (clist->bg_gc, &GTK_DTREE_ROW (node)->row.background);

		  gdk_gc_set_clip_rectangle (bg_gc, crect);
		  gdk_draw_arc (clist->clist_window, bg_gc, TRUE,
				xcenter - (justify_right * offset_x),
				cell_rectangle->y,
				offset_x, clist->row_height,
				(180 + (justify_right * 90)) * 64, 90 * 64);
		  gdk_gc_set_clip_rectangle (bg_gc, NULL);

		  gdk_draw_line (clist->clist_window, dtree->lines_gc, 
				 xcenter, cell_rectangle->y, xcenter, ycenter);

		  if (justify_right)
		    gdk_draw_arc (clist->clist_window, dtree->lines_gc, FALSE,
				  xcenter - offset_x, cell_rectangle->y,
				  offset_x, clist->row_height,
				  270 * 64, 90 * 64);
		  else
		    gdk_draw_arc (clist->clist_window, dtree->lines_gc, FALSE,
				  xcenter, cell_rectangle->y,
				  offset_x, clist->row_height,
				  180 * 64, 90 * 64);
		}
	      node = GTK_DTREE_ROW (node)->parent;
	    }
	}

      if (state != GTK_STATE_SELECTED)
	{
	  tree_rectangle.y = clip_rectangle->y;
	  tree_rectangle.height = clip_rectangle->height;
	  tree_rectangle.width = COLUMN_INSET + CELL_SPACING +
	    MIN (clist->column[dtree->tree_column].area.width + COLUMN_INSET,
		 TAB_SIZE);

	  if (justify_right)
	    tree_rectangle.x = MAX (xcenter + 1, column_left);
	  else
	    tree_rectangle.x = column_left;

	  if (!area)
	    gdk_draw_rectangle (clist->clist_window,
				GTK_WIDGET
				(dtree)->style->base_gc[GTK_STATE_NORMAL],
				TRUE,
				tree_rectangle.x,
				tree_rectangle.y,
				tree_rectangle.width,
				tree_rectangle.height);
	  else if (gdk_rectangle_intersect (area, &tree_rectangle,
					    &tc_rectangle))
	    gdk_draw_rectangle (clist->clist_window,
				GTK_WIDGET
				(dtree)->style->base_gc[GTK_STATE_NORMAL],
				TRUE,
				tc_rectangle.x,
				tc_rectangle.y,
				tc_rectangle.width,
				tc_rectangle.height);
	}

      xcenter = offset + (justification_factor * dtree->tree_indent / 2);

      get_cell_style (clist, &dtree_row->row, state, column, NULL, NULL,
		      &bg_gc);
      if (bg_gc == clist->bg_gc)
	gdk_gc_set_foreground (clist->bg_gc, &dtree_row->row.background);

      gdk_gc_set_clip_rectangle (bg_gc, crect);
      if (dtree_row->is_leaf)
	{
	  GdkPoint points[6];

	  points[0].x = offset + justification_factor * TAB_SIZE;
	  points[0].y = cell_rectangle->y;

	  points[1].x = points[0].x - justification_factor * 4;
	  points[1].y = points[0].y;

	  points[2].x = points[1].x - justification_factor * 2;
	  points[2].y = points[1].y + 3;

	  points[3].x = points[2].x;
	  points[3].y = points[2].y + clist->row_height - 5;

	  points[4].x = points[3].x + justification_factor * 2;
	  points[4].y = points[3].y + 3;

	  points[5].x = points[4].x + justification_factor * 4;
	  points[5].y = points[4].y;

	  gdk_draw_polygon (clist->clist_window, bg_gc, TRUE, points, 6);
	  gdk_draw_lines (clist->clist_window, dtree->lines_gc, points, 6);
	}
      else 
	{
	  gdk_draw_arc (clist->clist_window, bg_gc, TRUE,
			offset - (justify_right * 2 * TAB_SIZE),
			cell_rectangle->y,
			2 * TAB_SIZE, clist->row_height,
			(90 + (180 * justify_right)) * 64, 180 * 64);
	  gdk_draw_arc (clist->clist_window, dtree->lines_gc, FALSE,
			offset - (justify_right * 2 * TAB_SIZE),
			cell_rectangle->y,
			2 * TAB_SIZE, clist->row_height,
			(90 + (180 * justify_right)) * 64, 180 * 64);
	}
      gdk_gc_set_clip_rectangle (bg_gc, NULL);
      gdk_gc_set_clip_rectangle (dtree->lines_gc, NULL);

      offset += justification_factor * 3;
      break;
    default:
      xcenter = offset + justification_factor * PM_SIZE / 2;

      if (area)
	{
	  tree_rectangle.y = crect->y;
	  tree_rectangle.height = crect->height;

	  if (justify_right)
	    {
	      tree_rectangle.x = xcenter - PM_SIZE / 2 - 2;
	      tree_rectangle.width = (clip_rectangle->x +
				      clip_rectangle->width -tree_rectangle.x);
	    }
	  else
	    {
	      tree_rectangle.x = clip_rectangle->x + PM_SIZE / 2;
	      tree_rectangle.width = (xcenter + PM_SIZE / 2 + 2 -
				      clip_rectangle->x);
	    }

	  if (!gdk_rectangle_intersect (area, &tree_rectangle, &tc_rectangle))
	    break;
	}

      offset_x = 1;
      offset_y = 0;
      if (dtree->line_style == GTK_DTREE_LINES_DOTTED)
	{
	  offset_x += abs((clip_rectangle->x + clist->hoffset) % 2);
	  offset_y  = abs((cell_rectangle->y + clist->voffset) % 2);
	}

      clip_rectangle->y--;
      clip_rectangle->height++;
      gdk_gc_set_clip_rectangle (dtree->lines_gc, clip_rectangle);
      gdk_draw_line (clist->clist_window, dtree->lines_gc,
		     xcenter,
		     (dtree->show_stub || clist->row_list->data != dtree_row) ?
		     cell_rectangle->y + offset_y : ycenter,
		     xcenter,
		     (dtree_row->sibling.next) ? crect->y +crect->height : ycenter);

      gdk_draw_line (clist->clist_window, dtree->lines_gc,
		     xcenter + (justification_factor * offset_x), ycenter,
		     xcenter + (justification_factor * (PM_SIZE / 2 + 2)),
		     ycenter);

      node = dtree_row->parent;
      while (node)
	{
	  xcenter -= (justification_factor * dtree->tree_indent);

	  if (GTK_DTREE_ROW (node)->sibling.next)
	    gdk_draw_line (clist->clist_window, dtree->lines_gc, 
			   xcenter, cell_rectangle->y + offset_y,
			   xcenter, crect->y + crect->height);
	  node = GTK_DTREE_ROW (node)->parent;
	}
      gdk_gc_set_clip_rectangle (dtree->lines_gc, NULL);
      clip_rectangle->y++;
      clip_rectangle->height--;
      break;
    }
  return offset;
}

static void
draw_row (GtkCList     *clist,
	  GdkRectangle *area,
	  gint          row,
	  GtkCListRow  *clist_row)
{
  GtkWidget *widget;
  GtkDTree  *dtree;
  GdkRectangle *rect;
  GdkRectangle *crect;
  GdkRectangle row_rectangle;
  GdkRectangle cell_rectangle; 
  GdkRectangle clip_rectangle;
  GdkRectangle intersect_rectangle;
  gint last_column;
  gint column_left = 0;
  gint column_right = 0;
  gint offset = 0;
  gint state;
  gint i;

  g_return_if_fail (clist != NULL);

  /* bail now if we arn't drawable yet */
  if (!GTK_WIDGET_DRAWABLE (clist) || row < 0 || row >= clist->rows)
    return;

  widget = GTK_WIDGET (clist);
  dtree  = GTK_DTREE  (clist);

  /* if the function is passed the pointer to the row instead of null,
   * it avoids this expensive lookup */
  if (!clist_row)
    clist_row = (g_list_nth (clist->row_list, row))->data;

  /* rectangle of the entire row */
  row_rectangle.x = 0;
  row_rectangle.y = ROW_TOP_YPIXEL (clist, row);
  row_rectangle.width = clist->clist_window_width;
  row_rectangle.height = clist->row_height;

  /* rectangle of the cell spacing above the row */
  cell_rectangle.x = 0;
  cell_rectangle.y = row_rectangle.y - CELL_SPACING;
  cell_rectangle.width = row_rectangle.width;
  cell_rectangle.height = CELL_SPACING;

  /* rectangle used to clip drawing operations, its y and height
   * positions only need to be set once, so we set them once here. 
   * the x and width are set withing the drawing loop below once per
   * column */
  clip_rectangle.y = row_rectangle.y;
  clip_rectangle.height = row_rectangle.height;

  if (clist_row->state == GTK_STATE_NORMAL)
    {
      if (clist_row->fg_set)
	gdk_gc_set_foreground (clist->fg_gc, &clist_row->foreground);
      if (clist_row->bg_set)
	gdk_gc_set_foreground (clist->bg_gc, &clist_row->background);
    }
  
  state = clist_row->state;

  gdk_gc_set_foreground (dtree->lines_gc,
			 &widget->style->fg[clist_row->state]);

  /* draw the cell borders */
  if (area)
    {
      rect = &intersect_rectangle;
      crect = &intersect_rectangle;

      if (gdk_rectangle_intersect (area, &cell_rectangle, crect))
	gdk_draw_rectangle (clist->clist_window,
			    widget->style->base_gc[GTK_STATE_ACTIVE], TRUE,
			    crect->x, crect->y, crect->width, crect->height);
    }
  else
    {
      rect = &clip_rectangle;
      crect = &cell_rectangle;

      gdk_draw_rectangle (clist->clist_window,
			  widget->style->base_gc[GTK_STATE_ACTIVE], TRUE,
			  crect->x, crect->y, crect->width, crect->height);
    }

  /* horizontal black lines */
  if (dtree->line_style == GTK_DTREE_LINES_TABBED)
    { 

      column_right = (COLUMN_LEFT_XPIXEL (clist, dtree->tree_column) +
		      clist->column[dtree->tree_column].area.width +
		      COLUMN_INSET);
      column_left = (COLUMN_LEFT_XPIXEL (clist, dtree->tree_column) -
		     COLUMN_INSET - (dtree->tree_column != 0) * CELL_SPACING);

      switch (clist->column[dtree->tree_column].justification)
	{
	case GTK_JUSTIFY_CENTER:
	case GTK_JUSTIFY_FILL:
	case GTK_JUSTIFY_LEFT:
	  offset = (column_left + dtree->tree_indent *
		    (((GtkDTreeRow *)clist_row)->level - 1));

	  gdk_draw_line (clist->clist_window, dtree->lines_gc, 
			 MIN (offset + TAB_SIZE, column_right),
			 cell_rectangle.y,
			 clist->clist_window_width, cell_rectangle.y);
	  break;
	case GTK_JUSTIFY_RIGHT:
	  offset = (column_right - 1 - dtree->tree_indent *
		    (((GtkDTreeRow *)clist_row)->level - 1));

	  gdk_draw_line (clist->clist_window, dtree->lines_gc,
			 -1, cell_rectangle.y,
			 MAX (offset - TAB_SIZE, column_left),
			 cell_rectangle.y);
	  break;
	}
    }

  /* the last row has to clear its bottom cell spacing too */
  if (clist_row == clist->row_list_end->data)
    {
      cell_rectangle.y += clist->row_height + CELL_SPACING;

      if (!area || gdk_rectangle_intersect (area, &cell_rectangle, crect))
	{
	  gdk_draw_rectangle (clist->clist_window,
			      widget->style->base_gc[GTK_STATE_ACTIVE], TRUE,
			      crect->x, crect->y, crect->width, crect->height);

	  /* horizontal black lines */
	  if (dtree->line_style == GTK_DTREE_LINES_TABBED)
	    { 
	      switch (clist->column[dtree->tree_column].justification)
		{
		case GTK_JUSTIFY_CENTER:
		case GTK_JUSTIFY_FILL:
		case GTK_JUSTIFY_LEFT:
		  gdk_draw_line (clist->clist_window, dtree->lines_gc, 
				 MIN (column_left + TAB_SIZE + COLUMN_INSET +
				      (((GtkDTreeRow *)clist_row)->level > 1) *
				      MIN (dtree->tree_indent / 2, TAB_SIZE),
				      column_right),
				 cell_rectangle.y,
				 clist->clist_window_width, cell_rectangle.y);
		  break;
		case GTK_JUSTIFY_RIGHT:
		  gdk_draw_line (clist->clist_window, dtree->lines_gc, 
				 -1, cell_rectangle.y,
				 MAX (column_right - TAB_SIZE - 1 -
				      COLUMN_INSET -
				      (((GtkDTreeRow *)clist_row)->level > 1) *
				      MIN (dtree->tree_indent / 2, TAB_SIZE),
				      column_left - 1), cell_rectangle.y);
		  break;
		}
	    }
	}
    }	  

  for (last_column = clist->columns - 1;
       last_column >= 0 && !clist->column[last_column].visible; last_column--)
    ;

  /* iterate and draw all the columns (row cells) and draw their contents */
  for (i = 0; i < clist->columns; i++)
    {
      GtkStyle *style;
      GdkGC *fg_gc; 
      GdkGC *bg_gc;

      gint width;
      gint height;
      gint pixmap_width;
      gint string_width;
      gint old_offset;
      gint row_center_offset;

      if (!clist->column[i].visible)
	continue;

      get_cell_style (clist, clist_row, state, i, &style, &fg_gc, &bg_gc);

      /* calculate clipping region */
      clip_rectangle.x = clist->column[i].area.x + clist->hoffset;
      clip_rectangle.width = clist->column[i].area.width;

      cell_rectangle.x = clip_rectangle.x - COLUMN_INSET - CELL_SPACING;
      cell_rectangle.width = (clip_rectangle.width + 2 * COLUMN_INSET +
			      (1 + (i == last_column)) * CELL_SPACING);
      cell_rectangle.y = clip_rectangle.y;
      cell_rectangle.height = clip_rectangle.height;

      string_width = 0;
      pixmap_width = 0;

      if (area && !gdk_rectangle_intersect (area, &cell_rectangle,
					    &intersect_rectangle))
	{
	  if (i != dtree->tree_column)
	    continue;
	}
      else
	{
	  gdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
			      crect->x, crect->y, crect->width, crect->height);

	  /* calculate real width for column justification */
	  switch (clist_row->cell[i].type)
	    {
	    case GTK_CELL_TEXT:
	      width = gdk_string_width
		(style->font, GTK_CELL_TEXT (clist_row->cell[i])->text);
	      break;
	    case GTK_CELL_PIXMAP:
	      gdk_window_get_size
		(GTK_CELL_PIXMAP (clist_row->cell[i])->pixmap, &pixmap_width,
		 &height);
	      width = pixmap_width;
	      break;
	    case GTK_CELL_PIXTEXT:
	      if (GTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		gdk_window_get_size
		  (GTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
		   &pixmap_width, &height);

	      width = pixmap_width;

	      if (GTK_CELL_PIXTEXT (clist_row->cell[i])->text)
		{
		  string_width = gdk_string_width
		    (style->font, GTK_CELL_PIXTEXT (clist_row->cell[i])->text);
		  width += string_width;
		}

	      if (GTK_CELL_PIXTEXT (clist_row->cell[i])->text &&
		  GTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		width +=  GTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;

	      if (i == dtree->tree_column)
		width += (dtree->tree_indent *
			  ((GtkDTreeRow *)clist_row)->level);
	      break;
	    default:
	      continue;
	      break;
	    }

	  switch (clist->column[i].justification)
	    {
	    case GTK_JUSTIFY_LEFT:
	      offset = clip_rectangle.x + clist_row->cell[i].horizontal;
	      break;
	    case GTK_JUSTIFY_RIGHT:
	      offset = (clip_rectangle.x + clist_row->cell[i].horizontal +
			clip_rectangle.width - width);
	      break;
	    case GTK_JUSTIFY_CENTER:
	    case GTK_JUSTIFY_FILL:
	      offset = (clip_rectangle.x + clist_row->cell[i].horizontal +
			(clip_rectangle.width / 2) - (width / 2));
	      break;
	    };

	  if (i != dtree->tree_column)
	    {
	      offset += clist_row->cell[i].horizontal;
	      switch (clist_row->cell[i].type)
		{
		case GTK_CELL_PIXMAP:
		  draw_cell_pixmap
		    (clist->clist_window, &clip_rectangle, fg_gc,
		     GTK_CELL_PIXMAP (clist_row->cell[i])->pixmap,
		     GTK_CELL_PIXMAP (clist_row->cell[i])->mask,
		     offset,
		     clip_rectangle.y + clist_row->cell[i].vertical +
		     (clip_rectangle.height - height) / 2,
		     pixmap_width, height);
		  break;
		case GTK_CELL_PIXTEXT:
		  offset = draw_cell_pixmap
		    (clist->clist_window, &clip_rectangle, fg_gc,
		     GTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
		     GTK_CELL_PIXTEXT (clist_row->cell[i])->mask,
		     offset,
		     clip_rectangle.y + clist_row->cell[i].vertical +
		     (clip_rectangle.height - height) / 2,
		     pixmap_width, height);
		  offset += GTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;
		case GTK_CELL_TEXT:
		  if (style != GTK_WIDGET (clist)->style)
		    row_center_offset = (((clist->row_height -
					   style->font->ascent -
					   style->font->descent - 1) / 2) +
					 1.5 + style->font->ascent);
		  else
		    row_center_offset = clist->row_center_offset;

		  gdk_gc_set_clip_rectangle (fg_gc, &clip_rectangle);
		  gdk_draw_string
		    (clist->clist_window, style->font, fg_gc,
		     offset,
		     row_rectangle.y + row_center_offset +
		     clist_row->cell[i].vertical,
		     (clist_row->cell[i].type == GTK_CELL_PIXTEXT) ?
		     GTK_CELL_PIXTEXT (clist_row->cell[i])->text :
		     GTK_CELL_TEXT (clist_row->cell[i])->text);
		  gdk_gc_set_clip_rectangle (fg_gc, NULL);
		  break;
		default:
		  break;
		}
	      continue;
	    }
	}

      if (bg_gc == clist->bg_gc)
	gdk_gc_set_background (dtree->lines_gc, &clist_row->background);

      /* draw dtree->tree_column */
      cell_rectangle.y -= CELL_SPACING;
      cell_rectangle.height += CELL_SPACING;

      if (area && !gdk_rectangle_intersect (area, &cell_rectangle,
					    &intersect_rectangle))
	continue;

      /* draw lines */
      offset = gtk_dtree_draw_lines (dtree, (GtkDTreeRow *)clist_row, row, i,
				     state, &clip_rectangle, &cell_rectangle,
				     crect, area, style);

      /* draw expander */
      offset = gtk_dtree_draw_expander (dtree, (GtkDTreeRow *)clist_row,
					style, &clip_rectangle, offset);

      if (clist->column[i].justification == GTK_JUSTIFY_RIGHT)
	offset -= dtree->tree_spacing;
      else
	offset += dtree->tree_spacing;

      if (clist->column[i].justification == GTK_JUSTIFY_RIGHT)
	offset -= (pixmap_width + clist_row->cell[i].horizontal);
      else
	offset += clist_row->cell[i].horizontal;

      old_offset = offset;
      offset = draw_cell_pixmap (clist->clist_window, &clip_rectangle, fg_gc,
				 GTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
				 GTK_CELL_PIXTEXT (clist_row->cell[i])->mask,
				 offset, 
				 clip_rectangle.y + clist_row->cell[i].vertical
				 + (clip_rectangle.height - height) / 2,
				 pixmap_width, height);

      if (string_width)
	{ 
	  if (clist->column[i].justification == GTK_JUSTIFY_RIGHT)
	    {
	      offset = (old_offset - string_width);
	      if (GTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		offset -= GTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;
	    }
	  else
	    {
	      if (GTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		offset += GTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;
	    }

	  if (style != GTK_WIDGET (clist)->style)
	    row_center_offset = (((clist->row_height - style->font->ascent -
				   style->font->descent - 1) / 2) +
				 1.5 + style->font->ascent);
	  else
	    row_center_offset = clist->row_center_offset;
	  
	  gdk_gc_set_clip_rectangle (fg_gc, &clip_rectangle);
	  gdk_draw_string (clist->clist_window, style->font, fg_gc, offset,
			   row_rectangle.y + row_center_offset +
			   clist_row->cell[i].vertical,
			   GTK_CELL_PIXTEXT (clist_row->cell[i])->text);
	}
      gdk_gc_set_clip_rectangle (fg_gc, NULL);
    }

  /* draw focus rectangle */
  if (clist->focus_row == row &&
      GTK_WIDGET_CAN_FOCUS (widget) && GTK_WIDGET_HAS_FOCUS (widget))
    {
      if (!area)
	gdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
			    row_rectangle.x, row_rectangle.y,
			    row_rectangle.width - 1, row_rectangle.height - 1);
      else if (gdk_rectangle_intersect (area, &row_rectangle,
					&intersect_rectangle))
	{
	  gdk_gc_set_clip_rectangle (clist->xor_gc, &intersect_rectangle);
	  gdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
			      row_rectangle.x, row_rectangle.y,
			      row_rectangle.width - 1,
			      row_rectangle.height - 1);
	  gdk_gc_set_clip_rectangle (clist->xor_gc, NULL);
	}
    }
}

static void
tree_draw_node (GtkDTree     *dtree, 
	        GtkDTreeNode *node)
{
  GtkCList *clist;
  
  clist = GTK_CLIST (dtree);

  if (CLIST_UNFROZEN (clist) && gtk_dtree_is_viewable (dtree, node))
    {
      GtkDTreeNode *work;
      gint num = 0;
      
      work = GTK_DTREE_NODE (clist->row_list);
      while (work && work != node)
	{
	  work = GTK_DTREE_NODE_NEXT (work);
	  num++;
	}
      if (work && gtk_clist_row_is_visible (clist, num) != GTK_VISIBILITY_NONE)
	GTK_CLIST_CLASS_FW (clist)->draw_row
	  (clist, NULL, num, GTK_CLIST_ROW ((GList *) node));
    }
}

static GtkDTreeNode *
gtk_dtree_last_visible (GtkDTree     *dtree,
			GtkDTreeNode *node)
{
  GtkDTreeNode *tail;
  
  if (!node)
    return NULL;

  tail = GTK_DTREE_ROW (node)->children.tail;

  if (!tail || !GTK_DTREE_ROW (node)->expanded)
    return node;

  return gtk_dtree_last_visible (dtree, tail);
}

static void
gtk_dtree_link (GtkDTree     *dtree,
		GtkDTreeNode *node,
		GtkDTreeNode *parent,
		GtkDTreeNode *sibling,
		gboolean      update_focus_row)
{
  GtkCList *clist;
  GList *list_end;
  GList *list;
  GList *work;
  gboolean visible = FALSE;
  gint rows = 0;
  
  if (sibling)
    g_return_if_fail (GTK_DTREE_ROW (sibling)->parent == parent);
  g_return_if_fail (node != NULL);
  g_return_if_fail (node != sibling);
  g_return_if_fail (node != parent);

  clist = GTK_CLIST (dtree);

  if (update_focus_row && clist->selection_mode == GTK_SELECTION_EXTENDED)
    {
      GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  for (rows = 1, list_end = (GList *)node; list_end->next;
       list_end = list_end->next)
    rows++;

  GTK_DTREE_ROW (node)->parent = parent;
  GTK_DTREE_ROW (node)->sibling.next = sibling;

  if (!parent || (parent && (gtk_dtree_is_viewable (dtree, parent) &&
			     GTK_DTREE_ROW (parent)->expanded)))
    {
      visible = TRUE;
      clist->rows += rows;
    }

  if (sibling)
    {
      GtkDTreeNode *prev = GTK_DTREE_ROW (sibling)->sibling.prev;
      if (prev != NULL)
	  GTK_DTREE_ROW (prev)->sibling.next = node;
      GTK_DTREE_ROW (node)->sibling.prev = prev;
      GTK_DTREE_ROW (sibling)->sibling.prev = node;

      if (sibling == GTK_DTREE_NODE (clist->row_list))
	clist->row_list = (GList *) node;
      if (GTK_DTREE_NODE_PREV (sibling) &&
	  GTK_DTREE_NODE_NEXT (GTK_DTREE_NODE_PREV (sibling)) == sibling)
	{
	  list = (GList *)GTK_DTREE_NODE_PREV (sibling);
	  list->next = (GList *)node;
	}
      
      list = (GList *)node;
      list->prev = (GList *)GTK_DTREE_NODE_PREV (sibling);
      list_end->next = (GList *)sibling;
      list = (GList *)sibling;
      list->prev = list_end;
      if (parent && GTK_DTREE_ROW (parent)->children.head == sibling)
	GTK_DTREE_ROW (parent)->children.head = node;
    }
  else
    {
      GtkDTreeNode **tailp;

      if (parent)
	tailp = &GTK_DTREE_ROW (parent)->children.tail;
      else
	tailp = (GtkDTreeNode **) &GTK_CLIST(dtree)->row_list_end;

      if (*tailp)
	{
	  GTK_DTREE_ROW (*tailp)->sibling.next = node;
	  GTK_DTREE_ROW (node)->sibling.prev = *tailp;

	  /* find last visible child of sibling */
	  work = (GList *)gtk_dtree_last_visible (dtree, *tailp);

	  *tailp = node;
	  
	  list_end->next = work->next;
	  if (work->next)
	    list = work->next->prev = list_end;
	  work->next = (GList *)node;
	  list = (GList *)node;
	  list->prev = work;
	}
      else
	{
	  *tailp = node;
	  if (parent)
	    {
	      GTK_DTREE_ROW (parent)->children.head = node;
	      list = (GList *)node;
	      list->prev = (GList *)parent;
	      if (GTK_DTREE_ROW (parent)->expanded)
		{
		  list_end->next = (GList *)GTK_DTREE_NODE_NEXT (parent);
		  if (GTK_DTREE_NODE_NEXT(parent))
		    {
		      list = (GList *)GTK_DTREE_NODE_NEXT (parent);
		      list->prev = list_end;
		    }
		  list = (GList *)parent;
		  list->next = (GList *)node;
		}
	      else
		list_end->next = NULL;
	    }
	  else
	    {
	      clist->row_list = (GList *)node;
	      list = (GList *)node;
	      list->prev = NULL;
	      list_end->next = NULL;
	    }
	}
    }

  gtk_dtree_pre_recursive (dtree, node, tree_update_level, NULL); 

  if (clist->row_list_end == NULL ||
      clist->row_list_end->next == (GList *)node)
    clist->row_list_end = list_end;

  if (visible && update_focus_row && clist->row_list_end != list_end)
    {
      gint pos;
	  
      pos = g_list_position (clist->row_list, (GList *)node);

      if (pos <= clist->focus_row)
	{
	  clist->focus_row += rows;
	  clist->undo_anchor = clist->focus_row;
	}
    }
}

static void
gtk_dtree_unlink (GtkDTree     *dtree, 
		  GtkDTreeNode *node,
                  gboolean      update_focus_row)
{
  GtkCList *clist;
  gint rows;
  gint level;
  gint visible;
  GtkDTreeNode *work;
  GtkDTreeNode *parent;
  GList *list;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  clist = GTK_CLIST (dtree);
  
  if (update_focus_row && clist->selection_mode == GTK_SELECTION_EXTENDED)
    {
      GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  visible = gtk_dtree_is_viewable (dtree, node);

  /* clist->row_list_end unlinked ? */
  if (visible &&
      (GTK_DTREE_NODE_NEXT (node) == NULL ||
       (GTK_DTREE_ROW (node)->children.head &&
	gtk_dtree_is_ancestor (dtree, node,
			       GTK_DTREE_NODE (clist->row_list_end)))))
    clist->row_list_end = (GList *) (GTK_DTREE_NODE_PREV (node));

  /* update list */
  rows = 0;
  level = GTK_DTREE_ROW (node)->level;
  work = GTK_DTREE_NODE_NEXT (node);
  while (work && GTK_DTREE_ROW (work)->level > level)
    {
      work = GTK_DTREE_NODE_NEXT (work);
      rows++;
    }

  if (visible)
    {
      clist->rows -= (rows + 1);

      if (update_focus_row)
	{
	  gint pos;
	  
	  pos = g_list_position (clist->row_list, (GList *)node);
	  if (pos + rows < clist->focus_row)
	    clist->focus_row -= (rows + 1);
	  else if (pos <= clist->focus_row)
	    {
	      if (!GTK_DTREE_ROW (node)->sibling.next)
		clist->focus_row = MAX (pos - 1, 0);
	      else
		clist->focus_row = pos;
	      
	      clist->focus_row = MIN (clist->focus_row, clist->rows - 1);
	    }
	  clist->undo_anchor = clist->focus_row;
	}
    }

  if (work)
    {
      list = (GList *)GTK_DTREE_NODE_PREV (work);
      list->next = NULL;
      list = (GList *)work;
      list->prev = (GList *)GTK_DTREE_NODE_PREV (node);
    }

  if (GTK_DTREE_NODE_PREV (node) &&
      GTK_DTREE_NODE_NEXT (GTK_DTREE_NODE_PREV (node)) == node)
    {
      list = (GList *)GTK_DTREE_NODE_PREV (node);
      list->next = (GList *)work;
    }

  /* update tree */
  parent = GTK_DTREE_ROW (node)->parent;
  if (parent)
    {
      if (GTK_DTREE_ROW (parent)->children.head == node)
	{
	  GTK_DTREE_ROW (parent)->children.head = GTK_DTREE_ROW (node)->sibling.next;
	  if (!GTK_DTREE_ROW (parent)->children.head)
	    {
	      GTK_DTREE_ROW (parent)->children.tail = NULL;
	      gtk_dtree_collapse (dtree, parent);
	    }
	}
      else
	{
	  GtkDTreeNode *sibling = GTK_DTREE_ROW (node)->sibling.prev;
	  GTK_DTREE_ROW (sibling)->sibling.next = GTK_DTREE_ROW (node)->sibling.next;
	}
    }
  else
    {
    	/* gnbTODO: doesn't appear to maintain clist->row_list_end */
      if (clist->row_list == (GList *)node)
	clist->row_list = (GList *) (GTK_DTREE_ROW (node)->sibling.next);
      else
	{
	  GtkDTreeNode *sibling = GTK_DTREE_ROW (node)->sibling.prev;
	  GTK_DTREE_ROW (sibling)->sibling.next = GTK_DTREE_ROW (node)->sibling.next;
	}
    }
}

static void
real_row_move (GtkCList *clist,
	       gint      source_row,
	       gint      dest_row)
{
  GtkDTree *dtree;
  GtkDTreeNode *node;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  if (GTK_CLIST_AUTO_SORT (clist))
    return;

  if (source_row < 0 || source_row >= clist->rows ||
      dest_row   < 0 || dest_row   >= clist->rows ||
      source_row == dest_row)
    return;

  dtree = GTK_DTREE (clist);
  node = GTK_DTREE_NODE (g_list_nth (clist->row_list, source_row));

  if (source_row < dest_row)
    {
      GtkDTreeNode *work; 

      dest_row++;
      work = GTK_DTREE_ROW (node)->children.head;

      while (work && GTK_DTREE_ROW (work)->level > GTK_DTREE_ROW (node)->level)
	{
	  work = GTK_DTREE_NODE_NEXT (work);
	  dest_row++;
	}

      if (dest_row > clist->rows)
	dest_row = clist->rows;
    }

  if (dest_row < clist->rows)
    {
      GtkDTreeNode *sibling;

      sibling = GTK_DTREE_NODE (g_list_nth (clist->row_list, dest_row));
      gtk_dtree_move (dtree, node, GTK_DTREE_ROW (sibling)->parent, sibling);
    }
  else
    gtk_dtree_move (dtree, node, NULL, NULL);
}

static void
real_tree_move (GtkDTree     *dtree,
		GtkDTreeNode *node,
		GtkDTreeNode *new_parent, 
		GtkDTreeNode *new_sibling)
{
  GtkCList *clist;
  GtkDTreeNode *work;
  gboolean visible = FALSE;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (!new_sibling || 
		    GTK_DTREE_ROW (new_sibling)->parent == new_parent);

  if (new_parent && GTK_DTREE_ROW (new_parent)->is_leaf)
    return;

  /* new_parent != child of child */
  for (work = new_parent; work; work = GTK_DTREE_ROW (work)->parent)
    if (work == node)
      return;

  clist = GTK_CLIST (dtree);

  visible = gtk_dtree_is_viewable (dtree, node);

  if (clist->selection_mode == GTK_SELECTION_EXTENDED)
    {
      GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (GTK_CLIST_AUTO_SORT (clist))
    {
      if (new_parent == GTK_DTREE_ROW (node)->parent)
	return;
      
      if (new_parent)
	new_sibling = GTK_DTREE_ROW (new_parent)->children.head;
      else
	new_sibling = GTK_DTREE_NODE (clist->row_list);

      while (new_sibling && clist->compare
	     (clist, GTK_DTREE_ROW (node), GTK_DTREE_ROW (new_sibling)) > 0)
	new_sibling = GTK_DTREE_ROW (new_sibling)->sibling.next;
    }

  if (new_parent == GTK_DTREE_ROW (node)->parent && 
      new_sibling == GTK_DTREE_ROW (node)->sibling.next)
    return;

  gtk_clist_freeze (clist);

  work = NULL;
  if (gtk_dtree_is_viewable (dtree, node))
    work = GTK_DTREE_NODE (g_list_nth (clist->row_list, clist->focus_row));
      
  gtk_dtree_unlink (dtree, node, FALSE);
  gtk_dtree_link (dtree, node, new_parent, new_sibling, FALSE);
  
  if (work)
    {
      while (work &&  !gtk_dtree_is_viewable (dtree, work))
	work = GTK_DTREE_ROW (work)->parent;
      clist->focus_row = g_list_position (clist->row_list, (GList *)work);
      clist->undo_anchor = clist->focus_row;
    }

  if (clist->column[dtree->tree_column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist) &&
      (visible || gtk_dtree_is_viewable (dtree, node)))
    gtk_clist_set_column_width
      (clist, dtree->tree_column,
       gtk_clist_optimal_column_width (clist, dtree->tree_column));

  gtk_clist_thaw (clist);
}

static void
change_focus_row_expansion (GtkDTree          *dtree,
			    GtkCTreeExpansionType action)
{
  GtkCList *clist;
  GtkDTreeNode *node;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  if (gdk_pointer_is_grabbed () && GTK_WIDGET_HAS_GRAB (dtree))
    return;
  
  if (!(node =
	GTK_DTREE_NODE (g_list_nth (clist->row_list, clist->focus_row))) ||
      GTK_DTREE_ROW (node)->is_leaf || !(GTK_DTREE_ROW (node)->children.head))
    return;

  switch (action)
    {
    case GTK_CTREE_EXPANSION_EXPAND:
      gtk_dtree_expand (dtree, node);
      break;
    case GTK_CTREE_EXPANSION_EXPAND_RECURSIVE:
      gtk_dtree_expand_recursive (dtree, node);
      break;
    case GTK_CTREE_EXPANSION_COLLAPSE:
      gtk_dtree_collapse (dtree, node);
      break;
    case GTK_CTREE_EXPANSION_COLLAPSE_RECURSIVE:
      gtk_dtree_collapse_recursive (dtree, node);
      break;
    case GTK_CTREE_EXPANSION_TOGGLE:
      gtk_dtree_toggle_expansion (dtree, node);
      break;
    case GTK_CTREE_EXPANSION_TOGGLE_RECURSIVE:
      gtk_dtree_toggle_expansion_recursive (dtree, node);
      break;
    }
}

static void 
real_tree_expand (GtkDTree     *dtree,
		  GtkDTreeNode *node)
{
  GtkCList *clist;
  GtkDTreeNode *work;
  GtkRequisition requisition;
  gboolean visible;
  gint level;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (!node || GTK_DTREE_ROW (node)->expanded || GTK_DTREE_ROW (node)->is_leaf)
    return;

  clist = GTK_CLIST (dtree);
  
  GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);

  GTK_DTREE_ROW (node)->expanded = TRUE;
  level = GTK_DTREE_ROW (node)->level;

  visible = gtk_dtree_is_viewable (dtree, node);
  /* get cell width if tree_column is auto resized */
  if (visible && clist->column[dtree->tree_column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    GTK_CLIST_CLASS_FW (clist)->cell_size_request
      (clist, &GTK_DTREE_ROW (node)->row, dtree->tree_column, &requisition);

  /* unref/unset closed pixmap */
  if (GTK_CELL_PIXTEXT 
      (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap)
    {
      gdk_pixmap_unref
	(GTK_CELL_PIXTEXT
	 (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap);
      
      GTK_CELL_PIXTEXT
	(GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap = NULL;
      
      if (GTK_CELL_PIXTEXT 
	  (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask)
	{
	  gdk_pixmap_unref
	    (GTK_CELL_PIXTEXT 
	     (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask);
	  GTK_CELL_PIXTEXT 
	    (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask = NULL;
	}
    }

  /* set/ref opened pixmap */
  if (GTK_DTREE_ROW (node)->pixmap_opened)
    {
      GTK_CELL_PIXTEXT 
	(GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap = 
	gdk_pixmap_ref (GTK_DTREE_ROW (node)->pixmap_opened);

      if (GTK_DTREE_ROW (node)->mask_opened) 
	GTK_CELL_PIXTEXT 
	  (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask = 
	  gdk_pixmap_ref (GTK_DTREE_ROW (node)->mask_opened);
    }


  work = GTK_DTREE_ROW (node)->children.head;
  if (work)
    {
      GList *list = (GList *)work;
      gint *cell_width = NULL;
      gint tmp = 0;
      gint row;
      gint i;
      
      if (visible && !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
	{
	  cell_width = g_new0 (gint, clist->columns);
	  if (clist->column[dtree->tree_column].auto_resize)
	      cell_width[dtree->tree_column] = requisition.width;

	  while (work)
	    {
	      /* search maximum cell widths of auto_resize columns */
	      for (i = 0; i < clist->columns; i++)
		if (clist->column[i].auto_resize)
		  {
		    GTK_CLIST_CLASS_FW (clist)->cell_size_request
		      (clist, &GTK_DTREE_ROW (work)->row, i, &requisition);
		    cell_width[i] = MAX (requisition.width, cell_width[i]);
		  }

	      list = (GList *)work;
	      work = GTK_DTREE_NODE_NEXT (work);
	      tmp++;
	    }
	}
      else
	while (work)
	  {
	    list = (GList *)work;
	    work = GTK_DTREE_NODE_NEXT (work);
	    tmp++;
	  }

      list->next = (GList *)GTK_DTREE_NODE_NEXT (node);

      if (GTK_DTREE_NODE_NEXT (node))
	{
	  GList *tmp_list;

	  tmp_list = (GList *)GTK_DTREE_NODE_NEXT (node);
	  tmp_list->prev = list;
	}
      else
	clist->row_list_end = list;

      list = (GList *)node;
      list->next = (GList *)(GTK_DTREE_ROW (node)->children.head);

      if (visible)
	{
	  /* resize auto_resize columns if needed */
	  for (i = 0; i < clist->columns; i++)
	    if (clist->column[i].auto_resize &&
		cell_width[i] > clist->column[i].width)
	      gtk_clist_set_column_width (clist, i, cell_width[i]);
	  g_free (cell_width);

	  /* update focus_row position */
	  row = g_list_position (clist->row_list, (GList *)node);
	  if (row < clist->focus_row)
	    clist->focus_row += tmp;

	  clist->rows += tmp;
	  CLIST_REFRESH (clist);
	}
    }
  else if (visible && clist->column[dtree->tree_column].auto_resize)
    /* resize tree_column if needed */
    column_auto_resize (clist, &GTK_DTREE_ROW (node)->row, dtree->tree_column,
			requisition.width);
}

static void 
real_tree_collapse (GtkDTree     *dtree,
		    GtkDTreeNode *node)
{
  GtkCList *clist;
  GtkDTreeNode *work;
  GtkRequisition requisition;
  gboolean visible;
  gint level;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (!node || !GTK_DTREE_ROW (node)->expanded ||
      GTK_DTREE_ROW (node)->is_leaf)
    return;

  clist = GTK_CLIST (dtree);

  GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
  
  GTK_DTREE_ROW (node)->expanded = FALSE;
  level = GTK_DTREE_ROW (node)->level;

  visible = gtk_dtree_is_viewable (dtree, node);
  /* get cell width if tree_column is auto resized */
  if (visible && clist->column[dtree->tree_column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    GTK_CLIST_CLASS_FW (clist)->cell_size_request
      (clist, &GTK_DTREE_ROW (node)->row, dtree->tree_column, &requisition);

  /* unref/unset opened pixmap */
  if (GTK_CELL_PIXTEXT 
      (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap)
    {
      gdk_pixmap_unref
	(GTK_CELL_PIXTEXT
	 (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap);
      
      GTK_CELL_PIXTEXT
	(GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap = NULL;
      
      if (GTK_CELL_PIXTEXT 
	  (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask)
	{
	  gdk_pixmap_unref
	    (GTK_CELL_PIXTEXT 
	     (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask);
	  GTK_CELL_PIXTEXT 
	    (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask = NULL;
	}
    }

  /* set/ref closed pixmap */
  if (GTK_DTREE_ROW (node)->pixmap_closed)
    {
      GTK_CELL_PIXTEXT 
	(GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap = 
	gdk_pixmap_ref (GTK_DTREE_ROW (node)->pixmap_closed);

      if (GTK_DTREE_ROW (node)->mask_closed) 
	GTK_CELL_PIXTEXT 
	  (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask = 
	  gdk_pixmap_ref (GTK_DTREE_ROW (node)->mask_closed);
    }

  work = GTK_DTREE_ROW (node)->children.head;
  if (work)
    {
      gint tmp = 0;
      gint row;
      GList *list;

      while (work && GTK_DTREE_ROW (work)->level > level)
	{
	  work = GTK_DTREE_NODE_NEXT (work);
	  tmp++;
	}

      if (work)
	{
	  list = (GList *)node;
	  list->next = (GList *)work;
	  list = (GList *)GTK_DTREE_NODE_PREV (work);
	  list->next = NULL;
	  list = (GList *)work;
	  list->prev = (GList *)node;
	}
      else
	{
	  list = (GList *)node;
	  list->next = NULL;
	  clist->row_list_end = (GList *)node;
	}

      if (visible)
	{
	  /* resize auto_resize columns if needed */
	  auto_resize_columns (clist);

	  row = g_list_position (clist->row_list, (GList *)node);
	  if (row < clist->focus_row)
	    clist->focus_row -= tmp;
	  clist->rows -= tmp;
	  CLIST_REFRESH (clist);
	}
    }
  else if (visible && clist->column[dtree->tree_column].auto_resize &&
	   !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    /* resize tree_column if needed */
    column_auto_resize (clist, &GTK_DTREE_ROW (node)->row, dtree->tree_column,
			requisition.width);
    
}

static void
column_auto_resize (GtkCList    *clist,
		    GtkCListRow *clist_row,
		    gint         column,
		    gint         old_width)
{
  /* resize column if needed for auto_resize */
  GtkRequisition requisition;

  if (!clist->column[column].auto_resize ||
      GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    return;

  if (clist_row)
    GTK_CLIST_CLASS_FW (clist)->cell_size_request (clist, clist_row,
						   column, &requisition);
  else
    requisition.width = 0;

  if (requisition.width > clist->column[column].width)
    gtk_clist_set_column_width (clist, column, requisition.width);
  else if (requisition.width < old_width &&
	   old_width == clist->column[column].width)
    {
      GList *list;
      gint new_width;

      /* run a "gtk_clist_optimal_column_width" but break, if
       * the column doesn't shrink */
      if (GTK_CLIST_SHOW_TITLES (clist) && clist->column[column].button)
	new_width = (clist->column[column].button->requisition.width -
		     (CELL_SPACING + (2 * COLUMN_INSET)));
      else
	new_width = 0;

      for (list = clist->row_list; list; list = list->next)
	{
	  GTK_CLIST_CLASS_FW (clist)->cell_size_request
	    (clist, GTK_CLIST_ROW (list), column, &requisition);
	  new_width = MAX (new_width, requisition.width);
	  if (new_width == clist->column[column].width)
	    break;
	}
      if (new_width < clist->column[column].width)
	gtk_clist_set_column_width (clist, column, new_width);
    }
}

static void
auto_resize_columns (GtkCList *clist)
{
  gint i;

  if (GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    return;

  for (i = 0; i < clist->columns; i++)
    column_auto_resize (clist, NULL, i, clist->column[i].width);
}

static void
cell_size_request (GtkCList       *clist,
		   GtkCListRow    *clist_row,
		   gint            column,
		   GtkRequisition *requisition)
{
  GtkDTree *dtree;
  GtkStyle *style;
  gint width;
  gint height;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));
  g_return_if_fail (requisition != NULL);

  dtree = GTK_DTREE (clist);

  get_cell_style (clist, clist_row, GTK_STATE_NORMAL, column, &style,
		  NULL, NULL);

  switch (clist_row->cell[column].type)
    {
    case GTK_CELL_TEXT:
      requisition->width =
	gdk_string_width (style->font,
			  GTK_CELL_TEXT (clist_row->cell[column])->text);
      requisition->height = style->font->ascent + style->font->descent;
      break;
    case GTK_CELL_PIXTEXT:
      if (GTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap)
	{
	  gdk_window_get_size (GTK_CELL_PIXTEXT
			       (clist_row->cell[column])->pixmap,
			       &width, &height);
	  width += GTK_CELL_PIXTEXT (clist_row->cell[column])->spacing;
	}
      else
	width = height = 0;
	  
      requisition->width = width +
	gdk_string_width (style->font,
			  GTK_CELL_TEXT (clist_row->cell[column])->text);

      requisition->height = MAX (style->font->ascent + style->font->descent,
				 height);
      if (column == dtree->tree_column)
	{
	  requisition->width += (dtree->tree_spacing + dtree->tree_indent *
				 (((GtkDTreeRow *) clist_row)->level - 1));
	  switch (dtree->expander_style)
	    {
	    case GTK_DTREE_EXPANDER_NONE:
	      break;
	    case GTK_DTREE_EXPANDER_TRIANGLE:
	      requisition->width += PM_SIZE + 3;
	      break;
	    case GTK_DTREE_EXPANDER_SQUARE:
	    case GTK_DTREE_EXPANDER_CIRCULAR:
	      requisition->width += PM_SIZE + 1;
	      break;
	    }
	  if (dtree->line_style == GTK_DTREE_LINES_TABBED)
	    requisition->width += 3;
	}
      break;
    case GTK_CELL_PIXMAP:
      gdk_window_get_size (GTK_CELL_PIXMAP (clist_row->cell[column])->pixmap,
			   &width, &height);
      requisition->width = width;
      requisition->height = height;
      break;
    default:
      requisition->width  = 0;
      requisition->height = 0;
      break;
    }

  requisition->width  += clist_row->cell[column].horizontal;
  requisition->height += clist_row->cell[column].vertical;
}

static void
set_cell_contents (GtkCList    *clist,
		   GtkCListRow *clist_row,
		   gint         column,
		   GtkCellType  type,
		   const gchar *text,
		   guint8       spacing,
		   GdkPixmap   *pixmap,
		   GdkBitmap   *mask)
{
  gboolean visible = FALSE;
  GtkDTree *dtree;
  GtkRequisition requisition;
  gchar *old_text = NULL;
  GdkPixmap *old_pixmap = NULL;
  GdkBitmap *old_mask = NULL;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));
  g_return_if_fail (clist_row != NULL);

  dtree = GTK_DTREE (clist);

  if (clist->column[column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      GtkDTreeNode *parent;

      parent = ((GtkDTreeRow *)clist_row)->parent;
      if (!parent || (parent && GTK_DTREE_ROW (parent)->expanded &&
		      gtk_dtree_is_viewable (dtree, parent)))
	{
	  visible = TRUE;
	  GTK_CLIST_CLASS_FW (clist)->cell_size_request (clist, clist_row,
							 column, &requisition);
	}
    }

  switch (clist_row->cell[column].type)
    {
    case GTK_CELL_EMPTY:
      break;
    case GTK_CELL_TEXT:
      old_text = GTK_CELL_TEXT (clist_row->cell[column])->text;
      break;
    case GTK_CELL_PIXMAP:
      old_pixmap = GTK_CELL_PIXMAP (clist_row->cell[column])->pixmap;
      old_mask = GTK_CELL_PIXMAP (clist_row->cell[column])->mask;
      break;
    case GTK_CELL_PIXTEXT:
      old_text = GTK_CELL_PIXTEXT (clist_row->cell[column])->text;
      old_pixmap = GTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap;
      old_mask = GTK_CELL_PIXTEXT (clist_row->cell[column])->mask;
      break;
    case GTK_CELL_WIDGET:
      /* unimplimented */
      break;
      
    default:
      break;
    }

  clist_row->cell[column].type = GTK_CELL_EMPTY;
  if (column == dtree->tree_column && type != GTK_CELL_EMPTY)
    type = GTK_CELL_PIXTEXT;

  /* Note that pixmap and mask were already ref'ed by the caller
   */
  switch (type)
    {
    case GTK_CELL_TEXT:
      if (text)
	{
	  clist_row->cell[column].type = GTK_CELL_TEXT;
	  GTK_CELL_TEXT (clist_row->cell[column])->text = g_strdup (text);
	}
      break;
    case GTK_CELL_PIXMAP:
      if (pixmap)
	{
	  clist_row->cell[column].type = GTK_CELL_PIXMAP;
	  GTK_CELL_PIXMAP (clist_row->cell[column])->pixmap = pixmap;
	  /* We set the mask even if it is NULL */
	  GTK_CELL_PIXMAP (clist_row->cell[column])->mask = mask;
	}
      break;
    case GTK_CELL_PIXTEXT:
      if (column == dtree->tree_column)
	{
	  clist_row->cell[column].type = GTK_CELL_PIXTEXT;
	  GTK_CELL_PIXTEXT (clist_row->cell[column])->spacing = spacing;
	  if (text)
	    GTK_CELL_PIXTEXT (clist_row->cell[column])->text = g_strdup (text);
	  else
	    GTK_CELL_PIXTEXT (clist_row->cell[column])->text = NULL;
	  if (pixmap)
	    {
	      GTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap = pixmap;
	      GTK_CELL_PIXTEXT (clist_row->cell[column])->mask = mask;
	    }
	  else
	    {
	      GTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap = NULL;
	      GTK_CELL_PIXTEXT (clist_row->cell[column])->mask = NULL;
	    }
	}
      else if (text && pixmap)
	{
	  clist_row->cell[column].type = GTK_CELL_PIXTEXT;
	  GTK_CELL_PIXTEXT (clist_row->cell[column])->text = g_strdup (text);
	  GTK_CELL_PIXTEXT (clist_row->cell[column])->spacing = spacing;
	  GTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap = pixmap;
	  GTK_CELL_PIXTEXT (clist_row->cell[column])->mask = mask;
	}
      break;
    default:
      break;
    }
  
  if (visible && clist->column[column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    column_auto_resize (clist, clist_row, column, requisition.width);

  if (old_text)
    g_free (old_text);
  if (old_pixmap)
    gdk_pixmap_unref (old_pixmap);
  if (old_mask)
    gdk_pixmap_unref (old_mask);
}

static void 
set_node_info (GtkDTree     *dtree,
	       GtkDTreeNode *node,
	       const gchar  *text,
	       guint8        spacing,
	       GdkPixmap    *pixmap_closed,
	       GdkBitmap    *mask_closed,
	       GdkPixmap    *pixmap_opened,
	       GdkBitmap    *mask_opened,
	       gboolean      is_leaf,
	       gboolean      expanded)
{
  if (GTK_DTREE_ROW (node)->pixmap_opened)
    {
      gdk_pixmap_unref (GTK_DTREE_ROW (node)->pixmap_opened);
      if (GTK_DTREE_ROW (node)->mask_opened) 
	gdk_bitmap_unref (GTK_DTREE_ROW (node)->mask_opened);
    }
  if (GTK_DTREE_ROW (node)->pixmap_closed)
    {
      gdk_pixmap_unref (GTK_DTREE_ROW (node)->pixmap_closed);
      if (GTK_DTREE_ROW (node)->mask_closed) 
	gdk_bitmap_unref (GTK_DTREE_ROW (node)->mask_closed);
    }

  GTK_DTREE_ROW (node)->pixmap_opened = NULL;
  GTK_DTREE_ROW (node)->mask_opened   = NULL;
  GTK_DTREE_ROW (node)->pixmap_closed = NULL;
  GTK_DTREE_ROW (node)->mask_closed   = NULL;

  if (pixmap_closed)
    {
      GTK_DTREE_ROW (node)->pixmap_closed = gdk_pixmap_ref (pixmap_closed);
      if (mask_closed) 
	GTK_DTREE_ROW (node)->mask_closed = gdk_bitmap_ref (mask_closed);
    }
  if (pixmap_opened)
    {
      GTK_DTREE_ROW (node)->pixmap_opened = gdk_pixmap_ref (pixmap_opened);
      if (mask_opened) 
	GTK_DTREE_ROW (node)->mask_opened = gdk_bitmap_ref (mask_opened);
    }

  GTK_DTREE_ROW (node)->is_leaf  = is_leaf;
  GTK_DTREE_ROW (node)->expanded = (is_leaf) ? FALSE : expanded;

  if (GTK_DTREE_ROW (node)->expanded)
    gtk_dtree_node_set_pixtext (dtree, node, dtree->tree_column,
				text, spacing, pixmap_opened, mask_opened);
  else 
    gtk_dtree_node_set_pixtext (dtree, node, dtree->tree_column,
				text, spacing, pixmap_closed, mask_closed);
}

static void
tree_delete (GtkDTree     *dtree, 
	     GtkDTreeNode *node, 
	     gpointer      data)
{
  tree_unselect (dtree,  node, NULL);
  row_delete (dtree, GTK_DTREE_ROW (node));
  g_list_free_1 ((GList *)node);
}

static void
tree_delete_row (GtkDTree     *dtree, 
		 GtkDTreeNode *node, 
		 gpointer      data)
{
  row_delete (dtree, GTK_DTREE_ROW (node));
  g_list_free_1 ((GList *)node);
}

static void
tree_update_level (GtkDTree     *dtree, 
		   GtkDTreeNode *node, 
		   gpointer      data)
{
  if (!node)
    return;

  if (GTK_DTREE_ROW (node)->parent)
      GTK_DTREE_ROW (node)->level = 
	GTK_DTREE_ROW (GTK_DTREE_ROW (node)->parent)->level + 1;
  else
      GTK_DTREE_ROW (node)->level = 1;
}

static void
tree_select (GtkDTree     *dtree, 
	     GtkDTreeNode *node, 
	     gpointer      data)
{
  if (node && GTK_DTREE_ROW (node)->row.state != GTK_STATE_SELECTED &&
      GTK_DTREE_ROW (node)->row.selectable)
    gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_SELECT_ROW],
		     node, -1);
}

static void
tree_unselect (GtkDTree     *dtree, 
	       GtkDTreeNode *node, 
	       gpointer      data)
{
  if (node && GTK_DTREE_ROW (node)->row.state == GTK_STATE_SELECTED)
    gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_UNSELECT_ROW], 
		     node, -1);
}

static void
tree_expand (GtkDTree     *dtree, 
	     GtkDTreeNode *node, 
	     gpointer      data)
{
  if (node && !GTK_DTREE_ROW (node)->expanded)
    gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_EXPAND], node);
}

static void
tree_collapse (GtkDTree     *dtree, 
	       GtkDTreeNode *node, 
	       gpointer      data)
{
  if (node && GTK_DTREE_ROW (node)->expanded)
    gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_COLLAPSE], node);
}

static void
tree_collapse_to_depth (GtkDTree     *dtree, 
			GtkDTreeNode *node, 
			gint          depth)
{
  if (node && GTK_DTREE_ROW (node)->level == depth)
    gtk_dtree_collapse_recursive (dtree, node);
}

static void
tree_toggle_expansion (GtkDTree     *dtree,
		       GtkDTreeNode *node,
		       gpointer      data)
{
  if (!node)
    return;

  if (GTK_DTREE_ROW (node)->expanded)
    gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_COLLAPSE], node);
  else
    gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_EXPAND], node);
}

static GtkDTreeRow *
row_new (GtkDTree *dtree)
{
  GtkCList *clist;
  GtkDTreeRow *dtree_row;
  int i;

  clist = GTK_CLIST (dtree);
  dtree_row = g_chunk_new (GtkDTreeRow, clist->row_mem_chunk);
  dtree_row->row.cell = g_chunk_new (GtkCell, clist->cell_mem_chunk);

  for (i = 0; i < clist->columns; i++)
    {
      dtree_row->row.cell[i].type = GTK_CELL_EMPTY;
      dtree_row->row.cell[i].vertical = 0;
      dtree_row->row.cell[i].horizontal = 0;
      dtree_row->row.cell[i].style = NULL;
    }

  GTK_CELL_PIXTEXT (dtree_row->row.cell[dtree->tree_column])->text = NULL;

  dtree_row->row.fg_set     = FALSE;
  dtree_row->row.bg_set     = FALSE;
  dtree_row->row.style      = NULL;
  dtree_row->row.selectable = TRUE;
  dtree_row->row.state      = GTK_STATE_NORMAL;
  dtree_row->row.data       = NULL;
  dtree_row->row.destroy    = NULL;

  dtree_row->level         = 0;
  dtree_row->expanded      = FALSE;
  dtree_row->parent        = NULL;
  dtree_row->sibling.next  = NULL;
  dtree_row->sibling.prev  = NULL;
  dtree_row->children.head = NULL;
  dtree_row->children.tail = NULL;
  dtree_row->pixmap_closed = NULL;
  dtree_row->mask_closed   = NULL;
  dtree_row->pixmap_opened = NULL;
  dtree_row->mask_opened   = NULL;
  
  return dtree_row;
}

static void
row_delete (GtkDTree    *dtree,
	    GtkDTreeRow *dtree_row)
{
  GtkCList *clist;
  gint i;

  clist = GTK_CLIST (dtree);

  for (i = 0; i < clist->columns; i++)
    {
      GTK_CLIST_CLASS_FW (clist)->set_cell_contents
	(clist, &(dtree_row->row), i, GTK_CELL_EMPTY, NULL, 0, NULL, NULL);
      if (dtree_row->row.cell[i].style)
	{
	  if (GTK_WIDGET_REALIZED (dtree))
	    gtk_style_detach (dtree_row->row.cell[i].style);
	  gtk_style_unref (dtree_row->row.cell[i].style);
	}
    }

  if (dtree_row->row.style)
    {
      if (GTK_WIDGET_REALIZED (dtree))
	gtk_style_detach (dtree_row->row.style);
      gtk_style_unref (dtree_row->row.style);
    }

  if (dtree_row->pixmap_closed)
    {
      gdk_pixmap_unref (dtree_row->pixmap_closed);
      if (dtree_row->mask_closed)
	gdk_bitmap_unref (dtree_row->mask_closed);
    }

  if (dtree_row->pixmap_opened)
    {
      gdk_pixmap_unref (dtree_row->pixmap_opened);
      if (dtree_row->mask_opened)
	gdk_bitmap_unref (dtree_row->mask_opened);
    }

  if (dtree_row->row.destroy)
    {
      GtkDestroyNotify dnotify = dtree_row->row.destroy;
      gpointer ddata = dtree_row->row.data;

      dtree_row->row.destroy = NULL;
      dtree_row->row.data = NULL;

      dnotify (ddata);
    }

  g_mem_chunk_free (clist->cell_mem_chunk, dtree_row->row.cell);
  g_mem_chunk_free (clist->row_mem_chunk, dtree_row);
}

static void
real_select_row (GtkCList *clist,
		 gint      row,
		 gint      column,
		 GdkEvent *event)
{
  GList *node;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));
  
  if ((node = g_list_nth (clist->row_list, row)) &&
      GTK_DTREE_ROW (node)->row.selectable)
    gtk_signal_emit (GTK_OBJECT (clist), dtree_signals[TREE_SELECT_ROW],
		     node, column);
}

static void
real_unselect_row (GtkCList *clist,
		   gint      row,
		   gint      column,
		   GdkEvent *event)
{
  GList *node;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  if ((node = g_list_nth (clist->row_list, row)))
    gtk_signal_emit (GTK_OBJECT (clist), dtree_signals[TREE_UNSELECT_ROW],
		     node, column);
}

static void
real_tree_select (GtkDTree     *dtree,
		  GtkDTreeNode *node,
		  gint          column)
{
  GtkCList *clist;
  GList *list;
  GtkDTreeNode *sel_row;
  gboolean node_selected;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (!node || GTK_DTREE_ROW (node)->row.state == GTK_STATE_SELECTED ||
      !GTK_DTREE_ROW (node)->row.selectable)
    return;

  clist = GTK_CLIST (dtree);

  switch (clist->selection_mode)
    {
    case GTK_SELECTION_SINGLE:
    case GTK_SELECTION_BROWSE:

      node_selected = FALSE;
      list = clist->selection;

      while (list)
	{
	  sel_row = list->data;
	  list = list->next;
	  
	  if (node == sel_row)
	    node_selected = TRUE;
	  else
	    gtk_signal_emit (GTK_OBJECT (dtree),
			     dtree_signals[TREE_UNSELECT_ROW], sel_row, column);
	}

      if (node_selected)
	return;

    default:
      break;
    }

  GTK_DTREE_ROW (node)->row.state = GTK_STATE_SELECTED;

  if (!clist->selection)
    {
      clist->selection = g_list_append (clist->selection, node);
      clist->selection_end = clist->selection;
    }
  else
    clist->selection_end = g_list_append (clist->selection_end, node)->next;

  tree_draw_node (dtree, node);
}

static void
real_tree_unselect (GtkDTree     *dtree,
		    GtkDTreeNode *node,
		    gint          column)
{
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (!node || GTK_DTREE_ROW (node)->row.state != GTK_STATE_SELECTED)
    return;

  clist = GTK_CLIST (dtree);

  if (clist->selection_end && clist->selection_end->data == node)
    clist->selection_end = clist->selection_end->prev;

  clist->selection = g_list_remove (clist->selection, node);
  
  GTK_DTREE_ROW (node)->row.state = GTK_STATE_NORMAL;

  tree_draw_node (dtree, node);
}

static void
select_row_recursive (GtkDTree     *dtree, 
		      GtkDTreeNode *node, 
		      gpointer      data)
{
  if (!node || GTK_DTREE_ROW (node)->row.state == GTK_STATE_SELECTED ||
      !GTK_DTREE_ROW (node)->row.selectable)
    return;

  GTK_CLIST (dtree)->undo_unselection = 
    g_list_prepend (GTK_CLIST (dtree)->undo_unselection, node);
  gtk_dtree_select (dtree, node);
}

static void
real_select_all (GtkCList *clist)
{
  GtkDTree *dtree;
  GtkDTreeNode *node;
  
  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  dtree = GTK_DTREE (clist);

  switch (clist->selection_mode)
    {
    case GTK_SELECTION_SINGLE:
    case GTK_SELECTION_BROWSE:
      return;

    case GTK_SELECTION_EXTENDED:

      gtk_clist_freeze (clist);

      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
	  
      clist->anchor_state = GTK_STATE_SELECTED;
      clist->anchor = -1;
      clist->drag_pos = -1;
      clist->undo_anchor = clist->focus_row;

      for (node = GTK_DTREE_NODE (clist->row_list); node;
	   node = GTK_DTREE_NODE_NEXT (node))
	gtk_dtree_pre_recursive (dtree, node, select_row_recursive, NULL);

      gtk_clist_thaw (clist);
      break;

    case GTK_SELECTION_MULTIPLE:
      gtk_dtree_select_recursive (dtree, NULL);
      break;

    default:
      /* do nothing */
      break;
    }
}

static void
real_unselect_all (GtkCList *clist)
{
  GtkDTree *dtree;
  GtkDTreeNode *node;
  GList *list;
 
  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));
  
  dtree = GTK_DTREE (clist);

  switch (clist->selection_mode)
    {
    case GTK_SELECTION_BROWSE:
      if (clist->focus_row >= 0)
	{
	  gtk_dtree_select
	    (dtree,
	     GTK_DTREE_NODE (g_list_nth (clist->row_list, clist->focus_row)));
	  return;
	}
      break;

    case GTK_SELECTION_EXTENDED:
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;

      clist->anchor = -1;
      clist->drag_pos = -1;
      clist->undo_anchor = clist->focus_row;
      break;

    default:
      break;
    }

  list = clist->selection;

  while (list)
    {
      node = list->data;
      list = list->next;
      gtk_dtree_unselect (dtree, node);
    }
}

static gboolean
dtree_is_hot_spot (GtkDTree     *dtree, 
		   GtkDTreeNode *node,
		   gint          row, 
		   gint          x, 
		   gint          y)
{
  GtkDTreeRow *tree_row;
  GtkCList *clist;
  GtkCellPixText *cell;
  gint xl;
  gint yu;
  
  g_return_val_if_fail (dtree != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  clist = GTK_CLIST (dtree);

  if (!clist->column[dtree->tree_column].visible ||
      dtree->expander_style == GTK_DTREE_EXPANDER_NONE)
    return FALSE;

  tree_row = GTK_DTREE_ROW (node);

  cell = GTK_CELL_PIXTEXT(tree_row->row.cell[dtree->tree_column]);

  yu = (ROW_TOP_YPIXEL (clist, row) + (clist->row_height - PM_SIZE) / 2 -
	(clist->row_height - 1) % 2);

  if (clist->column[dtree->tree_column].justification == GTK_JUSTIFY_RIGHT)
    xl = (clist->column[dtree->tree_column].area.x + 
	  clist->column[dtree->tree_column].area.width - 1 + clist->hoffset -
	  (tree_row->level - 1) * dtree->tree_indent - PM_SIZE -
	  (dtree->line_style == GTK_DTREE_LINES_TABBED) * 3);
  else
    xl = (clist->column[dtree->tree_column].area.x + clist->hoffset +
	  (tree_row->level - 1) * dtree->tree_indent +
	  (dtree->line_style == GTK_DTREE_LINES_TABBED) * 3);

  return (x >= xl && x <= xl + PM_SIZE && y >= yu && y <= yu + PM_SIZE);
}

/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/


/***********************************************************
 *           Creation, insertion, deletion                 *
 ***********************************************************/

void
gtk_dtree_construct (GtkDTree    *dtree,
		     gint         columns, 
		     gint         tree_column,
		     gchar       *titles[])
{
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (GTK_OBJECT_CONSTRUCTED (dtree) == FALSE);

  clist = GTK_CLIST (dtree);

  clist->row_mem_chunk = g_mem_chunk_new ("dtree row mem chunk",
					  sizeof (GtkDTreeRow),
					  sizeof (GtkDTreeRow)
					  * CLIST_OPTIMUM_SIZE, 
					  G_ALLOC_AND_FREE);

  clist->cell_mem_chunk = g_mem_chunk_new ("dtree cell mem chunk",
					   sizeof (GtkCell) * columns,
					   sizeof (GtkCell) * columns
					   * CLIST_OPTIMUM_SIZE, 
					   G_ALLOC_AND_FREE);

  dtree->tree_column = tree_column;

  gtk_clist_construct (clist, columns, titles);
}

GtkWidget *
gtk_dtree_new_with_titles (gint         columns, 
			   gint         tree_column,
			   gchar       *titles[])
{
  GtkWidget *widget;

  g_return_val_if_fail (columns > 0, NULL);
  g_return_val_if_fail (tree_column >= 0 && tree_column < columns, NULL);

  widget = gtk_type_new (GTK_TYPE_DTREE);
  gtk_dtree_construct (GTK_DTREE (widget), columns, tree_column, titles);

  return widget;
}

GtkWidget *
gtk_dtree_new (gint columns, 
	       gint tree_column)
{
  return gtk_dtree_new_with_titles (columns, tree_column, NULL);
}

static gint
real_insert_row (GtkCList *clist,
		 gint      row,
		 gchar    *text[])
{
  GtkDTreeNode *parent = NULL;
  GtkDTreeNode *sibling;
  GtkDTreeNode *node;

  g_return_val_if_fail (clist != NULL, -1);
  g_return_val_if_fail (GTK_IS_DTREE (clist), -1);

  sibling = GTK_DTREE_NODE (g_list_nth (clist->row_list, row));
  if (sibling)
    parent = GTK_DTREE_ROW (sibling)->parent;

  node = gtk_dtree_insert_node (GTK_DTREE (clist), parent, sibling, text, 5,
				NULL, NULL, NULL, NULL, TRUE, FALSE);

  if (GTK_CLIST_AUTO_SORT (clist) || !sibling)
    return g_list_position (clist->row_list, (GList *) node);
  
  return row;
}

GtkDTreeNode * 
gtk_dtree_insert_node (GtkDTree     *dtree,
		       GtkDTreeNode *parent, 
		       GtkDTreeNode *sibling,
		       gchar        *text[],
		       guint8        spacing,
		       GdkPixmap    *pixmap_closed,
		       GdkBitmap    *mask_closed,
		       GdkPixmap    *pixmap_opened,
		       GdkBitmap    *mask_opened,
		       gboolean      is_leaf,
		       gboolean      expanded)
{
  GtkCList *clist;
  GtkDTreeRow *new_row;
  GtkDTreeNode *node;
  GList *list;
  gint i;

  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);
  if (sibling)
    g_return_val_if_fail (GTK_DTREE_ROW (sibling)->parent == parent, NULL);

  if (parent && GTK_DTREE_ROW (parent)->is_leaf)
    return NULL;

  clist = GTK_CLIST (dtree);

  /* create the row */
  new_row = row_new (dtree);
  list = g_list_alloc ();
  list->data = new_row;
  node = GTK_DTREE_NODE (list);

  if (text)
    for (i = 0; i < clist->columns; i++)
      if (text[i] && i != dtree->tree_column)
	GTK_CLIST_CLASS_FW (clist)->set_cell_contents
	  (clist, &(new_row->row), i, GTK_CELL_TEXT, text[i], 0, NULL, NULL);

  set_node_info (dtree, node, text ?
		 text[dtree->tree_column] : NULL, spacing, pixmap_closed,
		 mask_closed, pixmap_opened, mask_opened, is_leaf, expanded);

  /* sorted insertion */
  if (GTK_CLIST_AUTO_SORT (clist))
    {
      if (parent)
	sibling = GTK_DTREE_ROW (parent)->children.head;
      else
	sibling = GTK_DTREE_NODE (clist->row_list);

      while (sibling && clist->compare
	     (clist, GTK_DTREE_ROW (node), GTK_DTREE_ROW (sibling)) > 0)
	sibling = GTK_DTREE_ROW (sibling)->sibling.next;
    }

  gtk_dtree_link (dtree, node, parent, sibling, TRUE);

  if (text && !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist) &&
      gtk_dtree_is_viewable (dtree, node))
    {
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  column_auto_resize (clist, &(new_row->row), i, 0);
    }

  if (clist->rows == 1)
    {
      clist->focus_row = 0;
      if (clist->selection_mode == GTK_SELECTION_BROWSE)
	gtk_dtree_select (dtree, node);
    }


  CLIST_REFRESH (clist);

  return node;
}

GtkDTreeNode *
gtk_dtree_insert_gnode (GtkDTree          *dtree,
			GtkDTreeNode      *parent,
			GtkDTreeNode      *sibling,
			GNode             *gnode,
			GtkDTreeGNodeFunc  func,
			gpointer           data)
{
  GtkCList *clist;
  GtkDTreeNode *cnode = NULL;
  GtkDTreeNode *child = NULL;
  GtkDTreeNode *new_child;
  GList *list;
  GNode *work;
  guint depth = 1;

  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);
  g_return_val_if_fail (gnode != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);
  if (sibling)
    g_return_val_if_fail (GTK_DTREE_ROW (sibling)->parent == parent, NULL);
  
  clist = GTK_CLIST (dtree);

  if (parent)
    depth = GTK_DTREE_ROW (parent)->level + 1;

  list = g_list_alloc ();
  list->data = row_new (dtree);
  cnode = GTK_DTREE_NODE (list);

  gtk_clist_freeze (clist);

  set_node_info (dtree, cnode, "", 0, NULL, NULL, NULL, NULL, TRUE, FALSE);

  if (!func (dtree, depth, gnode, cnode, data))
    {
      tree_delete_row (dtree, cnode, NULL);
      return NULL;
    }

  if (GTK_CLIST_AUTO_SORT (clist))
    {
      if (parent)
	sibling = GTK_DTREE_ROW (parent)->children.head;
      else
	sibling = GTK_DTREE_NODE (clist->row_list);

      while (sibling && clist->compare
	     (clist, GTK_DTREE_ROW (cnode), GTK_DTREE_ROW (sibling)) > 0)
	sibling = GTK_DTREE_ROW (sibling)->sibling.next;
    }

  gtk_dtree_link (dtree, cnode, parent, sibling, TRUE);

  for (work = g_node_last_child (gnode); work; work = work->prev)
    {
      new_child = gtk_dtree_insert_gnode (dtree, cnode, child,
					  work, func, data);
      if (new_child)
	child = new_child;
    }	
  
  gtk_clist_thaw (clist);

  return cnode;
}

GNode *
gtk_dtree_export_to_gnode (GtkDTree          *dtree,
			   GNode             *parent,
			   GNode             *sibling,
			   GtkDTreeNode      *node,
			   GtkDTreeGNodeFunc  func,
			   gpointer           data)
{
  GtkDTreeNode *work;
  GNode *gnode;
  gint depth;

  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);
  if (sibling)
    {
      g_return_val_if_fail (parent != NULL, NULL);
      g_return_val_if_fail (sibling->parent == parent, NULL);
    }

  gnode = g_node_new (NULL);
  depth = g_node_depth (parent) + 1;
  
  if (!func (dtree, depth, gnode, node, data))
    {
      g_node_destroy (gnode);
      return NULL;
    }

  if (parent)
    g_node_insert_before (parent, sibling, gnode);

  if (!GTK_DTREE_ROW (node)->is_leaf)
    {
      GNode *new_sibling = NULL;

      for (work = GTK_DTREE_ROW (node)->children.head ; work ;
	   work = GTK_DTREE_ROW (work)->sibling.next)
	new_sibling = gtk_dtree_export_to_gnode (dtree, gnode, new_sibling,
						 work, func, data);

      g_node_reverse_children (gnode);
    }

  return gnode;
}
  
static void
real_remove_row (GtkCList *clist,
		 gint      row)
{
  GtkDTreeNode *node;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  node = GTK_DTREE_NODE (g_list_nth (clist->row_list, row));

  if (node)
    gtk_dtree_remove_node (GTK_DTREE (clist), node);
}

void
gtk_dtree_remove_node (GtkDTree     *dtree, 
		       GtkDTreeNode *node)
{
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  gtk_clist_freeze (clist);

  if (node)
    {
      gboolean visible;

      visible = gtk_dtree_is_viewable (dtree, node);
      gtk_dtree_unlink (dtree, node, TRUE);
      gtk_dtree_post_recursive (dtree, node, GTK_DTREE_FUNC (tree_delete),
				NULL);
      if (clist->selection_mode == GTK_SELECTION_BROWSE && !clist->selection &&
	  clist->focus_row >= 0)
	gtk_clist_select_row (clist, clist->focus_row, -1);

      auto_resize_columns (clist);
    }
  else
    gtk_clist_clear (clist);

  gtk_clist_thaw (clist);
}

static void
real_clear (GtkCList *clist)
{
  GtkDTree *dtree;
  GtkDTreeNode *work;
  GtkDTreeNode *ptr;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  dtree = GTK_DTREE (clist);

  /* remove all rows */
  work = GTK_DTREE_NODE (clist->row_list);
  clist->row_list = NULL;
  clist->row_list_end = NULL;

  GTK_CLIST_SET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  while (work)
    {
      ptr = work;
      work = GTK_DTREE_ROW (work)->sibling.next;
      gtk_dtree_post_recursive (dtree, ptr, GTK_DTREE_FUNC (tree_delete_row), 
				NULL);
    }
  GTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);

  parent_class->clear (clist);
}


/***********************************************************
 *  Generic recursive functions, querying / finding tree   *
 *  information                                            *
 ***********************************************************/


void
gtk_dtree_post_recursive (GtkDTree     *dtree, 
			  GtkDTreeNode *node,
			  GtkDTreeFunc  func,
			  gpointer      data)
{
  GtkDTreeNode *work;
  GtkDTreeNode *tmp;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (func != NULL);

  if (node)
    work = GTK_DTREE_ROW (node)->children.head;
  else
    work = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  while (work)
    {
      tmp = GTK_DTREE_ROW (work)->sibling.next;
      gtk_dtree_post_recursive (dtree, work, func, data);
      work = tmp;
    }

  if (node)
    func (dtree, node, data);
}

void
gtk_dtree_post_recursive_to_depth (GtkDTree     *dtree, 
				   GtkDTreeNode *node,
				   gint          depth,
				   GtkDTreeFunc  func,
				   gpointer      data)
{
  GtkDTreeNode *work;
  GtkDTreeNode *tmp;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (func != NULL);

  if (depth < 0)
    {
      gtk_dtree_post_recursive (dtree, node, func, data);
      return;
    }

  if (node)
    work = GTK_DTREE_ROW (node)->children.head;
  else
    work = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  if (work && GTK_DTREE_ROW (work)->level <= depth)
    {
      while (work)
	{
	  tmp = GTK_DTREE_ROW (work)->sibling.next;
	  gtk_dtree_post_recursive_to_depth (dtree, work, depth, func, data);
	  work = tmp;
	}
    }

  if (node && GTK_DTREE_ROW (node)->level <= depth)
    func (dtree, node, data);
}

void
gtk_dtree_pre_recursive (GtkDTree     *dtree, 
			 GtkDTreeNode *node,
			 GtkDTreeFunc  func,
			 gpointer      data)
{
  GtkDTreeNode *work;
  GtkDTreeNode *tmp;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (func != NULL);

  if (node)
    {
      work = GTK_DTREE_ROW (node)->children.head;
      func (dtree, node, data);
    }
  else
    work = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  while (work)
    {
      tmp = GTK_DTREE_ROW (work)->sibling.next;
      gtk_dtree_pre_recursive (dtree, work, func, data);
      work = tmp;
    }
}

void
gtk_dtree_pre_recursive_to_depth (GtkDTree     *dtree, 
				  GtkDTreeNode *node,
				  gint          depth, 
				  GtkDTreeFunc  func,
				  gpointer      data)
{
  GtkDTreeNode *work;
  GtkDTreeNode *tmp;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (func != NULL);

  if (depth < 0)
    {
      gtk_dtree_pre_recursive (dtree, node, func, data);
      return;
    }

  if (node)
    {
      work = GTK_DTREE_ROW (node)->children.head;
      if (GTK_DTREE_ROW (node)->level <= depth)
	func (dtree, node, data);
    }
  else
    work = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  if (work && GTK_DTREE_ROW (work)->level <= depth)
    {
      while (work)
	{
	  tmp = GTK_DTREE_ROW (work)->sibling.next;
	  gtk_dtree_pre_recursive_to_depth (dtree, work, depth, func, data);
	  work = tmp;
	}
    }
}

gboolean
gtk_dtree_is_viewable (GtkDTree     *dtree, 
		       GtkDTreeNode *node)
{ 
  GtkDTreeRow *work;

  g_return_val_if_fail (dtree != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  work = GTK_DTREE_ROW (node);

  while (work->parent && GTK_DTREE_ROW (work->parent)->expanded)
    work = GTK_DTREE_ROW (work->parent);

  if (!work->parent)
    return TRUE;

  return FALSE;
}

GtkDTreeNode * 
gtk_dtree_last (GtkDTree     *dtree,
		GtkDTreeNode *node)
{
  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);

  if (!node) 
    return NULL;

  if (GTK_DTREE_ROW (node)->parent)
    node = GTK_DTREE_ROW (GTK_DTREE_ROW (node)->parent)->children.tail;
  else
    node = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list_end);
  
  if (GTK_DTREE_ROW (node)->children.head)
    return gtk_dtree_last (dtree, GTK_DTREE_ROW (node)->children.head);
  
  return node;
}

GtkDTreeNode *
gtk_dtree_find_node_ptr (GtkDTree    *dtree,
			 GtkDTreeRow *dtree_row)
{
  GtkDTreeNode *node;
  
  g_return_val_if_fail (dtree != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), FALSE);
  g_return_val_if_fail (dtree_row != NULL, FALSE);
  
  if (dtree_row->parent)
    node = GTK_DTREE_ROW(dtree_row->parent)->children.head;
  else
    node = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  while (GTK_DTREE_ROW (node) != dtree_row)
    node = GTK_DTREE_ROW (node)->sibling.next;
  
  return node;
}

GtkDTreeNode *
gtk_dtree_node_nth (GtkDTree *dtree,
		    guint     row)
{
  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);

  if (((gint)row < 0) || ((gint)row >= GTK_CLIST(dtree)->rows))
    return NULL;
 
  return GTK_DTREE_NODE (g_list_nth (GTK_CLIST (dtree)->row_list, row));
}

gboolean
gtk_dtree_find (GtkDTree     *dtree,
		GtkDTreeNode *node,
		GtkDTreeNode *child)
{
  if (!child)
    return FALSE;

  if (!node)
    node = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  while (node)
    {
      if (node == child) 
	return TRUE;
      if (GTK_DTREE_ROW (node)->children.head)
	{
	  if (gtk_dtree_find (dtree, GTK_DTREE_ROW (node)->children.head, child))
	    return TRUE;
	}
      node = GTK_DTREE_ROW (node)->sibling.next;
    }
  return FALSE;
}

gboolean
gtk_dtree_is_ancestor (GtkDTree     *dtree,
		       GtkDTreeNode *node,
		       GtkDTreeNode *child)
{
  g_return_val_if_fail (GTK_IS_DTREE (dtree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  if (GTK_DTREE_ROW (node)->children.head)
    return gtk_dtree_find (dtree, GTK_DTREE_ROW (node)->children.head, child);

  return FALSE;
}

GtkDTreeNode *
gtk_dtree_find_by_row_data (GtkDTree     *dtree,
			    GtkDTreeNode *node,
			    gpointer      data)
{
  GtkDTreeNode *work;
  
  if (!node)
    node = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);
  
  while (node)
    {
      if (GTK_DTREE_ROW (node)->row.data == data) 
	return node;
      if (GTK_DTREE_ROW (node)->children.head &&
	  (work = gtk_dtree_find_by_row_data 
	   (dtree, GTK_DTREE_ROW (node)->children.head, data)))
	return work;
      node = GTK_DTREE_ROW (node)->sibling.next;
    }
  return NULL;
}

GList *
gtk_dtree_find_all_by_row_data (GtkDTree     *dtree,
				GtkDTreeNode *node,
				gpointer      data)
{
  GList *list = NULL;

  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);

  /* if node == NULL then look in the whole tree */
  if (!node)
    node = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  while (node)
    {
      if (GTK_DTREE_ROW (node)->row.data == data)
        list = g_list_append (list, node);

      if (GTK_DTREE_ROW (node)->children.head)
        {
	  GList *sub_list;

          sub_list = gtk_dtree_find_all_by_row_data (dtree,
						     GTK_DTREE_ROW
						     (node)->children.head,
						     data);
          list = g_list_concat (list, sub_list);
        }
      node = GTK_DTREE_ROW (node)->sibling.next;
    }
  return list;
}

GtkDTreeNode *
gtk_dtree_find_by_row_data_custom (GtkDTree     *dtree,
				   GtkDTreeNode *node,
				   gpointer      data,
				   GCompareFunc  func)
{
  GtkDTreeNode *work;

  g_return_val_if_fail (func != NULL, NULL);

  if (!node)
    node = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  while (node)
    {
      if (!func (GTK_DTREE_ROW (node)->row.data, data))
	return node;
      if (GTK_DTREE_ROW (node)->children.head &&
	  (work = gtk_dtree_find_by_row_data_custom
	   (dtree, GTK_DTREE_ROW (node)->children.head, data, func)))
	return work;
      node = GTK_DTREE_ROW (node)->sibling.next;
    }
  return NULL;
}

GList *
gtk_dtree_find_all_by_row_data_custom (GtkDTree     *dtree,
				       GtkDTreeNode *node,
				       gpointer      data,
				       GCompareFunc  func)
{
  GList *list = NULL;

  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);
  g_return_val_if_fail (func != NULL, NULL);

  /* if node == NULL then look in the whole tree */
  if (!node)
    node = GTK_DTREE_NODE (GTK_CLIST (dtree)->row_list);

  while (node)
    {
      if (!func (GTK_DTREE_ROW (node)->row.data, data))
        list = g_list_append (list, node);

      if (GTK_DTREE_ROW (node)->children.head)
        {
	  GList *sub_list;

          sub_list = gtk_dtree_find_all_by_row_data_custom (dtree,
							    GTK_DTREE_ROW
							    (node)->children.head,
							    data,
							    func);
          list = g_list_concat (list, sub_list);
        }
      node = GTK_DTREE_ROW (node)->sibling.next;
    }
  return list;
}

gboolean
gtk_dtree_is_hot_spot (GtkDTree *dtree, 
		       gint      x, 
		       gint      y)
{
  GtkDTreeNode *node;
  gint column;
  gint row;
  
  g_return_val_if_fail (dtree != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), FALSE);

  if (gtk_clist_get_selection_info (GTK_CLIST (dtree), x, y, &row, &column))
    if ((node = GTK_DTREE_NODE(g_list_nth (GTK_CLIST (dtree)->row_list, row))))
      return dtree_is_hot_spot (dtree, node, row, x, y);

  return FALSE;
}


/***********************************************************
 *   Tree signals : move, expand, collapse, (un)select     *
 ***********************************************************/


void
gtk_dtree_move (GtkDTree     *dtree,
		GtkDTreeNode *node,
		GtkDTreeNode *new_parent, 
		GtkDTreeNode *new_sibling)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);
  
  gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_MOVE], node,
		   new_parent, new_sibling);
}

void
gtk_dtree_expand (GtkDTree     *dtree,
		  GtkDTreeNode *node)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);
  
  if (GTK_DTREE_ROW (node)->is_leaf)
    return;

  gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_EXPAND], node);
}

void 
gtk_dtree_expand_recursive (GtkDTree     *dtree,
			    GtkDTreeNode *node)
{
  GtkCList *clist;
  gboolean thaw = FALSE;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  if (node && GTK_DTREE_ROW (node)->is_leaf)
    return;

  if (CLIST_UNFROZEN (clist) && (!node || gtk_dtree_is_viewable (dtree, node)))
    {
      gtk_clist_freeze (clist);
      thaw = TRUE;
    }

  gtk_dtree_post_recursive (dtree, node, GTK_DTREE_FUNC (tree_expand), NULL);

  if (thaw)
    gtk_clist_thaw (clist);
}

void 
gtk_dtree_expand_to_depth (GtkDTree     *dtree,
			   GtkDTreeNode *node,
			   gint          depth)
{
  GtkCList *clist;
  gboolean thaw = FALSE;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  if (node && GTK_DTREE_ROW (node)->is_leaf)
    return;

  if (CLIST_UNFROZEN (clist) && (!node || gtk_dtree_is_viewable (dtree, node)))
    {
      gtk_clist_freeze (clist);
      thaw = TRUE;
    }

  gtk_dtree_post_recursive_to_depth (dtree, node, depth,
				     GTK_DTREE_FUNC (tree_expand), NULL);

  if (thaw)
    gtk_clist_thaw (clist);
}

void
gtk_dtree_collapse (GtkDTree     *dtree,
		    GtkDTreeNode *node)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);
  
  if (GTK_DTREE_ROW (node)->is_leaf)
    return;

  gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_COLLAPSE], node);
}

void 
gtk_dtree_collapse_recursive (GtkDTree     *dtree,
			      GtkDTreeNode *node)
{
  GtkCList *clist;
  gboolean thaw = FALSE;
  gint i;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (node && GTK_DTREE_ROW (node)->is_leaf)
    return;

  clist = GTK_CLIST (dtree);

  if (CLIST_UNFROZEN (clist) && (!node || gtk_dtree_is_viewable (dtree, node)))
    {
      gtk_clist_freeze (clist);
      thaw = TRUE;
    }

  GTK_CLIST_SET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  gtk_dtree_post_recursive (dtree, node, GTK_DTREE_FUNC (tree_collapse), NULL);
  GTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].auto_resize)
      gtk_clist_set_column_width (clist, i,
				  gtk_clist_optimal_column_width (clist, i));

  if (thaw)
    gtk_clist_thaw (clist);
}

void 
gtk_dtree_collapse_to_depth (GtkDTree     *dtree,
			     GtkDTreeNode *node,
			     gint          depth)
{
  GtkCList *clist;
  gboolean thaw = FALSE;
  gint i;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (node && GTK_DTREE_ROW (node)->is_leaf)
    return;

  clist = GTK_CLIST (dtree);

  if (CLIST_UNFROZEN (clist) && (!node || gtk_dtree_is_viewable (dtree, node)))
    {
      gtk_clist_freeze (clist);
      thaw = TRUE;
    }

  GTK_CLIST_SET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  gtk_dtree_post_recursive_to_depth (dtree, node, depth,
				     GTK_DTREE_FUNC (tree_collapse_to_depth),
				     GINT_TO_POINTER (depth));
  GTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].auto_resize)
      gtk_clist_set_column_width (clist, i,
				  gtk_clist_optimal_column_width (clist, i));

  if (thaw)
    gtk_clist_thaw (clist);
}

void
gtk_dtree_toggle_expansion (GtkDTree     *dtree,
			    GtkDTreeNode *node)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);
  
  if (GTK_DTREE_ROW (node)->is_leaf)
    return;

  tree_toggle_expansion (dtree, node, NULL);
}

void 
gtk_dtree_toggle_expansion_recursive (GtkDTree     *dtree,
				      GtkDTreeNode *node)
{
  GtkCList *clist;
  gboolean thaw = FALSE;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  
  if (node && GTK_DTREE_ROW (node)->is_leaf)
    return;

  clist = GTK_CLIST (dtree);

  if (CLIST_UNFROZEN (clist) && (!node || gtk_dtree_is_viewable (dtree, node)))
    {
      gtk_clist_freeze (clist);
      thaw = TRUE;
    }
  
  gtk_dtree_post_recursive (dtree, node,
			    GTK_DTREE_FUNC (tree_toggle_expansion), NULL);

  if (thaw)
    gtk_clist_thaw (clist);
}

void
gtk_dtree_select (GtkDTree     *dtree, 
		  GtkDTreeNode *node)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  if (GTK_DTREE_ROW (node)->row.selectable)
    gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_SELECT_ROW],
		     node, -1);
}

void
gtk_dtree_unselect (GtkDTree     *dtree, 
		    GtkDTreeNode *node)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  gtk_signal_emit (GTK_OBJECT (dtree), dtree_signals[TREE_UNSELECT_ROW],
		   node, -1);
}

void
gtk_dtree_select_recursive (GtkDTree     *dtree, 
			    GtkDTreeNode *node)
{
  gtk_dtree_real_select_recursive (dtree, node, TRUE);
}

void
gtk_dtree_unselect_recursive (GtkDTree     *dtree, 
			      GtkDTreeNode *node)
{
  gtk_dtree_real_select_recursive (dtree, node, FALSE);
}

void
gtk_dtree_real_select_recursive (GtkDTree     *dtree, 
				 GtkDTreeNode *node, 
				 gint          state)
{
  GtkCList *clist;
  gboolean thaw = FALSE;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  if ((state && 
       (clist->selection_mode ==  GTK_SELECTION_BROWSE ||
	clist->selection_mode == GTK_SELECTION_SINGLE)) ||
      (!state && clist->selection_mode ==  GTK_SELECTION_BROWSE))
    return;

  if (CLIST_UNFROZEN (clist) && (!node || gtk_dtree_is_viewable (dtree, node)))
    {
      gtk_clist_freeze (clist);
      thaw = TRUE;
    }

  if (clist->selection_mode == GTK_SELECTION_EXTENDED)
    {
      GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (state)
    gtk_dtree_post_recursive (dtree, node,
			      GTK_DTREE_FUNC (tree_select), NULL);
  else 
    gtk_dtree_post_recursive (dtree, node,
			      GTK_DTREE_FUNC (tree_unselect), NULL);
  
  if (thaw)
    gtk_clist_thaw (clist);
}


/***********************************************************
 *           Analogons of GtkCList functions               *
 ***********************************************************/


void 
gtk_dtree_node_set_text (GtkDTree     *dtree,
			 GtkDTreeNode *node,
			 gint          column,
			 const gchar  *text)
{
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return;
  
  clist = GTK_CLIST (dtree);

  GTK_CLIST_CLASS_FW (clist)->set_cell_contents
    (clist, &(GTK_DTREE_ROW(node)->row), column, GTK_CELL_TEXT,
     text, 0, NULL, NULL);

  tree_draw_node (dtree, node);
}

void 
gtk_dtree_node_set_pixmap (GtkDTree     *dtree,
			   GtkDTreeNode *node,
			   gint          column,
			   GdkPixmap    *pixmap,
			   GdkBitmap    *mask)
{
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);
  g_return_if_fail (pixmap != NULL);

  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return;

  gdk_pixmap_ref (pixmap);
  if (mask) 
    gdk_pixmap_ref (mask);

  clist = GTK_CLIST (dtree);

  GTK_CLIST_CLASS_FW (clist)->set_cell_contents
    (clist, &(GTK_DTREE_ROW (node)->row), column, GTK_CELL_PIXMAP,
     NULL, 0, pixmap, mask);

  tree_draw_node (dtree, node);
}

void 
gtk_dtree_node_set_pixtext (GtkDTree     *dtree,
			    GtkDTreeNode *node,
			    gint          column,
			    const gchar  *text,
			    guint8        spacing,
			    GdkPixmap    *pixmap,
			    GdkBitmap    *mask)
{
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);
  if (column != dtree->tree_column)
    g_return_if_fail (pixmap != NULL);
  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return;

  clist = GTK_CLIST (dtree);

  if (pixmap)
    {
      gdk_pixmap_ref (pixmap);
      if (mask) 
	gdk_pixmap_ref (mask);
    }

  GTK_CLIST_CLASS_FW (clist)->set_cell_contents
    (clist, &(GTK_DTREE_ROW (node)->row), column, GTK_CELL_PIXTEXT,
     text, spacing, pixmap, mask);

  tree_draw_node (dtree, node);
}

void 
gtk_dtree_set_node_info (GtkDTree     *dtree,
			 GtkDTreeNode *node,
			 const gchar  *text,
			 guint8        spacing,
			 GdkPixmap    *pixmap_closed,
			 GdkBitmap    *mask_closed,
			 GdkPixmap    *pixmap_opened,
			 GdkBitmap    *mask_opened,
			 gboolean      is_leaf,
			 gboolean      expanded)
{
  gboolean old_leaf;
  gboolean old_expanded;
 
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  old_leaf = GTK_DTREE_ROW (node)->is_leaf;
  old_expanded = GTK_DTREE_ROW (node)->expanded;

  if (is_leaf && GTK_DTREE_ROW (node)->children.head)
    {
      GtkDTreeNode *work;
      GtkDTreeNode *ptr;
      
      work = GTK_DTREE_ROW (node)->children.head;
      while (work)
	{
	  ptr = work;
	  work = GTK_DTREE_ROW(work)->sibling.next;
	  gtk_dtree_remove_node (dtree, ptr);
	}
    }

  set_node_info (dtree, node, text, spacing, pixmap_closed, mask_closed,
		 pixmap_opened, mask_opened, is_leaf, expanded);

  if (!is_leaf && !old_leaf)
    {
      GTK_DTREE_ROW (node)->expanded = old_expanded;
      if (expanded && !old_expanded)
	gtk_dtree_expand (dtree, node);
      else if (!expanded && old_expanded)
	gtk_dtree_collapse (dtree, node);
    }

  GTK_DTREE_ROW (node)->expanded = (is_leaf) ? FALSE : expanded;
  
  tree_draw_node (dtree, node);
}

void
gtk_dtree_node_set_shift (GtkDTree     *dtree,
			  GtkDTreeNode *node,
			  gint          column,
			  gint          vertical,
			  gint          horizontal)
{
  GtkCList *clist;
  GtkRequisition requisition;
  gboolean visible = FALSE;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return;

  clist = GTK_CLIST (dtree);

  if (clist->column[column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      visible = gtk_dtree_is_viewable (dtree, node);
      if (visible)
	GTK_CLIST_CLASS_FW (clist)->cell_size_request
	  (clist, &GTK_DTREE_ROW (node)->row, column, &requisition);
    }

  GTK_DTREE_ROW (node)->row.cell[column].vertical   = vertical;
  GTK_DTREE_ROW (node)->row.cell[column].horizontal = horizontal;

  if (visible)
    column_auto_resize (clist, &GTK_DTREE_ROW (node)->row,
			column, requisition.width);

  tree_draw_node (dtree, node);
}

static void
remove_grab (GtkCList *clist)
{
  if (gdk_pointer_is_grabbed () && GTK_WIDGET_HAS_GRAB (clist))
    {
      gtk_grab_remove (GTK_WIDGET (clist));
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
    }

  if (clist->htimer)
    {
      gtk_timeout_remove (clist->htimer);
      clist->htimer = 0;
    }

  if (clist->vtimer)
    {
      gtk_timeout_remove (clist->vtimer);
      clist->vtimer = 0;
    }
}

void
gtk_dtree_node_set_selectable (GtkDTree     *dtree,
			       GtkDTreeNode *node,
			       gboolean      selectable)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  if (selectable == GTK_DTREE_ROW (node)->row.selectable)
    return;

  GTK_DTREE_ROW (node)->row.selectable = selectable;

  if (!selectable && GTK_DTREE_ROW (node)->row.state == GTK_STATE_SELECTED)
    {
      GtkCList *clist;

      clist = GTK_CLIST (dtree);

      if (clist->anchor >= 0 &&
	  clist->selection_mode == GTK_SELECTION_EXTENDED)
	{
	  clist->drag_button = 0;
	  remove_grab (clist);

	  GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
	}
      gtk_dtree_unselect (dtree, node);
    }      
}

gboolean
gtk_dtree_node_get_selectable (GtkDTree     *dtree,
			       GtkDTreeNode *node)
{
  g_return_val_if_fail (node != NULL, FALSE);

  return GTK_DTREE_ROW (node)->row.selectable;
}

GtkCellType 
gtk_dtree_node_get_cell_type (GtkDTree     *dtree,
			      GtkDTreeNode *node,
			      gint          column)
{
  g_return_val_if_fail (dtree != NULL, -1);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), -1);
  g_return_val_if_fail (node != NULL, -1);

  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return -1;

  return GTK_DTREE_ROW (node)->row.cell[column].type;
}

gint
gtk_dtree_node_get_text (GtkDTree      *dtree,
			 GtkDTreeNode  *node,
			 gint           column,
			 gchar        **text)
{
  g_return_val_if_fail (dtree != NULL, 0);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), 0);
  g_return_val_if_fail (node != NULL, 0);

  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return 0;

  if (GTK_DTREE_ROW (node)->row.cell[column].type != GTK_CELL_TEXT)
    return 0;

  if (text)
    *text = GTK_CELL_TEXT (GTK_DTREE_ROW (node)->row.cell[column])->text;

  return 1;
}

gint
gtk_dtree_node_get_pixmap (GtkDTree     *dtree,
			   GtkDTreeNode *node,
			   gint          column,
			   GdkPixmap   **pixmap,
			   GdkBitmap   **mask)
{
  g_return_val_if_fail (dtree != NULL, 0);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), 0);
  g_return_val_if_fail (node != NULL, 0);

  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return 0;

  if (GTK_DTREE_ROW (node)->row.cell[column].type != GTK_CELL_PIXMAP)
    return 0;

  if (pixmap)
    *pixmap = GTK_CELL_PIXMAP (GTK_DTREE_ROW(node)->row.cell[column])->pixmap;
  if (mask)
    *mask = GTK_CELL_PIXMAP (GTK_DTREE_ROW (node)->row.cell[column])->mask;

  return 1;
}

gint
gtk_dtree_node_get_pixtext (GtkDTree      *dtree,
			    GtkDTreeNode  *node,
			    gint           column,
			    gchar        **text,
			    guint8        *spacing,
			    GdkPixmap    **pixmap,
			    GdkBitmap    **mask)
{
  g_return_val_if_fail (dtree != NULL, 0);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), 0);
  g_return_val_if_fail (node != NULL, 0);
  
  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return 0;
  
  if (GTK_DTREE_ROW (node)->row.cell[column].type != GTK_CELL_PIXTEXT)
    return 0;
  
  if (text)
    *text = GTK_CELL_PIXTEXT (GTK_DTREE_ROW (node)->row.cell[column])->text;
  if (spacing)
    *spacing = GTK_CELL_PIXTEXT (GTK_DTREE_ROW 
				 (node)->row.cell[column])->spacing;
  if (pixmap)
    *pixmap = GTK_CELL_PIXTEXT (GTK_DTREE_ROW 
				(node)->row.cell[column])->pixmap;
  if (mask)
    *mask = GTK_CELL_PIXTEXT (GTK_DTREE_ROW (node)->row.cell[column])->mask;
  
  return 1;
}

gint
gtk_dtree_get_node_info (GtkDTree      *dtree,
			 GtkDTreeNode  *node,
			 gchar        **text,
			 guint8        *spacing,
			 GdkPixmap    **pixmap_closed,
			 GdkBitmap    **mask_closed,
			 GdkPixmap    **pixmap_opened,
			 GdkBitmap    **mask_opened,
			 gboolean      *is_leaf,
			 gboolean      *expanded)
{
  g_return_val_if_fail (dtree != NULL, 0);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), 0);
  g_return_val_if_fail (node != NULL, 0);
  
  if (text)
    *text = GTK_CELL_PIXTEXT 
      (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->text;
  if (spacing)
    *spacing = GTK_CELL_PIXTEXT 
      (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->spacing;
  if (pixmap_closed)
    *pixmap_closed = GTK_DTREE_ROW (node)->pixmap_closed;
  if (mask_closed)
    *mask_closed = GTK_DTREE_ROW (node)->mask_closed;
  if (pixmap_opened)
    *pixmap_opened = GTK_DTREE_ROW (node)->pixmap_opened;
  if (mask_opened)
    *mask_opened = GTK_DTREE_ROW (node)->mask_opened;
  if (is_leaf)
    *is_leaf = GTK_DTREE_ROW (node)->is_leaf;
  if (expanded)
    *expanded = GTK_DTREE_ROW (node)->expanded;
  
  return 1;
}

void
gtk_dtree_node_set_cell_style (GtkDTree     *dtree,
			       GtkDTreeNode *node,
			       gint          column,
			       GtkStyle     *style)
{
  GtkCList *clist;
  GtkRequisition requisition;
  gboolean visible = FALSE;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  clist = GTK_CLIST (dtree);

  if (column < 0 || column >= clist->columns)
    return;

  if (GTK_DTREE_ROW (node)->row.cell[column].style == style)
    return;

  if (clist->column[column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      visible = gtk_dtree_is_viewable (dtree, node);
      if (visible)
	GTK_CLIST_CLASS_FW (clist)->cell_size_request
	  (clist, &GTK_DTREE_ROW (node)->row, column, &requisition);
    }

  if (GTK_DTREE_ROW (node)->row.cell[column].style)
    {
      if (GTK_WIDGET_REALIZED (dtree))
        gtk_style_detach (GTK_DTREE_ROW (node)->row.cell[column].style);
      gtk_style_unref (GTK_DTREE_ROW (node)->row.cell[column].style);
    }

  GTK_DTREE_ROW (node)->row.cell[column].style = style;

  if (GTK_DTREE_ROW (node)->row.cell[column].style)
    {
      gtk_style_ref (GTK_DTREE_ROW (node)->row.cell[column].style);
      
      if (GTK_WIDGET_REALIZED (dtree))
        GTK_DTREE_ROW (node)->row.cell[column].style =
	  gtk_style_attach (GTK_DTREE_ROW (node)->row.cell[column].style,
			    clist->clist_window);
    }

  if (visible)
    column_auto_resize (clist, &GTK_DTREE_ROW (node)->row, column,
			requisition.width);

  tree_draw_node (dtree, node);
}

GtkStyle *
gtk_dtree_node_get_cell_style (GtkDTree     *dtree,
			       GtkDTreeNode *node,
			       gint          column)
{
  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);
  g_return_val_if_fail (node != NULL, NULL);

  if (column < 0 || column >= GTK_CLIST (dtree)->columns)
    return NULL;

  return GTK_DTREE_ROW (node)->row.cell[column].style;
}

void
gtk_dtree_node_set_row_style (GtkDTree     *dtree,
			      GtkDTreeNode *node,
			      GtkStyle     *style)
{
  GtkCList *clist;
  GtkRequisition requisition;
  gboolean visible;
  gint *old_width = NULL;
  gint i;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  clist = GTK_CLIST (dtree);

  if (GTK_DTREE_ROW (node)->row.style == style)
    return;
  
  visible = gtk_dtree_is_viewable (dtree, node);
  if (visible && !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      old_width = g_new (gint, clist->columns);
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  {
	    GTK_CLIST_CLASS_FW (clist)->cell_size_request
	      (clist, &GTK_DTREE_ROW (node)->row, i, &requisition);
	    old_width[i] = requisition.width;
	  }
    }

  if (GTK_DTREE_ROW (node)->row.style)
    {
      if (GTK_WIDGET_REALIZED (dtree))
        gtk_style_detach (GTK_DTREE_ROW (node)->row.style);
      gtk_style_unref (GTK_DTREE_ROW (node)->row.style);
    }

  GTK_DTREE_ROW (node)->row.style = style;

  if (GTK_DTREE_ROW (node)->row.style)
    {
      gtk_style_ref (GTK_DTREE_ROW (node)->row.style);
      
      if (GTK_WIDGET_REALIZED (dtree))
        GTK_DTREE_ROW (node)->row.style =
	  gtk_style_attach (GTK_DTREE_ROW (node)->row.style,
			    clist->clist_window);
    }

  if (visible && !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  column_auto_resize (clist, &GTK_DTREE_ROW (node)->row, i,
			      old_width[i]);
      g_free (old_width);
    }
  tree_draw_node (dtree, node);
}

GtkStyle *
gtk_dtree_node_get_row_style (GtkDTree     *dtree,
			      GtkDTreeNode *node)
{
  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);
  g_return_val_if_fail (node != NULL, NULL);

  return GTK_DTREE_ROW (node)->row.style;
}

void
gtk_dtree_node_set_foreground (GtkDTree     *dtree,
			       GtkDTreeNode *node,
			       GdkColor     *color)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  if (color)
    {
      GTK_DTREE_ROW (node)->row.foreground = *color;
      GTK_DTREE_ROW (node)->row.fg_set = TRUE;
      if (GTK_WIDGET_REALIZED (dtree))
	gdk_color_alloc (gtk_widget_get_colormap (GTK_WIDGET (dtree)),
			 &GTK_DTREE_ROW (node)->row.foreground);
    }
  else
    GTK_DTREE_ROW (node)->row.fg_set = FALSE;

  tree_draw_node (dtree, node);
}

void
gtk_dtree_node_set_background (GtkDTree     *dtree,
			       GtkDTreeNode *node,
			       GdkColor     *color)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  if (color)
    {
      GTK_DTREE_ROW (node)->row.background = *color;
      GTK_DTREE_ROW (node)->row.bg_set = TRUE;
      if (GTK_WIDGET_REALIZED (dtree))
	gdk_color_alloc (gtk_widget_get_colormap (GTK_WIDGET (dtree)),
			 &GTK_DTREE_ROW (node)->row.background);
    }
  else
    GTK_DTREE_ROW (node)->row.bg_set = FALSE;

  tree_draw_node (dtree, node);
}

void
gtk_dtree_node_set_row_data (GtkDTree     *dtree,
			     GtkDTreeNode *node,
			     gpointer      data)
{
  gtk_dtree_node_set_row_data_full (dtree, node, data, NULL);
}

void
gtk_dtree_node_set_row_data_full (GtkDTree         *dtree,
				  GtkDTreeNode     *node,
				  gpointer          data,
				  GtkDestroyNotify  destroy)
{
  GtkDestroyNotify dnotify;
  gpointer ddata;
  
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (node != NULL);

  dnotify = GTK_DTREE_ROW (node)->row.destroy;
  ddata = GTK_DTREE_ROW (node)->row.data;
  
  GTK_DTREE_ROW (node)->row.data = data;
  GTK_DTREE_ROW (node)->row.destroy = destroy;

  if (dnotify)
    dnotify (ddata);
}

gpointer
gtk_dtree_node_get_row_data (GtkDTree     *dtree,
			     GtkDTreeNode *node)
{
  g_return_val_if_fail (dtree != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), NULL);

  return node ? GTK_DTREE_ROW (node)->row.data : NULL;
}

void
gtk_dtree_node_moveto (GtkDTree     *dtree,
		       GtkDTreeNode *node,
		       gint          column,
		       gfloat        row_align,
		       gfloat        col_align)
{
  gint row = -1;
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  while (node && !gtk_dtree_is_viewable (dtree, node))
    node = GTK_DTREE_ROW (node)->parent;

  if (node)
    row = g_list_position (clist->row_list, (GList *)node);
  
  gtk_clist_moveto (clist, row, column, row_align, col_align);
}

GtkVisibility gtk_dtree_node_is_visible (GtkDTree     *dtree,
                                         GtkDTreeNode *node)
{
  gint row;
  
  g_return_val_if_fail (dtree != NULL, 0);
  g_return_val_if_fail (node != NULL, 0);
  
  row = g_list_position (GTK_CLIST (dtree)->row_list, (GList*) node);
  return gtk_clist_row_is_visible (GTK_CLIST (dtree), row);
}


/***********************************************************
 *             GtkDTree specific functions                 *
 ***********************************************************/

void
gtk_dtree_set_indent (GtkDTree *dtree, 
                      gint      indent)
{
  GtkCList *clist;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (indent >= 0);

  if (indent == dtree->tree_indent)
    return;

  clist = GTK_CLIST (dtree);
  dtree->tree_indent = indent;

  if (clist->column[dtree->tree_column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    gtk_clist_set_column_width
      (clist, dtree->tree_column,
       gtk_clist_optimal_column_width (clist, dtree->tree_column));
  else
    CLIST_REFRESH (dtree);
}

void
gtk_dtree_set_spacing (GtkDTree *dtree, 
		       gint      spacing)
{
  GtkCList *clist;
  gint old_spacing;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));
  g_return_if_fail (spacing >= 0);

  if (spacing == dtree->tree_spacing)
    return;

  clist = GTK_CLIST (dtree);

  old_spacing = dtree->tree_spacing;
  dtree->tree_spacing = spacing;

  if (clist->column[dtree->tree_column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    gtk_clist_set_column_width (clist, dtree->tree_column,
				clist->column[dtree->tree_column].width +
				spacing - old_spacing);
  else
    CLIST_REFRESH (dtree);
}

void
gtk_dtree_set_show_stub (GtkDTree *dtree, 
			 gboolean  show_stub)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  show_stub = show_stub != FALSE;

  if (show_stub != dtree->show_stub)
    {
      GtkCList *clist;

      clist = GTK_CLIST (dtree);
      dtree->show_stub = show_stub;

      if (CLIST_UNFROZEN (clist) && clist->rows &&
	  gtk_clist_row_is_visible (clist, 0) != GTK_VISIBILITY_NONE)
	GTK_CLIST_CLASS_FW (clist)->draw_row
	  (clist, NULL, 0, GTK_CLIST_ROW (clist->row_list));
    }
}

void 
gtk_dtree_set_line_style (GtkDTree          *dtree, 
			  GtkDTreeLineStyle  line_style)
{
  GtkCList *clist;
  GtkDTreeLineStyle old_style;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (line_style == dtree->line_style)
    return;

  clist = GTK_CLIST (dtree);

  old_style = dtree->line_style;
  dtree->line_style = line_style;

  if (clist->column[dtree->tree_column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      if (old_style == GTK_DTREE_LINES_TABBED)
	gtk_clist_set_column_width
	  (clist, dtree->tree_column,
	   clist->column[dtree->tree_column].width - 3);
      else if (line_style == GTK_DTREE_LINES_TABBED)
	gtk_clist_set_column_width
	  (clist, dtree->tree_column,
	   clist->column[dtree->tree_column].width + 3);
    }

  if (GTK_WIDGET_REALIZED (dtree))
    {
      switch (line_style)
	{
	case GTK_DTREE_LINES_SOLID:
	  if (GTK_WIDGET_REALIZED (dtree))
	    gdk_gc_set_line_attributes (dtree->lines_gc, 1, GDK_LINE_SOLID, 
					None, None);
	  break;
	case GTK_DTREE_LINES_DOTTED:
	  if (GTK_WIDGET_REALIZED (dtree))
	    gdk_gc_set_line_attributes (dtree->lines_gc, 1, 
					GDK_LINE_ON_OFF_DASH, None, None);
	  gdk_gc_set_dashes (dtree->lines_gc, 0, "\1\1", 2);
	  break;
	case GTK_DTREE_LINES_TABBED:
	  if (GTK_WIDGET_REALIZED (dtree))
	    gdk_gc_set_line_attributes (dtree->lines_gc, 1, GDK_LINE_SOLID, 
					None, None);
	  break;
	case GTK_DTREE_LINES_NONE:
	  break;
	default:
	  return;
	}
      CLIST_REFRESH (dtree);
    }
}

void 
gtk_dtree_set_expander_style (GtkDTree              *dtree, 
			      GtkDTreeExpanderStyle  expander_style)
{
  GtkCList *clist;
  GtkDTreeExpanderStyle old_style;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  if (expander_style == dtree->expander_style)
    return;

  clist = GTK_CLIST (dtree);

  old_style = dtree->expander_style;
  dtree->expander_style = expander_style;

  if (clist->column[dtree->tree_column].auto_resize &&
      !GTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      gint new_width;

      new_width = clist->column[dtree->tree_column].width;
      switch (old_style)
	{
	case GTK_DTREE_EXPANDER_NONE:
	  break;
	case GTK_DTREE_EXPANDER_TRIANGLE:
	  new_width -= PM_SIZE + 3;
	  break;
	case GTK_DTREE_EXPANDER_SQUARE:
	case GTK_DTREE_EXPANDER_CIRCULAR:
	  new_width -= PM_SIZE + 1;
	  break;
	}

      switch (expander_style)
	{
	case GTK_DTREE_EXPANDER_NONE:
	  break;
	case GTK_DTREE_EXPANDER_TRIANGLE:
	  new_width += PM_SIZE + 3;
	  break;
	case GTK_DTREE_EXPANDER_SQUARE:
	case GTK_DTREE_EXPANDER_CIRCULAR:
	  new_width += PM_SIZE + 1;
	  break;
	}

      gtk_clist_set_column_width (clist, dtree->tree_column, new_width);
    }

  if (GTK_WIDGET_DRAWABLE (clist))
    CLIST_REFRESH (clist);
}


/***********************************************************
 *             Tree sorting functions                      *
 ***********************************************************/


static void
tree_sort (GtkDTree     *dtree,
	   GtkDTreeNode *node,
	   gpointer      data)
{
  GtkDTreeNode *list_start;
  GtkDTreeNode *cmp;
  GtkDTreeNode *work;
  GtkCList *clist;

  clist = GTK_CLIST (dtree);

  if (node)
    list_start = GTK_DTREE_ROW (node)->children.head;
  else
    list_start = GTK_DTREE_NODE (clist->row_list);

  while (list_start)
    {
      cmp = list_start;
      work = GTK_DTREE_ROW (cmp)->sibling.next;
      while (work)
	{
	  if (clist->sort_type == GTK_SORT_ASCENDING)
	    {
	      if (clist->compare 
		  (clist, GTK_DTREE_ROW (work), GTK_DTREE_ROW (cmp)) < 0)
		cmp = work;
	    }
	  else
	    {
	      if (clist->compare 
		  (clist, GTK_DTREE_ROW (work), GTK_DTREE_ROW (cmp)) > 0)
		cmp = work;
	    }
	  work = GTK_DTREE_ROW (work)->sibling.next;
	}
      if (cmp == list_start)
	list_start = GTK_DTREE_ROW (cmp)->sibling.next;
      else
	{
	  gtk_dtree_unlink (dtree, cmp, FALSE);
	  gtk_dtree_link (dtree, cmp, node, list_start, FALSE);
	}
    }
}

void
gtk_dtree_sort_recursive (GtkDTree     *dtree, 
			  GtkDTreeNode *node)
{
  GtkCList *clist;
  GtkDTreeNode *focus_node = NULL;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  gtk_clist_freeze (clist);

  if (clist->selection_mode == GTK_SELECTION_EXTENDED)
    {
      GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (!node || (node && gtk_dtree_is_viewable (dtree, node)))
    focus_node =
      GTK_DTREE_NODE (g_list_nth (clist->row_list, clist->focus_row));
      
  gtk_dtree_post_recursive (dtree, node, GTK_DTREE_FUNC (tree_sort), NULL);

  if (!node)
    tree_sort (dtree, NULL, NULL);

  if (focus_node)
    {
      clist->focus_row = g_list_position (clist->row_list,(GList *)focus_node);
      clist->undo_anchor = clist->focus_row;
    }

  gtk_clist_thaw (clist);
}

static void
real_sort_list (GtkCList *clist)
{
  gtk_dtree_sort_recursive (GTK_DTREE (clist), NULL);
}

void
gtk_dtree_sort_node (GtkDTree     *dtree, 
		     GtkDTreeNode *node)
{
  GtkCList *clist;
  GtkDTreeNode *focus_node = NULL;

  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  clist = GTK_CLIST (dtree);

  gtk_clist_freeze (clist);

  if (clist->selection_mode == GTK_SELECTION_EXTENDED)
    {
      GTK_CLIST_CLASS_FW (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (!node || (node && gtk_dtree_is_viewable (dtree, node)))
    focus_node = GTK_DTREE_NODE
      (g_list_nth (clist->row_list, clist->focus_row));

  tree_sort (dtree, node, NULL);

  if (focus_node)
    {
      clist->focus_row = g_list_position (clist->row_list,(GList *)focus_node);
      clist->undo_anchor = clist->focus_row;
    }

  gtk_clist_thaw (clist);
}

/************************************************************************/

static void
fake_unselect_all (GtkCList *clist,
		   gint      row)
{
  GList *list;
  GList *focus_node = NULL;

  if (row >= 0 && (focus_node = g_list_nth (clist->row_list, row)))
    {
      if (GTK_DTREE_ROW (focus_node)->row.state == GTK_STATE_NORMAL &&
	  GTK_DTREE_ROW (focus_node)->row.selectable)
	{
	  GTK_DTREE_ROW (focus_node)->row.state = GTK_STATE_SELECTED;
	  
	  if (CLIST_UNFROZEN (clist) &&
	      gtk_clist_row_is_visible (clist, row) != GTK_VISIBILITY_NONE)
	    GTK_CLIST_CLASS_FW (clist)->draw_row (clist, NULL, row,
						  GTK_CLIST_ROW (focus_node));
	}  
    }

  clist->undo_selection = clist->selection;
  clist->selection = NULL;
  clist->selection_end = NULL;
  
  for (list = clist->undo_selection; list; list = list->next)
    {
      if (list->data == focus_node)
	continue;

      GTK_DTREE_ROW ((GList *)(list->data))->row.state = GTK_STATE_NORMAL;
      tree_draw_node (GTK_DTREE (clist), GTK_DTREE_NODE (list->data));
    }
}

static GList *
selection_find (GtkCList *clist,
		gint      row_number,
		GList    *row_list_element)
{
  return g_list_find (clist->selection, row_list_element);
}

static void
resync_selection (GtkCList *clist, GdkEvent *event)
{
  GtkDTree *dtree;
  GList *list;
  GtkDTreeNode *node;
  gint i;
  gint e;
  gint row;
  gboolean unselect;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  if (clist->selection_mode != GTK_SELECTION_EXTENDED)
    return;

  if (clist->anchor < 0 || clist->drag_pos < 0)
    return;

  dtree = GTK_DTREE (clist);
  
  clist->freeze_count++;

  i = MIN (clist->anchor, clist->drag_pos);
  e = MAX (clist->anchor, clist->drag_pos);

  if (clist->undo_selection)
    {
      list = clist->selection;
      clist->selection = clist->undo_selection;
      clist->selection_end = g_list_last (clist->selection);
      clist->undo_selection = list;
      list = clist->selection;

      while (list)
	{
	  node = list->data;
	  list = list->next;
	  
	  unselect = TRUE;

	  if (gtk_dtree_is_viewable (dtree, node))
	    {
	      row = g_list_position (clist->row_list, (GList *)node);
	      if (row >= i && row <= e)
		unselect = FALSE;
	    }
	  if (unselect && GTK_DTREE_ROW (node)->row.selectable)
	    {
	      GTK_DTREE_ROW (node)->row.state = GTK_STATE_SELECTED;
	      gtk_dtree_unselect (dtree, node);
	      clist->undo_selection = g_list_prepend (clist->undo_selection,
						      node);
	    }
	}
    }    

  if (clist->anchor < clist->drag_pos)
    {
      for (node = GTK_DTREE_NODE (g_list_nth (clist->row_list, i)); i <= e;
	   i++, node = GTK_DTREE_NODE_NEXT (node))
	if (GTK_DTREE_ROW (node)->row.selectable)
	  {
	    if (g_list_find (clist->selection, node))
	      {
		if (GTK_DTREE_ROW (node)->row.state == GTK_STATE_NORMAL)
		  {
		    GTK_DTREE_ROW (node)->row.state = GTK_STATE_SELECTED;
		    gtk_dtree_unselect (dtree, node);
		    clist->undo_selection =
		      g_list_prepend (clist->undo_selection, node);
		  }
	      }
	    else if (GTK_DTREE_ROW (node)->row.state == GTK_STATE_SELECTED)
	      {
		GTK_DTREE_ROW (node)->row.state = GTK_STATE_NORMAL;
		clist->undo_unselection =
		  g_list_prepend (clist->undo_unselection, node);
	      }
	  }
    }
  else
    {
      for (node = GTK_DTREE_NODE (g_list_nth (clist->row_list, e)); i <= e;
	   e--, node = GTK_DTREE_NODE_PREV (node))
	if (GTK_DTREE_ROW (node)->row.selectable)
	  {
	    if (g_list_find (clist->selection, node))
	      {
		if (GTK_DTREE_ROW (node)->row.state == GTK_STATE_NORMAL)
		  {
		    GTK_DTREE_ROW (node)->row.state = GTK_STATE_SELECTED;
		    gtk_dtree_unselect (dtree, node);
		    clist->undo_selection =
		      g_list_prepend (clist->undo_selection, node);
		  }
	      }
	    else if (GTK_DTREE_ROW (node)->row.state == GTK_STATE_SELECTED)
	      {
		GTK_DTREE_ROW (node)->row.state = GTK_STATE_NORMAL;
		clist->undo_unselection =
		  g_list_prepend (clist->undo_unselection, node);
	      }
	  }
    }

  clist->undo_unselection = g_list_reverse (clist->undo_unselection);
  for (list = clist->undo_unselection; list; list = list->next)
    gtk_dtree_select (dtree, list->data);

  clist->anchor = -1;
  clist->drag_pos = -1;

  if (!CLIST_UNFROZEN (clist))
    clist->freeze_count--;
}

static void
real_undo_selection (GtkCList *clist)
{
  GtkDTree *dtree;
  GList *work;

  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_DTREE (clist));

  if (clist->selection_mode != GTK_SELECTION_EXTENDED)
    return;

  if (!(clist->undo_selection || clist->undo_unselection))
    {
      gtk_clist_unselect_all (clist);
      return;
    }

  dtree = GTK_DTREE (clist);

  for (work = clist->undo_selection; work; work = work->next)
    if (GTK_DTREE_ROW (work->data)->row.selectable)
      gtk_dtree_select (dtree, GTK_DTREE_NODE (work->data));

  for (work = clist->undo_unselection; work; work = work->next)
    if (GTK_DTREE_ROW (work->data)->row.selectable)
      gtk_dtree_unselect (dtree, GTK_DTREE_NODE (work->data));

  if (GTK_WIDGET_HAS_FOCUS (clist) && clist->focus_row != clist->undo_anchor)
    {
      gtk_widget_draw_focus (GTK_WIDGET (clist));
      clist->focus_row = clist->undo_anchor;
      gtk_widget_draw_focus (GTK_WIDGET (clist));
    }
  else
    clist->focus_row = clist->undo_anchor;
  
  clist->undo_anchor = -1;
 
  g_list_free (clist->undo_selection);
  g_list_free (clist->undo_unselection);
  clist->undo_selection = NULL;
  clist->undo_unselection = NULL;

  if (ROW_TOP_YPIXEL (clist, clist->focus_row) + clist->row_height >
      clist->clist_window_height)
    gtk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
  else if (ROW_TOP_YPIXEL (clist, clist->focus_row) < 0)
    gtk_clist_moveto (clist, clist->focus_row, -1, 0, 0);

}

void
gtk_dtree_set_drag_compare_func (GtkDTree                *dtree,
				 GtkDTreeCompareDragFunc  cmp_func)
{
  g_return_if_fail (dtree != NULL);
  g_return_if_fail (GTK_IS_DTREE (dtree));

  dtree->drag_compare = cmp_func;
}

static gboolean
check_drag (GtkDTree        *dtree,
	    GtkDTreeNode    *drag_source,
	    GtkDTreeNode    *drag_target,
	    GtkCListDragPos  insert_pos)
{
  g_return_val_if_fail (dtree != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DTREE (dtree), FALSE);

  if (drag_source && drag_source != drag_target &&
      (!GTK_DTREE_ROW (drag_source)->children.head ||
       !gtk_dtree_is_ancestor (dtree, drag_source, drag_target)))
    {
      switch (insert_pos)
	{
	case GTK_CLIST_DRAG_NONE:
	  return FALSE;
	case GTK_CLIST_DRAG_AFTER:
	  if (GTK_DTREE_ROW (drag_target)->sibling.next != drag_source)
	    return (!dtree->drag_compare ||
		    dtree->drag_compare (dtree,
					 drag_source,
					 GTK_DTREE_ROW (drag_target)->parent,
					 GTK_DTREE_ROW(drag_target)->sibling.next));
	  break;
	case GTK_CLIST_DRAG_BEFORE:
	  if (GTK_DTREE_ROW (drag_source)->sibling.next != drag_target)
	    return (!dtree->drag_compare ||
		    dtree->drag_compare (dtree,
					 drag_source,
					 GTK_DTREE_ROW (drag_target)->parent,
					 drag_target));
	  break;
	case GTK_CLIST_DRAG_INTO:
	  if (!GTK_DTREE_ROW (drag_target)->is_leaf &&
	      GTK_DTREE_ROW (drag_target)->children.head != drag_source)
	    return (!dtree->drag_compare ||
		    dtree->drag_compare (dtree,
					 drag_source,
					 drag_target,
					 GTK_DTREE_ROW (drag_target)->children.head));
	  break;
	}
    }
  return FALSE;
}



/************************************/
static void
drag_dest_info_destroy (gpointer data)
{
  GtkCListDestInfo *info = data;

  g_free (info);
}

static void
drag_dest_cell (GtkCList         *clist,
		gint              x,
		gint              y,
		GtkCListDestInfo *dest_info)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (clist);

  dest_info->insert_pos = GTK_CLIST_DRAG_NONE;

  y -= (GTK_CONTAINER (widget)->border_width +
	widget->style->klass->ythickness + clist->column_title_area.height);
  dest_info->cell.row = ROW_FROM_YPIXEL (clist, y);

  if (dest_info->cell.row >= clist->rows)
    {
      dest_info->cell.row = clist->rows - 1;
      y = ROW_TOP_YPIXEL (clist, dest_info->cell.row) + clist->row_height;
    }
  if (dest_info->cell.row < -1)
    dest_info->cell.row = -1;

  x -= GTK_CONTAINER (widget)->border_width + widget->style->klass->xthickness;
  dest_info->cell.column = COLUMN_FROM_XPIXEL (clist, x);

  if (dest_info->cell.row >= 0)
    {
      gint y_delta;
      gint h = 0;

      y_delta = y - ROW_TOP_YPIXEL (clist, dest_info->cell.row);
      
      if (GTK_CLIST_DRAW_DRAG_RECT(clist) &&
	  !GTK_DTREE_ROW (g_list_nth (clist->row_list,
				      dest_info->cell.row))->is_leaf)
	{
	  dest_info->insert_pos = GTK_CLIST_DRAG_INTO;
	  h = clist->row_height / 4;
	}
      else if (GTK_CLIST_DRAW_DRAG_LINE(clist))
	{
	  dest_info->insert_pos = GTK_CLIST_DRAG_BEFORE;
	  h = clist->row_height / 2;
	}

      if (GTK_CLIST_DRAW_DRAG_LINE(clist))
	{
	  if (y_delta < h)
	    dest_info->insert_pos = GTK_CLIST_DRAG_BEFORE;
	  else if (clist->row_height - y_delta < h)
	    dest_info->insert_pos = GTK_CLIST_DRAG_AFTER;
	}
    }
}

static void
gtk_dtree_drag_begin (GtkWidget	     *widget,
		      GdkDragContext *context)
{
  GtkCList *clist;
  GtkDTree *dtree;
  gboolean use_icons;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DTREE (widget));
  g_return_if_fail (context != NULL);

  clist = GTK_CLIST (widget);
  dtree = GTK_DTREE (widget);

  use_icons = GTK_CLIST_USE_DRAG_ICONS (clist);
  GTK_CLIST_UNSET_FLAG (clist, CLIST_USE_DRAG_ICONS);
  GTK_WIDGET_CLASS (parent_class)->drag_begin (widget, context);

  if (use_icons)
    {
      GtkDTreeNode *node;

      GTK_CLIST_SET_FLAG (clist, CLIST_USE_DRAG_ICONS);
      node = GTK_DTREE_NODE (g_list_nth (clist->row_list,
					 clist->click_cell.row));
      if (node)
	{
	  if (GTK_CELL_PIXTEXT
	      (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap)
	    {
	      gtk_drag_set_icon_pixmap
		(context,
		 gtk_widget_get_colormap (widget),
		 GTK_CELL_PIXTEXT
		 (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->pixmap,
		 GTK_CELL_PIXTEXT
		 (GTK_DTREE_ROW (node)->row.cell[dtree->tree_column])->mask,
		 -2, -2);
	      return;
	    }
	}
      gtk_drag_set_icon_default (context);
    }
}

static gint
gtk_dtree_drag_motion (GtkWidget      *widget,
		       GdkDragContext *context,
		       gint            x,
		       gint            y,
		       guint           time)
{
  GtkCList *clist;
  GtkDTree *dtree;
  GtkCListDestInfo new_info;
  GtkCListDestInfo *dest_info;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DTREE (widget), FALSE);

  clist = GTK_CLIST (widget);
  dtree = GTK_DTREE (widget);

  dest_info = g_dataset_get_data (context, "gtk-clist-drag-dest");

  if (!dest_info)
    {
      dest_info = g_new (GtkCListDestInfo, 1);
	  
      dest_info->cell.row    = -1;
      dest_info->cell.column = -1;
      dest_info->insert_pos  = GTK_CLIST_DRAG_NONE;

      g_dataset_set_data_full (context, "gtk-clist-drag-dest", dest_info,
			       drag_dest_info_destroy);
    }

  drag_dest_cell (clist, x, y, &new_info);

  if (GTK_CLIST_REORDERABLE (clist))
    {
      GList *list;
      GdkAtom atom = gdk_atom_intern ("gtk-clist-drag-reorder", FALSE);

      list = context->targets;
      while (list)
	{
	  if (atom == (GdkAtom)GPOINTER_TO_INT (list->data))
	    break;
	  list = list->next;
	}

      if (list)
	{
	  GtkDTreeNode *drag_source;
	  GtkDTreeNode *drag_target;

	  drag_source = GTK_DTREE_NODE (g_list_nth (clist->row_list,
						    clist->click_cell.row));
	  drag_target = GTK_DTREE_NODE (g_list_nth (clist->row_list,
						    new_info.cell.row));

	  if (gtk_drag_get_source_widget (context) != widget ||
	      !check_drag (dtree, drag_source, drag_target,
			   new_info.insert_pos))
	    {
	      if (dest_info->cell.row < 0)
		{
		  gdk_drag_status (context, GDK_ACTION_DEFAULT, time);
		  return FALSE;
		}
	      return TRUE;
	    }

	  if (new_info.cell.row != dest_info->cell.row ||
	      (new_info.cell.row == dest_info->cell.row &&
	       dest_info->insert_pos != new_info.insert_pos))
	    {
	      if (dest_info->cell.row >= 0)
		GTK_CLIST_CLASS_FW (clist)->draw_drag_highlight
		  (clist,
		   g_list_nth (clist->row_list, dest_info->cell.row)->data,
		   dest_info->cell.row, dest_info->insert_pos);

	      dest_info->insert_pos  = new_info.insert_pos;
	      dest_info->cell.row    = new_info.cell.row;
	      dest_info->cell.column = new_info.cell.column;

	      GTK_CLIST_CLASS_FW (clist)->draw_drag_highlight
		(clist,
		 g_list_nth (clist->row_list, dest_info->cell.row)->data,
		 dest_info->cell.row, dest_info->insert_pos);

	      gdk_drag_status (context, context->suggested_action, time);
	    }
	  return TRUE;
	}
    }

  dest_info->insert_pos  = new_info.insert_pos;
  dest_info->cell.row    = new_info.cell.row;
  dest_info->cell.column = new_info.cell.column;
  return TRUE;
}

static void
gtk_dtree_drag_data_received (GtkWidget        *widget,
			      GdkDragContext   *context,
			      gint              x,
			      gint              y,
			      GtkSelectionData *selection_data,
			      guint             info,
			      guint32           time)
{
  GtkDTree *dtree;
  GtkCList *clist;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DTREE (widget));
  g_return_if_fail (context != NULL);
  g_return_if_fail (selection_data != NULL);

  dtree = GTK_DTREE (widget);
  clist = GTK_CLIST (widget);

  if (GTK_CLIST_REORDERABLE (clist) &&
      gtk_drag_get_source_widget (context) == widget &&
      selection_data->target ==
      gdk_atom_intern ("gtk-clist-drag-reorder", FALSE) &&
      selection_data->format == GTK_TYPE_POINTER &&
      selection_data->length == sizeof (GtkCListCellInfo))
    {
      GtkCListCellInfo *source_info;

      source_info = (GtkCListCellInfo *)(selection_data->data);
      if (source_info)
	{
	  GtkCListDestInfo dest_info;
	  GtkDTreeNode *source_node;
	  GtkDTreeNode *dest_node;

	  drag_dest_cell (clist, x, y, &dest_info);
	  
	  source_node = GTK_DTREE_NODE (g_list_nth (clist->row_list,
						    source_info->row));
	  dest_node = GTK_DTREE_NODE (g_list_nth (clist->row_list,
						  dest_info.cell.row));

	  if (!source_node || !dest_node)
	    return;

	  switch (dest_info.insert_pos)
	    {
	    case GTK_CLIST_DRAG_NONE:
	      break;
	    case GTK_CLIST_DRAG_INTO:
	      if (check_drag (dtree, source_node, dest_node,
			      dest_info.insert_pos))
		gtk_dtree_move (dtree, source_node, dest_node,
				GTK_DTREE_ROW (dest_node)->children.head);
	      g_dataset_remove_data (context, "gtk-clist-drag-dest");
	      break;
	    case GTK_CLIST_DRAG_BEFORE:
	      if (check_drag (dtree, source_node, dest_node,
			      dest_info.insert_pos))
		gtk_dtree_move (dtree, source_node,
				GTK_DTREE_ROW (dest_node)->parent, dest_node);
	      g_dataset_remove_data (context, "gtk-clist-drag-dest");
	      break;
	    case GTK_CLIST_DRAG_AFTER:
	      if (check_drag (dtree, source_node, dest_node,
			      dest_info.insert_pos))
		gtk_dtree_move (dtree, source_node,
				GTK_DTREE_ROW (dest_node)->parent, 
				GTK_DTREE_ROW (dest_node)->sibling.next);
	      g_dataset_remove_data (context, "gtk-clist-drag-dest");
	      break;
	    }
	}
    }
}
