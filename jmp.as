/ Program : Add01.pal
/ Date : 1/29/2015
/
/ Desc : This program test the tad command
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	cla cll 	/ clear AC and Link
	jmp Testa	/ Jump to test
	hlt
	jmp Main	/ To continue - goto Main
/
/ Data Section
/
*0250 			/ place data at address 0250
A, 	2 		

*300
Testa,
	cma
	jmp Testb
	hlt


*400
Testb,
	cma
	hlt


$Main 			/ End of Program; Main is entry point