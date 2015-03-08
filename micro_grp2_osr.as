/ Program : micro_grp2_osr.as
/ Date : February 13th, 2015
/
/ Desc : This program tests cases involving
/        the OSR and CLA options in the Group 2
/        microinstructions.
/ Checks for: 
/     1. Accumulator having the correct value after OSR instruction.
/     2. Verify that CLA clears the accumulator in the group 2 microinstruction set.
/ Note: Set OSR to 2730 decimal (AAA hexadecimal) for these test cases.
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	cla cll 	/ clear AC and Link; group 1
	osr 		/ Or contents of SR with AC
	sma 		/ verify that AC < 0
	hlt
	dca A 		/ store contents of accumulator
	osr 		/ Or contents of SR with AC again
	rar 		/ rotate AC right one bit
	osr 		/ Or again
	sma 		/ again skip if AC < 0
	hlt
	dca B		/ store contents of AC
	tad B 		/ load the value back into AC
	sma 		/ skip if AC < 0 (verify)
	hlt
	osr cla 	/ clear the AC and then OR with SR
	sma 		/ verify that AC still < 0  
	hlt
	dca C 		/ store AC in C <-- this value should be #AAA
	hlt			/ end test
/
/ Data Section
/
*0320 	/ place data at address 0320 = 0x0D0
A, 	0 	/ placeholders to store AC later
B, 	0 
C,	0
$Main 			/ End of Program; Main is entry point

/========================================================
/ Expected Results:
/ Store 0xAAA at loc 0x0D0
/ Store 0xFFF at loc 0x0D1
/ Store 0xAAA at loc 0x0D2

/ In the trace file, should see 15 instruction fetches,
/ 3 writes to memory (from DCA instructions), 
/ and 1 read from memory.

/ Expected Statistic Counts:
/ Total Instruction Count: 15
/	TAD: 1 instruction
/   DCA: 3 instructions
/   MICRO: 11 instructions