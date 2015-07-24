.section ".init"
.arm
.align 4
.global _start
.global _serviceList
.global _changeProcess

_run3dsxVector:
	b _run3dsx
_runHbmenuVector:
	b _runHbmenu
_changeProcessVector:
	b _changeProcess
_getBestProcessVector:
	b getBestProcess
_runTitleVector:
	b _runTitle
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

_changeProcess:
	ldr r4, =changeProcess
	b _start

_runTitle:
	ldr r4, =runTitle
	b _start

_stackless_memcpy:
	@ we want to be able to copy over the param block even if it's on the old stack
	@ memcpy can corrupt that old stack so we write our own crappy version that dont need no stack
	@ needs to preserve r0, r1, r2, r3, r4 and r8; fuck the rest
	@ param block should be 4-byte aligned
	@ sp : dst, r1 : src, r2 : size
	mov r6, #0
	_loop:
		ldr r5, [r1, r6]
		str r5, [sp, r6]
		add r6, #4
		cmp r6, r2
		blt _loop
	bx lr

_start:
	@ load stack argument in r8 so we can preserve it
	ldr r8, [sp]

	@ reset stack and copy argv buffer over
	@ need to copy argv to the stack because else it might be overwritten by the time the app gets to it
	mov sp, #0x10000000
	cmp r1, #0
	cmpne r2, #0
	beq skip_argv

	@ align r2 before doing anything with it
	add r2, #0xF
	and r2, #~0xF

	@ reserve stack space
	sub sp, sp, r2

	@ perform copy
	bl _stackless_memcpy
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

		@ MEMOP COMMIT
		ldr r0, =0x3
		@ addr0
		mov r1, #0x08000000
		@ addr1
		mov r2, #0
		@ size
		ldr r3, =_heap_size
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

	@ store r8 to pass preserved stack argument
	str r8, [sp, #-4]!

	bx r4

.global start_execution
.type start_execution, %function
start_execution:
	ldr lr, =_runHbmenuVector
	ldr pc, =0x00100000+0x000008000

.global svc_queryMemory
.type svc_queryMemory, %function
svc_queryMemory:
	push {r0, r1, r4-r6}
	svc  0x02
	ldr  r6, [sp]
	str  r1, [r6]
	str  r2, [r6, #4]
	str  r3, [r6, #8]
	str  r4, [r6, #0xc]
	ldr  r6, [sp, #4]
	str  r5, [r6]
	add  sp, sp, #8
	pop  {r4-r6}
	bx   lr

.global svc_duplicateHandle
.type svc_duplicateHandle, %function
svc_duplicateHandle:
	str r0, [sp, #-0x4]!
	svc 0x27
	ldr r3, [sp], #4
	str r1, [r3]
	bx  lr

.global _invalidate_icache
.type _invalidate_icache, %function
_invalidate_icache:
	cpsid i

	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	mcr p15, 0, r0, c7, c6, 0

	bx lr

.global svc_backdoor
.type svc_backdoor, %function
svc_backdoor:
	svc 0x7B
	bx lr
