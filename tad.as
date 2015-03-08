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
	tad A 		/ Add Positive with Zero AC - TEST 1
	tad B 		/ Add Positive to Positive - TEST 2
	tad C       / Add Zero to Positive AC - TEST 3
	cla cll		/ clear AC and Link
	tad C       / Add Zero to Zero - TEST 4
	tad D       / Add Negative to Zero AC - TEST 5
	tad A		/ Add Positive to Negative AC Results in Zero - TEST 6
	tad E		/ Add Negative to Zero AC
	tad C		/ Add Zero to Negative AC - TEST 7
	tad A       / Add Positive to Negative AC Result Negative - TEST 8
	tad I       / Add Positive to Negative AC Result Positive - TEST 9
	cla cll     / clear AC and Link
	tad A       / Add Positive to Zero AC 
	tad D       / Add Negative to Positive AC Result Zero - TEST 10
	tad A       / Add Positive to Zero AC
	tad E       / Add Negative to Positive AC Result Negative - TEST 11
	cla cll     / clear AC and Link
	tad B       / Add Positive to Zero AC
	tad D       / Add Negative to Positive AC Result Positive - TEST 12
    cla cll     / clear AC and Link
	tad G       / Add largest possible positive 
	tad G       / Add largest possible positive Result Overflow, Carryout of Zero - TEST 13
	cla cll     / clear AC and Link
	tad H       / Add largest possible negative 
	tad H       / Add largest possible negative Result Overflow, Carryout of One - TEST 14
	cla cll     / clear AC and Link
	tad G       / Add largest possible positive
	tad H       / Add largest possible negative/positive Result negative - TEST 15
	cla cll     / clear AC and Link
	tad G       / Add largest possible negative
	tad H       / Add largest possible positive/negative Result negative - TEST 16
	cla cll     / clear AC and Link
	tad D       / Add negative 
	tad E       / Add negative to negative AC - TEST 17
	hlt 		/ Halt program
	jmp Main	/ To continue - goto Main
/
/ Data Section
/
*0250 			/ place data at address 0250
A, 	2 		
B, 	3 		
C, 	0		
D, -2
E, -5
F, -5
G,  3777
H,  4000
I, 5

$Main 			/ End of Program; Main is entry point