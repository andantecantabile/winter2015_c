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

// BRANCH TYPE PARAMETERS:
#define BT_NO_BRANCH     0
#define BT_UNCONDITIONAL 1
#define BT_CONDITIONAL   2

// BRANCH TAKEN PARAMETERS:
#define BT_NOT_TAKEN 0
#define BT_TAKEN     1

// ARCHITECTURE PARAMETERS
//-------------------------
// PDP-8 ARCHITECTURE SPECIFICATIONS:
// Note: Since these are fixed for the PDP-8 architecture,
//       these numbers were hard-coded in rather than calculating
//       some values relative to others.  
//         i.e. ADDR_PAGE_HIGH + 1 is actually log2(PDP8_PAGE_NUM)
#define PDP8_WORD_SIZE 12		// size of words in memory in bits
#define PDP8_WORDS_PER_PAGE 128 // number of words per page
#define PDP8_PAGE_NUM 32		// number of "pages"
#define PDP8_MEMSIZE PDP8_WORDS_PER_PAGE*PDP8_PAGE_NUM
#define MEM_ARRAY_MAX PDP8_MEMSIZE-1	// max index number of mem array
    // total number of words in memory
#define PDP8_ADDR_SIZE 12	// size of address in bits, hard-coded
#define WORD_STRING_SIZE PDP8_WORD_SIZE+1
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

// STRUCT definition for return values from effective address module
typedef struct _effective_addr {
	short int EAddr;	// Effective Address 
	char flag_MemType_Indirect;		// flag for indirect memory reference
	char flag_MemType_AutoIndex;	// flag for autoindex memory reference
} s_effective_addr;

// STRUCT definition for return values from modules
typedef struct _updated_vals {
	short int PC;	// Program Counter [0:11]
	short int LR;	// Link Register [0]
	short int AC;	// Accumulator [0:11]
	short int SR;	// Switch Register [0:11]
//	short int memval_eaddr;	// Value to be stored in memory at EAddr
	char flag_HLT;	// indicating HLT micro instruction was given
	char flag_NOP;	// indicating illegal instruction was given
	char flag_branch_type;	// set to 0 if there was no branch, 1 for unconditional, 2 for conditional.
	char flag_branch_taken;	// set to 1 if a branch was taken
} s_updated_vals;

// STRUCT definition for debug flags
typedef struct _debug_flags {
	int mem_display;	// 1 = TURN ON MEMORY PRINT OUT ON FILE LOAD; 0 = NORMAL OPERATION
	int instr;			// 1 = DEBUG MODE; 0 = NORMAL OPERATION; long printouts for each instruction
	int module;		// 1 = TURN ON DEBUG PRINTS IN MODULES; 0 = NORMAL OPERATION
	int eaddr;			// 1 = TURN ON DEBUG PRINTS IN EFFECTIVE ADDRESS CALCULATION; 0 = NORMAL OP
	int short_mode;	// short debug print mode
} s_debug_flags;

// STRUCT definition for branch statistics
typedef struct _branch_stats {
	int total_cond_t_count;		// total number of taken conditional branches
	int total_cond_nt_count;	// total number of not-taken conditional branches
	int total_uncond_t_count;	// total number of taken unconditional branches
	int ISZ_t_count;			// number of ISZ branches taken
	int JMS_t_count;			// number of JMS branches taken (unconditional)
	int JMP_t_count;			// number of JMP branches taken (unconditional)
	int UI_cond_t_count;		// number of UI conditional branches taken
	int UI_uncond_t_count;	// number of UI unconditional (SKP) branches taken
	int ISZ_nt_count;			// number of ISZ branches NOT taken
	//int JMS_nt_count;			// number of JMS branches NOT taken
	//int JMP_nt_count;			// number of JMP branches NOT taken
	int UI_cond_nt_count;		// number of UI conditional branches NOT taken
	int UI_uncond_nt_count;	// number of UI unconditional (SKP) branches NOT taken
} s_branch_stats;

//---------------------------	
// END STRUCT DEFINITIONS
//=========================================================


//=========================================================
// FUNCTION NAME: int_to_binary_str
// Description: For a given int, returns a binary string.
// Parameters: Integer to be converted to binary, the
//    maximum number of characters to be returned, and 
//    pointer to the string to be written to.
// Return Value: No direct return value; the binary string
//    is returned by reference in the argument list.
//---------------------------------------------------------	
void int_to_binary_str(int x, int max_digits, char* str_binary)
{
	// y is a comparison value, initialized to the integer
	// number that has a '1' in the most significant bit
	// position, with all following bits as '0'.
	//int y = 2^(max_digits-1);
	int y = 1 << (max_digits-1);
	int i = 0; // offset into the char[] array

	if (max_digits > 0) {
		for (; y > 0; y = y >> 1)
		{
			if ((x & y) == y) {
				*(str_binary+i) = '1';
			}
			else {
				*(str_binary+i) = '0';
			}
			i++; // increment the array position
		}
	}
}


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

