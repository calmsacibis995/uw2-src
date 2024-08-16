/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_sline.c	1.2"

/***
 ***	NAME
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
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
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

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
export boolean s364_scanline_debug = 0;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/
#include "g_state.h"
#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_gs.h"
#include "s364_state.h"
#include "s364_asm.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/
/*
 * PURPOSE
 * 	This is a mandatory SI entry point. The scanline requested is
 * copied from the framebuffer and given out.
 *
 * RETURN VALUE
 *
 *	The scanline contents.
 */


STATIC SILine
s364_get_scanline(const SIint32 scanline_y)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	unsigned char *src_p, *dst_p;
	int	count;

#if (defined(__DEBUG__))
	if (s364_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_get_scanline) {y = %d}\n",
			scanline_y);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));

	if ( scanline_y < 0 || 
		scanline_y > (screen_state_p->generic_state.screen_virtual_height - 1))
	{
		return((SILine) screen_state_p->generic_state.screen_scanline_p);
	}

	S3_WAIT_FOR_GE_IDLE();
	S3_DISABLE_MMAP_REGISTERS();

	count = (screen_state_p->generic_state.screen_virtual_width * 
		 screen_state_p->generic_state.screen_depth) / 8;

	src_p = (unsigned char *) (framebuffer_p + ( scanline_y * 
		((screen_state_p->generic_state.screen_physical_width * 
		 screen_state_p->generic_state.screen_depth) / 8)));
	
	dst_p = (unsigned char *) screen_state_p->generic_state.screen_scanline_p;

	(void) memcpy(dst_p, src_p, (size_t)count);

	S3_ENABLE_MMAP_REGISTERS();
	return ((SILine) screen_state_p->generic_state.screen_scanline_p);

}

/*
 * PURPOSE
 * 	This is a mandatory SI entry point. The scanline_p contents is
 * copied into the framebuffer. The operation is done thru' Graphics
 * Engine. (pixtrans)
 *
 * RETURN VALUE
 *
 *	None.
 */
STATIC SIvoid
s364_set_scanline(const SIint32 scanline_y, const SILine scanline_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	unsigned char *src_p;

#if (defined(__DEBUG__))
	if (s364_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_set_scanline) {y = %d}\n",
			scanline_y);
	}
#endif
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));

	if (scanline_p == NULL || scanline_y < 0 || 
		scanline_y > (screen_state_p->generic_state.screen_virtual_height - 1))
	{
		return;
	}

	src_p = (unsigned char *)scanline_p;

    /*
     * Set the clipping rectangle to cover virtual screen, only if
     * required.
     */
    S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
	 * Copy the scanline using the pixtrans helper function.
	 * (x,y) = (0,scanline_y)
	 * (height,width) = (1, vitual width)
	 * depth = screen depth
	 * plan mask = all planes enabled
	 * rop = command copy through plane
	 * set an illegal value '-1' for the stipple type. 
	 *
	 * sourec step is set to zero : this is because only height is zero
	 * in case of a bug the same scan line will be repeated.
	 */

	(void) s364_asm_transfer_pixels_through_pixtrans_helper(
		(unsigned long *) src_p, 0, scanline_y, 0, 
		screen_state_p->generic_state.screen_virtual_width,
		1, screen_state_p->generic_state.screen_depth,
		(unsigned short) S3_MIX_FN_N, 
					(~0U >> (32U - screen_state_p->generic_state.
							screen_depth)),
		(unsigned int) S3_CMD_PX_MD_THRO_PLANE, -1);

	return;
}

#if 0
STATIC SIvoid
s364_set_scanline(const SIint32 scanline_y, const SILine scanline_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	unsigned char *src_p;
	unsigned char *dst_p;
	int count;

#if (defined(__DEBUG__))
	if (s364_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_set_scanline) {y = %d}\n",
			scanline_y);
	}
#endif
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));

	if (scanline_p == NULL || scanline_y < 0 || 
		scanline_y > (screen_state_p->generic_state.screen_virtual_height - 1))
	{
		return;
	}

	src_p = (unsigned char *)scanline_p;

	S3_WAIT_FOR_GE_IDLE();
	S3_DISABLE_MMAP_REGISTERS();

	count = (screen_state_p->generic_state.screen_virtual_width * 
		 screen_state_p->generic_state.screen_depth) / 8;

	dst_p = (unsigned char *) (framebuffer_p + ( scanline_y * 
		((screen_state_p->generic_state.screen_physical_width * 
		 screen_state_p->generic_state.screen_depth) / 8)));

	(void) memcpy(dst_p, src_p, (size_t)count);

	S3_ENABLE_MMAP_REGISTERS();

	return;
}
#endif

/*
 * s364_free_scanline
 *
 * PURPOSE
 *
 * we don't actually free the scanline at any point as this was
 * allocated 
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC SIvoid
s364_free_scanline(void)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();

#if (defined(__DEBUG__))
	if (s364_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_free_scanline) {}\n");
	}
#endif
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));

	S3_ENABLE_MMAP_REGISTERS();

	return;
}

/*
 * s364_scanline__initialize__
 *
 * PURPOSE
 *
 * Initializing the scanline module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_scanline__initialize__(SIScreenRec *si_screen_p,
							struct s364_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIConfigP config_p = si_screen_p->cfgPtr;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
#if (defined(__DEBUG__))
	if (s364_scanline_debug)
	{
		(void) fprintf(debug_stream_p, 
		"(s364_scanline__initialize__) {\n"
		"\tfunctions_p = %p\n"
		"\tflags_p = %p\n"
		"\tconfig_p = %p\n"
		"\toptions_p = %p\n"
		"}\n",
		(void *) functions_p, (void *) flags_p, 
		(void *) config_p, (void *) options_p);
	}
#endif

	functions_p->si_getsl = s364_get_scanline;
	functions_p->si_setsl = s364_set_scanline;
	functions_p->si_freesl = s364_free_scanline;
}
