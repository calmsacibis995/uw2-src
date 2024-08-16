/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364.c	1.3"

/***
 ***	NAME
 ***
 ***		s364.c : flagship source for the S364 display library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s364.h"
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the interface between the board level
 ***	S364 library/SI  and the chipset level functionality.
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***
 ***	SEE ALSO
 ***			devices/s364_init/s364i.c
 ***			s364_state.c
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***	HISTORY
 ***
 ***/

PUBLIC


/***
 ***	Includes.
 ***/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/errno.h>
#include <sys/inline.h>
#include "sidep.h"
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/*
 * Initialization flags.
 */
#define S364_INITIALIZE_CHIPSET_DETECTION_FAILED			(1 << 0)
#define S364_INITIALIZE_BUS_KIND_DETECTION_FAILED 			(1 << 1)
#define S364_INITIALIZE_DETECT_MEMORY_SIZE_FAILED 			(1 << 2)
#define S364_INITIALIZE_PARSE_USER_OPTIONS_FAILED 			(1 << 3)
#define S364_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND 		(1 << 4)
#define S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED 		(1 << 5)
#define S364_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED			(1 << 6)
#define S364_INITIALIZE_KD_MODE_SWITCH_FAILED				(1 << 7)
#define S364_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE	(1 << 8)
#define S364_INITIALIZE_MMAP_FAILED							(1 << 9)
#define S364_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC			(1 << 10)
#define S364_INITIALIZE_DAC_KIND_DETECTION_FAILED			(1 << 11)
#define S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED				(1 << 12)
#define S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL			(1 << 13)
#define S364_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC		(1 << 14)
#define S364_INITIALIZE_MEMORY_CLOCK_FREQUENCY_UNSUPPORTED  (1 << 15)

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/
enum s364_display_mode
{
#define DEFINE_DISPLAY_MODE(MODENAME,DESCRIPTION, FLAGS,\
 	WIDTH, HEIGHT, REFRESH_RATE, CLOCK,\
 	HORZ_DISPLAY_TIME, HORZ_SYNC_START, HORZ_SYNC_END, HORZ_TOTAL,\
 	VERT_DISPLAY_TIME, VERT_SYNC_START, VERT_SYNC_END, VERT_TOTAL)\
	S364_##MODENAME
#include "s364_modes.def"
#undef DEFINE_DISPLAY_MODE
};

struct s364_display_mode_table_entry 
{
	enum s364_display_mode	display_mode;
	char	*mode_description;

	unsigned int flags;

	int width;				/* Horizontal displayed width */
	int height;				/* Vertical displayed width */
	int refresh_rate;		/* Vertical refresh rate. */
	int clock_frequency; 	/* Clock frequency required in KHZ */
	
	/* 
	 * The following fields are specified in dots.
	 */
	unsigned int	horizontal_active_display;	
	unsigned int horizontal_sync_start;
	unsigned int horizontal_sync_end;
	unsigned int horizontal_total;

	/*
	 * The following parameters are specified in lines.
	 */
	unsigned int vertical_active_display;
	unsigned int vertical_sync_start;
	unsigned int vertical_sync_end;
	unsigned int vertical_total;
};

/***
 ***	Variables.
 ***/

#if (defined(__DEBUG__))
export boolean s364_debug = FALSE;
#endif



PRIVATE

/***
 *** 	Macros.
 ***/
#if (defined(USE_SYSI86))

/*
 * For the sysi86() call
 */
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>

/*
 * v86.h doesn't exist in ESMP
 */
#ifndef SI86IOPL					/* ESMP */
#include <sys/v86.h>
#endif

#if (defined(__STDC__))

/*
 * Rename the function to what is present in the system library.
 */
#define sysi86 _abi_sysi86

/*
 * prototype.
 */
extern int sysi86(int, ...);

#endif /* STDC */

#if (!defined(SET_IOPL))

#ifdef SI86IOPL					/* ESMP */
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl)>>12)
#else  /* SVR-4.2 */
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif
#endif /* SET_IOPL */
#endif /* USE_SYSI86 */


/***
 ***	Includes.
 ***/
#undef PRIVATE
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "generic.h"
#include "g_omm.h"

#include "lfb_map.h"

#include "s364_regs.h"
#include "s364_opt.h"
#include "s364_state.h"
#include "s364_cursor.h"
#include "s364_gs.h"
#include "s364_gbls.h"
#include "s364_mischw.h"

/***
 ***	Constants.
 ***/

/*
 * Bit definitions for the flags field of the mode table entry.
 */
#define	INTERLACE												(0x1 << 0U)
#define NON_INTERLACE											(0x1 << 1U)

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/


/***
 ***	Variables.
 ***/
extern void s364__initialize__(SIScreenRec *si_screen_p,
	struct s364_options_structure *options_p);
extern void s364__vt_switch_out__(void);
extern void s364__vt_switch_in__(void);

/*
 * Table of known mode values.
 */
STATIC struct s364_display_mode_table_entry s364_display_mode_table[] =
{
#define DEFINE_DISPLAY_MODE(MODENAME, DESCRIPTION, FLAGS, \
 	WIDTH, HEIGHT, REFRESH_RATE, CLOCK,\
 	HORZ_DISPLAY_TIME, HORZ_SYNC_START, HORZ_SYNC_END, HORZ_TOTAL,\
 	VERT_DISPLAY_TIME, VERT_SYNC_START, VERT_SYNC_END, VERT_TOTAL)\
	{\
		S364_##MODENAME, DESCRIPTION, FLAGS,\
		WIDTH, HEIGHT, REFRESH_RATE, CLOCK,\
	 	HORZ_DISPLAY_TIME, HORZ_SYNC_START, HORZ_SYNC_END, HORZ_TOTAL,\
		VERT_DISPLAY_TIME, VERT_SYNC_START, VERT_SYNC_END, VERT_TOTAL\
	}
#include "s364_modes.def"
#undef DEFINE_DISPLAY_MODE
};

/***
 *** 	Functions.
 ***/

/*
 * Function : s364_set_virtual_terminal_mode
 * 
 * PURPOSE
 *
 * 	Utility function to set the virtual terminal mode to KD_GRAPHICS 
 * or KD_TEXT.
 *
 * RETURN VALUE
 * 
 *	0 on failure
 *	1 on success
 *
 */
#if (defined(USE_KD_GRAPHICS_MODE))
STATIC int
s364_set_virtual_terminal_mode(
	SIint32 virtual_terminal_file_descriptor, int mode_type)
{
	ASSERT(mode_type == KD_GRAPHICS || mode_type == KD_TEXT0);
	
	/*
	 * Inform the kernel that the console is moving to "graphics
	 * mode". 
	 */
	if (ioctl(virtual_terminal_file_descriptor, KDSETMODE,
			  mode_type) == -1)
	{
		return (0);
	}
	return (1);
}
#endif /* USE_KD_GRAPHICS_MODE */


/*
 * Function : SI entry point: s364_virtual_terminal_save
 *
 * PURPOSE
 *
 * 		Switching out of the x server mode. Restore the chipset state 
 * back to the vga state, and also save the screen / offscreen memory.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_virtual_terminal_save(void)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	int		i;
	char	*dst_p;
	char 	*src_p;
	size_t	physical_width_in_bytes;
	struct standard_vga_state	*stdvga_regs;


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	/*
	 * Save the contents of offscreen memory if necessary.
	 * Wait for the graphics engine to finish with all its commands.
	 */
	S3_WAIT_FOR_GE_IDLE();
	if ((generic_current_screen_state_p->screen_contents_p == NULL) &&
		(screen_state_p->vt_switch_save_lines > 0))
	{
		int		required_memory = 
			screen_state_p->vt_switch_save_lines *
			screen_state_p->generic_state.screen_physical_width * 
			screen_state_p->generic_state.screen_depth / 8;
		if (required_memory > screen_state_p->video_memory_size)
		{
			required_memory = screen_state_p->video_memory_size;
		}
		generic_current_screen_state_p->screen_contents_p = 
			(void *)allocate_memory(required_memory);
	}
	/*
	 * Save the first vt_switch_save_lines of the frame buffer memory.
	 */
	src_p = (char *)screen_state_p->video_frame_buffer_base_address;
	dst_p = (char *)generic_current_screen_state_p->screen_contents_p;
	physical_width_in_bytes = screen_state_p->framebuffer_stride;

	/*
	 * TODO: put a warning message if the pointer is null. 
	 */
	if (dst_p != NULL)
	{
		S3_DISABLE_MMAP_REGISTERS();
		for (i = 0; i < screen_state_p->vt_switch_save_lines; i++)
		{
			(void) memcpy((void*)dst_p, (void*)src_p, physical_width_in_bytes);
			src_p += physical_width_in_bytes;
			dst_p += physical_width_in_bytes;
		}
		S3_ENABLE_MMAP_REGISTERS();
	}

	/*
	 * Inform each concerned module that we are about to lose control
	 * of the VT.
	 */
	s364__vt_switch_out__();

	/*
	 * Wait for the graphics engine to finish with all its commands.
	 */
	S3_WAIT_FOR_GE_IDLE();
	
	/*
	 * Switch back to VGA mode.
	 * color/mono and use the appropriate addresses.
	 * Here we have to determine if the original screen was 
	 * color/mono and use the appropriate addresses.
	 */
	stdvga_regs =  &screen_state_p->register_state.
		saved_standard_vga_registers;

	/*
	 * CRTC Reserved bits. Make them 0.
	 * bit 7 of horizontal blanking is reserved.
	 * bit 7 of preset row scan is reserved.
	 * bit 7,6 of cursor start scanline are reserved.
	 * bit 7 of cursor end scanline is reserved.
	 * bit 7 of underline location is reserved.
	 * bit 4 of mode control is reserved.
	 */
	stdvga_regs->standard_vga_crtc_registers.e_h_blank &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.preset_row_scan &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.cursor_start_scan_line &= 0x3f;
	stdvga_regs->standard_vga_crtc_registers.cursor_end_scan_line &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.under_line_loc &= 0x7f;
	stdvga_regs->standard_vga_crtc_registers.crtc_mode_control &= 0xef;

	/* 
	 * Attribute controller reserved bits.
	 * bit 4 of attribute mode control is reserved.
	 * bits 7,6 of color plane enable are reserved.
	 * bits 7,6,5,4 of horizontal pixel panning are reserved.
	 * bits 7,6,5,4 of pixel padding are reserved.
	 */
	stdvga_regs->standard_vga_attribute_controller_registers.
		attr_mode_control &= 0xef;
	stdvga_regs->standard_vga_attribute_controller_registers.
		color_plane_enable &= 0x3f;
	stdvga_regs->standard_vga_attribute_controller_registers.
		horiz_pixel_panning &= 0x0f;
	stdvga_regs->standard_vga_attribute_controller_registers.
		pixel_padding &= 0x0f;

	/*
	 * Force back to vga mode.
	 * lock enhanced registers.
	 */
	screen_state_p->register_state.saved_s3_enhanced_commands_registers.
		advfunc_control = 0x2;
	screen_state_p->register_state.saved_s3_system_control_registers.
		system_config &= (unsigned char )~SYS_CNFG_ENA_ENH;

	/*
	 * Forcibly unlock the vga crtc registers.
	 */
	if(stdvga_regs->standard_vga_crtc_registers.vert_ret_end & 0x80)
	{
		stdvga_regs->standard_vga_crtc_registers.vert_ret_end &=  
			~0x80;
	}
	if(screen_state_p->register_state.saved_s3_vga_registers.crt_reg_lock
		& 0x30)
	{
		screen_state_p->register_state.saved_s3_vga_registers.
			crt_reg_lock &= ~0x30;
	}


	/*
	 * Restore the chipset state.
	 */
	if (!(screen_state_p->register_state.generic_state.register_put_state)
		  ((struct generic_screen_state *) screen_state_p,
		  REGISTER_PUT_STATE_SAVED_STATE))
	{
#if (defined(__DEBUG__))
		if (s364_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s364_virtual_terminal_save) register "
						   "init failed.\n");
		}
