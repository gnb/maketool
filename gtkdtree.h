/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball, Josh MacDonald
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

#ifndef __GTK_DTREE_H__
#define __GTK_DTREE_H__

#include <gtk/gtkclist.h>
#include <gtk/gtkctree.h>

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

#define GTK_TYPE_DTREE            (gtk_dtree_get_type ())
#define GTK_DTREE(obj)            (GTK_CHECK_CAST ((obj), GTK_TYPE_DTREE, GtkDTree))
#define GTK_DTREE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_DTREE, GtkDTreeClass))
#define GTK_IS_DTREE(obj)         (GTK_CHECK_TYPE ((obj), GTK_TYPE_DTREE))
#define GTK_IS_DTREE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DTREE))

#define GTK_DTREE_ROW(_node_) ((GtkDTreeRow *)(((GList *)(_node_))->data))
#define GTK_DTREE_NODE(_node_) ((GtkDTreeNode *)((_node_)))
#define GTK_DTREE_NODE_NEXT(_nnode_) ((GtkDTreeNode *)(((GList *)(_nnode_))->next))
#define GTK_DTREE_NODE_PREV(_pnode_) ((GtkDTreeNode *)(((GList *)(_pnode_))->prev))
#define GTK_DTREE_FUNC(_func_) ((GtkDTreeFunc)(_func_))

typedef enum
{
  GTK_DTREE_POS_BEFORE,
  GTK_DTREE_POS_AS_CHILD,
  GTK_DTREE_POS_AFTER
} GtkDTreePos;

typedef enum
{
  GTK_DTREE_LINES_NONE,
  GTK_DTREE_LINES_SOLID,
  GTK_DTREE_LINES_DOTTED,
  GTK_DTREE_LINES_TABBED
} GtkDTreeLineStyle;

typedef enum
{
  GTK_DTREE_EXPANDER_NONE,
  GTK_DTREE_EXPANDER_SQUARE,
  GTK_DTREE_EXPANDER_TRIANGLE,
  GTK_DTREE_EXPANDER_CIRCULAR
} GtkDTreeExpanderStyle;

typedef struct _GtkDTree      GtkDTree;
typedef struct _GtkDTreeClass GtkDTreeClass;
typedef struct _GtkDTreeRow   GtkDTreeRow;
typedef struct _GtkDTreeNode  GtkDTreeNode;

typedef void (*GtkDTreeFunc) (GtkDTree     *dtree,
			      GtkDTreeNode *node,
			      gpointer      data);

typedef gboolean (*GtkDTreeGNodeFunc) (GtkDTree     *dtree,
                                       guint         depth,
                                       GNode        *gnode,
				       GtkDTreeNode *cnode,
                                       gpointer      data);

typedef gboolean (*GtkDTreeCompareDragFunc) (GtkDTree     *dtree,
                                             GtkDTreeNode *source_node,
                                             GtkDTreeNode *new_parent,
                                             GtkDTreeNode *new_sibling);

struct _GtkDTree
{
  GtkCList clist;
  
  GdkGC *lines_gc;
  
  gint tree_indent;
  gint tree_spacing;
  gint tree_column;

  guint line_style     : 2;
  guint expander_style : 2;
  guint show_stub      : 1;

  GtkDTreeCompareDragFunc drag_compare;
};

struct _GtkDTreeClass
{
  GtkCListClass parent_class;
  
  void (*tree_select_row)   (GtkDTree     *dtree,
			     GtkDTreeNode *row,
			     gint          column);
  void (*tree_unselect_row) (GtkDTree     *dtree,
			     GtkDTreeNode *row,
			     gint          column);
  void (*tree_expand)       (GtkDTree     *dtree,
			     GtkDTreeNode *node);
  void (*tree_collapse)     (GtkDTree     *dtree,
			     GtkDTreeNode *node);
  void (*tree_move)         (GtkDTree     *dtree,
			     GtkDTreeNode *node,
			     GtkDTreeNode *new_parent,
			     GtkDTreeNode *new_sibling);
  void (*change_focus_row_expansion) (GtkDTree *dtree,
				      GtkCTreeExpansionType action);
};

struct _GtkDTreeRow
{
  GtkCListRow row;
  
  GtkDTreeNode *parent;
  struct {
      GtkDTreeNode *next, *prev;
  } sibling;
  struct {
      GtkDTreeNode *head, *tail;
  } children;
  
  GdkPixmap *pixmap_closed;
  GdkBitmap *mask_closed;
  GdkPixmap *pixmap_opened;
  GdkBitmap *mask_opened;
  
  guint16 level;
  
  guint is_leaf  : 1;
  guint expanded : 1;
};

struct _GtkDTreeNode {
  GList list;
};


