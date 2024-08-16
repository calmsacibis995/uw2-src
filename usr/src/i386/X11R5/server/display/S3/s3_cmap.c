/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_cmap.c	1.11"

/***
 ***	NAME
 ***
 ***		s3_cmap.c : colormap handling for the S3 chipset.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s3_cmap.h"
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
export int s3_dac_kind_to_max_clock_table[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	MAXCLOCK
#include "s3_dacs.def"
#undef DEFINE_DAC
};

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s3_colormap_debug = FALSE;
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

#include <memory.h>
#include <sidep.h>
#include "stdenv.h"
#include "s3_regs.h"
#include "g_colormap.h"
#include "g_state.h"
#include "s3_globals.h"
#include "s3_state.h"
#include <string.h>
#include <ctype.h>
/***
 ***	Constants.
 ***/

STATIC const char *const
s3_dac_mode_to_dac_mode_description[] =
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	#DESCRIPTION
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};

STATIC const SIVisual 
s3_visuals_table[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	{\
		 TYPE, DEPTH, N_COLORMAPS, SIZE, N_VALID_BITS,\
		 R_MASK, G_MASK, B_MASK, R_OFFSET, G_OFFSET, B_OFFSET\
	}
#include "s3_vis.def"			
#undef DEFINE_VISUAL	

};

#if (defined(__DEBUG__))
STATIC const char *const s3_visual_kind_to_visual_kind_dump[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
					  #NAME
#include "s3_vis.def"			
#undef DEFINE_VISUAL	
};
#endif	

STATIC const unsigned int s3_dac_kind_to_flags_16_table[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	FLAGS16
#include "s3_dacs.def"
#undef DEFINE_DAC
};

STATIC const unsigned int s3_dac_kind_to_flags_24_table[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	FLAGS24
#include "s3_dacs.def"
#undef DEFINE_DAC
};

STATIC const unsigned int s3_dac_kind_to_flags_rgb_table[] = 
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	FLAGSRGB
#include "s3_dacs.def"
#undef DEFINE_DAC	
};

STATIC const char *const s3_dac_kind_to_dac_name[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESCRIPTION)\
		DESCRIPTION
#include "s3_dacs.def"
#undef DEFINE_DAC
};

#define S3_DAC_STATE_STAMP \
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('D' << 3) + ('A' << 4) +\
	 ('C' << 5) + ('_' << 6) + ('S' << 7) + ('T' << 8) + ('A' << 9) +\
	 ('T' << 10) + ('E' << 11)) 

/***
 ***	Macros.
 ***/
#define S3_COLORMAP_DAC_ACCESS_DELAY()\
{\
	volatile int __count = s3_dac_access_delay_count;\
	while(__count-- > 0)\
	{\
		;\
	}\
}

/*
 * Determining if a visual supports programmable colormaps
 */
#define S3_IS_PROGRAMMABLE_VISUAL(visual_p)\
	((visual_p)->si_visual_p->SVtype & 0x1)

#define BT485_DEFAULT_CLOCK_DOUBLER_THRESHOLD		80000

#define BT485_DEFAULT_DAC_EXTERNAL_SID_THRESHOLD	90000

#define TI_DEFAULT_CLOCK_DOUBLER_THRESHOLD			100000

#define TI_DEFAULT_DAC_EXTERNAL_SID_THRESHOLD		70000

/***
 *** 	Types.
 ***/
struct s3_dac_bt485_state
{
	/*
	 * Space for saving the original state of the dac registers.
	 */
	unsigned char	saved_command_register_0;
	unsigned char	saved_command_register_1;
	unsigned char	saved_command_register_2;
	unsigned char	saved_command_register_3;
	/*
	 * New values that are valid for the X session.
	 */
	unsigned char	command_register_0;
	unsigned char	command_register_1;
	unsigned char	command_register_2;
	unsigned char	command_register_3;
	/*
	 * Whether to use the clock doubler or not?
	 */
	boolean			use_clock_doubler;
	/*
	 * Whether to use the external serial input mode or not?
	 * And in case yes by how much must the horizontal parameters be reduced?
	 */
	boolean			external_sid_mode;

	unsigned char	horizontal_parameters_shift;
#if (defined(__DEBUG__))
	int	stamp;
#endif
};

struct s3_dac_ti_state
{

	/*
	 * Space for saving the original state of the dac registers.
	 */
	unsigned char saved_cursor_control_register;
	unsigned char saved_mux_control_1_register;
	unsigned char saved_mux_control_2_register;
	unsigned char saved_input_clock_select_register;
	unsigned char saved_output_clock_select_register;
	unsigned char saved_general_control_register;
	unsigned char saved_auxiliary_control_register;
	unsigned char saved_general_io_control_register;
	unsigned char saved_general_io_data_register;

	/*
	 * New values that are valid for the X session.
	 */
	unsigned char cursor_control_register;
	unsigned char mux_control_1_register;
	unsigned char mux_control_2_register;
	unsigned char input_clock_select_register;
	unsigned char output_clock_select_register;
	unsigned char general_control_register;
	unsigned char auxiliary_control_register;
	unsigned char general_io_control_register;
	unsigned char general_io_data_register;
	unsigned char sense_test_register ;

	/*
	 * Whether to use the clock doubler or not?
	 */
	boolean use_clock_doubler;
	/*
	 * Whether to use the external serial input mode or not?
	 * And in case yes by how much must the horizontal parameters be reduced?
	 */
	boolean external_sid_mode;
	
	unsigned char	horizontal_parameters_shift;

#if (defined(__DEBUG__))
	int	stamp;
#endif
};
/***
 ***	Variables.
 ***/


/***
 ***	Functions.
 ***/
/*
 * COLORMAP programming.
 */

/*
 * Dac programming functions.
 */
STATIC void
s3_dac_pseudocolor_get_color(const struct generic_visual *visual_p,
							   const int color_index, 
							   unsigned short *rgb_values_p)

{
	/*
	 * program the color index.
	 */
	outb(VGA_DAC_REGISTER_DAC_RD_AD, color_index);
		S3_COLORMAP_DAC_ACCESS_DELAY();

	/*
	 * program R, G, B values.
	 */
	*rgb_values_p++ = inb(VGA_DAC_REGISTER_DAC_DATA);
		S3_COLORMAP_DAC_ACCESS_DELAY();
	*rgb_values_p++ = inb(VGA_DAC_REGISTER_DAC_DATA);
		S3_COLORMAP_DAC_ACCESS_DELAY();
	*rgb_values_p = inb(VGA_DAC_REGISTER_DAC_DATA);
		S3_COLORMAP_DAC_ACCESS_DELAY();

#if (defined(__DEBUG__))
	if (s3_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_dac_pseudocolor_get_color) {\n"
			"\tcolor_index = %d\n"
			"\trgb_values_p = %p\n"
			"\t{\n"
			"\t\tred = 0x%hx\n"
			"\t\tgreen = 0x%hx\n"
			"\t\tblue = 0x%hx\n"
			"\t}\n"
			"}\n",
			  color_index, (void *) rgb_values_p, *rgb_values_p, 
			  *(rgb_values_p + 1), *(rgb_values_p + 2));
	}
#endif
}

STATIC void
s3_dac_pseudocolor_set_color(const struct generic_visual *visual_p,
							   const int color_index,
							   unsigned short *rgb_values_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (s3_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_dac_pseudocolor_set_color) {\n"
			"\tcolor_index = %d\n"
			"\trgb_values_p = %p\n"
			"\t{\n"
			"\t\tred = 0x%hx\n"
			"\t\tgreen = 0x%hx\n"
			"\t\tblue = 0x%hx\n"
			"\t}\n"
			"}\n",
			color_index,
			(void *) rgb_values_p,
			*rgb_values_p, *(rgb_values_p + 1),
			*(rgb_values_p + 2));
	}
#endif

	/*
	 * program color index.
	 */
	outb(VGA_DAC_REGISTER_DAC_WR_AD, color_index);
	S3_COLORMAP_DAC_ACCESS_DELAY();

	/*
	 * program R, G, B values. Do this during a vertical blank period
	 * to avoid any flicker in the screen.
	 */
#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif
	outb(VGA_DAC_REGISTER_DAC_DATA, *rgb_values_p++);
	S3_COLORMAP_DAC_ACCESS_DELAY();
	outb(VGA_DAC_REGISTER_DAC_DATA, *rgb_values_p++);
	S3_COLORMAP_DAC_ACCESS_DELAY();
	outb(VGA_DAC_REGISTER_DAC_DATA, *rgb_values_p);
	S3_COLORMAP_DAC_ACCESS_DELAY();
	
	return;
}


/*
 * PseudoColor operations
 */
/*
 * Setting a colormaps entries. Perform a check for the modifiability
 * of the colormap.
 */
STATIC SIBool
s3_dac_set_colormap_pseudocolor(SIint32 visual_number, 
								  SIint32 colormap_number,
								  SIColor *colors_p, SIint32 count)
{
	
	struct generic_visual *visual_p =
		&(generic_current_screen_state_p->
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
	ASSERT(colormap_number >=0 && colormap_number <
		   si_visual_p->SVcmapcnt);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	if (count <= 0)
	{
		return SI_SUCCEED;
	}
	
	colormap_p =
		&(generic_current_screen_state_p->
		  screen_colormaps_pp[visual_number][colormap_number]);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP,colormap_p));
	
	rgb_p = colormap_p->rgb_values_p;

	dac_set_color_p = visual_p->set_color_method_p;
	
	ASSERT(rgb_p != NULL);
	ASSERT(dac_set_color_p != NULL);
	
#if (defined(__DEBUG__))
	if (s3_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_dac_set_colormap_pseudocolor) {\n"
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
	if (! S3_IS_PROGRAMMABLE_VISUAL(visual_p))
	{
#if (defined(__DEBUG__))
		if (s3_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_colormap_set_colormap_pseudocolor)\n"
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
			if (s3_colormap_debug)
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
s3_dac_get_colormap_pseudocolor(SIint32 visual_number, 
				   SIint32 colormap_number,
				   SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p =
		&(generic_current_screen_state_p->
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
	ASSERT(colormap_number >=0 && colormap_number <
		   si_visual_p->SVcmapcnt);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));

	if (count <= 0)
	{
		return SI_SUCCEED;
	}

	colormap_p = &(generic_current_screen_state_p->
				   screen_colormaps_pp[visual_number][colormap_number]);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));
	
	rgb_p = colormap_p->rgb_values_p;
	dac_get_color_p = visual_p->get_color_method_p;
	
	ASSERT(rgb_p != NULL);
	ASSERT(dac_get_color_p != NULL);
	