#endif
		return (SI_FAIL);
	}
	
#if (defined(USE_KD_GRAPHICS_MODE))
	if (!s364_set_virtual_terminal_mode(screen_state_p->generic_state.
		screen_virtual_terminal_file_descriptor,KD_TEXT0))
	{
		return (SI_FAIL);
	}
#endif
	return (SI_SUCCEED);
}


/*
 * Function : SI entry point : s364_virtual_terminal_restore
 * 
 * PURPOSE
 *
 * 		VT switching into the X server. (X server VT is now the active VT.)
 * Restore the chipset and memory state to correspond to software copy.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_virtual_terminal_restore()
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	int		i;
	char	*dst_p;
	char 	*src_p;
	size_t	physical_width_in_bytes;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

#if (defined(USE_KD_GRAPHICS_MODE))
	if (!s364_set_virtual_terminal_mode(screen_state_p->generic_state.
		screen_virtual_terminal_file_descriptor,KD_GRAPHICS))
	{
		return (SI_FAIL);
	}
#endif

	/*
	 * Figure out the current addr base for the crtc register set.
	 */
	if (inb(VGA_GENERAL_REGISTER_MISC_RD) & 0x01)
	{
		/*
		 * Color.
		 */
		 screen_state_p->vga_crtc_address =
			VGA_CRTC_REGISTER_CRTC_ADR_COLOR;
		 screen_state_p->vga_crtc_data = 
			VGA_CRTC_REGISTER_CRTC_DATA_COLOR;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_COLOR;
	}
	else
	{
		/*
		 * Monochrome.
		 */
		 screen_state_p->vga_crtc_address = 
			VGA_CRTC_REGISTER_CRTC_ADR_MONO;
		 screen_state_p->vga_crtc_data =  
			VGA_CRTC_REGISTER_CRTC_DATA_MONO;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_MONO;
	}

	ASSERT(screen_state_p->register_state.generic_state.register_put_state);
		
	/*
	 * Change mode.
	 * Reinitialize the register context to correspond to the xserver
	 * register state.
	 */
	if (!(*screen_state_p->register_state.generic_state.register_put_state)
		  ((struct generic_screen_state *) screen_state_p,
		  REGISTER_PUT_STATE_XSERVER_STATE))
	{
#if (defined(__DEBUG__))
		if (s364_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s364_virtual_terminal_restore) register "
						   "init failed.\n");
		}
#endif
		return (SI_FAIL);
	}

	/*
	 * Restore the first vt_switch_save lines of the frame buffer
	 * memory that is saved in system memory.
	 */
	if (generic_current_screen_state_p->screen_contents_p != NULL)
	{
		dst_p = (char *)screen_state_p->video_frame_buffer_base_address;
		src_p = (char *)generic_current_screen_state_p->screen_contents_p;
		physical_width_in_bytes = screen_state_p->framebuffer_stride;

		S3_DISABLE_MMAP_REGISTERS();
		for (i = 0; i < screen_state_p->vt_switch_save_lines; i++)
		{
			(void) memcpy((void*)dst_p, (void*)src_p, physical_width_in_bytes);
			src_p += physical_width_in_bytes;
			dst_p += physical_width_in_bytes;
		}
		S3_ENABLE_MMAP_REGISTERS();
	}

	/*
	 * Inform other interested modules that we are about to regain
	 * control of our VT.
	 */
	s364__vt_switch_in__();

	return (SI_SUCCEED);
}

/*
 * Function : SI Entry Point : s364_restore_screen
 *
 * PURPOSE
 *
 * 		Exit out of the X server, basically call save_screen function but dont
 * attempt to save the video / offscreen memory. Remember to close/unmap the
 * mmaped areas and file descriptors. (Unixware 2.0 can panic).
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_restore_screen(void)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (s364_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_restore_screen) restore_screen called.\n");
	}
#endif	

	/*
	 * Last time around, we do not want to save the x server screen.
	 */
	screen_state_p->vt_switch_save_lines = 0;

	/*
	 * We dont care about the return value at this point.
	 */
	(void) s364_virtual_terminal_save();

	/*
	 * Unmap the mapped areas and close the file descriptors.
	 */
	if (munmap((caddr_t)screen_state_p->video_frame_buffer_base_address,
		(size_t)screen_state_p->linear_aperture_size) == -1)
	{
		perror("munmap");
		return (SI_FAIL);
	}

	if (munmap((caddr_t)screen_state_p->register_mmap_area_base_address,
		(size_t)S364_REGISTERS_MMAP_WINDOW_SIZE_IN_BYTES) == -1)
	{
		perror("munmap");
		return (SI_FAIL);
	}

	close (screen_state_p->mmap_fd_registers);

	close (screen_state_p->mmap_fd_framebuffer);

	return(SI_SUCCEED);
}

/*
 * Function : SI Entry Point : s364_screen_saver.
 *
 * PURPOSE
 *
 * 		Called for activating the screen saver. Just turn on/off 
 * the display using the vga seguencer register.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_screen_saver(SIBool flag)
{

#if (defined(__DEBUG__))
	if (s364_debug)
	{
		(void) fprintf(debug_stream_p,
					   	"(s364_screen_saver) screen saver called.\n");
		(void) fprintf(debug_stream_p, "(s364_screen_saver){\n"
						"flag = %s}\n", (flag == SI_TRUE) ? "SI_TRUE" :
						"SI_FALSE");
	}
#endif	

	if(flag == SI_TRUE)
	{
		S3_TURN_SCREEN_ON();
	}
	else if(flag == SI_FALSE)
	{
		S3_TURN_SCREEN_OFF();
	}

	return (SI_SUCCEED);
}

/*
 * Function : s364_vision864_specific_set_registers
 *
 * PURPOSE
 *
 * 		Vision864 is a DRAM Version. Do any initialization specific 
 * to this chip. The display fifo optimization parameters L,M and N
 * are computed in this function.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_vision864_specific_set_registers(SIConfigP config_p,
	  struct s364_options_structure *options_p,
	  struct s364_screen_state *screen_state_p)
{
	int	initialization_status = 0;
	int	start_display_fifo_fetch_value;

	/*
	 * Start display FIFO register CR-3B overridden.
	 * In 964 & 928 series of chipsets, this was "Data xfer execute posn"
	 * register.
	 */
	screen_state_p->register_state.s3_vga_registers.
		bkwd_compat_3 |= BKWD_3_ENB_SFF ;

#ifdef DELETE	
	start_display_fifo_fetch_value = screen_state_p->register_state.
		standard_vga_registers.standard_vga_crtc_registers.h_total - 5;
#endif


	/*
	 * We don't have info as to how to compute start display fifo
	 * fetch value. As of now, the values are hard coded.
	 */
	switch(screen_state_p->generic_state.screen_displayed_width)
	{
		case 640:
				start_display_fifo_fetch_value = 
					screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_total - 7;
				break;
		case 800:
				start_display_fifo_fetch_value = 
					screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_total - 10;
				break;
		case 1024:
				start_display_fifo_fetch_value = 
					screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_total - 12;
				/*
				 * M parameter value overridden. Hard coded now as we
				 * have no info how to compute it.
				 */
				screen_state_p->register_state.
					s3_system_extension_registers.extended_mem_control_2 
						= 0xd0;
				break;
		case 1152:
				start_display_fifo_fetch_value = 
					screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_total - 17;
				screen_state_p->register_state.
					s3_system_extension_registers.extended_mem_control_2 
						= 0xa0;
				break;
		case 1280:
				start_display_fifo_fetch_value = 
					screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_total - 20;
				screen_state_p->register_state.
					s3_system_extension_registers.extended_mem_control_2 
						= 0x60;
				break;
		case 1600:
				break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
	}	



	screen_state_p->register_state.s3_vga_registers.
		data_execute_position = start_display_fifo_fetch_value & 0xFF;

	screen_state_p->register_state.s3_system_extension_registers.
		extended_horz_ovfl |= (start_display_fifo_fetch_value & 0x100U) >> 2;

	/*
	 * Chipset cursor is used only in the case of vision864.
	 * Enable  windows cursor mode.
	 */

	screen_state_p->register_state.s3_system_extension_registers.
		extended_dac_control |= EX_DAC_CT_CURSOR_MODE_X11;	

	/*
	 * The dac on this board doesn't support a dac rgb width of 8.
	 * (or We don't have info on progrmming the dac for 8 rgb width.)
	 * Hack to make it 6.
	 */
	options_p->dac_rgb_width = S364_OPTIONS_DAC_RGB_WIDTH_6; 

	return(initialization_status);
}

/*
 * Function : s364_vision864_specific_set_registers
 *
 * PURPOSE
 *
 * 		Vision864 is a VRAM Version. Do any initialization specific 
 *	to this chip. 0 on success
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_vision964_specific_set_registers(SIConfigP config_p,
	  struct s364_options_structure *options_p,
	  struct s364_screen_state *screen_state_p)
{
	int initialization_status = 0;
	int data_execute_position_value;

	if( config_p->disp_w >= 1280 )
	{
		/*
		 * For horizontal resolutions more than or equal to 1280, set 
		 * Serial Access Mode control to 256 words.
		 * Refer Vision964 manual page 17-7.
		 */
		screen_state_p->register_state.s3_system_extension_registers.
		extended_linear_addr_window_control |= LAW_CTL_SAM_256;
	}

	data_execute_position_value =
		( int )(screen_state_p->register_state.standard_vga_registers.
		standard_vga_crtc_registers.h_total + 
		screen_state_p->register_state.standard_vga_registers.
		standard_vga_crtc_registers.s_h_sy_p) / 2;

	screen_state_p->register_state.s3_vga_registers.
		data_execute_position = data_execute_position_value & 0xFF ;
	
	screen_state_p->register_state.s3_system_extension_registers.
		extended_horz_ovfl |= (data_execute_position_value & 0x100U) >> 2;

	/*
	 * We should always enable Data Transfer Execute Position.
	 * Else, we get problems in modes above 1152x900x16 @60 Hz in
	 * #9GXE64PRO.
	 */
	screen_state_p->register_state.s3_vga_registers.
		bkwd_compat_3 |= BKWD_3_ENB_DTPC;

	return (initialization_status);
}



