/***********************************************************/
/* Title: ECE 586 Project: PDP-8 Instruction Set Simulator */
/* Date: February 20th, 2015                               */
/***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "ece586_proj.h"

//=========================================================
// PARAMETER SECTION
//=========================================================
// DEFAULT STARTING ADDRESS: (in decimal)
#define DEFAULT_STARTING_ADDRESS 128

// TRACE FILE NAMES:
#define PARAM_FILE_NAME "param.txt"
#define TRACE_FILE_NAME "trace.txt"
#define BRANCH_FILE_NAME "branch_trace.txt"
//---------------------------	
// END PARAMETER SECTION
//=========================================================

void execute_instructions();
void display_memory_array( s_mem_word* ptr_mem_array, GtkWidget* vtable);

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

// set/remove breakpoints
void toggle_breakpoint_button_callback (GtkWidget *widget, gpointer data)
{
    if (GTK_TOGGLE_BUTTON (widget)->active) 
    {
        /* If control reaches here, the toggle button is down */
    
    } else {
    
        /* If control reaches here, the toggle button is up */
    }
}

/* GLOBAL VARIABLES */
// NOTE: Given more time, these should be moved into main() or other
//       functions, and malloc() should be used to store these on the
//       heap.  But for the moment, putting these here to ensure that
//       on toggle or setting of various buttons, etc., the values
//       will be visible.
gtkMemoryLine mem_image[PDP8_MEMSIZE];  // would potentially need as many lines 
								// as there are in the PDP-8 memory.
// GLOBAL VARIABLES
s_mem_word mem_array [PDP8_MEMSIZE]; // Memory Space
s_debug_flags debug;				// Debug Flags
s_branch_stats branch_statistics; 	// Branch Statistics tracking

// VARIABLES:
// - Memory and Effective Address:
short int effective_address;	// Effective Address
short int memval_eaddr;			// Value of the word located in memory at the effective address
short int next_memval_eaddr;	// Value to be written to the memory location specified by the effective address
s_effective_addr curr_eaddr;	// Return values of calculated effective address, with flags for statistics.
// - Registers: Current and Next
short int reg_PC = DEFAULT_STARTING_ADDRESS;	// Program Counter [0:11]
short int reg_IR = 0;	// Current Instruction loaded from the PC.
char curr_opcode = 0;	// Current Opcode = IR[0:2]
char reg_LR = 0;		// Link Register [0] (One bit only!)
short int reg_AC = 0;	// Accumulator [0:11]
short int reg_SR = 0;	// Console Switch Register [0:11]
short int next_PC = 0;
short int next_AC = 0;	
short int next_LR = 0;
//short int next_SR = 0;
s_updated_vals next_vals;	// struct that holds return values from modules
// - Flags:
char flag_HLT = 0; 	// halt flag from group 2 microinstructions
//char flag_NOP = 0;	// flag to indicate a NOP for invalid instruction 
char flag_branch_taken = 0;	// set to 1 if a branch was taken
// - Input and Param File:
char* input_filename;	// name of input file - taken from command line
char* param_filename = PARAM_FILE_NAME;	// name of param file - taken from command line
// - Trace Files:
FILE* fp_tracefile;		// main trace file handle
char* trace_filename = TRACE_FILE_NAME;
FILE* fp_branchtrace;	// branch trace file handle
char* branch_filename = BRANCH_FILE_NAME;
// - Masks
short int address_mask = (1 << PDP8_ADDR_SIZE) - 1;
short int word_mask = (1 << PDP8_WORD_SIZE) - 1;
// - Statistics
int stat_instructions = 0;
int stat_clocks = 0;
int stat_and = 0;
int stat_tad = 0;
int stat_isz = 0;
int stat_dca = 0;
int stat_jms = 0;
int stat_jmp = 0;
int stat_io = 0;
int stat_ui = 0;

