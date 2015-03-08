/ Program : eaddr_test.as
/ Date : February 7th, 2015
/
/ Desc : This program tests the modes of addressing.
/ Checks for: 
/     1. Zero Page Addressing
/     2. Current Page Addressing
/     3. Indirect / Zero Page Addressing
/     4. Indirect / Current Page Addressing
/     5. Auto-indexing
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	cla cll 	/ clear AC and Link
	/ Test AND instruction - 24 instructions
	tad ANDstartval	/ set AC to 7777
	and z A1 		/ zero page addressing
	sza cla
	hlt
	tad ANDstartval	/ indirect current page
	and i B1
	sza cla
	hlt
	tad ANDstartval	/ indirect zero page
	and i C1
	sza cla
	hlt
	tad ANDstartval	/ indirect curr page to zero page
	and i D1
	sza cla
	hlt
	tad ANDstartval	/ auto-indexing
	and i pArr1
	sza cla
	hlt
	tad ANDstartval
	and i pArr2
	sza cla
	hlt
	tad ANDstartval
	and i pArr3
	sza cla
	hlt
	tad ANDstartval
	and i pArr4
	sza cla
	hlt
	jmp i pTest3
	/ Test TAD instruction - 16 instructions
*0400
Test3,	tad z A2 		/ set accumulator to A; from page zero
	sma	cla		/ skip next instr if AC < 0, and clear AC
	hlt
	tad i B2 	/ test indirect addressing of B
	sma cla 	/ skip next instr if AC < 0, and clear AC
	hlt
	tad i C2 	/ test indirect addressing of C on page zero
	sma cla
	hlt
	tad i D2 	/ test indirect addr. of D, on current page
	sma cla
	hlt
	tad i pArr1	/ auto-indexing 
	sma cla
	hlt
	tad i pArr2	/ auto-indexing 
	sma cla
	hlt
	tad i pArr3	/ auto-indexing 
	sma cla
	hlt
	tad i pArr4	/ auto-indexing 
	sma cla
	hlt
	/ Test ISZ instruction - 8 instructions
	isz z A3	/ zero page addressing
	hlt
	isz i B3	/ indirect curr page
	hlt
	isz i C3	/ indirect zero page
	hlt
	isz i D3	/ indirect curr page to zero page
	hlt
	isz i pArr1	/ auto-indexing
	hlt
	isz i pArr2	/ auto-indexing
	hlt
	isz i pArr3	/ auto-indexing
	hlt
	isz i pArr4	/ auto-indexing
	hlt
	/ Test DCA instruction: should result in 8 stores to memory
	/ 25 instructions
	cla
	tad DCAval	/ zero page addressing
	dca z A4
	sza cla
	hlt
	tad DCAval	/ indirect current page
	dca i B4
	sza cla
	hlt
	tad DCAval	/ indirect zero page
	dca i C4
	sza cla
	hlt
	tad DCAval	/ indirect current page to zero page
	dca i D4
	sza cla
	hlt
	tad DCAval	/ auto-increment addressing
	dca i pArr1 
	sza cla
	hlt
	tad DCAval	/ auto-increment addressing
	dca i pArr2 
	sza cla
	hlt
	tad DCAval	/ auto-increment addressing
	dca i pArr3 
	sza cla
	hlt
	tad DCAval	/ auto-increment addressing
	dca i pArr4 
	sza cla
	hlt
	/ +3 instructions
	cla cll iac
	sma
	hlt		/ end of testing
	jmp TestError
TestError, cla cll iac
	hlt
/
/ Data Section
/
*0140 / page zero
A1, 0		/ A1 = 0 for AND test
C1, 3051	/ C1 = address of G1
H1, 0000	/ H1 = 0 for AND test
pTest3, 0400 	/ ptr to Test3
*0150 / page zero
A2, 7777
C2, 3053	/ C2 = address of G2
H2, 4000	/ H2 equals -2048
A3, 7777	/ -1 for ISZ test
C3, 3055	/ C3 = address of G3
H3, 7777	/ H3 = -1 for ISZ test
A4, 0 		/ placeholder for DCA test
C4, 3057 	/ C4 = address of G4
H4, 0 		/ placeholder for DCA test
*10
pArr1,arr1-1
pArr2,arr2-1
pArr3,arr3-1
pArr4,arr4-1
*0270 	/ place arr1 at address 0300 = 0x0C0
arr1,0000;5252;7777;0;4444;5555;6666;7777
*0570
arr2,0000;5252;7777;0;3333;2222;1111;0000
*0170	/ zero-page
arr3,0;5252;7777;0;0;0;0;0
*7030	/ off page
arr4,0;5252;7777;0;0;0;0;0
*0320	/ place data at address 0x0D0
ANDstartval, 7777	/ start val for AND tests is -1
B1, 3050	/ B1 = address of F1
D1, 0142	/ D1 = address of H1
*0520
DCAval, 6363 	/ value to store for DCA tests
B2, 3052	/ B2 = address of F2
B3, 3054	/ B3 = address of F3
B4, 3056 	/ B4 = address of F4
D2, 0152	/ D2 = address of H2
D3, 0155	/ D3 = address of H3
D4, 0160	/ D4 = address of H4
*3050	/ place data at address 0x618
F1, 0		/ F1 equals 0 for AND with AC
G1, 0		/ G1 equals 0 for AND test
F2, 7777	/ F2 equals -1 for TAD test
G2, 7776	/ G2 equals -2 for TAD test
F3, 7777 	/ F3 equals -1 for ISZ test
G3, 7777 	/ G3 equals -1 for ISZ test
F4, 0 		/ placeholder for DCA test
G4, 0 		/ placeholder for DCA test
$Main 		/ End of Program; Main is entry point

/========================================================
/ Expected Results:

/ Expected Statistic Counts:
/ Total Instruction Count: 78 executed instructions
/   AND: 8 instructions
/   TAD: 24 instructions
/   ISZ: 8 instructions
/   DCA: 8 instructions
/   JMP: 1 instruction
/   MICRO: 29 instructions (CLA/CLL gets executed twice, and one HLT instruction)