#if (defined(__DEBUG__))
	if (s3_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_dac_get_colormap_pseudocolor) {\n"
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
			if (s3_colormap_debug)
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
s3_dac_get_colormap_truecolor(SIint32 visual_number, 
				   SIint32 colormap_number,
				   SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
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

#ifdef DELETE /*this is used for the older servers, ones before sicmap.c fix */
STATIC SIBool
s3_dac_get_colormap_truecolor(SIint32 visual_number, 
				   SIint32 colormap_number,
				   SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		  screen_visuals_list_p[visual_number]);

	SIVisualP si_visual_p = visual_p->si_visual_p;
	
	struct generic_colormap *colormap_p = &(generic_current_screen_state_p->
				   screen_colormaps_pp[visual_number][colormap_number]);

	int	num_red,num_green,num_blue;
	
	ASSERT(visual_number >= 0 && visual_number <
		   generic_current_screen_state_p->screen_number_of_visuals);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL,visual_p));
	ASSERT(colormap_number >=0 && colormap_number <
		   si_visual_p->SVcmapcnt);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));

	if (count <= 0)
	{
		return SI_SUCCEED;
	}
	
	num_red = (si_visual_p->SVredmask >> si_visual_p->SVredoffset) + 1;
	num_green = (si_visual_p->SVgreenmask >> si_visual_p->SVgreenoffset) + 1;
	num_blue = (si_visual_p->SVbluemask >> si_visual_p->SVblueoffset) + 1;
	
	while(count --)
	{
		unsigned int	tmp;

		if (colors_p->SCpindex >= 0 && colors_p->SCpindex < num_red)
		{
			tmp = ((colors_p->SCpindex * 65535) / num_red);
			colors_p->SCred = tmp;
		}
		if (colors_p->SCpindex >= 0 && colors_p->SCpindex < num_green)
		{
			tmp = ((colors_p->SCpindex * 65535) / num_green);
			colors_p->SCgreen = tmp;
		}
		if (colors_p->SCpindex >= 0 && colors_p->SCpindex < num_blue)
		{
			tmp = ((colors_p->SCpindex * 65535) / num_blue);
			colors_p->SCblue = tmp;
		}
		colors_p++;
	}
	return (SI_SUCCEED);
}
#endif

/*
 * Check whether a given DAC can support a display mode.
 * Returns a FALSE if the display mode cannot be supported by the dac.
 * This function is not as simple as it is named. Since this function
 * is called right during the set_mode function in the file s3.c
 * we get a very early chance to initialize the dac state/ and related
 * stuff.
 */
function boolean
s3_dac_check_display_mode_feasibility(struct s3_screen_state
										*screen_state_p)
{
	enum s3_dac_kind dac_kind;
	struct s3_dac_bt485_state	*bt485_state_p;
	struct s3_dac_ti_state		*ti_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) screen_state_p));

#if (defined(__DEBUG__))
	if (s3_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_dac_check_display_mode_feasibility) {\n"
			"\tscreen_state_p = %p\n"
			"}\n",
			(void *) screen_state_p);
	}
