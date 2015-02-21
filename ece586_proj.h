/***********************************************************/
/* Title: ECE 586 Project: PDP-8 Instruction Set Simulator */
/* Date: February 20th, 2015                               */
/* Description: Associated functions.                      */
/***********************************************************/

//=========================================================
// PARAMETER SECTION
//=========================================================
// GENERAL PARAMETERS:
//---------------------
#define TRUE 1
#define FALSE 0

// TRACE FILE PARAMETERS:
//#define FILENAME_MAX_LEN 50  // max number of characters for file name
//#define MAX_LINE_LENGTH 100
#define TF_READ  0
#define TF_WRITE 1
#define TF_FETCH 2

// ARCHITECTURE PARAMETERS
//-------------------------
// PDP-8 ARCHITECTURE SPECIFICATIONS:
// Note: Since these are fixed for the PDP-8 architecture,
//       these numbers were hard-coded in rather than calculating
//       some values relative to others.  
//         i.e. ADDR_PAGE_HIGH is actually log2(PDP8_PAGE_NUM)
#define PDP8_WORD_SIZE 12		// size of words in memory in bits
#define PDP8_WORDS_PER_PAGE 128 // number of words per page
#define PDP8_PAGE_NUM 32		// number of "pages"
#define PDP8_MEMSIZE PDP8_WORDS_PER_PAGE*PDP8_PAGE_NUM
#define MEM_ARRAY_MAX PDP8_MEMSIZE-1	// max index number of mem array
    // total number of words in memory
#define PDP8_ADDR_SIZE 12	// size of address in bits, hard-coded
// Note that addresses are specified with the bits numbered as follows:
//  <    p  a  g  e    > <     o  f  f  s  e  t     >
//  |___|___|___|___|___|___|___|___|___|___|___|___|
//    0   1   2   3   4   5   6   7   8   9  10  11
// Correspondingly, the index bounds for the page and offset bits 
// in the address are:
#define ADDR_PAGE_LOW 0   // upper index for page within address
#define ADDR_PAGE_HIGH 4  // lower index for page within address
#define ADDR_OFFSET_LOW ADDR_PAGE_HIGH + 1 	// upper index for offset within address
#define ADDR_OFFSET_HIGH PDP8_ADDR_SIZE - 1 // lower index for offset within address

// Memory Reference Addressing Mode Bit Positions:
#define ADDR_INDIRECT_ADDR_BIT 3
#define ADDR_MEMORY_PAGE_BIT 4

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

//---------------------------	
// END PARAMETER SECTION
//=========================================================


//=========================================================
// STRUCT DEFINITIONS
//--------------------------------------------------------

// STRUCT DEFINITION for a word in memory.
// Note that since the address size and word size for the PDP-8 
// are both 12 bits, an address and a data word both fit within
// a short int in C.
typedef struct _mem_word {
	short int value;	// value of the word stored at a memory location
	char valid; 		// "valid bit" indicating if the data is valid
} s_mem_word;

// STRUCT definition for return values from modules
typedef struct _updated_vals {
	short int PC;	// Program Counter [0:11]
	short int LR;	// Link Register [0]
	short int AC;	// Accumulator [0:11]
	short int SR;	// Switch Register [0:11]
	short int memval_eaddr;	// Value to be stored in memory at EAddr
	char flag_HLT;	// indicating HLT micro instruction was given
	char flag_NOP;	// indicating illegal instruction was given
	char flag_GRP2_SKP_UNCOND;	// set to 1 if the SKIP ALL grp 2 micro instruction was given
	char flag_GRP2_SKP;		// set to 1 if any other skip grp 2 microinstruction was given.
	char flag_branch_taken;	// set to 1 if a branch was taken
} updated_vals;

