/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_sline.c	1.3"

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
#include "s3_globals.h"
#include "s3_options.h"
#include "s3_state.h"
#include "s3_gs.h"

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
export boolean s3_scanline_debug = FALSE;
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

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SILine
s3_get_scanline(const SIint32 scanline_y)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_current_screen_state_p;
	int	numwords_to_read;

	ASSERT(generic_current_screen_state_p);
	ASSERT(generic_current_screen_state_p->screen_scanline_p);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	/*
	 * Check if a clipping rectangle reprogramming is required. If yes, 
	 * set it such that drawing is allowed for the entire screen.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 
			0,
			0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * The Rop has to be GXCopy.
	 */
	S3_STATE_SET_FG_ROP(screen_state_p,S3_MIX_FN_N);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_VIDEO_DATA);

	/*
	 * The read mask is all ones.
	 */
	S3_WAIT_FOR_FIFO(1);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
		(1 << screen_state_p->generic_state.screen_depth) -1);
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_RD_MASK);

	/*
	 * Select Foreground Mix. Image transfer as against to stippling.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	
	/*
	 * Blit a rectangle of height from display memory to the cpu memory.
	 */
	S3_WAIT_FOR_FIFO(5);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,0);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,scanline_y);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX );
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		screen_state_p->generic_state.screen_virtual_width - 1);

	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_READ | 
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);

	/*
	 * Compute the number of words to write.
	 */
	numwords_to_read = ( screen_state_p->generic_state.screen_virtual_width << 
		screen_state_p->generic_state.screen_depth_shift ) >>
		screen_state_p->pixtrans_width_shift;

#if (defined(__DEBUG__))
	if(s3_scanline_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_get_scanline) {\n"
			"\tscanline_y = %ld\n"
			"\tcmd = 0x%x\n"
			"\tnumwords = %d\n"
			"\t}\n",
			scanline_y, 
			(unsigned) screen_state_p->cmd_flags | 
			S3_CMD_TYPE_RECTFILL |
			S3_CMD_READ | 
			S3_CMD_USE_PIXTRANS | 
			S3_CMD_DRAW |
			S3_CMD_DIR_TYPE_AXIAL | 
			S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
			S3_CMD_AXIAL_Y_TOP_TO_BOTTOM,
			numwords_to_read
			);
	}
#endif

	/*
	 * Wait for the graphics engine to start pumping data. Then start
	 * reading the computed number of words.
	 */
	S3_WAIT_FOR_READ_DATA_IN_PIXTRANS_REGISTER();
#ifdef DELETE
	while (numwords_to_read--)
	{
		*bits_p++ =inw(S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS);
	}
#endif

	(*screen_state_p->screen_read_pixels_p)
		(screen_state_p->pixtrans_register,
		 numwords_to_read,
		 screen_state_p->generic_state.screen_scanline_p);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (generic_current_screen_state_p->screen_scanline_p);
}


STATIC SIvoid
s3_set_scanline(const SIint32 scanline_y, const SILine scanline_p)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_current_screen_state_p;
	int	numwords_to_write;
#ifdef DELETE
	unsigned short *bits_p = (unsigned short *)scanline_p;
#endif
	
	ASSERT(generic_current_screen_state_p);
	ASSERT(generic_current_screen_state_p->screen_scanline_p);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));


	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	/*
	 * Check if a clipping rectangle reprogramming is required. If yes, 
	 * set it such that drawing is allowed for the entire screen.
	 */

	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
	  
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 
			0,
			0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * The Rop has to be GXCopy.
	 */
	S3_STATE_SET_FG_ROP(screen_state_p,S3_MIX_FN_N);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);

	S3_WAIT_FOR_FIFO(1);
	/*
	 * The write mask is all ones.
	 */
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
		(1 << screen_state_p->generic_state.screen_depth) -1);
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_WRT_MASK);

	/*
	 * Select Foreground Mix. Image transfer as against to stippling.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);

	/*
	 * Blit a rectangle of height from cpu to the display memory.
	 */
	S3_WAIT_FOR_FIFO(5);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,0);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,scanline_y);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX );
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		screen_state_p->generic_state.screen_virtual_width - 1);

	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);

	/*
	 * Compute the number of words to write.
	 */
	numwords_to_write = ( screen_state_p->generic_state.screen_virtual_width << 
		screen_state_p->generic_state.screen_depth_shift ) >>
		screen_state_p->pixtrans_width_shift;

#if (defined(__DEBUG__))
	if(s3_scanline_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_set_scanline) {\n"
			"\tscanline_y = %ld\n"
			"\tcmd = 0x%x\n"
			"\tnumwords = %d\n"
			"\t}\n",
			scanline_y, 
			(unsigned) screen_state_p->cmd_flags | 
			S3_CMD_TYPE_RECTFILL |
			S3_CMD_WRITE | 
			S3_CMD_USE_PIXTRANS | 
			S3_CMD_DRAW |
			S3_CMD_DIR_TYPE_AXIAL | 
			S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
			S3_CMD_AXIAL_Y_TOP_TO_BOTTOM,
			numwords_to_write
			);
	}
#endif

#ifdef DELETE
	while (numwords_to_write--)
	{
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS, 
			*bits_p);
		bits_p++;
	}
#endif

	S3_WAIT_FOR_FIFO(8);

	(*screen_state_p->screen_write_pixels_p)
		(screen_state_p->pixtrans_register,
		 numwords_to_write,
		 (void *)scanline_p);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return;
}


/*
 * s3_free_scanline
 *
 * we don't actually free the scanline at any point as this was
 * allocated 
 */
STATIC SIvoid
s3_free_scanline(void)
{
#if (defined(__DEBUG__))
	if(s3_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "(s3_free_scanline) {}\n");
	}
#endif

	return;
}

function void
s3_scanline__initialize__(SIScreenRec *si_screen_p,
							struct s3_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIConfigP config_p = si_screen_p->cfgPtr;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
#if (defined(__DEBUG__))
	if(s3_scanline_debug)
	{
		(void) fprintf(debug_stream_p, 
		"(s3_scanline__initialize__) {\n"
		"\tfunctions_p = %p\n"
		"\tflags_p = %p\n"
		"\tconfig_p = %p\n"
		"\toptions_p = %p\n"
		"}\n",
		(void *) functions_p, (void *) flags_p, 
		(void *) config_p, (void *) options_p);
	}
#endif

	functions_p->si_getsl = s3_get_scanline;
	functions_p->si_setsl = s3_set_scanline;
	functions_p->si_freesl = s3_free_scanline;
}
