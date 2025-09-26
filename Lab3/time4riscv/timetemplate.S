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
#	addi	sp,sp,-4
#	sw	%reg,0(sp) 
#.end_macro

#.macro	POP (%reg)
#	lw	%reg,0(sp)
#	addi	sp,sp,4
#.end_macro
###################
# Macros for DTEK #
###################
#.macro	PUSH reg
#	addi sp,sp,-4
#	sw \reg,0(sp) 
#.endm

#.macro	POP reg
#	lw	\reg,0(sp)
#	addi	sp,sp,4
#.endm
#############################################################

	.data
	.align 2
mytime:	.word 	0x5957
timstr:	.asciz 	"text more text lots of text\0"
	.text
	.globl timetemplate, tick, time2string, display_string, delay

# Function for displaying a string with a newline at the end	
display_string:	
	li a7,4
	ecall
	li a0, 10
	li a7,11
	ecall
	jr ra
	
timetemplate:
	la	a0, timstr
	jal     display_string
	
	# wait a little
	li	a0, 2		# ms
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

	
# tick: update time pointed to by $a0
tick:	lw	t0, 0(a0)	# get time
	addi	t0, t0, 1	# increase
	andi	t1, t0, 0xf	# check lowest digit
	sltiu	t2, t1, 0xa	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x6	# adjust lowest digit
	
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
hex2asc:
	li	a0, 16		# test number (from 0 to 15)
	
	addi    sp, sp, -4
	sw      ra, 0(sp)
	
	jal	hexasc		# call hexasc
	
	li	a7, 11	# write a0 to stdout
	ecall

	lw      ra, 0(sp)
	addi    sp, sp, 4
	jr      ra	

  # You can write your own code for hexasc here
  #
hexasc:
	andi	a0, a0, 0xF
	li	t0, 0x9
	ble	a0, t0, lwr
	bge	a0, t0, hgr
	
lwr:

	addi	a0, a0, 0x30
	jr	ra

hgr:
	addi	a0, a0, 0x37
	jr	ra
	
time2string:
	# --- prologue: save callee-saved regs ---
	addi sp, sp, -20
	sw   ra, 16(sp)
	sw   s0, 12(sp)
	sw   s1, 8(sp)
	sw   s2, 4(sp)
	sw   s3, 0(sp)

	mv   s0, a0        # s0 = destination buffer
	mv   s1, a1        # s1 = time-info

	# --- nibble 1: tens of minutes (bits 15..12) ---
	srli s2, s1, 12
	andi s2, s2, 0xF
	mv   a0, s2        
	jal  hexasc        
	sb   a0, 0(s0)
	addi s0, s0, 1

	# --- nibble 2: ones of minutes (bits 11..8) ---
	srli s2, s1, 8
	andi s2, s2, 0xF
	mv   a0, s2
	jal  hexasc
	sb   a0, 0(s0)
	addi s0, s0, 1

	# --- colon ':' ---
	li   s3, 0x3A
	sb   s3, 0(s0)
	addi s0, s0, 1

	# --- nibble 3: tens of seconds (bits 7..4) ---
	srli s2, s1, 4
	andi s2, s2, 0xF
	mv   a0, s2
	jal  hexasc
	sb   a0, 0(s0)
	addi s0, s0, 1

	# --- nibble 4: ones of seconds (bits 3..0) ---
	andi s2, s1, 0xF
	mv   a0, s2
	li   t0, 2
	beq  a0, t0, print_TWO
	jal  hexasc		
	sb   a0, 0(s0)
	addi s0, s0, 1
	j	after2
	
print_TWO:
	li	a0, 'T'
	sb	a0, 0(s0)
	addi	s0,s0,1

	li	a0, 'W'
	sb	a0, 0(s0)
	addi	s0,s0,1

	li	a0, 'O'
	sb	a0, 0(s0)
	addi	s0,s0,1
	
after2:
	# --- null terminator ---
	li   s3, 0x00
	sb   s3, 0(s0)

	# --- epilogue: restore regs and return ---
	lw   s3, 0(sp)
	lw   s2, 4(sp)
	lw   s1, 8(sp)
	lw   s0, 12(sp)
	lw   ra, 16(sp)
	addi sp, sp, 20
	jr	ra
	

delay:

while:
	
        ble a0, zero, stop    
        addi a0, a0, -1       
        
	# inner loop count (adjustable constant)
        li   t0, 4771         
if:
        addi t0, t0, -1       
        bne t0, zero, if   

	# next ms
        j while          
       
stop:
	jr	ra
