/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_cmap.c	1.7"

/***
 ***	NAME
 ***
 ***		m64_cmap.c : colormap handling for the M64 chipset.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m64_cmap.h"
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

enum m64_visual_kind 
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	M64_VISUAL_##NAME
#include "m64_vis.def"
#undef DEFINE_VISUAL
};

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
export boolean m64_colormap_debug = 0;
export boolean m64_video_blank_debug = 0;
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
#include "g_state.h"
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_state.h"
#include "m64_mischw.h"

/***
 ***	Constants.
 ***/

STATIC const SIVisual 
m64_visuals_table[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	{\
		 TYPE, DEPTH, N_COLORMAPS, SIZE, N_VALID_BITS,\
		 R_MASK, G_MASK, B_MASK, R_OFFSET, G_OFFSET, B_OFFSET\
	}
#include "m64_vis.def"			
#undef DEFINE_VISUAL	
};

#if (defined(__DEBUG__))
STATIC const char *const m64_visual_kind_to_visual_kind_dump[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
					  #NAME
#include "m64_vis.def"			
#undef DEFINE_VISUAL	
};
#endif	

/***
 ***	Macros.
 ***/
/*
 * Determining if a visual supports programmable colormaps
 */
#define M64_IS_PROGRAMMABLE_VISUAL(visual_p)\
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
m64_set_colormap_pseudocolor(SIint32 visual_number, 
	SIint32 colormap_number, SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		  screen_visuals_list_p[visual_number]);
	SIVisualP si_visual_p = visual_p->si_visual_p;
	
	struct generic_colormap *colormap_p = NULL;

	unsigned short *rgb_p;
	unsigned int rgb_shift_count;
	void (*dac_set_color_p)(const struct generic_visual *this_p,
	   const int index, unsigned short *rgb_p);
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL, visual_p));
	ASSERT(si_visual_p);
	ASSERT(visual_number >= 0 && visual_number <
		   generic_current_screen_state_p->screen_number_of_visuals);
	ASSERT(colormap_number >=0 && colormap_number < si_visual_p->SVcmapcnt);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	if (count <= 0)
	{
		return SI_SUCCEED;
	}
	
	colormap_p = &(generic_current_screen_state_p->
		  screen_colormaps_pp[visual_number][colormap_number]);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP,colormap_p));
	
	rgb_p = colormap_p->rgb_values_p;

	dac_set_color_p = visual_p->set_color_method_p;
	
	ASSERT(rgb_p != NULL);
	ASSERT(dac_set_color_p != NULL);
	
#if (defined(__DEBUG__))
	if (m64_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_dac_set_colormap_pseudocolor) {\n"
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
	if (! M64_IS_PROGRAMMABLE_VISUAL(visual_p))
	{
#if (defined(__DEBUG__))
		if (m64_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
				"(m64_colormap_set_colormap_pseudocolor)\n"
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
	rgb_shift_count = (si_visual_p->SVbitsrgb == 8) ?
		8U : 10U;

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
				(visual_p,
				 colors_p->SCpindex, 
				 &(colormap_p->rgb_values_p[3 * colors_p->SCpindex]));
		}
		else
		{
#if (defined(__DEBUG__))
			if (m64_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tbad color index (0x%lx)\n",
							   colors_p->SCpindex);
			}
#endif
		}
		colors_p++;
	}
	return (SI_SUCCEED);
}


STATIC SIBool
m64_get_colormap_pseudocolor(SIint32 visual_number, 
				   SIint32 colormap_number,
				   SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		  screen_visuals_list_p[visual_number]);

	SIVisualP si_visual_p = visual_p->si_visual_p;
	
	struct generic_colormap *colormap_p = NULL;

	unsigned short *rgb_p;
	unsigned int rgb_shift_count;

	void (*dac_get_color_p)(const struct generic_visual *visual_p,
							const int index, unsigned short *rgb_p);
	
	ASSERT(visual_number >= 0 && visual_number < 
		generic_current_screen_state_p->screen_number_of_visuals);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL,visual_p));
	ASSERT(colormap_number >=0 && colormap_number < si_visual_p->SVcmapcnt);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, 
		generic_current_screen_state_p));

	if (count <= 0)
	{
		return SI_SUCCEED;
	}

	colormap_p = &(generic_current_screen_state_p->
				   screen_colormaps_pp[visual_number][colormap_number]);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));
	
	dac_get_color_p = visual_p->get_color_method_p;
	rgb_p = colormap_p->rgb_values_p;
	
	ASSERT(rgb_p != NULL);
	ASSERT(dac_get_color_p != NULL);
	

#if (defined(__DEBUG__))
	if (m64_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_get_colormap_pseudocolor) {\n"
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

	rgb_shift_count = (si_visual_p->SVbitsrgb == 8) ? 8U : 10U;
	
	while(count --)
	{
		if (colors_p->SCpindex >= 0 && 
			colors_p->SCpindex < si_visual_p->SVcmapsz)
		{
			rgb_p = &(colormap_p->
					  rgb_values_p[3 * colors_p->SCpindex]);
			
			(*dac_get_color_p)(visual_p, colors_p->SCpindex, rgb_p); 
			
			colors_p->SCred = (*rgb_p++) << rgb_shift_count;
			colors_p->SCgreen = (*rgb_p++) << rgb_shift_count;
			colors_p->SCblue = (*rgb_p++) << rgb_shift_count;

		}
#if (defined(__DEBUG__))
		else
		{
			if (m64_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tbad color index (0x%lx)\n", 
							   colors_p->SCpindex);
			}
		}
#endif
		colors_p++;
		
	}
	return (SI_SUCCEED);
}

/*
 * Just assumes that the individual r/g/b dacs are serially programmed
 * from 1....max entry of that dac.
 */
