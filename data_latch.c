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

// struct for widgets included for a single line in the memory image
typedef struct _gtkMemoryLine
{
    GtkWidget *label_arrow;
	GtkWidget *button_breakpoint;
	GtkWidget *button_image;
	GtkWidget *button_label;
	//GtkWidget *label_addr;
	//GtkWidget *label_value;
	GtkWidget *label_addr_octal;
	GtkWidget *label_addr_hex;
	GtkWidget *label_value_octal;
	GtkWidget *label_value_hex;
	GtkWidget *event_box[6]; // event boxes for bg color set
} gtkMemoryLine;

// struct for label widgets in the listing of registers
typedef struct _gtkRegisterValues
{
	GtkWidget *label_last_PC_hex;
	GtkWidget *label_last_PC_octal;
	GtkWidget *label_opcode;
	GtkWidget *label_last_IR_hex;
	GtkWidget *label_last_IR_octal;
	GtkWidget *label_last_IR_binary;
	GtkWidget *label_next_PC_hex;
	GtkWidget *label_next_PC_octal;
	GtkWidget *label_LR;
	GtkWidget *label_AC_hex;
	GtkWidget *label_AC_octal;
	GtkWidget *label_SR_hex;
	GtkWidget *label_SR_octal;
} gtkRegisterValues;

// Function Prototypes
void init();
void init_debug_strings();
void execute_instructions();
void display_memory_array( s_mem_word* ptr_mem_array, GtkWidget* vtable);
//void display_reg_values( gtkRegisterValues* gui_curr_reg, guiInstrVals* gui_last_instr);
void display_reg_values();

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
// Global GUI Variables
// - GUI
guiInstrVals gui_last_instr; // settings for last instruction executed
gtkRegisterValues gui_reg_values; // gui labels for the current register values
GtkWidget *vtable_mem;
int curr_mem_rows;	// number of memory array rows currently being displayed.

// Function to create an image box
GtkWidget *create_img_box( char* filename, GtkWidget *image, gchar *label_text)
{
    GtkWidget *box1;
    GtkWidget *label;
    //GtkImage *image;

    /* Create box for image and label */
    box1 = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box1), 2);

    image = gtk_image_new_from_file(filename);
	// use gtk_image_set_from_file(image) to update

    gtk_widget_queue_draw(image);
	
	// Label to be used as a hidden flag.
    label = gtk_label_new (label_text);

    /* Pack the pixmap and label into the box */
    gtk_box_pack_start (GTK_BOX (box1),
                        image, FALSE, FALSE, 3);

    gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 3);

    gtk_widget_show(image);
	
	// Note, do NOT display the label.  Try to use it as a toggle flag.

    return(box1);
}

// Toolbar action functions
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
    char buffer[20];	// temp buffer string
	char str_mem_index[10];
	char str_disp_index[10];
	//int oct_addr;	// octal address
	int addr = 0; // decimal index
	int disp_i; // display index
	//int i = 0;	// temp val for calc
	
	// get the button label data
	strncpy(buffer,(char *) gtk_label_get_text((GtkLabel*) data),100);
	// then parse it to get both the memory index and the display index
	//str_mem_index = strtok(buffer," ");
	strncpy(str_mem_index, strtok(buffer," "),9);
	//str_disp_index = strtok(NULL," ");
	strncpy(str_disp_index, strtok(NULL," "),9); 
	addr = atoi(str_mem_index);
	disp_i = atoi(str_disp_index);
	fprintf(stderr,"buffer: %s\n",buffer);
	fprintf(stderr, "addr: %d [%s], disp_i: %d [%s]\n",addr, str_mem_index, disp_i, str_disp_index);
	
	/*
	//oct_addr = atoi(buffer);
	// convert to decimal
	while (oct_addr != 0) {
		addr = addr + (oct_addr % 10) * pow(8,i++);
		oct_addr = oct_addr / 10;
	}
	*/
	
	/*
	// check what the current breakpoint value is, and switch the value
	if (mem_array[addr].breakpoint)
	{	// if it was on, turn it off
		mem_array[addr].breakpoint = FALSE;
		
		// change button image as well
		gtk_image_set_from_file((GtkImage*) mem_image[disp_i].button_image, "img_gtk-clear-rec-22.png");
		//gtk_widget_queue_draw(mem_image[disp_i].button_image);
	}
	else {
		// else, if it was off, turn it on
		mem_array[addr].breakpoint = TRUE;
		
		// change button image as well
		gtk_image_set_from_file((GtkImage*) mem_image[disp_i].button_image, "img_gtk-media-rec-22.png");
		gtk_widget_queue_draw(mem_image[disp_i].button_image);
	}
	*/
}

