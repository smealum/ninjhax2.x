.section ".init"
.arm
.align 4
.global _init
.global _start
.global _bootloaderAddress
.global _heap_size
.global _mini_got_start
.global _mini_got_end
.extern _main
.type _main, %function

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

	ldmfd sp!, {r0, r1, r2, r3, r4}

	ldr r0, =_bss_start
	ldr r1, =_bss_end
	mov r2, #0

	_clear_bss_loop:
		@ str r2, [r0], #4
		add r0, #4
		cmp r0, r1
		blt _clear_bss_loop

	@ save heap address
	mov r1, #0x08000000
	ldr r2, =_heap_base
	str r1, [r2]

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
	ldr	ip, _mini_got_start
	blx ip

_init:
	bx lr

_mini_got_start:
	.word _main
_mini_got_end:

.thumb
@ dont care about order, just want to know if same or not
@ ret 0 if identical, index of first difference + 1 if not
.global _strcmp
.thumb_func
_strcmp:
	mov r3, #0
__strcmp:
	add r3, #1
	ldrb r2, [r1]
	mov r12, r2
	ldrb r2, [r0]
	add r0, #1
	add r1, #1
	cmp r2, r12
	beq _strcmp_eq
	mov r0, r3
	bx lr
	_strcmp_eq:
	cmp r2, #0
	bne __strcmp
	mov r0, #0
	bx lr
