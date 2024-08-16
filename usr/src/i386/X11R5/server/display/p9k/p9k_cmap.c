/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_cmap.c	1.2"
/***
 ***	NAME
 ***
 ***		p9k_cmap.c : colormap handling for the P9000 chipset.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k_cmap.h"
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
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***/

PUBLIC

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
 *** Types.
 ***/

enum p9000_visual_kind 
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	P9000_VISUAL_##NAME
#include "p9k_vis.def"
#undef DEFINE_VISUAL
};

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
export enum debug_level p9000_colormap_debug = 0;
#endif

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <string.h>
#include <ctype.h>
#include <memory.h>
#include "g_colormap.h"
#include "p9k_opt.h"
#include "p9k_state.h"
#include "p9k_dacs.h"
#include "p9k_gbls.h"

/***
 ***	Constants.
 ***/

STATIC const SIVisual 
p9000_visuals_table[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	{\
		 TYPE, DEPTH, N_COLORMAPS, SIZE, N_VALID_BITS,\
		 R_MASK, G_MASK, B_MASK, R_OFFSET, G_OFFSET, B_OFFSET\
	}
#include "p9k_vis.def"			
#undef DEFINE_VISUAL	
};

#if (defined(__DEBUG__))
STATIC const char *const p9000_visual_kind_to_visual_kind_dump[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
					  #NAME
#include "p9k_vis.def"			
#undef DEFINE_VISUAL	
};
#endif	

/***
 ***	Macros.
 ***/

/*
 * Determining if a visual supports programmable colormaps
 */

#define P9000_IS_PROGRAMMABLE_VISUAL(visual_p)\
	((visual_p)->si_visual_p->SVtype & 0x1)

/***
 *** 	Types.
 ***/

/***
 ***	Variables.
 ***/

/***
 ***	Functions.
 ***/

/*
 * Setting a colormaps entries. Perform a check for the modifiability
 * of the colormap.
 */

STATIC SIBool
p9000_colormap_set_colormap_pseudocolor(SIint32 visual_number, 
	SIint32 colormap_number, SIColor *colors_p, SIint32 count)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct generic_visual *visual_p = &(screen_state_p->generic_state.
		  screen_visuals_list_p[visual_number]);
	SIVisualP si_visual_p = visual_p->si_visual_p;
	struct generic_colormap *colormap_p = NULL;
	unsigned short *rgb_p;
	unsigned int rgb_shift_count;
	void (*dac_set_color_p)(const int index, unsigned short *rgb_p);
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL, visual_p));
	ASSERT(si_visual_p);
	ASSERT(colormap_number >=0 && colormap_number < si_visual_p->SVcmapcnt);

	if (count <= 0)
	{
		return SI_SUCCEED;
	}
	
	colormap_p = &(screen_state_p->generic_state.
		  screen_colormaps_pp[visual_number][colormap_number]);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP,colormap_p));
	
	rgb_p = colormap_p->rgb_values_p;

	dac_set_color_p = visual_p->set_color_method_p;
	
	ASSERT(rgb_p != NULL);
	ASSERT(dac_set_color_p != NULL);
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_colormap,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_dac_set_colormap_pseudocolor) {\n"
			"\tvisual_number = %ld\n"
			"\tvisual_p = %p\n"
			"\tcolormap_number = %ld\n"
			"\tcolormap_p = %p\n"
			"\trgb_p = %p\n"					  
			"\tdac_set_color_p = %p\n"
			"\tcolors_p = %p\n"
			"\tcount = %ld\n"
			"}\n",
			visual_number, 
			(void *) visual_p,
			colormap_number,
			(void *) colormap_p,
			(void *) rgb_p,
			(void *) dac_set_color_p,
			(void *) colors_p, 
			count);
	}
#endif

	/*
	 * Check if the visual is a programmable visual. In case of a 
	 * non programmable visual return.
	 */

	if (! P9000_IS_PROGRAMMABLE_VISUAL(visual_p))
	{
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_colormap,INTERNAL))
		{
			(void) fprintf(debug_stream_p,
				"(p9000_colormap_set_colormap_pseudocolor)\n"
				"{\n"
				"\t# Called for non programmable colormap\n"
				"}\n");
		}
