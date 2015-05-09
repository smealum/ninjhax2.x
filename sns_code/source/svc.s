.arm

.align 4

.global svc_controlProcessMemory
.type svc_controlProcessMemory, %function
svc_controlProcessMemory:
	stmfd sp!, {r4, r5}
	ldr r4, [sp, #0x8]
	ldr r5, [sp, #0xC]
	svc 0x70
	ldmfd sp!, {r4, r5}
	bx lr

.global svc_mapProcessMemory
.type svc_mapProcessMemory, %function
svc_mapProcessMemory:
	svc 0x71
	bx lr

.global svc_unmapProcessMemory
.type svc_unmapProcessMemory, %function
svc_unmapProcessMemory:
	svc 0x72
	bx lr

.global svc_createPort
.type svc_createPort, %function
svc_createPort:
       push {r0, r1}
       svc 0x47
       ldr r3, [sp, #0]
       str r1, [r3]
       ldr r3, [sp, #4]
       str r2, [r3]
       add sp, sp, #8
       bx lr

.global svc_replyAndReceive
.type svc_replyAndReceive, %function
svc_replyAndReceive:
       str   r0, [sp,#-4]!
       svc   0x4f
       ldr   r2, [sp]
       str   r1, [r2]
       add   sp, sp, #4
       bx    lr

.global svc_acceptSession
.type svc_acceptSession, %function
svc_acceptSession:
       str   r0, [sp,#-4]!
       svc   0x4a
       ldr   r2, [sp]
       str   r1, [r2]
       add   sp, sp, #4
       bx    lr