#endif
	
	dac_kind = screen_state_p->dac_kind;
	ASSERT(dac_kind >= S3_DAC_NULL && dac_kind < S3_DAC_UNKNOWN);

	/*
	 * Very first thing. Check if the dac can support this serial
	 * clock frequency.
	 */
	if (screen_state_p->clock_frequency >
		s3_dac_kind_to_max_clock_table[dac_kind])
	{
		(void) fprintf(stderr,
		   S3_PIXEL_CLOCK_IS_TOO_HIGH_FOR_DAC_MESSAGE,
		   screen_state_p->clock_frequency / 1000,
		   screen_state_p->clock_frequency % 1000,
		   s3_dac_kind_to_dac_name[dac_kind]);
		/*
		 * Right now we are limiting the max frequency of the 
		 * BT485 to 110000 MHZ. Some cards have BT485's that can
		 * handle 135000 MHZ. Inform the user in such a case.
		 * Move this message to global.h in case this is permenant. 
		 * CALL UP 9GXE/PREMIER and find out if their cards can have
		 * both 110/135 MHZ dacs. If yes case move the message.
		 */
		if (screen_state_p->dac_kind == S3_DAC_BT485 &&
			screen_state_p->options_p->dac_max_frequency ==  
			S3_OPTIONS_DAC_MAX_FREQUENCY_DEFAULT &&
			s3_dac_kind_to_max_clock_table[dac_kind] < 135000 )
		{
			(void) fprintf(stderr,
			LIBRARY_NAME ": If your dac is a BT485KPJ135 increase your dac\n"
			LIBRARY_NAME ": frequency to 135MHZ to get 1280x1024 at higher \n"
			LIBRARY_NAME ": frequencies. Refer /usr/X/lib/LIBS3_OPTIONS file \n"
			LIBRARY_NAME ": for more details on increasing the frequency.\n");
		}
		/*
		 * By default, the maximum frequency of TI3025/3020 is limited to 
		 * 135MHz.
		 * But, according to XFree86, it is possible that this can operate
		 * at even faster clocks up to 200 Mhz. Make sure this and put out
		 * the message so that the user knows that he can increase the max
		 * frequency.
		 *
		 * DO THIS LATER.
		 */
		if ((screen_state_p->dac_kind == S3_DAC_TI3025 ||
		     screen_state_p->dac_kind == S3_DAC_TI3020) &&
			screen_state_p->options_p->dac_max_frequency ==  
			S3_OPTIONS_DAC_MAX_FREQUENCY_DEFAULT)
		{
			(void) fprintf(stderr, "");
		}

		return (FALSE);
	}

	/*
	 * Check if the dac can support the requested depth.
	 * ** basically check if we have the code in place.**
	 */
	switch (dac_kind)
	{
		case S3_DAC_SS2410:	/* Diamond stealth Pro. */
			/*
			 * Diamond stealth pro.
			 */
			if ((screen_state_p->generic_state.screen_depth != 4) &&
				(screen_state_p->generic_state.screen_depth != 8) &&
				(screen_state_p->generic_state.screen_depth != 16))
			{
				(void) fprintf(stderr,
				S3_DAC_UNSUPPORTED_DEPTH_MESSAGE,
				screen_state_p->generic_state.screen_depth,
			   	s3_dac_kind_to_dac_name[dac_kind]);
				return(FALSE);
				/*NOTREACHED*/
			}
			break;
		case S3_DAC_BT485: 	/* 9GXE/ All 4M cards. */
			if ((screen_state_p->generic_state.screen_depth != 4) &&
				(screen_state_p->generic_state.screen_depth != 8) &&
				(screen_state_p->generic_state.screen_depth != 16))
			{
				(void) fprintf(stderr,
				S3_DAC_UNSUPPORTED_DEPTH_MESSAGE,
				screen_state_p->generic_state.screen_depth,
			   	s3_dac_kind_to_dac_name[dac_kind]);
				return(FALSE);
				/*NOTREACHED*/
			}

			/*
			 * Allocate space for the dac state.
			 */
			screen_state_p->dac_state_p = allocate_and_clear_memory(
						sizeof(struct s3_dac_bt485_state));
			bt485_state_p = 
				(struct s3_dac_bt485_state *)screen_state_p->dac_state_p;
			bt485_state_p->use_clock_doubler = FALSE;
			/*
			 *  BT485 is a high performance dac which permits external
			 * 	serial input 32 bits wide. Check if we have to use this.
			 */
			
			bt485_state_p->external_sid_mode = FALSE;
			/*
			 * Check if the external sid mode has been disabled
			 * by the option. If not (auto configure), then proceed
			 * to check the threshold and auto configure this mode
			 * accordingly.
			 *
			 */
			if (screen_state_p->options_p->use_dac_external_sid_mode ==
				S3_OPTIONS_USE_DAC_EXTERNAL_SID_MODE_AUTO_CONFIGURE)
			{
				int  dac_external_sid_threshold = 
								BT485_DEFAULT_DAC_EXTERNAL_SID_THRESHOLD ;
			
				if (screen_state_p->options_p->dac_external_sid_threshold !=
								S3_OPTIONS_DAC_EXTERNAL_SID_THRESHOLD_DEFAULT)
				{
					dac_external_sid_threshold =
						screen_state_p->options_p->dac_external_sid_threshold ; 
				}
				/*
				 * FIX THIS LATER. For now all 16/24 bit modes have 
				 * external sid enabled.
				 */
				if (screen_state_p->generic_state.screen_depth > 8 ||
				   screen_state_p->clock_frequency > dac_external_sid_threshold)
				{
					bt485_state_p->external_sid_mode = TRUE;
				}
			}

			S3_UNLOCK_S3_VGA_REGISTERS();
			S3_UNLOCK_SYSTEM_REGISTERS();
			BT485_SELECT_REGISTER_SET(2);
			bt485_state_p->saved_command_register_1 = inb(BT485_COMMAND_REG_1);
			bt485_state_p->saved_command_register_2 = inb(BT485_COMMAND_REG_2);
			BT485_SELECT_REGISTER_SET(1);
			bt485_state_p->saved_command_register_0 = inb(BT485_COMMAND_REG_0);
			/* Begin the complex sequence to read in command register 3 */
			outb(BT485_COMMAND_REG_0, 
					bt485_state_p->saved_command_register_0 |
					BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK);
			BT485_SELECT_REGISTER_SET(0);
			outb(BT485_WRITE_ADDR, 0x01);
			BT485_SELECT_REGISTER_SET(2);
			bt485_state_p->saved_command_register_3 = inb(BT485_STATUS_REG);
			/* End complex sequence. */
			S3_LOCK_SYSTEM_REGISTERS();
			S3_LOCK_S3_VGA_REGISTERS();

			/*
			 * Initialize the BT485 Command Registers 0,1 and 2.
			 */
			bt485_state_p->command_register_0 = 
				bt485_state_p->saved_command_register_0 & 
				~BT485_COMMAND_REG_0_RESOLUTION_8BIT; 
			bt485_state_p->command_register_0 |= 
				(screen_state_p->options_p->dac_rgb_width ==
					S3_OPTIONS_DAC_RGB_WIDTH_8 ?
					BT485_COMMAND_REG_0_RESOLUTION_8BIT : 0);
			bt485_state_p->command_register_1 = 0;
			bt485_state_p->command_register_2 =  
				bt485_state_p->saved_command_register_2 &
				~BT485_COMMAND_REG_2_DISPLAY_MODE_INTERLACED;

#ifdef DELETE 
			/* 
			 * Looks like required only for bt485 h/w cursor. 
			 * Note: interlace_retrace_start = 0 means NI mode.
			 */
			if (screen_state_p->register_state.s3_vga_registers.
				interlace_retrace_start)
			{
				bt485_state_p->command_register_2 |= 
				BT485_COMMAND_REG_2_DISPLAY_MODE_INTERLACED;
			}
#endif

			if(bt485_state_p->external_sid_mode == TRUE)
			{
				unsigned int	tmp,tmp1;
				unsigned int	syncwidth;

				screen_state_p->register_state.s3_system_extension_registers.
					extended_dac_control |= EX_DAC_CT_ENB_SID;
				/*
				 * Looks like we have to do some extra stuff if this is
				 * enabled to get proper sync.
				 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_dac_control |= EX_DAC_CT_HWGC_EXOP;
				screen_state_p->register_state.s3_system_control_registers.
					hw_cursor_mode	 |= HGC_MODE_ENB_485;

				bt485_state_p->command_register_2 |=  (
					BT485_COMMAND_REG_2_PORTSEL_UNMASK |
					BT485_COMMAND_REG_2_CLKSEL_PCLK1);

				switch (screen_state_p->generic_state.screen_depth)
				{
					case 4: /* 8:1 Mux */
						bt485_state_p->horizontal_parameters_shift = 3U;
						bt485_state_p->command_register_1 |= 
							BT485_COMMAND_REG_1_PIXEL_SELECT_4;
						break;
					case 8: /* 4:1 Mux */
						bt485_state_p->horizontal_parameters_shift = 2U;
						bt485_state_p->command_register_1 |= 
							BT485_COMMAND_REG_1_PIXEL_SELECT_8;
						break;
					case 16: /* 2:1 Mux */
						bt485_state_p->horizontal_parameters_shift = 1U;
						bt485_state_p->command_register_1 |= (
							BT485_COMMAND_REG_1_PIXEL_SELECT_16 |
							BT485_COMMAND_REG_1_TRUE_COLOR_BYPASS |
							BT485_COMMAND_REG_1_16_BIT_MUX_2_IS_1 |
							(screen_state_p->options_p->dac_16_bit_color_mode 
								== S3_OPTIONS_DAC_16_BIT_COLOR_MODE_565 ?
								BT485_COMMAND_REG_1_16_BIT_FORMAT_565 : 0));
						break;
					case 24: /* 1:1 Mux */
						bt485_state_p->horizontal_parameters_shift = 0U;
						bt485_state_p->command_register_1 |= (
							BT485_COMMAND_REG_1_PIXEL_SELECT_24 |
							BT485_COMMAND_REG_1_TRUE_COLOR_BYPASS);
						break;
					default:
						/*CONSTANTCONDITION*/
						ASSERT(0);
						break;
				}
				/*
				 * In case of Premier928 4M card disable split transfers 
				 * in 16bit mode to work around the extra SC clock bug. 
				 * No idea as to what to do in case of E stepping chips. 
				 * The technote says that this bug has been fixed in E steps.
				 */
				if (strcmp(screen_state_p->generic_state.screen_config_p->model,
					"Premier928_4M") == 0)
				{
					if (screen_state_p->generic_state.screen_depth == 16 && 
						screen_state_p->chipset_step ==  S3_STEP_KIND_D_STEP)
					{
						screen_state_p->register_state.
						s3_system_extension_registers.
						extended_system_control_2 |= EX_SCTL_2_DIS_SPXF;
					}
				}
				/*
				 * Adjust the horizontal parameters appropriately.
				 * basically divide it by the mux rate, since SCLK is 
				 * also divided by mux rate now. Note: This logic will 
				 * not work for 24 bit mode since shift is 0.
				 */
				/* Htotal and H-display-end registers.*/
				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.h_total;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x01)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x01U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_total = 
					(tmp - (5<<bt485_state_p->horizontal_parameters_shift) + 5)
					>>bt485_state_p->horizontal_parameters_shift;

				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.h_d_end;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x02)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x02U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_d_end = 
					(tmp - (1<<bt485_state_p->horizontal_parameters_shift) + 1)
					>>bt485_state_p->horizontal_parameters_shift;

				/* Start and End H-Blank registers. */
				tmp = (unsigned int)screen_state_p->register_state.
				standard_vga_registers.standard_vga_crtc_registers.s_h_blank;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x04)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x04U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.s_h_blank = tmp >>
					bt485_state_p->horizontal_parameters_shift;

				tmp = (unsigned int)screen_state_p->register_state.
				standard_vga_registers.standard_vga_crtc_registers.e_h_blank;
				tmp1  = tmp & 0x1F;
				tmp &= 0xE0;
				if (screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_sy_p & 0x80)
				{
					screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_sy_p &= ~0x80U;
					tmp1 |= 0x20U;
				}
				tmp |= (tmp1 >> bt485_state_p->horizontal_parameters_shift);
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_blank = tmp;

				/* Start and End H-Sync registers. */
				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.s_h_sy_p;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x10)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x10U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.s_h_sy_p = tmp >>
					bt485_state_p->horizontal_parameters_shift;
				syncwidth = tmp & 0x1F;

				/* 
				 * The following computation which seems to be right does
				 * not work with chipsets which have the SC problem.
				 * i.e., the 928 D Step chipsets, namely the 9GXE.
				 */
				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.
					e_h_sy_p;
				tmp1  = tmp & 0x1F;
				tmp &= 0xE0;
				if ( tmp1 > syncwidth)
				{
					syncwidth = tmp1 - syncwidth;
				}
				else
				{
					syncwidth = (tmp1 | 0x20) - syncwidth; 
				} 
				syncwidth >>= bt485_state_p->horizontal_parameters_shift;
				tmp |= (unsigned int)(screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.
					s_h_sy_p + syncwidth) & 0x1F;

				if (strcmp(screen_state_p->generic_state.screen_config_p->model,
					"9GXE") == 0)
				{
					tmp = screen_state_p->register_state.standard_vga_registers.
						standard_vga_crtc_registers.h_total & 0x1F;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_sy_p = tmp;
			}
			/*
			 * BT485 Has an internal clock doubler which should be used for 
			 * frequencies above 80 (67.5?) Mhz of dot clock frequencies. 
			 * In these cases the clock frequency has to be divided by 2. 
			 * Ignore the clock doubler in case the option specifies 'no'.
			 * The default is to auto configure.
			 */
			if (screen_state_p->options_p->use_clock_doubler == 
					S3_OPTIONS_USE_CLOCK_DOUBLER_AUTO_CONFIGURE)
			{
				int  clock_doubler_threshold =
									BT485_DEFAULT_CLOCK_DOUBLER_THRESHOLD ;

				if (S3_OPTIONS_CLOCK_DOUBLER_THRESHOLD_DEFAULT != 
					screen_state_p->options_p->clock_doubler_threshold)
				{
					(void) fprintf(stderr,
						S3_DAC_BT485_THRESHOLD_CHANGE_WARNING_MESSAGE,
						BT485_DEFAULT_CLOCK_DOUBLER_THRESHOLD/1000,
						BT485_DEFAULT_CLOCK_DOUBLER_THRESHOLD%1000,
						screen_state_p->options_p->
							clock_doubler_threshold/1000,
						screen_state_p->options_p->
							clock_doubler_threshold%1000);

					clock_doubler_threshold =
						screen_state_p->options_p->clock_doubler_threshold;
				}

				if (screen_state_p->clock_frequency > 
					clock_doubler_threshold)
				{
					int clock_count = 0;

					screen_state_p->clock_frequency = 
						screen_state_p->clock_frequency >>  1;
					bt485_state_p->use_clock_doubler = TRUE;

					/*
					 * See the file s3.c get_mode function for an explanation .
					 */
					if (screen_state_p->clock_chip_p-> 
						number_of_clock_frequencies == 0)
					{
#define MIN_CLOCK_FREQUENCY_BOUND	0
#define MAX_CLOCK_FREQUENCY_BOUND	1

						const int min = screen_state_p->clock_chip_p->
							clock_frequency_table[MIN_CLOCK_FREQUENCY_BOUND];
						const int max = screen_state_p->clock_chip_p->
							clock_frequency_table[MAX_CLOCK_FREQUENCY_BOUND];

						/*
						 * Check if the synthesizer can generate the 
						 * requested freq.
						 */
						if ((screen_state_p->clock_frequency >= min ) && 
							(screen_state_p->clock_frequency <= max))
						{
							clock_count++;
						}
#undef MIN_CLOCK_FREQUENCY_BOUND
#undef MAX_CLOCK_FREQUENCY_BOUND
					}
					else
					{
						for (; clock_count <
						 screen_state_p->clock_chip_p->number_of_clock_frequencies &&
						 screen_state_p->clock_chip_p->
							clock_frequency_table[clock_count] !=
							screen_state_p->clock_frequency; ++clock_count)
						{
							;
						}
					}

					if (clock_count == screen_state_p->clock_chip_p->
										number_of_clock_frequencies)
					{
						/*
						 * Clock chip does not support the desired frequency.
						 */
						(void) fprintf(stderr,
							S3_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND_MESSAGE);
						return(FALSE);
					}
				}
			}
			STAMP_OBJECT(S3_DAC_STATE,bt485_state_p);
			break;

		case S3_DAC_TI3025: /* Dac on #9GXE 64pro */
		case S3_DAC_TI3020:
			if ((screen_state_p->generic_state.screen_depth != 4) &&
				(screen_state_p->generic_state.screen_depth != 8) &&
				(screen_state_p->generic_state.screen_depth != 16))
			{
				(void) fprintf(stderr,
				S3_DAC_UNSUPPORTED_DEPTH_MESSAGE,
				screen_state_p->generic_state.screen_depth,
			   	s3_dac_kind_to_dac_name[dac_kind]);
				return(FALSE);
				/*NOTREACHED*/
			}

			/*
			 * Allocate space for the dac state.
			 */
			screen_state_p->dac_state_p = allocate_and_clear_memory(
						sizeof(struct s3_dac_ti_state));
			ti_state_p = 
				(struct s3_dac_ti_state *)screen_state_p->dac_state_p;
			ti_state_p->use_clock_doubler = FALSE;
			/*
			 *  TI3025/3020 are high performance dacs which permit external
			 * 	serial input 32/64 bits wide. Check if we have to use this.
			 */
			
			ti_state_p->external_sid_mode = FALSE;
			/*
			 * Check if the external sid mode has been disabled
			 * by the option. If not (auto configure), then proceed
			 * to check the threshold and auto configure this mode
			 * accordingly.
			 *
			 */
			if (screen_state_p->options_p->use_dac_external_sid_mode ==
				S3_OPTIONS_USE_DAC_EXTERNAL_SID_MODE_AUTO_CONFIGURE)
			{
				int  dac_external_sid_threshold = 
								TI_DEFAULT_DAC_EXTERNAL_SID_THRESHOLD ;
			
				if (screen_state_p->options_p->dac_external_sid_threshold !=
								S3_OPTIONS_DAC_EXTERNAL_SID_THRESHOLD_DEFAULT)
				{
					dac_external_sid_threshold =
						screen_state_p->options_p->dac_external_sid_threshold ; 
				}
				/*
				 * FIX THIS LATER. For now all 16/24 bit modes have 
				 * external sid enabled.
				 */
				if (screen_state_p->generic_state.screen_depth > 8 ||
				   screen_state_p->clock_frequency > dac_external_sid_threshold)
				{
					ti_state_p->external_sid_mode = TRUE;
				}
			}

			S3_UNLOCK_S3_VGA_REGISTERS();
			S3_UNLOCK_SYSTEM_REGISTERS();
			/*
			 * Save TI DAC registers.
			 */
			S3_READ_TI_DAC_REGISTER(TI_CURS_CONTROL, ti_state_p->
											saved_cursor_control_register);
			S3_READ_TI_DAC_REGISTER(TI_MUX_CONTROL_1, ti_state_p->
											saved_mux_control_1_register);
			S3_READ_TI_DAC_REGISTER(TI_MUX_CONTROL_2, ti_state_p->
											saved_mux_control_2_register);
			S3_READ_TI_DAC_REGISTER(TI_INPUT_CLOCK_SELECT, ti_state_p->
											saved_input_clock_select_register);
			S3_READ_TI_DAC_REGISTER(TI_OUTPUT_CLOCK_SELECT, ti_state_p->
											saved_output_clock_select_register);
			S3_READ_TI_DAC_REGISTER(TI_GENERAL_CONTROL, ti_state_p->
											saved_general_control_register);
			S3_READ_TI_DAC_REGISTER(TI_AUXILIARY_CONTROL, ti_state_p->
											saved_auxiliary_control_register);
			S3_READ_TI_DAC_REGISTER(TI_GENERAL_IO_CONTROL, ti_state_p->
											saved_general_io_control_register);
			S3_READ_TI_DAC_REGISTER(TI_GENERAL_IO_DATA, ti_state_p->
											saved_general_io_data_register);
			S3_LOCK_SYSTEM_REGISTERS();
			S3_LOCK_S3_VGA_REGISTERS();

			/*
			 * Initialize TI dac registers.
			 */
			{
			unsigned char tmp_misc_out;

			tmp_misc_out = screen_state_p->register_state.
			standard_vga_registers.standard_vga_general_registers.misc_out ;

			screen_state_p->register_state.standard_vga_registers.
        	standard_vga_general_registers.misc_out |= 0xC0 ;

			/* 
			 * invert bits for the 3020/3025 
			 */
			ti_state_p->general_control_register = 0x00;
      		if (!(tmp_misc_out & 0x80)) 
			{
				ti_state_p->general_control_register |= 0x02; 
			}
      		if (!(tmp_misc_out & 0x40)) 
			{
				ti_state_p->general_control_register |= 0x01;
			}
			}

			if (ti_state_p->external_sid_mode == TRUE)
			{
				unsigned int    tmp,tmp1;
				unsigned int    syncwidth;

				screen_state_p->register_state.s3_system_extension_registers.
					extended_mem_control_1 |= EX_MCTL_1_PAR_VRAM ;

				screen_state_p->register_state.s3_system_extension_registers.
                    extended_dac_control |= EX_DAC_CT_ENB_SID;

				/* 
				 * set aux control to self clocked, window function complement 
				 */
				ti_state_p->auxiliary_control_register = TI_AUX_SELF_CLOCK | 
															TI_AUX_W_CMPL ;
				switch (screen_state_p->generic_state.screen_depth)
				{
					case 4 :
						break;

					case 8 : /* 8:1 MUX */

						ti_state_p->horizontal_parameters_shift = 3U ;

				/*
				 * set output clocking to VCLK/4, RCLK/8 like the fixed Bt485.
				 * RCLK/8 is used because of the 8:1 pixel-multiplexing below.
				 * the RCLK output is tied to the LCLK input which is the same
				 * as SCLK but with no blanking.  SCLK is the actual pixel
				 * shift clock for the pixel bus.
				 */
						ti_state_p->output_clock_select_register = 
															TI_OCLK_S_V4_R8 ;
						/* 
						 * set mux control 1 and 2 to provide pseudocolor 
						 * sub-mode 4   
						 */
						/* 
						 * this provides a 64-bit pixel bus with 8:1 
						 * multiplexing      
						 */
						ti_state_p->mux_control_1_register = 
														TI_MUX1_PSEUDO_COLOR ;
						ti_state_p->mux_control_2_register = TI_MUX2_BUS_PIX64 ;
						break;

					case 16 :
						break;

					default :
                        /*CONSTANTCONDITION*/
                        ASSERT(0);
                        break;
				}
				/*
				 * Adjust the horizontal parameters appropriately.
				 * basically divide it by the mux rate, since SCLK is 
				 * also divided by mux rate now. Note: This logic will 
				 * not work for 24 bit mode since shift is 0.
				 */
				/* Htotal and H-display-end registers.*/
				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.h_total;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x01)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x01U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_total = 
					(tmp - (5<<ti_state_p->horizontal_parameters_shift) + 5)
					>>ti_state_p->horizontal_parameters_shift;

				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.h_d_end;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x02)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x02U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.h_d_end = 
					(tmp - (1<<ti_state_p->horizontal_parameters_shift) + 1)
					>>ti_state_p->horizontal_parameters_shift;

				/* Start and End H-Blank registers. */
				tmp = (unsigned int)screen_state_p->register_state.
				standard_vga_registers.standard_vga_crtc_registers.s_h_blank;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x04)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x04U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.s_h_blank = tmp >>
					ti_state_p->horizontal_parameters_shift;

				tmp = (unsigned int)screen_state_p->register_state.
				standard_vga_registers.standard_vga_crtc_registers.e_h_blank;
				tmp1  = tmp & 0x1F;
				tmp &= 0xE0;
				if (screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_sy_p & 0x80)
				{
					screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_sy_p &= ~0x80U;
					tmp1 |= 0x20U;
				}
				tmp |= (tmp1 >> ti_state_p->horizontal_parameters_shift);
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_blank = tmp;

				/* Start and End H-Sync registers. */
				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.s_h_sy_p;
				if (screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl & 0x10)
				{
				 	screen_state_p->register_state.
					s3_system_extension_registers.extended_horz_ovfl &= ~0x10U;
					tmp |= 0x100U;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.s_h_sy_p = tmp >>
					ti_state_p->horizontal_parameters_shift;
				syncwidth = tmp & 0x1F;

				/* 
				 * The following computation which seems to be right does
				 * not work with chipsets which have the SC problem.
				 * i.e., the 928 D Step chipsets, namely the 9GXE.
				 */
				tmp = (unsigned int)screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.
					e_h_sy_p;
				tmp1  = tmp & 0x1F;
				tmp &= 0xE0;
				if ( tmp1 > syncwidth)
				{
					syncwidth = tmp1 - syncwidth;
				}
				else
				{
					syncwidth = (tmp1 | 0x20) - syncwidth; 
				} 
				syncwidth >>= ti_state_p->horizontal_parameters_shift;
				tmp |= (unsigned int)(screen_state_p->register_state.
					standard_vga_registers.standard_vga_crtc_registers.
					s_h_sy_p + syncwidth) & 0x1F;

				if (strcmp(screen_state_p->generic_state.screen_config_p->model,
					"9GXE") == 0)
				{
					tmp = screen_state_p->register_state.standard_vga_registers.
						standard_vga_crtc_registers.h_total & 0x1F;
				}
				screen_state_p->register_state.standard_vga_registers.
					standard_vga_crtc_registers.e_h_sy_p = tmp;
			}
			else
			{
				screen_state_p->register_state.s3_system_extension_registers.
                    extended_mem_control_1 &= ~EX_MCTL_1_PAR_VRAM ;

                screen_state_p->register_state.s3_system_extension_registers.
                    extended_dac_control &= ~EX_DAC_CT_ENB_SID;
				/* 
				 * set aux control to self clocked only                        
				 */
				ti_state_p->auxiliary_control_register = TI_AUX_SELF_CLOCK ;

				/*
				 * set output clocking to default of VGA.
				 */
				ti_state_p->output_clock_select_register = TI_OCLK_VGA ;

				/* 
				 * set mux control 1 and 2 to provide pseudocolor VGA          
			 	 */
				ti_state_p->mux_control_1_register = TI_MUX1_PSEUDO_COLOR ;
				ti_state_p->mux_control_2_register = TI_MUX2_BUS_VGA ;

			}

			/* 
			 * change to 8-bit DAC and re-route the data path and clocking 
			 */
			ti_state_p->general_io_control_register = TI_GIC_ALL_BITS ;

			if (screen_state_p->options_p->dac_rgb_width == 8)
				ti_state_p->general_io_data_register = TI_GID_TI_DAC_8BIT ;
			else
				ti_state_p->general_io_data_register = TI_GID_TI_DAC_6BIT ;

      		/* 
			 * for some reason the bios doesn't set this properly          
			 */
			ti_state_p->sense_test_register = 0;

			ti_state_p->input_clock_select_register = TI_ICLK_CLK1 ;
			/*
			 * TI3025/3020 have an internal clock doubler which should be 
			 * used for 
			 * frequencies above 100 Mhz of dot clock frequencies. 
			 * In these cases the clock frequency has to be divided by 2. 
			 * Ignore the clock doubler in case the option specifies 'no'.
			 * The default is to auto configure.
			 */
			if (screen_state_p->options_p->use_clock_doubler == 
					S3_OPTIONS_USE_CLOCK_DOUBLER_AUTO_CONFIGURE)
			{
				int  clock_doubler_threshold =
									TI_DEFAULT_CLOCK_DOUBLER_THRESHOLD ;

				if (S3_OPTIONS_CLOCK_DOUBLER_THRESHOLD_DEFAULT != 
					screen_state_p->options_p->clock_doubler_threshold)
				{
					(void) fprintf(stderr,
						S3_DAC_TI_THRESHOLD_CHANGE_WARNING_MESSAGE,
						TI_DEFAULT_CLOCK_DOUBLER_THRESHOLD/1000,
						TI_DEFAULT_CLOCK_DOUBLER_THRESHOLD%1000,
						screen_state_p->options_p->
							clock_doubler_threshold/1000,
						screen_state_p->options_p->
							clock_doubler_threshold%1000);

					clock_doubler_threshold =
						screen_state_p->options_p->clock_doubler_threshold;
				}

				if (screen_state_p->clock_frequency > 
					clock_doubler_threshold)
				{
					int clock_count = 0;

					screen_state_p->clock_frequency = 
						screen_state_p->clock_frequency >>  1;
					ti_state_p->use_clock_doubler = TRUE;
					ti_state_p->input_clock_select_register = 
														TI_ICLK_CLK1_DOUBLE ;
					/*
					 * See the file s3.c get_mode function for an explanation .
					 */
					if (screen_state_p->clock_chip_p-> 
						number_of_clock_frequencies == 0)
					{
#define MIN_CLOCK_FREQUENCY_BOUND	0
#define MAX_CLOCK_FREQUENCY_BOUND	1

						const int min = screen_state_p->clock_chip_p->
							clock_frequency_table[MIN_CLOCK_FREQUENCY_BOUND];
						const int max = screen_state_p->clock_chip_p->
							clock_frequency_table[MAX_CLOCK_FREQUENCY_BOUND];

						/*
						 * Check if the synthesizer can generate the 
						 * requested freq.
						 */
						if ((screen_state_p->clock_frequency >= min ) && 
							(screen_state_p->clock_frequency <= max))
						{
							clock_count++;
						}
#undef MIN_CLOCK_FREQUENCY_BOUND
#undef MAX_CLOCK_FREQUENCY_BOUND
					}
					else
					{
						for (; clock_count <
						 screen_state_p->clock_chip_p->number_of_clock_frequencies &&
						 screen_state_p->clock_chip_p->
							clock_frequency_table[clock_count] !=
							screen_state_p->clock_frequency; ++clock_count)
						{
							;
						}
					}

					if (clock_count == screen_state_p->clock_chip_p->
										number_of_clock_frequencies)
					{
						/*
						 * Clock chip does not support the desired frequency.
						 */
						(void) fprintf(stderr,
							S3_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND_MESSAGE);
						return(FALSE);
					}
				}
			}
			STAMP_OBJECT(S3_DAC_STATE,ti_state_p);
			break;
		default:
			/* 
			 * No support for depths other than 8/4 bpp on generic dacs.
			 */
			if ((screen_state_p->generic_state.screen_depth != 4) &&
				(screen_state_p->generic_state.screen_depth != 8))
			{
				(void) fprintf(stderr,
				S3_DAC_UNSUPPORTED_DEPTH_MESSAGE,
				screen_state_p->generic_state.screen_depth,
			   	s3_dac_kind_to_dac_name[dac_kind]);
				return(FALSE);
				/*NOTREACHED*/
			}


			break;
	}


	return(TRUE);
}

