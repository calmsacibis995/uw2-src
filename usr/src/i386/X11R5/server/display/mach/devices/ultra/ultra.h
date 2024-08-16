/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/ultra.h	1.5"
#if (! defined(__ULTRA_INCLUDED__))

#define __ULTRA_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"
#include "mach.h"
#include "l_opt.h"

/***
 ***	Constants.
 ***/

#if (defined(__DEBUG__))
#define LFB_SCREEN_STATE_STAMP\
	(('L' << 0) + ('F' << 1) + ('B' << 2) + ('S' << 3) +\
	 ('C' << 4) + ('R' << 5) + ('E' << 6) + ('E' << 7) +\
	 ('N' << 8) + ('S' << 9) + ('T' << 10) + ('A' << 11) +\
	 ('T' << 12))
#endif

/***
 ***	Macros.
 ***/

#define LFB_CURRENT_SCREEN_STATE_DECLARE()\
	struct lfb_screen_state *const screen_state_p = \
	(struct lfb_screen_state *) generic_current_screen_state_p

#define LFB_ALL_PLANES_ENABLED_MASK(screen_state_p)\
	((1 << (screen_state_p)->mach_state.generic_state.screen_depth) - 1) 

/***
 ***	Types.
 ***/

/*
 * The Linear Frame Buffer Module state structure
 */

struct lfb_screen_state 
{
	/*
	 * Chipset level module state 
	 */

	struct	mach_screen_state mach_state;
	
	boolean	is_lfb_enabled;

	/*
	 * Pointers to lfb modules state
	 */

	void *lfb_graphics_state_p;
	void *lfb_points_state_p;
	void *arc_state_p;

	/*
	 * lfb specific user options
	 */

	struct lfb_options_structure *options_p ;
		
	/*
	 * Pointer to the ATI frame buffer: 
	 */

	void *frame_buffer_p;

	/*
	 * frame buffer location and length measured in bytes
	 */

	unsigned int frame_buffer_address;
	unsigned int frame_buffer_length;
	
	/*
	 * The file descriptor for use with the MMAP call.
	 */

	int mmap_fd;
	
	/*
	 * Length of one screen line in bytes
	 */

	unsigned int frame_buffer_stride;

	/*
	 * This can be used if the frame buffer
	 * stride is a power of 2
	 */
	int is_frame_buffer_stride_a_power_of_two;
	
	unsigned int frame_buffer_stride_shift;
	unsigned int pixels_per_word_shift;

	/*
	 * The memory aperture interface on ATI is controlled by
	 * the register MEM_CFG.  This register needs to be
	 * saved and restored as and when we VT switch in / out.
	 */

	unsigned short saved_mem_cfg;

	/*APR20
	 *Pointer  to chipset layer
	 *si_restore function
	 */
	SIBool (*screen_restore_function_p)();

	/* JUN 16
	 * Pointer to chipset layer si_vt_save and si_vt_restore
	 * functions.
	 */
	SIBool (*virtual_terminal_save_function_p)();

	SIBool (*virtual_terminal_restore_function_p)();

#if defined(__DEBUG__)
	int stamp;
#endif

}; 

/*
 *	Forward declarations.
 */

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean lfb_debug ;
#endif

/*
 *	Current module state.
 */

/*
 * global pointer to lfb state
 */
extern struct lfb_state * lfb_current_state_p;

extern SIBool
DM_InitFunction(SIint32 virtual_terminal_file_descriptor,
				SIScreenRec *si_screen_p)
;


#endif
