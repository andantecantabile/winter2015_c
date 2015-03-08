/ Program : isz_test.as
/ Date : January 30th, 2015
/
/ Desc : This program tests cases involving
/        the ISZ instruction.
/ Checks for: 
/     1. spot check incrementation of positive numbers (variables A, B)
/     2. incrementation of 0 (variable C)
/     3. incrementation of -1 (variable D) => should result in skipping next instruction
/     4. spot check incrementation of a negative number (variable E)
/     5. incrementation of largest positive number, 2047 = 0x7FF (variable F)
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	cla cll 	/ clear AC and Link
	isz A 		/ increment A
	isz B 		/ increment B
	isz C 		/ increment C
	dca G		/ clear accumulator
	isz D 		/ increment D; on first run through, should skip next instruction and loop again 
	hlt 		/ Halt program
	isz E 		/ increment E
	isz F 		/ increment F
	dca	G		/ clear accumulator
	jmp Main	/ To continue - goto Main
/
/ Data Section
/
*0750 			/ place data at address 0750 = 0x1E8
A, 	7 			/ A equals 7
B, 	134 		/ B equals 92
*4304			/ place data at address 0x8C4
C, 	0			/ C equals 0
D,  7777		/ D equals -1
E,  4000		/ E equals -2048
*7023			/ place data at address 0xE13
F,  3777		/ F equals 2047
G,  0			/ G is just a place holder for storing accumulator value later.
$Main 			/ End of Program; Main is entry point

/========================================================
/ Expected Results:
/ Store 0x8 at loc 0x1E8
/ Store 0x5D at loc 0x1E9
/ Store 0x1 at loc 0x8C4
/ Store 0x0 at loc 0xE14  / accumulator should be 0
/ Store 0x0 at loc 0x8C5
/ Store 0x801 at loc 0x8C6
/ Store 0x800 at loc 0xE13
/ Store 0x0 at loc 0xE14  / accumulator should be 0
/ Loop again:
/ Store 0x9 at loc 0x1E8
/ Store 0x5E at loc 0x1E9
/ Store 0x2 at loc 0x8C4
/ Store 0x0 at loc 0xE14  / accumulator should be 0
/ Store 0x1 at loc 0x8C5
/ Halt.

/ In the trace file, should see 17 instruction fetches,
/ 13 writes to memory (from ISZ and DCA instructions), 
/ and 10 reads from memory (from ISZ instructions).

/ Expected Statistic Counts:
/ Total Instruction Count: 17
/   ISZ: 10 instructions
/   DCA: 3 instructions
/   JMP: 1 instruction
/   MICRO: 3 instructions (CLA/CLL gets executed twice, and one HLT instruction)