/*
 * Function : s364_compute_crtc_values.
 *
 * PURPOSE
 *
 * 		Check the availability of the requested mode in the modes table.
 * Updates the in memory values of the VGA crtc registers. 
 * Updates the in memory values of the S3 extended VGA registers.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_compute_crtc_values(SIConfigP config_p,
	  struct s364_options_structure *options_p,
	  struct s364_screen_state *screen_state_p)
{
	
	int 			initialization_status = 0;
	boolean			option_line_crtc_scan_failed = 1;
	int    			physical_width;
	unsigned char	tmp;

	struct vga_crt_controller_register_state 	*vga_crtc_registers;

	if (options_p->modedb_string) 
	{
		struct s364_display_mode_table_entry	*user_display_mode_p;
		float 	clock_frequency;

		user_display_mode_p = (struct s364_display_mode_table_entry *)
			allocate_and_clear_memory(sizeof(
			struct s364_display_mode_table_entry));

		if(sscanf(options_p->modedb_string,
			"%f %d %d %d %d %d %d %d %d", 
			&clock_frequency,
			&user_display_mode_p->horizontal_active_display,	
			&user_display_mode_p->horizontal_sync_start,
			&user_display_mode_p->horizontal_sync_end,
			&user_display_mode_p->horizontal_total,
			&user_display_mode_p->vertical_active_display,
			&user_display_mode_p->vertical_sync_start,
			&user_display_mode_p->vertical_sync_end,
			&user_display_mode_p->vertical_total) != 9)
		{
			(void) fprintf(stderr,
			S364_INITIALIZE_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE,
			options_p->modedb_string);
			free_memory(user_display_mode_p);
		}
		else
		{
			user_display_mode_p->mode_description = 
				S364_DEFAULT_USER_MODE_DESCRIPTION;
			user_display_mode_p->clock_frequency = clock_frequency * 1000;
			user_display_mode_p->width = config_p->disp_w;
			user_display_mode_p->height = config_p->disp_h;
			user_display_mode_p->refresh_rate = (clock_frequency * 1000000)/
				(user_display_mode_p->horizontal_total * 
				 user_display_mode_p->vertical_total) + 0.5;
			screen_state_p->display_mode_p = user_display_mode_p;
			option_line_crtc_scan_failed = 0;
		}
	}

	if (option_line_crtc_scan_failed)
	{
		/*
		 * check if the vendor level code has already initialized any
		 * mode tables. If not then check the mode table for any
		 * matching mode data.
		 */
		if (!screen_state_p->display_mode_p)
		{
			enum s364_display_mode mode_count;
			struct s364_display_mode_table_entry *entry_p;

			for(mode_count = S364_MODE_NULL + 1,
				entry_p = &(s364_display_mode_table[S364_MODE_NULL + 1]);
				mode_count < S364_MODE_COUNT; 
				++mode_count, ++entry_p)
			{
				double refresh_difference = /* make a positive value */
					(config_p->monitor_info.vfreq > entry_p->refresh_rate) ?
					(config_p->monitor_info.vfreq - entry_p->refresh_rate) :
					(entry_p->refresh_rate - config_p->monitor_info.vfreq);
				/*
				 * look for a matching entry in the modetable :
				 */
				if ((config_p->disp_w == entry_p->width) &&
					(config_p->disp_h == entry_p->height) &&
					(refresh_difference < DEFAULT_S364_EPSILON))
				{
					/*
					 * Found a match.
					 */
					screen_state_p->display_mode_p = entry_p;
					break;
				}
			}
		}
	}

	/*
	 * If display_mode_p is Null, we dont have the requested mode.
	 */
	if (!screen_state_p->display_mode_p)
	{
		return (S364_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND);
	}


	/*
	 * Temporary HACK. To be removed later.
	 */

	if( screen_state_p->chipset_kind == 
			S364_CHIPSET_KIND_S3_CHIPSET_VISION864 
		&& screen_state_p->bus_kind == S364_BUS_KIND_PCI 
		&& (screen_state_p->display_mode_p->horizontal_active_display == 1280
		|| (screen_state_p->display_mode_p->horizontal_active_display == 1152
		&& screen_state_p->display_mode_p->refresh_rate > 60 )) 
		&& screen_state_p->options_p->pci_9gxe64_1280_modes == 
			S364_OPTIONS_PCI_9GXE64_1280_MODES_NO) 
	{
		fprintf(stderr, 
			"ERROR: 1280x1024 and 1152x900 modes (except lower refresh rates)\n"
			"\t- are not yet supported on #9GXE64 PCI boards. \n"
			"\t- Try some other display mode. Refer README.S364\n");
		/*
		 * Return unused initialization status.
		 */
		return ( 1 << 30 );
	}

	/*
	 * Compute the screen map, i.e., the physical memory dimensions.
	 */
	if ((screen_state_p->generic_state.screen_virtual_width * 
		 screen_state_p->generic_state.screen_depth) > (1024 * 8))
	{
		physical_width = (screen_state_p->generic_state.screen_virtual_width *
			screen_state_p->generic_state.screen_depth) / 8;
	}
	else
	{
		physical_width = 1024 ;
	}


	screen_state_p->clock_frequency = 
		screen_state_p->display_mode_p->clock_frequency;

	if (screen_state_p->generic_state.screen_depth >= 8)
	{
		screen_state_p->register_state.s3_vga_registers.misc_1 |=
			MISC_1_ENH_256;
		screen_state_p->register_state.s3_system_extension_registers.
			extended_system_control_1 |= (unsigned char)
			(((unsigned)screen_state_p->generic_state.screen_depth - 1) 
			>> 3U) << 4U;
		screen_state_p->register_state.s3_enhanced_commands_registers.
			advfunc_control |= ADVFUNC_CNTL_ENH_PL_8;
	}
	else
	{
		/* screen depth 4 */
		screen_state_p->register_state.s3_enhanced_commands_registers.
			advfunc_control |= ADVFUNC_CNTL_ENH_PL_4;
	}

	/*
	 * update the necessary registers.
	 */
	screen_state_p->register_state.s3_vga_registers.mem_cfg = 
		MEM_CNFG_ENH_MAP | MEM_CNFG_CPUA_BASE | MEM_CNFG_VGA_16B;

	screen_state_p->register_state.s3_vga_registers.bkwd_compat_2 = 
		BKWD_2_BDR_SEL;

	screen_state_p->register_state.s3_vga_registers.bkwd_compat_3 = 0x00;

	screen_state_p->register_state.s3_vga_registers.crt_reg_lock  = 0x00; 

	screen_state_p->register_state.s3_vga_registers.misc_1 |= 
		MISC_1_TOP_MEM | MISC_1_REF_CNT_01;


	/* TODO: Handle when we have interlaced modes.*/
	screen_state_p->register_state.s3_vga_registers.
		interlace_retrace_start = 0x00; 
	/*
	 * Load the crtc Register Values.
	 */
	vga_crtc_registers = &(screen_state_p->register_state.
		standard_vga_registers.standard_vga_crtc_registers);

	vga_crtc_registers->h_total	= 
		(screen_state_p->display_mode_p->horizontal_total >> 3) - 5;

	vga_crtc_registers->h_d_end = 
		(screen_state_p->display_mode_p->horizontal_active_display >> 3) - 1;

	vga_crtc_registers->s_h_blank = 
		((screen_state_p->display_mode_p->horizontal_sync_start >> 3 ) - 1);

	vga_crtc_registers->e_h_blank = 
		((screen_state_p->display_mode_p->horizontal_sync_end >> 3) & 0x1F);

	vga_crtc_registers->s_h_sy_p = 
		screen_state_p->display_mode_p->horizontal_sync_start >> 3;

	vga_crtc_registers->e_h_sy_p = 
		(((screen_state_p->display_mode_p->horizontal_sync_end >> 3) & 0x20) 
			<< 2 ) | 
		(((screen_state_p->display_mode_p->horizontal_sync_end >> 3)) & 0x1F);

	vga_crtc_registers->v_total = 
		(screen_state_p->display_mode_p->vertical_total - 2) & 0xFF;

	vga_crtc_registers->ovfl_reg = 
		(((screen_state_p->display_mode_p->vertical_total -2) & 0x100) >> 8 ) | 
		(((screen_state_p->display_mode_p->vertical_active_display -1) & 0x100)
			>> 7 ) | 
		((screen_state_p->display_mode_p->vertical_sync_start & 0x100) >> 6) | 
		(((screen_state_p->display_mode_p->vertical_sync_start) & 0x100) >> 5)|
		0x10 | 
		(((screen_state_p->display_mode_p->vertical_total -2) & 0x200) >> 4 )| 
		(((screen_state_p->display_mode_p->vertical_active_display -1) & 0x200)
			>> 3 ) | 
		((screen_state_p->display_mode_p->vertical_sync_start & 0x200) >> 2 );

	vga_crtc_registers->preset_row_scan =  0;

	vga_crtc_registers->max_scan_lines = 
		((screen_state_p->display_mode_p->vertical_sync_start & 0x200) >> 4) | 
		0x40;

	vga_crtc_registers->vert_ret_start = 
		screen_state_p->display_mode_p->vertical_sync_start & 0xFF;

	vga_crtc_registers->vert_ret_end=
		(screen_state_p->display_mode_p->vertical_sync_end & 0x0F) | 0x20;

	vga_crtc_registers->vert_disp_end=
		(screen_state_p->display_mode_p->vertical_active_display -1) & 0xFF;

	vga_crtc_registers->screen_offset = (physical_width / 8 ) & 0xFF;

	vga_crtc_registers->start_vert_blank  = 
		screen_state_p->display_mode_p->vertical_sync_start & 0xFF;

	vga_crtc_registers->end_vert_blank = 
		(screen_state_p->display_mode_p->vertical_sync_start +1) & 0xFF;

	vga_crtc_registers->crtc_mode_control = 0xC3;

	vga_crtc_registers->line_cmp = 0xFF;

	vga_crtc_registers->start_addr_h = vga_crtc_registers->start_addr_l = 0;

	screen_state_p->register_state.s3_system_extension_registers.
		extended_horz_ovfl = 
	    ((((screen_state_p->display_mode_p->horizontal_total >> 3) - 5) & 
			0x100) >> 8) |
	    ((((screen_state_p->display_mode_p->horizontal_active_display >> 3) -1)
			& 0x100) >> 7) |
	    ((((screen_state_p->display_mode_p->horizontal_sync_start >> 3) - 1) & 
			0x100) >> 6) |
	    ((( screen_state_p->display_mode_p->horizontal_sync_start >> 3) & 
			0x100) >> 4);

	screen_state_p->register_state.s3_system_extension_registers.
		extended_vert_ovfl = 
		(((screen_state_p->display_mode_p->vertical_total -2) & 0x400) >> 10) | 
		(((screen_state_p->display_mode_p->vertical_active_display -1) & 0x400)
			>> 9 ) | 
		((screen_state_p->display_mode_p->vertical_sync_start & 0x400) >> 8 ) | 
		((screen_state_p->display_mode_p->vertical_sync_start & 0x400) >> 6 ) | 
		0x40;

	screen_state_p->register_state.s3_system_extension_registers.
		extended_system_control_2 |= ((physical_width / 8 ) & 0x300U) >> 4U;

	/*
	 * Compute the screen map and initialize the physical dimensions
	 * of the video memory. Update the screen states idea of physical
	 * dimensions.
	 */
	screen_state_p->generic_state.screen_physical_width =
		(physical_width * 8) / screen_state_p->generic_state.screen_depth;

	screen_state_p->generic_state.screen_physical_height = 
		(screen_state_p->video_memory_size * 8) /
		(screen_state_p->generic_state.screen_depth * 
		screen_state_p->generic_state.screen_physical_width);

	switch (screen_state_p->generic_state.screen_physical_width)
	{
		case 640:
				tmp = EX_SCTL_1_GE_SCR_W_640;
				break;
		case 800:
				tmp = EX_SCTL_1_GE_SCR_W_800;
				break;
		case 1024:
				tmp = EX_SCTL_1_GE_SCR_W_1024;
				break;
		case 1152:
				tmp = EX_SCTL_1_GE_SCR_W_1152;
				break;
		case 1280:
				tmp = EX_SCTL_1_GE_SCR_W_1280;
				break;
		case 1600:
				tmp = (screen_state_p->generic_state.screen_depth == 4 ?
						EX_SCTL_1_GE_SCR_W_1600x4 : EX_SCTL_1_GE_SCR_W_1600);
				break;
		case 2048:
				tmp = EX_SCTL_1_GE_SCR_W_2048;
				break;
	}
	screen_state_p->register_state.s3_system_extension_registers.
			extended_system_control_1 |= tmp;
	screen_state_p->register_state.s3_system_extension_registers.
			extended_system_control_2 |= 
			(unsigned)((physical_width / 8) & 0x300) >> 4U;

	return (initialization_status);
}

