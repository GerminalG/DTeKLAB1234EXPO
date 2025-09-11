# timetemplate.S
# Written 2015 by F Lundevall
# Ported 2024/06 by W Szczerek (from MIPS to RISC-V)
# Copyright abandonded - this file is in the public domain.

#############################################################
# Choose the macro syntax for RARS or DTEK-V board.         #
# The syntax for RARS is probably due to its MIPS heritage. #
#############################################################
###################
# Macros for RARS #
###################
#.macro	PUSH (%reg)
#	addi	sp,sp,-4          	#decreases the stack pointer, saves a register on the stack
#	sw	%reg,0(sp) 
#.end_macro

#.macro	POP (%reg)			#increases the stack pointer back, restores that register 
#	lw	%reg,0(sp)
#	addi	sp,sp,4
#.end_macro
###################
# Macros for DTEK #
###################
.macro	PUSH reg
	addi sp,sp,-4
	sw \reg,0(sp) 
.endm

.macro	POP reg
	lw	\reg,0(sp)
	addi	sp,sp,4
.endm
#############################################################

	.data
	.align 2
mytime:	.word 	0x5957
timstr:	.asciz 	"text more text lots of text\0"
	.text
	.globl timetemplate, tick, time2string

# Function for displaying a string with a newline at the end	
display_string:	
	li a7,4  			#print string (together with ecall)
	ecall				#At this point, a0 must contain the address of the string.syscall go through memory starting at the address of the string
	li a0, 10			# put ASCII code for newline '\n' 
	li a7,11			#print character....These two line of code combines (a0, a7) print newline character 
	ecall
	jr ra
	
timetemplate:
	la	a0, timstr		#load address of 'timstr:.asciz'
	jal     display_string
	
	# wait a little
	li	a0, 2		# ms. 2 as immediate, wait for 2ms. Delay function is expected to pause execution for some amount of time.
	jal	delay
	
	# call tick
	la 	a0, mytime
	jal	tick
	
	# call your function time2string
	la	a0, timstr
	la	t0, mytime
	lw	a1, 0(t0)
	jal	time2string
	
	# go back and do it all again
	j	timetemplate

	
# tick: update time pointed to by $a0, update time like once per second
tick:	lw	t0, 0(a0)	# get time
	addi	t0, t0, 1	# increase
	andi	t1, t0, 0xf	# check lowest digit
	sltiu	t2, t1, 0xa	# if digit < a (10), as if it is equal to 10 it is not a digit, okay, sltiu: set less than
	bnez	t2, tiend	# branch if not equal to zero. done 
	addi	t0, t0, 0x6	# adjust lowest digit if it is 10 then we need to carry it to the next nibble, as it is hex, 0xA+0x6=0x10
	
	
	andi	t1, t0, 0xf0	# check next digit
	sltiu	t2, t1, 0x60	# if digit < 6, okay
	bnez	t2, tiend
	addi	t0, t0, 0xa0	# adjust digit
	
	li	t3, 0xF
	slli	t3, t3, 0x8
	and	t1, t0, t3	# check minute digit
	addi	t3, x0, 0xA
	slli	t3, t3, 0x8
	slt	t2, t1, t3	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x600	# adjust digit - this one's okay, it's lower than 0x7FF 
	
	li	t3, 0xF
	slli	t3, t3, 0xC
	and	t1, t0, t3	# check last digit
	addi	t3, x0, 0x6
	slli	t3, t3, 0xC
	slt	t2, t1, t3	# if digit < 6, okay
	bnez	t2, tiend
	
	li	t3, 0xA
	slli	t3, t3, 0xC
	add	t0, t0, t3	# adjust last digit
tiend:	sw	t0,0(a0)	# save updated result
	jr	ra		# return

#########################################################
# Place for your functions: time2string, hex2asc, delay.#
#########################################################
time2string:
	
	addi	sp, sp, -16	#reserve 16 bytes
	sw	ra, 12(sp)
	sw	s0,  8(sp)
	sw	s1,  4(sp)
	sw	s2,  0(sp)	#spare, could omit if unused
	
	mv	s0, a0		#address of a 6-byte buffer mm:ss\0
	mv	s1, a1		#time word in BCD, MIN10 MIN1 SEC10 SEC1
	
	srli	t0, s1, 12	#EXTRACT the nibble (shift rught logical imm. and mask into t0
	andi	t0, t0, 0xF
	mv	a0, t0
	jal	hexasc
	sb	a0, 0(s0)
	
	srli	t0, s1, 8
	andi	t0, t0, 0xF
	mv	a0, t0
	jal	hexasc
	sb	a0, 1(s0)
	
	li	t1, 0x3A
	sb	t1, 2(s0)
	
	srli	t0, s1, 4	#s1 holds the packed 0x1653, srli/andi yield 5 (move bits [7:4] down to [3:0])
	andi	t0, t0, 0xF	#keep only 1 nibble
	mv	a0, t0
	jal	hexasc
	sb	a0, 3(s0)	#store in buffer 3
	
	andi	t0, s1, 0xF	
	mv	a0, t0
	jal	hexasc
	sb	a0, 4(s0)
	
	sb	x0, 5(s0)
	
	lw	s2, 0(sp)
	lw	s1, 4(sp)
	lw	s0, 8(sp)
	lw	ra, 12(sp)	
	addi	sp, sp, 16
	jr	ra
	#ra is saved and restored because subroutine calls another 'hexasc'
	#s0s0 holds destination buffer pointer and as callee-saved, it must be save and restore
	#s1, time word, same as above 
	#s2, keep a 16byte environment
	
	#a0, a1. Argument/return registers, caller-saved.
	#t0, t1. Temporaries, caller-saved.
	#x0, constant zero register, can't be changed, no saving.
	
	
hex2asc:
	li	a0, 15 # test number (from 0 to 15)
	
	addi    sp,sp,-4
	sw      ra,0(sp)
	
	jal	hexasc		# call hexasc
	
	li	a7, 11	# write a0 to stdout
	ecall

	lw      ra,0(sp)
	addi    sp,sp,4
	jr      ra	

  # You can write your own code for hexasc here
hexasc:
       andi a0, a0, 0xF     #keep only the least 4significant bits. AND reigster with immediate constant, which keep a0 under 0b1111
       li   t0, 0x9
       ble  a0, t0, nmb      #taken branch and the one below as well, they are not i instruction such as andi, therefore 0x9 needs to be stored in t0 first
       bgt  a0, t0, chr
       
nmb:
       addi a0, a0, 0x30
       jr ra
chr:
       li   t0, 0x9
       sub  a0, a0, t0
       addi a0, a0, 0x40
       jr ra
       

delay:

.loop_ms:
	ble	a0, zero, .done
	addi	a0, a0, -1
	li	t0, 15500000
.inner:
	addi	t0, t0, -1
	bnez	t0, .inner

.done:
	jr	ra

																			    