STATIC SIBool
m64_get_colormap_truecolor(SIint32 visual_number, 
				   SIint32 colormap_number,
				   SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		  screen_visuals_list_p[visual_number]);

	SIVisualP si_visual_p = visual_p->si_visual_p;
	int number_of_reds = 
		(((unsigned)si_visual_p->SVredmask) >> si_visual_p->SVredoffset) + 1;
	int number_of_greens = 
		(((unsigned)si_visual_p->SVgreenmask) >> 
		si_visual_p->SVgreenoffset) + 1;
	int number_of_blues =
		(((unsigned)si_visual_p->SVbluemask) >> si_visual_p->SVblueoffset) + 1;
	
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
m64_set_colormap_directcolor(SIint32 visual_number, 
	SIint32 colormap_number, SIColor *colors_p, SIint32 count)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		  screen_visuals_list_p[visual_number]);
	SIVisualP si_visual_p = visual_p->si_visual_p;
	
	struct generic_colormap *colormap_p = NULL;

	int dac_rgb_width = 6;
	unsigned short rgb[3];
	void (*dac_set_color_p)(const struct generic_visual *this_p,
	   const int index, unsigned short *rgb);
	void (*dac_get_color_p)(const struct generic_visual *visual_p,
		const int index, unsigned short *rgb);

	int number_of_reds = 
		(((unsigned)si_visual_p->SVredmask) >> si_visual_p->SVredoffset) + 1;
	int number_of_greens = 
		(((unsigned)si_visual_p->SVgreenmask) >> 
		si_visual_p->SVgreenoffset) + 1;
	int number_of_blues =
		(((unsigned)si_visual_p->SVbluemask) >> si_visual_p->SVblueoffset) + 1;
	
	ASSERT(number_of_reds > 0 && number_of_reds <=256);
	ASSERT(number_of_greens > 0 && number_of_greens <=256);
	ASSERT(number_of_blues > 0 && number_of_blues <=256);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL, visual_p));
	ASSERT(si_visual_p);
	ASSERT(visual_number >= 0 && visual_number <
		   generic_current_screen_state_p->screen_number_of_visuals);
	ASSERT(colormap_number >=0 && colormap_number < si_visual_p->SVcmapcnt);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));

	if (count <= 0)
	{
		return SI_SUCCEED;
	}

	colormap_p = &(generic_current_screen_state_p->
		  screen_colormaps_pp[visual_number][colormap_number]);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP,colormap_p));

	dac_get_color_p = visual_p->get_color_method_p;
	dac_set_color_p = visual_p->set_color_method_p;

	if (screen_state_p->options_p->dac_rgb_width == 
		M64_OPTIONS_DAC_RGB_WIDTH_8)
	{
		dac_rgb_width = 8;
	}

#if (defined(__DEBUG__))
	if (m64_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_set_colormap_directcolor) {\n"
			"\tvisual_number = %ld\n"
			"\tcolormap_number = %ld\n"
			"\tcolormap_p = %p\n"
			"\tdac_get_color_p = %p\n"
			"\tcolors_p = %p\n"
			"\tcount = %ld\n"
			"\tnumber_of_reds = %ld\n"
			"\tnumber_of_greens = %ld\n"
			"\tnumber_of_blues = %ld\n"
			"}\n",
			visual_number, colormap_number,
			(void *) colormap_p,
			(void *) dac_get_color_p,
			(void *) colors_p, count, number_of_reds,
			number_of_greens, number_of_blues);
	}
#endif

	/*
	 * Fill in the appropriate R/G or B colormap.
	 */

	for(;count --; colors_p++)
	{
		unsigned short *rgb_p;

		/*
		 * To update internal copy of colormap.
		 */
		rgb_p = &(colormap_p->rgb_values_p[3 * colors_p->SCpindex]);

		ASSERT(colors_p->SCpindex >= 0);
		ASSERT((colors_p->SCpindex <= number_of_reds) ||
			(colors_p->SCpindex <= number_of_greens) ||
			(colors_p->SCpindex <= number_of_blues));

		/*
		 * Save the RGB values already in the dac.
		 */
		(*dac_get_color_p)(visual_p, colors_p->SCpindex, rgb); 

		switch (colors_p->SCvalid)
		{
		case VALID_RED :
			rgb[0] = (colors_p->SCred >> 
				(16 - dac_rgb_width)) & ((1 << dac_rgb_width) - 1);
			*rgb_p = colors_p->SCred;
			break;
			
		case VALID_GREEN :
			rgb[1] = (colors_p->SCgreen >> 
				(16 - dac_rgb_width)) & ((1 << dac_rgb_width) - 1);
			*(rgb_p + 1) = colors_p->SCgreen;
			break;
			
		case VALID_BLUE :
			rgb[2] = (colors_p->SCblue >> 
				(16 - dac_rgb_width)) & ((1 << dac_rgb_width) - 1);
			*(rgb_p + 2) = colors_p->SCblue;
			break;
			
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}

		/*
		 * Program the dac.
		 */
		(*dac_set_color_p)(visual_p, colors_p->SCpindex, rgb);
	}
	
	return (SI_SUCCEED);
}

STATIC SIBool
m64_get_colormap_directcolor(SIint32 visual_number, 
	SIint32 colormap_number, SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		  screen_visuals_list_p[visual_number]);
	SIVisualP si_visual_p = visual_p->si_visual_p;
	
	struct generic_colormap *colormap_p = NULL;

	unsigned short rgb[3];
	void (*dac_get_color_p)(const struct generic_visual *visual_p,
		const int index, unsigned short *rgb);

	int number_of_reds = 
		(((unsigned)si_visual_p->SVredmask) >> si_visual_p->SVredoffset) + 1;
	int number_of_greens = 
		(((unsigned)si_visual_p->SVgreenmask) >> 
		si_visual_p->SVgreenoffset) + 1;
	int number_of_blues =
		(((unsigned)si_visual_p->SVbluemask) >> si_visual_p->SVblueoffset) + 1;
	
	ASSERT(number_of_reds > 0 && number_of_reds <=256);
	ASSERT(number_of_greens > 0 && number_of_greens <=256);
	ASSERT(number_of_blues > 0 && number_of_blues <=256);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL, visual_p));
	ASSERT(si_visual_p);
	ASSERT(visual_number >= 0 && visual_number <
		   generic_current_screen_state_p->screen_number_of_visuals);
	ASSERT(colormap_number >=0 && colormap_number < si_visual_p->SVcmapcnt);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));

	if (count <= 0)
	{
		return SI_SUCCEED;
	}

	dac_get_color_p = visual_p->get_color_method_p;

	colormap_p = &(generic_current_screen_state_p->
		  screen_colormaps_pp[visual_number][colormap_number]);

