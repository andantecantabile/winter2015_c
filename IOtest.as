/ Program : IO
/
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	
	cla cll 	/ clear AC and Link
/	kfc
	tad A 		/ add A to Accumulator
/	kfs
	and B 		/ and B with A
	kcc
	dca C 		/ store sum at C
	krs
	cla cll 	/ clear AC and Link
	krb
	tad D 		/ add D to Accumulator
/	tfl
	and E 		/ and E with D
	tsf
	dca F 		/ store sum at F
	tcf
	cla cll 	/ clear AC and Link
	tls
	tad G 		/ add G to Accumulator
/	skon
	and H 		/ and H
	ion
	dca I 		/ store sum at I
	iof
	cla cll 	/ clear AC and Link
	kcc krs
	tad J 		/ add J to Accumulator
	tsf tcf
	and K 		/ and K
	iof ion
	dca L 		/ store sum at L
	kcc tcf ion
	hlt 		/ Halt program
	jmp Main	/ To continue - goto Main
/
/ Data Section
/
*0250 			/ place data at address 0250
A, 5252			/ A equals 101 010 101 010 
B, 2525			/ B equals 010 101 010 101
C, 7777			/ C equals 111 111 111 111 initially; expect 0
D, 5252			/ D equals 101 010 101 010  
E, 7777			/ E equals 111 111 111 111 
F, 0			/ F equals 000 000 000 000 initially; expect 252
G, 2525			/ G equals 010 101 010 101 
H, 7777			/ H equals 111 111 111 111
I, 0			/ I equals 000 000 000 000 initially; expect 525
J, 7777			/ G equals 111 111 111 111
K, 0000			/ H equals 000 000 000 000
L, 0			/ I equals 000 000 000 000 initially; expect 525
$Main 			/ End of Program; Main is entry point