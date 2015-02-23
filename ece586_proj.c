/***********************************************************/
/* Title: ECE 586 Project: PDP-8 Instruction Set Simulator */
/* Date: February 20th, 2015                               */
/***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ece586_proj.h"

//=========================================================
// PARAMETER SECTION
//=========================================================
// GENERAL PARAMETERS:
//---------------------
#define TRUE 1
#define FALSE 0

// TRACE FILE PARAMETERS:
#define TF_READ  0
#define TF_WRITE 1
#define TF_FETCH 2

// ARCHITECTURE PARAMETERS
//-------------------------
// PDP-8 ARCHITECTURE SPECIFICATIONS:
#define PDP8_WORD_SIZE 12		// size of words in memory in bits
#define PDP8_WORDS_PER_PAGE 128 // number of words per page
#define PDP8_PAGE_NUM 32		// number of "pages"
#define PDP8_MEMSIZE PDP8_WORDS_PER_PAGE*PDP8_PAGE_NUM
#define MEM_ARRAY_MAX PDP8_MEMSIZE-1	// max index number of mem array
    // total number of words in memory
#define PDP8_ADDR_SIZE 12	// size of address in bits, hard-coded

// Instruction Opcode Indices: (bits numbered from left to right,
//    starting from the most significant bit as bit 0)
#define INSTR_OP_HIGH 0
#define INSTR_OP_LOW 2

// OPCODE NUMBERS:
#define	OP_AND 0
#define	OP_TAD 1
#define OP_ISZ 2
#define OP_DCA 3
#define OP_JMS 4
#define OP_JMP 5
#define OP_IO  6
#define OP_UI  7
	
// DEFAULT STARTING ADDRESS: (in decimal)
#define DEFAULT_STARTING_ADDRESS 128

// TRACE FILE NAMES:
#define PARAM_FILE_NAME "param.txt"
#define TRACE_FILE_NAME "trace.txt"
#define BRANCH_FILE_NAME "branch_trace.txt"

//---------------------------	
// END PARAMETER SECTION
//=========================================================


// GLOBAL VARIABLES
s_mem_word mem_array [PDP8_MEMSIZE]; // Memory Space
s_debug_flags debug;				// Debug Flags
s_branch_stats branch_statistics; 	// Branch Statistics tracking

// - Debug Print Strings:
char curr_opcode_str [10];
char bin_str_PC [PDP8_ADDR_SIZE];
char bin_str_next_PC [PDP8_ADDR_SIZE];
char bin_str_IR [PDP8_WORD_SIZE];
char bin_str_AC [PDP8_WORD_SIZE];
char bin_str_next_AC [PDP8_WORD_SIZE];
char bin_str_LR [1];
char bin_str_next_LR [1];
char bin_str_SR [PDP8_WORD_SIZE];
char bin_str_next_SR [PDP8_WORD_SIZE];
char bin_str_EAddr [PDP8_ADDR_SIZE];
char bin_str_next_EAddr [PDP8_ADDR_SIZE];
char bin_str_memval_EAddr [PDP8_WORD_SIZE];
char bin_str_next_memval_EAddr [PDP8_WORD_SIZE];
	
int main (int argc, char* argv[])
{
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
	short int next_SR = 0;
	s_updated_vals next_vals;	// struct that holds return values from modules
	// - Flags:
	char flag_HLT = 0; 	// halt flag from group 2 microinstructions
	char flag_NOP = 0;	// flag to indicate a NOP for invalid instruction 
	char flag_GRP2_SKP_UNCOND = 0;	// set to 1 if the SKIP ALL grp 2 micro instruction was given
	char flag_GRP2_SKP = 0;		// set to 1 if any other skip grp 2 microinstruction was given.
	char flag_branch_taken = 0;	// set to 1 if a branch was taken
	// - Debug Flags:
	debug.mem_display = 0;
	debug.instr = 0;
	debug.module = 0;
	debug.eaddr = 0;
	debug.short_mode = 0;
	/*
	char DEBUG_MEM_DISPLAY = 0;	// 1 = TURN ON MEMORY PRINT OUT ON FILE LOAD; 0 = NORMAL OPERATION
	char DEBUG = 0;			// 1 = DEBUG MODE; 0 = NORMAL OPERATION
	char DEBUG_MODULE = 0;	// 1 = TURN ON DEBUG PRINTS IN MODULES; 0 = NORMAL OPERATION
	char DEBUG_EADDR = 0;	// 1 = TURN ON DEBUG PRINTS IN EFFECTIVE ADDRESS CALCULATION; 0 = NORMAL OP
	char DEBUG_SUMMARY = 0;	// short debug print mode
	*/
	
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
	
	// Read in debug flag settings from param.txt.
	read_param_file(&debug, param_filename);
	
	// Read in memory from input file.
	load_memory_array(&mem_array[0], input_filename, debug.mem_display);
	
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
	fprintf(fp_branchtrace,"PC [octal]    BRANCH TYPE   TAKEN/NOT TAKEN    TARGET ADDRESS [octal]\n");
	fprintf(fp_branchtrace,"---------------------------------------------------------------------\n");
	
	//======================================================
	// MAIN LOOP
	while (!flag_HLT) {
		// STEP 1: FETCH INSTRUCTION
		reg_IR = read_mem(reg_PC, TF_FETCH, &mem_array[0], fp_tracefile);
		// set opcode
		curr_opcode = reg_IR >> (PDP8_WORD_SIZE - INSTR_OP_LOW + 1);
		
		// update stat for instruction count
		stat_instructions = stat_instructions + 1;
		
		if(debug.instr) {
			printf("================== INSTRUCTION #%-1d : %s ==================\n",stat_instructions, curr_opcode_str);
		}
		
		// STEP 2: CALCULATE EFFECTIVE ADDRESS IF NEEDED
		curr_eaddr = calc_eaddr(reg_IR, reg_PC, &mem_array[0], fp_tracefile, debug.eaddr);
		effective_address = curr_eaddr.EAddr;
											
		//--------------------------------
		// DEBUG PRINTS OF INITIAL VALUES
		//--------------------------------
		// Set opcode string.
		switch (curr_opcode) {
			case OP_AND:	curr_opcode_str = "AND";
							break;
			case OP_TAD:	curr_opcode_str = "TAD";
							break;
			case OP_ISZ:	curr_opcode_str = "ISZ";
							break;
			case OP_DCA:	curr_opcode_str = "DCA";
							break;
			case OP_JMS:	curr_opcode_str = "JMS";
							break;
			case OP_JMP:	curr_opcode_str = "JMP";
							break;
			case OP_IO:		curr_opcode_str = "IO ";
							break;
			case OP_UI:		curr_opcode_str = "UI ";
							break;
			default:		curr_opcode_str = "ILLEGAL OP";
							break;
		}
		// Get binary values of initial values
		int_to_binary_str(reg_PC, PDP8_ADDR_SIZE, &bin_str_PC);
		int_to_binary_str(reg_IR, PDP8_WORD_SIZE, &bin_str_IR);
		int_to_binary_str(effective_address, PDP8_ADDR_SIZE, &bin_str_EAddr);
		int_to_binary_str(memval_eaddr, PDP8_WORD_SIZE, &bin_str_memval_EAddr);
		int_to_binary_str(reg_AC, PDP8_WORD_SIZE, &bin_str_AC);
		int_to_binary_str(reg_LR, 1, &bin_str_LR);
		int_to_binary_str(reg_SR, PDP8_WORD_SIZE, &bin_str_SR);
		
		if(debug.instr) {
			//printf("================== INSTRUCTION #%-1d : %s ==================\n",stat_instructions, curr_opcode_str);
			printf("  VALUES BEFORE UPDATE:\n");
			printf("                  PC / IR: %s [%4o] / %s [%4o]\n", bin_str_PC, reg_PC, bin_str_IR, reg_IR);
			printf("EFFECTIVE ADDRESS / VALUE: %s [%4o] / %s [%4o]\n", bin_str_EAddr, effective_address, bin_str_memval_EAddr, memval_eaddr);
			printf("       LINK REGISTER [LR]: %1o   ACCUMULATOR [AC]: %s [%4o]\n", bin_str_LR, reg_LR, bin_str_AC, reg_AC);
			printf("     SWITCH REGISTER [SR]: %s [%4o]\n", bin_str_SR, reg_SR);
			//printf("                    IR: %s [%3x]\n", bin_str_IR,reg_IR);
			//printf("                OPCODE: %12.3s [%3x]\n", curr_opcode_str, curr_opcode);
			//printf("     VALUE @ EFF. ADDR: %s [%3x]\n", bin_str_memval_EAddr, memval_eaddr);
			//printf("    LINK REGISTER [LR]: %12s [%3x]\n", bin_str_LR, reg_LR);
			printf("----------------------------------------------------");
		}
		else if(debug.short_mode)	//Shortened version. Only want if the long version is off.  Shows changes caused at current PC by current OP.
		{
			printf("============== PC: %s [%4o] ===============\n", bin_str_PC, reg_PC);
			printf("  %s   LR: %o AC: %s [%4o]\n", curr_opcode_str, reg_LR, bin_str_AC, reg_AC);
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
			printf("-----------------------------------------------------\n");
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
							if (debug.module) printf("               NEXT AC: [%o]", next_AC);
							// Update Registers
							reg_PC = next_PC;
							reg_AC = next_AC;
							break;
			case OP_TAD:  	// first, load the value from memory at the effective address
							memval_eaddr = read_mem(effective_address, TF_READ, &mem_array[0], fp_tracefile);
							// AC = AC + M[EAddr];
							// LR is inverted if the above addition operation results in carry out
							next_AC = reg_AC + memval_eaddr;
							if ((next_AC >> PDP8_WORD_SIZE) != 0) {
								next_LR = LR ^ 1; // this will flip the least significant bit of the LR.
							}
							next_AC = next_AC & word_mask; 	// then clear higher-order bits in next_AC beyond
															// the size of the word
							// Debug Print
							if (debug.module) printf("            NEXT LR/AC: [%o][%o]", next_LR, next_AC);
							// Update Registers
							reg_PC = next_PC;
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
							if (debug.module) printf("               NEXT PC: [%o]", next_PC);
							if (debug.module) printf("         NEXT M[EAddr]: [%o]", next_memval_eaddr);
							// Update Registers
							reg_PC = next_PC;
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_PC, flag_branch_taken, BT_CONDITIONAL, &branch_statistics);
							break;
			case OP_DCA:  	// write AC to memory
							write_mem(effective_address, AC, &mem_array[0], fp_tracefile);
							next_AC = 0;
							// Debug Print
							if (debug.module) printf("               NEXT AC: [%o]", next_AC);
							// Update Registers
							AC = next_AC;
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
		
		// STEP 5: UPDATE STATS
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
			default: printf("WARNING! UNKNOWN OP CODE LR: %o AC: %x [%o]\n", reg_LR, reg_AC, reg_AC);
							break;
		}
		
	}
	// END MAIN LOOP
	//======================================================