//				if(DEBUG_MEM_DISPLAY) printf("Address: %3x [%4o] Value: %x [%o]", curr_addr, curr_addr, curr_data,curr_data);

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
void read_param_file(s_debug_flags* ptr_debug, char* param_filename)
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
void write_branch_trace(FILE* fp_branchtrace, short int PC, char opcode, short int target_address, 
	char flag_branch_taken, char flag_branch_type, s_branch_stats* branch_stats) {
	// Append PC, opcode (branch type), Branch T/NT, and target address in trace file
	switch (opcode) {
		case OP_ISZ:
			if (flag_branch_taken == BT_TAKEN) {
				fprintf(fp_branchtrace, "%x [%o]    CONDITIONAL [ISZ]   TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
				(branch_stats->ISZ_t_count)++;
				(branch_stats->total_cond_t_count)++;
			}
			else {
				fprintf(fp_branchtrace, "%x [%o]    CONDITIONAL [ISZ]   NOT TAKEN          %x [%o]\n", PC, PC, target_address, target_address);
				(branch_stats->ISZ_nt_count)++;
				(branch_stats->total_cond_nt_count)++;
			}
			break;
		
		case OP_JMS:
			fprintf(fp_branchtrace, "%x [%o]    UNCONDITIONAL [JMS] TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
			(branch_stats->JMS_t_count)++;
			(branch_stats->total_uncond_t_count)++;
			break;
		
		case OP_JMP:
			fprintf(fp_branchtrace, "%x [%o]    UNCONDITIONAL [JMP] TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
			(branch_stats->JMP_t_count)++;
			(branch_stats->total_uncond_t_count)++;
			break;
			
		case OP_UI:
			if (flag_branch_type == BT_UNCONDITIONAL) {
				if (flag_branch_taken == BT_TAKEN) {
					fprintf(fp_branchtrace, "%x [%o]    UNCONDITIONAL [UI]  TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
					(branch_stats->UI_uncond_t_count)++;
					(branch_stats->total_uncond_t_count)++;
				}
				else {
					fprintf(fp_branchtrace, "%x [%o]    UNCONDITIONAL [UI]  NOT TAKEN          %x [%o]\n", PC, PC, target_address, target_address);
					(branch_stats->UI_uncond_nt_count)++; // NOTE: This should never be incremented. If it is, there is an error somewhere!
				}
			}
			else if (flag_branch_type == BT_CONDITIONAL)
			{
				if (flag_branch_taken == BT_TAKEN) {
					fprintf(fp_branchtrace, "%x [%o]    CONDITIONAL [UI]    TAKEN              %x [%o]\n", PC, PC, target_address, target_address);
					(branch_stats->UI_cond_t_count)++;
					(branch_stats->total_cond_t_count)++;
				}
				else {
					fprintf(fp_branchtrace, "%x [%o]    CONDITIONAL [UI]    NOT TAKEN          %x [%o]\n", PC, PC, target_address, target_address);
					(branch_stats->UI_cond_nt_count)++;
					(branch_stats->total_cond_nt_count)++;
				}
			}
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
// Description: Given an address, read the word located
//   at the index in the mem_array specified by the
//   given address.  Note that the valid bit for 
//   the specified memory address must also be set.
//   Also must be given a flag for the type of read that 
//   is occurring.
//---------------------------------------------------------
short int read_mem(short int address, char read_type, s_mem_word* ptr_mem_array, FILE* fp_tracefile) {
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
void write_mem(short int address, short int value, s_mem_word* ptr_mem_array, FILE* fp_tracefile) {
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


//=========================================================
// CALCULATE EFFECTIVE ADDRESS FUNCTION
//---------------------------------------------------------	
// Function: calc_eaddr
// Description: Given the current instruction, calculates
//    the effective address to be used.  Also will set flags
//    indicating if indirect addressing or auto-index mode
//    was used in calculating the effective address.  
//    Second parameter is the program counter (needed to
//    determine the current page).
//    Third and fourth parameters are the pointer to the
//    mem_array and file pointer to the trace file, to be
//    passed along to write_mem() if needed.
//    Last parameter is a flag for debug prints.
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
// Note: Uses the ADDR_INDIRECT_ADDR_BIT and the
//    ADDR_MEMORY_PAGE_BIT defined above.
//---------------------------------------------------------
s_effective_addr calc_eaddr(short int reg_IR, short int reg_PC, s_mem_word* ptr_mem_array, FILE* fp_tracefile, char debug)
{
	s_effective_addr ret_EAddr;	// struct to be returned
	short int EffectiveAddress; // temporary EAddr value 
	short int IndirectAddrLoc;  // temp value used for indirect mode
	short int CurrentPage;		// current page of the given PC, used in current page mode
	// initialize output flags for indirect and auto-indexing to be false
	ret_EAddr.flag_MemType_Indirect = FALSE;
	ret_EAddr.flag_MemType_AutoIndex = FALSE;
	
	// masks for indirect addressing bit and page bit
	//short int indirect_addr_bit_mask = 2^(ADDR_INDIRECT_ADDR_BIT);
	short int indirect_addr_bit_mask = 1 << (PDP8_WORD_SIZE - ADDR_INDIRECT_ADDR_BIT - 1);
	short int memory_page_bit_mask = 1 << (PDP8_WORD_SIZE - ADDR_MEMORY_PAGE_BIT - 1);
	// the following offset mask will have '1's in all bit positions 
	// containing the offset
	short int offset_mask = (1 << (PDP8_WORD_SIZE - ADDR_OFFSET_LOW)) - 1;
	// similarly, page_mask has '1's in the bit positions specifying the page
	// first, set '1' bits for each bit position in the page bits
	short int page_mask = (1 << (ADDR_PAGE_HIGH - ADDR_PAGE_LOW + 1)) - 1;
	// then, shift the page bits into the correct position, 
	// located to the left of the offset bits
	page_mask = page_mask << (ADDR_OFFSET_HIGH - ADDR_OFFSET_LOW + 1);
	
	// determine the opcode
	char curr_opcode = reg_IR >> (PDP8_WORD_SIZE - INSTR_OP_LOW + 1);
	
	// Check if the opcode of the current instruction is a memory reference 
	// instruction (opcodes 0 to 5).
	if (curr_opcode >= 0) && (curr_opcode <= 5)) {
		// strings for effective address and current instruction for debug
		char* p_str_EAddr = malloc(PDP8_ADDR_SIZE*sizeof(char));
		if (p_str_EAddr == null) {
			fprintf(stderr,"malloc() for p_str_EAddr failed in calc_eaddr()\n");
			exit(-1);
		}
		char* p_str_IndirectAddr = malloc(PDP8_ADDR_SIZE*sizeof(char));
		if (p_str_IndirectAddr == null) {
			fprintf(stderr,"malloc() for p_str_IndirectAddr failed in calc_eaddr()\n");
			exit(-1);
		}
		char* p_str_IR = malloc(PDP8_WORD_SIZE*sizeof(char));
		if (p_str_IR == null) {
			fprintf(stderr,"malloc() for p_str_IR failed in calc_eaddr()\n");
			exit(-1);
		}
		// convert the given IR to a binary string for debug
		int_to_binary_str(reg_IR, PDP8_WORD_SIZE, p_str_IR);
		
		// determine the current page from the PC
		CurrentPage = reg_PC & page_mask;
		// determine offset from the IR
		CurrentOffset = reg_IR & offset_mask;
		
		// Debug Initial Prints:
		if (debug)
		begin
			printf("BEGIN EFFECTIVE ADDRESS CALC:");
			printf("         CURRENT INSTR: %s [%o]", *p_str_IR, reg_IR);
			printf(" ");
			printf("   0   1   2   3   4   5   6   7   8   9  10  11");
			printf(" +---+---+---+---+---+---+---+---+---+---+---+---+");
			printf(" | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c |",
				*p_str_IR,*(p_str_IR+1),*(p_str_IR+2),*(p_str_IR+3),
				*(p_str_IR+4),*(p_str_IR+5),*(p_str_IR+6),*(p_str_IR+7),
				*(p_str_IR+8),*(p_str_IR+9),*(p_str_IR+10),*(p_str_IR+11));
			printf(" +---+---+---+---+---+---+---+---+---+---+---+---+");
			printf(" |  op-code  | I | M |      O  F  F  S  E  T     |");
			printf(" +---+---+---+---+---+---+---+---+---+---+---+---+");
			printf(" ");
		end
		
		// Check if the Indirect Addressing bit set...
		if ((reg_IR & indirect_addr_bit_mask) == 0)
		{ 	// If Indirect Addressing bit = 0 (Direct addressing) 
			// Check if the Memory Page bit is set...
			if ((reg_IR & memory_page_bit_mask) == 0)
			{  
				// Check if the AutoIndex registers are selected.
				//--------------------------------------------
				// ADDRESSING MODE 1: Zero Page Addressing
				//    Effective Address <- 00000 + Offset
				EffectiveAddress = CurrentOffset;
				
				// convert to binary string for debug print
				int_to_binary_str(EffectiveAddress, PDP8_ADDR_SIZE, p_str_EAddr);
				
				if (debug) {
					printf("  -- ZERO PAGE ADDRESSING --");
					printf("     EFFECTIVE ADDRESS: %s [%o]", *p_str_EAddr, EffectiveAddress);
				}
				//--------------------------------------------
			}
			else // Memory Page bit = 1
			{  
				//--------------------------------------------
				// ADDRESSING MODE 2: Current Page Addressing
				//    Effective Address <- Old_PC[0:4] + Offset
				EffectiveAddress = CurrentPage + CurrentOffset;
				
				// convert to binary string for debug print
				int_to_binary_str(EffectiveAddress, PDP8_ADDR_SIZE, p_str_EAddr);
				
				if (debug) {
					printf("  -- CURRENT PAGE ADDRESSING --");
					printf("     EFFECTIVE ADDRESS: %s [%o]", *p_str_EAddr, EffectiveAddress);
				}
				//--------------------------------------------
			}
		} // end if for direct addressing
		else // Indirect Addressing bit = 1
		{
			// Check if locations outside of 0010 through 0017 (octal) have been indicated
			if (!((CurrentOffset >= 8) && (CurrentOffset < 15)))
			{
				//--------------------------------------------
				// ADDRESSING MODE 3: Indirect Addressing
				if ((reg_IR & memory_page_bit_mask) == 0)	// check if memory page bit is 0
				{  	// If Zero Page Addressing, then: 
					//    Effective Address <- C(00000 + Offset)
					IndirectAddrLoc = CurrentOffset;
					//FUNCTION CALL TO READMEM FOR LOG
					EffectiveAddress = read_mem(IndirectAddrLoc, TF_READ, ptr_mem_array, fp_tracefile);	
					//EffectiveAddress = *(ptr_mem_array+IndirectAddrLoc);
					
					// convert to binary string for debug print
					int_to_binary_str(EffectiveAddress, PDP8_ADDR_SIZE, p_str_EAddr);
					int_to_binary_str(IndirectAddrLoc, PDP8_ADDR_SIZE, p_str_IndirectAddr);
					
					if (debug) {
						printf("  -- INDIRECT ADDRESSING; ZERO PAGE --");
						printf("INDIRECT ADDR LOCATION: %s [%o]", *p_str_IndirectAddr, IndirectAddrLoc);
						printf("     EFFECTIVE ADDRESS: %s [%o]", *p_str_EAddr, EffectiveAddress);
					}
				}
				else
				{  	// If Current Page Addressing, then:
					//    Effective Address <- C(Old_PC[0:4] + Offset)
					IndirectAddrLoc = CurrentPage + CurrentOffset;
					//FUNCTION CALL TO READMEM TO OBTAIN VALUE at IndirectAddrLoc
					EffectiveAddress = read_mem(IndirectAddrLoc, TF_READ, ptr_mem_array, fp_tracefile);
					//EffectiveAddress = *(ptr_mem_array+IndirectAddrLoc);
					
					// convert to binary string for debug print
					int_to_binary_str(EffectiveAddress, PDP8_ADDR_SIZE, p_str_EAddr);
					int_to_binary_str(IndirectAddrLoc, PDP8_ADDR_SIZE, p_str_IndirectAddr);
					
					if (debug) {
						printf("  -- INDIRECT ADDRESSING; CURRENT PAGE --");
						printf("INDIRECT ADDR LOCATION: %s [%o]", *p_str_IndirectAddr, IndirectAddrLoc);
						printf("     EFFECTIVE ADDRESS: %s [%o]", *p_str_EAddr, EffectiveAddress);
					}
				}
			  
				// set flag to indicate indirect addressing was used
				ret_EAddr.flag_MemType_Indirect = TRUE;
			}
			else // offset location from 0010 through 0017 (octal) was specified
			{
				//--------------------------------------------
				// ADDRESSING MODE 4: AutoIndex Addressing
				//      C(AutoIndex_Register) <- C(AutoIndex_Register) + 1
				//      EffectiveAddress <- C(AutoIndex_Register)
				IndirectAddrLoc = CurrentOffset;
				EffectiveAddress = *(ptr_mem_array+IndirectAddrLoc); // pre-incremented value
				
				//FUNCTION CALL TO READMEM FOR LOG
				read_mem(IndirectAddrLoc, TF_READ, ptr_mem_array, fp_tracefile);
				
				// convert to binary string for debug print
				int_to_binary_str(EffectiveAddress, PDP8_ADDR_SIZE, p_str_EAddr);
				int_to_binary_str(IndirectAddrLoc, PDP8_ADDR_SIZE, p_str_IndirectAddr);

				if (debug)
				{
					printf("  -- AUTO-INDEX ADDRESSING --");
					printf("INDIRECT ADDR LOCATION: %s [%o]", *p_str_IndirectAddr, IndirectAddrLoc);
					printf("  Orig M[IndirectAddr]: %s [%o]", *p_str_EAddr, EffectiveAddress);
				}
				
				// Calculate the actual incremented value to be used as the effective address
				EffectiveAddress++; // read mem val and increment by 1
				// convert to binary string for debug print
				int_to_binary_str(EffectiveAddress, PDP8_ADDR_SIZE, p_str_EAddr);
				
				// Write new value to memory
				write_mem(IndirectAddrLoc, EffectiveAddress, ptr_mem_array, fp_tracefile);
				
				// set flag to indicate auto-index addressing was used
				ret_EAddr.flag_MemType_AutoIndex = TRUE;
				
				if (debug) {
					printf("     EFFECTIVE ADDRESS: %s [%o]", *p_str_EAddr, EffectiveAddress);
				}
			}
		} // end else where indirect addressing bit = 1
		
		if (debug) {
			printf("----------------------------------------------------");
		}
	
		free(p_str_EAddr); // free the strings that were used for debug
		free(p_str_IndirectAddr);
		free(p_str_IR);	
		
		ret_EAddr.EAddr = EffectiveAddress; // save calculated EAddr to return value struct
	}
		
	return ret_EAddr;
}

//=========================================================
// UI SUBROUTINE
//---------------------------------------------------------	
// Function: module_UI
// Description: Given the current instruction, PC, AC, LR, 
//    and SR as parameters this subroutine will execute 
//    the given UI instruction.
// Return Values: The next PC, AC, LR, and SR values will 
//    be returned in the struct, as well as flags 
//    indicating:
//        if a NOP was executed, 
//        if a HLT was issued, 
//        if a grp2 skip instruction
//---------------------------------------------------------
s_updated_vals module_UI (short int IR, short int PC, short int AC, char LR, short int SR, char debug)
{
	s_updated_vals ret_vals;	// return value struct
	char flag_skip_instr = 0;	// flag indicating if next instruction should be skipped
	char[16] str_skip_check = ""; // temporary string value used to indicate the type of skip conditions that were checked
	char[16] str_skip_type = "";  // string to hold the skip types for debug print
	short int tmp_val = 0;		// temporary value used in calculating result for RAR/RAL operations
	short int tmp_rotate = 0;	// temporary value used for result of rotation.
	ret_vals.PC = PC;	// initialize all return values
	ret_vals.AC = AC;
	ret_vals.LR = LR;
	ret_vals.SR = SR;
	ret_vals.flag_HLT = FALSE;
	ret_vals.flag_NOP = FALSE;
	ret_vals.flag_branch_type = BT_NO_BRANCH;
	ret_vals.flag_branch_taken = BT_NOT_TAKEN;
	
	// mask to be used after addition and inversion of the LR bit, 
	// to ensure that result in a register only contains that 
	// number of bits
	short int word_mask = (1 << PDP8_WORD_SIZE) - 1;
	
	char[WORD_STRING_SIZE] str_IR;	// string for displaying binary value of IR
	int_to_binary_str(IR, PDP8_WORD_SIZE, &str_IR);
	
	if (debug) {
		printf(" \n");
		printf("***************** UI MODULE DEBUG ******************\n");
		printf("   Current Instr: %s [%o]\n", str_IR, IR);
		printf(" \n");
		printf("   0   1   2   3   4   5   6   7   8   9  10  11\n");
		printf(" +---+---+---+---+---+---+---+---+---+---+---+---+\n");
		printf(" | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c | %c |\n",
			str_IR[0],str_IR[1],str_IR[2],str_IR[3],str_IR[4],str_IR[5],
			str_IR[6],str_IR[7],str_IR[8],str_IR[9],str_IR[10],str_IR[11]);
		printf(" +---+---+---+---+---+---+---+---+---+---+---+---+");
	}
	
	// Group 1 Microinstructions: Check if bit 3 is a 0
	if ( ((IR >> (PDP8_WORD_SIZE-3-1)) & 1) == 0) {
		// Print debug statements for bit identification
		if (debug) {
			printf("                  cla cll cma cml rar ral 0/1 iac\n");
			printf(" \n");
			printf(" Group 1 Microinstructions:\n");
		}
		
		// if bits IR[4:11] == 0, then the instruction is a NOP
		if ( (IR << 4) == 0) {
			ret_vals.flag_NOP = TRUE;
		}
		else {
			// Sequence 1: CLA/CLL
			// Check if bits 4 and 5 were set
			if ( ((IR >> (PDP8_WORD_SIZE - 4-1)) & 1) == 1) {
				ret_vals.AC = 0;
				if (debug) {
					printf(" -- Clear Accumulator\n");
					printf("                NEW AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
				}
			}
			if ( ((IR >> (PDP8_WORD_SIZE - 5-1)) & 1) == 1) {
				ret_vals.LR = 0;
				if (debug) {
					printf(" -- Clear Link Register\n");
					printf("                NEW LR: [%o]\n", ret_vals.LR, ret_vals.LR);
				}
			}
			
			// Sequence 2: CMA/CML
			// Check if bits 6 and 7 were set
			// Check if bits 4 and 5 were set
			if ( ((IR >> (PDP8_WORD_SIZE - 6-1)) & 1) == 1) {
				ret_vals.AC = ~(ret_vals.AC);
				if (debug) {
					printf(" -- Complement Accumulator\n");
					printf("                NEW AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
				}
			}
			if ( ((IR >> (PDP8_WORD_SIZE - 7-1)) & 1) == 1) {
				ret_vals.LR = (ret_vals.LR) ^ 1;
				if (debug) {
					printf(" -- Complement Link Register");
					printf("                NEW LR: [%o]\n", ret_vals.LR, ret_vals.LR);
				}
			}
			
			// Sequence 3: IAC
			// Check if bit 11 is set
			if ( ((IR >> (PDP8_WORD_SIZE - 11-1)) & 1) == 1) {
				ret_vals.AC = (ret_vals.AC) + 1;
				if ((ret_vals.AC >> PDP8_WORD_SIZE) != 0) {
					ret_vals.LR = (ret_vals.LR) ^ 1; // this will flip the least significant bit of the LR.
				}
				ret_vals.AC = ret_vals.AC & word_mask;
				
				if (debug) {
					printf(" -- Complement Accumulator\n");
					printf("                NEW AC: %3x [%4o]\n", ret_vals.AC, ret_vals.AC);
					printf("                NEW LR: %3x [%4o]", nextLR, nextLR);
				}
			}
			
			// Sequence 4: RAR/RAL
			// Check if bits 8 and 9 have been set simultaneously: if so, this
			// is an invalid instruction, since a rotate right and rotate left has
			// been simultaneously indicated. Print a warning, but allow instruction 
			// to "execute" since performing the right and left rotate will cancel 
			// each other out and leave the AC unchanged.
			if ((((IR >> (PDP8_WORD_SIZE - 8-1)) & 1) == 1) && ((IR >> (PDP8_WORD_SIZE - 9-1)) == 1))
			{
				fprintf(stderr,"WARNING: Micro Op instruction conflict at PC = [%o]. Rotate Left and Rotate Right enabled simultaneously.", PC);
			}
			// Rotate right:
			// Check bit 8, which indicates a rotate right operation
			if (((IR >> (PDP8_WORD_SIZE - 8-1)) & 1) == 1) {
				// Let tmp_val be set to the concatenation of LR and AC, 
				// {LR,AC}
				tmp_val = (ret_vals.LR << PDP8_WORD_SIZE) + ret_vals.AC;
				
				// Rotate right:
				// Check if bit 10 is 0: If so, only need to rotate one bit position
				if (((IR >> (PDP8_WORD_SIZE - 10-1)) & 1) == 0) {
					// First make the most significant bit of tmp_rotate
					// equal to the least significant bit of tmp_val
					tmp_rotate = (tmp_val & 1) << PDP8_WORD_SIZE;
					// Then OR the remaining more significant bits of tmp_val
					// with tmp_rotate
					tmp_rotate = tmp_rotate | (tmp_val >> 1);
					
					// Next, the new value of LR should be the most significant
					// bit of tmp_rotate, and the AC should be set to all the 
					// less significant bits of tmp_rotate.
					ret_vals.LR = (tmp_rotate >> PDP8_WORD_SIZE);
					ret_vals.AC = (tmp_rotate & word_mask);
					
					// Print out new calculated values
					if (debug) {
						printf(" -- Rotate Accumulator and Link Right\n");
						printf("                NEW LR: %x [%o]\n", ret_vals.LR, ret_vals.LR);
						printf("                NEW AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
					}
				}
				else { // Otherwise bit 10 (the 0/1 bit) is 1 --> rotate 2 positions right
					// First set the two most significant bits of tmp_rotate
					// equal to the two least significant bits of tmp_val
					tmp_rotate = (tmp_val & 3) << (PDP8_WORD_SIZE-1);
					// Then OR the remaining more significant bits of tmp_val
					// with tmp_rotate
					tmp_rotate = tmp_rotate | (tmp_val >> 2);
					
					// Next, the new value of LR should be the most significant
					// bit of tmp_rotate, and the AC should be set to all the 
					// less significant bits of tmp_rotate.
					ret_vals.LR = (tmp_rotate >> PDP8_WORD_SIZE);
					ret_vals.AC = (tmp_rotate & word_mask);
					
					// Print out new calculated values
					if (debug) {
						printf(" -- Rotate Accumulator and Link Right\n");
						printf("                NEW LR: %x [%o]\n", ret_vals.LR, ret_vals.LR);
						printf("                NEW AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
					}
				}
			} // end rotate right
				
			// Rotate left:
			// Check if bit 9 is 0: RAL
			if (((IR >> (PDP8_WORD_SIZE - 9-1)) & 1) == 0) {
				// Check if bit 10 is 0: If so, only need to rotate one bit position
				if (((IR >> (PDP8_WORD_SIZE - 10-1)) & 1) == 0) {
					// First set the least significant bit of tmp_rotate to be
					// the most significant of tmp_val.
					tmp_rotate = (tmp_val >> PDP8_WORD_SIZE);
					// Then OR tmp_rotate with all the remaining bits of tmp_val
					// after shifting them left one position.
					tmp_rotate = tmp_rotate | ((tmp_val & word_mask) << 1);
					// Next, the new value of LR should be the most significant
					// bit of tmp_rotate, and the AC should be set to all the 
					// less significant bits of tmp_rotate.
					ret_vals.LR = (tmp_rotate >> PDP8_WORD_SIZE);
					ret_vals.AC = (tmp_rotate & word_mask);
					
					// Print out new calculated values
					if (debug) {
						printf(" -- Rotate Accumulator and Link Left\n");
						printf("                NEW LR: %x [%o]\n", ret_vals.LR, ret_vals.LR);
						printf("                NEW AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
					}
				}
				else { // Otherwise bit 10(the 0/1 bit) is 1 --> rotate 2 positions left
					// First set the two least significant bits of tmp_rotate to be
					// the least significant two of tmp_val
					tmp_rotate = (tmp_val >> (PDP8_WORD_SIZE - 1));
					// Then OR tmp_rotate with all remaining bits of tmp_val
					// after shifting them left two positions. Actually, what 
					// is done here: shift tmp_val to the left 1 bit and AND 
					// result with word_mask to set the most significant two 
					// bits to 0, then shift left one more bit position, so that
					// the next most significant bit will have been shifted two
					// bits left.
					tmp_rotate = tmp_rotate | (((tmp_val << 1) & word_mask) << 1);
					
					// Next, the new value of LR should be the most significant
					// bit of tmp_rotate, and the AC should be set to all the 
					// less significant bits of tmp_rotate.
					ret_vals.LR = (tmp_rotate >> PDP8_WORD_SIZE);
					ret_vals.AC = (tmp_rotate & word_mask);
					
					// Print out new calculated values
					if (debug) {
						printf(" -- Rotate Accumulator and Link Left Twice\n");
						printf("                NEW LR: %x [%o]\n", ret_vals.LR, ret_vals.LR);
						printf("                NEW AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
					}
				}
			} // end rotate left operation
		} // End sequence 1-4 operations
	} // End Group 1 Microinstructions section
	else // Bit 3 is 1, indicating either Group 2 or 3 Microinstructions
	{
		// Check if bit 11 is 0: Group 2 Microinstructions
		if (((IR >> (PDP8_WORD_SIZE - 11-1)) & 1) == 0) {
			// debug print
			if (debug) {
				printf("                  cla sma sza snl 0/1 osr hlt\n");
				printf(" \n");
				printf(" Group 2 Microinstructions:\n");
			}
			
			// Check if bit 8 is set to 0: 
			// OR subgroup
			if (((IR >> (PDP8_WORD_SIZE - 8-1)) & 1) == 0) {
				// check if any of bits 5 through 7 were set, indicating
				// that an OR skip condition was to be checked (this instruction
				// should be flagged as a conditional branch instruction)
				if (((IR >> (PDP8_WORD_SIZE - 7-1)) & 7) != 0) {
					ret_vals.flag_branch_type = BT_CONDITIONAL;
				}
				
				// SMA - Skip on Minus Accumulator: check bit 5
				if (((IR >> (PDP8_WORD_SIZE - 5-1)) & 1) == 0) {
					// if most significant bit of AC is 1, then skip
					if ((ret_vals.AC >> (PDP8_WORD_SIZE - 1)) == 1) {
						flag_skip_instr = 1;
						strcat(&str_skip_type," SMA");
					}
					strcat(&str_skip_check," SMA");
				}
				
				// SZA - Skip on Zero Accumulator: check bit 6
				if (((IR >> (PDP8_WORD_SIZE - 6-1)) & 1) == 0) {
					// if AC is 0, then skip
					if (ret_vals.AC == 0) {
						flag_skip_instr = 1;
						strcat(&str_skip_type," SZA");
					}
					strcat(&str_skip_check," SZA");
				}
				
				// SNL - Skip on Nonzero Link: check bit 7
				if (((IR >> (PDP8_WORD_SIZE - 7-1)) & 1) == 0) {
					// if LR is not 0, then skip
					if (ret_vals.LR != 0) {
						flag_skip_instr = 1;
						strcat(&str_skip_type," SNL");
					}
					strcat(&str_skip_check," SNL");
				}
			
				// debug print: indicating if any of the OR skip conditions were met
				if ((debug) && (ret_vals.flag_branch_type == BT_CONDITIONAL)) {
					if (flag_skip_instr) {
						printf(" -- OR group condition(s) met:%s\n",str_skip_type);
					}
					else {
						printf(" -- OR group condition(s) not met.\n");
					}
					printf("    Checked for: %s\n", str_skip_check);
				}
				
			} // end OR subgroup
			else // otherwise bit 8 = 1: indicating AND subgroup
			{
				// check if bits [5:7] were all zero: Skip Always should be 
				// flagged as an unconditional branch. 
				if (((IR >> (PDP8_WORD_SIZE - 7-1)) & 7) == 0) {
					ret_vals.flag_branch_type = BT_UNCONDITIONAL;
					str_skip_check = "None. (Unconditional branch)";
				}
				else {
					// Otherwise, it is a conditional branch.
					ret_vals.flag_branch_type = BT_CONDITIONAL;
				}
				
				// set skip flag to true initially
				flag_skip_instr = 1; 
				
				// Note that for the AND section, str_skip_type is used 
				// to indicate the conditions that failed.
				
				// SPA - Skip on Positive Accumulator: check bit 5
				if (((IR >> (PDP8_WORD_SIZE - 5-1)) & 1) == 0) {
					// if most significant bit of AC is not 0, then 
					// then AC is negative, and condition is not met
					if ((ret_vals.AC >> (PDP8_WORD_SIZE - 1)) == 1) {
						flag_skip_instr = 0;
						strcat(&str_skip_type," SPA");
					} 
					strcat(&str_skip_check," SPA");
				}
				
				// SNA - Skip on Nonzero Accumulator: check bit 6
				if (((IR >> (PDP8_WORD_SIZE - 6-1)) & 1) == 0) {
					// if AC is 0, then condition was not met, so
					// do not skip
					if (ret_vals.AC == 0) {
						flag_skip_instr = 0;
						strcat(&str_skip_type," SNA");
					}
					strcat(&str_skip_check," SNA");
				}
				
				// SZL - Skip on Zero Link: check bit 7
				if (((IR >> (PDP8_WORD_SIZE - 7-1)) & 1) == 0) {
					// if LR is non-zero, then condition not satisfied,
					// so do not skip
					if (ret_vals.LR != 0) {
						flag_skip_instr = 0;
						strcat(&str_skip_type," SZL");
					}
					strcat(&str_skip_check," SZL");
				}
			
				// debug print: indicating if any of the OR skip conditions were met
				if ((debug) && (ret_vals.flag_branch_type != BT_NOT_TAKEN)) {
					if (flag_skip_instr) {
						printf(" -- AND group condition(s) met.\n");
					}
					else {
						printf(" -- AND group condition(s) not met: %s\n", str_skip_type);
					}
					printf("    Checked for: %s\n", str_skip_check);
				}
				
			} // end AND subgroup
			
			// if the flag_skip_instr variable was set, 
			// then increment PC by 1
			if (flag_skip_instr) {
				ret_vals.PC++;
			}
			
			// CLA - Clear Accumulator: check if bit 4 is set
			if (((IR >> (PDP8_WORD_SIZE - 4-1)) & 1) == 1) {
				ret_vals.AC = 0;
				if (debug) {
					printf(" -- CLA - Clear Accumulator\n");
					printf("                NEW AC: %x [%o]", ret_vals.AC, ret_vals.AC);
				}
			}
			
			// OSR - Or Switch Register with Accumulator: check if bit 9 is set
			if (((IR >> (PDP8_WORD_SIZE - 9-1)) & 1) == 1) {
				if (debug) {
					printf(" -- OSR - Or Switch Register with Accumulator\n");
					printf("           Previous AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
					printf("           Previous SR: %x [%o]\n", ret_vals.SR, ret_vals.SR);
				}
				
				ret_vals.AC = ret_vals.AC | ret_vals.SR;
				
				if (debug) printf("                NEW AC: %x [%o]\n", ret_vals.AC, ret_vals.AC);
			}
			
			// HLT - HaLT: check if bit 10 is set
			if (((IR >> (PDP8_WORD_SIZE - 10-1)) & 1) == 1) {
				if (debug) {
					printf(" -- HLT - Halt\n");
				}
				
				ret_vals.flag_HLT = TRUE; // set halt flag
			}
			
		} // End Group 2 Microinstructions
		else { // Bit 11 is set to 1: Group 3 Microinstructions
			// These are not implemented, so should be noted as 
			// illegal/unrecognized instructions
			fprintf(stderr,"WARNING: Group 3 MicroOp called at PC = [%o]. Group 3 MicroOps not enabled in simulation.\n", PC);
			// Note that next values were initialized to be equal to current values,
			// so no changes will be made to the registers. (Except for the pre-incremented PC,
			// which will just go to the next instruction as it should.)
		} // End Group 3 Microinstructions
	} // End Group 2 and 3 Microinstructions section
		
	return ret_vals; // return updated values
}