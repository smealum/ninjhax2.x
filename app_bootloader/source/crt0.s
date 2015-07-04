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
	b _start

_start:
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

	mov sp, #0x10000000

	bx r4

.global svc_duplicateHandle
.type svc_duplicateHandle, %function
svc_duplicateHandle:
	str r0, [sp, #-0x4]!
	svc 0x27
	ldr r3, [sp], #4
	str r1, [r3]
	bx  lr
