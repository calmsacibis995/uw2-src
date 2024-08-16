/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/generic.c	1.1"
/***
 ***	NAME
 ***
 ***		generic.c : initialization for the generic module.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "generic.h"
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
 ***	SEE ALSO
 ***
 ***		g_gs.c : graphics state handling.
 ***		g_colormap.c : colormap and visual structures.
 ***		g_regs.c : register access methods.
 ***
 ***	CAVEATS
 ***
 ***	BUGS
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
#include "g_state.h"
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
export enum debug_level generic_debug = DEBUG_LEVEL_NONE;
#endif

/*
 *	Current module state.
 */

#include "stdenv.h"

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void generic__initialize__(SIScreenRec *si_screen_p);

/***
 ***	Includes.
 ***/
#include "g_colormap.h"
#include "g_gs.h"
#include "g_regs.h"

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
 * No Operation functions in three flavours.
 */

function SIBool
generic_no_operation_fail(void)
{

#if (defined(__DEBUG__))

	if (generic_debug)
	{
		(void) fprintf(debug_stream_p, 
"(generic_no_operation_fail)\n"
"{}\n");
	}

#endif /* __DEBUG__ */	

	return (SI_FAIL);
}

function SIBool
generic_no_operation_succeed(void)
{

#if (defined(__DEBUG__))

	if (generic_debug)
	{
		(void) fprintf(debug_stream_p, 
"(generic_no_operation_succeed)\n"
"{}\n");
	}

#endif /* __DEBUG__ */

	return (SI_SUCCEED);

}

function SIvoid
generic_no_operation_void(void)
{

#if (defined(__DEBUG__))

	if (generic_debug)
	{
		(void) fprintf(debug_stream_p, 
					   "(generic_no_operation_void)\n"
"{}\n");
	}

#endif /* __DEBUG__ */

	return;

}

/*
 * Initialize the display library.
 */

function SIBool
generic_initialize_display_library(
	SIint32 virtual_terminal_file_descriptor,	/* FD associated with the VT */
    SIScreenRec *si_screen_p,
    struct generic_screen_state *screen_state_p)
{
	SIFlagsP flags_p;
	SIConfigP config_p;
	SIFunctionsP functions_p;
	int initialization_status = 0;
	
#if (defined(__DEBUG__))
	if (generic_debug)
	{
		(void) fprintf(debug_stream_p, 
"(generic_initialize_display_library)\n"
"{\n"\
"\tvirtual_terminal_file_descriptor = %ld\n"\
"\tsi_screen_p = %p\n"\
"\tscreen_state_p = %p\n"\
"}\n",
					   virtual_terminal_file_descriptor,
					   (void *) si_screen_p,
					   (void *) screen_state_p);
	}
#endif /* __DEBUG__ */	

	ASSERT(screen_state_p != NULL);
	
	/*
	 * Tuck away pointers into the screen private structure.
	 */

	flags_p = screen_state_p->screen_flags_p = si_screen_p->flagsPtr;
	config_p = screen_state_p->screen_config_p = si_screen_p->cfgPtr;
	functions_p = screen_state_p->screen_functions_p =
		si_screen_p->funcsPtr;
	
	screen_state_p->screen_server_version_number =
		flags_p->SIserver_version;

	/*
	 * Warn the user if the X server revision is not quite expected.
	 * reset the SDD version to match the X servers expectations.
	 */

	if (flags_p->SIserver_version < X_SI_VERSION1_1)
	{
		(void) fprintf(stderr,
					   GENERIC_MESSAGE_ENABLING_COMPATIBILITY_MODE);

		/*
		 * A consequence of the X server having an earlier revision
		 * number is that for line draw and rect fill the parameters
		 * passed down from the server will be different.
		 */

		screen_state_p->screen_sdd_version_number =
			DM_SI_VERSION_1_0;
	}
	else
	{
		screen_state_p->screen_sdd_version_number =
			DM_SI_VERSION_1_1;
	}
	
	/*
	 * Set up pointers to functions, configurations etc.
	 */

	flags_p->SIdm_version = 
		screen_state_p->screen_sdd_version_number;
	
	flags_p->SIlinelen = config_p->virt_w;

	flags_p->SIlinecnt = config_p->virt_h;

	flags_p->SIxppin =  (config_p->virt_w /
						 config_p->monitor_info.width); 

	flags_p->SIyppin = (config_p->virt_h /
						config_p->monitor_info.height);