#if (defined(__DEBUG__))
	if (m64_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_get_colormap_directcolor) {\n"
			"\tvisual_number = %ld\n"
			"\tcolormap_number = %ld\n"
			"\tcolormap_p = %p\n"
			"\tdac_get_color_p = %p\n"
			"\tcolors_p = %p\n"
			"\tcount = %ld\n"
			"\tnumber_of_reds = %ld\n"
			"\tnumber_of_greens = %ld\n"
			"\tnumber_of_blues = %ld\n"
			"}\n",
			visual_number, colormap_number,
			(void *) colormap_p,
			(void *) dac_get_color_p,
			(void *) colors_p, count, number_of_reds,
			number_of_greens, number_of_blues);
	}
#endif

	/*
	 * Fill in the appropriate R/G or B colormap.
	 */
	for(;count --; colors_p++)
	{
		unsigned short *rgb_p;

		/*
		 * To update internal copy of colormap.
		 */
		rgb_p = &(colormap_p->rgb_values_p[3 * colors_p->SCpindex]);

		ASSERT(colors_p->SCpindex >= 0);
		ASSERT((colors_p->SCpindex <= number_of_reds) ||
			(colors_p->SCpindex <= number_of_greens) ||
			(colors_p->SCpindex <= number_of_blues));

		/*
		 * Get the RGB values already in the dac.
		 */
		(*dac_get_color_p)(visual_p, colors_p->SCpindex, rgb); 

		switch (colors_p->SCvalid)
		{
		case VALID_RED :
			colors_p->SCred = ((unsigned long)(rgb[0]) << 16) / number_of_reds;
			*rgb_p = colors_p->SCred;
			break;
			
		case VALID_GREEN :
			colors_p->SCgreen = 
				((unsigned long)(rgb[1]) << 16) / number_of_greens; 
			*(rgb_p + 1) = colors_p->SCgreen;
			break;
			
		case VALID_BLUE :
			colors_p->SCblue = 
				((unsigned long)(rgb[2]) << 16) / number_of_blues;
			*(rgb_p + 2) = colors_p->SCblue;
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
m64_cmap_video_blank_onoff(SIBool on)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (m64_video_blank_debug)
	{
		(void)fprintf(debug_stream_p, 
			"\tm64_video_blank_onoff()\n"
			"\t{\n"
			"\ton = %d\n"
			"\t}\n",
			on);
	}
#endif
	
	/* 
	 * Call the dac function for video blanking.
	 */
	screen_state_p->dac_state_p->dac_video_blank(on);
	return (SI_SUCCEED);
}

STATIC void
m64_colormap_initialize_static_colormap( struct generic_colormap *colormap_p,
	struct m64_options_structure *options_p)
{
	FILE *colormap_file_p;
	char *colormap_file_name_p;
	int colormap_file_line_number = 0;
	char line_buffer[M64_DEFAULT_COLORMAP_DESCRIPTION_FILE_LINE_SIZE];
	int colormap_entry_count = 0;
	unsigned short *rgb_p;
	int rgb_shift_count;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));

	ASSERT(!M64_IS_PROGRAMMABLE_VISUAL(colormap_p->visual_p));
	
	rgb_p = colormap_p->rgb_values_p;

	colormap_file_name_p =
		(options_p->static_colormap_description_file) ?
		options_p->static_colormap_description_file :
		M64_DEFAULT_COLORMAP_DESCRIPTION_FILE_NAME;

	rgb_shift_count = 
		(colormap_p->visual_p->si_visual_p->SVbitsrgb == 8) ? 8U : 10U;

	if ((colormap_file_p = fopen(colormap_file_name_p, "r")) == NULL)
	{
		(void) fprintf(stderr,
		   	M64_CANNOT_OPEN_COLORMAP_DESCRIPTION_FILE_MESSAGE,
			colormap_file_name_p);
		perror("");
		
		/*
		 * Servers Default colormap 0 = black, 1 = white.
		 */
		*rgb_p++ = (unsigned short)0x0 >> rgb_shift_count;
		*rgb_p++ = (unsigned short)0x0 >> rgb_shift_count;
		*rgb_p++ = (unsigned short)0x0 >> rgb_shift_count;
		*rgb_p++ = (unsigned short)0xFFFF >> rgb_shift_count;
		*rgb_p++ = (unsigned short)0xFFFF >> rgb_shift_count;
		*rgb_p++ = (unsigned short)0xFFFF >> rgb_shift_count;
		for(colormap_entry_count = 2; 
			colormap_entry_count < colormap_p->si_colormap.sz;
			colormap_entry_count++)
		{
			int tmp = ((colormap_entry_count  * 0xFFFF)
				/ colormap_p->si_colormap.sz) >> rgb_shift_count;
			
			*rgb_p ++ = (unsigned short) tmp; 
			*rgb_p ++ = (unsigned short) tmp; 
			*rgb_p ++ = (unsigned short) tmp;
			
		}
	}
	else						/* open succeeded */
	{
		/*
		 * Read in the colormap description file : look for entries of
		 * the format :
		 * [<WS>] <NUM> <NUM> <NUM> [ '#'.* ]
		 */
		while((colormap_entry_count < colormap_p->si_colormap.sz) && 
			  (fgets(line_buffer, sizeof(line_buffer),
					 colormap_file_p) != NULL))
		{
			char *character_p = line_buffer;
			unsigned int red_value, blue_value, green_value;
			
			colormap_file_line_number ++;

			/*
			 * Skip leading white space on the line.
			 */
			while (*character_p && isspace(*character_p))
			{
				character_p ++;
			}

			/*
			 * Skip an empty line.
			 */
			if (!*character_p)
			{
				continue;
			}
			
				 
			/*
			 * If the first non white space character is a number,
			 * treat this line as a colormap entry description.  If
			 * not, the line is treated as a comment.
			 */
			if (isdigit(*character_p))
			{
				if(sscanf(character_p,
						  M64_DEFAULT_COLORMAP_DESCRIPTION_LINE_FORMAT,
						  &red_value, &blue_value, &green_value) != 3)
				{
					(void) fprintf(stderr,
					M64_COLORMAP_DESCRIPTION_LINE_DOES_NOT_MATCH_FORMAT_MESSAGE,
					colormap_file_name_p, colormap_file_line_number, 
					line_buffer);
					return;
				}
				else
				{
				
					/*
					 * Create the colormap entry.
					 */
					*rgb_p++ = (unsigned short) red_value;
					*rgb_p++ = (unsigned short) blue_value;
					*rgb_p++ = (unsigned short) green_value;

					colormap_entry_count ++;

				}
			}
			
			/*
			 * Check if fgets had prematurely truncated this line.
			 */
			if (*(line_buffer + strlen(line_buffer)) != '\n')
			{
				int c;
				
				/*
				 * A very long line was read : skip characters forward
				 * till a newline or EOF.
				 */
				
				while ((c = getc(colormap_file_p)) != '\n' &&
					   (c != EOF))
				{
					;
				}
			}
		}
	}
}

