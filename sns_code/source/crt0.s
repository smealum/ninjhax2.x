.section ".init"
.arm
.align 4
.global _start

_start:
	ldr r0, =_bss_start
	ldr r1, =_bss_end
	mov r2, #0

	_clear_bss_loop:
		str r2, [r0], #4
		cmp r0, r1
		blt _clear_bss_loop
		
	blx _main
