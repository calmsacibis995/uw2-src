/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_colormap.c	1.7"

/***
 ***	NAME
 ***
 ***	mach_colormap.c : colormap, DAC and visual handling for the
 ***					MACH display library.
 ***	
 ***	SYNOPSIS
 ***
 ***	#include "m_colormap.h"
 ***	
 ***	DESCRIPTION
 ***
 ***	This module implements dac, colormap and visual handling for
 ***	the MACH display library.
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
export boolean mach_colormap_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

struct mach_dac_tlc34075_extended_registers
{
#if	(defined(__DEBUG__))
	int stamp;
#endif
	/*
	 * Page 0 contains the conventional palette address and data
	 * registers.
	 * Page 1 is a reserved page.
	 */

	/*
	 * Page 2.
	 */
	unsigned char general_control; 		/* index 0 */
	unsigned char input_clock_sel;		/* index 1 */
	unsigned char output_clock_sel;		/* index 2 */
	unsigned char mux_control;			/* index 3 */
	
	/*
	 * Page 3.
	 */
	unsigned char palette_page;		/* index 0 */
	unsigned char test_register;		/* index 2 */
};

#define MACH_DAC_TLC34075_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('D' << 5) + ('A' << 6) + ('C' << 7) +\
	 ('_' << 8) + ('T' << 9) + ('L' << 10) + ('C' << 11) +\
	 ('3' << 12) + ('4' << 13) + ('0' << 14) + ('7' << 15) +\
	 ('5' << 16))

struct mach_dac_bt481_extended_registers
{
#if (defined(__DEBUG__))
	int stamp;
#endif
	/*
	 * Page 0 contains the standard color palette read and write
	 * registers.
	 */
	/*
	 * Page 1.
	 */
	unsigned char overlay_write_address; /* index 0 */
	unsigned char overlay_register;		/* index 1 */
	unsigned char mode_control;			/* index 2 */
	unsigned char overlay_read_address;	/* index 3 */
	
};	
		
/***
 ***	Includes.
 ***/

#include "g_colormap.h"
#include "m_regs.h"
#include <memory.h>
#include <ctype.h>
#include <string.h>

/***
 ***	Constants.
 ***/

#define MACH_DAC_MASK_8_BIT 		0xFFU
#define MACH_DAC_MASK_4_BIT 		0x0FU

/*
 * The extended dac register mappings .These are valid only for the
 * TLC34075 dac.
 */
/* Page 0 : palette access registers */
/* Page 1 : reserved */
/* Page 2 : special control */
#define MACH_DAC_TLC34075_GENERAL_CONTROL   MACH_REGISTER_DAC_W_INDEX /*0*/
#define MACH_DAC_TLC34075_INPUT_CLOCK_SEL	MACH_REGISTER_DAC_DATA    /*1*/
#define MACH_DAC_TLC34075_OUTPUT_CLOCK_SEL	MACH_REGISTER_DAC_MASK	  /*2*/
#define MACH_DAC_TLC34075_MUX_CNTL			MACH_REGISTER_DAC_R_INDEX /*3*/
/* Page 3 : test and reset set */
#define MACH_DAC_TLC34075_PALETTE_PAGE      MACH_REGISTER_DAC_W_INDEX /*0*/
#define MACH_DAC_TLC34075_TEST              MACH_REGISTER_DAC_MASK	  /*2*/
#define MACH_DAC_TLC34075_RESET_STATE       MACH_REGISTER_DAC_R_INDEX /*3*/

STATIC const char *const
mach_dac_mode_to_dac_mode_description[] =
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	#DESCRIPTION
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};

STATIC const SIVisual 
mach_visuals_table[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	{\
		 TYPE, DEPTH, N_COLORMAPS, SIZE, N_VALID_BITS,\
		 R_MASK, G_MASK, B_MASK, R_OFFSET, G_OFFSET, B_OFFSET\
	}
#include "m_visual.def"			
#undef DEFINE_VISUAL	

};

#if (defined(__DEBUG__))
STATIC const char *const mach_visual_kind_to_visual_kind_dump[] =
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
					  #NAME
#include "m_visual.def"			
#undef DEFINE_VISUAL	
};
#endif	

STATIC const int mach_dac_kind_to_max_clock_table[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	MAXCLOCK
#include "m_dacs.def"
#undef DEFINE_DAC
};

STATIC const unsigned int mach_dac_kind_to_flags_16_table[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	FLAGS16
#include "m_dacs.def"
#undef DEFINE_DAC
};

STATIC const unsigned int mach_dac_kind_to_flags_24_table[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	FLAGS24
#include "m_dacs.def"
#undef DEFINE_DAC
};

STATIC const unsigned int mach_dac_kind_to_flags_rgb_table[] = 
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, SET, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	FLAGSRGB
#include "m_dacs.def"
#undef DEFINE_DAC	
};

STATIC const char *const mach_dac_kind_to_dac_name[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESCRIPTION)\
		DESCRIPTION
#include "m_dacs.def"
#undef DEFINE_DAC
};


/***
 ***	Macros.
 ***/


#define MACH_COLORMAP_DAC_ACCESS_DELAY()\
{\
	volatile int __count = mach_colormap_dac_access_delay_count;\
	while(__count-- > 0)\
	{\
		;\
	}\
}

/*
 * Macro for selecting the register set corresponding to offset
 * using the extended dac access feature.
 */
#define MACH_ENABLE_EXTENDED_DAC_ACCESS(ext_ge_config, dac_register_set) \
{\
	  (ext_ge_config) = ((ext_ge_config) &\
		  ~MACH_EXT_GE_CONFIG_DAC_EXT_ADDR) |\
		  ((dac_register_set)  <<\
		   MACH_EXT_GE_CONFIG_DAC_EXT_ADDR_SHIFT) ;\
	outw(MACH_REGISTER_EXT_GE_CONFIG, (ext_ge_config));\
}

/*
 * Determining if a visual is programmable
 */

#define MACH_IS_PROGRAMMABLE_VISUAL(visual_p)\
	((visual_p)->si_visual_p->SVtype & 0x1)

#define MACH_IS_DAC_PROGRAMMABLE(visual_p)									\
	((visual_p)->dac_flags & 												\
	 (MACH_EXT_GE_CONFIG_PIXEL_WIDTH_4 | MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8))

/***
 *** 	Variables.
 ***/

STATIC int mach_colormap_dac_access_delay_count = 0;

/***
 ***	Functions.
 ***/
 /*
  * Screen off/on functions for screen saver.
  */
STATIC SIBool
mach_video_blank_onoff(SIBool on)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void)fprintf(debug_stream_p, 
			"\tmach_video_blank_onoff()\n"
			"\t{\n"
			"\ton = %d\n"
			"\t}\n",
			on);
	}
#endif
	
	/*
	 * attempt to switch off the display by programming the disp_control 
	 * register. For depths less than or equal to 8 (pseudo color)
	 * the transition can be made smoother by programming the dac mask register.
	 */
	if (on == SI_FALSE)
	{
		/*
		 * Turn screen off.
		 */
		if (screen_state_p->generic_state.screen_depth <= 8)
		{
			outb(MACH_REGISTER_DAC_MASK, 0);
		}
		else
		{
			MACH_DISABLE_CRT_CONTROLLER(
				screen_state_p->register_state.disp_cntl);
		}
	}
	else
	{
		/*
		 * Turn screen on.
		 */
		if (screen_state_p->generic_state.screen_depth <= 8)
		{
			outb(MACH_REGISTER_DAC_MASK,  
				(1U << screen_state_p->generic_state.screen_depth ) - 1);
		}
		else
		{
			MACH_ENABLE_CRT_CONTROLLER(
				screen_state_p->register_state.disp_cntl);
		}
	}
	return (SI_SUCCEED);
}

/*
 * COLORMAP programming.
 */
/*
 * Null method.
 */
/*ARGSUSED*/
STATIC SIBool
mach_colormap_operation_fail(SIint32 state_index, SIint32 state_flags,
							 SIGStateP state_p)
{
#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_colormap_operation_fail)\n"
"{\n"
"\tstate_index = %ld\n"
"\tstate_flags = 0x%lx\n"
"\tstate_p = %p\n"
"}\n",
					   state_index,
					   state_flags,
					   (void *) state_p);
	}
#endif
	return (SI_FAIL);
}

/*
 * Dac programming functions.
 */