void gtk_step_clicked(GtkWidget *widget, gpointer data)
{
	// execute the instructions; pass value of true to indicate
	// that only one instruction should be executed.
	execute_instructions(TRUE);

	// display the current register values
	display_reg_values(&gui_reg_values, &gui_last_instr);
	
	// Display loaded memory in the GUI.
	display_memory_array(&mem_array[0], vtable_mem);
}

void gtk_start_clicked(GtkWidget *widget, gpointer data)
{
	// execute the instructions; pass value of false to indicate
	// that instructions should continue to be executed.
	execute_instructions(FALSE);
	
	// display the current register values
	display_reg_values(&gui_reg_values, &gui_last_instr);
	
	// Display loaded memory in the GUI.
	display_memory_array(&mem_array[0], vtable_mem);
}


int main (int argc, char* argv[])
{
  int i, j;
  
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
  GtkWidget *lbl_vtable_mem_header_octal1;
  GtkWidget *lbl_vtable_mem_header_hex1;
  GtkWidget *lbl_vtable_mem_header_octal2;
  GtkWidget *lbl_vtable_mem_header_hex2;
  GtkWidget *vbox;
  GtkWidget *hbox_main;
  //GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *vbox4;
  
  // IMAGE ICON ITEMS
  GtkWidget *image_box;

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
  
  // CURRENT REGISTER VALUES
  GtkWidget *vbox2a;
  GtkWidget *lbl_last_PC;
  GtkWidget *lbl_opcode;
  GtkWidget *lbl_last_IR;
  GtkWidget *lbl_PC;
  GtkWidget *lbl_LR;
  GtkWidget *lbl_AC;
  GtkWidget *lbl_SR;
  GtkWidget *hbox1;
  GtkWidget *hbox2;
  GtkWidget *hbox2a;
  GtkWidget *hbox3;
  GtkWidget *hbox4;
  GtkWidget *hbox5;
  GtkWidget *hbox6;
  GtkWidget *hbox7;
  GtkWidget *hsep1;
  GtkWidget *hsep2;  
  
  //char buffer[1024];
  curr_mem_rows = 0; // initialized memory array rows currently displayed to be 0

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
  hbox_main = gtk_hbox_new(FALSE, 0);
  // recall that parameters 3 - 5 are for: expand, fill, padding.
  gtk_box_pack_start(GTK_BOX(vbox), hbox_main, FALSE, FALSE, 0);
  gtk_widget_show(hbox_main);

  vbox2a = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox_main),vbox2a, FALSE, FALSE, 5);
  gtk_widget_show(vbox2a);
  
  // PC of last executed instruction:
  hbox1 = gtk_hbox_new(FALSE, 0);
  lbl_last_PC = gtk_label_new("PC of Last Executed Instruction: ");
  gui_reg_values.label_last_PC_octal = gtk_label_new("");
  gui_reg_values.label_last_PC_hex = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox1), lbl_last_PC, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox1), gui_reg_values.label_last_PC_octal, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox1), gui_reg_values.label_last_PC_hex, FALSE, FALSE, 5);
  gtk_widget_show(hbox1);
  gtk_widget_show(lbl_last_PC);
  gtk_widget_show(gui_reg_values.label_last_PC_octal);
  gtk_widget_show(gui_reg_values.label_last_PC_hex);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox1, TRUE, TRUE, 3);
  
  // Instruction Opcode:
  hbox2a = gtk_hbox_new(FALSE, 0);
  lbl_opcode = gtk_label_new("Instruction Opcode: ");
  gui_reg_values.label_opcode = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox2a), lbl_opcode, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox2a), gui_reg_values.label_opcode, FALSE, FALSE, 5);
  gtk_widget_show(hbox2a);
  gtk_widget_show(lbl_opcode);
  gtk_widget_show(gui_reg_values.label_opcode);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox2a, TRUE, TRUE, 3);
  
  // IR of last executed instruction:
  hbox2 = gtk_hbox_new(FALSE, 0);
  lbl_last_IR = gtk_label_new("IR of Last Executed Instruction: ");
  gui_reg_values.label_last_IR_octal = gtk_label_new("");
  gui_reg_values.label_last_IR_hex = gtk_label_new("");
  gui_reg_values.label_last_IR_binary = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox2), lbl_last_IR, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox2), gui_reg_values.label_last_IR_octal, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox2), gui_reg_values.label_last_IR_hex, FALSE, FALSE, 5);
  hbox3 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox3), gui_reg_values.label_last_IR_binary, FALSE, FALSE, 5);
  gtk_widget_show(hbox2);
  gtk_widget_show(lbl_last_IR);
  gtk_widget_show(gui_reg_values.label_last_IR_octal);
  gtk_widget_show(gui_reg_values.label_last_IR_hex);
  gtk_widget_show(hbox3);
  gtk_widget_show(gui_reg_values.label_last_IR_binary);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox2, TRUE, TRUE, 3);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox3, TRUE, TRUE, 3);
  
  // separator
  hsep1 = gtk_hseparator_new ();
  gtk_widget_show(hsep1);
  gtk_box_pack_start(GTK_BOX(vbox2a), hsep1, TRUE, TRUE, 3);
  
  // next Program Counter:
  hbox4 = gtk_hbox_new(FALSE, 0);
  lbl_PC = gtk_label_new("Program Counter (PC): ");
  gui_reg_values.label_next_PC_octal = gtk_label_new("");
  gui_reg_values.label_next_PC_hex = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox4), lbl_PC, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox4), gui_reg_values.label_next_PC_octal, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox4), gui_reg_values.label_next_PC_hex, FALSE, FALSE, 5);
  gtk_widget_show(lbl_PC);
  gtk_widget_show(gui_reg_values.label_next_PC_octal);
  gtk_widget_show(gui_reg_values.label_next_PC_hex);
  gtk_widget_show(hbox4);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox4, TRUE, TRUE, 3);
  
  // Link Register:
  hbox5 = gtk_hbox_new(FALSE, 0);
  lbl_LR = gtk_label_new("Link Register (LR): ");
  gui_reg_values.label_LR = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox5), lbl_LR, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox5), gui_reg_values.label_LR, FALSE, FALSE, 5);
  gtk_widget_show(lbl_LR);
  gtk_widget_show(gui_reg_values.label_LR);
  gtk_widget_show(hbox5);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox5, TRUE, TRUE, 3);
  
  // Accumulator:
  hbox6 = gtk_hbox_new(FALSE, 0);
  lbl_AC = gtk_label_new("Accumulator (AC): ");
  gui_reg_values.label_AC_octal = gtk_label_new("");
  gui_reg_values.label_AC_hex = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox6), lbl_AC, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox6), gui_reg_values.label_AC_octal, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox6), gui_reg_values.label_AC_hex, FALSE, FALSE, 5);
  gtk_widget_show(lbl_AC);
  gtk_widget_show(gui_reg_values.label_AC_octal);
  gtk_widget_show(gui_reg_values.label_AC_hex);
  gtk_widget_show(hbox6);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox6, TRUE, TRUE, 3);
  
  // Switch Register:
  hbox7 = gtk_hbox_new(FALSE, 0);
  lbl_SR = gtk_label_new("Switch Register (SR): ");
  gui_reg_values.label_SR_octal = gtk_label_new("");
  gui_reg_values.label_SR_hex = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox7), lbl_SR, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox7), gui_reg_values.label_SR_octal, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox7), gui_reg_values.label_SR_hex, FALSE, FALSE, 5);
  gtk_widget_show(lbl_SR);
  gtk_widget_show(gui_reg_values.label_SR_octal);
  gtk_widget_show(gui_reg_values.label_SR_hex);
  gtk_widget_show(hbox7);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox7, TRUE, TRUE, 3);
  
  // create vtable1 to contain the register values on the left hand side and 
  // the memory table (scrolled window) on the right hand side.
  /*
  vtable1 = gtk_table_new(4, 4, TRUE);
  gtk_table_set_row_spacings(GTK_TABLE(vtable1), 2);
  gtk_table_set_col_spacings(GTK_TABLE(vtable1), 2);
  gtk_box_pack_start(GTK_BOX(hbox_main), vtable1, TRUE, TRUE, 1);
  gtk_widget_show(vtable1);
  */
  
  // create vbox3 to contain the memory section
  vbox3 = gtk_vbox_new(FALSE, 0);
  // recall that parameters 3 - 5 are for: expand, fill, padding.
  //gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, TRUE, 1);
  // (left col, right col, top row, bottom row, xoptions, yoptions, xpadding, ypadding)
  //gtk_table_attach(GTK_TABLE(vtable1),vbox3, 2, 4, 0, 4, 0, 0, 2, 2);
  gtk_box_pack_start(GTK_BOX(hbox_main),vbox3, TRUE, TRUE, 1);
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
  lbl_vtable_mem_header_addr = gtk_label_new("Address");
  lbl_vtable_mem_header_value = gtk_label_new("Value");
  lbl_vtable_mem_header_octal1 = gtk_label_new("(Octal)");
  lbl_vtable_mem_header_hex1 = gtk_label_new("(Hex)");
  lbl_vtable_mem_header_octal2 = gtk_label_new("(Octal)");
  lbl_vtable_mem_header_hex2 = gtk_label_new("(Hex)");
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
  vtable_mem = gtk_table_new(PDP8_MEMSIZE+3, 6, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(vtable_mem), 1);
  gtk_table_set_col_spacings(GTK_TABLE(vtable_mem), 1);
  
  /* SCROLLED WINDOW INSTANTIATION */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);  
  gtk_container_border_width (GTK_CONTAINER (scrolled_window), 10);
  gtk_widget_set_usize(scrolled_window, 400, 450);
  // make horizontal scrollbar policy automatic and 
  // make the vertical scrollbar policy always on.
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  //gtk_container_add(GTK_CONTAINER(window),scrolled_window);
  gtk_box_pack_start(GTK_BOX(vbox4), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show(scrolled_window);
  
  // attach column labels and separator to the table
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_addr, 2, 4, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_value, 4, 6, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_octal1, 2, 3, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_hex1, 3, 4, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_octal2, 4, 5, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), lbl_vtable_mem_header_hex2, 5, 6, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_separator, 0, 6, 2, 3);
  gtk_widget_show(lbl_vtable_mem_header_addr);
  gtk_widget_show(lbl_vtable_mem_header_value);
  gtk_widget_show(lbl_vtable_mem_header_octal1);
  gtk_widget_show(lbl_vtable_mem_header_hex1);
  gtk_widget_show(lbl_vtable_mem_header_octal2);
  gtk_widget_show(lbl_vtable_mem_header_hex2);
  gtk_widget_show(mem_separator);
  
  //gtk_container_add(GTK_CONTAINER(scrolled_window), vtable_mem);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vtable_mem);
  gtk_widget_show(vtable_mem);
  
  // Create all the labels and buttons for the memory array, 
  // but do not display them yet.
  for (i = 0; i < PDP8_MEMSIZE; i++)
  {
	  //sprintf (buffer, "cell (%d,%d)\n", i, j);
	  
	  // create image box with the default clear button
	  //image_box = create_img_box("img_gtk-clear-rec-22.png", mem_image[i].button_image, "0");
	  mem_image[i].button_image = gtk_image_new_from_file("img_gtk-clear-rec-22.png");
	  // use gtk_image_set_from_file(image) to update
	  gtk_widget_queue_draw(mem_image[i].button_image);
	  // initialize invisible label
	  mem_image[i].button_label = gtk_label_new("");

	  mem_image[i].label_arrow = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
	  //mem_image[i].button_breakpoint = gtk_toggle_button_new();
	  mem_image[i].button_breakpoint = gtk_button_new();
	  gtk_container_add (GTK_CONTAINER(mem_image[i].button_breakpoint), mem_image[i].button_image);
	  //mem_image[i].label_addr = gtk_label_new("");
	  //mem_image[i].label_value = gtk_label_new("");
	  mem_image[i].label_addr_octal = gtk_label_new("");
	  mem_image[i].label_addr_hex = gtk_label_new("");
	  mem_image[i].label_value_octal = gtk_label_new("");
	  mem_image[i].label_value_hex = gtk_label_new("");
	  
	  for (j = 0; j < 6; j++) {
		  mem_image[i].event_box[j] = gtk_event_box_new();
	  }
	  
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[0]), mem_image[i].label_arrow);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[1]), mem_image[i].button_breakpoint);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[2]), mem_image[i].label_addr_octal);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[3]), mem_image[i].label_addr_hex);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[4]), mem_image[i].label_value_octal);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[5]), mem_image[i].label_value_hex);

	  // attach "clicked" signal for breakpoint button
	  gtk_signal_connect (GTK_OBJECT (mem_image[i].button_breakpoint), "clicked", GTK_SIGNAL_FUNC (toggle_breakpoint_button_callback), (gpointer) mem_image[i].button_label);
	  //gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (toggle_breakpoint_button_callback), (gpointer) "memory_index");
	  	  
	  /*
	  // attach widgets to the table
	  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_arrow, 0, 1, i, i+1);
	  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].button_breakpoint, 1, 2, i, i+1);
	  //gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_addr, 2, 3, i, i+1);
	  //gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_value, 3, 4, i, i+1);
	  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_addr_octal, 2, 3, i, i+1);
	  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_addr_hex, 3, 4, i, i+1);
	  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_value_octal, 4, 5, i, i+1);
	  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[i].label_value_hex, 5, 6, i, i+1);
	  */
	  
	  // show elements:
	  //gtk_widget_show (mem_image[i].label_arrow);
	  //gtk_widget_show (mem_image[i].button_breakpoint);
	  //gtk_widget_show (mem_image[i].label_addr);
	  //gtk_widget_show (mem_image[i].label_value);
	  
	  // to change the label's text after creation:
	  //gtk_label_set_text( (GtkLabel) mem_image[i].label_addr, str_address );
	  //gtk_label_set_text( (GtkLabel) mem_image[i].label_value, str_value );
  }
  
  // attach only the first row of the memory array
  // attach widgets to the table
  /*
  for (j = 0; j < 6; j++) {
	  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[0].event_box[j], j, j+1, 3, 4);
  }
  */
  
  /*
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[0].label_arrow, 0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[0].button_breakpoint, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[0].label_addr_octal, 2, 3, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[0].label_addr_hex, 3, 4, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[0].label_value_octal, 4, 5, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (vtable_mem), mem_image[0].label_value_hex, 5, 6, 0, 1);
  */
  
  // Resize table afterwards [rows, cols]
  gtk_table_resize (GTK_TABLE(vtable_mem),4,4);
  
  /* STATUS BAR INSTANTIATION */
  statusbar = gtk_statusbar_new();
  gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, TRUE, 1);

  
  /* SIGNAL CONNECTS */
  g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(tog_status), "activate", G_CALLBACK(toggle_statusbar), statusbar);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(toolitem_step), "clicked", GTK_SIGNAL_FUNC(gtk_step_clicked), NULL);
  g_signal_connect(G_OBJECT(toolitem_start), "clicked", GTK_SIGNAL_FUNC(gtk_start_clicked), NULL);
	  
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