/***********************************************************
 *           Creation, insertion, deletion                 *
 ***********************************************************/

GtkType gtk_dtree_get_type                       (void);
void gtk_dtree_construct                         (GtkDTree     *dtree,
						  gint          columns, 
						  gint          tree_column,
						  gchar        *titles[]);
GtkWidget * gtk_dtree_new_with_titles            (gint          columns, 
						  gint          tree_column,
						  gchar        *titles[]);
GtkWidget * gtk_dtree_new                        (gint          columns, 
						  gint          tree_column);
GtkDTreeNode * gtk_dtree_insert_node             (GtkDTree     *dtree,
						  GtkDTreeNode *parent, 
						  GtkDTreeNode *sibling,
						  gchar        *text[],
						  guint8        spacing,
						  GdkPixmap    *pixmap_closed,
						  GdkBitmap    *mask_closed,
						  GdkPixmap    *pixmap_opened,
						  GdkBitmap    *mask_opened,
						  gboolean      is_leaf,
						  gboolean      expanded);
void gtk_dtree_remove_node                       (GtkDTree     *dtree, 
						  GtkDTreeNode *node);
GtkDTreeNode * gtk_dtree_insert_gnode            (GtkDTree          *dtree,
						  GtkDTreeNode      *parent,
						  GtkDTreeNode      *sibling,
						  GNode             *gnode,
						  GtkDTreeGNodeFunc  func,
						  gpointer           data);
GNode * gtk_dtree_export_to_gnode                (GtkDTree          *dtree,
						  GNode             *parent,
						  GNode             *sibling,
						  GtkDTreeNode      *node,
						  GtkDTreeGNodeFunc  func,
						  gpointer           data);

/***********************************************************
 *  Generic recursive functions, querying / finding tree   *
 *  information                                            *
 ***********************************************************/

void gtk_dtree_post_recursive                    (GtkDTree     *dtree, 
						  GtkDTreeNode *node,
						  GtkDTreeFunc  func,
						  gpointer      data);
void gtk_dtree_post_recursive_to_depth           (GtkDTree     *dtree, 
						  GtkDTreeNode *node,
						  gint          depth,
						  GtkDTreeFunc  func,
						  gpointer      data);
void gtk_dtree_pre_recursive                     (GtkDTree     *dtree, 
						  GtkDTreeNode *node,
						  GtkDTreeFunc  func,
						  gpointer      data);
void gtk_dtree_pre_recursive_to_depth            (GtkDTree     *dtree, 
						  GtkDTreeNode *node,
						  gint          depth,
						  GtkDTreeFunc  func,
						  gpointer      data);
gboolean gtk_dtree_is_viewable                   (GtkDTree     *dtree, 
					          GtkDTreeNode *node);
GtkDTreeNode * gtk_dtree_last                    (GtkDTree     *dtree,
					          GtkDTreeNode *node);
GtkDTreeNode * gtk_dtree_find_node_ptr           (GtkDTree     *dtree,
					          GtkDTreeRow  *dtree_row);
GtkDTreeNode * gtk_dtree_node_nth                (GtkDTree     *dtree,
						  guint         row);
gboolean gtk_dtree_find                          (GtkDTree     *dtree,
					          GtkDTreeNode *node,
					          GtkDTreeNode *child);
gboolean gtk_dtree_is_ancestor                   (GtkDTree     *dtree,
					          GtkDTreeNode *node,
					          GtkDTreeNode *child);
GtkDTreeNode * gtk_dtree_find_by_row_data        (GtkDTree     *dtree,
					          GtkDTreeNode *node,
					          gpointer      data);
/* returns a GList of all GtkDTreeNodes with row->data == data. */
GList * gtk_dtree_find_all_by_row_data           (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gpointer      data);
GtkDTreeNode * gtk_dtree_find_by_row_data_custom (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gpointer      data,
						  GCompareFunc  func);
/* returns a GList of all GtkDTreeNodes with row->data == data. */
GList * gtk_dtree_find_all_by_row_data_custom    (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gpointer      data,
						  GCompareFunc  func);
gboolean gtk_dtree_is_hot_spot                   (GtkDTree     *dtree,
					          gint          x,
					          gint          y);

/***********************************************************
 *   Tree signals : move, expand, collapse, (un)select     *
 ***********************************************************/

void gtk_dtree_move                              (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  GtkDTreeNode *new_parent, 
						  GtkDTreeNode *new_sibling);
