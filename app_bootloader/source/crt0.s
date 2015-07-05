.section ".init"
.arm
.align 4
.global _start
.global _serviceList

_run3dsxVector:
	b _run3dsx
_runHbmenuVector:
	b _runHbmenu
bx lr
bx lr
bx lr
bx lr
bx lr
bx lr

_serviceList:
	.space 0x4+0xC*8, 0x00

_run3dsx:
	ldr r4, =run3dsx
	b _start

_runHbmenu:
	ldr r4, =runHbmenu
	mov r1, #0
	mov r2, #0
	b _start

_start:
	@ reset stack and copy argv buffer over
	@ need to copy argv to the stack because else it might be overwritten by the time the app gets to it
	mov sp, #0x10000000
	cmp r1, #0
	cmpne r2, #0
	beq skip_argv

	sub sp, sp, r2
	stmfd sp!, {r0, r1, r2}
	add r0, sp, #0xC
	bl memcpy
	ldmfd sp!, {r0, r1, r2}
	mov r1, sp
	b done_argv

	skip_argv:
	mov r1, #0
	mov r2, #0
	done_argv:

	@ allocate bss/heap
	@ no need to initialize as OS does that already.
	@ need to save registers because _runBootloader takes parameters
	stmfd sp!, {r0, r1, r2, r3, r4}

		@ LINEAR MEMOP_COMMIT
		ldr r0, =0x10003
		@ addr0
		mov r1, #0
		@ addr1
		mov r2, #0
		@ size
		ldr r3, =_heap_size
		ldr r3, [r3]
		@ RW permissions
		mov r4, #3

		@ svcControlMemory
		svc 0x01

		@ save linear heap 
		ldr r2, =_heap_base
		str r1, [r2]

	ldmfd sp!, {r0, r1, r2, r3, r4}

	bx r4

.global start_execution
.type start_execution, %function
start_execution:
	ldr lr, =_runHbmenu
	ldr pc, =0x00100000+0x000008000

.global svc_duplicateHandle
.type svc_duplicateHandle, %function
svc_duplicateHandle:
	str r0, [sp, #-0x4]!
	svc 0x27
	ldr r3, [sp], #4
	str r1, [r3]
	bx  lr
