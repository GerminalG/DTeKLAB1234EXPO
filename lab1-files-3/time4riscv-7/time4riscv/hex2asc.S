  # hexmain.S
  # Written 2015-09-04 by F Lundevall
  # Copyright abandonded - this file is in the public domain.

	.text
	.globl hex2asc

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
       