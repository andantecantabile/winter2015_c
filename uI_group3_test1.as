/ Program : andtest01
/
/ Desc : This program computes c = a + b
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	
	cla cll 	/ clear AC and Link
/	mql
	tad A 		/ add A to Accumulator
/	mqa
	and B 		/ add B to Accumulator
/	swp
	dca C 		/ store sum at C
/	cam
	tad A 		/ add A to Accumulator
/	cla swp
	and B 		/ add B to Accumulator
/	
	dca D 		/ store sum at D
	hlt 		/ Halt program
	jmp Main	/ To continue - goto Main
/
/ Data Section
/
*0250 			/ place data at address 0250
A, 5252			/ A equals 101 010 101 010 
B, 2525			/ B equals 010 101 010 101
C, 7777			/ C equals 111 111 111 111 initially; expect 0
D, 7777			/ C equals 111 111 111 111 initially; expect 0
$Main 			/ End of Program; Main is entry point