// STRUCT definition for debug flags
typedef struct _debug_flags {
	int mem_display;	// 1 = TURN ON MEMORY PRINT OUT ON FILE LOAD; 0 = NORMAL OPERATION
	int instr;			// 1 = DEBUG MODE; 0 = NORMAL OPERATION; long printouts for each instruction
	int module;		// 1 = TURN ON DEBUG PRINTS IN MODULES; 0 = NORMAL OPERATION
	int eaddr;			// 1 = TURN ON DEBUG PRINTS IN EFFECTIVE ADDRESS CALCULATION; 0 = NORMAL OP
	int short_mode;	// short debug print mode
} debug_flags;

//---------------------------	
// END STRUCT DEFINITIONS
//=========================================================



//=========================================================
// FUNCTION NAME: load_memory_array
// Description: Loads data words from the given input file
//    into the corresponding locations in the mem_array.
// Parameters: Pointer to the main memory array, the name
//    of the input file, and a flag indicating if debug 
//    print out of the starting memory values should be
//    printed.
// Return Value: none.
//---------------------------------------------------------	
void load_memory_array(s_mem_word* ptr_mem_array, char* input_filename, char DEBUG_MEM_DISPLAY)
{
	// Variables:
	FILE* fp_inputfile;	// file handle for input file
	char c; // current character
	int i;	// loop counter
	short int curr_addr; // address location to write the current data.
	short int curr_data; // current data to be written.
	
	// all valid bits must be initialized to 0
	for(i = 0; i <= MEM_ARRAY_MAX; i=i+1) {
		(ptr_mem_array+i)->valid = 0;
	}
	
	// Open the input file to read in values to the memory array.
	if ((fp_inputfile = fopen(input_filename, "r")) == NULL) {
		fprintf (stderr, "Unable to open input file %s\n", input_filename);
		exit (1);
	}
		
	c = fgetc(fp_inputfile); // get first char
		
	while (c != EOF) {
		// Check first character for '@', indicating memory address
		if (c == '@') {
			//if (DEBUG) printf("r = %s", r);
			
			curr_addr = 0;
			c = fgetc(fp_inputfile);  // get first digit of address
			
			while ((c != '\n') && (c != '\r') && (c != EOF)) {
				// shift left by 4, before adding next hexadecimal digit.
				curr_addr = curr_addr << 4;
				
				// check if current digit is between 0 and 9
				if ((c >= '0') && (c <= '9')) {
					//if (DEBUG) printf("c = %s [%x]", c, c - "0");
					curr_addr = curr_addr + (c - '0');	// add value
				}
				else // otherwise the digit is between A and F
				{
					// subtract ASCII for "a" before adding the offset of 10
					//if (DEBUG) printf("c = %s [%x]", c, (c - "a" + 10));
					curr_addr = curr_addr + c - 'a' + 10;
				}
				
				c = fgetc(fp_inputfile); // get next character
			}
			
			//if (DEBUG) printf("curr_addr = %x [%o]", curr_addr, curr_addr);
		}
		else {
			// otherwise, this line has a data value, and first
			// octal character has been read.
			
			// initialize current data value to 0.
			curr_data = 0;
			
			while ((c != '\n') && (c != '\r') && (c != EOF)) {
				// shift left by 4, before adding next hexadecimal digit.
				curr_data = curr_data << 4;

				// check if current digit is between 0 and 9
				if ((c >= '0') && (c <= '9')) {
					//if (DEBUG) printf("c = %s [%x]", c, c - "0");
					curr_data = curr_data + (c - '0');	// add value
				}
				else	// otherwise the digit is between A and F
				{
					// subtract ASCII for "a" before adding the offset of 10
					//if (DEBUG) printf("c = %s [%x]", c, (c - "a" + 10));
					curr_data = curr_data + c - 'a' + 10;
				}
				
				c = fgetc(fp_inputfile); // get next character
			}
			
			// set corresponding value in the mem_array and valid bit
			(ptr_mem_array+curr_addr)->value = curr_data;
			(ptr_mem_array+curr_addr)->valid = 1;

//				if(DEBUG_MEM_DISPLAY) $display("Address: %3x [%4o] Value: %x [%o]", curr_addr, curr_addr, curr_data,curr_data);

			// increment the current address
			curr_addr = curr_addr + 1;
		}
		
		c = fgetc(fp_inputfile); // get next character
		// NOTE: This should be the first character on the next line,
		//       since the previous character read should either have been
		//       a newline, carriage return, or EOF.
	}
		
	// print out memory image at beginning of program
	if(DEBUG_MEM_DISPLAY) {
		printf(" \n");
		printf("=====================================================\n");
		printf(" STARTING MEMORY: \n");
		for(i = 0; i <= MEM_ARRAY_MAX; i=i+1) {
			if ((ptr_mem_array+i)->valid != 0)
				printf("Address: %3x [%4o] Value: %3x [%4o]\n", i, i, (ptr_mem_array+i)->value,(ptr_mem_array+i)->value); 
		}
		printf("=====================================================\n");
	}
	
	fclose(fp_inputfile);
}
//---------------------------------------------------------
// END READ INPUT FILE FUNCTION
//=========================================================


