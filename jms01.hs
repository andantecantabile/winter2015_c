/ Program : jms01.as
/
/ This program tests the JMS instruction
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	
	cla cll 	/ clear AC and Link
	tad A 		/ add A to Accumulator
	jms SUB1 	/ Jump to subroutine SUB
	tad D		/ add D to AC 
	dca E 		/ store sum at D
	hlt 		/ Halt program
	jmp Main	/ To continue - goto Main
SUB1,
	0			/First word reserved for return address
	tad B		/ Add B to AC
	dca C 		/ store sum at C
	jmp i SUB1	/To continue, go to SUB
/	
/
/ Data Section
/
*0250 			/ place data at address 0250
A, 	2 			/ A equals 2
B, 	3 			/ B equals 3
C, 	0
D, 	4
E,  0
$Main 			/ End of Program; Main is entry point