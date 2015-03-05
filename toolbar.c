#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

// struct for widgets included for a single line in the memory image
typedef struct _gtkMemoryLine
{
    GtkWidget *label_arrow;
	GtkWidget *button_breakpoint;
	GtkWidget *label_addr;
	GtkWidget *label_value;
} gtkMemoryLine;

// hide and display the status bar
void toggle_statusbar(GtkWidget *widget, gpointer statusbar) 
{
  if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
    gtk_widget_show(statusbar);
  } else {
    gtk_widget_hide(statusbar);
  }
}

/* GLOBAL VARIABLES */
// NOTE: Given more time, these should be moved into main() or other
//       functions, and malloc() should be used to store these on the
//       heap.  But for the moment, putting these here to ensure that
//       on toggle or setting of various buttons, etc., the values
//       will be visible.
gtkMemoryLine mem_image[4096];  // would potentially need as many lines 
								// as there are in the PDP-8 memory.

int main (int argc, char* argv[])
{
  // WINDOW ITEMS
  GtkWidget *window;
  GtkWidget *scrolled_window;
  GtkWidget *vtable1;
  GtkWidget *vframe_mem;
  GtkWidget *vtable_mem;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *vbox4;

  // MENU ITEMS
  GtkWidget *menubar;
  GtkWidget *filemenu;
  GtkWidget *file;
  GtkWidget *menu_open;
  GtkWidget *menu_restart;
  GtkWidget *quit;
  GtkWidget *menusep;
  // VIEW MENU ITEMS
  GtkWidget *view;
  GtkWidget *viewmenu;
  GtkWidget *tog_status;
  GtkWidget *tog_statistics;
  GtkWidget *statusbar;

  // TOOLBAR ITEMS
  GtkWidget *toolbar;
  GtkToolItem *toolitem_restart;
  GtkToolItem *toolitem_open;
  GtkToolItem *toolitemsep;
  GtkToolItem *toolitem_start;
  GtkToolItem *toolitem_step;

  GtkAccelGroup *accel_group = NULL;

  gtk_init(&argc, &argv);

  /* MAIN WINDOW SET UP */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "PDP-8 Instruction Set Simulator");
  gtk_window_set_default_size(GTK_WINDOW(window),650,500);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);  

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  /* MENU ITEM INSTANTIATIONS */
  menubar = gtk_menu_bar_new();
  filemenu = gtk_menu_new();
  viewmenu = gtk_menu_new();

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  file = gtk_menu_item_new_with_mnemonic("_File");
  menu_open = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
  menu_restart = gtk_image_menu_item_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, NULL);
  menusep = gtk_separator_menu_item_new();
  quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);

  gtk_widget_add_accelerator(quit, "activate", accel_group, 
      GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 

  view = gtk_menu_item_new_with_mnemonic("_View");
  tog_status = gtk_check_menu_item_new_with_label("View Status Bar");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tog_status), TRUE);
  tog_statistics = gtk_check_menu_item_new_with_label("View Statistics");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tog_statistics), TRUE);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), filemenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), menu_open);
  gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), menu_restart);
  gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), menusep);
  gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quit);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
  
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(view), viewmenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewmenu), tog_status);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewmenu), tog_statistics);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view);
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 1);

  /* TOOLBAR INSTANTIATIONS */
  toolbar = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

  gtk_container_set_border_width(GTK_CONTAINER(toolbar), 2);

  toolitem_restart = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_restart, -1);

  toolitem_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_open, -1);
  
  toolitemsep = gtk_separator_tool_item_new();
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitemsep, -1); 

  toolitem_start = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_LAST);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_start, -1);

  toolitem_step = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_step, -1); 

  gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
  
  // create vbox2 to contain updated contents on execution of the simulator
  vbox2 = gtk_vbox_new(FALSE, 0);
  // recall that parameters 3 - 5 are for: expand, fill, padding.
  gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 0);
  
  // create vtable1 to contain the register values on the left hand side and 
  // the memory table (scrolled window) on the right hand side.
  vtable1 = gtk_table_new(4, 4, TRUE);
  gtk_table_set_row_spacings(GTK_TABLE(vtable1), 2);
  gtk_table_set_col_spacings(GTK_TABLE(vtable1), 2);
  gtk_box_pack_start(GTK_BOX(vbox2), vtable1, TRUE, TRUE, 1);
  gtk_widget_show(vtable1);
  
  // create vbox3 to contain the memory section
  vbox3 = gtk_vbox_new(FALSE, 0);
  // recall that parameters 3 - 5 are for: expand, fill, padding.
  //gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, TRUE, 1);
  // (left col, right col, top row, bottom row, xoptions, yoptions, xpadding, ypadding)
  gtk_table_attach(GTK_TABLE(vtable1),vbox3, 2, 4, 0, 4, 0, 0, 2, 2);
  
  /* MEMORY DATA FRAME */
  vframe_mem = gtk_frame_new(NULL);
  // set label of the frame
  gtk_frame_set_label(GTK_FRAME(vframe_mem),"Memory");
  // left-align the label
  gtk_frame_set_label_align(GTK_FRAME(vframe_mem), 0.0, 0.0);
  // set the style of the frame
  gtk_frame_set_shadow_type(GTK_FRAME(vframe_mem),GTK_SHADOW_ETCHED_OUT);
  //gtk_box_pack_start(GTK_BOX(vbox3), vframe_mem, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(vbox3),vframe_mem);
  gtk_widget_show(vframe_mem);
  
  // create vbox4 to contain the memory scrolled window and its header
  vbox4 = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(vframe_mem),vbox4);
  
  /* MEMORY TABLE SET UP */
  vtable_mem = gtk_table_new(4, 4, TRUE);
  gtk_table_set_row_spacings(GTK_TABLE(vtable_mem), 2);
  gtk_table_set_col_spacings(GTK_TABLE(vtable_mem), 2);
  
  /* SCROLLED WINDOW INSTANTIATION */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);  
  gtk_container_border_width (GTK_CONTAINER (scrolled_window), 10);
  gtk_widget_set_usize(scrolled_window, 300, 400);
  // make horizontal scrollbar policy automatic and 
  // make the vertical scrollbar policy always on.
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  //gtk_container_add(GTK_CONTAINER(window),scrolled_window);
  gtk_box_pack_start(GTK_BOX(vbox4), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show(scrolled_window);
  
  //gtk_container_add(GTK_CONTAINER(scrolled_window), vtable_mem);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vtable_mem);
  gtk_widget_show(vtable_mem);
  
  /* STATUS BAR INSTANTIATION */
  statusbar = gtk_statusbar_new();
  gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, TRUE, 1);

  
  /* SIGNAL CONNECTS */
  
  g_signal_connect_swapped(G_OBJECT(window), "destroy",
      G_CALLBACK(gtk_main_quit), NULL);

  g_signal_connect(G_OBJECT(tog_status), "activate", G_CALLBACK(toggle_statusbar), statusbar);

  g_signal_connect(G_OBJECT(quit), "activate",
      G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}

