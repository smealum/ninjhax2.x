.nds

.include "../build/constants.s"

.loadtable "unicode.tbl"

.create "menu_ropbin.bin",0x0

MENU_OBJECT_LOC equ MENU_LOADEDROP_BUFADR

APP_START_LINEAR equ 0xBABE0002

GPU_REG_BASE equ 0x1EB00000

; WAITLOOP_DST equ (MENU_LOADEDROP_BUFADR - (waitLoop_end - waitLoop_start))
; WAITLOOP_OFFSET equ (- waitLoop_start - (waitLoop_end - waitLoop_start))
WAITLOOP_DST equ (HB_MEM0_WAITLOOP_TOP_ADDR - (waitLoop_end - waitLoop_start))
WAITLOOP_OFFSET equ (- waitLoop_start + WAITLOOP_DST - MENU_LOADEDROP_BUFADR)

MENU_SCREENSHOTS_FILEOBJECT equ (MENU_LOADEDROP_BUFADR + screenshots_obj)
MENU_SHAREDMEMBLOCK_PTR equ (MENU_LOADEDROP_BKP_BUFADR + sharedmemAddress) ; in BKP because we want it to persist
MENU_SHAREDMEMBLOCK_HANDLE equ (MENU_LOADEDROP_BKP_BUFADR + sharedmemHandle) ; in BKP because we want it to persist
MENU_EVENTHANDLE_PTR equ (MENU_LOADEDROP_BKP_BUFADR + eventHandle) ; in BKP because we want it to persist

MENU_DSP_BINARY equ (MENU_DSP_BINARY_AFTERSIG - 0x100)
MENU_SHAREDDSPBLOCK_PTR equ (MENU_LOADEDROP_BKP_BUFADR + shareddspAddress) ; in BKP because we want it to persist
MENU_SHAREDDSPBLOCK_HANDLE equ (MENU_LOADEDROP_BKP_BUFADR + shareddspHandle) ; in BKP because we want it to persist
HB_DSP_SIZE equ ((MENU_DSP_BINARY_SIZE + 0xfff) & 0xfffff000)
HB_DSP_ADDR equ (HB_MEM0_ADDR - HB_DSP_SIZE)

.include "menu_include.s"
.include "app_code_reloc.s"

