.section ".init"
.arm
.align 4
.global _init
.global _start

_start:
	ldr r0, =_bss_start
	ldr r1, =_bss_end
	mov r2, #0

	_clear_bss_loop:
		str r2, [r0], #4
		cmp r0, r1
		blt _clear_bss_loop

	# blx __libc_init_array

	mov r0, #0
	mov r1, #0
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	mov r9, #0
	mov r10, #0
	mov r11, #0
	mov r12, #0
	mov sp, #0x10000000
	blx _main

_init:
	bx lr