int main (int argc, char* argv[])
{
  int i;
  
  // CHECK COMMAND LINE ARGUMENTS:
  if (argc < 2) {
  fprintf (stderr, "Usage: <command> <inputfile>\n");
  fprintf (stderr, "Usage: <command> <inputfile> <switch_register>\n");
  exit (1);
  }
	
  // WINDOW ITEMS
  GtkWidget *window;
  GtkWidget *scrolled_window;
  GtkWidget *vtable1;
  GtkWidget *vframe_mem;
  GtkWidget *vtable_mem_header;
  GtkWidget *mem_separator;
  GtkWidget *lbl_vtable_mem_header_addr;
  GtkWidget *lbl_vtable_mem_header_value;
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
  
  char buffer[1024];

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
  gtk_widget_show_all(vbox);
  //gtk_widget_show(vbox);
  
  // create vbox2 to contain updated contents on execution of the simulator
  vbox2 = gtk_vbox_new(FALSE, 0);
  // recall that parameters 3 - 5 are for: expand, fill, padding.
  gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show(vbox2);
  
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
  gtk_widget_show(vbox3);
  
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
  gtk_widget_show(vbox4);
  
  /* MEMORY TABLE HEADER SET UP */
  // create table header labels
  lbl_vtable_mem_header_addr = gtk_label_new("\nAddress");
  lbl_vtable_mem_header_value = gtk_label_new("Value in Octal\n[Hexadecimal]");
  mem_separator = gtk_hseparator_new ();
  /*
  vtable_mem_header = gtk_table_new(4, 4, TRUE);
  gtk_table_set_row_spacings(GTK_TABLE(vtable_mem_header), 0);
  gtk_table_set_col_spacings(GTK_TABLE(vtable_mem_header), 0);
  // attach labels to the header table
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem_header), lbl_vtable_mem_header_addr, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem_header), lbl_vtable_mem_header_value, 2, 3, 0, 1);
  gtk_widget_show_all(vtable_mem_header);
  // add header table to vbox4
  gtk_box_pack_start(GTK_BOX(vbox4), vtable_mem_header, FALSE, FALSE, 0);
  */
  
  /* MEMORY TABLE SET UP */
  vtable_mem = gtk_table_new(PDP8_MEMSIZE+1, 4, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(vtable_mem), 1);
  gtk_table_set_col_spacings(GTK_TABLE(vtable_mem), 1);
  
  /* SCROLLED WINDOW INSTANTIATION */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);  
  gtk_container_border_width (GTK_CONTAINER (scrolled_window), 1);
  gtk_widget_set_usize(scrolled_window, 400, 400);
  // make horizontal scrollbar policy automatic and 
  // make the vertical scrollbar policy always on.
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  //gtk_container_add(GTK_CONTAINER(window),scrolled_window);
  gtk_box_pack_start(GTK_BOX(vbox4), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show(scrolled_window);
  
  // attach column labels and separator to the table
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_addr, 2, 3, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_value, 3, 4, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_separator, 0, 4, 1, 2);
  gtk_widget_show(lbl_vtable_mem_header_addr);
  gtk_widget_show(lbl_vtable_mem_header_value);
  gtk_widget_show(mem_separator);
  
  //gtk_container_add(GTK_CONTAINER(scrolled_window), vtable_mem);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vtable_mem);
  gtk_widget_show(vtable_mem);
  
  // Create all the labels and buttons for the memory array, 
  // but do not display them yet.
  for (i = 0; i < PDP8_MEMSIZE; i++)
  {
	  //sprintf (buffer, "cell (%d,%d)\n", i, j);

	  mem_image[i].label_arrow = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
	  mem_image[i].button_breakpoint = gtk_toggle_button_new();
	  mem_image[i].label_addr = gtk_label_new("");
	  mem_image[i].label_value = gtk_label_new("");
	  
	  // attach widgets to the table
	//  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_arrow, 0, 1, i, i+1);
	//  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].button_breakpoint, 1, 2, i, i+1);
	//  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_addr, 2, 3, i, i+1);
	//  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_value, 3, 4, i, i+1);

	  // attach "clicked" signal for breakpoint button
	  //gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_breakpoint_button_callback), (gpointer) "memory_index");
	  
	  // show elements:
	  //gtk_widget_show (mem_image[i].label_arrow);
	  //gtk_widget_show (mem_image[i].button_breakpoint);
	  //gtk_widget_show (mem_image[i].label_addr);
	  //gtk_widget_show (mem_image[i].label_value);
	  
	  // to change the label's text after creation:
	  //gtk_label_set_text( (GtkLabel) mem_image[i].label_addr, str_address );
	  //gtk_label_set_text( (GtkLabel) mem_image[i].label_value, str_value );
  }
  
  // Resize table afterwards [rows, cols]
  gtk_table_resize (GTK_TABLE(vtable_mem),3,4);
  
  /* STATUS BAR INSTANTIATION */
  statusbar = gtk_statusbar_new();
  gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, TRUE, 1);

  
  /* SIGNAL CONNECTS */
  
  g_signal_connect_swapped(G_OBJECT(window), "destroy",
      G_CALLBACK(gtk_main_quit), NULL);

  g_signal_connect(G_OBJECT(tog_status), "activate", G_CALLBACK(toggle_statusbar), statusbar);

  g_signal_connect(G_OBJECT(quit), "activate",
      G_CALLBACK(gtk_main_quit), NULL);
	  
  // store input filename
  input_filename = argv[1];
  // if more than 2 arguments given, third argument is the Switch Register value
  if (argc > 2) reg_SR = atoi(argv[2]); 

  // Read in debug flag settings from param.txt.
  read_param_file(&debug, param_filename);

  // Read in memory from input file.
  load_memory_array(&mem_array[0], input_filename, debug.mem_display);
  
  // Display loaded memory in the GUI.
  display_memory_array(&mem_array[0], vtable_mem);

  //gtk_widget_show_all(window);
  gtk_widget_show(window);

  gtk_main();

  return 0;
}