STATIC int
s3_dac_initialize_unknown_dac(struct generic_screen_state *generic_state_p)
{
#if (defined(__DEBUG__))
	if(s3_colormap_debug)
	{
		(void) fprintf(debug_stream_p,"Initializing dac of unknown type.\n");
	}
#endif
	return(1);
}

STATIC int
s3_dac_uninitialize_unknown_dac(struct generic_screen_state *generic_state_p)
{
#if (defined(__DEBUG__))
	if(s3_colormap_debug)
	{
		(void) fprintf(debug_stream_p,"Uninitializing dac of unknown type.\n");
	}
#endif
	return(1);
}

/*
 *  Dacs that dont need any special initialization.
 *  Mostly pseudo color dacs for 8/4 bit modes.
 *  ASSUMPTION: Default visual is the very first one in the list.
 *  In case this assumption changes fix it by passing it down to 
 *  the function.
 */
STATIC int
s3_dac_initialize_generic_dac(struct generic_screen_state *generic_state_p)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		screen_visuals_list_p[0]);
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;
	
	unsigned int dac_mask = 0;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	/*
	 * Initialize the dac according to the default visual (0)
	 */
	if ((generic_state_p->screen_depth == 4) ||
		(generic_state_p->screen_depth == 8 ))
	{
		dac_mask = (1U << (unsigned)generic_state_p->screen_depth) - 1;
		outb(VGA_DAC_REGISTER_DAC_AD_MK,dac_mask);
		if (S3_IS_PROGRAMMABLE_VISUAL(visual_p))
		{
			generic_state_p->screen_functions_p->si_set_colormap =
				s3_dac_set_colormap_pseudocolor;
		}
		else
		{
			generic_state_p->screen_functions_p->si_set_colormap =
				s3_no_operation_succeed;
		}
		generic_state_p->screen_functions_p->si_get_colormap =
			s3_dac_get_colormap_pseudocolor;
	}
	else
	{
		ASSERT (!(S3_IS_PROGRAMMABLE_VISUAL(visual_p)));
	}
	return 1;
}