STATIC void
mach_dac_pseudocolor_get_color(const struct generic_visual *visual_p,
							   const int color_index, 
							   unsigned short *rgb_values_p)

{
#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_pseudocolor_get_color)\n"
"{\n"
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
	 * program the color index.
	 */

	outb(MACH_REGISTER_DAC_R_INDEX, color_index);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	/*
	 * retrieve R, G, B values.
	 */
	*rgb_values_p ++ = inb(MACH_REGISTER_DAC_DATA);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	*rgb_values_p ++ = inb(MACH_REGISTER_DAC_DATA);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	*rgb_values_p ++ = inb(MACH_REGISTER_DAC_DATA);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	
}

STATIC void
mach_dac_pseudocolor_set_color(const struct generic_visual *visual_p,
							   const int color_index,
							   unsigned short *rgb_values_p)
{
#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_pseudocolor_set_color)\n"
"{\n"
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
	outb(MACH_REGISTER_DAC_W_INDEX, color_index);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	/*
	 * program R, G, B values.
	 */
	outb(MACH_REGISTER_DAC_DATA, *rgb_values_p++);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	outb(MACH_REGISTER_DAC_DATA, *rgb_values_p++);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	outb(MACH_REGISTER_DAC_DATA, *rgb_values_p++);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
	
	return;
}


/*
 * PseudoColor operations
 */
/*
 * mach_colormap_set_colormap
 * 
 * Setting a colormap's entries.  Since the current SI calls this
 * entry point irrespective of the type of colormap supported (ie: a
 * bug) we need to check whether the currently selected colormap and
 * visual are actually modifiable.
 */

STATIC SIBool
mach_colormap_set_colormap(SIint32 visual_number, 
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
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_set_colormap_pseudocolor)\n"
"{\n"
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
	 * Check if the DAC is in a programmable mode.  If not, return.
	 */

	if (!MACH_IS_DAC_PROGRAMMABLE(visual_p))
	{
		
#if (defined(__DEBUG__))
		if (mach_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_colormap_set_colormap)\n"
"{\n"
"\t# Called for non programmable dac mode\n"
"}\n"
						   );
		
		}
#endif
		return (SI_SUCCEED);
	}

	/*
	 * The mach DACs support 8 and 6 bits per rgb in industry
	 * standard 4/8 bit mode.
	 */
	rgb_shift_count = (si_visual_p->SVbitsrgb == 8) ?
		8U : 10U;

#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_colormap_set_colormap)\n"
"{\n"
"\t# %ld bit programming\n"
"}\n",
					   si_visual_p->SVdepth);
	}
#endif

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
			 * Program the DAC.
			 */
			(*dac_set_color_p)
				(visual_p,
				 colors_p->SCpindex, 
				 &(colormap_p->rgb_values_p[3 * colors_p->SCpindex]));
			
#if (defined(__DEBUG__))
			if (mach_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
							   "(mach_colormap_set_colormap)\n"							   
							   "\t{\n"
							   "\t\tindex = 0x%lx\n"
							   "\t\tred = 0x%hx\n"
							   "\t\tgreen = 0x%hx\n"
							   "\t\tblue = 0x%hx\n"
							   "\t}\n",
							   colors_p->SCpindex,
							   colors_p->SCred,
							   colors_p->SCgreen,
							   colors_p->SCblue);
			}
#endif

		}
#if (defined(__DEBUG__))
		else
		{
			if (mach_colormap_debug)
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
 * mach_get_colormap_truecolor
 *
 * Fill in the values of a true color map
 */

STATIC SIBool
mach_colormap_get_colormap_truecolor(SIint32 visual_number,
									 SIint32 colormap_number,
									 SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p =
		&(generic_current_screen_state_p->
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
			colors_p->SCgreen = (colors_p->SCpindex << 16) /
				number_of_greens; 
			break;
			
		case VALID_BLUE :
			colors_p->SCblue = (colors_p->SCpindex << 16) /
				number_of_blues;
			break;
			
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		
		}
	}
	
	return SI_SUCCEED;
}

#ifdef DELETE /* Needed for the old server, before sicmap.c fix */
STATIC SIBool
mach_colormap_get_colormap_truecolor(SIint32 visual_number,
									 SIint32 colormap_number,
									 SIColor *colors_p, SIint32 count)
{
	struct generic_visual *visual_p =
		&(generic_current_screen_state_p->
		  screen_visuals_list_p[visual_number]);

	SIVisualP si_visual_p = visual_p->si_visual_p;
	
	int	num_red,num_green,num_blue;
	
	ASSERT(visual_number >= 0 && visual_number <
		   generic_current_screen_state_p->screen_number_of_visuals);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_VISUAL,visual_p));
	ASSERT(colormap_number >=0 && colormap_number <
		   si_visual_p->SVcmapcnt);

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
 * mach_colormap_get_colormap
 *
 * Entry point to get a colormap entry.  Intended for those DAC modes 
 * with readable colormaps (industry standard 4/8 bit DACS).
 */

STATIC SIBool
mach_colormap_get_colormap(SIint32 visual_number, 
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
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_get_colormap_pseudocolor)\n"
"{\n"
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
			
			/*
			 * This reads the contents of the colormap from the DAC's
			 * registers.  For static colormaps, this won't harm
			 * anything, and also provides a hook for debugging (ie:
			 * get_colormap() operation can be used to look at the
			 * DAC's contents.
			 */
			(*dac_get_color_p)(visual_p, colors_p->SCpindex, rgb_p); 
			
			colors_p->SCred = (*rgb_p++) << rgb_shift_count;
			colors_p->SCgreen = (*rgb_p++) << rgb_shift_count;
			colors_p->SCblue = (*rgb_p++) << rgb_shift_count;

#if (defined(__DEBUG__))
			if (mach_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
"\t{\n"
"\t\tindex = 0x%lx\n"
"\t\tred = 0x%hx\n"
"\t\tgreen = 0x%hx\n"
"\t\tblue = 0x%hx\n"
"\t}\n",
							   colors_p->SCpindex,
							   colors_p->SCred,
							   colors_p->SCgreen,
							   colors_p->SCblue);
			}
#endif

		}
#if (defined(__DEBUG__))
		else
		{
			if (mach_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tbad color index (0x%lx)\n", 
							   colors_p->SCpindex);
			}
		}
#endif
		
		colors_p++;
		
	}

#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
					   "}\n");
	}
#endif
	
	return (SI_SUCCEED);
}

/*
 * Check whether a given DAC can support a display mode.
 * Returns `TRUE' if the DAC can support the mode, `FALSE' otherwise.
 */

function boolean
mach_dac_check_display_mode_feasibility(struct mach_screen_state
										*screen_state_p)
{
	enum mach_dac_kind dac_kind;
	int max_dac_clock;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 (struct generic_screen_state *)
							 screen_state_p));
#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_check_display_mode_feasibility)\n"
"{\n"
"\tscreen_state_p = %p\n"
"}\n",
					   (void *) screen_state_p);
	}
#endif
	
	dac_kind = screen_state_p->dac_kind;
	ASSERT(dac_kind >= MACH_DAC_ATI_68830 && dac_kind < MACH_DAC_UNKNOWN);
	
	/*
	 * Check the pixel clock frequency supported by the DAC.
	 * If this is specified by user option, take the option value else
	 * pick up the max dac clock frequency from the builtin tables.
	 */

	if (screen_state_p->options_p->dac_max_frequency !=
		MACH_OPTIONS_DAC_MAX_FREQUENCY_DEFAULT)
	{
		max_dac_clock = 
			screen_state_p->options_p->dac_max_frequency * 100;
	}
	else
	{
		max_dac_clock = mach_dac_kind_to_max_clock_table[dac_kind];
	}
		
	if (screen_state_p->clock_frequency >
		max_dac_clock)
	{
		(void) fprintf(stderr,
					   MACH_PIXEL_CLOCK_IS_TOO_HIGH_FOR_DAC_MESSAGE,
					   screen_state_p->clock_frequency / 100,
					   screen_state_p->clock_frequency % 100,
					   mach_dac_kind_to_dac_name[dac_kind],
					   max_dac_clock / 100, max_dac_clock % 100);
		return (FALSE);
	}

	/*
	 * DAC specific checks here.
	 */
	switch (screen_state_p->dac_kind)
	{
	case MACH_DAC_TI_TLC_34075 :
		
		if (screen_state_p->generic_state.screen_displayed_width > 1024 &&
			screen_state_p->generic_state.screen_depth > 8)
		{
			(void) fprintf(stderr,
						   MACH_TLC_DAC_CANNOT_SUPPORT_HI_RES_HI_DEPTH_MODE,
						   mach_dac_kind_to_dac_name[screen_state_p->dac_kind]
						   );
			return (FALSE);
		}
		break;

	case MACH_DAC_ATI_68875_CFN :
	case MACH_DAC_IMS_G176J_80Z :
	case MACH_DAC_BT_481 :

		/* 
		 * The only checks needed are those on the clock frequency
		 * and these have already been done for the IMS DAC.
		 * Additional checks on the RGB modes allowed are done at the
		 * time of choosing the visual for the BT481 dac.
		 */
		break;
		
	default :
		(void) fprintf(stderr, MACH_UNTESTED_DAC_INITIALIZATION_MESSAGE,
					  mach_dac_kind_to_dac_name[screen_state_p->dac_kind]);
		
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
		
	}	
	
	return (TRUE);
}

