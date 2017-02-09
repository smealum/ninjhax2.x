.section ".init"
.arm
.align 4
.global _start
.type _start, %function
.global _serviceList
.global _changeProcess
.global _appCodeAddress
.type _changeProcess, %function

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
_runTitleCustomVector:
	b _runTitleCustom
_doExtendedCommandVector:
	b doExtendedCommand

_appCodeAddress:
	.word 0x0

_serviceList:
	.space 0x4+0xC*16, 0x00

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

_runTitleCustom:
	ldr r4, =runTitleCustom
	b _start

_stackless_memcpy:
	@ we want to be able to copy over the param block even if it's on the old stack
	@ memcpy can corrupt that old stack so we write our own crappy version that dont need no stack
	@ needs to preserve r0, r1, r2, r3, r4 and r8; fuck the rest
	@ param block should be 4-byte aligned
	@ sp : dst, r1 : src, r2 : size
	sub r6, r2, #4
	_loop:
		ldr r5, [r1, r6]
		str r5, [sp, r6]
		sub r6, #4
		cmp r6, #0
		bge _loop
	bx lr

_start:
	@ load stack arguments in r8, r9 so we can preserve them
	ldr r8, [sp]
	ldr r9, [sp, #4]

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
	stmfd sp!, {r0, r1, r2, r3, r4, r5}

		mov r5, #0x02000000

		@ allocate placeholder
			@ MEMOP COMMIT
			ldr r0, =0x3
			@ addr0
			mov r1, #0x09000000
			@ addr1
			mov r2, #0
			@ size
			mov r3, r5
			@ RW permissions
			mov r4, #3

			@ svcControlMemory
			svc 0x01
			cmp r0, #0
			strne r0, [r0]

		@ allocate actual heap/bss
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

		@ free placeholder
			@ MEMOP FREE
			ldr r0, =0x1
			@ addr0
			mov r1, #0x09000000
			@ addr1
			mov r2, #0
			@ size
			mov r3, r5
			@ RW permissions
			mov r4, #3

			@ svcControlMemory
			svc 0x01

	ldmfd sp!, {r0, r1, r2, r3, r4, r5}

	@ store r8 to pass preserved stack argument
	str r9, [sp, #-4]!
	str r8, [sp, #-4]!

	bx r4

.global start_execution
.type start_execution, %function
start_execution:
	ldr lr, =_runHbmenuVector
	bic sp, #7
	ldr pc, =0x00100000+0x000008000

.global svc_setThreadPriority
.type svc_setThreadPriority, %function
svc_setThreadPriority:
	svc 0x0C
	bx  lr

.global svc_duplicateHandle
.type svc_duplicateHandle, %function
svc_duplicateHandle:
	str r0, [sp, #-0x4]!
	svc 0x27
	ldr r3, [sp], #4
	str r1, [r3]
	bx  lr

.global svc_getResourceLimit
.type svc_getResourceLimit, %function
svc_getResourceLimit:
	str r0, [sp, #-4]!
	svc 0x38
	ldr r2, [sp], #4
	str r1, [r2]
	bx lr

.global svc_getResourceLimitLimitValues
.type svc_getResourceLimitLimitValues, %function
svc_getResourceLimitLimitValues:
	svc 0x39
	bx lr

.global svc_getResourceLimitCurrentValue
.type svc_getResourceLimitCurrentValue, %function
svc_getResourceLimitCurrentValue:
	svc 0x3a
	bx lr