/*
 * Function : s364_compute_initial_register_values
 * 
 * PURPOSE
 *
 * 		Provide reasonable initialization defaults for the s364/generic vga 
 * registers depending on the mode.  The s3vga registers are handled in
 * the compute crtc_parameters function.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_compute_initial_register_values(SIConfigP config_p, 
	struct s364_options_structure *options_p,
	struct s364_screen_state *screen_state_p)
{
	int initialization_status = 0;
	int	tmp;
	unsigned char	misc_output_register = 0;
	struct vga_general_register_state   *vga_general_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_general_registers);
	struct vga_sequencer_register_state   *vga_sequencer_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_sequencer_registers);
	struct vga_graphics_controller_register_state   *vga_gc_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_graphics_controller_registers);
	struct vga_attribute_controller_register_state   *vga_ac_registers = 
			&(screen_state_p->register_state.standard_vga_registers.
			standard_vga_attribute_controller_registers);


	/*
	 * Provide defaults for the standard vga general, sequencer, 
	 * attribute controller and the graphics controller registers.
	 */
	misc_output_register |= (MISC_OUT_ENB_RAM | MISC_OUT_EXTERNAL_CLOCK);	
	/*
	 * If Color screen then program for accessing crtc address at
	 * 3d? address.
	 */
	if ( config_p->depth != 1 )
	{
		 misc_output_register |= MISC_OUT_COLOR_EMULATION;
	}
	if (options_p->hsync_polarity != S364_OPTIONS_HSYNC_POLARITY_POSITIVE)
	{
		misc_output_register |=  MISC_OUT_HSPBAR;
	}
	if (options_p->hsync_polarity != S364_OPTIONS_HSYNC_POLARITY_POSITIVE)
	{
		misc_output_register |= MISC_OUT_VSPBAR;
	}
	vga_general_registers->misc_out 		= 	misc_output_register;
	vga_general_registers->feature_control	=	0; 

	vga_sequencer_registers->reset_sync		=	0x03;
	vga_sequencer_registers->clk_mode		=	0x21;
	vga_sequencer_registers->enab_wr_plane	=	0x0F;
	vga_sequencer_registers->ch_font_sel	=	0x00;
	vga_sequencer_registers->mem_mode		=	0x0E;

	if (config_p->depth == 4)
	{
		vga_ac_registers->attr_mode_control 	= 	0x01; 
	}
	else
	{
		vga_ac_registers->attr_mode_control 	= 	0x41; 
	}
	vga_ac_registers->border_color			= 	0x00;
	vga_ac_registers->color_plane_enable	= 	0x0F;
	vga_ac_registers->horiz_pixel_panning 	= 	0x00;
	vga_ac_registers->pixel_padding 		= 	0x00;

	vga_gc_registers->set_reset 				= 	0x00;
	vga_gc_registers->enable_set_reset			= 	0x00;
	vga_gc_registers->color_cmp					= 	0x00;
	vga_gc_registers->raster_op_rotate_counter	=	0x00;
	vga_gc_registers->read_plane_select			= 	0x00;
	vga_gc_registers->graphics_controller_mode	= 	0x40;
	vga_gc_registers->mem_map_mode_control		=	0x05;
	vga_gc_registers->cmp_dont_care				=	0x0F;
	vga_gc_registers->bit_mask					=	0xFF;

	/*
	 * S3 vga register.
	 */
	screen_state_p->register_state.s3_vga_registers.config_1 = 
		screen_state_p->register_state.saved_s3_vga_registers.config_1 & 
		~CONFIG_1_DISPLAY_MEMORY_SIZE_BITS;
	switch(screen_state_p->video_memory_size)
	{
		case 1024*1024:
			screen_state_p->register_state.s3_vga_registers.config_1 |= 
				CONFIG_1_DISPLAY_MEMORY_SIZE_1024;
			break;
		case 2048*1024:
			screen_state_p->register_state.s3_vga_registers.config_1 |= 
				CONFIG_1_DISPLAY_MEMORY_SIZE_2048;
			break;
		case 3072*1024:
			screen_state_p->register_state.s3_vga_registers.config_1 |= 
				CONFIG_1_DISPLAY_MEMORY_SIZE_3072;
			break;
		case 4096*1024:
			screen_state_p->register_state.s3_vga_registers.config_1 |= 
				CONFIG_1_DISPLAY_MEMORY_SIZE_4096;
			break;
		case 6144*1024:
			screen_state_p->register_state.s3_vga_registers.config_1 |= 
				CONFIG_1_DISPLAY_MEMORY_SIZE_6144;
			break;
		case 8192*1024:
			screen_state_p->register_state.s3_vga_registers.config_1 |= 
				CONFIG_1_DISPLAY_MEMORY_SIZE_8192;
			break;
	}

	/*
	 * Initialize the S364 system control registers.
	 */
	screen_state_p->register_state.s3_system_control_registers.
		system_config = (screen_state_p->register_state.
		saved_s3_system_control_registers.system_config & 0xF1) | 
		SYS_CNFG_ENA_ENH;

	/*
	 * Initialize the S364 system extension registers.
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_system_control_1 = 
		screen_state_p->register_state.saved_s3_system_extension_registers.
		extended_system_control_1 & EX_SCTL_1_ENB_BREQ;
	screen_state_p->register_state.s3_system_extension_registers.
		extended_system_control_2 = 
		screen_state_p->register_state.saved_s3_system_extension_registers.
		extended_system_control_2 & 0x4F;

	/*
	 * Disable linear addressing by default.
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_linear_addr_window_control = 
		(screen_state_p->register_state.saved_s3_system_extension_registers.
		extended_linear_addr_window_control  & 0x58) | 
		((tmp = screen_state_p->video_memory_size / (1024*1024)) < 3 ?
			tmp : 3);

	/*
	 * Position of the linear aperture.
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_linear_addr_window_pos =
			(unsigned) screen_state_p->linear_aperture_location >> 16U;

	/*
	 * x64 chipsets provide a technique for maximizing video performance
	 * using a display FIFO that provides the display refresh data to the
	 * RAMDAC (from the display memory).
	 * This involves programming of three parameters L,M and N. Refer 
	 * page 7-8 of S3864 manual for more details.
	 */

	/*
	 * M parameter value. This value has to be programmed in 
	 * memory_control_2 register. This register bits have a 
	 * different meaning in the 801 & 928 series of chipsets.
	 * Override the value for x64 chipsets.
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_mem_control_2 = 0xf8;

	/* 
	 * N parameter value. The value 0xff ensures that the display FIFO 
	 * will always be filled before the memory is released.
	 *
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_memory_control_3 = 0xff;

	/*
	 * L parameter value.  11 bit value
	 * Number of bytes of displayed pixels per scanline divided by 
	 * 			4  for 1 MB memory configuration.
	 *			8  for 2 MB and 4 MB memory configuration
	 */
	tmp = ((screen_state_p->generic_state.screen_displayed_width * 
		screen_state_p->generic_state.screen_depth) >> 3) / 
		( screen_state_p->video_memory_size <= 1024 ? 4 : 8);

	screen_state_p->register_state.s3_system_extension_registers.
		extended_memory_control_4 = ((unsigned)(tmp & 0x700U) >> 8) |
		EXT_MCTL_4_ENB_DFLC ;

	screen_state_p->register_state.s3_system_extension_registers.
		extended_memory_control_5 = tmp & 0xFF ;

	screen_state_p->register_state.s3_system_extension_registers.
		extended_miscellaneous_control = 
		screen_state_p->register_state.saved_s3_system_extension_registers.
		extended_miscellaneous_control & 0xC6;

	screen_state_p->register_state.s3_system_extension_registers.
		extended_miscellaneous_control_1 = 
		screen_state_p->register_state.saved_s3_system_extension_registers.
		extended_miscellaneous_control_1  & 0x80;
	
	screen_state_p->register_state.s3_system_extension_registers.
		extended_miscellaneous_control_3 = 
		screen_state_p->register_state.saved_s3_system_extension_registers.
		extended_miscellaneous_control_3  & 0x77;
	/*
	 * Provide reasonable CRTC defaults depending on the mode.
	 */
	if ((initialization_status = s364_compute_crtc_values(config_p, 
		options_p, screen_state_p)))
	{
		return (initialization_status);
	}

	/*
	 * S364 Enhanced commands registers.
	 */
	screen_state_p->register_state.s3_enhanced_commands_registers.
		advfunc_control |=  ADVFUNC_CNTL_ENB_EHFC_ENHANCED;

	/*
	 * Call the chipset specific init routines.
	 */
	if (screen_state_p->chipset_kind == 
			S364_CHIPSET_KIND_S3_CHIPSET_VISION964)
	{
		initialization_status |= s364_vision964_specific_set_registers(
					config_p, options_p, screen_state_p);
	}
	else
	{
		initialization_status |= s364_vision864_specific_set_registers(
					config_p, options_p, screen_state_p);
	}
	if (initialization_status)
	{
		return(initialization_status);
	}

	/*
	 * Provision for user to supply info on new registers.
	 * initialize these crtc registers and their values.
	 */
	{
		char *new_regs_string = NULL;

		if (screen_state_p->options_p->register_values_string != NULL &&
			((new_regs_string = 
			strdup(screen_state_p->options_p->register_values_string))!=NULL)) 
		{
			struct new_s3_register_node *current_new_s3_regs_node_p;
			struct new_s3_register_node *tmp_new_s3_regs_node_p;
			char *tuple_string;
			/*
			 * Parse the tokens from the user specified string.
			 */
			while((tuple_string = strtok(new_regs_string,",")) != NULL)
			{
				tmp_new_s3_regs_node_p = (struct new_s3_register_node *)
					allocate_memory(sizeof(struct new_s3_register_node));
#if (defined(__DEBUG__))
					if (s364_debug)
					{
						(void) fprintf(debug_stream_p, 
							"\t(s364_get_mode) tuple = <%s>\n",tuple_string);
					}
#endif
				/*
				 * Get the 4 tuple. and the original value of the register.
				 */
				if(sscanf(tuple_string,"%i %i %i %i",
					&(tmp_new_s3_regs_node_p->index),
					&(tmp_new_s3_regs_node_p->mask),
					&(tmp_new_s3_regs_node_p->value),
					&(tmp_new_s3_regs_node_p->rbits)) != 4)
				{
					(void)fprintf(stderr,
						S364_PARSE_REGISTERS_FAILED_MESSAGE,tuple_string);
					free_memory(tmp_new_s3_regs_node_p);
				}
				else
				{
					S3_READ_CRTC_REGISTER(tmp_new_s3_regs_node_p->index,
						tmp_new_s3_regs_node_p->saved_value);
					tmp_new_s3_regs_node_p->next_p = NULL; 
					if (screen_state_p->register_state.
						new_s3_registers_list_head_p == NULL)
					{
						screen_state_p->register_state.
						new_s3_registers_list_head_p = 
							tmp_new_s3_regs_node_p;
						current_new_s3_regs_node_p = 
							tmp_new_s3_regs_node_p;
					}
					else
					{
						current_new_s3_regs_node_p->next_p = 
							tmp_new_s3_regs_node_p;
						current_new_s3_regs_node_p = 
							tmp_new_s3_regs_node_p;
					}
				}
				new_regs_string = NULL;
			}
		}
	}

	return ( initialization_status);
}