//=========================================================
// FUNCTION NAME: read_param_file
// Description: Reads debug flags that have been set in the
//    parameter file. (param.txt)
// Parameters: Pointer to the debug flags struct, the name
//    of the param file.
// Return Value: none.
//---------------------------------------------------------	
void read_param_file(debug_flags* ptr_debug, char* param_filename)
{
	FILE* fp_paramfile;	// file handle for param file
	char c;
	
	// Open the input file to read in debug parameters.
	if ((fp_paramfile = fopen(param_filename, "r")) == NULL) {
		fprintf (stderr, "Unable to open input file %s\n", param_filename);
		exit (1);
	}
	
	//fscanf(fp_paramfile, "%c^\n", &c);
	//ptr_debug->mem_display = atoi(&c);
	fscanf(fp_paramfile, "%d", &(ptr_debug->mem_display));
	printf("DEBUG_MEM_DISPLAY is set to: %d\n", ptr_debug->mem_display);
	fscanf(fp_paramfile, "%d", &(ptr_debug->instr));
	printf("DEBUG is set to: %d\n", ptr_debug->instr);
	fscanf(fp_paramfile, "%d", &(ptr_debug->module));
	printf("DEBUG_MODULE is set to: %d\n", ptr_debug->module);
	fscanf(fp_paramfile, "%d", &(ptr_debug->eaddr));
	printf("DEBUG_EADDR is set to: %d\n", ptr_debug->eaddr);
	fscanf(fp_paramfile, "%d", &(ptr_debug->short_mode));
	printf("UI_DEBUG is set to: %d\n", ptr_debug->short_mode);
	
	fclose(fp_paramfile);
}
//---------------------------------------------------------
// END READ PARAM FILE FUNCTION
//=========================================================


