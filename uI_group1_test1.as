/ Program : micro Ops Group 1
/
/ Testing each order of operations separately
/
/-------------------------------------------
/
/ Code Section
/
*0200			/ start at address 0200
Main, 	
	cla cll 	/ AC=0 LR=0
				/ Check that each Order of Operations section acts correctly alone
				/ Order of Operations 1 (combos of cla and cll)
	tad A1 		/ AC=7777 LR =0
	tad A2		/ AC=0 LR=1
	tad A1 		/ AC=7777 LR=1					
	cla			/ AC=0 LR=1		
	tad A1		/ AC=7777 LR=1 
	cll			/ AC=7777 LR=0	
	tad A2		/ AC=0 LR=1
	tad A1 		/ AC=7777 LR=1
	cll cla		/ AC=0 LR=0 
				/ Order of Operations 2 (combos of cma cml) I12
	tad A3		/ AC=2525 LR=0
	cma			/ AC=5252 LR=0
	tad A0		/ Separate uIs
	cml			/ AC=5252 LR=1
	tad A0		/ Separate uIs
	cma cml		/ AC=2525 LR=0
				/ Order of Operations 3 (iac) I18
	tad A0		/ Separate uIs
	iac			/ AC=2526 LR=0
				/ Order of Operations 4 (rar, rtr, ral, rtl) I20
	cll cla		/ AC=0 LR=0
	tad A1		/ AC=7777 LR=0
	tad A2		/ AC=0 LR=1
	tad A3		/ AC=2525 [555] LR=1
	rar			/ AC=5252 [aaa] LR=1
	tad A0		/ Separate uIs
	ral			/ AC=2525 LR=1
	tad A0		/ Separate uIs
	rar ral		/ AC=2525 LR=1
	tad A0		/ Separate uIs
	rtr			/ AC=6525 [d55] LR=0
	tad A0		/ Separate uIs
	rtl			/ AC=2525 LR=1
	tad A0		/ Separate uIs
	rtr rtl		/ AC=2525 LR=1
				/ Check that all possible combinations of orders of operations interact correctly
	
	
	
	
	
	
	
	
	
	
	
	
	
	hlt 		/ Halt program
	jmp Main	/ To continue - goto Main
/
/ Data Section
/
*0250 			/ place data at address 0250
A0, 0
A1, 7777		/ A is all 1s 
A2, 1			/ 1 bit
A3, 2525		/ 010 101 010 101

				/Results


$Main 			/ End of Program; Main is entry point