.data
.align 2
welcome_msg: .asciz "================================================\n===== RISC-V Boot-Up Process Now Complete ======\n================================================\n"
	
.section .text
.align 2
.globl _start, enable_interrupt
	
_isr_handler:
	j _isr_routine	   /* ISR service routine here */
	j _start  	   /* This is the address that a "hard reset" will go to */
	
_isr_routine:
	// Reserve space on the stack for all 32 registers
 	addi sp, sp, -4*32
	
	// Push all registers on the stack (except x0 and x2/sp)
	sw x1, 0(sp)	
	sw x3, 8(sp)
	sw x4, 12(sp)
	sw x5, 16(sp)
	sw x6, 20(sp)
	sw x7, 24(sp)
	sw x8, 28(sp)
	sw x9, 32(sp)
	sw x10, 36(sp)
	sw x11, 40(sp)
	sw x12, 44(sp)
	sw x13, 48(sp)
	sw x14, 52(sp)
	sw x15, 56(sp)
	sw x16, 60(sp)
	sw x17, 64(sp)
	sw x18, 68(sp)
	sw x19, 72(sp)
	sw x20, 76(sp)
	sw x21, 80(sp)
	sw x22, 84(sp)
	sw x23, 88(sp)
	sw x24, 92(sp)
	sw x25, 96(sp)
	sw x26, 100(sp)
	sw x27, 104(sp)
	sw x28, 108(sp)
	sw x29, 112(sp)
	sw x30, 116(sp)
	sw x31, 120(sp)
	
	// Read mcause to determine the cause of the trap
	csrr t0, mcause
	// Check if this is an external interrupt (MSB is '1')
	li t1, 0x80000000
	bgtu t0,t1, external_irq
	
	// It's an exception (e.g., ecall), not an interrupt
	add a6, t0, zero
	addi t1, zero, 11
	beq t0, t1, skip_init_args
	csrr a0, mepc
skip_init_args:
	jal handle_exception	
	csrr t0, mepc
	addi t0,t0,4 // Advance past the instruction that caused the exception
	csrw mepc, t0
	j restore

external_irq:
    // It's an external interrupt, call the C handler
	jal handle_interrupt
    // After the C handler returns, restore context and continue
	j restore

restore:
	/* Restore all registers from the stack */
	lw x1, 0(sp)
	lw x3, 8(sp)
	lw x4, 12(sp)
	lw x5, 16(sp)
	lw x6, 20(sp)
	lw x7, 24(sp)
	lw x8, 28(sp)
	lw x9, 32(sp)
	lw x10, 36(sp)
	lw x11, 40(sp)
	lw x12, 44(sp)
	lw x13, 48(sp)
	lw x14, 52(sp)
	lw x15, 56(sp)
	lw x16, 60(sp)
	lw x17, 64(sp)
	lw x18, 68(sp)
	lw x19, 72(sp)
	lw x20, 76(sp)
	lw x21, 80(sp)
	lw x22, 84(sp)
	lw x23, 88(sp)
	lw x24, 92(sp)
	lw x25, 96(sp)
	lw x26, 100(sp)
	lw x27, 104(sp)
	lw x28, 108(sp)
	lw x29, 112(sp)
	lw x30, 116(sp)
	lw x31, 120(sp)
	
	addi sp, sp, 4*32
	mret // Return from machine trap
	
_start:
	// Set the interrupt service routine handler address
	la t0, _isr_handler
	csrw mtvec, t0
	
	// Setup a stack pointer
	li sp, 0x01FFFFFC
	
	// Go to the main C function
	jal main
	

enable_interrupt:

csrsi mie,16
csrsi mstatus,3
jr ra
