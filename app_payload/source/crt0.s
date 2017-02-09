.section ".init"
.arm
.align 4
.global _start
.type _start, %function
.global _main
.type _main, %function
.global _heap_size
.global _heap_base
.global _appCodeAddress
.global _tidLow
.global _tidHigh
.global _mediatype

b _start

_appCodeAddress:
	.word 0x0
_tidLow:
	.word 0x0
_tidHigh:
	.word 0x0
_mediatype:
	.word 0x0

_start:
	@ reset stack
	mov sp, #0x10000000

	@ allocate bss/heap
	@ no need to initialize as OS does that already.
	@ need to save registers because _runBootloader takes parameters

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

	b _main

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