.orga 0x0

	object:
	rop: ; real ROP starts here

		; debug
			writehwreg 0x202A04, 0x01FFFFFF

		; do some permanent relocs to app_code and app_bootloader
			store HB_MEM0_ROPBIN_ADDR + appBootloader - object, MENU_LOADEDROP_BKP_BUFADR + appCode + 0x4
			store HB_MEM0_ROPBIN_ADDR + appCode - object, MENU_LOADEDROP_BKP_BUFADR + appBootloader + 0x4 * 7
			store HB_MEM0_ROPBIN_ADDR + appBootloader - object, MENU_LOADEDROP_BUFADR + appCode + 0x4
			store HB_MEM0_ROPBIN_ADDR + appCode - object, MENU_LOADEDROP_BUFADR + appBootloader + 0x4 * 7

		; looks like this is actually not needed
		; plug dsp handle leak
			dsp_register_interrupt_events

		; unload dsp stuff
			dsp_unload_component

		; set n3ds cpu config so we can have same timing everywhere
			ptm_configure_n3ds_imm 0x00

		; same for syscore cpu percent
			apt_open_session 0, 0
			apt_set_app_cpu_limit 5
			; get_cmdbuf 4
			; .word ROP_MENU_LDR_R0R0_POP_R4PC
			; 	.word 0xDEADBABE
			; .word 0xDEADBABE
			apt_close_session 0, 0

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
			
			; sleep 1000*1000*1000, 0x00000000

			busyloop 3*1000*1000

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
				srv_get_handle MENU_LOADEDROP_BUFADR + nwmextString, 8, MENU_NWMEXT_HANDLE

				create_event MENU_EVENTHANDLE_PTR, 0

			; overwrite jump_sp APT_TitleLaunch's destination
				store MENU_LOADEDROP_BUFADR + APT_TitleLaunch_end, MENU_LOADEDROP_BKP_BUFADR + APT_TitleLaunch - 0x8 ; a = skip APT_TitleLaunch, dst = sp location

			; memcpy ropbins into hb:mem0
				memcpy HB_MEM0_ROPBIN_ADDR, MENU_LOADEDROP_BUFADR, 0x8000, 0, 0
				memcpy HB_MEM0_ROPBIN_BKP_ADDR, MENU_LOADEDROP_BKP_BUFADR, 0x8000, 0, 0

		APT_TitleLaunch_end:

		; invalidate dcache for app_code before we write to it
			invalidate_dcache MENU_OBJECT_LOC + appCode, 0x4000

		; adjust gsp commands (can't preprocess aliases)
			add_and_store_3 APP_START_LINEAR, 0xBABE0003, 0 - 0x00100000, MENU_OBJECT_LOC + gxCommandAppHook - object + 0x8
			add_and_store_3 APP_START_LINEAR, 0xBABE0007, 0 - 0x00100000, MENU_OBJECT_LOC + gxCommandAppCode - object + 0x8

		; relocate app_code
			relocate

		; flush app_code because we just wrote to it and are about to DMA it
			flush_dcache MENU_OBJECT_LOC + appCode, 0x4000

			writehwreg 0x202A04, 0x0100FF00
			
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppHook - object
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppHook - object
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppHook - object
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppHook - object

		; launch app that we want to takeover
			nss_launch_title 0xBABE0004, 0xBABE0005
			; nss_launch_title_raw 0xBABE0004, 0xBABE0005
			; nss_launch_title_update 0xBABE0004, 0xBABE0005, 0xBABE000A

			; busyloop 5*1000*1000

		; takeover app
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppHook - object
			send_gx_cmd MENU_OBJECT_LOC + gxCommandAppCode - object

			; busyloop 3*1000*1000

		; sleep for a bit
			sleep 200*1000*1000, 0x00000000

			writehwreg 0x202A04, 0x0100FFFF

		; release gsp rights
			gsp_release_right

		; sleep for a bit
			sleep 200*1000*1000, 0x00000000

		; ; try acquire gsp rights
		; 	gsp_try_acquire_right
		; 	get_cmdbuf 4
		; 	.word ROP_MENU_LDR_R0R0_POP_R4PC
		; 		.word 0xDEADBABE
		; 	cond_jump_sp_r0 MENU_LOADEDROP_BUFADR + takeover_loop + WAITLOOP_OFFSET, 0x00000000

		; memcpy wait loop to destination
		; do it before sending handles because bootloader overwrites the ropbin, so let's avoid a race condition !
			memcpy WAITLOOP_DST, (MENU_OBJECT_LOC+waitLoop_start-object), (waitLoop_end-waitLoop_start), 0, 0

		; so actually from now on it'll just be bootloader that's responsible for populating param block in hb:mem0
		; since initially it's all 00s anyway we're good, never need this memcpy
		; ; memcpy parameter block into hb:mem0
		; 	memcpy HB_MEM0_PARAMBLK_ADDR, MENU_PARAMETER_BUFADR, MENU_PARAMETER_SIZE, 0, 0

			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + fsUserString, MENU_FS_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + nssString, MENU_NSS_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + irrstString, MENU_IRRST_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + amsysString, MENU_AMSYS_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + ptmsysmString, MENU_PTMSYSM_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + gsplcdString, MENU_GSPLCD_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + nwmextString, MENU_NWMEXT_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + newssString, MENU_NEWSS_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + hbMem0String, MENU_SHAREDMEMBLOCK_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + hbNdspString, MENU_SHAREDDSPBLOCK_HANDLE
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + hbKillString, MENU_EVENTHANDLE_PTR
			wait_for_parameter_and_send MENU_LOADEDROP_BUFADR + bosspString, (MENU_BOSSP_HANDLE_MINUS_0x18 + 0x18)

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
		.word 0x01000004 ; command header (SetTextureCopy)
		.word MENU_OBJECT_LOC + appHook - object ; source address
		.word 0xDEADBABE ; destination address (standin, will be filled in)
		.word 0x00000100 ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused

	gxCommandAppCode:
		.word 0x01000004 ; command header (SetTextureCopy)
		.word MENU_OBJECT_LOC + appCode - object ; source address
		.word 0xDEADBABE ; destination address (standin, will be filled in)
		.word 0x00010000 ; size
		.word 0xFFFFFFFF ; dim in
		.word 0xFFFFFFFF ; dim out
		.word 0x00000008 ; flags
		.word 0x00000000 ; unused

	.align 0x4
	eventHandle:
		.word 0x00000000
	sharedmemAddress:
		.word 0x00000000
	sharedmemHandle:
		.word 0x00000000
	shareddspAddress:
		.word 0x00000000
	shareddspHandle:
		.word 0x00000000
	nwmextHandle:
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
	nwmextString:
		.ascii "nwm::EXT"
		.byte 0x00
	newssString:
		.ascii "news:s"
		.byte 0x00
		.byte 0x00
	hbMem0String:
		.ascii "hb:mem0"
		.byte 0x00
	hbNdspString:
		.ascii "hb:ndsp"
		.byte 0x00
	hbKillString:
		.ascii "hb:kill"
		.byte 0x00
	bosspString:
		.ascii "boss:P"
		.byte 0x00
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
		.word ROP_MENU_STRNE_R4R0x4_POP_R4PC ; strne r4, [r0, #4] ; pop {r4, pc}
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
			cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_keycomboend + WAITLOOP_OFFSET, 0x1000001C, 0x382 ; L+R+B+DOWN

				nss_terminate_tid_deref MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_tidlow, MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_tidhigh, 1000*1000*1000, WAITLOOP_OFFSET

				sleep 2000*1000*1000, 0x00000000

				apt_open_session 0, WAITLOOP_OFFSET
				apt_finalize 0x300
				apt_close_session 0, WAITLOOP_OFFSET

				load_store MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_marker + 4, MENU_LOADEDROP_BUFADR + WAITLOOP_OFFSET + waitLoop_marker - 4

			waitLoop_keycomboend:

			; check event
			wait_synchronizationn DUMMY_PTR, MENU_EVENTHANDLE_PTR, 1, 1, 1000*1000, 0, 1, WAITLOOP_OFFSET

			cond_jump_sp_r0 MENU_LOADEDROP_BUFADR + waitLoop_eventend + WAITLOOP_OFFSET, 0x0

				exit_process

			waitLoop_eventend:

			apt_open_session 1, WAITLOOP_OFFSET
			apt_inquire_notification 0x101, MENU_LOADEDROP_BUFADR + notificationType, 1, WAITLOOP_OFFSET
			store_r0 MENU_LOADEDROP_BUFADR + notificationError
			apt_close_session 1, WAITLOOP_OFFSET

			; first check for error
			cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_notificationend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + notificationError, 0x0

				; power button pressed notification type
				cond_jump_sp MENU_LOADEDROP_BUFADR + waitLoop_powerend + WAITLOOP_OFFSET, MENU_LOADEDROP_BUFADR + notificationType, 0x8

					nss_shutdown_async
					
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

					set_lr MENU_NOP
					load_r0 MENU_LOADEDROP_BUFADR + linearMem
					memcpy_r0_lr MENU_LOADEDROP_BUFADR + displayCapture, 0x20

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

					ptm_configure_n3ds MENU_LOADEDROP_BUFADR + waitLoop_n3ds_cpu + WAITLOOP_OFFSET, WAITLOOP_OFFSET

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

				ptm_configure_n3ds MENU_LOADEDROP_BUFADR + waitLoop_n3ds_cpu + WAITLOOP_OFFSET, WAITLOOP_OFFSET

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
				.word ROP_MENU_POP_R4R5R6R7R8R9R10PC ; rop gadget to replace pivot with
				waitLoop_tidlow:
				.word 0xBABE0008 ; tid_low
				waitLoop_tidhigh:
				.word 0xBABE0009 ; tid_high
				.ascii "cnfg"
				waitLoop_n3ds_cpu:
				.word 0x000000FF ; by default we do nothing with the n3ds cpu config
				.word 0x00000000
			gsp_acquire_right
			writehwreg 0x202A04, 0x01FF00FF
			; todo : add cache invalidation for ropbin
			sleep 100*1000*1000, 0x00000000
			memcpy MENU_LOADEDROP_BUFADR, HB_MEM0_ROPBIN_ADDR, 0x8000, 0, 0
			flush_dcache MENU_LOADEDROP_BUFADR, 0x8000
			jump_sp MENU_LOADEDROP_BUFADR
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
	appBootloader_end:


.Close