//void display_reg_values( gtkRegisterValues* gui_curr_reg, guiInstrVals* gui_last_instr )
void display_reg_values()
{
	char buffer[512];
	
	// Set all label values
	// - Last PC
	sprintf(buffer,"%04o",gui_last_instr.last_PC);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_PC_octal, buffer );
	sprintf(buffer,"%03x",gui_last_instr.last_PC);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_PC_hex, buffer );
	// - Last Opcode
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_opcode, gui_last_instr.str_opcode );
	// - Last IR
	sprintf(buffer,"%04o",reg_IR);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_IR_octal, buffer );
	sprintf(buffer,"%03x",reg_IR);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_IR_hex, buffer );
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_IR_binary, gui_last_instr.IR_detail );
	// - Next PC
	sprintf(buffer,"%04o",reg_PC);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_next_PC_octal, buffer );
	sprintf(buffer,"%03x",reg_PC);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_next_PC_hex, buffer );
	// - AC
	sprintf(buffer,"%04o",reg_AC);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_AC_octal, buffer );
	sprintf(buffer,"%03x",reg_AC);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_AC_hex, buffer );
	// - LR
	sprintf(buffer,"%o",reg_LR);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_LR, buffer );
	
}

void display_memory_array( s_mem_word* ptr_mem_array, GtkWidget* vtable) {
	int i;	// mem_array index
	int j; // event box index
	int k; // display index for gui table
	char curr_addr_octal[10]; // address location in octal
	char curr_addr_hex[10]; // address location in hex.
	char curr_data_octal[10]; // current data in octal.
	char curr_data_hex[10];	// current data in hex.
	char str_data [20];	// data to store in the label
	
	// set table headers
//	GtkWidget* lbl_header_addr = gtk_label_new("\nAddress");
//	GtkWidget* lbl_header_value = gtk_label_new("Value in Octal\n[Hexadecimal]");
	
	// attach column labels to the table
//	gtk_table_attach_defaults (GTK_TABLE (vtable), lbl_header_addr, 1, 2, 0, 1);
//	gtk_table_attach_defaults (GTK_TABLE (vtable), lbl_header_value, 2, 3, 0, 1);
	
	// start populating table at the 4th row
	k = 3;
	
	// Resize table initially [rows, cols]
	gtk_table_resize (GTK_TABLE(vtable),PDP8_MEMSIZE+3,4);
	
	// display any lines that are valid
	for(i = 0; i <= MEM_ARRAY_MAX; i=i+1) {
		if((ptr_mem_array+i)->valid == 1) {
			//curr_addr = i;
			//curr_data = (ptr_mem_array+curr_addr)->value;
			
			// save strings of the address and data
			//sprintf(curr_addr, "%04o [%03x]", i, i);
			//sprintf(curr_data, "%04o [%03x]", (ptr_mem_array+i)->value, (ptr_mem_array+i)->value);
			sprintf(curr_addr_octal, "%04o", i);
			sprintf(curr_addr_hex,"%03x", i);
			sprintf(curr_data_octal, "%04o", (ptr_mem_array+i)->value);
			sprintf(curr_data_hex, "%03x", (ptr_mem_array+i)->value);
			// for the data label, store the memory array index, and then the display table index.
			sprintf(str_data, "%d %d ", i, k);
			
			// attach widgets to the table - ONLY DO ONCE, if not already attached
			if (k > curr_mem_rows+2)
			{
				/*
				gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_arrow, 0, 1, k, k+1);
				gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].button_breakpoint, 1, 2, k, k+1);
				//gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_addr, 2, 3, k, k+1);
				//gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_value, 3, 4, k, k+1);
				gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_addr_octal, 2, 3, k, k+1);
				gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_addr_hex, 3, 4, k, k+1);
				gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_value_octal, 4, 5, k, k+1);
				gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].label_value_hex, 5, 6, k, k+1);
				*/
				for (j = 0; j < 6; j++) {
					gtk_table_attach_defaults (GTK_TABLE (vtable), mem_image[k].event_box[j], j, j+1, k, k+1);
				}
				curr_mem_rows++;
			}
						  
			// to change the label's text after creation:
			//gtk_label_set_text( (GtkLabel*) mem_image[k].label_addr, curr_addr );
			//gtk_label_set_text( (GtkLabel*) mem_image[k].label_value, curr_data );
			gtk_label_set_text( (GtkLabel*) mem_image[k].label_addr_octal, curr_addr_octal );
			gtk_label_set_text( (GtkLabel*) mem_image[k].label_addr_hex, curr_addr_hex );
			gtk_label_set_text( (GtkLabel*) mem_image[k].label_value_octal, curr_data_octal );
			gtk_label_set_text( (GtkLabel*) mem_image[k].label_value_hex, curr_data_hex );
			gtk_label_set_text( (GtkLabel*) mem_image[k].button_label, str_data);
			  
			// show elements:
			if (reg_PC == i) {
				// Display arrow for next instruction to be executed
				gtk_widget_show (mem_image[k].label_arrow);
			}
			else {
				gtk_widget_hide (mem_image[k].label_arrow);
			}
			gtk_widget_show (mem_image[k].button_breakpoint);
			gtk_widget_show (mem_image[k].button_image);
			//gtk_widget_show (mem_image[k].label_addr);
			//gtk_widget_show (mem_image[k].label_value);
			gtk_widget_show (mem_image[k].label_addr_octal);
			gtk_widget_show (mem_image[k].label_addr_hex);
			gtk_widget_show (mem_image[k].label_value_octal);
			gtk_widget_show (mem_image[k].label_value_hex);
			
			// set the button image depending on the value of the 
			// breakpoint flag
			if ((ptr_mem_array+i)->breakpoint) {
				gtk_image_set_from_file((GtkImage*) mem_image[k].button_image, "img_gtk-media-rec-22.png");
				gtk_widget_queue_draw(mem_image[k].button_image);
			}
			else {
				gtk_image_set_from_file((GtkImage*) mem_image[k].button_image, "img_gtk-clear-rec-22.png");
				gtk_widget_queue_draw(mem_image[k].button_image);
			}
			
			for (j = 0; j < 6; j++) {
				gtk_widget_show (mem_image[k].event_box[j]);
			}
			
			// set color background of all elements in this row
			
			
			k++; // increment the display index
		}
	}
	
	// Resize table finally [rows, cols]
	gtk_table_resize (GTK_TABLE(vtable),k,4);
}