/*
 * Function : s364_save_original_vga_state
 * 
 * PURPOSE
 *
 * 	Function which saves the chipset state to be restored at the time
 * of relinquishing the virtual terminal or at the time of server exit. 
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_save_original_vga_state(struct s364_screen_state *screen_state_p)
{
	struct s3_register_state *register_state_p = 
					&screen_state_p->register_state;
	struct vga_sequencer_register_state *vga_sequencer_registers;
	unsigned char	*register_set;
	int		i;

	 /*
	  * Do this first to avoid any damage to the monitor.
	  * Turn off the screen.
	  */
	S3_TURN_SCREEN_OFF();

	/*
	 * First save the vga text mode register state.
	 */
	S3_RESET_AND_HOLD_SEQUENCER();

	/*
	 * Unlock all registers. 
	 */
	S3_UNLOCK_CRT_TIMING_REGISTERS();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();
	S3_UNLOCK_CLOCK_REGISTERS();

	 /*
	  * General registers. Note that the status registers are 
	  * read only.
	  */
	 register_state_p->saved_standard_vga_registers.
		standard_vga_general_registers.misc_out = 
			inb(VGA_GENERAL_REGISTER_MISC_RD);

	 register_state_p->saved_standard_vga_registers.
	 	standard_vga_general_registers.feature_control = 
			inb(VGA_GENERAL_REGISTER_FCR_RD);

	/*
	 * Sequencer Registers. These registers dont autoincrement.
	 */
	vga_sequencer_registers = &(register_state_p->
		saved_standard_vga_registers.standard_vga_sequencer_registers);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_RST_SYNC_INDEX,
		vga_sequencer_registers->reset_sync); 
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX, 
		vga_sequencer_registers->clk_mode);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_EN_WT_PL_INDEX, 
		vga_sequencer_registers->enab_wr_plane);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_CH_FONT_SL_INDEX, 
		vga_sequencer_registers->ch_font_sel);
	S3_READ_SEQUENCER_REGISTER( VGA_SEQUENCER_REGISTER_MEM_MODE_INDEX,
		vga_sequencer_registers->mem_mode);
	
	/*
	 * CRT Controller controller Registers. 
	 * These Registers dont autoincrement on a read. 
	 */
	register_set = (unsigned char *)&register_state_p->
		saved_standard_vga_registers.standard_vga_crtc_registers;

	for ( i = VGA_CRTC_REGISTER_H_TOTAL_INDEX; 
		 i <= VGA_CRTC_REGISTER_LCM_INDEX;
		 i++)
	{
		S3_READ_CRTC_REGISTER(i,*register_set);
		register_set++;
	}
	S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_GCCL_INDEX,*register_set);
	register_set++;
	S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_AT_CNTL_F_INDEX,*register_set);
	register_set++;
	S3_READ_CRTC_REGISTER(VGA_CRTC_REGISTER_AT_CNTL_I_INDEX,*register_set);

	/*
	 * Save the Graphics controller registers.
	 * These registers dont autoincrement on read.
	 */
	register_set = (unsigned char *)&register_state_p-> 
		saved_standard_vga_registers.
		standard_vga_graphics_controller_registers;
	for ( i = VGA_GRAPHICS_CONTROLLER_REGISTER_SET_RST_DT; 
		 i <= VGA_GRAPHICS_CONTROLLER_REGISTER_BIT_MASK;
		 i++)
	{
		S3_READ_GRAPHICS_CONTROLLER_REGISTER(i,*register_set);
		register_set++;
	}

	/*
	 * Save the Attribute controller registers.
	 */
	register_set = (unsigned char *)&register_state_p->
		saved_standard_vga_registers.
		standard_vga_attribute_controller_registers;
	for ( i = VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR00; 
		 i <= VGA_ATTRIBUTE_CNTL_REGISTER_PX_PAD;
		 i++)
	{
		S3_READ_ATTRIBUTE_CONTROLLER_REGISTER(i,*register_set);
		register_set++;
	}
	S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP();
	outb(VGA_ATTRIBUTE_CNTL_REGISTER_ADR,0x20);

	register_set = 
		(unsigned char *)&register_state_p->saved_s3_vga_registers;
	for ( i = S3_VGA_REGISTER_CHIP_ID_INDEX;
		  i <= S3_VGA_REGISTER_IL_RTSTART_INDEX;
		  i++)
	{
		S3_READ_CRTC_REGISTER(i,*register_set);
		register_set++;
	}

	register_set = (unsigned char *)&register_state_p->
			saved_s3_system_control_registers;
	for ( i = S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX;
		  i <= S3_SYSTEM_CONTROL_REGISTER_HWGC_DY_INDEX;
		  i++)
	{
		S3_READ_CRTC_REGISTER(i,*register_set);
		register_set++;
		/*
		 * Handle registers that are 16 bits wide.
		 */
		switch (i)
		{
			case S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_INDEX :
			case S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_INDEX :
			case S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_INDEX:	
				i++;
				S3_READ_CRTC_REGISTER(i,*register_set);
				register_set++;
			break;
		}
	}

	register_set = (unsigned char *)&register_state_p->
			saved_s3_system_extension_registers;
	for ( i = S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_1;
		  i <=  S3_SYSTEM_EXTENSION_REGISTER_EXT_MISC_3;
		  i++)
		{
			S3_READ_CRTC_REGISTER(i,*register_set);
			register_set++;
			/*
			 * This is a word wide register.
			 */
			if ( i == S3_SYSTEM_EXTENSION_REGISTER_LAW_POS)
			{
				i++;
				S3_READ_CRTC_REGISTER(i,*register_set);
				register_set++;
			}
		}

	S3_START_SEQUENCER();
	S3_TURN_SCREEN_ON();

#if (defined(__DEBUG__))
	if (s364_debug)
	{
		(void) fprintf(debug_stream_p, "(s364) Save of vga state over.\n");
	}
#endif
}

/*
 * Function : s364_check_display_mode_feasibility
 * 
 * PURPOSE
 *
 * 	Utility function called to determine if the selected mode can
 * be supported by the available display hardware/monitor combination.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_check_display_mode_feasibility(SIConfigP config_p, 
	struct s364_screen_state *screen_state_p)
{
	int						initialization_status = 0;

	/*
	 * Memory requirements check first.
	 */
	if ((screen_state_p->video_memory_size * 8 ) < 
		(config_p->virt_w * config_p->virt_h * config_p->depth))
	{
		/*
		 * Not enough memory to handle this mode.
		 */
		initialization_status |= 
			S364_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE;
	}

	/*
	 * Check if the clock chip can support the required clock frequency.
	 */

	initialization_status |=
		(screen_state_p->clock_chip_p->check_support_for_frequency(
		screen_state_p->clock_frequency));

	/*
	 * Assumption: dac module knows about the initialization status return
	 * codes
	 */
	if(screen_state_p->dac_state_p->check_support_for_mode)
	{
		initialization_status |= 
			screen_state_p->dac_state_p->check_support_for_mode(screen_state_p);
	}

	/*
	 * Check if the monitor can take the display mode.
	 */
	return (initialization_status);
}

/*
 * Function : s364_memory_map_registers_and_framebuffer
 * 
 * PURPOSE
 *
 * 	Do the necessary mmaping the register address space and the framebuffer.
 * Remember to save the opened file descriptors, since they are required
 * at server exit time for close. 
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_memory_map_registers_and_framebuffer(
	struct s364_screen_state *screen_state_p)
{
	int initialization_status = 0;
	struct	lfbmap_struct	map; /* arguments for the MAP ioctl */
	struct	lfbmap_unit_struct	map_unit;
	int mmap_fd;

	mmap_fd = open(DEFAULT_S364_FRAMEBUFFER_MMAP_DEVICE_PATH,O_RDWR);

	if (mmap_fd == -1)
	{
		perror("open");
		return (S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED);
	}

	map_unit.map_area_index = 0;
	map_unit.map_start_address_p = screen_state_p->linear_aperture_location;
	map_unit.map_length = screen_state_p->linear_aperture_size;
	
	map.map_count = 1;			/* one area to be mapped */
	map.map_struct_list_p = &map_unit;

	if (ioctl(mmap_fd, MAP_SETBOUNDS, &map) < 0)
	{
		perror("ioctl");
		(void) close(mmap_fd);
		return  -1;
	}

	screen_state_p->video_frame_buffer_base_address = 
		(void *)mmap(0, screen_state_p->linear_aperture_size,
			 PROT_READ|PROT_WRITE, MAP_SHARED,
			 mmap_fd, screen_state_p->linear_aperture_location);


	if ((caddr_t)screen_state_p->video_frame_buffer_base_address == 
		(caddr_t) -1)
	{
		close(mmap_fd);
		perror("mmap");
		return (S364_INITIALIZE_MMAP_FAILED);
	}

	screen_state_p->mmap_fd_framebuffer = mmap_fd;

	mmap_fd = open(DEFAULT_S364_REGISTERS_MMAP_DEVICE_PATH, O_RDWR);
	if (mmap_fd == -1)
	{
		perror("open");
		return (S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED);
	}

	screen_state_p->register_mmap_area_base_address = (unsigned char *)mmap
		((caddr_t)0,S364_REGISTERS_MMAP_WINDOW_SIZE_IN_BYTES,
		PROT_READ|PROT_WRITE, MAP_SHARED, mmap_fd, 
		(off_t)S364_REGISTERS_MMAP_WINDOW_PHYSICAL_BASE_ADDRESS);

	if ((caddr_t)screen_state_p->register_mmap_area_base_address  == 
		(caddr_t) -1)
	{
		close(mmap_fd);
		perror("mmap");

		/*
		 * unmap the framebuffer.
		 */
		if (munmap(screen_state_p->video_frame_buffer_base_address,
			screen_state_p->linear_aperture_size) == -1)
		{
			perror("munmap");
		}

		return (S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED);
	}

	screen_state_p->mmap_fd_registers = mmap_fd;

	return (initialization_status);
}