void display_memory_array( s_mem_word* ptr_mem_array, GtkWidget* vtable) {
	int i;	// loop counter
	int k; // display index
	char curr_addr[50]; // address location to write the current data.
	char curr_data[50]; // current data to be written.
	
	// set table headers
//	GtkWidget* lbl_header_addr = gtk_label_new("\nAddress");
//	GtkWidget* lbl_header_value = gtk_label_new("Value in Octal\n[Hexadecimal]");
	
	// attach column labels to the table
//	gtk_table_attach_defaults (GTK_TABLE (vtable), lbl_header_addr, 1, 2, 0, 1);
//	gtk_table_attach_defaults (GTK_TABLE (vtable), lbl_header_value, 2, 3, 0, 1);
	
	// start populating table at the 2nd row
	k = 2;
	
	// Resize table initially [rows, cols]
	gtk_table_resize (GTK_TABLE(vtable),PDP8_MEMSIZE+2,4);
	
	// display any lines that are valid
	for(i = 0; i <= MEM_ARRAY_MAX; i=i+1) {
		if((ptr_mem_array+i)->valid == 1) {
			//curr_addr = i;
			//curr_data = (ptr_mem_array+curr_addr)->value;
			
			sprintf(curr_addr, "%04o [%03x]", i, i);
			sprintf(curr_data, "%04o [%03x]", (ptr_mem_array+i)->value, (ptr_mem_array+i)->value);
			
			// attach widgets to the table
			gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_arrow, 0, 1, k, k+1);
			gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].button_breakpoint, 1, 2, k, k+1);
			gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_addr, 2, 3, k, k+1);
			gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_value, 3, 4, k, k+1);

			// attach "clicked" signal for breakpoint button
			gtk_signal_connect (GTK_OBJECT (mem_image[k].button_breakpoint), "clicked", GTK_SIGNAL_FUNC (toggle_breakpoint_button_callback), (gpointer) curr_addr);
			  
			// show elements:
			if (reg_PC == i) {
				// Display arrow for next instruction to be executed
				gtk_widget_show (mem_image[k].label_arrow);
			}
			gtk_widget_show (mem_image[k].button_breakpoint);
			gtk_widget_show (mem_image[k].label_addr);
			gtk_widget_show (mem_image[k].label_value);
			  
			// to change the label's text after creation:
			gtk_label_set_text( (GtkLabel*) mem_image[k].label_addr, curr_addr );
			gtk_label_set_text( (GtkLabel*) mem_image[k].label_value, curr_data );
			
			k++; // increment the display index
		}
	}
	
	// Resize table finally [rows, cols]
	gtk_table_resize (GTK_TABLE(vtable),k,4);
}