/*
 * m64_cmap__vt_switch_in__
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
m64_cmap__vt_switch_in__(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	struct generic_visual 	*visual_p ; 
	SIVisualP 				si_visual_p;
	struct generic_colormap *colormap_p;
	int						i;
	unsigned short 			*rgb_p;
	void (*dac_set_color_p)(const struct generic_visual *g_vis_p,
	   const int index, unsigned short *rgb_p);

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (m64_colormap_debug)
	{
		(void)fprintf(debug_stream_p, "(m64_cmap__vt_switch_in__){}\n");
	}
#endif

	visual_p = &(generic_current_screen_state_p->screen_visuals_list_p[0]);
	colormap_p = &(generic_current_screen_state_p->screen_colormaps_pp[0][0]);
	si_visual_p = visual_p->si_visual_p;
	dac_set_color_p = visual_p->set_color_method_p;
	rgb_p = colormap_p->rgb_values_p;


	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL, visual_p));
	ASSERT(si_visual_p);

	if (! M64_IS_PROGRAMMABLE_VISUAL(visual_p))
	{
		/* 
		 * Nothing to program.
		 */
		return;
	}

	/*
	 * Check the visual type and do appropriate programming. 
	 */
    if (generic_current_screen_state_p->screen_visuals_list_p[0].
		si_visual_p->SVtype == PseudoColor)
	{
		for ( i = 0; i < si_visual_p->SVcmapsz; ++i)
		{
			(*dac_set_color_p) (visual_p, i, rgb_p);
			rgb_p += 3;
		}
	}
	else
	{
		unsigned short rgb_values[3];
		int dac_rgb_width = 6;
		int number_of_reds, number_of_greens, number_of_blues;
		int colormap_size;

		number_of_reds = (((unsigned)si_visual_p->SVredmask) >> 
			si_visual_p->SVredoffset) + 1;
		number_of_greens = (((unsigned)si_visual_p->SVgreenmask) >> 
			si_visual_p->SVgreenoffset) + 1;
		number_of_blues = (((unsigned)si_visual_p->SVbluemask) >> 
			si_visual_p->SVblueoffset) + 1;

		if (screen_state_p->options_p->dac_rgb_width == 
			M64_OPTIONS_DAC_RGB_WIDTH_8)
		{
			dac_rgb_width = 8;
		}


		colormap_size = (number_of_reds > number_of_greens ? 
			number_of_reds : number_of_greens);
		colormap_size = (colormap_size > number_of_blues ?
			colormap_size : number_of_blues);

		for ( i = 0; i < colormap_size; ++i)
		{
			/*
			 * convert the stored rgb values into the form
			 * accepted by the dac.
			 */
			rgb_values[0] = (*rgb_p >> 
				(16 - dac_rgb_width)) & ((1 << dac_rgb_width) - 1);
			rgb_values[1] = (*(rgb_p + 1) >> 
				(16 - dac_rgb_width)) & ((1 << dac_rgb_width) - 1);
			rgb_values[2] = (*(rgb_p + 2) >> 
				(16 - dac_rgb_width)) & ((1 << dac_rgb_width) - 1);

			(*dac_set_color_p) (visual_p, i, rgb_values);

			rgb_p += 3;
		}
	}

	return;
}

function void
m64_colormap__initialize__(SIScreenRec *si_screen_p,
	struct m64_options_structure *options_p)
{ 
	struct generic_screen_state	*generic_state_p = 
		(struct generic_screen_state *) si_screen_p->vendorPriv;
	struct m64_screen_state *screen_state_p =
		(struct m64_screen_state *) si_screen_p->vendorPriv;
	struct m64_dac_state 	*dac_state_p = screen_state_p->dac_state_p;
	SIVisualP 				si_visual_p = 0;
	enum m64_visual_kind 	visual_kind;
	struct generic_colormap	*colormap_p;
	int 					tmp_count;
	int 					index;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_screen_state *) generic_state_p));

	ASSERT((void *) generic_current_screen_state_p == (void *)screen_state_p);

	/*
	 * Initialize the video blank function.
	 */
	generic_state_p->screen_functions_p->si_vb_onoff = 
		m64_cmap_video_blank_onoff;

	/*
	 * Attempt to read in the visuals.
	 */
	if (!screen_state_p->generic_state.screen_visuals_list_p)
	{
		enum m64_visual_kind screen_default_visual = M64_VISUAL_NULL;
		enum m64_dac_mode dac_mode;
		unsigned int mask;
		int visual_count = 0;
		unsigned int screen_visual_list = options_p->screen_visual_list;
		unsigned int default_visual_as_option_type = 0;
		
#if (defined(__DEBUG__))
		if (m64_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
				"(m64_colormap__initialize__)  reading in visuals {\n"
				"\tvisual_list = 0x%x\n"
				"}\n",
				options_p->screen_visual_list);
		}