/*
 * Dacs for 8/4 bit pseudo color modes.
 */
STATIC int
s3_dac_uninitialize_generic_dac(struct generic_screen_state *generic_state_p)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	/*
	 * Uninitialize the dac.
	 */

	/*
	 * Restore the dac mask.
	 */
	outb(VGA_DAC_REGISTER_DAC_AD_MK,
		 screen_state_p->register_state.saved_standard_vga_registers.
		 standard_vga_dac_registers.dac_mask);
	return 1;
}

/*
 *  Dac on the Diamond Stealth Pro board.
 *  ASSUMPTION: Default visual is the very first one in the list.
 *  In case this assumption changes fix it by passing it down to 
 *  the function.
 */
STATIC int
s3_dac_initialize_ss2410_dac(struct generic_screen_state *generic_state_p)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		screen_visuals_list_p[0]);
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	switch (visual_p->si_visual_p->SVtype)
	{
		case TRUECOLOR_AVAIL:
			/* 
			 * 16 bit color for now.
			 */
			S3_UNLOCK_S3_VGA_REGISTERS();
			S3_UNLOCK_SYSTEM_REGISTERS();

			/*
			 * Select the command register, select appropriate bits per
			 * pixel and reset the RS3:2 bits.
			 */
			S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
				(screen_state_p->register_state.s3_system_extension_registers.
				extended_dac_control & 0xBC) | 1);
			if (screen_state_p->generic_state.screen_depth == 16 )
			{
				switch (screen_state_p->options_p->dac_16_bit_color_mode)
				{
					case S3_OPTIONS_DAC_16_BIT_COLOR_MODE_555:
						outb(VGA_DAC_REGISTER_DAC_AD_MK,0xa0);
					break;
					case S3_OPTIONS_DAC_16_BIT_COLOR_MODE_565:
						outb(VGA_DAC_REGISTER_DAC_AD_MK,0xa6);
					break;
				}
			}
			else if (screen_state_p->generic_state.screen_depth == 24 )
			{
						outb(VGA_DAC_REGISTER_DAC_AD_MK,0x9e);
			}
			S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
				screen_state_p->register_state.s3_system_extension_registers.
				extended_dac_control & 0xBF);
			outb(VGA_DAC_REGISTER_DAC_AD_MK,0xFF);

			S3_LOCK_SYSTEM_REGISTERS();
			S3_LOCK_S3_VGA_REGISTERS();
			/* 
			 * assume true color for all other depths.
			 */
			generic_state_p->screen_functions_p->si_get_colormap =
				s3_dac_get_colormap_truecolor;
			generic_state_p->screen_functions_p->si_set_colormap =
				s3_no_operation_succeed;
				break;
		default:
			return (s3_dac_initialize_generic_dac(generic_state_p));
			/*NOTREACHED*/
			break;
	}

	return 1;
}

/*
 *  ASSUMPTION: Default visual is the very first one in the list.
 *  In case this assumption changes fix it by passing it down to 
 *  the function.
 */
STATIC int
s3_dac_uninitialize_ss2410_dac(struct generic_screen_state *generic_state_p)
{
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		screen_visuals_list_p[0]);
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	/*
	 * Uninitialize the dac.
	 */
	switch (visual_p->si_visual_p->SVtype)
	{
		case TRUECOLOR_AVAIL:
			/* 
			 * 16 bit color for now.
			 */
			S3_UNLOCK_S3_VGA_REGISTERS();
			S3_UNLOCK_SYSTEM_REGISTERS();

			/*
			 * Select the command register, select pseudo color
			 * mode and reset the RS3:2 bits and dac mask.
			 */
			S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
				(screen_state_p->register_state.s3_system_extension_registers.
				extended_dac_control & 0xBC) | 1);
			outb(VGA_DAC_REGISTER_DAC_AD_MK,0x0);
			S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
				screen_state_p->register_state.s3_system_extension_registers.
				extended_dac_control & 0xBF);
			outb(VGA_DAC_REGISTER_DAC_AD_MK,
				 screen_state_p->register_state.saved_standard_vga_registers.
				 standard_vga_dac_registers.dac_mask);

			S3_LOCK_SYSTEM_REGISTERS();
			S3_LOCK_S3_VGA_REGISTERS();
			break;
		default:
			return(s3_dac_uninitialize_generic_dac(generic_state_p));
			/*NOTREACHED*/
			break;
	}
	return 1;
}

/*
 * Comments on Bt485 programming :
 *
 * (A) Below 80MHz programming.
 *    ~~~~~~~~~~~~~~~~~~~~~~~~
 *	1. We can use either Pixel port select or VGA port select. Power on
 * 	default value in CR25 (PORTSEL mask) is 0, which selects the 8-bit VGA
 *	port. In case we select pixel port, then we have to worry about
 *	selecting 8:1, 4:1, 2:1 or 1:1 MUX for 4, 8, 16-24 bit operations
 *	respectively.
 *
 *	2. Power on default value for CR14 is 0, which enables the 256 color
 *	pixel palette addressing by the pixel data which can be coming either
 *  from Pixel port or vga port.
 *
 *	3. Power on default value for CR24 is 0, which selects PCLK0.
 *	
 *	Beacuse of above three power on default values, we only have to
 *  program the Pixel mask register to either 0x0f or 0xff for 
 *	4 and 8 bit operations respectively. In case we decide to
 *	go for 16/24 bit operations, bt485 init issues will increase.
 *
 * (B) Above 80MHz operation :
 *   ~~~~~~~~~~~~~~~~~~~~~~~~~
 *  We have to select clock doubler.
 *
 */
STATIC int
s3_dac_initialize_bt485_dac(struct generic_screen_state *generic_state_p)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;
	struct s3_dac_bt485_state	*bt485_state_p = 
			(struct s3_dac_bt485_state *)screen_state_p->dac_state_p;
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		screen_visuals_list_p[0]);

	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_DAC_STATE, bt485_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	/*
	 * Choose the dac chip internal clock doubler in case of BT485 if the
	 * pixel clock frequency is above 80 MHZ.
	 */
	if ( bt485_state_p->use_clock_doubler == TRUE )
	{
#if (defined(__DEBUG__))
		if (s3_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_dac_initialize_bt485_dac){\n"
				"\tEnabling the internal clock doubler...\n"
				"}\n");
		}
