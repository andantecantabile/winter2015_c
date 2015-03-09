/***********************************************************/
/* Title: ECE 586 Project: PDP-8 Instruction Set Simulator */
/* Date: March 8th, 2015                                   */
/* Comments: Takes an input test file as the first command */
/*    line parameter.  The starting Switch Register value  */
/*    may be specified as an optional second command line  */
/*    parameter.                                           */
/***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gui_final.h"

//=========================================================
// PARAMETER SECTION
//=========================================================
// DEFAULT STARTING ADDRESS: (in decimal)
#define DEFAULT_STARTING_ADDRESS 128

// TRACE FILE NAMES:
#define PARAM_FILE_NAME "param.txt"
#define TRACE_FILE_NAME "trace.txt"
#define BRANCH_FILE_NAME "branch_trace.txt"

// WINDOW DIMENSIONS
#define MAIN_WINDOW_WIDTH 800
#define MAIN_WINDOW_HEIGHT 600
#define SCROLLED_WINDOW_WIDTH 400
#define SCROLLED_WINDOW_HEIGHT 520

#define BUTTON_IMG_FILE_DEFAULT "img_gtk-clear-rec-22.png"
#define BUTTON_IMG_FILE_RED "img_gtk-media-rec-22.png"
#define BUTTON_IMG_FILE_TEAL "img_gtk-teal-rec-22.png"
#define BUTTON_IMG_FILE_SM_DEF "img_gtk-clear-rec-22.png"
#define BUTTON_IMG_FILE_SM_TEAL "img_gtk-teal-rec-22.png"
//---------------------------	
// END PARAMETER SECTION
//=========================================================

// struct for widgets included for a single line in the memory image
typedef struct _gtkMemoryLine
{
    GtkWidget *label_arrow;			// arrow used to indicate current PC
	GtkWidget *button_breakpoint;	// "toggle" type button used to set breakpoint
	GtkWidget *button_image;		// image for the breakpoint button; 
		// Note that the source of the button image has to be dynamically changed
		// on button click depending on the status of the breakpoint value in the
		// corresponding entry in the memory array.
	GtkWidget *button_label;	// this is an important "hidden" label which
		// is used to pass back data on click of the breakpoint button.
		// The text of this label is set to a space separated list of the
		// following two values (in decimal): 
		//    1. index of the corresponding value in the memory array
		//    2. index of the current display row in the GUI memory table
		//       located in the scrolled window
	GtkWidget *label_addr_octal;	// labels for the memory address
	GtkWidget *label_addr_hex;
	GtkWidget *label_value_octal;	// labels for the value at the current addr
	GtkWidget *label_value_hex;
	GtkWidget *event_box[6]; // event boxes for bg color set
		// Note: These event boxes are used to pack the above items inside them;
		//       The reason these are used is because the background color of
		//       these event boxes is dynamically updated on "step" and "start"
		//       actions to reflect the results of the last instruction performed.
} gtkMemoryLine;

// struct for label widgets in the listing of registers
typedef struct _gtkRegisterValues
{
	// Previous instruction values
	GtkWidget *label_last_PC_hex;		// Previous PC (instruction
	GtkWidget *label_last_PC_octal;		//   that was last executed)
	GtkWidget *label_opcode;			// Opcode text label
	GtkWidget *label_last_IR_hex;		// IR labels
	GtkWidget *label_last_IR_octal;
	GtkWidget *label_last_IR_binary;
	GtkWidget *label_last_eaddr_hex;	// Effective Address
	GtkWidget *label_last_eaddr_octal;
	// Updated Register Values
	GtkWidget *label_next_PC_hex;		// PC
	GtkWidget *label_next_PC_octal;
	GtkWidget *label_LR;				// LR
	GtkWidget *label_AC_hex;			// AC
	GtkWidget *label_AC_octal;
	GtkWidget *label_SR_hex;			// SR
	GtkWidget *label_SR_octal;
} gtkRegisterValues;

typedef struct _gtkSRBit
{
	GtkWidget *label_SR_bit;
	GtkWidget *button_set;
	GtkWidget *button_image;
	GtkWidget *button_label;
} gtkSRBit;

// Function Prototypes
void clear_stats();
void init_trace_files();
void init_debug_strings();
void execute_instructions();
void display_memory_array( s_mem_word* ptr_mem_array, GtkWidget* vtable);
void display_reg_values();
void display_SR_bit_values();
void print_statistics();

//------------------------------------
/* GLOBAL VARIABLES */
// NOTE: Given more time, these should be moved into main() or other
//       functions, and malloc() should be used to store these on the
//       heap.  But for the moment, putting these here to ensure that
//       on toggle or setting of various buttons, etc., the values
//       will be visible.
// The values of the following variables MUST persist between 
// all function calls.

// - Main Memory Array
s_mem_word mem_array [PDP8_MEMSIZE]; // Memory Space
// - Memory Display Array for the GUI
gtkMemoryLine mem_image[PDP8_MEMSIZE];  // would potentially need as many lines 
								// as there are in the PDP-8 memory.
gtkSRBit gui_SR_bits[PDP8_WORD_SIZE]; // gui objects for SR bits

// - Debug and Branch statistics
s_debug_flags debug;				// Debug Flags
s_branch_stats branch_statistics; 	// Branch Statistics tracking

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
int reg_SR_binary[PDP8_WORD_SIZE];
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
guiInstrVals gui_last_instr; 		// settings for last instruction executed
gtkRegisterValues gui_reg_values; 	// gui labels for the current register values
GtkWidget *vtable_mem;	// table used inside the scrolled window to align elements
	// on each row of the memory array
