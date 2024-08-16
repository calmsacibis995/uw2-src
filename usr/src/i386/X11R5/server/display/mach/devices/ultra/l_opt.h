/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l_opt.h	1.2"

#if (! defined(__L_OPT_INCLUDED__))

#define __L_OPT_INCLUDED__



#include "stdenv.h"
#include "global.h"


enum lfb_options_use_linear_frame_buffer
{
	LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_YES,
	LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_NO,
	LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_AUTO_CONFIGURE,
	lfb_options_use_linear_frame_buffer_end_enumeration
};

enum lfb_options_linear_frame_buffer_page_size
{
	LFB_OPTIONS_LINEAR_FRAME_BUFFER_PAGE_SIZE_AUTO_CONFIGURE,
	LFB_OPTIONS_LINEAR_FRAME_BUFFER_PAGE_SIZE_1,
	LFB_OPTIONS_LINEAR_FRAME_BUFFER_PAGE_SIZE_4,
	lfb_options_linear_frame_buffer_page_size_end_enumeration
};


#define	LFB_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT	1
#define	LFB_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT	2


#define	LFB_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINTS	1


#define	LFB_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS	1




struct lfb_options_structure
{
	int arc_cache_size;
	unsigned int arcdraw_options;
	unsigned int bitblt_options;
	int frame_buffer_physical_address;
	int frame_buffer_size;
	enum lfb_options_linear_frame_buffer_page_size linear_frame_buffer_page_size;
	int number_of_graphics_states;
	unsigned int pointdraw_options;
	enum lfb_options_use_linear_frame_buffer use_linear_frame_buffer;

};

/*
 * Names of the option defaults
 */

#define LFB_OPTIONS_ARC_CACHE_SIZE_DEFAULT\
	16
#define LFB_OPTIONS_ARCDRAW_OPTIONS_DEFAULT\
	( LFB_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS )
#define LFB_OPTIONS_BITBLT_OPTIONS_DEFAULT\
	( LFB_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT |LFB_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT )
#define LFB_OPTIONS_FRAME_BUFFER_PHYSICAL_ADDRESS_DEFAULT\
	0
#define LFB_OPTIONS_FRAME_BUFFER_SIZE_DEFAULT\
	0
#define LFB_OPTIONS_LINEAR_FRAME_BUFFER_PAGE_SIZE_DEFAULT\
	LFB_OPTIONS_LINEAR_FRAME_BUFFER_PAGE_SIZE_AUTO_CONFIGURE
#define LFB_OPTIONS_NUMBER_OF_GRAPHICS_STATES_DEFAULT\
	8
#define LFB_OPTIONS_POINTDRAW_OPTIONS_DEFAULT\
	( LFB_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINTS )
#define LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_DEFAULT\
	LFB_OPTIONS_USE_LINEAR_FRAME_BUFFER_AUTO_CONFIGURE


#if (defined(__DEBUG__))
extern boolean	lfb_options_debug ;
#endif


extern struct lfb_options_structure *
lfb_options_parse (struct lfb_options_structure *option_struct_p,
			   const char *option_string_p)
;


#endif