#endif
		return (SI_SUCCEED);
	}

	/*
	 * Some dacs support 8 bits per rgb. Most of them are 6bpp.
	 */

	rgb_shift_count = 16 - si_visual_p->SVbitsrgb;

	/*
	 * Loop over all colormap entries.
	 */

	while(count-- > 0)
	{

		if (colors_p->SCpindex >= 0 && 
			colors_p->SCpindex < si_visual_p->SVcmapsz)
		{
			
			/*
			 * Update internal copy of colormap.
			 */

			rgb_p = &(colormap_p->
					  rgb_values_p[3 * colors_p->SCpindex]);

			*rgb_p++ = (unsigned short) colors_p->SCred >>
				rgb_shift_count;

			*rgb_p++ = (unsigned short) colors_p->SCgreen >>
				rgb_shift_count;

			*rgb_p++ = (unsigned short) colors_p->SCblue >>
				rgb_shift_count;
			
			/*
			 * Program the dac.
			 */

			(*dac_set_color_p)
				(colors_p->SCpindex, 
				 &(colormap_p->rgb_values_p[3 * colors_p->SCpindex]));
		}

		colors_p++;
	}

	return (SI_SUCCEED);
}


STATIC SIBool
p9000_colormap_get_colormap_pseudocolor(SIint32 visual_number, 
				   SIint32 colormap_number,
				   SIColor *colors_p, SIint32 count)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct generic_visual *visual_p = &(screen_state_p->generic_state.
		  screen_visuals_list_p[visual_number]);
	SIVisualP si_visual_p = visual_p->si_visual_p;
	struct generic_colormap *colormap_p = NULL;
	unsigned short *rgb_p;
	unsigned int rgb_shift_count;
	void (*dac_get_color_p)(const int index, unsigned short *rgb_p);
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL,visual_p));
	ASSERT(colormap_number >=0 && colormap_number < si_visual_p->SVcmapcnt);

	if (count <= 0)
	{
		return SI_SUCCEED;
	}

	colormap_p = &(screen_state_p->generic_state.
				   screen_colormaps_pp[visual_number][colormap_number]);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));
	
	dac_get_color_p = visual_p->get_color_method_p;
	rgb_p = colormap_p->rgb_values_p;
	
	ASSERT(rgb_p != NULL);
	ASSERT(dac_get_color_p != NULL);
	

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_colormap,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_get_colormap_pseudocolor) {\n"
			"\tvisual_number = %ld\n"
			"\tcolormap_number = %ld\n"
			"\tcolormap_p = %p\n"
			"\trgb_p = %p\n"					  
			"\tdac_get_color_p = %p\n"
			"\tcolors_p = %p\n"
			"\tcount = %ld\n"
			"}\n",
			visual_number, colormap_number,
			(void *) colormap_p, (void *) rgb_p,
			(void *) dac_get_color_p,
			(void *) colors_p, count);
	}
#endif

	rgb_shift_count = 16 - si_visual_p->SVbitsrgb; 
	
	while(count --)
	{
		if (colors_p->SCpindex >= 0 && 
			colors_p->SCpindex < si_visual_p->SVcmapsz)
		{
			rgb_p = &(colormap_p->
					  rgb_values_p[3 * colors_p->SCpindex]);
			
			(*dac_get_color_p)(colors_p->SCpindex, rgb_p); 
			
			colors_p->SCred = (*rgb_p++) << rgb_shift_count;
			colors_p->SCgreen = (*rgb_p++) << rgb_shift_count;
			colors_p->SCblue = (*rgb_p++) << rgb_shift_count;

		}
		colors_p++;
		
	}
	return (SI_SUCCEED);
}

/*
 * Just assumes that the individual r/g/b dacs are serially programmed
 * from 1....max entry of that dac.
 */