/*
 * Function : s364_get_board_configuration
 *
 * PURPOSE
 *
 * Determine the board configuration like chipset type/revision kind of
 * dac, clock etc... Use the utility functions present in misc_hw.c
 * determine misc stuff like dacs and clocks.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int 
s364_get_board_configuration( SIConfigP config_p, 
	struct s364_screen_state *screen_state_p, 
	struct s364_options_structure *s364_options_p)
{
	int initialization_status = 0;
	unsigned char chip_id_register_value; 	/* for chipset type */
	unsigned char config_register_1;		/* for video memory type,buskind*/

	/*
	 * Try to detect the chip and revision number.
	 */
	S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CHIP_ID_INDEX,chip_id_register_value);
	if (!screen_state_p->chipset_kind)
	{
		enum s364_chipset_kind detected_chipset_kind = 
			S364_CHIPSET_KIND_S3_CHIPSET_UNKNOWN;

		switch (chip_id_register_value & 0xF0)
		{
			case 0xC0:
				detected_chipset_kind =
					S364_CHIPSET_KIND_S3_CHIPSET_VISION864;
				break;
			case 0xD0:
				detected_chipset_kind =
					S364_CHIPSET_KIND_S3_CHIPSET_VISION964;
				break;
			default:
				detected_chipset_kind =
					S364_CHIPSET_KIND_S3_CHIPSET_UNKNOWN;
				break;
		}
		if (s364_options_p->chipset_name == 
			S364_OPTIONS_CHIPSET_NAME_AUTO_DETECT)
		{
			screen_state_p->chipset_kind = detected_chipset_kind;
		}
		else if (s364_options_p->chipset_name == 
			S364_OPTIONS_CHIPSET_NAME_VISION864) 
		{
			screen_state_p->chipset_kind = 
				S364_CHIPSET_KIND_S3_CHIPSET_VISION864;
		}
		else if (s364_options_p->chipset_name == 
			S364_OPTIONS_CHIPSET_NAME_VISION964) 
		{
			screen_state_p->chipset_kind = 
				S364_CHIPSET_KIND_S3_CHIPSET_VISION964;
		}

		/*
		 * Print override message.
		 */
		if ((screen_state_p->chipset_kind != detected_chipset_kind) &&
			(screen_state_p->chipset_kind != 
				S364_CHIPSET_KIND_S3_CHIPSET_UNKNOWN))
		{
			(void) fprintf(stderr, S364_CHIPSET_KIND_OVERRIDE_MESSAGE,
			s364_chipset_kind_to_chipset_kind_description[
				detected_chipset_kind],
			s364_chipset_kind_to_chipset_kind_description[
				screen_state_p->chipset_kind]);
		}

		/* 
		 * Unknown chipset kind is an error. No warnings. 
		 */
		if (screen_state_p->chipset_kind == 
			S364_CHIPSET_KIND_S3_CHIPSET_UNKNOWN)
		{
			return(S364_INITIALIZE_CHIPSET_DETECTION_FAILED);
		}
	}

	/*
	 * Get the product class and revision number.
	 */
	screen_state_p->chipset_revision_number = 
			s364_options_p->chipset_revision_number;

	if (s364_options_p->chipset_revision_number == -1) 
	{
		screen_state_p->chipset_revision_number = chip_id_register_value & 0x0F;
	}

	/*
	 * Get the size of the installed memory on the board.
	 */
	if (! screen_state_p->video_memory_size )
	{
		S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CNFG_REG1_INDEX,
			config_register_1);
		{
			switch (config_register_1 & CONFIG_1_DISPLAY_MEMORY_SIZE_BITS)
			{
				case CONFIG_1_DISPLAY_MEMORY_SIZE_1024:
					screen_state_p->video_memory_size = 1024*1024;
					break;
				case CONFIG_1_DISPLAY_MEMORY_SIZE_2048:
					screen_state_p->video_memory_size = 2*1024*1024;
					break;
				case CONFIG_1_DISPLAY_MEMORY_SIZE_3072:
						screen_state_p->video_memory_size = 3*1024*1024;
					break;
				case CONFIG_1_DISPLAY_MEMORY_SIZE_4096:
						screen_state_p->video_memory_size = 4*1024*1024;
					break;
				case CONFIG_1_DISPLAY_MEMORY_SIZE_6144:
					if (screen_state_p->chipset_kind == 
							S364_CHIPSET_KIND_S3_CHIPSET_VISION964)
					{
						screen_state_p->video_memory_size = 6*1024*1024;
					}
					else
					{
						initialization_status |= 
							S364_INITIALIZE_DETECT_MEMORY_SIZE_FAILED;
					}
					break;
				case CONFIG_1_DISPLAY_MEMORY_SIZE_8192:
					if (screen_state_p->chipset_kind == 
							S364_CHIPSET_KIND_S3_CHIPSET_VISION964)
					{
						screen_state_p->video_memory_size = 8*1024*1024;
					}
					else
					{
						initialization_status |= 
							S364_INITIALIZE_DETECT_MEMORY_SIZE_FAILED;
					}
					break;
				default:
					/* CONSTANTCONDITION */
					ASSERT(0);
					initialization_status |= 
						S364_INITIALIZE_DETECT_MEMORY_SIZE_FAILED;
					break;
			}
		}

		if ((screen_state_p->video_memory_size >> 10) != config_p->videoRam )
		{
			(void) fprintf(stderr,S364_MEMORY_SIZE_OVERRIDE_MESSAGE,
				screen_state_p->video_memory_size >> 10,
				config_p->videoRam);
			screen_state_p->video_memory_size = config_p->videoRam << 10U;
			initialization_status &= 
				~S364_INITIALIZE_DETECT_MEMORY_SIZE_FAILED;
		}
	}

	/*
	 * Try to detect the bus kind.
	 */
	if (!screen_state_p->bus_kind)
	{
		enum s364_bus_kind	detected_bus_kind;

		S3_READ_CRTC_REGISTER(S3_VGA_REGISTER_CNFG_REG1_INDEX,
			config_register_1);

		switch  (config_register_1 & 0x03)
		{
			case CONFIG_1_SYSTEM_BUS_SELECT_VLB:
				detected_bus_kind = S364_BUS_KIND_VLB;
				break;
			case CONFIG_1_SYSTEM_BUS_SELECT_PCI:
				detected_bus_kind = S364_BUS_KIND_PCI;
				break;
			default:
				detected_bus_kind = S364_BUS_KIND_UNKNOWN_BUS;
				break;
		}

		if (s364_options_p->bus_kind == S364_OPTIONS_BUS_KIND_AUTO_DETECT)
		{
			screen_state_p->bus_kind = detected_bus_kind;
		}
		else if (s364_options_p->bus_kind == S364_OPTIONS_BUS_KIND_VLB)
		{
			screen_state_p->bus_kind = S364_BUS_KIND_VLB;
		}
		else if (s364_options_p->bus_kind == S364_OPTIONS_BUS_KIND_PCI)
		{
			screen_state_p->bus_kind = S364_BUS_KIND_PCI;
		}

		/*
		 * Print override message.
		 */
		if ((screen_state_p->bus_kind != detected_bus_kind) &&
			(screen_state_p->bus_kind != S364_BUS_KIND_UNKNOWN_BUS))
		{
			(void) fprintf(stderr, S364_BUS_KIND_OVERRIDE_MESSAGE,
			s364_bus_kind_to_bus_kind_description[detected_bus_kind],
			s364_bus_kind_to_bus_kind_description[screen_state_p->bus_kind]);
		}

		if ( screen_state_p->bus_kind == S364_BUS_KIND_UNKNOWN_BUS)
		{
			return(S364_INITIALIZE_BUS_KIND_DETECTION_FAILED);
		}
	}

	/*
	 * Determine the dac kind. 
	 */
	if (!screen_state_p->dac_state_p)
	{
		int		return_value;


		return_value = s364_dac_detect_dac(screen_state_p);
		if (s364_options_p->dac_name == S364_OPTIONS_DAC_NAME_USE_BUILTIN)
		{
			if (return_value)
			{
				return (S364_INITIALIZE_DAC_KIND_DETECTION_FAILED);
			}
		}
		else
		{
			char	*detected_dac_name;

			/*
			 * CAVEAT: even if detect dac had failed, the dac name pointer
			 * contains a valid null string.
			 */
			detected_dac_name = allocate_memory(
				strlen(screen_state_p->dac_state_p->dac_name) + 1);
			strcpy(detected_dac_name, screen_state_p->dac_state_p->dac_name);

			screen_state_p->dac_state_p = NULL;

			switch (s364_options_p->dac_name)
			{
				case  S364_OPTIONS_DAC_NAME_ATT21C498	:
					screen_state_p->dac_state_p = 
						&(s364_dac_state_table[S364_DAC_ATT21C498]);
					break;
				case  S364_OPTIONS_DAC_NAME_BT485KPJ110:
					screen_state_p->dac_state_p = 
						&(s364_dac_state_table[S364_DAC_BT485KPJ110]);
					break;
				case S364_OPTIONS_DAC_NAME_BT485KPJ135:
					screen_state_p->dac_state_p = 
						&(s364_dac_state_table[S364_DAC_BT485KPJ135]);
					break;
				case S364_OPTIONS_DAC_NAME_PTVP3025_135MDN :
					screen_state_p->dac_state_p = 
						&(s364_dac_state_table[S364_DAC_PTVP3025_135MDN ]);
					break;
				case S364_OPTIONS_DAC_NAME_PTVP3025_175MDN :
					screen_state_p->dac_state_p = 
						&(s364_dac_state_table[S364_DAC_PTVP3025_175MDN ]);
					break;
				case S364_OPTIONS_DAC_NAME_PTVP3025_200MDN:
					screen_state_p->dac_state_p = 
						&(s364_dac_state_table[S364_DAC_PTVP3025_200MDN ]);
					break;
				default :
					/*CONSTANTCONDITION*/
					ASSERT(0);
					break;
			}

			if (strcmp(detected_dac_name, 
				screen_state_p->dac_state_p->dac_name) != 0)
			{
				(void) fprintf(stderr,
					S364_DAC_OVERRIDE_MESSAGE,
					detected_dac_name,
					screen_state_p->dac_state_p->dac_name);
			}

			free_memory(detected_dac_name);
		}
	}

	/*
	 * Determine the clock chip kind.
	 */
	if (!screen_state_p->clock_chip_p)
	{
		int result;

		result = (s364_clock_detect_clock_chip(screen_state_p));

		if (s364_options_p->clock_chip_name == 
			S364_OPTIONS_CLOCK_CHIP_NAME_USE_BUILTIN)
		{
			if (result)
			{
				initialization_status |= 
					S364_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED;
			}
		}
		else
		{
			switch (s364_options_p->clock_chip_name)
			{
				case S364_OPTIONS_CLOCK_CHIP_NAME_ICD_2061A:
					screen_state_p->clock_chip_p = 
						&(s364_clock_chip_table[S364_CLOCK_CHIP_ICD_2061A]);
					break;
				case S364_OPTIONS_CLOCK_CHIP_NAME_TVP3025:
					screen_state_p->clock_chip_p = 
						&(s364_clock_chip_table[S364_CLOCK_CHIP_TVP3025]);
					break;
				default:
					/*CONSTANTCONDITION*/
					ASSERT(0);
					break;
			}
		}
	}

	/*
	 * Initialize the physical address of the framebuffer and the register
	 * space in system memory space in the screen state.
	 */
	screen_state_p->linear_aperture_location = 
		s364_options_p->framebuffer_physical_address << 20U;
	screen_state_p->linear_aperture_size =  screen_state_p->video_memory_size;

	ASSERT(screen_state_p->linear_aperture_location > 32<<20U);

	return(initialization_status);
}

