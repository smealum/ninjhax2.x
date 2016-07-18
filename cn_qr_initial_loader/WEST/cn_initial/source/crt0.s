.section ".init"
.arm
.align 4
.global _init
.global _start

_start:
	# blx __libc_init_array
	# swi 0xa
	# mov r0, #0
	# mov r1, #0
	# mov r2, #0
	# mov r3, #0
	# mov r4, #0
	# mov r5, #0
	# mov r6, #0
	# mov r7, #0
	# mov r8, #0
	# mov r9, #0
	# mov r10, #0
	# mov r11, #0
	# mov r12, #0
	mov sp, #0x10000000
	blx _main

_init:
	bx lr

.global getThreadCommandBuffer
.type getThreadCommandBuffer, %function
getThreadCommandBuffer:
	mrc p15, 0, r0, c13, c0, 3
	add r0, #0x80
	bx lr

.global svc_releaseMutex
.type svc_releaseMutex, %function
svc_releaseMutex:
	svc 0x14
	bx lr

.global svc_controlMemory
.type svc_controlMemory, %function
svc_controlMemory:
	stmfd sp!, {r0, r4}
	ldr r0, [sp, #0x8]
	ldr r4, [sp, #0x8+0x4]
	svc 0x01
	ldr r2, [sp], #4
	str r1, [r2]
	ldr r4, [sp], #4
	bx lr

.global svc_sleepThread
.type svc_sleepThread, %function
svc_sleepThread:
	svc 0x0A
	bx lr

.global svc_signalEvent
.type svc_signalEvent, %function
svc_signalEvent:
	svc 0x18
	bx lr

.global svc_closeHandle
.type svc_closeHandle, %function
svc_closeHandle:
	svc 0x23
	bx lr

.global svc_waitSynchronization1
.type svc_waitSynchronization1, %function
svc_waitSynchronization1:
	svc 0x24
	bx  lr

.global svc_exitProcess
.type svc_exitProcess, %function
svc_exitProcess:
	svc 0x03
	bx  lr

.global svc_waitSynchronizationN
.type svc_waitSynchronizationN, %function
svc_waitSynchronizationN:
	str r5, [sp, #-4]!
	mov r5, r0
	ldr r0, [sp, #0x4]
	ldr r4, [sp, #0x4+0x4]
	svc 0x25
	str r1, [r5]
	ldr r5, [sp], #4
	bx lr

.global svc_sendSyncRequest
.type svc_sendSyncRequest, %function
svc_sendSyncRequest:
	svc 0x32
	bx lr
