  # hexmain.S
  # Written 2015-09-04 by F Lundevall
  # Copyright abandonded - this file is in the public domain.

	.text
	.globl hex2asc

hex2asc:
	li	a0, 14		# test number (from 0 to 15)
	
	addi    sp,sp,-4
	sw      ra,0(sp)
	
	jal	hexasc		# call hexasc
	
	li	a7, 11	# write a0 to stdout
	ecall

	lw      ra,0(sp)
	addi    sp,sp,4
	jr      ra	

  # You can write your own code for hexasc here
  #
hexasc:
	andi	a0,a0,0xF
	li	t0,0x9
	ble	a0,t0,lwr
	bge	a0,t0,hgr
	
lwr:

	addi	a0,a0,0x30
	jr	ra

hgr:
	addi	a0,a0,0x37
	jr	ra