/*
 * mach_dac_program_misc_cntl
 *
 * Helper function to program the MISC CNTL register.
 */
STATIC void
mach_dac_program_misc_cntl(struct mach_screen_state *state_p,
						   unsigned int blank_adjust, 
						   unsigned int pixel_delay)
{

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, state_p));
	
	/*
	 * clear relevant bits of the shadow memory location.
	 */
	state_p->register_state.misc_cntl &=
		~(MACH_R_MISC_CNTL_PIXEL_DELAY |
		  MACH_R_MISC_CNTL_BLANK_ADJUST);
	state_p->register_state.misc_cntl |= 
		(((pixel_delay << MACH_R_MISC_CNTL_PIXEL_DELAY_SHIFT) |
		  (blank_adjust << MACH_R_MISC_CNTL_BLANK_ADJUST_SHIFT)) &
		 (MACH_R_MISC_CNTL_PIXEL_DELAY |
		  MACH_R_MISC_CNTL_BLANK_ADJUST));
	outb(MACH_REGISTER_MISC_CNTL + 1,
		 (state_p->register_state.misc_cntl >> 8U));

#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_program_misc_cntl)\n"
"{\n"
"\tmisc_cntl = 0x%x\n"
"}\n",
					   state_p->register_state.misc_cntl);
	}
#endif
	
}

STATIC int
mach_dac_program_low_clock(struct mach_screen_state *mach_state_p)
{
	int low_clock;
	
	/*
	 * Guarantee a low value for the CLOCK  (< 50 MHz).
	 */
	for(low_clock = 0; 
		low_clock < DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES;
		low_clock++)
	{
		if (mach_state_p->clock_chip_p->clock_frequency_table[low_clock] ==
			5035 )
		{
			/*
			 * Found 50.35 MHz clock
			 */
			break;
		}
	}

	if (low_clock == DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES)
	{
		(void) fprintf(stderr,
					   MACH_CANNOT_RESET_CLOCK_TO_LOW_VALUE_MESSAGE);
		return 0;
	}

	outw(MACH_REGISTER_CLOCK_SEL,
		 ((mach_state_p->register_state.clock_sel &
		   ~MACH_CLOCK_SEL_CLK_SEL) | 
		  (low_clock << MACH_CLOCK_SEL_CLK_SEL_SHIFT) |
		  0x0040));				/* clock/2 to get 25.18 Mhz */

	MACH_COLORMAP_DAC_ACCESS_DELAY();

	return 1; 
}

/*
 * mach_dac_initialize_bt478
 *
 * 
 * 
 */
STATIC int
mach_dac_initialize_bt478(struct generic_screen_state
						  *generic_state_p)
{
	return 0;
}

STATIC int
mach_dac_uninitialize_bt478(struct generic_screen_state
							*generic_state_p)
{
	return 0;
}

/*
 * mach_dac_initialize_bt481
 *
 *
 */
STATIC int
mach_dac_initialize_bt481(struct generic_screen_state
						  *generic_state_p)
{
	struct mach_screen_state *mach_state_p =
		(struct mach_screen_state *) generic_state_p;

	struct generic_visual *visual_p;
	
	unsigned int mpx_flag;
	unsigned short misc_cntl;
	int low_clock;
	unsigned char dac_mask;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, mach_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	ASSERT(mach_state_p->dac_kind == MACH_DAC_BT_481);

	/*
	 * program the dac according to the default visual (0)
	 */
	visual_p = generic_state_p->screen_visuals_list_p;
	
	mpx_flag = (mach_state_p->clock_frequency >= 8000) ? 
		MACH_EXT_GE_CONFIG_MULTIPLEX_PIXELS : 0;
   
	/*
	 * First, disable VGA passthrough and select a low clock rate.
	 */
	MACH_PASSTHROUGH_8514(mach_state_p->register_state.clock_sel);

	/*
	 * Guarantee a low value for the CLOCK  (< 50 MHz).
	 */
	for(low_clock = 0; 
		low_clock < DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES;
		low_clock++)
	{
		if (mach_state_p->clock_chip_p->clock_frequency_table[low_clock] ==
			5035 )
		{
			/*
			 * Found 50.35 MHz clock
			 */
			break;
		}
	}

	if (low_clock == DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES)
	{
		(void) fprintf(stderr,
					   MACH_CANNOT_RESET_CLOCK_TO_LOW_VALUE_MESSAGE);
		return 0;
	}

	outw(MACH_REGISTER_CLOCK_SEL,
		 ((mach_state_p->register_state.clock_sel &
		   ~MACH_CLOCK_SEL_CLK_SEL) | 
		  (low_clock << MACH_CLOCK_SEL_CLK_SEL_SHIFT) |
		  MACH_CLOCK_SEL_CLK_DIV_2)); /* clock/2 to get 25.18 Mhz */

	MACH_COLORMAP_DAC_ACCESS_DELAY();

	switch(generic_state_p->screen_depth)
	{
	case 4:
	case 8:
		outw(MACH_REGISTER_EXT_GE_CONFIG,
			 (mach_state_p->register_state.ext_ge_config &
			  (MACH_EXT_GE_CONFIG_MONITOR_ALIAS |
			   MACH_EXT_GE_CONFIG_ALIAS_ENA)) |	/* monitor alias */
			 MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8 |	/* force 8 bpp */
			 MACH_EXT_GE_CONFIG_RAMDAC_PAGE_1);	/* bank 1 in RAMDAC */

		outb(MACH_REGISTER_DAC_MASK, 0); /* MAGIC_NUMBER */
		mach_state_p->register_state.ext_ge_config =
			((mach_state_p->register_state.ext_ge_config & 
			  (MACH_EXT_GE_CONFIG_ALIAS_ENA |
			   MACH_EXT_GE_CONFIG_MONITOR_ALIAS)) |	/* monitor alias */
			 (visual_p->dac_flags &
			  MACH_EXT_GE_CONFIG_DAC_8_BIT_ENABLE) |
			 (visual_p->dac_flags & MACH_EXT_GE_CONFIG_PIXEL_WIDTH) |
			 MACH_EXT_GE_CONFIG_RAMDAC_PAGE_0);

		outw(MACH_REGISTER_EXT_GE_CONFIG,
			 mach_state_p->register_state.ext_ge_config);
		
		dac_mask = 
			((1 << mach_state_p->generic_state.screen_depth) - 1); 
		
		break;

	case 16:
		
		dac_mask = 0;

		outb(MACH_REGISTER_DAC_MASK, 0);

		misc_cntl = inw(MACH_REGISTER_R_MISC_CNTL);

		outw(MACH_REGISTER_R_MISC_CNTL,
			 (misc_cntl & 0x00F0));	/* preserve ROM_PAGE sel */

		outw(MACH_REGISTER_EXT_GE_CONFIG,
			 (visual_p->dac_flags &
			  MACH_EXT_GE_CONFIG_16_BIT_COLOR_MODE) | /* set RGB mode */
			 (mach_state_p->register_state.ext_ge_config &
			  (MACH_EXT_GE_CONFIG_MONITOR_ALIAS |
			   MACH_EXT_GE_CONFIG_ALIAS_ENA)) |	/* monitor alias */
			 MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8 |	/* force 8bpp */
			 MACH_EXT_GE_CONFIG_RAMDAC_PAGE_1);	/* select DAC page 1 */

		outb(MACH_REGISTER_DAC_MASK, 0x0a2); /* MAGIC NUMBER */

		mach_state_p->register_state.ext_ge_config =
			((visual_p->dac_flags &
			  MACH_EXT_GE_CONFIG_16_BIT_COLOR_MODE) | 
			 mpx_flag |
			 (visual_p->dac_flags & MACH_EXT_GE_CONFIG_PIXEL_WIDTH) |
			 (mach_state_p->register_state.ext_ge_config & 
			  (MACH_EXT_GE_CONFIG_MONITOR_ALIAS |
			   MACH_EXT_GE_CONFIG_ALIAS_ENA)) |
			 MACH_EXT_GE_CONFIG_RAMDAC_PAGE_0);

		outw(MACH_REGISTER_EXT_GE_CONFIG,
			 mach_state_p->register_state.ext_ge_config);
		
		break;

	default:
		return 0;				/* don't know how to initialize in 24
								   bit mode */
	}

	outw(MACH_REGISTER_DAC_MASK, dac_mask);
	
	/*
	 * Restore the clock sel register.
	 */
	MACH_PASSTHROUGH_8514(mach_state_p->register_state.clock_sel);
	
	return 1;
}