#endif
		bt485_state_p->command_register_3 = 
			bt485_state_p->saved_command_register_3 | 
			BT485_COMMAND_REG_3_2X_CLOCK_MULTIPLY;
		/* Mask off reserved bits. */
		bt485_state_p->command_register_3  &= 0x0F;
	} 
	/*
	 * First Program the BT485 Command Registers.
	 */
	S3_TURN_SCREEN_OFF();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();
	if (bt485_state_p->external_sid_mode == TRUE)
	{
		volatile int k = s3_graphics_engine_loop_timeout_count;
		/*
		 * Put BT485 in sleep mode before changing pclks.
		 */
		BT485_SELECT_REGISTER_SET(1);
		outb(BT485_COMMAND_REG_0, bt485_state_p->saved_command_register_0 |
									BT485_COMMAND_REG_0_SLEEP_MODE);
		while (k--);	/* Wait for a couple of seconds. */
		BT485_SELECT_REGISTER_SET(2);
		outb(BT485_COMMAND_REG_1,bt485_state_p->command_register_1); 
		outb(BT485_COMMAND_REG_2,bt485_state_p->command_register_2); 
		/*
		 * Workaround for 928 Hardware BUG:
		 * Workaround for the Number 9 GXE cards with BT485  in muxmode.
		 * For D-Stepping set GEN OUT PORT bit 5 to 1.
		 * For E-Step OR G-Step set crtc register 65 bit 5 to 1.
		 * 	E-Step||G-Step && PCI, Reg 65 |= 0x41
		 * 	E-Step||G-Step && !PCI, Reg 65 |= 0x21
		 */
		if (strcmp(screen_state_p->generic_state.screen_config_p->model,
			"9GXE") == 0)
		{
			unsigned char tmp;
			if (screen_state_p->chipset_step ==  S3_STEP_KIND_D_STEP)
			{
				S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
				tmp |= 0x20;
				S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
			}
			else if ( (screen_state_p->chipset_step ==  S3_STEP_KIND_E_STEP)
				|| (screen_state_p->chipset_step ==  S3_STEP_KIND_G_STEP) )
			{
				/* !PCI && (E-Step||G-Step) && 9GXE */
				int val = 0x21;	

				if (screen_state_p->bus_kind == S3_BUS_KIND_PCI)
					val = 0x41;

				S3_READ_CRTC_REGISTER(0x65,tmp);
				tmp |= val;
				S3_WRITE_CRTC_REGISTER(0x65, tmp);
			}
		}
	}
	BT485_SELECT_REGISTER_SET(1);
	outb(BT485_COMMAND_REG_0,bt485_state_p->command_register_0); 
	if ( bt485_state_p->use_clock_doubler == TRUE )
	{
		/* Begin the complex sequence to write command register 3 */
		outb(BT485_COMMAND_REG_0, 
		bt485_state_p->command_register_0 |
				BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK);
		BT485_SELECT_REGISTER_SET(0);
		outb(BT485_WRITE_ADDR, 0x01);
		BT485_SELECT_REGISTER_SET(2);
		outb(BT485_STATUS_REG,bt485_state_p->command_register_3); 
		/* End complex sequence. */
	}
	BT485_SELECT_REGISTER_SET(0);
	S3_LOCK_SYSTEM_REGISTERS();
	S3_LOCK_S3_VGA_REGISTERS();
	S3_TURN_SCREEN_ON();
	
	switch (visual_p->si_visual_p->SVtype)
	{
		case TRUECOLOR_AVAIL:
			/*
			 * assume true color for all other depths.
			 */
			generic_state_p->screen_functions_p->si_get_colormap =
				s3_dac_get_colormap_truecolor;
			generic_state_p->screen_functions_p->si_set_colormap =
				s3_no_operation_succeed;
				break;
		default:
			return (s3_dac_initialize_generic_dac(generic_state_p));
			/*NOTREACHED*/
			break;
	}
	return 1;
}

STATIC int
s3_dac_uninitialize_bt485_dac(struct generic_screen_state *generic_state_p)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;
	struct s3_dac_bt485_state	*bt485_state_p = 
			(struct s3_dac_bt485_state *)screen_state_p->dac_state_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_DAC_STATE, bt485_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	/*
	 * Restore the dac mask.
	 */
	outb(VGA_DAC_REGISTER_DAC_AD_MK,
		 screen_state_p->register_state.saved_standard_vga_registers.
		 standard_vga_dac_registers.dac_mask);

	/*
	 * Uninitialize the dac, restore the dac registers.
	 */
	S3_TURN_SCREEN_OFF();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();
	if (bt485_state_p->external_sid_mode == TRUE)
	{
		if (strcmp(screen_state_p->generic_state.screen_config_p->model,
			"9GXE") == 0) 
		{
			unsigned char tmp;
			if (screen_state_p->chipset_step ==  S3_STEP_KIND_D_STEP)
			{
				S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
				tmp &= ~0x20;
				S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
			}
			else if (screen_state_p->chipset_step ==  S3_STEP_KIND_E_STEP)
			{
				S3_READ_CRTC_REGISTER(0x65,tmp);
				tmp &= ~0x20;
				S3_WRITE_CRTC_REGISTER(0x65, tmp);
			}
		}
	}
	/*
	 * Put BT485 in sleep mode before changing pclks.
	 */
	BT485_SELECT_REGISTER_SET(1);
	outb(BT485_COMMAND_REG_0, bt485_state_p->command_register_0 |
								BT485_COMMAND_REG_0_SLEEP_MODE);
	BT485_SELECT_REGISTER_SET(2);
	outb(BT485_COMMAND_REG_1,bt485_state_p->saved_command_register_1); 
	outb(BT485_COMMAND_REG_2,bt485_state_p->saved_command_register_2); 
	BT485_SELECT_REGISTER_SET(1);
	outb(BT485_COMMAND_REG_0,bt485_state_p->saved_command_register_0); 
	if ( bt485_state_p->use_clock_doubler == TRUE )
	{
		/* Begin the complex sequence to read in command register 3 */
		outb(BT485_COMMAND_REG_0, 
		bt485_state_p->command_register_0 |
				BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK);
		BT485_SELECT_REGISTER_SET(0);
		outb(BT485_WRITE_ADDR, 0x01);
		BT485_SELECT_REGISTER_SET(2);
		outb(BT485_STATUS_REG,bt485_state_p->saved_command_register_3 & 0x0F); 
		/* End complex sequence. */
	}
	BT485_SELECT_REGISTER_SET(0);
	S3_LOCK_SYSTEM_REGISTERS();
	S3_LOCK_S3_VGA_REGISTERS();
	S3_TURN_SCREEN_ON();
	return 1;
}



STATIC int
s3_dac_initialize_ti_dac(struct generic_screen_state *generic_state_p)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;
	struct s3_dac_ti_state	*ti_state_p = 
			(struct s3_dac_ti_state *)screen_state_p->dac_state_p;
	struct generic_visual *visual_p = &(generic_current_screen_state_p->
		screen_visuals_list_p[0]);

	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_DAC_STATE, ti_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	S3_TURN_SCREEN_OFF();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();

	/*
	 * Program the Hsync and Vsync polarities.
	 * It should be the inverse of that in MiscOut.
	 */
	S3_WRITE_TI_DAC_REGISTER(TI_GENERAL_CONTROL, ti_state_p->
												general_io_control_register);
	/*
	 * Input clock is CLK1 or CLK1_DOUBLE
	 */
	S3_WRITE_TI_DAC_REGISTER(TI_INPUT_CLOCK_SELECT, ti_state_p->
												input_clock_select_register);

	if (ti_state_p->external_sid_mode == TRUE)
	{
		/*
		 * Workaround for 928 Hardware BUG:
		 * Workaround for the Number 9 GXE cards in muxmode.
		 * For D-Stepping set GEN OUT PORT bit 5 to 1.
		 * For E-Step OR G-Step set crtc register 65 bit 5 to 1.
		 * 	E-Step||G-Step && PCI, Reg 65 |= 0x41
		 * 	E-Step||G-Step && !PCI, Reg 65 |= 0x21
		 */
		if (strcmp(screen_state_p->generic_state.screen_config_p->model,
			"9GXE") == 0)
		{
			unsigned char tmp;
			if (screen_state_p->chipset_step ==  S3_STEP_KIND_D_STEP)
			{
				S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
				tmp |= 0x20;
				S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
			}
			else if ( (screen_state_p->chipset_step ==  S3_STEP_KIND_E_STEP)
				|| (screen_state_p->chipset_step ==  S3_STEP_KIND_G_STEP) )
			{
				/* !PCI && (E-Step||G-Step) && 9GXE */
				int val = 0x21;	

				if (screen_state_p->bus_kind == S3_BUS_KIND_PCI)
					val = 0x41;

				S3_READ_CRTC_REGISTER(0x65,tmp);
				tmp |= val;
				S3_WRITE_CRTC_REGISTER(0x65, tmp);
			}
		}
	}
	S3_WRITE_TI_DAC_REGISTER(TI_AUXILIARY_CONTROL, ti_state_p->
											auxiliary_control_register);
	S3_WRITE_TI_DAC_REGISTER(TI_OUTPUT_CLOCK_SELECT, ti_state_p->
											output_clock_select_register);
	S3_WRITE_TI_DAC_REGISTER(TI_MUX_CONTROL_1, ti_state_p->
											mux_control_1_register);
	S3_WRITE_TI_DAC_REGISTER(TI_MUX_CONTROL_2, ti_state_p->
											mux_control_2_register);
	S3_WRITE_TI_DAC_REGISTER(TI_GENERAL_IO_CONTROL, ti_state_p->
											general_io_control_register);
	S3_WRITE_TI_DAC_REGISTER(TI_GENERAL_IO_DATA, ti_state_p->
											general_io_data_register);
	S3_WRITE_TI_DAC_REGISTER(TI_SENSE_TEST, ti_state_p->sense_test_register);

	S3_LOCK_SYSTEM_REGISTERS();
	S3_LOCK_S3_VGA_REGISTERS();
	S3_TURN_SCREEN_ON();
	
	switch (visual_p->si_visual_p->SVtype)
	{
		case TRUECOLOR_AVAIL:
#ifdef DELETE
			/*
			 * assume true color for all other depths.
			 */
			generic_state_p->screen_functions_p->si_get_colormap =
				s3_dac_get_colormap_truecolor;
#endif
			generic_state_p->screen_functions_p->si_set_colormap =
				s3_no_operation_succeed;
				break;
		default:
			return (s3_dac_initialize_generic_dac(generic_state_p));
			/*NOTREACHED*/
			break;
	}
	return 1;
}

