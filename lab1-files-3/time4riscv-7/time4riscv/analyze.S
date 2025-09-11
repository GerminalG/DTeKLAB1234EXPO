# analyze.S
# Written 2015 by F Lundevall
# Ported 2024/07 by W Szczerek (from MIPS to RISC-V)
# Copyright abandonded - this file is in the public domain.

	.text
	.globl analyze
analyze:
	li	s0, 0x30                       #30 is where number starts on ASCII Table. li load immediate
loop:
	mv	a0, s0				# copy from s0 to a0. mv move.syscal prints what is in a0 and has to be a0, not a1 or anything else, they will be ignored, s0 stable across program's variable, a0 syscal input	
	li	a7, 11				# environment call with a7 = 11 will print out
	                                       #a7 is reserved register for system call nr (order for service I WANT), a0-a6 are syscall argument(parameters, extra info) which nothing would happen since simulator only look at a7 for systcall number
	ecall					# one byte from a0 to the Run I/O window. environment call

	addi	s0, s0, 0x03	# what happens if the constant is changed? It move on the table accordingly, such as when it was 1, it moves one by one, and now it is 3, it only shows every three letter from the start of 0 (Ox30)
	
	li	t0, 0x5A	               #Z is 5A on ASCII table 
	ble	s0, t0, loop                   #<=
    	jr 	ra				#jr jump register. ra return address	


#a7 value see notability LAB a7 = 11 print character 
# li a0, 'H'
# li a7, 11
# ecall
#print character 'H'