STATIC int
mach_dac_uninitialize_bt481(struct generic_screen_state
							*generic_state_p)
{
	struct mach_screen_state *mach_state_p =
		(struct mach_screen_state *) generic_state_p;
	
	unsigned short clock;
	
	clock = inw(MACH_REGISTER_R_MISC_CNTL);

	/*
	 * Reset pixel delay fields.
	 */
	outw(MACH_REGISTER_MISC_CNTL,
		 (clock & MACH_R_MISC_CNTL_ROM_PAGE_SEL));

	outw(MACH_REGISTER_HORZ_OVERSCAN, 0); /* should this be here? */
	
	/*
	 * place the dac in 6 bit mode, with 8 bpp. Reset the ext dac
	 * address fields to zero.
	 */
	outw(MACH_REGISTER_EXT_GE_CONFIG,
		 (mach_state_p->register_state.ext_ge_config & 
		  (MACH_EXT_GE_CONFIG_MONITOR_ALIAS |
		   MACH_EXT_GE_CONFIG_ALIAS_ENA)) |	/* restore alias */
		 MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8 |
		 MACH_EXT_GE_CONFIG_RAMDAC_PAGE_0);	/* 8 bpp */

	/*
     * program the dac read mask.
	 */
	outb(MACH_REGISTER_DAC_MASK,
		 0xFF);
	
	return 1;
}

/*
 * mach_dac_initialize_tlc34075
 *
 * Routine for Initializing the TLC34075 dac for the desired depth.
 * The method and values are adopted from the appnote 'Mach32
 * Mode setting without the BIOS' AN0005-11/12/92.
 * Note: This DAC is synonymous with ATI34075.
 * TLC34075 DAC supports 4/8 bpp, 16bpp with 4 weightings and 
 * 24bpp with 4 weightings. 
 */
STATIC int
mach_dac_initialize_tlc34075(struct generic_screen_state *generic_state_p)
{
	struct mach_screen_state *mach_state_p =
		(struct mach_screen_state *) generic_state_p;
	
	struct generic_visual *visual_p;
	struct mach_dac_tlc34075_extended_registers
		*dac_registers_p;

	int dac_mask = 0;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, mach_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	ASSERT(mach_state_p->dac_kind == MACH_DAC_TI_TLC_34075 ||
		   mach_state_p->dac_kind == MACH_DAC_ATI_68875_CFN);

	/*
	 * program the dac according to the default visual (0)
	 */
	visual_p = generic_state_p->screen_visuals_list_p;
	
	/*
	 * First, disable VGA passthrough and select a low clock rate.
	 */
	MACH_PASSTHROUGH_8514(mach_state_p->register_state.clock_sel);

	if (mach_dac_program_low_clock(mach_state_p) == 0)
	{
		/*
		 * Unable to switch to a low clock frequency.
		 */
		return 0;
	}
	
	/*
	 * The dac programming goes through only if bpp is 8 and
	 * bits per rgb is 6. ----why??----
	 */
	mach_state_p->register_state.ext_ge_config = 
		(mach_state_p->register_state.ext_ge_config  & 
		~MACH_EXT_GE_CONFIG_PIXEL_WIDTH &
		~MACH_EXT_GE_CONFIG_DAC_8_BIT_ENABLE ) |
		(MACH_EXT_GE_CONFIG_6_BIT_DAC_OPERATION |
		MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8);

	outw(MACH_REGISTER_EXT_GE_CONFIG,
		mach_state_p->register_state.ext_ge_config); 

	/*
	 * allocate space for the dac private state if necessary.
	 */
	if (mach_state_p->dac_state_p == NULL)
	{
		dac_registers_p = mach_state_p->dac_state_p = 
			allocate_and_clear_memory(
				  sizeof (struct mach_dac_tlc34075_extended_registers));

#if (defined(__DEBUG__))
		STAMP_OBJECT(MACH_DAC_TLC34075, dac_registers_p);
#endif
		/*
		 * Read in the values of the extended registers.
		 */
		MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
			register_state.ext_ge_config, 2);
		
		dac_registers_p->general_control = 
			inb(MACH_DAC_TLC34075_GENERAL_CONTROL);
		dac_registers_p->input_clock_sel =
			inb(MACH_DAC_TLC34075_INPUT_CLOCK_SEL);
		dac_registers_p->output_clock_sel =
			inb(MACH_DAC_TLC34075_OUTPUT_CLOCK_SEL);
		dac_registers_p->mux_control = 
			inb(MACH_DAC_TLC34075_MUX_CNTL);
		
		
		MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
			register_state.ext_ge_config, 3);
		dac_registers_p->palette_page = 
			inb(MACH_DAC_TLC34075_PALETTE_PAGE);
		dac_registers_p->test_register = 
			inb(MACH_DAC_TLC34075_TEST);
		
#if (defined(__DEBUG__))
		if (mach_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
"\t# DAC registers\n"
"\t{\n"
"\t\tgeneral_control = 0x%x\n"
"\t\tinput_clock_sel = 0x%x\n"
"\t\toutput_clock_sel = 0x%x\n"
"\t\tmux_control = 0x%x\n"
"\t\tpalette_page = 0x%x\n"
"\t\ttest = 0x%x\n"
"\t}\n",
						   dac_registers_p->general_control,
						   dac_registers_p->input_clock_sel,
						   dac_registers_p->output_clock_sel,
						   dac_registers_p->mux_control,
						   dac_registers_p->palette_page,
						   dac_registers_p->test_register);
		
		}
#endif

	}
#if (defined(__DEBUG__))
	else
	{
		dac_registers_p = mach_state_p->dac_state_p;
		
		ASSERT(IS_OBJECT_STAMPED(MACH_DAC_TLC34075, dac_registers_p));
	}