#endif
		/*
		 * Extract the screen default visual from the config file
		 * structure. Add this to screen_visual_list.
		 */
		switch(screen_state_p->generic_state.screen_config_p->visual_type)
		{
			case STATICGRAY_AVAIL :
				screen_default_visual = M64_VISUAL_STATIC_GRAY;
				default_visual_as_option_type = 
					M64_OPTIONS_SCREEN_VISUAL_LIST_STATIC_GRAY;
				break;
				
			case GRAYSCALE_AVAIL :
				screen_default_visual = M64_VISUAL_GRAY_SCALE;
				default_visual_as_option_type = 
					M64_OPTIONS_SCREEN_VISUAL_LIST_GRAY_SCALE;
				break;

			case STATICCOLOR_AVAIL :
				screen_default_visual = M64_VISUAL_STATIC_COLOR;
				default_visual_as_option_type = 
					M64_OPTIONS_SCREEN_VISUAL_LIST_STATIC_COLOR;
				break;

			case PSEUDOCOLOR_AVAIL :
				screen_default_visual = M64_VISUAL_PSEUDO_COLOR;
				default_visual_as_option_type = 
					M64_OPTIONS_SCREEN_VISUAL_LIST_PSEUDO_COLOR;
				break;
				
			case TRUECOLOR_AVAIL :
				default_visual_as_option_type = 
					M64_OPTIONS_SCREEN_VISUAL_LIST_TRUE_COLOR;
				if (screen_state_p->generic_state.screen_depth == 16)
				{
					if(options_p->dac_16_bit_color_mode == 
						M64_OPTIONS_DAC_16_BIT_COLOR_MODE_555)
					{
						screen_default_visual = M64_VISUAL_TRUE_COLOR_16_555;
					}
					else
					{
						ASSERT(options_p->dac_16_bit_color_mode == 
							M64_OPTIONS_DAC_16_BIT_COLOR_MODE_565);
						screen_default_visual = M64_VISUAL_TRUE_COLOR_16_565;
					}
				}
				else if (screen_state_p->generic_state.screen_depth == 24)
				{
					screen_default_visual = M64_VISUAL_TRUE_COLOR_24_RGB;
				}
				else if (screen_state_p->generic_state.screen_depth == 32)
				{
					if (options_p->dac_32_bit_color_mode ==
						M64_OPTIONS_DAC_32_BIT_COLOR_MODE_DEFAULT)
					{
						if( strcmp(screen_state_p->dac_state_p->dac_name, 
							"M64_DAC_STG1702") == 0)
						{
							screen_default_visual = 
								M64_VISUAL_TRUE_COLOR_32_ARGB;
						}
						else 
						{
							ASSERT(strcmp(screen_state_p->dac_state_p->
								dac_name, "M64_DAC_ATI68860") == 0);
							screen_default_visual = 
								M64_VISUAL_TRUE_COLOR_32_ABGR;
						}

					}
					else if (options_p->dac_32_bit_color_mode ==
						M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ARGB)
					{
						screen_default_visual = M64_VISUAL_TRUE_COLOR_32_ARGB;
					}
					else if (options_p->dac_32_bit_color_mode ==
						M64_OPTIONS_DAC_32_BIT_COLOR_MODE_RGBA)
					{
						screen_default_visual = M64_VISUAL_TRUE_COLOR_32_RGBA;
					}
					else
					{
						ASSERT(options_p->dac_32_bit_color_mode == 
							M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ABGR);
						screen_default_visual = M64_VISUAL_TRUE_COLOR_32_ABGR;
					}
				}
				break;

			case DIRECTCOLOR_AVAIL :
				default_visual_as_option_type = 
					M64_OPTIONS_SCREEN_VISUAL_LIST_DIRECT_COLOR; 
				if (screen_state_p->generic_state.screen_depth == 16)
				{
					if(options_p->dac_16_bit_color_mode == 
						M64_OPTIONS_DAC_16_BIT_COLOR_MODE_555)
					{
						screen_default_visual = M64_VISUAL_DIRECT_COLOR_16_555;
					}
					else
					{
						ASSERT(options_p->dac_16_bit_color_mode == 
							M64_OPTIONS_DAC_16_BIT_COLOR_MODE_565);
						screen_default_visual = M64_VISUAL_DIRECT_COLOR_16_565;
					}
				}
				else if (screen_state_p->generic_state.screen_depth == 24)
				{
					screen_default_visual = M64_VISUAL_DIRECT_COLOR_24_RGB;
				}
				else if (screen_state_p->generic_state.screen_depth == 32)
				{
					if (options_p->dac_32_bit_color_mode ==
						M64_OPTIONS_DAC_32_BIT_COLOR_MODE_DEFAULT)
					{
						ASSERT(strcmp(screen_state_p->dac_state_p->dac_name, 
							"M64_DAC_ATI68860") == 0);
						screen_default_visual = 
							M64_VISUAL_DIRECT_COLOR_32_ABGR;
					}
					else if (options_p->dac_32_bit_color_mode ==
						M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ARGB)
					{
						screen_default_visual = M64_VISUAL_DIRECT_COLOR_32_ARGB;
					}
					else if (options_p->dac_32_bit_color_mode ==
						M64_OPTIONS_DAC_32_BIT_COLOR_MODE_RGBA)
					{
						screen_default_visual = M64_VISUAL_DIRECT_COLOR_32_RGBA;
					}
					else
					{
						ASSERT(options_p->dac_32_bit_color_mode == 
							M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ABGR);
						screen_default_visual = M64_VISUAL_DIRECT_COLOR_32_ABGR;
					}
				}
				break;
				
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
		}

		ASSERT(screen_default_visual != M64_VISUAL_NULL);
		visual_count = 1;	/* the screen_default_visual is already known.*/

		/*
		 * Count the number of visuals the user wants
		 * NOTE: currently SI does not support multiple visuals.
		 */
		for (mask = 0x1; mask; mask <<= 1)
		{
			switch (mask & screen_visual_list)
			{
				case 0:	/* this type is not requested, ignore. */
					break;
				case M64_OPTIONS_SCREEN_VISUAL_LIST_PSEUDO_COLOR :
				case M64_OPTIONS_SCREEN_VISUAL_LIST_STATIC_COLOR :
				case M64_OPTIONS_SCREEN_VISUAL_LIST_GRAY_SCALE :
				case M64_OPTIONS_SCREEN_VISUAL_LIST_STATIC_GRAY :
				case M64_OPTIONS_SCREEN_VISUAL_LIST_TRUE_COLOR:
				case M64_OPTIONS_SCREEN_VISUAL_LIST_DIRECT_COLOR:
					/*
					 * found a useable visual other than the screen 
					 * default visual type.
					 */
					if(default_visual_as_option_type != 
						(mask & screen_visual_list))
					{
						visual_count ++;
					}
					break;
					
				default :
					/*CONSTANTCONDITION*/
					ASSERT(0);
					break;
			}
		}

		/*
		 * Allocate space for the required number of visuals
		 * and colormaps.
		 */
		if (visual_count > 0)
		{
			struct generic_visual *tmp_visual_p, *visuals_list_p;
			
			/*
			 * update SI's visual count
			 */
			si_screen_p->flagsPtr->SIvisualCNT =
			screen_state_p->generic_state.screen_number_of_visuals =
				visual_count;		

			/*
			 * Allocate space for SI,generic visuals and the colormap
			 * pointers.
			 */
			si_screen_p->flagsPtr->SIvisuals = si_visual_p =
				allocate_and_clear_memory(visual_count * sizeof(SIVisual));

			screen_state_p->generic_state.screen_visuals_list_p =
				visuals_list_p = allocate_and_clear_memory(visual_count * 
				sizeof(struct generic_visual));

			screen_state_p->generic_state.screen_colormaps_pp =
				allocate_and_clear_memory(visual_count * 
				sizeof(struct generic_colormap *));

#if (defined(__DEBUG__))
			if (m64_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
				"(m64_colormap__initialize__) {\n"
				"\tnumber of visuals = %d\n"
				"\tvisuals_list = %p\n"
				"\tcolormaps_list_pp = %p\n"
				"}\n",
				visual_count, 
				(void *) screen_state_p->generic_state.screen_visuals_list_p,
				(void *) screen_state_p->generic_state.screen_colormaps_pp);
			}