void gtk_dtree_expand                            (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_expand_recursive                  (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_expand_to_depth                   (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          depth);
void gtk_dtree_collapse                          (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_collapse_recursive                (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_collapse_to_depth                 (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          depth);
void gtk_dtree_toggle_expansion                  (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_toggle_expansion_recursive        (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_select                            (GtkDTree     *dtree, 
						  GtkDTreeNode *node);
void gtk_dtree_select_recursive                  (GtkDTree     *dtree, 
						  GtkDTreeNode *node);
void gtk_dtree_unselect                          (GtkDTree     *dtree, 
						  GtkDTreeNode *node);
void gtk_dtree_unselect_recursive                (GtkDTree     *dtree, 
						  GtkDTreeNode *node);
void gtk_dtree_real_select_recursive             (GtkDTree     *dtree, 
						  GtkDTreeNode *node, 
						  gint          state);

/***********************************************************
 *           Analogons of GtkCList functions               *
 ***********************************************************/

void gtk_dtree_node_set_text                     (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  const gchar  *text);
void gtk_dtree_node_set_pixmap                   (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  GdkPixmap    *pixmap,
						  GdkBitmap    *mask);
void gtk_dtree_node_set_pixtext                  (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  const gchar  *text,
						  guint8        spacing,
						  GdkPixmap    *pixmap,
						  GdkBitmap    *mask);
void gtk_dtree_set_node_info                     (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  const gchar  *text,
						  guint8        spacing,
						  GdkPixmap    *pixmap_closed,
						  GdkBitmap    *mask_closed,
						  GdkPixmap    *pixmap_opened,
						  GdkBitmap    *mask_opened,
						  gboolean      is_leaf,
						  gboolean      expanded);
void gtk_dtree_node_set_shift                    (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  gint          vertical,
						  gint          horizontal);
void gtk_dtree_node_set_selectable               (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gboolean      selectable);
gboolean gtk_dtree_node_get_selectable           (GtkDTree     *dtree,
						  GtkDTreeNode *node);
GtkCellType gtk_dtree_node_get_cell_type         (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column);
gint gtk_dtree_node_get_text                     (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  gchar       **text);
gint gtk_dtree_node_get_pixmap                   (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  GdkPixmap   **pixmap,
						  GdkBitmap   **mask);
gint gtk_dtree_node_get_pixtext                  (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  gchar       **text,
						  guint8       *spacing,
						  GdkPixmap   **pixmap,
						  GdkBitmap   **mask);
gint gtk_dtree_get_node_info                     (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gchar       **text,
						  guint8       *spacing,
						  GdkPixmap   **pixmap_closed,
						  GdkBitmap   **mask_closed,
						  GdkPixmap   **pixmap_opened,
						  GdkBitmap   **mask_opened,
						  gboolean     *is_leaf,
						  gboolean     *expanded);
void gtk_dtree_node_set_row_style                (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  GtkStyle     *style);
GtkStyle * gtk_dtree_node_get_row_style          (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_node_set_cell_style               (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  GtkStyle     *style);
GtkStyle * gtk_dtree_node_get_cell_style         (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column);
void gtk_dtree_node_set_foreground               (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  GdkColor     *color);
void gtk_dtree_node_set_background               (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  GdkColor     *color);
void gtk_dtree_node_set_row_data                 (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gpointer      data);
void gtk_dtree_node_set_row_data_full            (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gpointer      data,
						  GtkDestroyNotify destroy);
gpointer gtk_dtree_node_get_row_data             (GtkDTree     *dtree,
						  GtkDTreeNode *node);
void gtk_dtree_node_moveto                       (GtkDTree     *dtree,
						  GtkDTreeNode *node,
						  gint          column,
						  gfloat        row_align,
						  gfloat        col_align);
GtkVisibility gtk_dtree_node_is_visible          (GtkDTree     *dtree,
						  GtkDTreeNode *node);

/***********************************************************
 *             GtkDTree specific functions                 *
 ***********************************************************/

void gtk_dtree_set_indent            (GtkDTree                *dtree, 
				      gint                     indent);
void gtk_dtree_set_spacing           (GtkDTree                *dtree, 
				      gint                     spacing);
void gtk_dtree_set_show_stub         (GtkDTree                *dtree, 
				      gboolean                 show_stub);
void gtk_dtree_set_line_style        (GtkDTree                *dtree, 
				      GtkDTreeLineStyle        line_style);
void gtk_dtree_set_expander_style    (GtkDTree                *dtree, 
				      GtkDTreeExpanderStyle    expander_style);
void gtk_dtree_set_drag_compare_func (GtkDTree     	      *dtree,
				      GtkDTreeCompareDragFunc  cmp_func);

/***********************************************************
 *             Tree sorting functions                      *
 ***********************************************************/

void gtk_dtree_sort_node                         (GtkDTree     *dtree, 
						  GtkDTreeNode *node);
void gtk_dtree_sort_recursive                    (GtkDTree     *dtree, 
						  GtkDTreeNode *node);



#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GTK_DTREE_H__ */
