/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_cmap.h	1.3"

#if (! defined(__S3_CMAP_INCLUDED__))

#define __S3_CMAP_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/
#include "s3_options.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/
#define DEFINE_DAC_MODES()\
	DEFINE_DAC_MODE(s3_DAC_24_BIT_R_G_B_A_SUPPORTED,0,"24 bit RGBa mode"),\
	DEFINE_DAC_MODE(S3_DAC_24_BIT_A_B_G_R_SUPPORTED,1,"24 bit aBGR mode"),\
	DEFINE_DAC_MODE(S3_DAC_24_BIT_R_G_B_SUPPORTED,2,"24 bit RGB mode"),\
	DEFINE_DAC_MODE(S3_DAC_24_BIT_B_G_R_SUPPORTED,3,"24 bit BGR mode"),\
	DEFINE_DAC_MODE(S3_DAC_16_BIT_5_5_5_SUPPORTED,4,"16 bit 555 mode"),\
	DEFINE_DAC_MODE(S3_DAC_16_BIT_5_6_5_SUPPORTED,5,"16 bit 565 mode"),\
	DEFINE_DAC_MODE(S3_DAC_16_BIT_6_5_5_SUPPORTED,6,"16 bit 655 mode"),\
	DEFINE_DAC_MODE(S3_DAC_16_BIT_6_6_4_SUPPORTED,7,"16 bit 664 mode"),\
	DEFINE_DAC_MODE(S3_DAC_16_BIT_6_4_4_SUPPORTED,8,"16 bit 644 mode"),\
	DEFINE_DAC_MODE(COUNT,9,"")

#define S3_DAC_BITS_PER_RGB_6 				(0x1 << 0U)
#define S3_DAC_BITS_PER_RGB_8				(0x1 << 1U)

/***
 *** Types.
 ***/

enum s3_dac_mode
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	NAME = (0x0001 << SHIFT)
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};

enum s3_visual_kind 
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	S3_VISUAL_##NAME
#include "s3_vis.def"
#undef DEFINE_VISUAL
};

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/
extern int s3_dac_kind_to_max_clock_table[] ;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean s3_colormap_debug ;
#endif

/*
 *	Current module state.
 */

extern boolean
s3_dac_check_display_mode_feasibility(struct s3_screen_state
										*screen_state_p)
;

extern void
s3_colormap__initialize__(SIScreenRec *si_screen_p,
							struct s3_options_structure *options_p)
;


#endif