#endif

	switch(generic_state_p->screen_depth)
	{
	case 4 :
	case 8 :
		/*
		 * Set pixel delay to 3.
		 */
		mach_dac_program_misc_cntl(mach_state_p, 0, 3);
	
		/*
		 * enable third set of dac registers.
		 */
		MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
			register_state.ext_ge_config, 2);
	
		/*
		 * Set the input clock source to CLK0.
		 * Set the output clock to SCLK/1 and VCLK disabled.
		 * Set the MUX control to 8/16.
		 */
		outb(MACH_DAC_TLC34075_INPUT_CLOCK_SEL, 0);
		outb(MACH_DAC_TLC34075_OUTPUT_CLOCK_SEL, 0x30);
		outb(MACH_DAC_TLC34075_MUX_CNTL, 0x2d);
		
		dac_mask = (1 << visual_p->si_visual_p->SVdepth) - 1;

		/*
		 * Patch the appropriate get / set colormap pointers.
		 */
		generic_state_p->screen_functions_p->si_get_colormap =
			mach_colormap_get_colormap;
		generic_state_p->screen_functions_p->si_set_colormap =
			mach_colormap_set_colormap;

		break;
		
	case 16 :
		/* Disable overlay feature : WHAT DOES THIS MEAN? */
		outb(MACH_REGISTER_DAC_MASK, 0);

		/*FALLTHROUGH*/
	case 24 :

		dac_mask = 0;
		
		/*
		 * enable third set of dac registers.
		 */
		MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
            register_state.ext_ge_config, 2);
		
		/*
		 * blank adjust = 1, pixel delay = 0
		 */
		mach_dac_program_misc_cntl(mach_state_p, 1, 0);
		
		/*
		 * Input clock source is CLK3.
		 */
		outb(MACH_DAC_TLC34075_INPUT_CLOCK_SEL, 1);

		/*
		 * Output clock source is SCLK/1 and VCLK/1 for modes which
		 * require PCLK/2, set VCKL/2.
		 */
		if(mach_state_p->register_state.clock_sel & MACH_CLOCK_SEL_CLK_DIV_2)
		{
			/*
			 * Make clock div/1. Update both register and state.
			 */
			mach_state_p->register_state.clock_sel  &= 
				~MACH_CLOCK_SEL_CLK_DIV_2;
			outw(MACH_REGISTER_CLOCK_SEL, 
				 mach_state_p->register_state.clock_sel);
			
			if(generic_state_p->screen_displayed_width == 640)
			{
				/*
				 * set blank adjust to 2 pixel delay to 0.
				 */
				mach_dac_program_misc_cntl(mach_state_p, 2, 0);
			}
			outb(MACH_DAC_TLC34075_OUTPUT_CLOCK_SEL, 8);
		}
		else
		{
			outb(MACH_DAC_TLC34075_OUTPUT_CLOCK_SEL, 0);
		}
		outb(MACH_DAC_TLC34075_MUX_CNTL, 0x0d);

		/*
		 * Set the colormap handling methods.
		 */

		/*
		 * Patch the appropriate get / set colormap pointers.
		 */

		generic_state_p->screen_functions_p->si_get_colormap =
			mach_colormap_get_colormap_truecolor;
		generic_state_p->screen_functions_p->si_set_colormap =
			(SIBool (*)(SIint32, SIint32, SIColor *, SIint32))
			mach_colormap_operation_fail;

		break;

	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}

	/*
	 * Update the register state.
	 */
	mach_state_p->register_state.ext_ge_config = 
		(mach_state_p->register_state.ext_ge_config  & 
		 ~MACH_EXT_GE_CONFIG_PIXEL_WIDTH &
		 ~MACH_EXT_GE_CONFIG_16_BIT_COLOR_MODE &
		 ~MACH_EXT_GE_CONFIG_MULTIPLEX_PIXELS &
		 ~MACH_EXT_GE_CONFIG_24_BIT_COLOR_CONFIG &
		 ~MACH_EXT_GE_CONFIG_24_BIT_COLOR_ORDER &
		 ~MACH_EXT_GE_CONFIG_DAC_EXT_ADDR &
		 ~MACH_EXT_GE_CONFIG_DAC_8_BIT_ENABLE)
		| visual_p->dac_flags;


	outw(MACH_REGISTER_EXT_GE_CONFIG,
		 mach_state_p->register_state.ext_ge_config);
	
#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_initialize_tlc34075)\n"
"{\n"
"\text_ge_config = 0x%x\n"
"}\n",
					   mach_state_p->register_state.ext_ge_config);
	}
#endif

	/*
	 * Enable multiplex pixels for hi res, hi clock modes.
	 */
	if ( mach_state_p->clock_frequency >= 8000 )
	{
#ifdef DELETE
		ASSERT(generic_state_p->screen_depth <= 8);
#endif

		MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
			register_state.ext_ge_config, 2);

		/*
		 * *WARNING* : MAGIC NUMBERS
		 *
		 * output clock is SCLK/2 and VCLK/2
		 * MUX control is 8/16
		 * input clock source is CLK3 ( must be last )
		 * whatever that means...
		 */
		outb(MACH_DAC_TLC34075_OUTPUT_CLOCK_SEL, 0x9);
		outb(MACH_DAC_TLC34075_MUX_CNTL, 0x1d);
		outb(MACH_DAC_TLC34075_INPUT_CLOCK_SEL, 0x1);

		/*
		 * Update the register state with this value.
		 */
		mach_state_p->register_state.ext_ge_config = 
			(mach_state_p->register_state.ext_ge_config  & 
			 ~MACH_EXT_GE_CONFIG_PIXEL_WIDTH &
			 ~MACH_EXT_GE_CONFIG_16_BIT_COLOR_MODE &
			 ~MACH_EXT_GE_CONFIG_MULTIPLEX_PIXELS &
			 ~MACH_EXT_GE_CONFIG_24_BIT_COLOR_CONFIG &
			 ~MACH_EXT_GE_CONFIG_24_BIT_COLOR_ORDER &
			 ~MACH_EXT_GE_CONFIG_DAC_EXT_ADDR &
			 ~MACH_EXT_GE_CONFIG_DAC_8_BIT_ENABLE ) |
			 (MACH_EXT_GE_CONFIG_MULTIPLEX_PIXELS |
			  (visual_p->dac_flags &
			   (MACH_EXT_GE_CONFIG_DAC_8_BIT_ENABLE|
				MACH_EXT_GE_CONFIG_PIXEL_WIDTH)));

		outw(MACH_REGISTER_EXT_GE_CONFIG,
			 mach_state_p->register_state.ext_ge_config);
		
		/*
		 * set blank adjust as 1, pixel delay as 0.
		 */
		mach_dac_program_misc_cntl(mach_state_p, 1, 0);

#if (defined(__DEBUG__))
		if (mach_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_dac_initialize_tlc34075)\n"
"{\n"
"\tenabling mux : ext_ge_config = 0x%x\n"
"}\n",
						   mach_state_p->register_state.ext_ge_config);
			
			
		}
#endif
	}

	/*
	 * Restore normal dac access.
	 */
	MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
        register_state.ext_ge_config, 0);
	outb(MACH_REGISTER_DAC_MASK, dac_mask);

	/*
	 * restore clock sel value.
	 */
	MACH_PASSTHROUGH_8514(mach_state_p->register_state.clock_sel);
	
	return 1;
}


STATIC int
mach_dac_uninitialize_tlc34075(struct generic_screen_state *generic_state_p)
{
	struct mach_screen_state *mach_state_p =
		(struct mach_screen_state *) generic_state_p;

	int local_control;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, mach_state_p));

	ASSERT(generic_state_p->screen_number_of_visuals > 0);
	
	ASSERT(mach_state_p->dac_kind == MACH_DAC_TI_TLC_34075 ||
		   mach_state_p->dac_kind == MACH_DAC_ATI_68875_CFN);

#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(mach_dac_uninitialize_tlc34075)\n{\n");
	}
#endif


	if (mach_dac_program_low_clock(mach_state_p) == 0)
	{
		/*
		 * Programming a low clock frequency failed.
		 */
		return 0;
	}

	/*
	 * The dac programming goes through only if bpp is 8 and
	 * bits per rgb is 6. ----why??----
	 */
	mach_state_p->register_state.ext_ge_config = 
		(mach_state_p->register_state.ext_ge_config  & 
		~MACH_EXT_GE_CONFIG_PIXEL_WIDTH &
		~MACH_EXT_GE_CONFIG_DAC_8_BIT_ENABLE ) |
		(MACH_EXT_GE_CONFIG_6_BIT_DAC_OPERATION |
		MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8);

	outw(MACH_REGISTER_EXT_GE_CONFIG,
		mach_state_p->register_state.ext_ge_config); 
	
	/*BUG : is this supposed to be readable? */
	local_control = inw(MACH_REGISTER_LOCAL_CONTROL);
	outw(MACH_REGISTER_LOCAL_CONTROL, local_control |
		 MACH_LOCAL_CONTROL_DAC_BLANK_ADJ);

	/*
	 * Reset dac to 8 bit mode.
	 */
	MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
									register_state.ext_ge_config, 2);
	MACH_COLORMAP_DAC_ACCESS_DELAY();

	/*
	 * The default VGA mode set up for the DAC.
	 */
	/*MAGICNUMBERS*/

	outb(MACH_DAC_TLC34075_MUX_CNTL, 0x2D);
	MACH_COLORMAP_DAC_ACCESS_DELAY();

	outb(MACH_DAC_TLC34075_INPUT_CLOCK_SEL, 0);
	MACH_COLORMAP_DAC_ACCESS_DELAY();
	
 	outb(MACH_DAC_TLC34075_OUTPUT_CLOCK_SEL, 0x30);
	MACH_COLORMAP_DAC_ACCESS_DELAY();

	outb(MACH_DAC_TLC34075_GENERAL_CONTROL,0x03);
	MACH_COLORMAP_DAC_ACCESS_DELAY();
	
	MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
									register_state.ext_ge_config, 3);
	MACH_COLORMAP_DAC_ACCESS_DELAY();

	outb(MACH_DAC_TLC34075_PALETTE_PAGE, 0);
	MACH_COLORMAP_DAC_ACCESS_DELAY();

	
	MACH_ENABLE_EXTENDED_DAC_ACCESS(mach_state_p->
									register_state.ext_ge_config, 0);
	MACH_COLORMAP_DAC_ACCESS_DELAY();
	
	mach_dac_program_misc_cntl(mach_state_p, 0, 3);
	
	mach_state_p->register_state.ext_ge_config =
		((MACH_EXT_GE_CONFIG_6_BIT_DAC_OPERATION |
		  MACH_EXT_GE_CONFIG_RAMDAC_PAGE_0 |
		  MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8) |
		 (mach_state_p->register_state.ext_ge_config &
		  (MACH_EXT_GE_CONFIG_ALIAS_ENA |
		   MACH_EXT_GE_CONFIG_MONITOR_ALIAS)));

	outw(MACH_REGISTER_EXT_GE_CONFIG,
		 mach_state_p->register_state.ext_ge_config);

	outb(MACH_REGISTER_DAC_MASK, 0xFF);
	MACH_COLORMAP_DAC_ACCESS_DELAY();