//=========================================================
// FUNCTION NAME: write_branch_trace
// Description: Writes line to branch trace file.
// Parameters: FILE pointer to the branch trace file, 
//    current PC, opcode, target address, flag indicating 
//    if branch was taken, flag indicating type of branch.
// Return Value: none.
//---------------------------------------------------------	
void write_branch_trace(FILE* fp_branchtrace, short int PC, char opcode, short int target_address, char flag_branch_taken, char flag_branch_type) {
	// Append PC, opcode (branch type), Branch T/NT, and target address in trace file
	switch (opcode) {
		case OP_ISZ:
			if (flag_branch_taken)
				fprintf(fp_branchtrace, "%x [%o]    ISZ           TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
			else
				fprintf(fp_branchtrace, "%x [%o]    ISZ           NOT TAKEN          %x [%o]\n", PC, PC, target_address, target_address);
			break;
		
		case OP_JMS:
			fprintf(fp_branchtrace, "%x [%o]    JMS           TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
			break;
		
		case OP_JMP:
			fprintf(fp_branchtrace, "%x [%o]    JMP           TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
			break;
			
		case OP_UI:
			if (flag_branch_taken)
				fprintf(fp_branchtrace, "%x [%o]    MICRO_OPS     TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
			else
				fprintf(fp_branchtrace, "%x [%o]    MICRO_OPS     NOT TAKEN          %x [%o]\n", PC, PC, target_address, target_address);
			break;
	}
}
//---------------------------------------------------------
// END WRITE BRANCH_TRACE FUNCTION
//=========================================================




//=========================================================
// READ FROM MEMORY FUNCTION
//---------------------------------------------------------	
// Function: read_mem
// Description: Given an address and a value, read the
//   given value at the index in the mem_array specified
//   by the given address.  Note that the valid bit for 
//   the specified memory address must also be set.
//   Also must be given a flag for the type of read that 
//   is occurring.
//---------------------------------------------------------
short int read_mem(FILE* fp_tracefile, s_mem_word* ptr_mem_array, short int address, short int value, char read_type) {
	// Append current read_type and address in trace file
	// Note, "read_type" input should be either "TF_READ" or "TF_FETCH".
	
	// Check if the valid bit of the requested address was set. 
	// If not, print a warning in the trace file.
	if ((ptr_mem_array+address)->valid == 1) {
		fprintf(fp_tracefile, "%d %x\n", read_type, address);
	}
	else
	{
		fprintf(fp_tracefile, "%d %x\n", read_type, address);
		fprintf(stderr,"[Warning: Invalid memory location accessed at %x [%o]; read_type: %d\n", address, address, read_type);
	}
	
	return (ptr_mem_array+address)->value;
}
//---------------------------------------------------------
// END READ FROM MEMORY FUNCTION
//=========================================================


//=========================================================
// WRITE TO MEMORY FUNCTION
//---------------------------------------------------------	
// Function: write_mem
// Description: Given an address and a value, store the
//   given value at the index in the mem_array specified
//   by the given address.  Note that the valid bit for 
//   the specified memory address must also be set.
//---------------------------------------------------------
void write_mem(FILE* fp_tracefile, s_mem_word* ptr_mem_array, short int address, short int value) {
	// Append current opcode and address in trace file
	// Note, no "opcode" input, because there is only one WRITE option.
	fprintf(fp_tracefile, "%d %x\n", TF_WRITE, address);

	// update value at given address in the mem_array
	(ptr_mem_array+address)->value = value;
	// set valid bit for the memory location
	(ptr_mem_array+address)->valid = 1;
	
	/*
	if(DEBUG)
	begin
		printf("VERIFY MEMORY WRITE (from function write_mem):\n");
		printf("      UPDATED M[EAddr]: %b [%x]\n", mem_array[address][0:MEM_ARRAY_DATA_HIGH], mem_array[address][0:MEM_ARRAY_DATA_HIGH]);
		printf("----------------------------------------------------\n");
	end
	*/		
}
//---------------------------------------------------------
// END WRITE TO MEMORY FUNCTION
//=========================================================



/*
	//--------------------------------------------------------------------	
	// Calculate next Effective Address:
	//------------------------------------
	// Calculate the effective address for memory reference 
	// instructions (opcodes 0 - 5).  Note that these 
	// instructions are specified as follows:
	//          0   1   2   3   4   5   6   7   8   9  10  11
	//        +---+---+---+---+---+---+---+---+---+---+---+---+
	//        | Op-code   | I | M |      O  F  F  S  E  T     |
	//        +---+---+---+---+---+---+---+---+---+---+---+---+
	//    Bits 0 - 2 : Operation Code
	//    Bit 3 : Indirect Addressing Bit (0:Direct/1:Indirect)
	//    Bit 4 : Memory Page (0:Zero Page/1 Current Page)
	//    Bits 5 - 11 : Offset Address 
	// Notes:
	//    2015-01-23: Preliminary version; need to add function calls for
	//                recording memory accesses in the output trace file.
	//---------------------------------------------------------------------	
	// initialize output flags for indirect and auto-indexing to be false 
	MemType_Indirect = FALSE;
	MemType_AutoIndex = FALSE;
	
	CurrentInstr = mem_array[PC]; // find the current instruction using the PC
  
	// Check if the opcode of the current instruction is a memory reference 
	// instruction (opcodes 0 to 5).
	if ((CurrentInstr[INSTR_OP_LOW:INSTR_OP_HIGH] >= 0) && (CurrentInstr[INSTR_OP_LOW:INSTR_OP_HIGH] <= 5))
	begin
		
	if (DEBUG_EADDR)
		begin
			$display("BEGIN EFFECTIVE ADDRESS CALC FOR NEXT INSTRUCTION:");
			$display("         CURRENT INSTR: %b [%h]", CurrentInstr, CurrentInstr);
			$display(" ");
			$display("   0   1   2   3   4   5   6   7   8   9  10  11");
			$display(" +---+---+---+---+---+---+---+---+---+---+---+---+");
			$display(" | %b | %b | %b | %b | %b | %b | %b | %b | %b | %b | %b | %b |",
				CurrentInstr[0],CurrentInstr[1],CurrentInstr[2],CurrentInstr[3],
				CurrentInstr[4],CurrentInstr[5],CurrentInstr[6],CurrentInstr[7],
				CurrentInstr[8],CurrentInstr[9],CurrentInstr[10],CurrentInstr[11]);
			$display(" +---+---+---+---+---+---+---+---+---+---+---+---+");
			$display(" |  op-code  | I | M |      O  F  F  S  E  T     |");
			$display(" +---+---+---+---+---+---+---+---+---+---+---+---+");
			$display(" ");
		end
		
		// Check if the Indirect Addressing bit set...
		if (CurrentInstr[ADDR_INDIRECT_ADDR_BIT] == 0)
		begin // If Indirect Addressing bit = 0 (Direct addressing) 
			// Check if the Memory Page bit is set...
			if (CurrentInstr[ADDR_MEMORY_PAGE_BIT] == 0)
			begin  
				// Check if the AutoIndex registers are selected.
				//--------------------------------------------
				// ADDRESSING MODE 1: Zero Page Addressing
				//    Effective Address <- 00000 + Offset
				EffectiveAddress = {5'b0, CurrentInstr[ADDR_OFFSET_LOW:ADDR_OFFSET_HIGH]};
				
				if (DEBUG_EADDR)
				begin
					$display("  -- ZERO PAGE ADDRESSING --");
					$display("     EFFECTIVE ADDRESS: %b [%h]", EffectiveAddress, EffectiveAddress);
				end
				//--------------------------------------------
			end
			else // Memory Page bit = 1
			begin  
				//--------------------------------------------
				// ADDRESSING MODE 2: Current Page Addressing
				//    Effective Address <- Old_PC[0:4] + Offset
				EffectiveAddress = {PC[ADDR_PAGE_LOW:ADDR_PAGE_HIGH], CurrentInstr[ADDR_OFFSET_LOW:ADDR_OFFSET_HIGH]};
				
				if (DEBUG_EADDR)
				begin
					$display("  -- CURRENT PAGE ADDRESSING --");
					$display("     EFFECTIVE ADDRESS: %b [%h]", EffectiveAddress, EffectiveAddress);
				end
				//--------------------------------------------
			end
		end // end if for direct addressing
		else // Indirect Addressing bit = 1
		begin
			// Check if locations outside of 0010 through 0017 (octal) have been indicated
			if (!((CurrentInstr[ADDR_OFFSET_LOW:ADDR_OFFSET_HIGH] >= 8) && (CurrentInstr[ADDR_OFFSET_LOW:ADDR_OFFSET_HIGH] < 15)))
			begin
				//--------------------------------------------
				// ADDRESSING MODE 3: Indirect Addressing
				if (CurrentInstr[ADDR_MEMORY_PAGE_BIT] == 0)
				begin  // If Zero Page Addressing, then: 
					//    Effective Address <- C(00000 + Offset)
					IndirectAddrLoc = {5'b0, CurrentInstr[ADDR_OFFSET_LOW:ADDR_OFFSET_HIGH]};
					EffectiveAddress = mem_array[IndirectAddrLoc];
					//FUNCTION CALL TO READMEM FOR LOG
					tracecheck = writetrace(READ, IndirectAddrLoc);	
					
					if (DEBUG_EADDR)
					begin
						$display("  -- INDIRECT ADDRESSING; ZERO PAGE --");
						$display("INDIRECT ADDR LOCATION: %b [%h]", IndirectAddrLoc, IndirectAddrLoc);
						$display("     EFFECTIVE ADDRESS: %b [%h]", EffectiveAddress, EffectiveAddress);
					end
				end
				else
				begin  // If Current Page Addressing, then:
					//    Effective Address <- C(Old_PC[0:4] + Offset)
					IndirectAddrLoc = {PC[ADDR_PAGE_LOW:ADDR_PAGE_HIGH], CurrentInstr[ADDR_OFFSET_LOW:ADDR_OFFSET_HIGH]};
					EffectiveAddress = mem_array[IndirectAddrLoc];
					//FUNCTION CALL TO READMEM FOR LOG
					readcheck = read_mem(READ, IndirectAddrLoc, 0);
					//tracecheck = writetrace(READ, IndirectAddrLoc);	
					
					if (DEBUG_EADDR)
					begin
						$display("  -- INDIRECT ADDRESSING; CURRENT PAGE --");
						$display("INDIRECT ADDR LOCATION: %b [%h]", IndirectAddrLoc, IndirectAddrLoc);
						$display("     EFFECTIVE ADDRESS: %b [%h]", EffectiveAddress, EffectiveAddress);
					end
				end
			  
			 // set flag to indicate indirect addressing was used
			 MemType_Indirect = TRUE;
			end
			else // offset location from 0010 through 0017 (octal) was specified
			begin
				//--------------------------------------------
				// ADDRESSING MODE 4: AutoIndex Addressing
				//      C(AutoIndex_Register) <- C(AutoIndex_Register) + 1
				//      EffectiveAddress <- C(AutoIndex_Register)
				IndirectAddrLoc = {5'b0, CurrentInstr[ADDR_OFFSET_LOW:ADDR_OFFSET_HIGH]};

				if (DEBUG_EADDR)
				begin
					$display("  -- AUTO-INDEX ADDRESSING --");
					$display("INDIRECT ADDR LOCATION: %b [%h]", IndirectAddrLoc, IndirectAddrLoc);
					$display("  Orig M[IndirectAddr]: %b [%h]", mem_array[IndirectAddrLoc], mem_array[IndirectAddrLoc]);
				end
				
				EffectiveAddress = mem_array[IndirectAddrLoc] + 1; // read mem val and increment by 1
				//FUNCTION CALL TO READMEM FOR LOG
				readcheck = read_mem(READ, IndirectAddrLoc, 0);
				//tracecheck = writetrace(READ, IndirectAddrLoc);	
				
				// Write new value to memory
				writecheck = write_mem(IndirectAddrLoc, EffectiveAddress);
				//mem_array[IndirectAddrLoc] = EffectiveAddress; // update value in mem
				
				//FUNCTION CALL TO WRITEMEM FOR LOG
				//tracecheck = writetrace(WRITE, IndirectAddrLoc);	
				
				// set flag to indicate auto-index addressing was used
				MemType_AutoIndex = TRUE;
				
				
				if (DEBUG_EADDR)
				begin
					$display("     EFFECTIVE ADDRESS: %b [%h]", EffectiveAddress, EffectiveAddress);
				end
			end
		end // end else where indirect addressing bit = 1
		
		if (DEBUG_EADDR)
		begin
			$display("----------------------------------------------------");
		end
		
		// get the value in memory at the EAddr, which may be used
		// by the instruction, depending on the opcode.  note that
		// recording the memory read/write from this effective address
		// in the tracefile is performed on the negedge of clock at
		// the same time that the changes to the registers/memory
		// are recorded for the current opcode.  This just obtains
		// the memory value for input to the opcode modules.
		memval_eaddr = mem_array[EffectiveAddress];
		
	end // end if block where opcode of the current instruction is a memory reference instr.
	else // otherwise don't use an effective address.
	begin
		EffectiveAddress = 'x;
		memval_eaddr = 'x;
	end
*/