STATIC SIBool
p9000_colormap_get_colormap_truecolor(SIint32 visual_number, 
				   SIint32 colormap_number,
				   SIColor *colors_p, SIint32 count)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct generic_visual *visual_p = &(screen_state_p->generic_state.
		  screen_visuals_list_p[visual_number]);

	SIVisualP si_visual_p = visual_p->si_visual_p;

	int number_of_reds = 
		(si_visual_p->SVredmask >> si_visual_p->SVredoffset) + 1;
	int number_of_greens = 
		(si_visual_p->SVgreenmask >> si_visual_p->SVgreenoffset) + 1;
	int number_of_blues =
		(si_visual_p->SVbluemask >> si_visual_p->SVblueoffset) + 1;
	
	ASSERT(number_of_reds > 0);
	ASSERT(number_of_blues > 0);
	ASSERT(number_of_greens > 0);
	
	if (count <= 0)
	{
		return SI_SUCCEED;
	}
	
	/*
	 * Fill in the appropriate R/G or B colormap.
	 */

	for(;count --; colors_p++)
	{

		ASSERT(colors_p->SCpindex >= 0);

		switch (colors_p->SCvalid)
		{
		case VALID_RED :
			colors_p->SCred = (colors_p->SCpindex << 16) / number_of_reds;
			break;
			
		case VALID_GREEN :
			colors_p->SCgreen = (colors_p->SCpindex << 16) / number_of_greens; 
			break;
			
		case VALID_BLUE :
			colors_p->SCblue = (colors_p->SCpindex << 16) / number_of_blues;
			break;
			
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
	}
	return (SI_SUCCEED);
}

STATIC SIBool
p9000_colormap_get_colormap_directcolor(SIint32 visual_number, 
	SIint32 colormap_number, SIColor *colors_p, SIint32 count)
{
	return (SI_SUCCEED);
}

STATIC SIBool
p9000_colormap_set_colormap_directcolor(SIint32 visual_number, 
	SIint32 colormap_number, SIColor *colors_p, SIint32 count)
{
	return (SI_SUCCEED);
}


/*
 * p9000_colormap__vt_switch_in__
 *
 * Called when the X server is going to switch into a virtual
 * terminal.  In this module we need to reprogram the xservers
 * colormaps.
 * CAVEAT: Visual number 0 is assumed to be default visual to be
 * restored and colormap number 0 is the default colormap to be
 * restored. This is because SI currently does not support multiple
 * visuals and colormaps.
 */

function void
p9000_colormap__vt_switch_in__(void)
{
	int						i;
	unsigned short 			*rgb_p;
	struct generic_visual 	*visual_p ; 
	SIVisualP 				si_visual_p;
	struct generic_colormap *colormap_p;
	void (*dac_set_color_p)(const int index, unsigned short *rgb_p);
	P9000_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_colormap,SCAFFOLDING))
	{
		(void)fprintf(debug_stream_p, "(p9000_cmap__vt_switch_in__){}\n");
	}
#endif

	visual_p = &(screen_state_p->generic_state.screen_visuals_list_p[0]);
	colormap_p = &(screen_state_p->generic_state.screen_colormaps_pp[0][0]);

	si_visual_p = visual_p->si_visual_p;
	dac_set_color_p = visual_p->set_color_method_p;
	rgb_p = colormap_p->rgb_values_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL, visual_p));
	ASSERT(si_visual_p);

	if (! P9000_IS_PROGRAMMABLE_VISUAL(visual_p))
	{
		/* 
		 * Nothing to program.
		 */

		return;
	}

	/*
	 * Check the visual type and do appropriate programming. 
	 * This will work only for pseudo color. 
	 * TODO: Directcolor visuals.
	 */

	for ( i = 0; i < si_visual_p->SVcmapsz; ++i)
	{
		(*dac_set_color_p) (i, rgb_p);
		rgb_p += 3;
	}

	return;

}