#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"\text_ge_config = 0x%x\n"
"}\n",
					   mach_state_p->register_state.ext_ge_config);
		
	}
#endif
	
	return 1;
}

/*
 * IMS-G176 : the MACH 8 DAC and the lowest common denominator for a
 * lot of DACs in the market.
 */

STATIC int
mach_dac_initialize_ims_g176(struct generic_screen_state *state_p)
{
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 (struct mach_screen_state *)
							 state_p));
	
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_initialize_ims_g176)\n"
"{\n"
"\tstate_p = %p\n"
"\t(depth) = %d\n"
"}\n",
					   (void *) state_p,
					   state_p->screen_depth);
	}
#endif

	switch (state_p->screen_depth)
	{
	case 4:
		
		/*
		 * program the dac mask register for 4 bits.
		 */
		outb(MACH_REGISTER_DAC_MASK, MACH_DAC_MASK_4_BIT);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
		break;
	case 8 :
		/*
		 * program the dac mask register for 4 bits.
		 */
		outb(MACH_REGISTER_DAC_MASK, MACH_DAC_MASK_8_BIT);
		MACH_COLORMAP_DAC_ACCESS_DELAY();
		break;

	default :
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}
	
	/*
	 * Set the colormap programming methods.
	 */
	
	state_p->screen_functions_p->si_get_colormap =
		mach_colormap_get_colormap;
	state_p->screen_functions_p->si_set_colormap =
		mach_colormap_set_colormap;
	
	return 1;
	
}

/*
 * DAC programming.
 */
STATIC int
mach_dac_uninitialize_ims_g176(struct generic_screen_state *state_p)
{
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 ((struct mach_screen_state *) state_p)));
	
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_uninitialize_ims_g176)\n"
"{\n"
"\tstate_p = %p\n"
"}\n",
					   (void *) state_p);
	}
#endif
	
	/*
	 * Set the dac mask back to 8 bits wide.
	 */
	outb(MACH_REGISTER_DAC_MASK, MACH_DAC_MASK_8_BIT);
	MACH_COLORMAP_DAC_ACCESS_DELAY();
	return 1;
}

/*ARGSUSED*/
STATIC int
mach_dac_initialization_failed(struct mach_screen_state *state_p)
{
#if (defined(__DEBUG__))
	if (mach_colormap_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_dac_initialization_failed)\n"
"{\n"
"\tstate_p = %p\n"
"}\n",
					   (void *) state_p);
	}
#endif

	(void) fprintf(stderr, MACH_DAC_INITIALIZATION_FAILED_MESSAGE);
	
	return 0;
}

/*
 * Init and Uninit tables.
 */
STATIC int (*mach_dac_kind_to_init_function_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
		 INIT
#include "m_dacs.def"
#undef DEFINE_DAC
};

STATIC int (*mach_dac_kind_to_uninit_function_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
		 UNINIT
#include "m_dacs.def"
#undef DEFINE_DAC
};

STATIC void (*mach_dac_kind_to_get_color_method_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	    GET
#include "m_dacs.def"
#undef DEFINE_DAC
};

STATIC void (*mach_dac_kind_to_set_color_method_table[])() =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD,\
				   MAXCLOCKS, FLAGS16, FLAGS24, FLAGSRGB, DESC)\
		 DOWNLOAD
#include "m_dacs.def"
#undef DEFINE_DAC
};

#if (defined(EXTRA_FUNCTIONALITY))

/*
 * Initialization of static colormaps
 */

STATIC void
mach_colormap_initialize_static_colormap(
	struct generic_colormap *colormap_p,
	struct mach_options_structure *options_p)
{
	FILE *colormap_file_p;
	char *colormap_file_name_p;
	int colormap_file_line_number = 0;
	char line_buffer[DEFAULT_MACH_COLORMAP_DESCRIPTION_FILE_LINE_SIZE];
	int colormap_entry_count = 0;
	unsigned short *rgb_p;
	int rgb_shift_count;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_COLORMAP, colormap_p));

	ASSERT(!MACH_IS_PROGRAMMABLE_VISUAL(colormap_p->visual_p));
	
	rgb_p = colormap_p->rgb_values_p;

	colormap_file_name_p =
		(options_p->static_colormap_description_file) ?
		options_p->static_colormap_description_file :
		DEFAULT_MACH_COLORMAP_DESCRIPTION_FILE_NAME;

	rgb_shift_count = 
		(colormap_p->visual_p->si_visual_p->SVbitsrgb == 8) ? 8U : 10U;

	if ((colormap_file_p = fopen(colormap_file_name_p, "r")) == NULL)
	{
		(void) fprintf(stderr,
		   MACH_CANNOT_OPEN_COLORMAP_DESCRIPTION_FILE_MESSAGE,
					   colormap_file_name_p);
		perror("");
		
		/*
		 * Initialize Black and White appropriately and fill the rest
		 * of the colormap with shades of gray.
		 */
		
		*rgb_p++ = 0;		*rgb_p++ = 0;		*rgb_p++ = 0;
		*rgb_p++ = ~0;		*rgb_p++ = ~0;		*rgb_p++ = ~0;

		for(colormap_entry_count = 2; 
			colormap_entry_count < colormap_p->si_colormap.sz;
			colormap_entry_count++)
		{
			int tmp = (colormap_p->si_colormap.sz -
					   colormap_entry_count);
			
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
						  DEFAULT_MACH_COLORMAP_DESCRIPTION_LINE_FORMAT,
						  &red_value, &blue_value, &green_value) != 3)
				{
					(void) fprintf(stderr,
			   MACH_COLORMAP_DESCRIPTION_LINE_DOES_NOT_MATCH_FORMAT_MESSAGE,
								   colormap_file_name_p, 
								   colormap_file_line_number,
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

#endif /* EXTRA_FUNCTIONALITY */
										 
function void
mach_colormap__initialize__(SIScreenRec *si_screen_p,
							struct mach_options_structure *options_p)
{

	enum mach_visual_kind visual_kind;
	struct generic_colormap *colormap_p;
	struct mach_screen_state *screen_state_p =
		(struct mach_screen_state *) si_screen_p->vendorPriv;
	SIVisualP si_visual_p = 0;
	int tmp_count;
	int index;
	
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_screen_state *) screen_state_p));

	ASSERT((void *) generic_current_screen_state_p == (void *)
		   screen_state_p);
	
	/*
	 * Patch the screen saver function.
	 */
	si_screen_p->funcsPtr->si_vb_onoff = mach_video_blank_onoff;

	/*
	 * set the dac access delay value.
	 */

	mach_colormap_dac_access_delay_count =
		options_p->dac_access_delay_count;

	ASSERT(mach_colormap_dac_access_delay_count > 0);
	
	if (!screen_state_p->generic_state.screen_visuals_list_p)
	{
		enum mach_visual_kind screen_default_visual =
			MACH_VISUAL_NULL;
		unsigned int mask;
		enum mach_dac_mode dac_mode;
		int visual_count;
		int dac_flags = 0;

		unsigned int screen_8_4_bit_visual_list = /* save a copy */
			options_p->screen_8_4_bit_visual_list;
		
		
#if (defined(__DEBUG__))
		if (mach_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"{\n"
"\t# reading in visuals\n"
"\tvisual_list = 0x%x\n"
"}\n",
						   options_p->screen_8_4_bit_visual_list);
		}