STATIC int
s3_dac_uninitialize_ti_dac(struct generic_screen_state *generic_state_p)
{
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) generic_state_p;
	struct s3_dac_ti_state	*ti_state_p = 
			(struct s3_dac_ti_state *)screen_state_p->dac_state_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_DAC_STATE, ti_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	/*
	 * Restore the dac mask.
	 */
	outb(VGA_DAC_REGISTER_DAC_AD_MK,
		 screen_state_p->register_state.saved_standard_vga_registers.
		 standard_vga_dac_registers.dac_mask);

	/*
	 * Uninitialize the dac, restore the dac registers.
	 */
	S3_TURN_SCREEN_OFF();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();
	if (ti_state_p->external_sid_mode == TRUE)
	{
		if (strcmp(screen_state_p->generic_state.screen_config_p->model,
			"9GXE") == 0) 
		{
			unsigned char tmp;
			if (screen_state_p->chipset_step ==  S3_STEP_KIND_D_STEP)
			{
				S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
				tmp &= ~0x20;
				S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
					tmp);
			}
			else if (screen_state_p->chipset_step ==  S3_STEP_KIND_E_STEP)
			{
				S3_READ_CRTC_REGISTER(0x65,tmp);
				tmp &= ~0x20;
				S3_WRITE_CRTC_REGISTER(0x65, tmp);
			}
		}
	}

	/*
	 * Restore TI DAC registers.
	 */
	S3_WRITE_TI_DAC_REGISTER(TI_CURS_CONTROL, ti_state_p->
									saved_cursor_control_register);
	S3_WRITE_TI_DAC_REGISTER(TI_MUX_CONTROL_1, ti_state_p->
									saved_mux_control_1_register);
	S3_WRITE_TI_DAC_REGISTER(TI_MUX_CONTROL_2, ti_state_p->
									saved_mux_control_2_register);
	S3_WRITE_TI_DAC_REGISTER(TI_INPUT_CLOCK_SELECT, ti_state_p->
									saved_input_clock_select_register);
	S3_WRITE_TI_DAC_REGISTER(TI_OUTPUT_CLOCK_SELECT, ti_state_p->
									saved_output_clock_select_register);
	S3_WRITE_TI_DAC_REGISTER(TI_GENERAL_CONTROL, ti_state_p->
									saved_general_control_register);
	S3_WRITE_TI_DAC_REGISTER(TI_AUXILIARY_CONTROL, ti_state_p->
									saved_auxiliary_control_register);
	S3_WRITE_TI_DAC_REGISTER(TI_GENERAL_IO_CONTROL, ti_state_p->
									saved_general_io_control_register);
	S3_WRITE_TI_DAC_REGISTER(TI_GENERAL_IO_DATA, ti_state_p->
									saved_general_io_data_register);
	S3_LOCK_SYSTEM_REGISTERS();
	S3_LOCK_S3_VGA_REGISTERS();
	S3_TURN_SCREEN_ON();
	return 1;
}

/*
 * Initialization
 */

/*
 * Init and Uninit tables.
 */
STATIC int (*s3_dac_kind_to_init_function_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
		 INIT
#include "s3_dacs.def"
#undef DEFINE_DAC
};

STATIC int (*s3_dac_kind_to_uninit_function_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
		 UNINIT
#include "s3_dacs.def"
#undef DEFINE_DAC
};

STATIC void (*s3_dac_kind_to_get_color_method_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	    GET
#include "s3_dacs.def"
#undef DEFINE_DAC
};

STATIC void (*s3_dac_kind_to_set_color_method_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
		 DOWNLOAD
#include "s3_dacs.def"
#undef DEFINE_DAC
};

STATIC void
s3_colormap_initialize_static_colormap(
	struct generic_colormap *colormap_p,
	struct s3_options_structure *options_p)
{
	FILE *colormap_file_p;
	char *colormap_file_name_p;
	int colormap_file_line_number = 0;
	char line_buffer[DEFAULT_S3_COLORMAP_DESCRIPTION_FILE_LINE_SIZE];
	int colormap_entry_count = 0;
	unsigned short *rgb_p;
	int rgb_shift_count;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));

	ASSERT(!S3_IS_PROGRAMMABLE_VISUAL(colormap_p->visual_p));
	
	rgb_p = colormap_p->rgb_values_p;

	colormap_file_name_p =
		(options_p->static_colormap_description_file) ?
		options_p->static_colormap_description_file :
		DEFAULT_S3_COLORMAP_DESCRIPTION_FILE_NAME;

	rgb_shift_count = 
		(colormap_p->visual_p->si_visual_p->SVbitsrgb == 8) ? 8U : 10U;

	if ((colormap_file_p = fopen(colormap_file_name_p, "r")) == NULL)
	{
		(void) fprintf(stderr,
		   	S3_CANNOT_OPEN_COLORMAP_DESCRIPTION_FILE_MESSAGE,
			colormap_file_name_p);
		perror("");
		(void) fprintf(stderr,
			S3_USING_DEFAULT_STATIC_COLORMAP_MESSAGE);
		
		/*
		 * Servers Default colormap. 0 - black, 1 - white.
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
			/*
			 * Grayscale for now.
			 */
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
		 * [<WS>] <NUM>, <NUM>, <NUM>, [ '#'.* ]
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
						  DEFAULT_S3_COLORMAP_DESCRIPTION_LINE_FORMAT,
						  &red_value, &blue_value, &green_value) != 3)
				{
					(void) fprintf(stderr,
					S3_COLORMAP_DESCRIPTION_LINE_DOES_NOT_MATCH_FORMAT_MESSAGE,
					colormap_file_name_p, colormap_file_line_number, 
					line_buffer);
					return;
				}
				else
				{
				
					/*
					 * Create the colormap entry.
					 */
					*rgb_p++ = (unsigned short) red_value >> rgb_shift_count;
					*rgb_p++ = (unsigned short) blue_value >> rgb_shift_count;
					*rgb_p++ = (unsigned short) green_value >> rgb_shift_count;

					colormap_entry_count ++;

				}
			}
			
			/*
			 * Check if fgets had prematurely truncated this line.
			 */
			if (*(line_buffer + strlen(line_buffer) -1) != '\n')
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

function void
s3_colormap__initialize__(SIScreenRec *si_screen_p,
							struct s3_options_structure *options_p)
{ 
	enum s3_visual_kind visual_kind;
	struct generic_colormap *colormap_p;
	struct s3_screen_state *screen_state_p =
		(struct s3_screen_state *) si_screen_p->vendorPriv;
	SIVisualP si_visual_p = 0;
	int tmp_count;
	int index;
	
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_screen_state *) screen_state_p));

	ASSERT((void *) generic_current_screen_state_p == (void *)
		   screen_state_p);

	/*
	 * Attempt to read in the visuals.
	 */
	if (!screen_state_p->generic_state.screen_visuals_list_p)
	{
		enum s3_visual_kind screen_default_visual = S3_VISUAL_NULL;
		unsigned int mask;
		enum s3_dac_mode dac_mode;
		int visual_count;
		int dac_flags = 0;

		unsigned int screen_8_4_bit_visual_list = /* save a copy */
			options_p->screen_8_4_bit_visual_list;
		
#if (defined(__DEBUG__))
		if (s3_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_colormap__initialize__)  reading in visuals {\n"
				"\tvisual_list = 0x%x\n"
				"}\n",
				options_p->screen_8_4_bit_visual_list);
		}
#endif
		/*
		 * Extract the screen default visual from the config file
		 * structure. Add this to the list of 8-4 bit visual list.
		 */
		switch (screen_state_p->generic_state.screen_depth)
		{
		case 4 :
		case 8 :
			switch(screen_state_p->generic_state.screen_config_p->visual_type)
			{
			case STATICGRAY_AVAIL :
				screen_default_visual = S3_VISUAL_STATIC_GRAY;
				screen_8_4_bit_visual_list |=
					S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY;
				break;
				
			case GRAYSCALE_AVAIL :
				screen_default_visual = S3_VISUAL_GRAY_SCALE;
				screen_8_4_bit_visual_list |=
					S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE;
				break;

			case STATICCOLOR_AVAIL :
				screen_default_visual = S3_VISUAL_STATIC_COLOR;
				screen_8_4_bit_visual_list |=
					S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR;
				break;

			case PSEUDOCOLOR_AVAIL :
				screen_default_visual = S3_VISUAL_PSEUDO_COLOR;
				screen_8_4_bit_visual_list |=
					S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR;
				break;
				
			case DIRECTCOLOR_AVAIL :
			case TRUECOLOR_AVAIL :
				(void) fprintf(stderr,
					   S3_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE,
					   screen_state_p->generic_state.screen_depth);
				break;
				
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
			}
			break;

		case 16 :				/* No default visual here */
		case 24:
			screen_default_visual = S3_VISUAL_NULL;
			visual_count = 1;
			break;
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}

		/*
		 * Count the number of visuals the user wants
		 * NOTE: currently SI does not support multiple visuals.
		 */
		switch (screen_state_p->generic_state.screen_depth)
		{
		case 4 :
		case 8 :
			for (visual_count = 0, mask = 0x1;
				 mask;
				 mask <<= 1)
			{
				switch (mask & screen_8_4_bit_visual_list)
				{
				case 0:		/* ignore */
					break;
				case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR :
				case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR :
				case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE :
				case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY :
					/*
					 * found a useable visual
					 */
					visual_count ++;
					break;
					
				default :
					/*CONSTANTCONDITION*/
					ASSERT(0);
					break;
				}
			}
			break;
			/*
			 * Only one visual is possible in True color mode at any 
			 * given time.
			 */
		case 16 :
			switch(options_p->dac_16_bit_color_mode)
			{
				case S3_OPTIONS_DAC_16_BIT_COLOR_MODE_555:
					dac_mode = S3_DAC_16_BIT_5_5_5_SUPPORTED;
					visual_kind = S3_VISUAL_TRUE_COLOR_16_555;
				break;

				case S3_OPTIONS_DAC_16_BIT_COLOR_MODE_565:
					dac_mode = S3_DAC_16_BIT_5_6_5_SUPPORTED;
					visual_kind = S3_VISUAL_TRUE_COLOR_16_565;
				break;

				case S3_OPTIONS_DAC_16_BIT_COLOR_MODE_655:
					dac_mode = S3_DAC_16_BIT_6_5_5_SUPPORTED;
					visual_kind = S3_VISUAL_TRUE_COLOR_16_655;
				break;

				case S3_OPTIONS_DAC_16_BIT_COLOR_MODE_664:
					dac_mode = S3_DAC_16_BIT_6_6_4_SUPPORTED;
					visual_kind = S3_VISUAL_TRUE_COLOR_16_664;
				break;

				default :
					/*CONSTANTCONDITION*/
					ASSERT(0);
				break;

			}
			/*
			 * Check if the dac mode is supported by the dac.
			 */
			if (!(s3_dac_kind_to_flags_16_table[screen_state_p->dac_kind] &
				  dac_mode))
			{
				(void) fprintf(stderr,
					   S3_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE,
					   s3_dac_kind_to_dac_name[screen_state_p->dac_kind],
					   s3_dac_mode_to_dac_mode_description[dac_mode]);
				visual_count = 0;
			}
			/*
			 * Note that visual count is already one by the time you
			 * get here. There is exactly one visual with 16 bit - 
			 * TrueColor.
			 */
			break;
		case 24:
			/*
			 * Check if the dac mode is supported by the dac.
			 */
			/* NO 24 bit modes for now. */
			if (!(s3_dac_kind_to_flags_24_table[screen_state_p->dac_kind] &
				  dac_mode))
			{
				(void) fprintf(stderr,
					   S3_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE,
					   s3_dac_kind_to_dac_name[screen_state_p->dac_kind],
					   s3_dac_mode_to_dac_mode_description[dac_mode]);
				visual_count = 0;
			}
			break;

		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
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
			 * Allocate space for SI and generic visuals and stamp them.
			 */
			si_screen_p->flagsPtr->SIvisuals = si_visual_p =
				allocate_and_clear_memory(visual_count * sizeof(SIVisual));

			screen_state_p->generic_state.screen_visuals_list_p =
				visuals_list_p = 
				allocate_and_clear_memory(visual_count * 
										  sizeof(struct generic_visual));

			for(tmp_count = 0; tmp_count < visual_count; tmp_count ++)
			{
				tmp_visual_p = 
					&(screen_state_p->generic_state.
					screen_visuals_list_p[tmp_count]);

#if (defined(__DEBUG__))
				STAMP_OBJECT(GENERIC_VISUAL, tmp_visual_p);
#endif				
				/*
				 * The SI visual
				 */
				tmp_visual_p->si_visual_p = &(si_visual_p[tmp_count]);
				/*
				 * Information to the DAC.
				 */
				tmp_visual_p->dac_flags = dac_flags;
				/*
				 * Methods.
				 */
				tmp_visual_p->get_color_method_p =
					s3_dac_kind_to_get_color_method_table
						[screen_state_p->dac_kind];

				/*
				 * The set-colormap-method is valid only for visuals
				 * with a programmable color map.
				 */
				tmp_visual_p->set_color_method_p =
						s3_dac_kind_to_set_color_method_table
						[screen_state_p->dac_kind];
			}
			
			/*
			 * Allocate space for colormap pointers.
 			 */
			screen_state_p->generic_state.screen_colormaps_pp =
				allocate_and_clear_memory(visual_count * 
										  sizeof(struct generic_colormap *));

#if (defined(__DEBUG__))
			if (s3_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
				"(s3_colormap__initialize__) {\n"
				"\tnumber of visuals = %d\n"
				"\tvisuals_list = %p\n"
				"\tcolormaps_list_pp = %p\n"
				"}\n",
				visual_count,
				(void *) screen_state_p->generic_state.
					screen_visuals_list_p,
				(void *) screen_state_p->generic_state.
					screen_colormaps_pp);
			}