function void
p9000_colormap__initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
{ 
	int		index;
	SIVisualP	si_visual_p = 0;
	struct generic_colormap	*colormap_p;
	struct p9000_screen_state *screen_state_p =
		(struct p9000_screen_state *) si_screen_p->vendorPriv;
	P9000_DAC_FUNCTIONS_DECLARE();
	


	/*
	 * Attempt to read in the visuals.
	 * Make sure the board layer is not handling visuals
	 */

	if (!screen_state_p->generic_state.screen_visuals_list_p)
	{
		int visual_count = 0;
		enum p9000_visual_kind screen_default_visual = P9000_VISUAL_NULL;
		
		/*
		 * Extract the screen default visual from the config file
		 * structure. 
		 */

		switch(screen_state_p->generic_state.screen_config_p->visual_type)
		{
			case STATICGRAY_AVAIL :
				screen_default_visual = P9000_VISUAL_STATIC_GRAY;
				break;
				
			case GRAYSCALE_AVAIL :
				screen_default_visual = P9000_VISUAL_GRAY_SCALE;
				break;

			case STATICCOLOR_AVAIL :
				screen_default_visual = P9000_VISUAL_STATIC_COLOR;
				break;

			case PSEUDOCOLOR_AVAIL :
				screen_default_visual = P9000_VISUAL_PSEUDO_COLOR;
				break;
			case TRUECOLOR_AVAIL :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
			case DIRECTCOLOR_AVAIL :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
		}

		ASSERT(screen_default_visual != P9000_VISUAL_NULL);

		if (screen_default_visual != P9000_VISUAL_NULL)
		{
			visual_count = 1;	
		}


		/*
		 * Allocate space for the required number of visuals
		 * and colormaps.
		 */

		if (visual_count > 0)
		{
			struct generic_visual *tmp_visual_p, *visuals_list_p;
			
			ASSERT(visual_count == 1);

			/*
			 * update SI's visual count
			 */

			si_screen_p->flagsPtr->SIvisualCNT =
				screen_state_p->generic_state.screen_number_of_visuals =
					visual_count;		

			/*
			 * Allocate space for SI,generic visual and the colormap
			 * pointers.
			 */

			si_screen_p->flagsPtr->SIvisuals = si_visual_p =
				allocate_and_clear_memory(sizeof(SIVisual));

			screen_state_p->generic_state.screen_visuals_list_p =
				visuals_list_p = allocate_and_clear_memory( 
				sizeof(struct generic_visual));

			screen_state_p->generic_state.screen_colormaps_pp =
				allocate_and_clear_memory(sizeof(struct generic_colormap *));


			/*
			 * Initialize the generic and SI visual. 
			 */

			tmp_visual_p = screen_state_p->generic_state.
				screen_visuals_list_p;

			STAMP_OBJECT(GENERIC_VISUAL, tmp_visual_p);

			tmp_visual_p->si_visual_p = si_visual_p;


			/*
			 * Copy visuals in, allocate colormap space.
			 */

			tmp_visual_p = visuals_list_p;
			
			*si_visual_p = p9000_visuals_table[screen_default_visual];
			si_visual_p->SVdepth = screen_state_p->generic_state.screen_depth;


			/*
			 * Assumption: TRUE/DIRECT colors are supported only for
			 * depths greater than 16.
			 */

			if (si_visual_p->SVdepth < 16)
			{
				si_visual_p->SVcmapsz = (1U << si_visual_p->SVdepth);
			}

			si_visual_p->SVbitsrgb = 
				(*dac_functions_p->get_bits_per_rgb_p)();
		
			ASSERT(si_visual_p->SVbitsrgb  > 0);
					

			/*
			 * Initialize the set and get color methods for this visual.
			 */

			switch(screen_default_visual)
			{
				case  P9000_VISUAL_TRUE_COLOR_24_RGB:
				case  P9000_VISUAL_TRUE_COLOR_16_565:
				case  P9000_VISUAL_TRUE_COLOR_16_555:

					tmp_visual_p->get_color_method_p = 
						dac_functions_p->dac_get_truecolor_p;

					tmp_visual_p->set_color_method_p = NULL;

					break;

				case  P9000_VISUAL_DIRECT_COLOR_24_RGB:
				case  P9000_VISUAL_DIRECT_COLOR_16_565:
				case  P9000_VISUAL_DIRECT_COLOR_16_555:

					tmp_visual_p->get_color_method_p = 
						dac_functions_p->dac_get_directcolor_p;

					tmp_visual_p->set_color_method_p =
						dac_functions_p->dac_set_directcolor_p;
					break;

				default:
					tmp_visual_p->get_color_method_p =
						dac_functions_p->dac_get_pseudocolor_p;
					tmp_visual_p->set_color_method_p =
						dac_functions_p->dac_set_pseudocolor_p;
					break;
			}

			/*
			 * Allocate the number of colormaps specified in
			 * the visual.
			 */

			screen_state_p->generic_state.screen_colormaps_pp[0] = 
				colormap_p = allocate_and_clear_memory(si_visual_p->SVcmapcnt *
				sizeof(struct generic_colormap));
			
			/*
			 * Initialize colormaps.
			 */

			for (index = 0; index < si_visual_p->SVcmapcnt; index ++)
			{
				/*
				 * fill in SI information.
				 */

				colormap_p[index].si_colormap.visual = si_visual_p->SVtype;
				colormap_p[index].si_colormap.sz = si_visual_p->SVcmapsz;

				/*
				 * allocate memory for RGB values.
				 */

				colormap_p[index].rgb_values_p = allocate_and_clear_memory(
					3 * si_visual_p->SVcmapsz * sizeof(unsigned short));
				colormap_p[index].visual_p = tmp_visual_p;
				
				STAMP_OBJECT(GENERIC_COLORMAP,
							 &(colormap_p[index]));

				/*
				 * In 4/8 bit modes, the hardware supports a
				 * programmable visual, but for visuals other than
				 * pseudocolor, we are trying to fake
				 * `staticness'.  For such visuals, fill up the
				 * default colormap and program the DAC
				 * accordingly.
				 * Assumption here is that there is no TrueColor
				 * with 8/4 bits per pixel. So if we have a non
				 * Programmable visual here it is a static color
				 * visual.
				 */

				if (!P9000_IS_PROGRAMMABLE_VISUAL(tmp_visual_p) &&
					screen_state_p->generic_state.screen_depth < 16)
				{
					/*CONSTANTCONDITION*/
					ASSERT(0);
				}
			}


			/*
			 * Initialize set and get colormap methods for the default
			 * visual only. No idea how this will work for multiple
			 * visuals (once si starts handling them.)
			 */

			tmp_visual_p = 
				screen_state_p->generic_state.screen_visuals_list_p;

			switch(screen_state_p->generic_state.screen_depth)
			{ 
				case 4:
				case 8:

					if(P9000_IS_PROGRAMMABLE_VISUAL(tmp_visual_p))
					{
						screen_state_p->generic_state.
							screen_functions_p->si_set_colormap =
							p9000_colormap_set_colormap_pseudocolor;
					}
					else
					{
						screen_state_p->generic_state.
							screen_functions_p->si_set_colormap =
							(SIBool (*)())
							p9000_global_no_operation_succeed;
					}

					screen_state_p->generic_state.
						screen_functions_p->si_get_colormap =
						p9000_colormap_get_colormap_pseudocolor;

					break;

				case 16:
				case 24:

					if(P9000_IS_PROGRAMMABLE_VISUAL(tmp_visual_p))
					{
						screen_state_p->generic_state.
							screen_functions_p->si_set_colormap =
							p9000_colormap_set_colormap_directcolor;

						screen_state_p->generic_state.
							screen_functions_p->si_get_colormap =
							p9000_colormap_get_colormap_directcolor;
					}
					else
					{
						screen_state_p->generic_state.
							screen_functions_p->si_set_colormap =
							(SIBool (*)())
							p9000_global_no_operation_succeed;

						screen_state_p->generic_state.
							screen_functions_p->si_get_colormap =
							p9000_colormap_get_colormap_truecolor;
					}
					break;
			}
		}
		else
		{
			si_screen_p->flagsPtr->SIvisualCNT = 0;
		}
	}
}