#endif

			/*
			 * Initialize the generic and SI visuals. The screen
			 * default_visual will be the first one followed by
			 * the remaining members of the list.
			 */
			ASSERT(visual_count > 0);
			for(tmp_count = 0; tmp_count < visual_count; tmp_count ++)
			{
				tmp_visual_p = &(screen_state_p->generic_state.
					screen_visuals_list_p[tmp_count]);

#if (defined(__DEBUG__))
				STAMP_OBJECT(GENERIC_VISUAL, tmp_visual_p);
#endif				
				/*
				 * The SI visual
				 */
				tmp_visual_p->si_visual_p = &(si_visual_p[tmp_count]);
			}

			/*
			 * Copy visuals in, allocate colormap space.
			 */
			tmp_count = 0;
			tmp_visual_p = visuals_list_p;
			
			*si_visual_p = m64_visuals_table[screen_default_visual];
			si_visual_p->SVdepth = screen_state_p->generic_state.screen_depth;
			/*
			 * Assumption: TRUE/DIRECT colors are supported only for
			 * depths greater than 16.
			 */
			if (si_visual_p->SVdepth < 16)
			{
				si_visual_p->SVcmapsz = (1U << si_visual_p->SVdepth);
			}
			si_visual_p->SVbitsrgb = 6;

			if (options_p->dac_rgb_width == M64_OPTIONS_DAC_RGB_WIDTH_8)
			{
				si_visual_p->SVbitsrgb = 8;
			}

			/*
			 * Pass down the visual type thro dac flags so that 
			 * check display mode feasibility can use it efficiently.
			 */
			tmp_visual_p->dac_flags = screen_default_visual;

			/*
			 * Initialize the set and get color methods for this visual.
			 */
			switch(screen_default_visual)
			{
				case  M64_VISUAL_TRUE_COLOR_32_ARGB:
				case  M64_VISUAL_TRUE_COLOR_32_ABGR:
				case  M64_VISUAL_TRUE_COLOR_32_RGBA:
				case  M64_VISUAL_TRUE_COLOR_24_RGB:
				case  M64_VISUAL_TRUE_COLOR_16_565:
				case  M64_VISUAL_TRUE_COLOR_16_555:
					tmp_visual_p->get_color_method_p = 
						screen_state_p->dac_state_p->dac_get_directcolor;
					tmp_visual_p->set_color_method_p =
						screen_state_p->dac_state_p->dac_set_directcolor;
					break;

				case  M64_VISUAL_DIRECT_COLOR_32_ARGB:
				case  M64_VISUAL_DIRECT_COLOR_32_ABGR:
				case  M64_VISUAL_DIRECT_COLOR_32_RGBA:
				case  M64_VISUAL_DIRECT_COLOR_24_RGB:
				case  M64_VISUAL_DIRECT_COLOR_16_565:
				case  M64_VISUAL_DIRECT_COLOR_16_555:
					tmp_visual_p->get_color_method_p =
						screen_state_p->dac_state_p->dac_get_directcolor;
					tmp_visual_p->set_color_method_p =
						screen_state_p->dac_state_p->dac_set_directcolor;
					break;
				default:
					tmp_visual_p->get_color_method_p =
						screen_state_p->dac_state_p->dac_get_pseudocolor;
					tmp_visual_p->set_color_method_p =
						screen_state_p->dac_state_p->dac_set_pseudocolor;
					break;
			}

			/*
			 * Allocate the number of colormaps specified in
			 * the visual.
			 */
			screen_state_p->generic_state.screen_colormaps_pp[tmp_count] = 
				colormap_p = allocate_and_clear_memory(si_visual_p->SVcmapcnt *
				sizeof(struct generic_colormap));
			
#if (defined(__DEBUG__))
			if (m64_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
				"(m64_colormap__initialize__) {\n"
				"\tvisual_kind = %s (DEFAULT)\n"
				"\tsi_visual_p = %p\n"
				"\ttmp_visual_p = %p\n"
				"\tcolormap_list_p = %p\n"
				"\tn_colormaps = %ld\n"
				"\t}\n",
				m64_visual_kind_to_visual_kind_dump[ screen_default_visual],
				(void *) si_visual_p,
				(void *) tmp_visual_p,
				(void *) colormap_p,
				si_visual_p->SVcmapcnt);
			}