	/*
	 * Fill in the mandatory functions.  
	 * Proper initialization will be done by the chipset specific
	 * library. 
	 */

	functions_p->si_init = 
		(SIBool (*)(int, SIConfigP, SIInfoP, SIFunctions **))
			generic_no_operation_succeed;

	functions_p->si_restore = 
		(SIBool (*)(SIvoid)) generic_no_operation_succeed;
	functions_p->si_vt_save = 
		(SIBool (*)(SIvoid)) generic_no_operation_succeed;
	functions_p->si_vt_restore = 
		(SIBool (*)(SIvoid)) generic_no_operation_succeed;
	functions_p->si_vb_onoff = 
		(SIBool (*)(SIBool)) generic_no_operation_succeed;

	/*
	 * We don't support request caching at the generic level.
	 */

	functions_p->si_initcache = 
		(SIBool (*)(SIvoid)) generic_no_operation_fail;
	functions_p->si_flushcache = 
		(SIBool (*)(SIvoid)) generic_no_operation_fail;

	/* GS initialization is in "g_gs.c" */

	/*
	 * Screen changes handled by the lower layers.
	 */

	functions_p->si_screen =  
		(SIBool (*)(SIint32, SIint32)) generic_no_operation_succeed;

	/*
	 * Scanline handling.
	 */

	functions_p->si_getsl = (SILine (*)(SIint32))
		generic_no_operation_succeed; 
	functions_p->si_setsl = (SIvoid (*)(SIint32, SILine))
		generic_no_operation_succeed; 
	functions_p->si_freesl = (SIvoid (*)(SIvoid))
		generic_no_operation_succeed;

	/*
	 * Colormap handling
	 */

	functions_p->si_set_colormap = 
		(SIBool (*)(SIint32, SIint32, SIColor *, SIint32)) 
		generic_no_operation_succeed;
	functions_p->si_get_colormap =  
		(SIBool (*)(SIint32, SIint32, SIColor *, SIint32))
		generic_no_operation_succeed;

	/*
	 * Cursor handling.
	 */

	flags_p->SIcursortype = CURSOR_TRUEHDWR;
	flags_p->SIcurscnt = 0;
	flags_p->SIcurswidth = 0;
	flags_p->SIcursheight = 0;
	flags_p->SIcursmask = 0;
	functions_p->si_hcurs_download = 
		(SIBool (*)(SIint32, SICursorP)) generic_no_operation_fail;
	functions_p->si_hcurs_turnon = 
		(SIBool (*)(SIint32)) generic_no_operation_succeed;
	functions_p->si_hcurs_turnoff = 
		(SIBool (*)(SIint32)) generic_no_operation_succeed;
	functions_p->si_hcurs_move = 
		(SIBool (*)(SIint32, SIint32, SIint32)) generic_no_operation_succeed;

	/*
	 * Spans.
	 */

	flags_p->SIavail_spans = 0;
	functions_p->si_fillspans = 
		(SIBool (*)(SIint32, SIPointP, SIint32 *)) generic_no_operation_fail;

	/*
	 * Bitblt functions.
	 */

