/ Program : dca_test.as
/ Date : January 30th, 2015
/
/ Desc : This program tests cases involving
/        the DCA instruction.
/ Checks the following: 
/     1. store positive number from AC (to variable D)
/     2. store value 0 from AC (to variable E)
/     3. store value -1 from AC (to variable F)
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	cla cll 	/ clear AC and Link
	tad A 		/ add A to AC
	dca D 		/ store in D and clear AC
	dca E		/ store in E and clear accumulator
	tad B 		/ add B to AC
	dca	F		/ store in F and clear accumulator
	dca			/ check behavior when no location is specified with dca
	hlt			/ halt - stop program.
	jmp Main	/ To continue - goto Main
/
/ Data Section
/
*0540 			/ place data at address 0540 = 0x160
A, 	7 			/ A equals 7
B, 	7777 		/ B equals -1
*4304			/ place data at address 0x8C4
D,  333			/ D equals 0xDB, placeholder value to be written over
E,  333			/ E equals 0xDB, placeholder value
*7023			/ place data at address 0xE13
F,  333			/ F equals 0xDB, placeholder value
$Main 			/ End of Program; Main is entry point

/========================================================
/ Expected Results:
/ Read 0x7 from loc 0x160
/ Store 0x7 at loc 0x8C4
/ Store 0x0 at loc 0x8C5
/ Read 0xFFF from loc 0x161
/ Store 0xFFF at loc 0x8C6
/ Store 0x0 at loc 0x0 ?
/ Halt.

/ Expected Statistic Counts:
/ Total Instruction Count: 8
/   DCA: 4 instructions
/   TAD: 2 instructions
/   MICRO: 2 instructions (CLA/CLL gets executed once, and one HLT instruction)