#endif
		/*
		 * extract the screen default visual from the config file
		 * structure.  Add this to the list of 8-4 bit visual list.
		 */
		switch (screen_state_p->generic_state.screen_depth)
		{
		case 4 :
		case 8 :
			switch(screen_state_p->generic_state.screen_config_p->visual_type)
			{
			case STATICGRAY_AVAIL :
				screen_default_visual = MACH_VISUAL_STATIC_GRAY;
				screen_8_4_bit_visual_list |=
					MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY;
				break;
				
			case GRAYSCALE_AVAIL :
				screen_default_visual = MACH_VISUAL_GRAY_SCALE;
				screen_8_4_bit_visual_list |=
					MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE;
				break;

			case STATICCOLOR_AVAIL :
				screen_default_visual = MACH_VISUAL_STATIC_COLOR;
				screen_8_4_bit_visual_list |=
					MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR;
				break;

			case PSEUDOCOLOR_AVAIL :
				screen_default_visual = MACH_VISUAL_PSEUDO_COLOR;
				screen_8_4_bit_visual_list |=
					MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR;
				break;
				
			case DIRECTCOLOR_AVAIL :
			case TRUECOLOR_AVAIL :
				(void) fprintf(stderr,
					   MACH_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE,
					   screen_state_p->generic_state.screen_depth);
				break;
				
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
			}
			
			visual_count = 0;
			break;

		case 16 :				/* No `default' visual here */
		case 24 :
			screen_default_visual = MACH_VISUAL_NULL;
			visual_count = 1;
			break;
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
		
		/*
		 * Set the graphics engines idea of pixel width.
		 */
		switch (screen_state_p->generic_state.screen_depth)
		{
		case 4 :
			dac_flags |= MACH_EXT_GE_CONFIG_PIXEL_WIDTH_4;
			break;
		case 8 :
			dac_flags |= MACH_EXT_GE_CONFIG_PIXEL_WIDTH_8;
			break;
		case 16 :
			dac_flags |= MACH_EXT_GE_CONFIG_PIXEL_WIDTH_16;
			break;
		case 24 :
			dac_flags |= MACH_EXT_GE_CONFIG_PIXEL_WIDTH_24;
			break;
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
	
		/*
		 * Parse user supplied RGB width.
		 */
		switch(options_p->dac_rgb_width)
		{
		case MACH_OPTIONS_DAC_RGB_WIDTH_6 :
			
			dac_flags |= MACH_EXT_GE_CONFIG_6_BIT_DAC_OPERATION;
			
			if (!(mach_dac_kind_to_flags_rgb_table[screen_state_p->dac_kind] & 
				  MACH_DAC_BITS_PER_RGB_6))
			{
				(void) fprintf(stderr,
					   MACH_DAC_DOES_NOT_OFFER_X_BIT_RGB_MESSAGE,
					   mach_dac_kind_to_dac_name[screen_state_p->dac_kind],
					   6);
			}
			break;

		case MACH_OPTIONS_DAC_RGB_WIDTH_8 :

			dac_flags |= MACH_EXT_GE_CONFIG_8_BIT_DAC_OPERATION;

			if (!(mach_dac_kind_to_flags_rgb_table[screen_state_p->dac_kind] & 
				  MACH_DAC_BITS_PER_RGB_8))
			{
				(void) fprintf(stderr,
					   MACH_DAC_DOES_NOT_OFFER_X_BIT_RGB_MESSAGE,
					   mach_dac_kind_to_dac_name[screen_state_p->dac_kind],
					   8);
			}
			break;
			
		case MACH_OPTIONS_DAC_RGB_WIDTH_DEFAULT :

			/*
			 * For Hi color modes we use 8 bits per RGB.  Others
			 * use the VGA standard of 6 bits per RGB.
			 */
			dac_flags |= 
				(screen_state_p->generic_state.screen_depth > 8) ? 
					MACH_EXT_GE_CONFIG_8_BIT_DAC_OPERATION :
					MACH_EXT_GE_CONFIG_6_BIT_DAC_OPERATION;
			break;
			
		}
				
		/*
		 * Count the number of visuals the user wants.  Currently SI
		 * doesn't support having multiple visuals per screen, but I'm
		 * putting this code in so that we have less reworking to do
		 * when SI does get cleaned up.
		 */
		switch (screen_state_p->generic_state.screen_depth)
		{
		case 4 :
		case 8 :

			/*
			 * Walk a bit down the bitfield ...
			 */
			for (visual_count = 0, mask = 0x1;
				 mask;
				 mask <<= 1)
			{
				switch (mask & screen_8_4_bit_visual_list)
				{
					
				case 0:			/* ignore */
					break;
					
				case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR :
				case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR :
				case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE :
				case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY :
					/* 
					 * found a useable visual.
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

		case 16 :

			/*
			 * Since the DAC is in True color mode, only one visual is
			 * possible : at any given time.
			 */

			switch (options_p->dac_16_bit_color_mode)
			{
			case MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_555 :
				dac_mode = MACH_DAC_16_BIT_5_5_5_SUPPORTED;
				dac_flags |= MACH_EXT_GE_CONFIG_16_BIT_555_MODE;
				visual_kind = MACH_VISUAL_TRUE_COLOR_16_555;
				break;
				
			case MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_565 :
				dac_mode = MACH_DAC_16_BIT_5_6_5_SUPPORTED;
				dac_flags |= MACH_EXT_GE_CONFIG_16_BIT_565_MODE;
				visual_kind = MACH_VISUAL_TRUE_COLOR_16_565;
				break;
				
			case MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_655 :
				dac_mode = MACH_DAC_16_BIT_6_5_5_SUPPORTED;
				dac_flags |= MACH_EXT_GE_CONFIG_16_BIT_655_MODE;
				visual_kind = MACH_VISUAL_TRUE_COLOR_16_655;
				break;
				
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
			
			}

			/*
			 * Check if the dac mode is supported by the dac.
			 */
			if (!(mach_dac_kind_to_flags_16_table[screen_state_p->dac_kind] &
				  dac_mode))
			{
				(void) fprintf(stderr,
					   MACH_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE,
					   mach_dac_kind_to_dac_name[screen_state_p->dac_kind],
					   mach_dac_mode_to_dac_mode_description[dac_mode]);
				visual_count = 0;
			}
			break;
			
		case 24:

			/*
			 * Since the DAC is in True color mode, only one visual is
			 * possible at a time.
			 */

			switch(options_p->dac_24_bit_color_mode)
			{
			case MACH_OPTIONS_DAC_24_BIT_COLOR_MODE_RGBA :
				dac_mode = MACH_DAC_24_BIT_R_G_B_A_SUPPORTED;
				dac_flags |= 
					(MACH_EXT_GE_CONFIG_24_BIT_RGBA_MODE |
					 MACH_EXT_GE_CONFIG_24_BIT_4_BPP);
				visual_kind = MACH_VISUAL_TRUE_COLOR_24_RGBA;
				break;
				
			case MACH_OPTIONS_DAC_24_BIT_COLOR_MODE_ABGR :
				dac_mode = MACH_DAC_24_BIT_A_B_G_R_SUPPORTED;
				dac_flags |= 
					(MACH_EXT_GE_CONFIG_24_BIT_ABGR_MODE |
					 MACH_EXT_GE_CONFIG_24_BIT_4_BPP);
				visual_kind = MACH_VISUAL_TRUE_COLOR_24_ABGR;
				break;
				
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
			}

			/*
			 * Check if the dac mode is supported by the dac.
			 */
			if (!(mach_dac_kind_to_flags_24_table[screen_state_p->dac_kind] &
				  dac_mode))
			{
				(void) fprintf(stderr,
					   MACH_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE,
					   mach_dac_kind_to_dac_name[screen_state_p->dac_kind],
					   mach_dac_mode_to_dac_mode_description[dac_mode]);
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
					&(screen_state_p->
					  generic_state.screen_visuals_list_p[tmp_count]);

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
					mach_dac_kind_to_get_color_method_table
						[screen_state_p->dac_kind];

				/*
				 * The set-colormap-method is valid only for visuals
				 * with a programmable color map.
				 */

				if (MACH_IS_DAC_PROGRAMMABLE(tmp_visual_p))
				{
					tmp_visual_p->set_color_method_p =
						mach_dac_kind_to_set_color_method_table
							[screen_state_p->dac_kind]; 
				}
				
				
			}
			
			/*
			 * Allocate space for colormap pointers.
 			 */
			screen_state_p->generic_state.screen_colormaps_pp =
				allocate_and_clear_memory(visual_count * 
										  sizeof(struct generic_colormap *));


#if (defined(__DEBUG__))
			if (mach_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"{\n"
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
			case 24 :
				
				/*
				 * Only one visual around.
				 */
				tmp_count = 0;
				tmp_visual_p = visuals_list_p;
				
				*si_visual_p = mach_visuals_table[visual_kind];
				si_visual_p->SVbitsrgb = (dac_flags &
					MACH_EXT_GE_CONFIG_8_BIT_DAC_OPERATION) ? 8 : 6;
				
				/*
				 * Allocate the number of colormaps specified in
				 * the visual.
				 */
				screen_state_p->generic_state.screen_colormaps_pp[tmp_count] =
					colormap_p =
						allocate_and_clear_memory(si_visual_p->SVcmapcnt *
									  sizeof(struct generic_colormap));
				
#if (defined(__DEBUG__))
				if (mach_colormap_debug)
				{
					(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"{\n"
"\tvisual_kind = %s\n"
"\tsi_visual_p = %p\n"
"\ttmp_visual_p = %p\n"
"\tcolormap_list_p = %p\n"
"\tn_colormaps = %ld\n"
"}\n",
						   mach_visual_kind_to_visual_kind_dump[visual_kind],
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
					
					/*
					 * Since these visuals supported in this mode are
					 * not programmable,  we don't need to fill the
					 * colormap with default values
					 */
					 
#if (defined(__DEBUG__))
					if (mach_colormap_debug)
					{
						(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"\t{\n"
"\t\tvisual = %d\n"
"\t\tsize = %d\n"
"\t\trgb_values_p = %p\n"
"\t}\n",
									   colormap_p[index].
									   si_colormap.visual,
									   colormap_p[index].si_colormap.sz,
									   (void *)
									   colormap_p[index].rgb_values_p);
					}

					STAMP_OBJECT(GENERIC_COLORMAP,
								 &(colormap_p[index]));
#endif
				}

#if (defined(__DEBUG__))
				if (mach_colormap_debug)
				{
					(void) fprintf(debug_stream_p,"}\n");
				}
#endif

				break;
				
			case 4:
			case 8:
				
				tmp_count = 0;
				tmp_visual_p = visuals_list_p;
				
				/*
				 * First fill in the default visual as the very first one.
				 */

				*si_visual_p = mach_visuals_table[screen_default_visual];
				si_visual_p->SVdepth =
					screen_state_p->generic_state.screen_depth;
				si_visual_p->SVcmapsz = 
					(1U << si_visual_p->SVdepth);
				si_visual_p->SVbitsrgb = 
					(dac_flags &
					 MACH_EXT_GE_CONFIG_8_BIT_DAC_OPERATION) ?
						 8 : 6;
				/*
				 * Allocate the number of colormaps specified in
				 * the visual.
				 */
				screen_state_p->generic_state.screen_colormaps_pp[tmp_count] =
					colormap_p =
						allocate_and_clear_memory(si_visual_p->SVcmapcnt *
										  sizeof(struct generic_colormap));
				
#if (defined(__DEBUG__))
				if (mach_colormap_debug)
				{
					(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"{\n"
"\tvisual_kind = %s (DEFAULT)\n"
"\tsi_visual_p = %p\n"
"\ttmp_visual_p = %p\n"
"\tcolormap_list_p = %p\n"
"\tn_colormaps = %ld\n"
"}\n",
								   mach_visual_kind_to_visual_kind_dump[screen_default_visual],
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

					if (mach_colormap_debug)
					{
						(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"\t{\n"
"\t\tvisual = %d\n"
"\t\tsize = %d\n"
"\t\trgb_values_p = %p\n"
"\t}\n",
									   colormap_p[index].
									   si_colormap.visual,
									   colormap_p[index].si_colormap.sz,
									   (void *)
									   colormap_p[index].rgb_values_p);
					}

					STAMP_OBJECT(GENERIC_COLORMAP,
								 &(colormap_p[index]));

#endif
 
#if (defined(EXTRA_FUNCTIONALITY))

					/*
					 * In 4/8 bit modes, the hardware supports a
					 * programmable visual, but for visuals other than
					 * pseudocolor, we are trying to fake
					 * `staticness'.  For such visuals, fill up the
					 * default colormap and program the DAC
					 * accordingly.
					 */

					if (!MACH_IS_PROGRAMMABLE_VISUAL(tmp_visual_p))
					{
						mach_colormap_initialize_static_colormap(
							&(colormap_p[index]),
							options_p);
					}

#endif /* EXTRA_FUNCTIONALITY */
					
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
						
					case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR :
						visual_kind = MACH_VISUAL_PSEUDO_COLOR;
						break;

					case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR :
						visual_kind = MACH_VISUAL_STATIC_COLOR;
						break;
						
					case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY :
						visual_kind = MACH_VISUAL_STATIC_GRAY;
						break;

					case MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE :
						visual_kind = MACH_VISUAL_GRAY_SCALE;
						break;
							
					default :
						(void) fprintf(stderr,
						   MACH_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE,
						   screen_state_p->generic_state.screen_depth);
						/*FALLTHROUGH*/
					case 0 :				/* continue */
						continue;
					}
					
					if (visual_kind == screen_default_visual)
					{
						continue; /* we have already handled this */
					}

					*si_visual_p = mach_visuals_table[visual_kind];
					si_visual_p->SVdepth =
						screen_state_p->generic_state.screen_depth;
					si_visual_p->SVcmapsz = 
						(1U << si_visual_p->SVdepth);
					si_visual_p->SVbitsrgb = (dac_flags &
						MACH_EXT_GE_CONFIG_8_BIT_DAC_OPERATION) ?
							8 : 6;
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
					if (mach_colormap_debug)
					{
						(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"{\n"
"\tvisual_kind = %s\n"
"\tsi_visual_p = %p\n"
"\ttmp_visual_p = %p\n"
"\tcolormap_list_p = %p\n"
"\tn_colormaps = %ld\n"
"}\n",
									   mach_visual_kind_to_visual_kind_dump[visual_kind],
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
						if (mach_colormap_debug)
						{
							(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"\t{\n"
"\t\tvisual = %d\n"
"\t\tsize = %d\n"
"\t\trgb_values_p = %p\n"
"\t}\n",
										   colormap_p[index].
										   	si_colormap.visual,
										   colormap_p[index].si_colormap.sz,
										   (void *)
										   colormap_p[index].rgb_values_p);
						}

						STAMP_OBJECT(GENERIC_COLORMAP,
									 &(colormap_p[index]));
#endif
					}
					

#if (defined(__DEBUG__))
					if (mach_colormap_debug)
					{
						(void) fprintf(debug_stream_p,
									   "}\n");
					}
#endif

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
			if (mach_colormap_debug)
			{
				(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"{\n"
"\tNo visuals read in.\n"
"}\n"
							   );
			}
#endif
			si_screen_p->flagsPtr->SIvisualCNT = 0;
		}
	}
#if (defined(__DEBUG__))
	else
	{
		if (mach_colormap_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_colormap__initialize__)\n"
"{\n"
"\tvisuals already read in by board layer.\n"
"}\n"
						   );
		}
	}
#endif

	ASSERT(screen_state_p->dac_kind >= MACH_DAC_ATI_68830 &&
		   screen_state_p->dac_kind < MACH_DAC_UNKNOWN);

	/*
	 * Patch in the DAC init and uninit functions.
	 */

	if (screen_state_p->register_state.generic_state.dac_init == NULL)
	{
		screen_state_p->register_state.generic_state.dac_init = 
			mach_dac_kind_to_init_function_table[screen_state_p->dac_kind];
	}
	
	if (screen_state_p->register_state.generic_state.dac_uninit ==
		NULL)
	{
		screen_state_p->register_state.generic_state.dac_uninit =
			mach_dac_kind_to_uninit_function_table[screen_state_p->dac_kind];
	}

}