GtkToolItem *toolitem_start;	// tool bar buttons
GtkToolItem *toolitem_step;
static GtkToolItem *toolitem_restart;
int curr_mem_rows;	// number of memory array rows currently being displayed.
// - GUI COLORS - RGBA
// These are used on each redraw of the memory table display.
GdkColor color_last_instr;
GdkColor color_eaddr_read;
GdkColor color_eaddr_write;
GdkColor color_read;
GdkColor color_write;
GdkColor color_original_bg;
GdkColor color_default_bg;
GdkColor color_btn_select;
// END VARIABLES
//-------------------------------------

//=======================================
// GUI Display-Related Functions
//----------------------------------

// Function to create an image box
// This function is used in the creation of each breakpoint button in
// the memory table.
GtkWidget *create_img_box( char* filename, GtkWidget *image, gchar *label_text)
{
    GtkWidget *box1;
    GtkWidget *label;

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
	
	// Note, do NOT display the label.  This widget is only used
	// to pass data to the function which gets called on click of 
	// the breakpoint button.

    return(box1);
}

// Toolbar action functions
// hide and display the status bar
// Note: Did not have time to utilize the status bar (write messages to it).
void toggle_statusbar(GtkWidget *widget, gpointer statusbar) 
{
  if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
    gtk_widget_show(statusbar);
  } else {
    gtk_widget_hide(statusbar);
  }
}

// Function to set/remove breakpoints
void toggle_breakpoint_button_callback (GtkWidget *widget, gpointer data)
{
    char buffer[20];	// temp buffer string
	char str_mem_index[10];
	char str_disp_index[10];
	int addr = 0; // decimal index
	int disp_i; // display index
	//int oct_addr;	// octal address
	//int i = 0;	// temp val for calc
	
	// get the button label data
	strncpy(buffer,(char *) gtk_label_get_text((GtkLabel*) data),18);
	// then parse it to get both the memory index and the display index
	strncpy(str_mem_index, strtok(buffer," "),9);
	strncpy(str_disp_index, strtok(NULL," "),9); 
	addr = atoi(str_mem_index);
	disp_i = atoi(str_disp_index);
	// debug prints:	
	//fprintf(stderr,"buffer: %s\n",buffer);
	//fprintf(stderr, "addr: %d [%s], disp_i: %d [%s]\n",addr, str_mem_index, disp_i, str_disp_index);
	
	/* 
	// Initially, was passing the octal string value, and then 
	// realized can just pass the decimal value, so this was 
	// not needed anymore.
	//oct_addr = atoi(buffer);
	// convert to decimal
	while (oct_addr != 0) {
		addr = addr + (oct_addr % 10) * pow(8,i++);
		oct_addr = oct_addr / 10;
	}
	*/
	
	// check what the current breakpoint value is, and switch the value
	if (mem_array[addr].breakpoint)
	{	// if it was on, turn it off
		mem_array[addr].breakpoint = FALSE;
		
		// change button image as well
		gtk_image_set_from_file((GtkImage*) mem_image[disp_i].button_image, BUTTON_IMG_FILE_DEFAULT);
		gtk_widget_queue_draw(mem_image[disp_i].button_image);
	}
	else {
		// else, if it was off, turn it on
		mem_array[addr].breakpoint = TRUE;
		
		// change button image as well
		gtk_image_set_from_file((GtkImage*) mem_image[disp_i].button_image, BUTTON_IMG_FILE_RED);
		gtk_widget_queue_draw(mem_image[disp_i].button_image);
	}

}

// Function to be executed when a toggle SR button has been clicked
void toggle_SR_bit_button_callback(GtkWidget *widget, gpointer data)
{
	int i;
	char buffer[20];	// temp buffer string
	
	// get the button label data
	strncpy(buffer,(char *) gtk_label_get_text((GtkLabel*) data),8);
	
	// convert to int
	i = atoi(buffer);
	
	//fprintf(stderr,"clicked button #%s: %d\n",buffer, i);
	//fprintf(stderr," SR[%d]: %d\n",i, reg_SR_binary[i]);
	
	// check what the current SR bit value is, and switch the value
	if (reg_SR_binary[i])
	{	// if it was on, turn it off
		reg_SR_binary[i] = 0;
		gtk_label_set_text( (GtkLabel*) gui_SR_bits[i].label_SR_bit, "0");
		
		// change button image as well
		gtk_image_set_from_file((GtkImage*) gui_SR_bits[i].button_image, BUTTON_IMG_FILE_SM_DEF);
		gtk_widget_queue_draw(gui_SR_bits[i].button_image);
	}
	else {
		// else, if it was off, turn it on
		reg_SR_binary[i] = 1;
		gtk_label_set_text( (GtkLabel*) gui_SR_bits[i].label_SR_bit, "1");
		
		// change button image as well
		gtk_image_set_from_file((GtkImage*) gui_SR_bits[i].button_image, BUTTON_IMG_FILE_SM_TEAL);
		gtk_widget_queue_draw(gui_SR_bits[i].button_image);
	}
	
	// recalculate the SR value
	reg_SR = binary_reg_to_short_int((int*)&reg_SR_binary[0]);
	
	// - Redisplay the SR
	sprintf(buffer,"%04o",reg_SR);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_SR_octal, buffer );
	sprintf(buffer,"%03x",reg_SR);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_SR_hex, buffer );
}

// Function to be executed when the step button is clicked
void gtk_step_clicked(GtkWidget *widget, gpointer data)
{
	// disable SR set bit buttons
	int i = 0;
	for (i = 0; i < PDP8_WORD_SIZE; i++)
	{
		gtk_widget_set_sensitive ((GtkWidget*) gui_SR_bits[i].button_set, FALSE);
	}
	
	// execute the instructions; pass value of true to indicate
	// that only one instruction should be executed. (STEP mode)
	execute_instructions(TRUE);

	// display the current register values
	display_reg_values(&gui_reg_values, &gui_last_instr);
	
	// Display loaded memory in the GUI.
	display_memory_array(&mem_array[0], vtable_mem);
}

