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
/*
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
*/
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


	
int main (int argc, char* argv[])
{
	// VARIABLES:
	// - Memory and Effective Address:
	short int effective_address;	// Effective Address
	short int memval_eaddr;			// Value of the word located in memory at the effective address
	short int next_memval_eaddr;	// Value to be written to the memory location specified by the effective address
	s_effective_addr curr_eaddr;	// Return values of calculated effective address, with flags for statistics.
	curr_eaddr.EAddr = 0;
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

	// - Debug Print Strings:
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
			printf("                  PC / IR: %s [%4o] / %s [%4o]\n", bin_str_PC, reg_PC, bin_str_IR, reg_IR);
			printf("        EFFECTIVE ADDRESS: %s [%o] \n", bin_str_EAddr, effective_address);
			//printf("EFFECTIVE ADDRESS / VALUE: %s [%4o] / %s [%4o]\n", bin_str_EAddr, effective_address, bin_str_memval_EAddr, memval_eaddr);
			printf("       LINK REGISTER [LR]: %1o   ACCUMULATOR [AC]: %s [%4o]\n", reg_LR, bin_str_AC, reg_AC);
			printf("     SWITCH REGISTER [SR]: %s [%4o]\n", bin_str_SR, reg_SR);
			//printf("                    IR: %s [%3x]\n", bin_str_IR,reg_IR);
			//printf("                OPCODE: %12.3s [%3x]\n", curr_opcode_str, curr_opcode);
			//printf("     VALUE @ EFF. ADDR: %s [%3x]\n", bin_str_memval_EAddr, memval_eaddr);
			//printf("    LINK REGISTER [LR]: %12s [%3x]\n", bin_str_LR, reg_LR);
			printf("----------------------------------------------------\n");
		}
		else if(debug.short_mode)	//Shortened version. Only want if the long version is off.  Shows changes caused at current PC by current OP.
		{
			printf("              PC: %s [%4o]\n", bin_str_PC, reg_PC);
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
							if (debug.module) printf("      CURRENT M[EADDR]: [%o]\n", memval_eaddr);
							if (debug.module) printf("               NEXT AC: [%o]\n", next_AC);
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
							if (debug.module) printf("            NEXT LR/AC: [%o][%o]\n", next_LR, next_AC);
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
							if (debug.module) printf("               NEXT PC: [%o]\n", next_PC);
							if (debug.module) printf("         NEXT M[EAddr]: [%o]\n", next_memval_eaddr);
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_PC, flag_branch_taken, BT_CONDITIONAL, &branch_statistics);
							// Update Registers
							break;
			case OP_DCA:  	// write AC to memory
							write_mem(effective_address, reg_AC, &mem_array[0], fp_tracefile);
							next_AC = 0;
							// Debug Print
							if (debug.module) printf("               NEXT AC: [%o]\n", next_AC);
							// Update Registers
							reg_AC = next_AC;
							break;
			case OP_JMS:  	next_memval_eaddr = next_PC;
							next_PC = (effective_address + 1) & address_mask;
							// write updated value to memory
							write_mem(effective_address, next_memval_eaddr, &mem_array[0], fp_tracefile);
							// Debug Print
							if (debug.module) printf("               NEXT PC: [%o]\n", next_PC);
							if (debug.module) printf("         NEXT M[EAddr]: [%o]\n", next_memval_eaddr);
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_PC, BT_TAKEN, BT_UNCONDITIONAL, &branch_statistics);
							// Update Registers
							break;
			case OP_JMP:  	next_PC = effective_address;
							// Debug Print
							if (debug.module) printf("               NEXT PC: [%o]\n", next_PC);
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_PC, BT_TAKEN, BT_UNCONDITIONAL, &branch_statistics);
							// Update Registers
							break;
			case OP_IO:   	// Not implemented.
							fprintf(stderr,"WARNING: IO instruction encountered at PC: [%o]\n",reg_PC);
							// Update Registers
							break;
			case OP_UI:   	// Obtain next values from the UI subroutine
							// Note that next_PC is passed as an argument here since PC has been pre-incremented.
							next_vals = module_UI(reg_IR, next_PC, reg_AC, reg_LR, reg_SR, debug.module);
							
							// Debug Print
							if (debug.module) {
								printf("               NEXT PC: [%o]\n", next_vals.PC);
								printf("            NEXT LR/AC: %x/%x [%o/%o]\n",next_vals.LR,next_vals.AC,next_vals.LR,next_vals.AC);
							}
							// update branch statistics, and write to branch trace file
							write_branch_trace(fp_branchtrace, reg_PC, curr_opcode, next_vals.PC, next_vals.flag_branch_taken, next_vals.flag_branch_type, &branch_statistics);
							next_PC = next_vals.PC;
							// Update Registers
							reg_AC = next_vals.AC;
							reg_LR = next_vals.LR;
							flag_HLT = next_vals.flag_HLT;
							break;
			default: 		fprintf(stderr,"WARNING! UNKNOWN OP CODE: %d LR: %o AC: %x [%o]\n", curr_opcode, reg_LR, reg_AC, reg_AC);
							break;
		}
		
		if (debug.short_mode) {
			// get binary string of the updated AC
			int_to_binary_str(reg_AC, PDP8_WORD_SIZE, (char**)&bin_str_AC);
			// print updated values after executing current instruction
			printf("  %s   LR: %o AC: %s [%4o] AFTER EXECUTION\n", curr_opcode_str, reg_LR, bin_str_AC, reg_AC);
			printf("-----------------------------------------------------\n");
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
			default: printf("WARNING! UNKNOWN OP CODE LR: %o AC: %x [%o]\n", reg_LR, reg_AC, reg_AC);
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
					printf("Address: %3x [%4o] Value: %3x [%4o]\n", i, i, mem_array[i].value,mem_array[i].value); 
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

	return 0;
}
