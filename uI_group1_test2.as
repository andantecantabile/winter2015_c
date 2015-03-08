/ Program : micro Ops Group 1
/
/ Testing the combinations of the order of operations groups
/
/-------------------------------------------
/
/ Code Section
/
*0200				/ start at address 0200
Main, 	
	cla cll 		/ AC=0 LR=0
					/ Check that all possible combinations of orders of operations interact correctly
	tad A0			/ Separates uI
	iac rar			/ AC=0 LR=1
	tad A3			/ AC=2525 [555] LR=1
	cma rar			/ AC=6525 [d55] LR=0
	tad A0			/ Separates uI
	cma iac			/ AC=1253 [2ab] LR=0
	tad A0			/ Separates uI
	cma iac rar		/ AC=3252 [6aa] LR=1
	tad A0			/ Separates uI
	cla rar			/ AC=4000 [800] LR=0
	tad A0			/ Separates uI
	cla iac			/ AC=1 LR=0
	tad A3			/ AC=2526 [556] LR=0
	cla iac rar		/ AC=0 LR=1
	tad A3			/ AC=2525 [555] LR=1		I17
	cla cma			/ AC=7777 LR=1
	tad A3			/ AC=2524 [554] LR=1
	cla cma rar		/ AC=3777 [7ff] LR=1
	tad A3			/ AC=6524 [d54] LR=1
	cla cma iac		/ AC=0 LR=0
	tad A3			/ AC=2525 [555] LR=0
	cla cma iac rar	/ AC=4000 [800] LR=0
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