// Function to be executed when the start (continue) button is clicked
void gtk_start_clicked(GtkWidget *widget, gpointer data)
{
	// disable SR set bit buttons
	int i = 0;
	for (i = 0; i < PDP8_WORD_SIZE; i++)
	{
		gtk_widget_set_sensitive ((GtkWidget*) gui_SR_bits[i].button_set, FALSE);
	}
	
	// execute the instructions; pass value of false to indicate
	// that instructions should continue to be executed.
	execute_instructions(FALSE);
	
	// display the current register values
	display_reg_values(&gui_reg_values, &gui_last_instr);
	
	// Display loaded memory in the GUI.
	display_memory_array(&mem_array[0], vtable_mem);
}

// Function to be executed when the restart button is clicked.
void gtk_restart_clicked(GtkWidget *widget, gpointer data)
{
	int i = 0; // loop index
	
	// clear the trace files
	init_trace_files(); 
	
	// reset all register values
	reg_PC = DEFAULT_STARTING_ADDRESS;	// Program Counter [0:11]
	reg_IR = 0;	// Current Instruction loaded from the PC.
	curr_opcode = 0;	// Current Opcode = IR[0:2]
	reg_LR = 0;		// Link Register [0] (One bit only!)
	reg_AC = 0;	// Accumulator [0:11]
	// NOTE: Do NOT reset the switch register value.
	//reg_SR = 0;	// Console Switch Register [0:11]
	next_PC = 0;
	next_AC = 0;	
	next_LR = 0;
	
	// clear read/write index data; set it out of range of the memory array indices
	gui_last_instr.index_read = PDP8_MEMSIZE+1;
	gui_last_instr.index_write = PDP8_MEMSIZE+1;
	gui_last_instr.index_eaddr_read = PDP8_MEMSIZE+1;
	gui_last_instr.index_eaddr_write = PDP8_MEMSIZE+1;
	
	// set this flag_eaddr to FALSE for the previous instruction, which 
	// will signal that the effective address of the previously executed
	// instruction should not be displayed.
	gui_last_instr.flag_eaddr = FALSE;
	
	// similarly, set the gui value for last PC to be out of range of the 
	// memory array, so that in display_reg_values(), all labels
	// relating to the last instruction executed will be cleared.
	gui_last_instr.last_PC = PDP8_MEMSIZE+1;
	
	// clear all statistics variables (including branch stats)
	clear_stats();
	
	// Read in debug flag settings from param.txt.
    read_param_file(&debug, param_filename);

	// clear the output trace files
	init_trace_files();
	
	// Read in memory from input file.
	load_memory_array(&mem_array[0], input_filename, debug.mem_display);
  
	// Display loaded memory in the GUI.
	display_memory_array(&mem_array[0], vtable_mem);
	
	// Display register values
	display_reg_values();
	
	// Display the SR bits
	display_SR_bit_values();
	
	// disable the restart button
	gtk_widget_set_sensitive ((GtkWidget*) toolitem_restart, FALSE);
	
	// enable the start and step button
	gtk_widget_set_sensitive ((GtkWidget*) toolitem_start, TRUE);
	gtk_widget_set_sensitive ((GtkWidget*) toolitem_step, TRUE);
	
	// enable the SR set buttons
	for (i = 0; i < PDP8_WORD_SIZE; i++)
	{
		gtk_widget_set_sensitive ((GtkWidget*) gui_SR_bits[i].button_set, TRUE);
	}
}

// This function was being used to try to obtain the default
// background color of the event box in the memory array window.
// In the meantime, used some other default color for every event 
// box in the memory table.
void widget_realized_bg (GtkWidget *widget) {
  //GdkColor *color = NULL;
  GtkStyle *style = gtk_widget_get_style (widget);

  if (style != NULL) {
	// set original background color
    color_original_bg = style->bg[GTK_STATE_NORMAL];

    // somehow this is not working, it sets backgrounds to black.
  }
}