#endif
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
				
#if (defined(__DEBUG__))
				if (m64_colormap_debug)
				{
					(void) fprintf(debug_stream_p,
					"\t{\n"
					"\t\tvisual = %d\n"
					"\t\tsize = %d\n"
					"\t\trgb_values_p = %p\n"
					"\t}\n",
					colormap_p[index].
					si_colormap.visual,
					colormap_p[index].si_colormap.sz,
					(void *)colormap_p[index].rgb_values_p);
				}
				STAMP_OBJECT(GENERIC_COLORMAP,
							 &(colormap_p[index]));
#endif
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
				if (!M64_IS_PROGRAMMABLE_VISUAL(tmp_visual_p) &&
					screen_state_p->generic_state.screen_depth < 16)
				{
					m64_colormap_initialize_static_colormap(
						&(colormap_p[index]), options_p);
				}
			}

			/*
			 * Now handle the remaining visuals in the list. We just finished
			 * with the screen default visual.
			 */
			for (mask = 0x1; 
				mask && (tmp_count < (visual_count - 1)); 
				mask <<= 1)
			{
				
				if ((default_visual_as_option_type == mask) ||
					((mask & options_p->screen_visual_list) == 0))
				{
					continue; /* we have already handled this */
				}

				switch(mask & options_p->screen_visual_list)
				{
					case M64_OPTIONS_SCREEN_VISUAL_LIST_PSEUDO_COLOR :
						visual_kind = M64_VISUAL_PSEUDO_COLOR;
						break;

					case M64_OPTIONS_SCREEN_VISUAL_LIST_STATIC_COLOR :
						visual_kind = M64_VISUAL_STATIC_COLOR;
						break;
						
					case M64_OPTIONS_SCREEN_VISUAL_LIST_STATIC_GRAY :
						visual_kind = M64_VISUAL_STATIC_GRAY;
						break;

					case M64_OPTIONS_SCREEN_VISUAL_LIST_GRAY_SCALE :
						visual_kind = M64_VISUAL_GRAY_SCALE;
						break;

					case M64_OPTIONS_SCREEN_VISUAL_LIST_TRUE_COLOR :
						if (screen_state_p->generic_state.screen_depth == 16)
						{
							if(options_p->dac_16_bit_color_mode == 
								M64_OPTIONS_DAC_16_BIT_COLOR_MODE_555)
							{
								visual_kind = M64_VISUAL_TRUE_COLOR_16_555;
							}
							else
							{
								ASSERT(options_p->dac_16_bit_color_mode == 
									M64_OPTIONS_DAC_16_BIT_COLOR_MODE_565);
								visual_kind = M64_VISUAL_TRUE_COLOR_16_565;
							}
						}
						else if(screen_state_p->generic_state.screen_depth == 
							24)
						{
							visual_kind = M64_VISUAL_TRUE_COLOR_24_RGB;
						}
						else if(screen_state_p->generic_state.screen_depth == 
							32)
						{
							if (options_p->dac_32_bit_color_mode ==
								M64_OPTIONS_DAC_32_BIT_COLOR_MODE_DEFAULT)
							{
								if( strcmp(screen_state_p->dac_state_p->
									dac_name, "M64_DAC_STG1702") == 0)
								{
									visual_kind = 
										M64_VISUAL_TRUE_COLOR_32_ARGB;
								}
								else 
								{
									ASSERT(strcmp(screen_state_p->dac_state_p->
										dac_name, "M64_DAC_ATI68860") == 0);
									visual_kind = 
										M64_VISUAL_TRUE_COLOR_32_ABGR;
								}

							}
							else if (options_p->dac_32_bit_color_mode ==
								M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ARGB)
							{
								visual_kind = M64_VISUAL_TRUE_COLOR_32_ARGB;
							}
							else if (options_p->dac_32_bit_color_mode ==
								M64_OPTIONS_DAC_32_BIT_COLOR_MODE_RGBA)
							{
								visual_kind = M64_VISUAL_TRUE_COLOR_32_RGBA;
							}
							else
							{
								ASSERT(options_p->dac_32_bit_color_mode == 
									M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ABGR);
								visual_kind = M64_VISUAL_TRUE_COLOR_32_ABGR;
							}
						}
						break;

					case M64_OPTIONS_SCREEN_VISUAL_LIST_DIRECT_COLOR :
						if (screen_state_p->generic_state.screen_depth == 16)
						{
							if(options_p->dac_16_bit_color_mode == 
								M64_OPTIONS_DAC_16_BIT_COLOR_MODE_555)
							{
								visual_kind = M64_VISUAL_DIRECT_COLOR_16_555;
							}
							else
							{
								ASSERT(options_p->dac_16_bit_color_mode == 
									M64_OPTIONS_DAC_16_BIT_COLOR_MODE_565);
								visual_kind = M64_VISUAL_DIRECT_COLOR_16_565;
							}
						}
						else if(screen_state_p->generic_state.screen_depth == 
							24)
						{
							visual_kind = M64_VISUAL_DIRECT_COLOR_24_RGB;
						}
						else if(screen_state_p->generic_state.screen_depth == 
							32)
						{
							if (options_p->dac_32_bit_color_mode ==
								M64_OPTIONS_DAC_32_BIT_COLOR_MODE_DEFAULT)
							{
								ASSERT(strcmp(screen_state_p->dac_state_p->
									dac_name, "M64_DAC_ATI68860") == 0);
								visual_kind = 
									M64_VISUAL_DIRECT_COLOR_32_ABGR;
							}
							else if (options_p->dac_32_bit_color_mode ==
								M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ARGB)
							{
								visual_kind = M64_VISUAL_DIRECT_COLOR_32_ARGB;
							}
							else if (options_p->dac_32_bit_color_mode ==
								M64_OPTIONS_DAC_32_BIT_COLOR_MODE_RGBA)
							{
								visual_kind = M64_VISUAL_DIRECT_COLOR_32_RGBA;
							}
							else
							{
								ASSERT(options_p->dac_32_bit_color_mode == 
									M64_OPTIONS_DAC_32_BIT_COLOR_MODE_ABGR);
								visual_kind = M64_VISUAL_DIRECT_COLOR_32_ABGR;
							}
						}
						break;
					default:
						visual_kind = M64_VISUAL_NULL;
						break;
						
				}

				if (visual_kind == M64_VISUAL_NULL)
				{
					/*
					 * Empty slot in the mask, not in list.
					 */
					continue;
				}

				/*
				 * Found a slot in mask that is in the visual list.
				 * Advance to the correct visual slot.
				 */
				++si_visual_p;
				++tmp_visual_p;
				++tmp_count;

				*si_visual_p = m64_visuals_table[visual_kind];
				si_visual_p->SVdepth = 
					screen_state_p->generic_state.screen_depth;
				if (si_visual_p->SVdepth < 16)
				{
					si_visual_p->SVcmapsz = (1U << si_visual_p->SVdepth);
				}
				si_visual_p->SVbitsrgb = 6;

				if (options_p->dac_rgb_width == M64_OPTIONS_DAC_RGB_WIDTH_8)
				{
					si_visual_p->SVbitsrgb = 8;
				}

				tmp_visual_p->dac_flags = visual_kind;
				switch(visual_kind)
				{
					case  M64_VISUAL_TRUE_COLOR_32_ARGB:
					case  M64_VISUAL_TRUE_COLOR_32_ABGR:
					case  M64_VISUAL_TRUE_COLOR_32_RGBA:
					case  M64_VISUAL_TRUE_COLOR_24_RGB:
					case  M64_VISUAL_TRUE_COLOR_16_565:
					case  M64_VISUAL_TRUE_COLOR_16_555:
						tmp_visual_p->get_color_method_p = 
							screen_state_p->dac_state_p->dac_get_directcolor;
						tmp_visual_p->set_color_method_p =
							screen_state_p->dac_state_p->dac_set_directcolor;
						break;

					case  M64_VISUAL_DIRECT_COLOR_32_ARGB:
					case  M64_VISUAL_DIRECT_COLOR_32_ABGR:
					case  M64_VISUAL_DIRECT_COLOR_32_RGBA:
					case  M64_VISUAL_DIRECT_COLOR_24_RGB:
					case  M64_VISUAL_DIRECT_COLOR_16_565:
					case  M64_VISUAL_DIRECT_COLOR_16_555:
						tmp_visual_p->get_color_method_p =
							screen_state_p->dac_state_p->dac_get_directcolor;
						tmp_visual_p->set_color_method_p =
							screen_state_p->dac_state_p->dac_set_directcolor;
						break;
					default:
						tmp_visual_p->get_color_method_p =
							screen_state_p->dac_state_p->dac_get_pseudocolor;
						tmp_visual_p->set_color_method_p =
							screen_state_p->dac_state_p->dac_set_pseudocolor;
						break;
				}

				/*
				 * Allocate the number of colormaps specified in
				 * the visual.
				 */
				screen_state_p->generic_state.screen_colormaps_pp[tmp_count] =
					colormap_p = allocate_and_clear_memory(si_visual_p->
						SVcmapcnt * sizeof(struct generic_colormap));
				
#if (defined(__DEBUG__))
				if (m64_colormap_debug)
				{
					(void) fprintf(debug_stream_p,
					"(m64_colormap__initialize__) {\n"
					"\tvisual_kind = %s\n"
					"\tsi_visual_p = %p\n"
					"\ttmp_visual_p = %p\n"
					"\tcolormap_list_p = %p\n"
					"\tn_colormaps = %ld\n"
					"\t}\n",
					m64_visual_kind_to_visual_kind_dump[visual_kind],
					(void *) si_visual_p,
					(void *) tmp_visual_p,
					(void *) colormap_p,
					si_visual_p->SVcmapcnt);
				}
#endif
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
					
#if (defined(__DEBUG__))
					if (m64_colormap_debug)
					{
						(void) fprintf(debug_stream_p,
						"\t{\n"
						"\t\tvisual = %d\n"
						"\t\tsize = %d\n"
						"\t\trgb_values_p = %p\n"
						"\t}\n",
						colormap_p[index].
						si_colormap.visual,
						colormap_p[index].si_colormap.sz,
						(void *)colormap_p[index].rgb_values_p);
					}
					STAMP_OBJECT(GENERIC_COLORMAP,&(colormap_p[index]));
#endif
				}
				
			}

			/*
			 * Initialize set and get colormap methods for the default
			 * visual only. No idea how this will work for multiple
			 * visuals (once si starts handling them.)
			 */
			tmp_visual_p = 
				&(screen_state_p->generic_state.screen_visuals_list_p[0]);
			switch(screen_state_p->generic_state.screen_depth)
			{ 
				case 4:
				case 8:
					if(M64_IS_PROGRAMMABLE_VISUAL(tmp_visual_p))
					{
						generic_state_p->screen_functions_p->si_set_colormap =
							m64_set_colormap_pseudocolor;
					}
					else
					{
						generic_state_p->screen_functions_p->si_set_colormap =
							m64_no_operation_succeed;
					}
					generic_state_p->screen_functions_p->si_get_colormap =
						m64_get_colormap_pseudocolor;
					break;
				case 16:
				case 24:
				case 32:
					if(M64_IS_PROGRAMMABLE_VISUAL(tmp_visual_p))
					{
						generic_state_p->screen_functions_p->si_set_colormap =
							m64_set_colormap_directcolor;
						generic_state_p->screen_functions_p->si_get_colormap =
							m64_get_colormap_directcolor;
					}
					else
					{
						generic_state_p->screen_functions_p->si_set_colormap =
							m64_no_operation_succeed;
						generic_state_p->screen_functions_p->si_get_colormap =
							m64_get_colormap_truecolor;
					}
					break;
			}
		}
		else
		{
#if (defined(__DEBUG__))
			if (m64_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
				"(m64_colormap__initialize__) {\n"
				"\tNo visuals read in.\n"
				"}\n");
			}
#endif
			si_screen_p->flagsPtr->SIvisualCNT = 0;
		}
	}
	else
	{
#if (defined(__DEBUG__))
		if (m64_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
				"(m64_colormap__initialize__) {\n"
				"\tvisuals already read in by board layer.\n}\n");
		}
#endif
	}
}