void execute_instructions(int flag_step) {
	// - Debug Print Strings:
	// - Debug Flags:
	debug.mem_display = 0;
	debug.instr = 0;
	debug.module = 0;
	debug.eaddr = 0;
	debug.short_mode = 0;
	// Effective Address
	curr_eaddr.EAddr = 0;

	// Initialize and allocate space for debug print strings
	char* curr_opcode_str = malloc((DEBUG_STR_LEN*sizeof(char))+1);
	if (curr_opcode_str == NULL) {
		fprintf(stderr,"malloc() for curr_opcode_str failed in main()\n");
		exit(-1);
	}
	char* bin_str_PC = malloc((PDP8_ADDR_SIZE*sizeof(char))+1);
	if (bin_str_PC == NULL) {
		fprintf(stderr,"malloc() for bin_str_PC failed in main()\n");
		exit(-1);
	}
	char* bin_str_next_PC = malloc((PDP8_ADDR_SIZE*sizeof(char))+1);
	if (bin_str_next_PC == NULL) {
		fprintf(stderr,"malloc() for bin_str_next_PC failed in main()\n");
		exit(-1);
	}
	char* bin_str_IR = malloc((PDP8_WORD_SIZE*sizeof(char))+1);
	if (bin_str_IR == NULL) {
		fprintf(stderr,"malloc() for bin_str_IR failed in main()\n");
		exit(-1);
	}
	char* bin_str_AC = malloc((PDP8_WORD_SIZE*sizeof(char))+1);
	if (bin_str_AC == NULL) {
		fprintf(stderr,"malloc() for bin_str_AC failed in main()\n");
		exit(-1);
	}
	char* bin_str_next_AC = malloc((PDP8_WORD_SIZE*sizeof(char))+1);
	if (bin_str_next_AC == NULL) {
		fprintf(stderr,"malloc() for bin_str_next_AC failed in main()\n");
		exit(-1);
	}
	char* bin_str_LR = malloc((PDP8_LR_SIZE*sizeof(char))+1);
	if (bin_str_LR == NULL) {
		fprintf(stderr,"malloc() for bin_str_LR failed in main()\n");
		exit(-1);
	}
	char* bin_str_next_LR = malloc((PDP8_LR_SIZE*sizeof(char))+1);
	if (bin_str_next_LR == NULL) {
		fprintf(stderr,"malloc() for bin_str_next_LR failed in main()\n");
		exit(-1);
	}
	char* bin_str_SR = malloc((PDP8_WORD_SIZE*sizeof(char))+1);
	if (bin_str_SR == NULL) {
		fprintf(stderr,"malloc() for bin_str_SR failed in main()\n");
		exit(-1);
	}
	char* bin_str_EAddr = malloc((PDP8_ADDR_SIZE*sizeof(char))+1);
	if (bin_str_EAddr == NULL) {
		fprintf(stderr,"malloc() for bin_str_EAddr failed in main()\n");
		exit(-1);
	}
	char* bin_str_next_EAddr = malloc((PDP8_ADDR_SIZE*sizeof(char))+1);
	if (bin_str_next_EAddr == NULL) {
		fprintf(stderr,"malloc() for bin_str_next_EAddr failed in main()\n");
		exit(-1);
	}
	char* bin_str_memval_EAddr = malloc((PDP8_WORD_SIZE*sizeof(char))+1);
	if (bin_str_memval_EAddr == NULL) {
		fprintf(stderr,"malloc() for bin_str_memval_EAddr failed in main()\n");
		exit(-1);
	}
	char* bin_str_next_memval_EAddr = malloc((PDP8_WORD_SIZE*sizeof(char))+1);
	if (bin_str_next_memval_EAddr == NULL) {
		fprintf(stderr,"malloc() for bin_str_next_memval_EAddr failed in main()\n");
		exit(-1);
	}
	
	// Open the trace and branch trace files.
	if ((fp_tracefile = fopen(trace_filename, "w+")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", trace_filename);
		exit (1);
	}
	fclose(fp_tracefile);
	if ((fp_tracefile = fopen(trace_filename, "a")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", trace_filename);
		exit (1);
	}
	if ((fp_branchtrace = fopen(branch_filename, "w+")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", branch_filename);
		exit (1);
	}
	fclose(fp_branchtrace);
	if ((fp_branchtrace = fopen(branch_filename, "a")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", branch_filename);
		exit (1);
	}
	// Note: Leave trace files open for append.
	
	// print header for branch trace file
	fprintf(fp_branchtrace,"PC [octal]    BRANCH TYPE         TAKEN/NOT TAKEN    TARGET ADDRESS [octal]\n");
	fprintf(fp_branchtrace,"---------------------------------------------------------------------------\n");
	
	//int counter = 0;
	
	//======================================================
	// MAIN LOOP
	//while (!flag_HLT && (counter < 10)) {
	while (!flag_HLT) {
		//counter++;
		effective_address = 0;
		memval_eaddr = 0;
		
		// STEP 1: FETCH INSTRUCTION
		reg_IR = read_mem(reg_PC, TF_FETCH, &mem_array[0], fp_tracefile);
		// set opcode
		curr_opcode = reg_IR >> (PDP8_WORD_SIZE - INSTR_OP_LOW - 1);
		
		// update stat for instruction count
		stat_instructions = stat_instructions + 1;
		
		// Set opcode string.
		switch (curr_opcode) {
			case OP_AND:	strncpy(curr_opcode_str,"AND",DEBUG_STR_N);
							break;
			case OP_TAD:	strncpy(curr_opcode_str,"TAD",DEBUG_STR_N);
							break;
			case OP_ISZ:	strncpy(curr_opcode_str,"ISZ",DEBUG_STR_N);
							break;
			case OP_DCA:	strncpy(curr_opcode_str,"DCA",DEBUG_STR_N);
							break;
			case OP_JMS:	strncpy(curr_opcode_str,"JMS",DEBUG_STR_N);
							break;
			case OP_JMP:	strncpy(curr_opcode_str,"JMP",DEBUG_STR_N);
							break;
			case OP_IO:		strncpy(curr_opcode_str,"IO ",DEBUG_STR_N);
							break;
			case OP_UI:		strncpy(curr_opcode_str,"UI ",DEBUG_STR_N);
							break;
			default:		strncpy(curr_opcode_str,"ILLEGAL OP",DEBUG_STR_N);
							break;
		}
		
		if(debug.instr || debug.short_mode) {
			printf("\n================== INSTRUCTION #%-1d : %s ==================\n",stat_instructions, curr_opcode_str);
		}
		
		// STEP 2: CALCULATE EFFECTIVE ADDRESS IF NEEDED
		if ((curr_opcode >= 0) && (curr_opcode <= 5)) {
			curr_eaddr = calc_eaddr(reg_IR, reg_PC, &mem_array[0], fp_tracefile, debug.eaddr);
			effective_address = curr_eaddr.EAddr;
		}
											
		//--------------------------------
		// DEBUG PRINTS OF INITIAL VALUES
		//--------------------------------
		
		if (debug.instr || debug.short_mode) {
			// Get binary values of initial values
			//printf("   &bin_str_PC: %x\n",(unsigned int)&bin_str_PC);
			int_to_binary_str(reg_PC, PDP8_ADDR_SIZE, (char**)&bin_str_PC);
			int_to_binary_str(reg_IR, PDP8_WORD_SIZE, (char**)&bin_str_IR);
			int_to_binary_str(effective_address, PDP8_ADDR_SIZE, (char**)&bin_str_EAddr);
			//int_to_binary_str(memval_eaddr, PDP8_WORD_SIZE, (char**)&bin_str_memval_EAddr);
			int_to_binary_str(reg_AC, PDP8_WORD_SIZE, (char**)&bin_str_AC);
			int_to_binary_str(reg_LR, 1, (char**)&bin_str_LR);
			int_to_binary_str(reg_SR, PDP8_WORD_SIZE, (char**)&bin_str_SR);
		}
			
		if(debug.instr) {
			//printf("================== INSTRUCTION #%-1d : %s ==================\n",stat_instructions, curr_opcode_str);
			printf("  VALUES BEFORE UPDATE:\n");
			printf("                  PC / IR: %s [%04o] / %s [%04o]\n", bin_str_PC, reg_PC, bin_str_IR, reg_IR);
			printf("        EFFECTIVE ADDRESS: %s [%04o] \n", bin_str_EAddr, effective_address);
			//printf("EFFECTIVE ADDRESS / VALUE: %s [%4o] / %s [%4o]\n", bin_str_EAddr, effective_address, bin_str_memval_EAddr, memval_eaddr);
			printf("       LINK REGISTER [LR]: %1o   ACCUMULATOR [AC]: %s [%04o]\n", reg_LR, bin_str_AC, reg_AC);
			printf("     SWITCH REGISTER [SR]: %s [%04o]\n", bin_str_SR, reg_SR);
			//printf("                    IR: %s [%3x]\n", bin_str_IR,reg_IR);
			//printf("                OPCODE: %12.3s [%3x]\n", curr_opcode_str, curr_opcode);
			//printf("     VALUE @ EFF. ADDR: %s [%3x]\n", bin_str_memval_EAddr, memval_eaddr);
			//printf("    LINK REGISTER [LR]: %12s [%3x]\n", bin_str_LR, reg_LR);
			printf("----------------------------------------------------\n");
		}
		else if(debug.short_mode)	//Shortened version. Only want if the long version is off.  Shows changes caused at current PC by current OP.
		{
			printf("              PC: %s [%04o]\n", bin_str_PC, reg_PC);
			// This would print current values of LR/AC:
			//printf("  %s   LR: %o AC: %s [%4o]\n", curr_opcode_str, reg_LR, bin_str_AC, reg_AC);
			/*
			switch (curr_opcode) {
				case OP_AND:	printf("  AND    LR: %s AC: %s [%3x]\n", reg_LR, next_vals.AC, next_vals.AC);
								break;
				case OP_TAD:  	printf("  TAD    LR: %b AC: %b [%3x]\n", next_vals.LR, next_vals.AC, next_vals.AC);
								break;
				case OP_ISZ:  	printf("  ISZ    LR: %b AC: %b [%3x]\n", reg_LR, reg_AC, reg_AC);
								break;
				case OP_DCA:  	printf("  DCA    LR: %b AC: %b [%3x]\n", reg_LR, next_vals.AC, next_vals.AC);
								break;
				case OP_JMS:  	printf("  JMS    LR: %b AC: %b [%3x]\n", reg_LR, reg_AC, reg_AC);
								break;
				case OP_JMP:  	printf("  JMP    LR: %b AC: %b [%3x]\n", reg_LR, reg_AC, reg_AC);
								break;
				case OP_IO:   	printf("  IO     LR: %b AC: %b [%3x]\n", reg_LR, next_vals.AC, next_vals.AC);
								break;
				case OP_UI:   	printf("  UI     LR: %b AC: %b [%3x]\n", next_vals.LR, next_vals.AC, next_vals.AC);
								break;
				default: printf("WARNING! UNKNOWN OP CODE LR: %b AC: %b [%x]\n", reg_LR, reg_AC, reg_AC);
								break;
			}
			*/
		}
		
		// SET NEXT_PC
		next_PC = (reg_PC + 1) & address_mask;
		
		// STEP 4: EXECUTE CURRENT INSTRUCTION
		switch (curr_opcode) {
			case OP_AND:	// first, load the value from memory at the effective address
							memval_eaddr = read_mem(effective_address, TF_READ, &mem_array[0], fp_tracefile);
							// AC = AC AND M[EAddr]
							next_AC = reg_AC & memval_eaddr;
							// Debug Print
							if (debug.module) printf("      CURRENT M[EADDR]: [%04o]\n", memval_eaddr);
							if (debug.module) printf("               NEXT AC: [%04o]\n", next_AC);
							// Update Registers
							reg_AC = next_AC;
							break;
			case OP_TAD:  	// first, load the value from memory at the effective address
							memval_eaddr = read_mem(effective_address, TF_READ, &mem_array[0], fp_tracefile);
							// AC = AC + M[EAddr];
							// LR is inverted if the above addition operation results in carry out
							next_AC = reg_AC + memval_eaddr;
							if ((next_AC >> PDP8_WORD_SIZE) != 0) {
								next_LR = reg_LR ^ 1; // this will flip the least significant bit of the LR.
							}
							next_AC = next_AC & word_mask; 	// then clear higher-order bits in next_AC beyond
															// the size of the word
							// Debug Print
							if (debug.module) printf("            NEXT LR/AC: [%o][%04o]\n", next_LR, next_AC);
							// Update Registers
							reg_AC = next_AC;
							reg_LR = next_LR;
							break;
			case OP_ISZ:  	// first, load the value from memory at the effective address
							memval_eaddr = read_mem(effective_address, TF_READ, &mem_array[0], fp_tracefile);
							flag_branch_taken = 0;	// initialize flag that branch was taken to false.
							next_memval_eaddr = (memval_eaddr + 1) & address_mask; // increment M[EAddr]
							// write updated value to memory
							write_mem(effective_address, next_memval_eaddr, &mem_array[0], fp_tracefile);
							// if updated value == 0, then the next instruction should be skipped
							if (next_memval_eaddr == 0) {
								flag_branch_taken = 1;
								next_PC++; // skip next instruction
							}
							// Debug Print
							if (debug.module) printf("               NEXT PC: [%04o]\n", next_PC);
							if (debug.module) printf("         NEXT M[EAddr]: [%04o]\n", next_memval_eaddr);
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_PC, flag_branch_taken, BT_CONDITIONAL, &branch_statistics);
							// Update Registers
							break;
			case OP_DCA:  	// write AC to memory
							write_mem(effective_address, reg_AC, &mem_array[0], fp_tracefile);
							next_AC = 0;
							// Debug Print
							if (debug.module) printf("               NEXT AC: [%04o]\n", next_AC);
							// Update Registers
							reg_AC = next_AC;
							break;
			case OP_JMS:  	next_memval_eaddr = next_PC;
							next_PC = (effective_address + 1) & address_mask;
							// write updated value to memory
							write_mem(effective_address, next_memval_eaddr, &mem_array[0], fp_tracefile);
							// Debug Print
							if (debug.module) printf("               NEXT PC: [%04o]\n", next_PC);
							if (debug.module) printf("         NEXT M[EAddr]: [%04o]\n", next_memval_eaddr);
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_PC, BT_TAKEN, BT_UNCONDITIONAL, &branch_statistics);
							// Update Registers
							break;
			case OP_JMP:  	next_PC = effective_address;
							// Debug Print
							if (debug.module) printf("               NEXT PC: [%04o]\n", next_PC);
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_PC, BT_TAKEN, BT_UNCONDITIONAL, &branch_statistics);
							// Update Registers
							break;
			case OP_IO:   	// Not implemented.
							fprintf(stderr,"WARNING: IO instruction encountered at PC: [%04o]\n",reg_PC);
							// Update Registers
							break;
			case OP_UI:   	// Obtain next values from the UI subroutine
							// Note that next_PC is passed as an argument here since PC has been pre-incremented.
							next_vals = module_UI(reg_IR, next_PC, reg_AC, reg_LR, reg_SR, debug.module);
							
							// Debug Print
							if (debug.module) {
								printf("               NEXT PC: [%04o]\n", next_vals.PC);
								printf("            NEXT LR/AC: %x/%03x [%o/%04o]\n",next_vals.LR,next_vals.AC,next_vals.LR,next_vals.AC);
							}
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_vals.PC, next_vals.flag_branch_taken, next_vals.flag_branch_type, &branch_statistics);
							next_PC = next_vals.PC;
							// Update Registers
							reg_AC = next_vals.AC;
							reg_LR = next_vals.LR;
							flag_HLT = next_vals.flag_HLT;
							break;
			default: 		fprintf(stderr,"WARNING! UNKNOWN OP CODE: %d LR: %o AC: %03x [%04o]\n", curr_opcode, reg_LR, reg_AC, reg_AC);
							break;
		}
		
		if (debug.short_mode) {
			// get binary string of the updated AC
			int_to_binary_str(reg_AC, PDP8_WORD_SIZE, (char**)&bin_str_AC);
			// print updated values after executing current instruction
			printf("  %s   LR: %o AC: %s [%04o] AFTER EXECUTION\n", curr_opcode_str, reg_LR, bin_str_AC, reg_AC);
			printf("----------------------------------------------------------\n");
		}
		
		// STEP 5: UPDATE THE PROGRAM COUNTER (PC)
		reg_PC = next_PC;
		
		// STEP 6: UPDATE STATS
		switch (curr_opcode) {
			case OP_AND:	stat_and = stat_and + 1;
							stat_clocks = stat_clocks + 2;	
							if (curr_eaddr.flag_MemType_AutoIndex)
								stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_Indirect)
								stat_clocks = stat_clocks + 1;
							break;
			case OP_TAD:  	stat_tad = stat_tad + 1;
							stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_AutoIndex)
								stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_Indirect)
								stat_clocks = stat_clocks + 1;
							break;
			case OP_ISZ:  	stat_isz = stat_isz + 1;
							stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_AutoIndex)
								stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_Indirect)
								stat_clocks = stat_clocks + 1;
							break;
			case OP_DCA:  	stat_dca = stat_dca + 1;
							stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_AutoIndex)
								stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_Indirect)
								stat_clocks = stat_clocks + 1;
							break;
			case OP_JMS:  	stat_jms = stat_jms + 1;
							stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_AutoIndex)
								stat_clocks = stat_clocks + 2;
							if (curr_eaddr.flag_MemType_Indirect)
								stat_clocks = stat_clocks + 1;
							break;
			case OP_JMP:  	stat_jmp = stat_jmp + 1;
							stat_clocks = stat_clocks + 1;	
							break;
			case OP_IO:   	stat_io = stat_io + 1;
							break;
			case OP_UI:   	stat_ui = stat_ui + 1;
							stat_clocks = stat_clocks + 1;
							break;
			default: printf("WARNING! UNKNOWN OP CODE LR: %o AC: %03x [%04o]\n", reg_LR, reg_AC, reg_AC);
							break;
		}
	}
	
	// END MAIN LOOP
	//======================================================
	
	
	//======================================================
	// PRINT OUT STATISTICS AND MEMORY IMAGE
	//----------------------------------------
	// Print words in memory at all valid memory locations.
	if (debug.mem_display) {
		printf(" \n");
		printf("=====================================================\n");
		printf(" RESULTING MEMORY: \n");
		int i;
		for(i = 0; i <= MEM_ARRAY_MAX; i=i+1) {
			if (mem_array[i].valid != 0)
					printf("Address: %03x [%04o] Value: %03x [%04o]\n", i, i, mem_array[i].value,mem_array[i].value); 
		}
		printf("=====================================================\n");
	}

	printf(" \n");
	//printf("=====================================================\n");
	printf("\nSTATISTICS\n");
	printf("CPU Clocks Used: %-1d\n", stat_clocks);	// left-aligned
	printf("Total Instructions: %-1d\n", stat_instructions);  // left-aligned
	printf("Number of Instructions by Type:\n");
	printf("	AND: %d\n", stat_and);
	printf("	TAD: %d\n", stat_tad);
	printf("	ISZ: %d\n", stat_isz);
	printf("	DCA: %d\n", stat_dca);
	printf("	JMS: %d\n", stat_jms);
	printf("	JMP: %d\n", stat_jmp);
	printf("	 IO: %d\n", stat_io);
	printf("	 UI: %d\n", stat_ui);
	printf("=====================================================\n");
	printf("\nBRANCH STATISTICS\n");
	printf("Total Number of Unconditional Branches Taken: %d\n",branch_statistics.total_uncond_t_count);
	printf("         JMS Branches Taken: %d\n",branch_statistics.JMS_t_count);
	printf("         JMP Branches Taken: %d\n",branch_statistics.JMP_t_count);
	printf("   Uncond UI Branches Taken: %d\n",branch_statistics.UI_uncond_t_count);
	printf("Total Number of Conditional Branches Taken: %d out of %d\n",branch_statistics.total_cond_t_count,(branch_statistics.total_cond_t_count + branch_statistics.total_cond_nt_count));
	printf("         ISZ Branches Taken: %d out of %d\n",branch_statistics.ISZ_t_count,(branch_statistics.ISZ_t_count+branch_statistics.ISZ_nt_count));
	printf("     Cond UI Branches Taken: %d out of %d\n",branch_statistics.UI_cond_t_count,(branch_statistics.UI_cond_t_count+branch_statistics.UI_cond_nt_count));
	printf("=====================================================\n");
	// END PRINTING OF STATISTICS
	//======================================================
	
	
	// close the trace files
	fclose(fp_tracefile);
	fclose(fp_branchtrace);
}