int main (int argc, char* argv[])
{
  int i, j; // loop indices
  char buffer[20]; // temp string
  
  // CHECK COMMAND LINE ARGUMENTS:
  if (argc < 2) {
  fprintf (stderr, "Usage: <command> <inputfile>\n");
  fprintf (stderr, "Usage: <command> <inputfile> <switch_register>\n");
  exit (1);
  }
  
  // store input filename
  input_filename = argv[1];
  // if more than 2 arguments given, third argument is the Switch Register value
  if (argc > 2) reg_SR = atoi(argv[2]); 
  
  // convert to binary
  for (i = 0; i < PDP8_WORD_SIZE; i++)
  {
	  reg_SR_binary[i]=0;
  }
  
  short_int_to_binary_reg(reg_SR,(int*)&reg_SR_binary[0]);
  /*
  fprintf(stderr,"reg SR binary: %d %d %d %d %d %d %d %d %d %d %d %d\n",
	reg_SR_binary[0],reg_SR_binary[1],reg_SR_binary[2],
	reg_SR_binary[3],reg_SR_binary[4],reg_SR_binary[5],
	reg_SR_binary[6],reg_SR_binary[7],reg_SR_binary[8],
	reg_SR_binary[9],reg_SR_binary[10],reg_SR_binary[11]);
  */
  
  // INITIALIZE COLORS
  //gboolean res;
  //-- Last Instruction Color
  //rgba(255, 183, 0, 1), hsla(43, 100%, 50%, 1), #ffb700
  //res = gdk_rgba_parse (&color_last_instr,"rgba(255,183,0,1)");
  if (gdk_color_parse ("#ffb700", &color_last_instr) == 0) ; //failed to parse color
  //-- EAddr Read Color
  //rgba(53, 133, 0, 1), hsla(96, 100%, 26%, 1), #358500
  //rgba(84, 182, 154, 1), hsla(163, 40%, 52%, 1), #54b69a
  //res = gdk_rgba_parse (&color_eaddr_read,"rgba(84,182,154,1)");
  if (gdk_color_parse ("#54b69a", &color_eaddr_read) == 0) ; //failed to parse color
  //-- EAddr Write Color
  //rgba(144, 84, 182, 1), hsla(277, 40%, 52%, 1), #9054b6
  //res = gdk_rgba_parse (&color_eaddr_write,"rgba(144,84,182,1)");
  //if (gdk_color_parse ("#9054b6", &color_eaddr_write) == 0) ; //failed to parse color
  if (gdk_color_parse ("#8171cc", &color_eaddr_write) == 0) ; //failed to parse color
  //-- Memory Read Color
  //rgba(0, 119, 133, 1), hsla(186, 100%, 26%, 1), #007785
  //res = gdk_rgba_parse (&color_read,"rgba(0,119,133,1)");
  if (gdk_color_parse ("#007785", &color_read) == 0) ; //failed to parse color
  //-- Memory Write Color
  //rgba(96, 41, 79, 1), hsla(319, 40%, 27%, 1), #60294f
  //res = gdk_rgba_parse (&color_write,"rgba(96,41,79,1)");
  //if (gdk_color_parse ("#60294f", &color_write) == 0) ; //failed to parse color
  if (gdk_color_parse ("#cc71a9", &color_write) == 0) ; //failed to parse color
  //-- Default BG color
  if (gdk_color_parse ("#d9d7e0", &color_default_bg) == 0) ; //failed to parse color
  //-- button select color
  if (gdk_color_parse ("#00f508", &color_btn_select) == 0) ; //failed to parse color
  
  // WINDOW ITEMS
  GtkWidget *window;	// Main Window
  GtkWidget *scrolled_window;	// Scrolled Window to contain memory table
  //GtkWidget *vtable1;
  GtkWidget *vframe_mem;	// Memory Frame 
  //GtkWidget *vtable_mem_header;
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
  GtkWidget *vtable_sr; // SR bit table
  
  // IMAGE ICON ITEMS
  //GtkWidget *image_box;

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
  GtkToolItem *toolitem_open;
  GtkToolItem *toolitemsep;
  
  // CURRENT REGISTER VALUES
  GtkWidget *vbox2a;
  GtkWidget *lbl_last_PC;
  GtkWidget *lbl_opcode;
  GtkWidget *lbl_last_IR;
  GtkWidget *lbl_last_eaddr;
  GtkWidget *lbl_PC;
  GtkWidget *lbl_LR;
  GtkWidget *lbl_AC;
  GtkWidget *lbl_SR;
  GtkWidget *lbl_color_key;
  GtkWidget *hbox1;
  GtkWidget *hbox2;
  GtkWidget *hbox2a;
  GtkWidget *hbox3;
  GtkWidget *hbox3b;
  GtkWidget *hbox4;
  GtkWidget *hbox5;
  GtkWidget *hbox6;
  GtkWidget *hbox7;
  GtkWidget *hbox7b;
  GtkWidget *hbox8;
  GtkWidget *hbox9;
  GtkWidget *hsep1;
  GtkWidget *hsep2;
  GtkWidget *hsep3;
  GtkWidget *event_box_last_instr;
  GtkWidget *lbl_eb_last_instr;
  GtkWidget *event_box_eaddr_read;
  GtkWidget *lbl_eb_eaddr_read;
  GtkWidget *event_box_eaddr_write;
  GtkWidget *lbl_eb_eaddr_write;
  GtkWidget *event_box_mem_read;
  GtkWidget *lbl_eb_mem_read;
  GtkWidget *event_box_mem_write;
  GtkWidget *lbl_eb_mem_write;
  
  curr_mem_rows = 0; // initialized memory array rows currently displayed to be 0

  GtkAccelGroup *accel_group = NULL;

  gtk_init(&argc, &argv);

  /* MAIN WINDOW SET UP */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "PDP-8 Instruction Set Simulator");
  gtk_window_set_default_size(GTK_WINDOW(window),MAIN_WINDOW_WIDTH,MAIN_WINDOW_HEIGHT);
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

  toolitem_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_open, -1);
  // disable the open file button; didn't have time to implement
  gtk_widget_set_sensitive ((GtkWidget*) toolitem_open, FALSE);
  
  toolitemsep = gtk_separator_tool_item_new();
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitemsep, -1); 

  toolitem_start = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_LAST);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_start, -1);

  toolitem_step = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_step, -1); 

  toolitem_restart = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_restart, -1);
  // initially, disable the restart button
  gtk_widget_set_sensitive ((GtkWidget*) toolitem_restart, FALSE);

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
  
  // Effective Address of last executed instruction:
  hbox3b = gtk_hbox_new(FALSE, 0);
  lbl_last_eaddr = gtk_label_new("Effective Address: ");
  gui_reg_values.label_last_eaddr_octal = gtk_label_new("");
  gui_reg_values.label_last_eaddr_hex = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox3b), lbl_last_eaddr, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox3b), gui_reg_values.label_last_eaddr_octal, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox3b), gui_reg_values.label_last_eaddr_hex, FALSE, FALSE, 5);
  gtk_widget_show(hbox3b);
  gtk_widget_show(lbl_last_eaddr);
  gtk_widget_show(gui_reg_values.label_last_eaddr_octal);
  gtk_widget_show(gui_reg_values.label_last_eaddr_hex);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox3b, TRUE, TRUE, 3);
  
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
  
  // separator
  hsep2 = gtk_hseparator_new ();
  gtk_widget_show(hsep2);
  gtk_box_pack_start(GTK_BOX(vbox2a), hsep2, TRUE, TRUE, 3);
  
  // Switch Register:
  hbox7 = gtk_hbox_new(FALSE, 0);
  lbl_SR = gtk_label_new("Switch Register (SR): ");
  sprintf(buffer,"%04o",reg_SR);
  gui_reg_values.label_SR_octal = gtk_label_new(buffer);
  sprintf(buffer,"%03x",reg_SR);
  gui_reg_values.label_SR_hex = gtk_label_new(buffer);
  gtk_box_pack_start(GTK_BOX(hbox7), lbl_SR, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox7), gui_reg_values.label_SR_octal, FALSE, FALSE, 5);
  gtk_box_pack_end(GTK_BOX(hbox7), gui_reg_values.label_SR_hex, FALSE, FALSE, 5);
  gtk_widget_show(lbl_SR);
  gtk_widget_show(gui_reg_values.label_SR_octal);
  gtk_widget_show(gui_reg_values.label_SR_hex);
  gtk_widget_show(hbox7);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox7, TRUE, TRUE, 3);
  
  hbox7b = gtk_hbox_new(FALSE, 0);
  vtable_sr = gtk_table_new(3, PDP8_WORD_SIZE, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(vtable_sr), 1);
  gtk_table_set_col_spacings(GTK_TABLE(vtable_sr), 1);
  for (i = 0; i < PDP8_WORD_SIZE; i++)
  {
	  // determine which image and label to use for current SR bit
	  if (reg_SR_binary[i])
	  {
		  // create image box with the set teal button
		  gui_SR_bits[i].button_image = gtk_image_new_from_file(BUTTON_IMG_FILE_SM_TEAL);
		  // use gtk_image_set_from_file(image) to update
		  gtk_widget_queue_draw(gui_SR_bits[i].button_image);
		  // initialize the visible label
		  sprintf(buffer,"%d",reg_SR_binary[i]);
		  gui_SR_bits[i].label_SR_bit = gtk_label_new(buffer);
	  }
	  else
	  {
		  // create image box with the default clear button
		  gui_SR_bits[i].button_image = gtk_image_new_from_file(BUTTON_IMG_FILE_SM_DEF);
		  // use gtk_image_set_from_file(image) to update
		  gtk_widget_queue_draw(gui_SR_bits[i].button_image);
		  // initialize the visible label
		  sprintf(buffer,"%d",reg_SR_binary[i]);
		  gui_SR_bits[i].label_SR_bit = gtk_label_new(buffer);
	  }
	  //fprintf(stderr,"bit #%d; %s\n", i, buffer);
	  
	  // initialize bit position labels 
	  sprintf(buffer,"%d",i);
	  gui_SR_bits[i].button_label = gtk_label_new(buffer);
	  
	  //fprintf(stderr,"   position #: %s\n",buffer);

	  // Initialize the toggle button
	  gui_SR_bits[i].button_set = gtk_button_new(); 
	  gtk_container_add (GTK_CONTAINER(gui_SR_bits[i].button_set), gui_SR_bits[i].button_image);
	  
	  // attach "clicked" signal for toggle button
	  gtk_signal_connect (GTK_OBJECT (gui_SR_bits[i].button_set), "clicked", GTK_SIGNAL_FUNC (toggle_SR_bit_button_callback), (gpointer) gui_SR_bits[i].button_label);
	  gtk_widget_show(gui_SR_bits[i].button_image);
	  gtk_widget_show(gui_SR_bits[i].button_label);
	  gtk_widget_show(gui_SR_bits[i].button_set);
	  gtk_widget_show(gui_SR_bits[i].label_SR_bit);
	  
	  // attach bit position label, button and visible value label to the table
	  gtk_table_attach_defaults (GTK_TABLE (vtable_sr), gui_SR_bits[i].button_label, i, i+1, 0, 1);
	  gtk_table_attach_defaults (GTK_TABLE (vtable_sr), gui_SR_bits[i].button_set, i, i+1, 1, 2);
	  gtk_table_attach_defaults (GTK_TABLE (vtable_sr), gui_SR_bits[i].label_SR_bit, i, i+1, 2, 3);
  }
  
  gtk_widget_show(vtable_sr);
  gtk_box_pack_start(GTK_BOX(hbox7b), vtable_sr, TRUE, TRUE, 3);
  gtk_widget_show(hbox7b);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox7b, TRUE, TRUE, 3);
  
  // separator
  hsep3 = gtk_hseparator_new ();
  gtk_widget_show(hsep3);
  gtk_box_pack_start(GTK_BOX(vbox2a), hsep3, TRUE, TRUE, 3);
  
  // Color Legend for the Memory Array Window
  // Header Label for the color legend
  hbox8 = gtk_hbox_new(FALSE, 0);
  lbl_color_key = gtk_label_new("Color Reference Key for the Memory Window: ");
  gtk_box_pack_start(GTK_BOX(hbox8), lbl_color_key, FALSE, FALSE, 5);
  gtk_widget_show(lbl_color_key);
  gtk_widget_show(hbox8);
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox8, TRUE, TRUE, 3);
  // Color boxes with labels
  hbox9 = gtk_hbox_new(FALSE, 0);
  event_box_last_instr = gtk_event_box_new();
  lbl_eb_last_instr = gtk_label_new("   Prev PC   ");
  gtk_container_add( GTK_CONTAINER(event_box_last_instr), lbl_eb_last_instr);
  event_box_eaddr_read = gtk_event_box_new();
  lbl_eb_eaddr_read = gtk_label_new("   EAddr Read   ");
  gtk_container_add( GTK_CONTAINER(event_box_eaddr_read), lbl_eb_eaddr_read);
  event_box_eaddr_write = gtk_event_box_new();
  lbl_eb_eaddr_write = gtk_label_new("   EAddr Write   ");
  gtk_container_add( GTK_CONTAINER(event_box_eaddr_write), lbl_eb_eaddr_write);
  event_box_mem_read = gtk_event_box_new();
  lbl_eb_mem_read = gtk_label_new("   Mem Read   ");
  gtk_container_add( GTK_CONTAINER(event_box_mem_read), lbl_eb_mem_read);
  event_box_mem_write = gtk_event_box_new();
  lbl_eb_mem_write = gtk_label_new("   Mem Write   ");
  gtk_container_add( GTK_CONTAINER(event_box_mem_write), lbl_eb_mem_write);
  gtk_box_pack_start(GTK_BOX(hbox9), event_box_last_instr, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(hbox9), event_box_eaddr_read, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(hbox9), event_box_eaddr_write, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(hbox9), event_box_mem_read, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(hbox9), event_box_mem_write, FALSE, TRUE, 10);
  // set colors
  gtk_widget_modify_bg(event_box_last_instr, GTK_STATE_NORMAL, &color_last_instr);
  gtk_widget_modify_bg(event_box_eaddr_read, GTK_STATE_NORMAL, &color_eaddr_read);
  gtk_widget_modify_bg(event_box_eaddr_write, GTK_STATE_NORMAL, &color_eaddr_write);
  gtk_widget_modify_bg(event_box_mem_read, GTK_STATE_NORMAL, &color_read);
  gtk_widget_modify_bg(event_box_mem_write, GTK_STATE_NORMAL, &color_write);
  // show event boxes and labels
  gtk_widget_show(event_box_last_instr);
  gtk_widget_show(lbl_eb_last_instr);
  gtk_widget_show(event_box_eaddr_read);
  gtk_widget_show(lbl_eb_eaddr_read);
  gtk_widget_show(event_box_eaddr_write);
  gtk_widget_show(lbl_eb_eaddr_write);
  gtk_widget_show(event_box_mem_read);
  gtk_widget_show(lbl_eb_mem_read);
  gtk_widget_show(event_box_mem_write);
  gtk_widget_show(lbl_eb_mem_write);
  gtk_widget_show(hbox9);
  // put the horizontal box into the vertical vbox2a
  gtk_box_pack_start(GTK_BOX(vbox2a), hbox9, TRUE, TRUE, 3);
  
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
  // Originally was trying to put the column headers above the scrolled window, but
  // was not seeing consistent alignment.
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
  gtk_widget_set_usize(scrolled_window, SCROLLED_WINDOW_WIDTH, SCROLLED_WINDOW_HEIGHT);
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
	  mem_image[i].button_image = gtk_image_new_from_file(BUTTON_IMG_FILE_DEFAULT);
	  // use gtk_image_set_from_file(image) to update
	  gtk_widget_queue_draw(mem_image[i].button_image);
	  // initialize invisible label
	  mem_image[i].button_label = gtk_label_new("");

	  mem_image[i].label_arrow = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
	  // Originally made the breakpoint button a "toggle button" type, 
	  // but switched instead to using a regular button, and handled the
	  // "toggle" action in the button.
	  // Note: Test if can use the toggle button type here.
	  mem_image[i].button_breakpoint = gtk_toggle_button_new();
	  //mem_image[i].button_breakpoint = gtk_button_new(); 
	  gtk_container_add (GTK_CONTAINER(mem_image[i].button_breakpoint), mem_image[i].button_image);
	  mem_image[i].label_addr_octal = gtk_label_new("");
	  mem_image[i].label_addr_hex = gtk_label_new("");
	  mem_image[i].label_value_octal = gtk_label_new("");
	  mem_image[i].label_value_hex = gtk_label_new("");
	  
	  // initialize the event boxes
	  for (j = 0; j < 6; j++) {
		  mem_image[i].event_box[j] = gtk_event_box_new();
	  }
	  
	  // Put the labels and button into the event boxes
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[0]), mem_image[i].label_arrow);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[1]), mem_image[i].button_breakpoint);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[2]), mem_image[i].label_addr_octal);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[3]), mem_image[i].label_addr_hex);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[4]), mem_image[i].label_value_octal);
	  gtk_container_add( GTK_CONTAINER(mem_image[i].event_box[5]), mem_image[i].label_value_hex);

	  // attach "clicked" signal for breakpoint button
	  //gtk_signal_connect (GTK_OBJECT (mem_image[i].button_breakpoint), "clicked", GTK_SIGNAL_FUNC (toggle_breakpoint_button_callback), (gpointer) mem_image[i].button_label);
	  gtk_signal_connect (GTK_OBJECT (mem_image[i].button_breakpoint), "clicked", GTK_SIGNAL_FUNC (toggle_breakpoint_button_callback), (gpointer) mem_image[i].button_label);
	  	  
	  /*
	  // NOTE: Was originally attaching the widgets here in main(), but if it is done here,
	  //       the scrolled window will expand, even though the elements that have been 
	  //       attached are not visible. (It will just scroll through white space.)
	  //       So instead, this attachment section was moved to the display_memory_array()
	  //       function. Requires maintaining a counter of the last row that was attached
	  //       to vtable_mem, so that elements are only attached ONCE to the table.
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
  // Antiquated test: Do not use this, because header labels take up the first three
  // rows (row numbers 0 through 2)
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
  g_signal_connect(G_OBJECT(toolitem_restart), "clicked", GTK_SIGNAL_FUNC(gtk_restart_clicked), NULL);
  // connect to capture original background color
  g_signal_connect (mem_image[0].event_box[0], "realize", G_CALLBACK (widget_realized_bg), NULL);
  
  // - Debug Print Strings:
  // - Debug Flags:
  debug.mem_display = 0;
  debug.instr = 0;
  debug.module = 0;
  debug.eaddr = 0;
  debug.short_mode = 0;
  // Effective Address
  curr_eaddr.EAddr = 0;

  // Read in debug flag settings from param.txt.
  read_param_file(&debug, param_filename);
  
  init_trace_files();

  // Read in memory from input file.
  load_memory_array(&mem_array[0], input_filename, debug.mem_display);
  
  // Display loaded memory in the GUI.
  display_memory_array(&mem_array[0], vtable_mem);

  //gtk_widget_show_all(window);
  gtk_widget_show(window);

  gtk_main();

  return 0;
}

// Update the register value fields in the GUI with the current register
// values.
//void display_reg_values( gtkRegisterValues* gui_curr_reg, guiInstrVals* gui_last_instr )
void display_reg_values()
{
	char buffer[512];
	
	// Set all label values
	if (gui_last_instr.last_PC < PDP8_MEMSIZE) {
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
		// - Last Effective Address
		if (gui_last_instr.flag_eaddr) {
			sprintf(buffer,"%04o",gui_last_instr.last_eaddr);
			gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_eaddr_octal, buffer );
			sprintf(buffer,"%03x",gui_last_instr.last_eaddr);
			gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_eaddr_hex, buffer );
		}
		else {
			gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_eaddr_octal, "" );
			gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_eaddr_hex, "" );
		}
	}
	else {
		// otherwise, this is likely a restart, so clear the "last executed" fields
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_PC_octal, "" );
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_PC_hex, "" );
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_opcode, "" );
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_IR_octal, "" );
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_IR_hex, "" );
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_IR_binary, "" );
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_eaddr_octal, "" );
		gtk_label_set_text( (GtkLabel*) gui_reg_values.label_last_eaddr_hex, "" );
	}
	
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
	// - SR
	sprintf(buffer,"%04o",reg_SR);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_SR_octal, buffer );
	sprintf(buffer,"%03x",reg_SR);
	gtk_label_set_text( (GtkLabel*) gui_reg_values.label_SR_hex, buffer );
}

void display_SR_bit_values() {
	
}

// Display the Memory Table with the current values
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
			//sprintf(str_data, "%04o %04o ", i, k);	// send 4 digits octal

			
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
				gtk_image_set_from_file((GtkImage*) mem_image[k].button_image, BUTTON_IMG_FILE_RED);
				gtk_widget_queue_draw(mem_image[k].button_image);
			}
			else {
				gtk_image_set_from_file((GtkImage*) mem_image[k].button_image, BUTTON_IMG_FILE_DEFAULT);
				gtk_widget_queue_draw(mem_image[k].button_image);
			}
			
			for (j = 0; j < 6; j++) {
				gtk_widget_show (mem_image[k].event_box[j]);
			}
			
			// set color background of all elements in this row
			// if it is the last executed instruction, or has been
			// read or written to in memory in the last instruction
			// check for any other breakpoints that might have been 
			// touched on memory read/write accesses
			if (i == gui_last_instr.last_PC) {
				// set color for last PC
				for (j = 0; j < 6; j++) {
					//gtk_widget_override_background_color (mem_image[k].event_box[j], &color_last_instr);
					gtk_widget_modify_bg(mem_image[k].event_box[j], GTK_STATE_NORMAL, &color_last_instr);
				}
			}
			else if (i == gui_last_instr.index_eaddr_write) {
				// set color for eaddr write
				for (j = 0; j < 6; j++) {
					gtk_widget_modify_bg(mem_image[k].event_box[j], GTK_STATE_NORMAL, &color_eaddr_write);
				}
			}
			else if (i == gui_last_instr.index_eaddr_read) {
				// set color for eaddr read
				for (j = 0; j < 6; j++) {
					gtk_widget_modify_bg(mem_image[k].event_box[j], GTK_STATE_NORMAL, &color_eaddr_read);
				}
			}
			else if (i == gui_last_instr.index_write) {
				// set color for write
				for (j = 0; j < 6; j++) {
					gtk_widget_modify_bg(mem_image[k].event_box[j], GTK_STATE_NORMAL, &color_write);
				}
			}
			else if (i == gui_last_instr.index_read) {
				// set color for read
				for (j = 0; j < 6; j++) {
					gtk_widget_modify_bg(mem_image[k].event_box[j], GTK_STATE_NORMAL, &color_read);
				}
			}
			else {
				// use default color
				for (j = 0; j < 6; j++) {
					//gtk_widget_override_background_color (mem_image[k].event_box[j], &color_original_bg);
					//gtk_widget_modify_bg(mem_image[k].event_box[j], GTK_STATE_NORMAL, &color_original_bg);
					gtk_widget_modify_bg(mem_image[k].event_box[j], GTK_STATE_NORMAL, &color_default_bg);
				}
			}
			
			k++; // increment the display index
		}
	}
	
	// Resize table finally [rows, cols]
	gtk_table_resize (GTK_TABLE(vtable),k,4);
}

// Clears the output trace files.
void init_trace_files() 
{
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

// Function called to execute a set of instructions.
// If the flag_step argument is TRUE, only one instruction is executed.
// If the flag_step argument is FALSE, instructions will be executed
// until either:
//      - a HALT instruction is encountered
//      - the NEXT PC to be executed has a breakpoint
//      - the current instruction has read or written to a memory
//        location that has a breakpoint set; note that this 
//        read/write may occur either in Effective Address calculation
//        or as a memory read/write in the actual execution of 
//        the instruction.
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
	
	// enable the restart button
	gtk_widget_set_sensitive ((GtkWidget*) toolitem_restart, TRUE);
	
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
		
/*
		// Originally was checking for a breakpoint occurring for
		// the current PC; but this allows execution of the instruction
		// for which the breakpoint is set.  Check was moved to the 
		// end of the loop to check for a breakpoint on the next PC.
		
		// check if there was a breakpoint set on this instruction
		// (if so, stop execution AFTER this instruction completes)
		if (mem_array[reg_PC].breakpoint) {
			flag_break = TRUE;
		}
*/		
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
		
		// debug header print for instruction
		if(debug.instr || debug.short_mode) {
			printf("\n================== INSTRUCTION #%-1d : %s ==================\n",stat_instructions, curr_opcode_str);
		}
		
		// STEP 2: CALCULATE EFFECTIVE ADDRESS IF NEEDED
		if ((curr_opcode >= 0) && (curr_opcode <= 5)) {
			curr_eaddr = calc_eaddr(reg_IR, reg_PC, &mem_array[0], fp_tracefile, debug.eaddr, &gui_last_instr);
			effective_address = curr_eaddr.EAddr;
		}
		else {
			// no effective address used.
			gui_last_instr.flag_eaddr = FALSE;
			gui_last_instr.last_eaddr = 0;
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
		strncpy(gui_last_instr.IR_detail, "    0      1     2     3      4     5     6      7     8     9    10    11\n",MAX_IR_DETAIL);
		strncat(gui_last_instr.IR_detail," +----+----+----+----+----+----+----+----+----+----+----+----+\n",MAX_IR_DETAIL);
		sprintf(buffer,"  | %c  |  %c  |  %c  |  %c   |  %c  |  %c  |  %c  |  %c  |  %c  |  %c  |  %c  |  %c  |\n",
			bin_str_IR[0],bin_str_IR[1],bin_str_IR[2],bin_str_IR[3],bin_str_IR[4],bin_str_IR[5],
			bin_str_IR[6],bin_str_IR[7],bin_str_IR[8],bin_str_IR[9],bin_str_IR[10],bin_str_IR[11]);
		strncat(gui_last_instr.IR_detail,buffer,MAX_IR_DETAIL);
		strncat(gui_last_instr.IR_detail," +----+----+----+----+----+----+----+----+----+----+----+----+\n",MAX_IR_DETAIL);
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
							next_AC = reg_AC & (memval_eaddr & word_mask);
							// Debug Print
							if (debug.module) printf("      CURRENT M[EADDR]: [%04o]\n", memval_eaddr);
							if (debug.module) printf("               NEXT AC: [%04o]\n", next_AC);
							// Update Registers
							reg_AC = next_AC & word_mask;
							break;
			case OP_TAD:  	// first, load the value from memory at the effective address
							memval_eaddr = read_mem(effective_address, TF_READ, &mem_array[0], fp_tracefile);
							gui_last_instr.index_read = effective_address;
							// AC = AC + M[EAddr];
							// LR is inverted if the above addition operation results in carry out
							next_AC = (reg_AC & word_mask) + (memval_eaddr & word_mask);
							//if (debug.short_mode) printf("next_AC: %x [%o]\n",next_AC, next_AC);
							if ((next_AC >> PDP8_WORD_SIZE) != 0) {
								next_LR = reg_LR ^ 1; // this will flip the least significant bit of the LR.
							}
							else {
								next_LR = reg_LR;
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
								strncat(gui_last_instr.IR_detail,"                           cla  cll cma cml rar ral 0/1 iac\n",MAX_IR_DETAIL);
							} 
							else if (((reg_IR >> (PDP8_WORD_SIZE - 11-1)) & 1) == 0) {
								// Group 2 micro ops
								if (((reg_IR >> (PDP8_WORD_SIZE - 8-1)) & 1) == 0) { // OR Group
									strncat(gui_last_instr.IR_detail,"                                  cla sma sza snl 0/1 osr hlt\n",MAX_IR_DETAIL);
								}
								else { // AND Group
									strncat(gui_last_instr.IR_detail,"                                  cla spa sna szl 0/1 osr hlt\n",MAX_IR_DETAIL);
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

		// check if there was a breakpoint set on this instruction
		// (if so, stop execution AFTER this instruction completes)
		if (mem_array[reg_PC].breakpoint) {
			flag_break = TRUE;
		}
		else {
			// check for any other breakpoints that might have been 
			// touched on memory read/write accesses
			if (gui_last_instr.index_eaddr_read < PDP8_MEMSIZE) {
				if (mem_array[gui_last_instr.index_eaddr_read].breakpoint) {
					flag_break = TRUE;
				}
			}
			if (gui_last_instr.index_eaddr_write < PDP8_MEMSIZE) {
				if (mem_array[gui_last_instr.index_eaddr_write].breakpoint) {
					flag_break = TRUE;
				}
			}
			if (gui_last_instr.index_read < PDP8_MEMSIZE) {
				if (mem_array[gui_last_instr.index_read].breakpoint) {
					flag_break = TRUE;
				}
			}
			if (gui_last_instr.index_write < PDP8_MEMSIZE) {
				if (mem_array[gui_last_instr.index_write].breakpoint) {
					flag_break = TRUE;
				}
			}
		}

	} while (!flag_HLT && !flag_step && !flag_break);
	// END MAIN LOOP
	//======================================================
	
	// If the halt flag is set, then print statistics,
	// and disable the start and step buttons
	if (flag_HLT) {
		print_statistics();
		
		// disable the step and continue buttons
		gtk_widget_set_sensitive ((GtkWidget*) toolitem_start, FALSE);
		gtk_widget_set_sensitive ((GtkWidget*) toolitem_step, FALSE);
	}
	
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

// function to print statistics
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

// function to clear all statistics variables
// for instruction counts, clock cycles, and 
// branch statistics.
void clear_stats () {
	stat_clocks = 0;
	stat_instructions = 0;
	stat_and = 0;
	stat_tad = 0;
	stat_isz = 0;
	stat_dca = 0;
	stat_jms = 0;
	stat_jmp = 0;
	stat_io = 0;
	stat_ui = 0;
	branch_statistics.total_uncond_t_count = 0;
	branch_statistics.JMS_t_count = 0;
	branch_statistics.JMP_t_count = 0;
	branch_statistics.UI_uncond_t_count = 0;
	branch_statistics.total_cond_t_count = 0;
	branch_statistics.total_cond_nt_count = 0;
	branch_statistics.ISZ_t_count = 0;
	branch_statistics.ISZ_nt_count = 0;
	branch_statistics.UI_cond_t_count = 0;
	branch_statistics.UI_cond_nt_count = 0;
}