/*
//=========================================================
// UPDATE FOR NEXT INSTRUCTION
//---------------------------------------------------------



//---------------------------------------------------------
always@(negedge clk)
begin
	//---------------------------------------------------------	
	//Increment Global Instruction Count Stats
	//---------------------------------------------------------	
	
	
	// Update modified register and memory values depending on outputs
	// from the active opcode module for this instruction.
	case(mem_array[PC][0:2])

		OP_JMS:
			begin
				if(DEBUG)
				begin
					printf("VALUES AFTER UPDATE [OP_JMS]:");
					printf("               NEXT PC: %b [%h]", nextPC4, nextPC4);
					printf("         NEXT M[EAddr]: %b [%h]", next_memval_eaddr4, next_memval_eaddr4);
					printf("----------------------------------------------------");
				end
				
				// write to branch trace file BEFORE update of PC
				branch_check = branchtrace(PC, mem_array[PC][0:2], 1, nextPC4);

				//Update Changes to Registers
				PC = nextPC4;		//Program Counter
				
				// Write new value to memory / update trace file
				writecheck = write_mem(EffectiveAddress, next_memval_eaddr4);
				//tracecheck = writetrace(WRITE, EffectiveAddress);		
			end
		OP_JMP:
			begin
				if(DEBUG)
				begin
					printf("VALUES AFTER UPDATE [OP_JMP]:");
					printf("               NEXT PC: %b [%h]", nextPC5, nextPC5);
					printf("----------------------------------------------------");
				end
				
				// write to branch trace file BEFORE update of PC
				branch_check = branchtrace(PC, mem_array[PC][0:2], 1, nextPC5);
				
				//Update Changes to Registers
				PC = nextPC5;		//Program Counter
				
			end
		OP_IO:
			begin
				if(DEBUG)
				begin
					printf("VALUES AFTER UPDATE [OP_IO]:");
					printf("                  NONE.");
					printf("----------------------------------------------------");
				end
				
				//Update Changes to Registers
				
			end
		default:	//UI
			begin
				if(DEBUG)
				begin
					printf("VALUES AFTER UPDATE [OP_UI]:");
					printf("               NEXT PC: %b [%h]", nextPC7, nextPC7);
					printf("               NEXT AC: %b [%h]", nextAC7, nextAC7);
					printf("               NEXT LR: %12b [%h]", nextLINK7, nextLINK7);  
					printf("               NEXT SR: %b [%h]", nextSR7, nextSR7);
					printf("----------------------------------------------------");
				end
				
				// write to branch trace file BEFORE update of PC,
				// ONLY if a grp 2 skip instruction was specified 
				if (flag_grp2_skip_instr)
					branch_check = branchtrace(PC, mem_array[PC][0:2], flag_branch_taken7, nextPC7);
				
				//Update Changes to Registers
				PC = nextPC7;		//Program Counter
				LINK = nextLINK7;	//Link Bit
				AC = nextAC7;		//Accumulator
				SR = nextSR7;		//Console Switch Register
				
				
			end
	endcase
	
	//--------------------------------------------------
	// Check if the microinstruction for HLT was given,
	// indicated by this flag from the ui module.
	// If HLT, then print out statistics and stop.
	if(flag_HLT7)
	begin
		// print out memory image at end of program
		if(DEBUG_MEM_DISPLAY)
		begin
			printf(" ");
			printf("=====================================================");
			printf(" RESULTING MEMORY: ");
			for(i = 0; i <= mem_array_max; i=i+1)
			begin
				//if(mem_array[i]!=0)
				if (mem_array_valid[i] != 0)
					printf("Address: %3h [%4o] Value: %h [%o]", i, i, mem_array[i],mem_array[i]); 
			end
			printf("=====================================================");
		end
	
		printf(" ");
		//printf("=====================================================");
		printf("\nSTATISTICS");
		printf("CPU Clocks Used: %-1d", stat_clocks);	// left-aligned
		printf("Total Instructions: %-1d", stat_instructions);  // left-aligned
		printf("Number of Instructions by Type:");
		printf("	AND: %d", stat_and);
		printf("	TAD: %d", stat_tad);
		printf("	ISZ: %d", stat_isz);
		printf("	DCA: %d", stat_dca);
		printf("	JMS: %d", stat_jms);
		printf("	JMP: %d", stat_jmp);
		printf("	 IO: %d", stat_io);
		printf("	 UI: %d", stat_ui);
		printf("=====================================================");
		$fclose(tracehandle);
		$fclose(branch_handle);
		$stop;
	end	
	

end
//---------------------------------------------------------
// UPDATE FOR NEXT INSTRUCTION
//=========================================================
*/

	// close the trace files
	fclose(fp_tracefile);
	fclose(fp_branchtrace);

	return 0;
}