#endif
			/*
			 * Copy visuals in, allocate colormap space.
			 */
			switch (screen_state_p->generic_state.screen_depth)
			{
			case 16 :
			case 24:
				
				/*
				 * Only one visual around.
				 */
				tmp_count = 0;
				tmp_visual_p = visuals_list_p;
				
				*si_visual_p = s3_visuals_table[visual_kind];
				si_visual_p->SVbitsrgb = 6;
				if (options_p->dac_rgb_width == 8)
				{
					si_visual_p->SVbitsrgb = 8;
				}
				
				/*
				 * Allocate the number of colormaps specified in
				 * the visual.
				 */
				screen_state_p->generic_state.
				screen_colormaps_pp[tmp_count] = colormap_p =
					allocate_and_clear_memory(si_visual_p->SVcmapcnt *
								  sizeof(struct generic_colormap));
				
#if (defined(__DEBUG__))
				if (s3_colormap_debug)
				{
					(void) fprintf(debug_stream_p,
					"(s3_colormap__initialize__) {\n"
					"\tvisual_kind = %s\n"
					"\tsi_visual_p = %p\n"
					"\ttmp_visual_p = %p\n"
					"\tcolormap_list_p = %p\n"
					"\tn_colormaps = %ld\n"
					"\t}\n",
					s3_visual_kind_to_visual_kind_dump[visual_kind],
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
					colormap_p[index].si_colormap.visual = 
						si_visual_p->SVtype;
					colormap_p[index].si_colormap.sz = 
						si_visual_p->SVcmapsz;
					/*
					 * allocate memory for RGB values.
					 */
					colormap_p[index].rgb_values_p =
						allocate_and_clear_memory(3 * si_visual_p->SVcmapsz *
												  sizeof(unsigned short));
					colormap_p[index].visual_p = tmp_visual_p;
					
#if (defined(__DEBUG__))
					if (s3_colormap_debug)
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
				}

				break;
				
			case 4:
			case 8:
				
				tmp_count = 0;
				tmp_visual_p = visuals_list_p;
				
				/*
				 * First fill in the default visual as the very first one.
				 */

				*si_visual_p = s3_visuals_table[screen_default_visual];
				si_visual_p->SVdepth =
					screen_state_p->generic_state.screen_depth;
				si_visual_p->SVcmapsz = 
					(1U << si_visual_p->SVdepth);
				si_visual_p->SVbitsrgb = 6;
				if (options_p->dac_rgb_width == 8)
				{
					si_visual_p->SVbitsrgb = 8;
				}

				/*
				 * Allocate the number of colormaps specified in
				 * the visual.
				 */
				screen_state_p->generic_state.
					screen_colormaps_pp[tmp_count] = colormap_p =
						allocate_and_clear_memory(si_visual_p->SVcmapcnt *
										  sizeof(struct generic_colormap));
				
#if (defined(__DEBUG__))
				if (s3_colormap_debug)
				{
					(void) fprintf(debug_stream_p,
					"(s3_colormap__initialize__) {\n"
					"\tvisual_kind = %s (DEFAULT)\n"
					"\tsi_visual_p = %p\n"
					"\ttmp_visual_p = %p\n"
					"\tcolormap_list_p = %p\n"
					"\tn_colormaps = %ld\n"
					"\t}\n",
					s3_visual_kind_to_visual_kind_dump[ screen_default_visual],
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
					colormap_p[index].si_colormap.visual = 
						si_visual_p->SVtype;
					colormap_p[index].si_colormap.sz = 
						si_visual_p->SVcmapsz;
					/*
					 * allocate memory for RGB values.
					 */
					colormap_p[index].rgb_values_p =
						allocate_and_clear_memory(3 * si_visual_p->SVcmapsz *
												  sizeof(unsigned short));
#ifdef DELETE
						memset((void*)colormap_p[index].rgb_values_p,
						0xFF,
						(3 * si_visual_p->SVcmapsz * sizeof(unsigned short)));
#endif

					colormap_p[index].visual_p = tmp_visual_p;
					
#if (defined(__DEBUG__))
					if (s3_colormap_debug)
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
                     */

					/*
					 * Assumption here is that there is no TrueColor
					 * with 8/4 bits per pixel. So if we have a non
					 * Programmable visual here it is a static color
					 * visual.
					 */
                    if (!S3_IS_PROGRAMMABLE_VISUAL(tmp_visual_p))
                    {
                        s3_colormap_initialize_static_colormap(
                            &(colormap_p[index]), options_p);
                    }
				}

				si_visual_p++;
				tmp_visual_p++;
				/*
				 * Now handle the remaining colormaps.
				 */
				for (mask = 0x1, tmp_count = 1; 
					 mask && (tmp_count < visual_count); 
					 mask <<= 1)
				{
					switch(mask & options_p->screen_8_4_bit_visual_list)
					{
						
					case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR :
						visual_kind = S3_VISUAL_PSEUDO_COLOR;
						break;

					case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR :
						visual_kind = S3_VISUAL_STATIC_COLOR;
						break;
						
					case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY :
						visual_kind = S3_VISUAL_STATIC_GRAY;
						break;

					case S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE :
						visual_kind = S3_VISUAL_GRAY_SCALE;
						break;
							
					default :
						(void) fprintf(stderr,
						   S3_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE,
						   screen_state_p->generic_state.screen_depth);
						/*FALLTHROUGH*/
					case 0 :				/* continue */
						continue;
					}
					
					if (visual_kind == screen_default_visual)
					{
						continue; /* we have already handled this */
					}

					*si_visual_p = s3_visuals_table[visual_kind];
					si_visual_p->SVdepth =
						screen_state_p->generic_state.screen_depth;
					si_visual_p->SVcmapsz = 
						(1U << si_visual_p->SVdepth);
					si_visual_p->SVbitsrgb = 6;
					if (options_p->dac_rgb_width == 8)
					{
						si_visual_p->SVbitsrgb = 8;
					}
					/*
					 * Allocate the number of colormaps specified in
					 * the visual.
					 */
					screen_state_p->generic_state.
						screen_colormaps_pp[tmp_count] =
						colormap_p =
						allocate_and_clear_memory(si_visual_p->SVcmapcnt *
											sizeof(struct generic_colormap));
					
#if (defined(__DEBUG__))
					if (s3_colormap_debug)
					{
						(void) fprintf(debug_stream_p,
						"(s3_colormap__initialize__) {\n"
						"\tvisual_kind = %s\n"
						"\tsi_visual_p = %p\n"
						"\ttmp_visual_p = %p\n"
						"\tcolormap_list_p = %p\n"
						"\tn_colormaps = %ld\n"
						"\t}\n",
						s3_visual_kind_to_visual_kind_dump[visual_kind],
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
						colormap_p[index].si_colormap.visual = 
							si_visual_p->SVtype;
						colormap_p[index].si_colormap.sz = 
							si_visual_p->SVcmapsz;
						/*
						 * allocate memory for RGB values.
						 */
						colormap_p[index].rgb_values_p =
							allocate_and_clear_memory(
							    3 * si_visual_p->SVcmapsz *
								sizeof(unsigned short));

						colormap_p[index].visual_p = tmp_visual_p;
						
#if (defined(__DEBUG__))
						if (s3_colormap_debug)
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
					
					/*
					 * advance to the next visual.
					 */
					si_visual_p ++;
					tmp_visual_p ++;
					tmp_count ++;
				}
				break;
				
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
				
			}
			
		}
		else
		{
#if (defined(__DEBUG__))
			if (s3_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
				"(s3_colormap__initialize__) {\n"
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
		if (s3_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_colormap__initialize__) {\n"
				"\tvisuals already read in by board layer.\n}\n");
		}
#endif
	}

	/*
	 * Patch in the DAC init and uninit functions.
	 */
	if (screen_state_p->register_state.generic_state.dac_init 
		== NULL)
	{
		screen_state_p->register_state.generic_state.dac_init = 
			s3_dac_kind_to_init_function_table[screen_state_p->dac_kind];
	}
	
	if (screen_state_p->register_state.generic_state.dac_uninit ==
		NULL)
	{
		screen_state_p->register_state.generic_state.dac_uninit =
			s3_dac_kind_to_uninit_function_table[screen_state_p->dac_kind];
	}
}