	flags_p->SIavail_bitblt = 0;
	flags_p->SIavail_stplblt = 0;
	functions_p->si_ss_bitblt = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIint32, SIint32,
					SIint32)) generic_no_operation_fail;
	functions_p->si_ms_bitblt = 
		(SIBool (*)(SIbitmapP, SIint32, SIint32, SIint32, SIint32,
					SIint32, SIint32)) generic_no_operation_fail;
	functions_p->si_sm_bitblt = 
		(SIBool (*)(SIbitmapP, SIint32, SIint32, SIint32, SIint32,
					SIint32, SIint32)) generic_no_operation_fail;
	functions_p->si_ss_stplblt = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIint32, SIint32,
					SIint32, SIint32, SIint32)) generic_no_operation_fail;
	functions_p->si_ms_stplblt = 
		(SIBool (*)(SIbitmapP, SIint32, SIint32, SIint32, SIint32,
					SIint32, SIint32, SIint32, SIint32)) 
			generic_no_operation_fail;
	functions_p->si_sm_stplblt = 
		(SIBool (*)(SIbitmapP, SIint32, SIint32, SIint32, SIint32,
					SIint32, SIint32, SIint32, SIint32)) 
			generic_no_operation_fail;
	
	/*
	 * Filled polygons, tiling and stippling.
	 */

	flags_p->SIavail_fpoly = 0;
	flags_p->SItilewidth = 0;
	flags_p->SItileheight = 0;
	flags_p->SIstipplewidth = 0;
	flags_p->SIstippleheight = 0;
	functions_p->si_poly_clip = 
		(SIvoid (*)(SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_void;
	functions_p->si_poly_fconvex = 
		(SIBool (*)(SIint32, SIPointP)) generic_no_operation_fail;
	functions_p->si_poly_fgeneral = 
		(SIBool (*)(SIint32, SIPointP)) generic_no_operation_fail;
	functions_p->si_poly_fillrect = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIRectOutlineP)) 
		generic_no_operation_fail;

	/*
	 * Points.
	 */

	flags_p->SIavail_point = 0;
	functions_p->si_plot_points = 
		(SIBool (*)(SIint32, SIPointP)) generic_no_operation_fail;

	/*
	 * Line drawing.
	 */

	flags_p->SIavail_line = 0;
	functions_p->si_line_clip = 
		(SIvoid (*)(SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_void;
	functions_p->si_line_onebitline = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIPointP, SIint32, SIint32))
		generic_no_operation_fail;
	functions_p->si_line_onebitseg = 
		(SIBool (*)(SIint32, SIint32, SIint32, SISegmentP, SIint32)) 
		generic_no_operation_fail;
	functions_p->si_line_onebitrect = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_fail;

	/*
	 * Arc drawing.
	 */

	flags_p->SIavail_drawarc = 0;
	functions_p->si_drawarc_clip = 
		(SIvoid (*)(SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_void;
	functions_p->si_drawarc = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_fail;

	flags_p->SIavail_fillarc = 0;
	functions_p->si_fillarc_clip = 
		(SIvoid (*)(SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_void;
	functions_p->si_fillarc = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_fail;

	/*
	 * Fonts.
	 */

	flags_p->SIfontcnt = 0;
	flags_p->SIavail_font = 0;
	functions_p->si_font_check = 
		(SIBool (*)(SIint32, SIFontInfoP)) generic_no_operation_fail;
	functions_p->si_font_download = 
		(SIBool (*)(SIint32, SIFontInfoP, SIGlyphP)) 
		generic_no_operation_fail;
	functions_p->si_font_free = 
		(SIBool (*)(SIint32)) generic_no_operation_succeed;
	functions_p->si_font_clip = 
		(SIvoid (*)(SIint32, SIint32, SIint32, SIint32)) 
		generic_no_operation_void;
	functions_p->si_font_stplblt = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIint32, SIint16 *, SIint32))
		generic_no_operation_fail;

	/*
	 * Caching.
	 */

	flags_p->SIavail_memcache = 0;
	functions_p->si_cache_alloc = 
		(SIBool (*)(SIbitmapP, SIint32)) generic_no_operation_fail;
	functions_p->si_cache_free = 
		(SIBool (*)(SIbitmapP)) generic_no_operation_succeed;
	functions_p->si_cache_lock = 
		(SIBool (*)(SIbitmapP)) generic_no_operation_succeed;
	functions_p->si_cache_unlock = 
		(SIBool (*)(SIbitmapP)) generic_no_operation_succeed;

	/*
	 * Visual information will be determined by the board specific
	 * layer.
	 */

	flags_p->SIvisuals = (SIVisualP) 0;
	flags_p->SIvisualCNT = 0;

#ifdef si_exten_init
#undef si_exten_init
#endif

	/*
	 * No extension hooks.
	 */

	flags_p->SIavail_exten = 0;
	functions_p->si_exten_init = 
		(SIBool (*)(SIvoid)) generic_no_operation_fail;

	/*
	 * Set the keyboard hook only if the server revision
	 * is after version 1.1.
	 */

	if (screen_state_p->screen_server_version_number >=
		X_SI_VERSION1_1)
	{
		
		/*
		 * No hook for the keyboard.
		 */
		flags_p->SIkeybd_event = 0;
		functions_p->si_proc_keybdevent = 
			(SIBool (*)(SIint32)) generic_no_operation_fail;
		
		/*
		 * similarly for the LFB code hooks.
		 */

		flags_p->SIfb_pbits = SI_FB_NULL; 
		flags_p->SIfb_width = 0;
   	}
	
	/*
	 * Graphics state handling is done in `generic_gs.c'.
	 * Screen state initialization is done in `generic_state.c'.
	 * Colormap initialization is done in `generic_colormap.c'.
	 * These are automagically invoked by this munch generated
	 * function. 
	 */

	generic__initialize__(si_screen_p);
	
	return (initialization_status);
}

				