void init() 
{
	// - Debug Print Strings:
	// - Debug Flags:
	debug.mem_display = 0;
	debug.instr = 0;
	debug.module = 0;
	debug.eaddr = 0;
	debug.short_mode = 0;
	// Effective Address
	curr_eaddr.EAddr = 0;

	
	
	// clear the trace files
	if ((fp_tracefile = fopen(trace_filename, "w+")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", trace_filename);
		exit (1);
	}
	fclose(fp_tracefile);
	if ((fp_branchtrace = fopen(branch_filename, "w+")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", branch_filename);
		exit (1);
	}
	// print header for branch trace file
	fprintf(fp_branchtrace,"PC [octal]    BRANCH TYPE         TAKEN/NOT TAKEN    TARGET ADDRESS [octal]\n");
	fprintf(fp_branchtrace,"---------------------------------------------------------------------------\n");
	fclose(fp_branchtrace);
}

void execute_instructions(int flag_step) 
{
	char buffer[512]; // temp buffer for formatting string for IR binary
	
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
	
	
	// initialize breakpoint flag to FALSE.
	int flag_break = 0;
	
	// clear read/write index data; set it out of range of the memory array indices
	gui_last_instr.index_read = PDP8_MEMSIZE+1;
	gui_last_instr.index_write = PDP8_MEMSIZE+1;
	gui_last_instr.index_eaddr_read = PDP8_MEMSIZE+1;
	gui_last_instr.index_eaddr_write = PDP8_MEMSIZE+1;

	// Open the trace and branch trace files.
	if ((fp_tracefile = fopen(trace_filename, "a")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", trace_filename);
		exit (1);
	}
	if ((fp_branchtrace = fopen(branch_filename, "a")) == NULL) {
		fprintf (stderr, "Unable to open trace file %s\n", branch_filename);
		exit (1);
	}
	// Note: Leave trace files open for append.
	
	//======================================================
	// MAIN LOOP
	do {
		//counter++;
		effective_address = 0;
		memval_eaddr = 0;
		
		// STEP 1: FETCH INSTRUCTION
		reg_IR = read_mem(reg_PC, TF_FETCH, &mem_array[0], fp_tracefile);
		// set opcode
		curr_opcode = reg_IR >> (PDP8_WORD_SIZE - INSTR_OP_LOW - 1);
		
		// check if there was a breakpoint set on this instruction
		// (if so, stop execution AFTER this instruction completes)
		if (mem_array[reg_PC].breakpoint) {
			flag_break = TRUE;
		}
		
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
			curr_eaddr = calc_eaddr(reg_IR, reg_PC, &mem_array[0], fp_tracefile, debug.eaddr, &gui_last_instr);
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
		}
		
		// set up format for the IR value
		strncpy(gui_last_instr.IR_detail, "   0   1   2   3   4   5   6   7   8   9  10  11\n",MAX_IR_DETAIL);
		strncat(gui_last_instr.IR_detail," +---+---+---+---+---+---+---+---+---+---+---+---+\n",MAX_IR_DETAIL);
		sprintf(buffer," | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c |\n",
			bin_str_IR[0],bin_str_IR[1],bin_str_IR[2],bin_str_IR[3],bin_str_IR[4],bin_str_IR[5],
			bin_str_IR[6],bin_str_IR[7],bin_str_IR[8],bin_str_IR[9],bin_str_IR[10],bin_str_IR[11]);
		strncat(gui_last_instr.IR_detail,buffer,MAX_IR_DETAIL);
		strncat(gui_last_instr.IR_detail," +---+---+---+---+---+---+---+---+---+---+---+---+\n",MAX_IR_DETAIL);
		// save PC of executed instruction
		gui_last_instr.last_PC = reg_PC;
		// save opcode string
		strncpy(gui_last_instr.str_opcode, curr_opcode_str, 15);
		
		// STEP 3: SET NEXT_PC
		next_PC = (reg_PC + 1) & address_mask;
		
		// STEP 4: EXECUTE CURRENT INSTRUCTION
		switch (curr_opcode) {
			case OP_AND:	// first, load the value from memory at the effective address
							memval_eaddr = read_mem(effective_address, TF_READ, &mem_array[0], fp_tracefile);
							gui_last_instr.index_read = effective_address;
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
							gui_last_instr.index_read = effective_address;
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
							gui_last_instr.index_read = effective_address;
							flag_branch_taken = 0;	// initialize flag that branch was taken to false.
							next_memval_eaddr = (memval_eaddr + 1) & address_mask; // increment M[EAddr]
							// write updated value to memory
							write_mem(effective_address, next_memval_eaddr, &mem_array[0], fp_tracefile);
							gui_last_instr.index_write = effective_address;
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
							gui_last_instr.index_write = effective_address;
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
							gui_last_instr.index_write = effective_address;
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
							// GUI string detail
							if ( ((reg_IR >> (PDP8_WORD_SIZE-3-1)) & 1) == 0) {
								// Group 1 micro ops
								strncat(gui_last_instr.IR_detail,"                  cla cll cma cml rar ral 0/1 iac\n",MAX_IR_DETAIL);
							} 
							else if (((reg_IR >> (PDP8_WORD_SIZE - 11-1)) & 1) == 0) {
								// Group 2 micro ops
								if (((reg_IR >> (PDP8_WORD_SIZE - 8-1)) & 1) == 0) { // OR Group
									strncat(gui_last_instr.IR_detail,"                  cla sma sza snl 0/1 osr hlt\n",MAX_IR_DETAIL);
								}
								else { // AND Group
									strncat(gui_last_instr.IR_detail,"                  cla spa sna szl 0/1 osr hlt\n",MAX_IR_DETAIL);
								}
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
	} while (!flag_HLT && !flag_step && !flag_break);
	// END MAIN LOOP
	//======================================================
	
	// free the debug strings
	free(curr_opcode_str);
	free(bin_str_PC);
	free(bin_str_next_PC);
	free(bin_str_IR);
	free(bin_str_AC);
	free(bin_str_next_AC);
	free(bin_str_LR);
	free(bin_str_next_LR);
	free(bin_str_SR);
	free(bin_str_EAddr);
	free(bin_str_next_EAddr);
	free(bin_str_memval_EAddr);
	free(bin_str_next_memval_EAddr);
	
	// close the trace files
	fclose(fp_tracefile);
	fclose(fp_branchtrace);
}
	
void print_statistics()
{
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
}

