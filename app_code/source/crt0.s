.section ".init"
.arm
.align 4
.global _init
.global _start
.global _bootloaderAddress
.global _heap_size

b _start

_bootloaderAddress:
	.word 0x00000000

_heap_size:
	.word 0x01000000

_start:
	@ allocate bss/heap
	@ no need to initialize as OS does that already.
	stmfd sp!, {r0, r1, r2, r3, r4}

		@ MEMOP COMMIT
		ldr r0, =0x3
		@ addr0
		mov r1, #0x08000000
		@ addr1
		mov r2, #0
		@ size
		adr r3, _heap_size
		ldr r3, [r3]
		@ RW permissions
		mov r4, #3

		@ svcControlMemory
		svc 0x01

		@ save heap address
		mov r1, #0x08000000
		ldr r2, =_heap_base
		str r1, [r2]

	ldmfd sp!, {r0, r1, r2, r3, r4}

	@ ldr r0, =_bss_start
	@ ldr r1, =_bss_end
	@ mov r2, #0

	@ _clear_bss_loop:
	@ 	str r2, [r0], #4
	@ 	cmp r0, r1
	@ 	blt _clear_bss_loop

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
