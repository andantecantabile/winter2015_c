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

// Instruction Opcode Indices:
#define INSTR_OP_LOW 0
#define INSTR_OP_HIGH 2

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
debug_flags debug;	// Debug Flags
	
int main (int argc, char* argv[])
{
	// VARIABLES:
	// - Memory and Effective Address:
	short int effective_address;	// Effective Address
	short int memval_eaddr;			// Value of the word located in memory at the effective address
	short int next_memval_eaddr;	// Value to be written to the memory location specified by the effective address
	// - Registers: Current and Next
	short int reg_PC = DEFAULT_STARTING_ADDRESS;	// Program Counter [0:11]
	char reg_LR = 0;		// Link Register [0] (One bit only!)
	short int reg_AC = 0;	// Accumulator [0:11]
	short int reg_SR = 0;	// Console Switch Register [0:11]
	updated_vals next_vals;	// struct that holds return values from modules
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
	load_memory_array(&mem_array, input_filename, debug.mem_display);
	
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
	
/*
//=========================================================
// UPDATE FOR NEXT INSTRUCTION
//---------------------------------------------------------
//Statistics
integer stat_instructions = 0;
integer stat_clocks = 0;
integer stat_and = 0;
integer stat_tad = 0;
integer stat_isz = 0;
integer stat_dca = 0;
integer stat_jms = 0;
integer stat_jmp = 0;
integer stat_io = 0;
integer stat_ui = 0;

reg MemType_Indirect, MemType_AutoIndex;
// instruction indicated by the current value of the PC (program counter)
reg [0:PDP8_WORD_SIZE-1] CurrentInstr;
// location in memory of the effective address (for indirect addressing)
reg [0:PDP8_ADDR_SIZE-1] IndirectAddrLoc;
reg [0:2]prevOp;
//---------------------------------------------------------
always@(negedge clk)
begin
	//---------------------------------------------------------	
	//Increment Global Instruction Count Stats
	//---------------------------------------------------------	
	stat_instructions = stat_instructions + 1;
	//tracecheck = writetrace(FETCH, PC);	
	readcheck = read_mem(FETCH, PC, 0);
	
	//---------------------------------------------------------	
	//Update Instruction Specific Stats, Register changes, and Tracefile
	//---------------------------------------------------------	
	if(DEBUG)
	begin
		$display("================== INSTRUCTION #%-1d ==================",stat_instructions);
		$display("  VALUES BEFORE UPDATE:");
		$display("                    PC: %b [%h]", PC,PC);
		$display("                OPCODE: %12.3b [%h]", mem_array[PC][0:2], mem_array[PC][0:2]);
		$display("     EFFECTIVE ADDRESS: %b [%h]", EffectiveAddress, EffectiveAddress);
		$display("     VALUE @ EFF. ADDR: %b [%h]", memval_eaddr, memval_eaddr);
		$display("      ACCUMULATOR [AC]: %b [%h]", AC, AC);
		$display("    LINK REGISTER [LR]: %12b [%h]", LINK, LINK);
		$display("  SWITCH REGISTER [SR]: %b [%h]", SR, SR);
		$display("----------------------------------------------------");
	end
	else if(UI_DEBUG)	//Shortened version. Only want if the long version is off.  Shows changes caused at current PC by current OP.
	begin
		$display("============== PC: %b [%h] ===============",PC, PC);
		case (mem_array[PC][0:2])
			OP_AND:  $display("  AND    LR: %b AC: %b [%h]", LINK, nextAC0, nextAC0);
			OP_TAD:  $display("  TAD    LR: %b AC: %b [%h]", nextLINK1, nextAC1, nextAC1);
			OP_ISZ:  $display("  ISZ    LR: %b AC: %b [%h]", LINK, AC, AC);
			OP_DCA:  $display("  DCA    LR: %b AC: %b [%h]", LINK, nextAC3, nextAC3);
			OP_JMS:  $display("  JMS    LR: %b AC: %b [%h]", LINK, AC, AC);
			OP_JMP:  $display("  JMP    LR: %b AC: %b [%h]", LINK, AC, AC);
			OP_IO:   $display("  IO     LR: %b AC: %b [%h]", LINK, nextAC6, nextAC6);
			OP_UI:   $display("  UI     LR: %b AC: %b [%h]", nextLINK7, nextAC7, nextAC7);
			default: $display("WARNING! UKNOWN OP CODE LR: %b AC: %b [%h]", LINK, AC, AC);
		endcase
		$display("-----------------------------------------------------\n");
	end
	
	
	
	// Update modified register and memory values depending on outputs
	// from the active opcode module for this instruction.
	case(mem_array[PC][0:2])
		OP_AND:
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_AND]:");
				//	$display("                    PC: %b", PC);
				//	$display("     EFFECTIVE ADDRESS: %h", EffectiveAddress);
					$display("               NEXT PC: %b [%h]", nextPC0, nextPC0);
					$display("               NEXT AC: %b [%h]", nextAC0, nextAC0);
					$display("----------------------------------------------------");
				end

				//Update Changes to Registers
				PC = nextPC0;		//Program Counter
				AC = nextAC0;		//Accumulator
								
				stat_and = stat_and + 1;
				stat_clocks = stat_clocks + 2;	
				if (MemType_AutoIndex)
					stat_clocks = stat_clocks + 2;
				if (MemType_Indirect)
					stat_clocks = stat_clocks + 1;
				//tracecheck = writetrace(READ, EffectiveAddress);
				readcheck = read_mem(READ, EffectiveAddress, 0);
			end
		OP_TAD:
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_TAD]:");
					$display("               NEXT PC: %b [%h]", nextPC1, nextPC1);
					$display("               NEXT AC: %b [%h]", nextAC1, nextAC1);
					$display("               NEXT LR: %12b [%h]", nextLINK1, nextLINK1);
					$display("----------------------------------------------------");
				end

				//Update Changes to Registers
				PC = nextPC1;		//Program Counter
				LINK = nextLINK1;	//Link Bit
				AC = nextAC1;		//Accumulator
				
				stat_tad = stat_tad + 1;
				stat_clocks = stat_clocks + 2;
				if (MemType_AutoIndex)
					stat_clocks = stat_clocks + 2;
				if (MemType_Indirect)
					stat_clocks = stat_clocks + 1;
				//tracecheck = writetrace(READ, EffectiveAddress);	
				readcheck = read_mem(READ, EffectiveAddress, 0);
			end
		OP_ISZ:
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_ISZ]:");
					$display("               NEXT PC: %b [%h]", nextPC2, nextPC2);
					$display("         NEXT M[EAddr]: %b [%h]", next_memval_eaddr2, next_memval_eaddr2);
					$display("----------------------------------------------------");
				end
			
				// write to branch trace file BEFORE update of PC
				branch_check = branchtrace(PC, mem_array[PC][0:2], flag_branch_taken2, nextPC2);
			
				// Update Changes to Registers
				PC = nextPC2;		//Program Counter
				
				stat_isz = stat_isz + 1;
				stat_clocks = stat_clocks + 2;
				if (MemType_AutoIndex)
					stat_clocks = stat_clocks + 2;
				if (MemType_Indirect)
					stat_clocks = stat_clocks + 1;
				
				// write data to memory / update trace file
				readcheck = read_mem(READ, EffectiveAddress, 0);
				writecheck = write_mem(EffectiveAddress, next_memval_eaddr2);
				//tracecheck = writetrace(READ, EffectiveAddress);	
				//tracecheck = writetrace(WRITE, EffectiveAddress);	
			end
		OP_DCA:
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_DCA]:");
					$display("               NEXT PC: %b [%h]", nextPC3, nextPC3);
					$display("               NEXT AC: %b [%h]", nextAC3, nextAC3);
					$display("         NEXT M[EAddr]: %b [%h]", next_memval_eaddr3, next_memval_eaddr3);
					$display("----------------------------------------------------");
				end

				//Update Changes to Registers
				AC = nextAC3;		//Accumulator
				PC = nextPC3;		// Program Counter
				
				stat_dca = stat_dca + 1;
				stat_clocks = stat_clocks + 2;
				if (MemType_AutoIndex)
					stat_clocks = stat_clocks + 2;
				if (MemType_Indirect)
					stat_clocks = stat_clocks + 1;
					
				// Write new value to memory / update trace file
				writecheck = write_mem(EffectiveAddress, next_memval_eaddr3);
				//tracecheck = writetrace(WRITE, EffectiveAddress);	
			end
		OP_JMS:
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_JMS]:");
					$display("               NEXT PC: %b [%h]", nextPC4, nextPC4);
					$display("         NEXT M[EAddr]: %b [%h]", next_memval_eaddr4, next_memval_eaddr4);
					$display("----------------------------------------------------");
				end
				
				// write to branch trace file BEFORE update of PC
				branch_check = branchtrace(PC, mem_array[PC][0:2], 1, nextPC4);

				//Update Changes to Registers
				PC = nextPC4;		//Program Counter
				
				stat_jms = stat_jms + 1;
				stat_clocks = stat_clocks + 2;
				if (MemType_AutoIndex)
					stat_clocks = stat_clocks + 2;
				if (MemType_Indirect)
					stat_clocks = stat_clocks + 1;
				
				// Write new value to memory / update trace file
				writecheck = write_mem(EffectiveAddress, next_memval_eaddr4);
				//tracecheck = writetrace(WRITE, EffectiveAddress);		
			end
		OP_JMP:
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_JMP]:");
					$display("               NEXT PC: %b [%h]", nextPC5, nextPC5);
					$display("----------------------------------------------------");
				end
				
				// write to branch trace file BEFORE update of PC
				branch_check = branchtrace(PC, mem_array[PC][0:2], 1, nextPC5);
				
				//Update Changes to Registers
				PC = nextPC5;		//Program Counter
				
				stat_jmp = stat_jmp + 1;
				stat_clocks = stat_clocks + 1;	
			end
		OP_IO:
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_IO]:");
					$display("                  NONE.");
					$display("----------------------------------------------------");
				end
				
				//Update Changes to Registers
				
				stat_io = stat_io + 1;
			end
		default:	//UI
			begin
				if(DEBUG)
				begin
					$display("VALUES AFTER UPDATE [OP_UI]:");
					$display("               NEXT PC: %b [%h]", nextPC7, nextPC7);
					$display("               NEXT AC: %b [%h]", nextAC7, nextAC7);
					$display("               NEXT LR: %12b [%h]", nextLINK7, nextLINK7);  
					$display("               NEXT SR: %b [%h]", nextSR7, nextSR7);
					$display("----------------------------------------------------");
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
				
				stat_ui = stat_ui + 1;
				stat_clocks = stat_clocks + 1;
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
			$display(" ");
			$display("=====================================================");
			$display(" RESULTING MEMORY: ");
			for(i = 0; i <= mem_array_max; i=i+1)
			begin
				//if(mem_array[i]!=0)
				if (mem_array_valid[i] != 0)
					$display("Address: %3h [%4o] Value: %h [%o]", i, i, mem_array[i],mem_array[i]); 
			end
			$display("=====================================================");
		end
	
		$display(" ");
		//$display("=====================================================");
		$display("\nSTATISTICS");
		$display("CPU Clocks Used: %-1d", stat_clocks);	// left-aligned
		$display("Total Instructions: %-1d", stat_instructions);  // left-aligned
		$display("Number of Instructions by Type:");
		$display("	AND: %d", stat_and);
		$display("	TAD: %d", stat_tad);
		$display("	ISZ: %d", stat_isz);
		$display("	DCA: %d", stat_dca);
		$display("	JMS: %d", stat_jms);
		$display("	JMP: %d", stat_jmp);
		$display("	 IO: %d", stat_io);
		$display("	 UI: %d", stat_ui);
		$display("=====================================================");
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
