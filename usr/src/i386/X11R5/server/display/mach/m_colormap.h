/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_colormap.h	1.2"

#if (! defined(__M_COLORMAP_INCLUDED__))

#define __M_COLORMAP_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "m_opt.h"
#include "m_state.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

#define DEFINE_DAC_MODES()\
	DEFINE_DAC_MODE(MACH_DAC_24_BIT_R_G_B_A_SUPPORTED,0,"24 bit RGBa mode"),\
	DEFINE_DAC_MODE(MACH_DAC_24_BIT_A_B_G_R_SUPPORTED,1,"24 bit aBGR mode"),\
	DEFINE_DAC_MODE(MACH_DAC_24_BIT_R_G_B_SUPPORTED,2,"24 bit RGB mode"),\
	DEFINE_DAC_MODE(MACH_DAC_24_BIT_B_G_R_SUPPORTED,3,"24 bit BGR mode"),\
	DEFINE_DAC_MODE(MACH_DAC_16_BIT_5_5_5_SUPPORTED,4,"16 bit 555 mode"),\
	DEFINE_DAC_MODE(MACH_DAC_16_BIT_5_6_5_SUPPORTED,5,"16 bit 565 mode"),\
	DEFINE_DAC_MODE(MACH_DAC_16_BIT_6_5_5_SUPPORTED,6,"16 bit 655 mode"),\
	DEFINE_DAC_MODE(MACH_DAC_16_BIT_6_6_4_SUPPORTED,7,"16 bit 664 mode"),\
	DEFINE_DAC_MODE(MACH_DAC_16_BIT_6_4_4_SUPPORTED,8,"16 bit 644 mode"),\
	DEFINE_DAC_MODE(COUNT,9,"")

#define MACH_DAC_BITS_PER_RGB_6 				(0x1 << 0U)
#define MACH_DAC_BITS_PER_RGB_8					(0x1 << 1U)

enum mach_dac_mode
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	NAME = (0x0001 << SHIFT)
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};

enum mach_visual_kind 
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	MACH_VISUAL_##NAME
#include "m_visual.def"
#undef DEFINE_VISUAL
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
extern boolean mach_colormap_debug ;
#endif

/*
 *	Current module state.
 */

extern boolean
mach_dac_check_display_mode_feasibility(struct mach_screen_state
										*screen_state_p)
;

extern void
mach_colormap__initialize__(SIScreenRec *si_screen_p,
							struct mach_options_structure *options_p)
;


#endif