/* 
 * Function : s364_initialize_display_library
 * 
 * PURPOSE
 *
 * Initialize the S364 chipset specific SDD code.  This code will parse the
 * user specified options and configure its behaviour accordingly.  
 * There is no way for the chipset level code to determine type of dac 
 * on a S364 card. Hence it is wholly the responsibility of the board 
 * level code to do this. This function will also save the chipset
 * state that would have to be restored finally.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
function SIBool
s364_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
								SIScreenRec *si_screen_p, 
								struct s364_screen_state *screen_state_p)
{
	SIFlagsP flags_p;
	SIFunctions saved_generic_functions;
	int initialization_status = 0;	/* error flags returned to caller */
	char options_string[S364_DEFAULT_OPTION_STRING_SIZE];
	int options_string_size = sizeof(options_string);
	struct s364_options_structure *s364_options_p;
	SIConfigP config_p = si_screen_p->cfgPtr;
	
#if (defined(__DEBUG__))

	extern void s364_debug_control(boolean);
	
	s364_debug_control(TRUE);

	if (s364_debug)
	{
		(void) fprintf (debug_stream_p,
			"(s364_initialize_display_library){\n"
			"\tvirtual_terminal_descriptor = %d\n"
			"\tsi_screen_p = 0x%x\n"
			"\ts364_screen_state_p = 0x%x\n"
			"}\n",
			virtual_terminal_file_descriptor,
			(unsigned) si_screen_p,
			(unsigned) screen_state_p);
	}

#endif /* __DEBUG__ */


	ASSERT(screen_state_p != NULL);
	ASSERT(si_screen_p != NULL);

	flags_p = si_screen_p->flagsPtr;

	(void) fprintf(stderr, S364_MESSAGE_STARTUP);
	
	/*
	 * Initialize the generic data structure.
	 */

	if (initialization_status = 
		generic_initialize_display_library(virtual_terminal_file_descriptor,
		   si_screen_p, (struct generic_screen_state *) screen_state_p))
	{

#if (defined(__DEBUG__))
		if (s364_debug)
		{
			(void) fprintf(debug_stream_p, 
						   "generic initialization failed.\n");
		}
#endif /* __DEBUG__ */

		return (initialization_status);

	}

	/*
	 * Tuck away a copy of the generic screen functions.
	 */

	saved_generic_functions = *si_screen_p->funcsPtr;

	/*
	 * Convenient macro.
	 */
#define APPEND_STRING(destination, source_string_p, free_size)	\
	{															\
		const char *source_p = source_string_p;					\
		int source_size = strlen(source_p) + 1;					\
		if (free_size < source_size)							\
		{														\
			(void) fprintf(stderr, 								\
				S364_MESSAGE_BUFFER_SIZE_EXCEEDED,				\
				destination, source_p);							\
			return (SI_FAIL);									\
		}														\
		else													\
		{														\
			strncat(destination, source_p, free_size);			\
			free_size -= source_size;							\
		}														\
	}

	/*
	 * User option handling.  Null terminate the buffer first.
	 */

	options_string[0] = '\0';
	
	/*
	 * Add a file parsing option if the standard options file is
	 * present.
	 */

	if (access(S364_DEFAULT_STANDARD_OPTION_FILE_NAME, F_OK | R_OK) == 0)
	{
		APPEND_STRING(options_string,
					  S364_DEFAULT_PARSE_STANDARD_OPTIONS_FILE_COMMAND, 
					  options_string_size);
	}

	/*
	 * Add parsing options for the user specified option string.
	 */

	APPEND_STRING(options_string, config_p->info, options_string_size);
	APPEND_STRING(options_string, " ", options_string_size);

	/*
	 * Add parsing options for the standard environment option variables 
	 */
	APPEND_STRING(options_string,
				  S364_DEFAULT_PARSE_STANDARD_ENVIRONMENT_VARIABLE_COMMAND,
				  options_string_size);
	
	/*
	 * parse user configurable options.
	 */
	if (!(s364_options_p = 
		  s364_options_parse(NULL, options_string)))
	{
		
#if (defined(__DEBUG__))
		if (s364_debug)
		{
			(void) fprintf(debug_stream_p, 
						   "parsing of user options failed.\n"); 
		}
#endif /* __DEBUG__ */
		
		initialization_status |=
			S364_INITIALIZE_PARSE_USER_OPTIONS_FAILED;

		return (initialization_status);

	}
	else
	{
		/*
		 * Save the options for later persual by code.
		 */
		screen_state_p->options_p = s364_options_p;
	}

	/*
	 * Set the SDD version number as requested.
	 */
	if (s364_options_p->si_interface_version !=
		S364_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE)
	{
		switch (s364_options_p->si_interface_version)
		{
		case S364_OPTIONS_SI_INTERFACE_VERSION_1_0:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version = DM_SI_VERSION_1_0;
			break;
		case S364_OPTIONS_SI_INTERFACE_VERSION_1_1:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version = DM_SI_VERSION_1_1;
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
	}

#if (defined(USE_KD_GRAPHICS_MODE))
	if (!s364_set_virtual_terminal_mode(virtual_terminal_file_descriptor,
		KD_GRAPHICS))
	{
		initialization_status |= S364_INITIALIZE_KD_MODE_SWITCH_FAILED;
		return (initialization_status);
	}
#endif

#if (defined(USE_SYSI86))

	/*
	 * Note that we need this for exactly one register. CONFIG_CNTL
	 * which cannot be memory mapped in any way.
	 * The X server needs access to many I/O addresses.  We use the
	 * `sysi86()' call to get access to these.  Another alternative to
	 * this approach would be to have a kernel driver which
	 * allows the X server process access to the ATI registers.
	 */
	if (SET_IOPL(PS_IOPL) < 0)
	{
		perror(S364_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE);
		free_memory(s364_options_p);
		return (SI_FAIL);
	}
#endif /* USE_SYSI86 */
	

	/*
	 * Figure out the current addr base for the crtc register set.
	 */
	if (inb(VGA_GENERAL_REGISTER_MISC_RD) & 0x01)
	{
		/*
		 * Color.
		 */
		 screen_state_p->vga_crtc_address = 
			VGA_CRTC_REGISTER_CRTC_ADR_COLOR;
		 screen_state_p->vga_crtc_data =
			VGA_CRTC_REGISTER_CRTC_DATA_COLOR;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_COLOR;
	}
	else
	{
		/*
		 * Monochrome.
		 */
		 screen_state_p->vga_crtc_address =
			VGA_CRTC_REGISTER_CRTC_ADR_MONO;
		 screen_state_p->vga_crtc_data =  
			VGA_CRTC_REGISTER_CRTC_DATA_MONO;
		 screen_state_p->vga_input_status_address =
			VGA_GENERAL_REGISTER_STATUS_1_MONO;
	}

	/*
	 * Allow access to the S3 VGA and S3 system registers.
	 */
	S3_UNLOCK_CRT_TIMING_REGISTERS();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();

	/*
	 * Get the board configuration. This includes amount of memory,
	 * bus type, dac kind etc...
	 */
	initialization_status |= s364_get_board_configuration( config_p,
		screen_state_p, s364_options_p);
	if (initialization_status)
	{
#if (defined(__DEBUG__))
		if (s364_debug)
		{
			(void) fprintf(debug_stream_p, 
				"(s364_initialize_display_library)"
				"\tFailed to get board data.\n");
		}
#endif /* __DEBUG__ */
		return(initialization_status);
	}

	/*
	 * Enable mmap access to linear framebuffer and register space.
	 */
	initialization_status |= 
		s364_memory_map_registers_and_framebuffer(screen_state_p);
	if ( initialization_status) return(initialization_status);

	/*
	 * Save the original state.
	 */
	s364_save_original_vga_state(screen_state_p);

	/*
	 * The time has come to fill in state. Call the function which would
	 * initialize the register state with reasonable defaults.
	 */
	initialization_status |= s364_compute_initial_register_values(
		config_p, s364_options_p, screen_state_p);
	if ( initialization_status) return(initialization_status);

	/*
	 * How many lines of the screen does the user want saved?
	 * Limit this to the max video memory lines available.
	 */
	screen_state_p->vt_switch_save_lines =
		((s364_options_p->vt_switch_save_lines >
		  screen_state_p->generic_state.screen_physical_height) ? 
		 screen_state_p->generic_state.screen_physical_height : 
		 s364_options_p->vt_switch_save_lines);
	/*
	 * Set the loop timeout value.
	 */
	s364_graphics_engine_loop_timeout_count =
		s364_options_p->graphics_engine_loop_timeout_count;

	ASSERT(s364_graphics_engine_loop_timeout_count > 0);

	/*
	 * Set the micro delay count.
	 */
	s364_graphics_engine_micro_delay_count =
		s364_options_p->graphics_engine_micro_delay_count;

	ASSERT(s364_graphics_engine_micro_delay_count > 0);

	s364_crtc_sync_loop_timeout_count = 
		s364_options_p->crtc_sync_timeout_count;

	ASSERT(s364_crtc_sync_loop_timeout_count > 0 );


	/*
	 * We cannot pass the framebuffer pointer up for small apertures.
	 * Si has no idea of the select page mechanism. Moreover 
	 * core server seems to have a bug in the 16bit drawing modes when
	 * directly accessing the framebuffer memory.
	 */
	if (s364_options_p->framebuffer_access_for_core_server ==
			S364_OPTIONS_FRAMEBUFFER_ACCESS_FOR_CORE_SERVER_YES) 
	{
		flags_p->SIfb_pbits = screen_state_p->video_frame_buffer_base_address;
		flags_p->SIfb_width = 
			screen_state_p->generic_state.screen_physical_width;
	}
	else
	{
		flags_p->SIfb_pbits = SI_FB_NULL; 
		flags_p->SIfb_width = 0;
	}

	si_screen_p->funcsPtr->si_vt_save = s364_virtual_terminal_save;
	si_screen_p->funcsPtr->si_vt_restore = s364_virtual_terminal_restore;
	si_screen_p->funcsPtr->si_restore = s364_restore_screen;
	si_screen_p->funcsPtr->si_vb_onoff = s364_screen_saver;
	
	/*
	 * Initialize the off-screen memory manager.
	 */
	 {
		struct omm_initialization_options omm_options; 

		/*
		 * Create the initialization string for the off-screen memory
		 * manager if required.
		 */

		char *buffer_p;

		char *cursor_named_allocation_string_p = "";

		/*
		 * Do a named allocate, only if there is a possibility 
		 * of using the chipset cursor.
		 */
		if ((s364_options_p->cursor_type == 
				S364_OPTIONS_CURSOR_TYPE_AUTO_CONFIGURE ||
			s364_options_p->cursor_type == 
				S364_OPTIONS_CURSOR_TYPE_CHIPSET_CURSOR) &&
			(screen_state_p->chipset_kind == 
				S364_CHIPSET_KIND_S3_CHIPSET_VISION864)) 
		{
			cursor_named_allocation_string_p = 
				s364_cursor_make_named_allocation_string(
					si_screen_p, s364_options_p);
		}

		/* 
		 * Add one for the comma which will seperate each named allocate
		 * string
		 */

		buffer_p = (char *)allocate_memory(
			DEFAULT_S364_OMM_INITIALIZATION_STRING_LENGTH + 1 +
			strlen(cursor_named_allocation_string_p) + 1 +
			strlen(s364_options_p->omm_named_allocation_list));

		/*
		 *See if the user has supplied any additional named allocate
		 *requests
		 */

		if (s364_options_p->omm_named_allocation_list &&
			s364_options_p->omm_named_allocation_list[0] != '\0')
		{
			sprintf(buffer_p, DEFAULT_S364_OMM_INITIALIZATION_FORMAT ",%s,%s",
					DEFAULT_S364_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0,
					cursor_named_allocation_string_p,
					s364_options_p->omm_named_allocation_list);
		}
		else
		{
			sprintf(buffer_p, DEFAULT_S364_OMM_INITIALIZATION_FORMAT ",%s",
					DEFAULT_S364_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0,
					cursor_named_allocation_string_p);
		}
		
		omm_options.total_width =
		 screen_state_p->generic_state.screen_physical_width;

		omm_options.total_height =
		   screen_state_p->generic_state.screen_physical_height;

		omm_options.total_depth = screen_state_p->generic_state.screen_depth;

		omm_options.horizontal_constraint = 
			(s364_options_p->omm_horizontal_constraint ?
				s364_options_p->omm_horizontal_constraint : 
				(1U << screen_state_p->pixels_per_long_shift));

		omm_options.vertical_constraint = 
		   (s364_options_p->omm_vertical_constraint ?
				s364_options_p->omm_vertical_constraint :
				DEFAULT_S364_OMM_VERTICAL_CONSTRAINT);

		omm_options.neighbour_list_increment = 
			s364_options_p->omm_neighbour_list_increment;

		omm_options.full_coalesce_watermark = 
			s364_options_p->omm_full_coalesce_watermark;

		omm_options.hash_list_size = s364_options_p->omm_hash_list_size;

		omm_options.named_allocations_p = buffer_p;

		 (void)omm_initialize(&omm_options);

		 free_memory(buffer_p);
	}

	s364_mischw_initialize(si_screen_p, s364_options_p);

	/*
	 * Let each module initialize itself.
	 */
	s364__initialize__(si_screen_p, s364_options_p);

	/*
	 * We have all information about the requested display mode , physical
	 * screen dimensions and video memory installed. Check if the 
	 * chipset/dac/clock/monitor combination can support these for display
	 * and drawing purposes. This is done here because by this time
	 * the individual module initialization also has gone through including
	 * the number of visuals we need to support etc.
	 */
	initialization_status |= 
		s364_check_display_mode_feasibility(config_p,screen_state_p);
	if (initialization_status) return (initialization_status);

	if(screen_state_p->generic_state.screen_current_graphics_state_p)
	{
		int count;

		for(count = 0; count <
			screen_state_p->generic_state.screen_number_of_graphics_states; 
			count ++)
		{
			struct s364_graphics_state *graphics_state_p = 
				(struct s364_graphics_state *) screen_state_p->generic_state.
				screen_graphics_state_list_pp[count];
					   
			ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
			 (struct generic_graphics_state *) graphics_state_p));
	
			ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

			/*
			 * Save the generic function table
			 */

			graphics_state_p->generic_si_functions = saved_generic_functions;
		}
	}

	/*
	 * Verbose startup messages.
	 */
	if (s364_options_p->verbose_startup > 0)
	{
		(void) fprintf(stderr, S364_VERBOSE_STARTUP_PROLOGUE);
		
		(void) fprintf(stderr,
			S364_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE,
			screen_state_p->generic_state.screen_server_version_number,
			screen_state_p->generic_state.screen_sdd_version_number,
			((s364_options_p->si_interface_version ==
				S364_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE) ? 
				S364_VERBOSE_STARTUP_AUTO_CONFIGURED :
				S364_VERBOSE_STARTUP_USER_SPECIFIED)); 

		(void) fprintf(stderr,
			S364_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE,
			config_p->model,

			s364_chipset_kind_to_chipset_kind_description[
				screen_state_p->chipset_kind],
			(s364_options_p->chipset_name == 
				S364_OPTIONS_CHIPSET_NAME_AUTO_DETECT ? 
				S364_VERBOSE_STARTUP_AUTO_DETECTED : 
				S364_VERBOSE_STARTUP_USER_SPECIFIED),

			screen_state_p->chipset_revision_number,
			(s364_options_p->chipset_revision_number == 
				S364_OPTIONS_CHIPSET_REVISION_NUMBER_DEFAULT ? 
				S364_VERBOSE_STARTUP_AUTO_DETECTED : 
				S364_VERBOSE_STARTUP_USER_SPECIFIED),

			screen_state_p->dac_state_p->dac_name,
			(s364_options_p->dac_name == S364_OPTIONS_DAC_NAME_USE_BUILTIN ?
				S364_VERBOSE_STARTUP_BUILTIN_DEFAULT :
				S364_VERBOSE_STARTUP_USER_SPECIFIED),

			s364_bus_kind_to_bus_kind_description[screen_state_p->bus_kind],
			(s364_options_p->bus_kind == S364_OPTIONS_BUS_KIND_AUTO_DETECT ?
				S364_VERBOSE_STARTUP_AUTO_DETECTED :
				S364_VERBOSE_STARTUP_USER_SPECIFIED),

			screen_state_p->clock_chip_p->clock_chip_name_p,
			(s364_options_p->clock_chip_name == 
				S364_OPTIONS_CLOCK_CHIP_NAME_USE_BUILTIN?
				S364_VERBOSE_STARTUP_BUILTIN_DEFAULT:
				S364_VERBOSE_STARTUP_USER_SPECIFIED),

			screen_state_p->video_memory_size / 1024,

			screen_state_p->linear_aperture_location / (1024 * 1024),
			(s364_options_p->framebuffer_physical_address == 
				S364_OPTIONS_FRAMEBUFFER_PHYSICAL_ADDRESS_DEFAULT?
				S364_VERBOSE_STARTUP_BUILTIN_DEFAULT:
				S364_VERBOSE_STARTUP_USER_SPECIFIED));


		(void) fprintf(stderr,
			S364_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE,
			screen_state_p->generic_state.screen_physical_width,
			screen_state_p->generic_state.screen_physical_height,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height,
			screen_state_p->generic_state.screen_displayed_width,
			screen_state_p->generic_state.screen_displayed_height,
			screen_state_p->generic_state.screen_depth);

		(void) fprintf(stderr,
			S364_VERBOSE_STARTUP_DEFAULT_TIMEOUTS_MESSAGE,
			s364_graphics_engine_micro_delay_count,
			s364_graphics_engine_loop_timeout_count,
			s364_crtc_sync_loop_timeout_count);

		(void) fprintf(stderr, S364_VERBOSE_STARTUP_EPILOGUE);
	}

	config_p->IdentString = S364_DEFAULT_IDENT_STRING;
	return (initialization_status);
}

/*
 *	Function : s364_print_initialization_failure_message
 *
 * PURPOSE
 *
 *	Print the cause for initialization failure from the initialization 
 *  status flag.  Called by the vendor level DM_Initfunction on failure
 *  of the s364_initialize_display_library routine.
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_print_initialization_failure_message(
    const int status,
	const SIScreenRec *si_screen_p)
{
	struct s364_screen_state				*screen_state_p;
	struct s364_display_mode_table_entry 	*entry_p; 
	int										count;

	screen_state_p = (struct s364_screen_state *)si_screen_p->vendorPriv;

	if (status & S364_INITIALIZE_CHIPSET_DETECTION_FAILED)
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_CHIPSET_DETECTION_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_BUS_KIND_DETECTION_FAILED)
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_BUS_KIND_DETECTION_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_DETECT_MEMORY_SIZE_FAILED)
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_VIDEO_MEMORY_SIZE_DETECTION_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED)
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_DAC_KIND_DETECTION_FAILED )
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_DAC_KIND_DETECTION_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED )
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_MMAP_FAILED)
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_MMAP_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_KD_MODE_SWITCH_FAILED )
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_KD_MODE_SWITCH_FAILED_MESSAGE);
	}

	if (status & S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL )
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL_MESSAGE);
	}

	if (status & S364_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC)
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC_MESSAGE,
			 screen_state_p->display_mode_p->mode_description,
			 screen_state_p->dac_state_p->dac_name);
	}

	if (status & S364_INITIALIZE_PARSE_USER_OPTIONS_FAILED)
	{
		(void)fprintf(stderr,
			 S364_INITIALIZE_PARSE_USER_OPTIONS_FAILED_MESSAGE);
	}


	if (status & S364_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND)
	{
		int clock_frequency = screen_state_p->clock_frequency;

		(void)fprintf(stderr,
			 S364_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND_MESSAGE,
			si_screen_p->cfgPtr->disp_w, si_screen_p->cfgPtr->disp_h,
			si_screen_p->cfgPtr->depth, 
			(int)si_screen_p->cfgPtr->monitor_info.vfreq);

		/*
		 * List available modes.
		 */
		for(count = S364_MODE_NULL + 1,
			entry_p = &(s364_display_mode_table[S364_MODE_NULL + 1]);
			count < S364_MODE_COUNT; count ++, entry_p++)
		{
			/*
			 * Check if the clock/dac/memory combination can support this mode.
			 */
			if ((entry_p->clock_frequency <= 
					screen_state_p->dac_state_p->max_frequency) &&
				((si_screen_p->cfgPtr->virt_w * si_screen_p->cfgPtr->virt_h * 
					si_screen_p->cfgPtr->depth) <=
					(screen_state_p->video_memory_size * 8)))
			{
				screen_state_p->clock_frequency = entry_p->clock_frequency;

	 			if (!screen_state_p->clock_chip_p->check_support_for_frequency(
					screen_state_p->clock_frequency))
				{
					(void) fprintf(stderr, "\t\t%s\n",
						entry_p->mode_description);
				}
			}
		}
		screen_state_p->clock_frequency = clock_frequency;
	}

	if (status & S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED)
	{
		(void)fprintf(stderr,
			S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED_MESSAGE,
			screen_state_p->clock_frequency/1000,
			screen_state_p->clock_frequency%1000);
	}

	if (status & S364_INITIALIZE_MEMORY_CLOCK_FREQUENCY_UNSUPPORTED)
	{
		(void)fprintf(stderr,
			S364_INITIALIZE_MEMORY_CLOCK_FREQUENCY_UNSUPPORTED_MESSAGE);
	}

	if (status & S364_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE)
	{
		(void)fprintf(stderr,
			S364_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE_MESSAGE,
			screen_state_p->video_memory_size / 1024,
			si_screen_p->cfgPtr->disp_w, 
			si_screen_p->cfgPtr->disp_h,
			si_screen_p->cfgPtr->depth);
	}

	if (status & S364_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC)
	{
		(void)fprintf(stderr,
			S364_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC_MESSAGE,
			screen_state_p->clock_frequency/1000,
			screen_state_p->clock_frequency%1000,
			screen_state_p->dac_state_p->max_frequency/1000,
			screen_state_p->dac_state_p->max_frequency%1000,
			screen_state_p->dac_state_p->dac_name);
	}

	return;
}
