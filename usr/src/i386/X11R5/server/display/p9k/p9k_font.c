/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_font.c	1.2"
/***
 ***	NAME
 ***
 ***		p9000_font.c : Font handling code for the P9000 display
 ***	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k_font.h"
 ***
 ***	DESCRIPTION
 ***
 ***	All kinds of fonts are accepted by the font drawing code --
 ***	there are no builtin limits on the glyph sizes, number of
 ***	characters in a font etc.  Options exist to control these --
 ***	please see the option description files.  Right-to-left
 ***	drawing fonts are also handled, though this section of the
 ***	code has not been fully tested.
 ***
 ***	Font support consists of downloading the glyphs into system
 ***	memory.  Packing of the glyph bits is done at this stage to
 ***	speed up drawing.
 ***
 ***	Font drawing is done using the P9000's pixel1 command to
 ***	perform pixel expansion. 
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
 ***	p9k_opt.gen : option description file for the P9000 display
 ***				library.
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
#include "p9k_opt.h"

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
export enum debug_level p9000_font_debug = DEBUG_LEVEL_NONE;	
export enum debug_level p9000_font_string_debug = DEBUG_LEVEL_NONE; 
								/* dump incoming strings */
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/*
 * Cached glyph information.
 *
 */

struct p9000_glyph
{

	/*
	 * Saved SI glyph information
	 */

	SIGlyph si_glyph;

	/*
	 * number of long word transfers for the bits of this glyph.  This
	 * number can be zero for a small glyph.
	 */

	int glyph_whole_transfers;
	
	/*
	 * The width of the last line of the glyph.
	 */

	int glyph_last_width;

	/*
	 * Flag to indicate if the glyphs ascent and descent extends over
	 * that of the font.
	 */
	
	boolean is_out_of_font_bounds;
	

	/*
	 * pointer to the start of the packed bits for this glyph.  This
	 * points into the per-font packed glyph bits array.
	 */

	unsigned long *glyph_packed_bits_p;

#if (defined(__DEBUG__))
	int stamp;
#endif
};


/*
 * Font kinds.
 */

#define DEFINE_FONT_TYPES()\
	DEFINE_FONT_TYPE(NULL),\
	DEFINE_FONT_TYPE(TERMINAL_FONT),\
	DEFINE_FONT_TYPE(NON_TERMINAL_FONT),\
	DEFINE_FONT_TYPE(COUNT)

enum p9000_font_kind
{

#define DEFINE_FONT_TYPE(NAME)\
	P9000_FONT_KIND_ ## NAME
		DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE

};


/*
 * Structure of a downloaded font.
 */

struct p9000_font
{

	/*
	 * SI's font information
	 */

	SIFontInfo si_font_info;
	
	/*
	 * The type of this font.
	 */

	enum p9000_font_kind font_kind;
	
	/*
	 * Pointer to the array of p9000_glyph structures.
	 */

	struct p9000_glyph *font_glyphs_p;
	
	/*
	 * Pointer to the packed glyph bits for all the glyphs.
	 */

	unsigned long *font_glyph_packed_bits_p;

	/*
	 * Maximum size used by any one glyph.
	 */
	
	int max_glyph_size;
	
	/*
	 * Pointer to draw function for this font.
	 */

	void (*draw_glyphs)
		(const struct p9000_font *const font_p,
		 const short *glyph_start_p, 
		 const short *glyph_end_p,
		 const int x_origin,
		 const int y_origin,
		 const int x_end,
		 const unsigned short minterm, 
		 const int draw_mode);

#if (defined(__DEBUG__))
	int stamp;
#endif

};

/*
 * The state of the font handling code.
 */

struct p9000_font_state
{

	/*
	 * Parameters controlling the downloadability and offscreen
	 * cacheability of this font.
	 */

	int max_number_of_downloadable_fonts;
	int max_supported_glyph_width;
	int max_supported_glyph_height;

	/*
	 * List of downloaded fonts.
	 */

	struct p9000_font **font_list_pp;
	
#if (defined(__DEBUG__))
	int stamp;
#endif

};

/***
 ***	Includes.
 ***/

#include "p9k_state.h"
#include "p9k.h"
#include "p9k_gs.h"
#include "p9k_asm.h"
#include "p9k_regs.h"

/***
 ***	Constants.
 ***/

/*
 * Flags controlling the drawing.
 */

#define P9000_FONT_DO_POLY_TEXT 		(0x0001 << 0U)
#define P9000_FONT_DO_IMAGE_TEXT				(0x0001 << 1U)

#if (defined(__DEBUG__))

STATIC const char *const p9000_font_kind_to_dump[] = 
{

#define DEFINE_FONT_TYPE(NAME)\
	# NAME
	DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE

};

#endif

#if (defined(__DEBUG__))
#define P9000_FONT_DEBUG_STAMP 		0xdeadbeef
#endif

/*
 * Stamps.
 */

#define P9000_GLYPH_STAMP									\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +	\
	 ('_' << 4) + ('G' << 5) + ('L' << 6) + ('Y' << 7) +	\
	 ('P' << 8) + ('H' << 9) + ('_' << 10) + ('S' << 11) +	\
	 ('T' << 12) + ('A' << 13) + ('M' << 14))

#define P9000_FONT_STAMP									\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +	\
	 ('_' << 4) + ('F' << 5) + ('O' << 6) + ('N' << 7) +	\
	 ('T' << 8) + ('_' << 9) + ('S' << 10) + ('T' << 11) +	\
	 ('A' << 12) + ('M' << 13))

#define P9000_FONT_STATE_STAMP									\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +		\
	 ('_' << 4) + ('F' << 5) + ('O' << 6) + ('N' << 7) +		\
	 ('T' << 8) + ('_' << 9) + ('S' << 10) + ('T' << 11) +		\
	 ('A' << 12) + ('T' << 13) + ('E' << 14) + ('_' << 15) +	\
	 ('S' << 16) + ('T' << 17) + ('A' << 18) + ('M' << 19))

/***
 ***	Macros.
 ***/

#define P9000_CURRENT_FONT_STATE_DECLARE()\
	struct p9000_font_state *font_state_p = \
		screen_state_p->font_state_p

/***
 ***	Functions.
 ***/


#if (defined(__DEBUG__))
#include <ctype.h>

STATIC void
p9000_font_glyph_list_dump_helper(
	int font_id,
	int glyph_count, 
	short *glyph_list_p)
{

	(void) fprintf(debug_stream_p,"%d\t\"", font_id);
	
	while(--glyph_count > 0)
	{
		unsigned int c = *glyph_list_p;
		(void) fprintf(debug_stream_p, 
					   (isalnum(c) ? "%c " : "\\%3.3o "), c);
		glyph_list_p++;
	}
	(void) fprintf(debug_stream_p, "\"\n");
}
#endif /* DEBUG */

/*
 * p9000_font_draw_terminal_glyphs
 *
 * Draw terminal glyphs.
 */

STATIC void
p9000_font_draw_terminal_glyphs(const struct p9000_font *const font_p,
								const short *glyph_start_p,
								const short *glyph_end_p,
								const int x_origin,
								const int y_origin,
								const int x_end,
								const unsigned short minterm,
								const int draw_mode)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	
	const struct p9000_glyph *const font_glyphs_p =
		font_p->font_glyphs_p;

	const unsigned int swap_flag = /* swapping instructions to the
									  graphics processor */ 
		P9000_ADDRESS_SWAP_HALF_WORDS |
		P9000_ADDRESS_SWAP_BYTES |
		P9000_ADDRESS_SWAP_BITS;

	int y_top = y_origin - 
		font_glyphs_p->si_glyph.SFascent;
	
	const int max_glyph_size = font_p->max_glyph_size;
	
	const unsigned long *font_glyph_packed_bits_p =
		font_p->font_glyph_packed_bits_p;

	const int glyph_width = 
		font_glyphs_p->si_glyph.SFwidth;
	
	const int glyph_whole_transfers = 
		font_glyphs_p->glyph_whole_transfers;
	
	const int glyph_last_width =
		font_glyphs_p->glyph_last_width;
				 
	ASSERT(font_p->font_kind == P9000_FONT_KIND_TERMINAL_FONT);

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_font, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_font_draw_terminal_glyphs)\n"
					   "{\n"
					   "\tfont_p = 0x%p\n"
					   "\tglyph_start_p = 0x%p\n"
					   "\tglyph_end_p = 0x%p\n"
					   "\tx_origin = %d\n"
					   "\ty_origin = %d\n"
					   "}\n",
					   (void *) font_p, (void *) glyph_start_p,
					   (void *) glyph_end_p, x_origin, y_origin);
	}
#endif

	ASSERT(glyph_start_p <= glyph_end_p);

	/*
	 * Setup the registers for the data transfer.
	 */

	/*
	 * scanline increment.
	 */

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_ABS,
		1);
	

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		x_origin);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_ABS,
		y_top);
	
	/*
	 * Program the minterm.
	 */
	 
	P9000_STATE_SET_RASTER(register_state_p, minterm);
	
	if (max_glyph_size == P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE)
	{
		
		do
		{
			const short glyph_index = *glyph_start_p;

			/*
			 * Compute the glyph bits address.
			 */

			register const unsigned long *bits_p = 
				font_glyph_packed_bits_p + 
					(glyph_index <<
					 P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE_SHIFT);

#if (defined(__DEBUG__))
			const struct p9000_glyph *const glyph_p =
				font_glyphs_p + glyph_index;
#endif

			ASSERT(bits_p == glyph_p->glyph_packed_bits_p);
		
			/*
			 * Advance destination stipple rectangle.
			 */

			P9000_NEXT_PIXELS_COMMAND(glyph_width);
		
			{
				register int tmp = glyph_whole_transfers;
			
				if (tmp > 0)
				{
					do
					{
						P9000_PIXEL1_COMMAND(*bits_p, 32, swap_flag);
						bits_p ++;
					} while (--tmp > 0);
				}
						
				/*
				 * Pump the last few bits of the character.
				 */

				P9000_PIXEL1_COMMAND(*bits_p, glyph_last_width,
									 swap_flag);
		
			}

		} while (++glyph_start_p < glyph_end_p);
		
	}
	else
	{
		do
		{
			const short glyph_index = *glyph_start_p;

			const struct p9000_glyph *const glyph_p =
				font_glyphs_p + glyph_index;

			register const unsigned long *bits_p = 
				glyph_p->glyph_packed_bits_p;
			
			/*
			 * Advance destination stipple rectangle.
			 */

			P9000_NEXT_PIXELS_COMMAND(glyph_width);
		
			{
				register int tmp = glyph_whole_transfers;
			
				if (tmp > 0)
				{
					do
					{
						P9000_PIXEL1_COMMAND(*bits_p, 32, swap_flag);
						bits_p ++;
					} while (--tmp > 0);
				}
						
				/*
				 * Pump the last few bits of the character.
				 */

				P9000_PIXEL1_COMMAND(*bits_p, glyph_last_width,
									 swap_flag);
		
			}

		} while (++glyph_start_p < glyph_end_p);
	}
}

/*
 * p9000_font_draw_non_terminal_glyphs
 *
 * Draw non-terminal glyphs onto screen.
 */

STATIC void
p9000_font_draw_non_terminal_glyphs(const struct p9000_font *const font_p,
									const short *glyph_start_p, 
									const short *glyph_end_p,
									const int x_origin,
									const int y_origin,
									const int x_end,
									const unsigned short minterm,
									const int draw_mode)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	const struct p9000_glyph *const font_glyphs_p =
		font_p->font_glyphs_p;

	const unsigned int swap_flag = /* swapping instructions to the
									  graphics processor */ 
		P9000_ADDRESS_SWAP_HALF_WORDS |
		P9000_ADDRESS_SWAP_BYTES |
		P9000_ADDRESS_SWAP_BITS;

	const int max_glyph_size = font_p->max_glyph_size;
	
	const unsigned long *font_glyph_packed_bits_p =
		font_p->font_glyph_packed_bits_p;
	
	register int x = x_origin;
	
	ASSERT(font_p->font_kind == P9000_FONT_KIND_NON_TERMINAL_FONT ||
		   font_p->font_kind == P9000_FONT_KIND_TERMINAL_FONT);

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_font, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_font_draw_non_terminal_glyphs)\n"
					   "{\n"
					   "\tfont_p = 0x%p\n"
					   "\tglyph_start_p = 0x%p\n"
					   "\tglyph_end_p = 0x%p\n"
					   "\tx_origin = %d\n"
					   "\ty_origin = %d\n"
					   "}\n",
					   (void *) font_p, (void *) glyph_start_p,
					   (void *) glyph_end_p, x_origin, y_origin);
	}
#endif

	ASSERT(glyph_start_p <= glyph_end_p);

	/*
	 * Setup the registers for the data transfer.
	 */

	/*
	 * scanline increment.
	 */

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_ABS,
		1);
	

	if (draw_mode == SGOPQStipple)
	{

		/*
		 * When opaque stippling non-terminal glyphs, we can optimize
		 * somewhat by using the graphics engines transparent
		 * stippling mode only for those glyphs which extend beyond
		 * the background rectangle.
		 */

		unsigned int opaque_stipple_minterm = 
			(P9000_SOURCE_MASK & P9000_FOREGROUND_COLOR_MASK) |
			(~P9000_SOURCE_MASK & P9000_BACKGROUND_COLOR_MASK);
			
		do
		{
			const short glyph_index = *glyph_start_p;
			const struct p9000_glyph *const glyph_p = 
				font_glyphs_p + glyph_index;
		
			const int x_left =
				x + glyph_p->si_glyph.SFlbearing; /* inclusive */

			const int glyph_width  = 
				glyph_p->si_glyph.SFrbearing -
					glyph_p->si_glyph.SFlbearing; /* exclusive */

			const int y_top = 
				y_origin - glyph_p->si_glyph.SFascent; /* inclusive */

			const int x_right = x_left + glyph_width;
			
			/*
			 * The bits for the glyph.
			 */

			register const unsigned long *bits_p = 
				glyph_p->glyph_packed_bits_p;
		
#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
			{
				(void) fprintf(debug_stream_p,
							   "\t{\n"
							   "\t\tglyph_index = %d\n"
							   "\t\tx = %d\n"
							   "\t\tx_left = %d\n"
							   "\t\tglyph_width = %d\n"
							   "\t\ty_top = %d\n"
							   "\t}\n",
							   glyph_index, x, x_left, glyph_width,
							   y_top);
			}
#endif
		
			/*
			 * Adjust the origin for the next glyph
			 */

			x += glyph_p->si_glyph.SFwidth;

			/*
			 * X start : goes to x0 and x1.
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_0,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_ABS,
				x_left);

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_ABS,
				x_left);
		
			/*
			 * Right edge of transferred block: goes to x2.
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_2,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_ABS,
				x_right);

			/*
			 * Y top : goes to y1.
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_Y_32,
				P9000_PARAMETER_COORDINATE_ABS,
				y_top);
		
			if (x_left < x_origin || glyph_p->is_out_of_font_bounds ||
				x_right > x_end)
			{

				/*
				 * Some portion of the glyph extends beyond the background
				 * rectangle.  We need to transparent stipple the
				 * glyph bits for this glyph.
				 */

				P9000_STATE_SET_RASTER(register_state_p, minterm);
				
#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
				{
					(void) fprintf(debug_stream_p,
								   "\t# glyph out of bounds.\n"
								   );
				}
#endif

			}
			else
			{

				/*
				 * we can opaque stipple the glyph as it lies entirely in 
				 * the background rectangle.
				 */

				P9000_STATE_SET_RASTER(register_state_p, 
									   opaque_stipple_minterm);
#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
				{
					(void) fprintf(debug_stream_p,
								   "\t# glyph in bounds.\n"
								   );
				}
#endif

			}
		
			/*
			 * Draw the glyph.  Transfer as many whole transfers as there
			 * in the glyph.
			 */

			/*
			 * for a non-null glyph glyph_last_width will be non zero
			 */

			if (glyph_p->glyph_last_width > 0)
			{
				register int tmp = glyph_p->glyph_whole_transfers;
			
				if (tmp > 0)
				{
					do
					{
						P9000_PIXEL1_COMMAND(*bits_p, 32, swap_flag);
						bits_p++;
					} while (--tmp > 0);
				}
				

				/*
				 * Pump the last few bits of the character.
				 */

				P9000_PIXEL1_COMMAND(*bits_p, glyph_p->glyph_last_width,
									 swap_flag);
		
			}

		} while (++glyph_start_p < glyph_end_p);
	}
	else						/* transparent stippling */
	{
		
		/*
		 * Program the minterm.
		 */
	
		P9000_STATE_SET_RASTER(register_state_p, minterm);
	
		do
		{
			const short glyph_index = *glyph_start_p;
			const struct p9000_glyph *const glyph_p = 
				font_glyphs_p + glyph_index;
		
			const int x_left =
				x + glyph_p->si_glyph.SFlbearing; /* inclusive */

			const int glyph_width  = 
				glyph_p->si_glyph.SFrbearing -
					glyph_p->si_glyph.SFlbearing; /* exclusive */

			const int y_top = 
				y_origin - glyph_p->si_glyph.SFascent; /* inclusive */

			/*
			 * The bits for the glyph.
			 */

			register const unsigned long *bits_p = 
				glyph_p->glyph_packed_bits_p;
		
#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
			{
				(void) fprintf(debug_stream_p,
							   "\t{\n"
							   "\t\tglyph_index = %d\n"
							   "\t\tx = %d\n"
							   "\t\tx_left = %d\n"
							   "\t\tglyph_width = %d\n"
							   "\t\ty_top = %d\n"
							   "\t}\n",
							   glyph_index, x, x_left, glyph_width,
							   y_top);
			}
#endif
		
			/*
			 * Adjust the origin for the next glyph
			 */

			x += glyph_p->si_glyph.SFwidth;

		
			/*
			 * X start : goes to x0 and x1.
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_0,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_ABS,
				x_left);

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_ABS,
				x_left);
		
			/*
			 * Right edge of transferred block: goes to x2.
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_2,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_ABS,
				x_left + glyph_width);

			/*
			 * Y top : goes to y1.
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_Y_32,
				P9000_PARAMETER_COORDINATE_ABS,
				y_top);
		
		
			/*
			 * Draw the glyph.  Transfer as many whole transfers as there
			 * in the glyph.
			 */

			if (glyph_p->glyph_last_width > 0)
			{
				register int tmp = glyph_p->glyph_whole_transfers;
			
				if (tmp > 0)
				{
					do
					{
						P9000_PIXEL1_COMMAND(*bits_p, 32, swap_flag);
						bits_p++;
					} while (--tmp > 0);
				}
					
				/*
				 * Pump the last few bits of the character.
				 */

				P9000_PIXEL1_COMMAND(*bits_p, glyph_p->glyph_last_width,
									 swap_flag);
		
			}

		} while (++glyph_start_p < glyph_end_p);
	}
}

/*
 * p9000_font_is_font_downloadable
 *
 * Check if a font can be downloaded into the SDD.
 */

STATIC SIBool
p9000_font_is_font_downloadable(
	SIint32 font_index, 
	SIFontInfoP fontinfo_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_FONT_STATE, font_state_p));
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_font, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_font_is_font_downloadable)\n"
					   "{\n"
					   "\tfont_index = %ld\n"
					   "\tfontinfo_p = %p\n"
					   "}\n",
					   font_index, (void *) fontinfo_p);
	}
#endif
	

	/*
	 * Check if the index is in bounds.
	 * Check if the font is of a size suitable for downloading.
	 */

	if ((font_index < 0 || font_index >=
		font_state_p->max_number_of_downloadable_fonts) ||
		((fontinfo_p->SFmax.SFascent + fontinfo_p->SFmax.SFdescent) >
		  font_state_p->max_supported_glyph_height) ||
		((fontinfo_p->SFmax.SFrbearing - fontinfo_p->SFmin.SFlbearing) >
		  font_state_p->max_supported_glyph_width))
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
				   "(p9000_font_is_font_downloadable)\n"
				   "{\n"
				   "\t# index/font size is out of bounds.\n"
				   "}\n");
		}
#endif

		return (SI_FAIL);
	}

	/*
	 * Do the following checks also.
	 * Option says don't draw terminal fonts and if we are downloaded a
	 * terminal font or option says don't draw non terminal font and we
	 * are downloaded a non-terminal font (any font with SFTerminal not 
	 * set) return failure.
	 */

	if ((!(screen_state_p->options_p->fontdraw_options & 
	 	   P9000_OPTIONS_FONTDRAW_OPTIONS_USE_TERMINAL_FONTS) &&
		 (fontinfo_p->SFflag & SFTerminalFont)) ||
	    (!(screen_state_p->options_p->fontdraw_options & 
	 	   P9000_OPTIONS_FONTDRAW_OPTIONS_USE_NON_TERMINAL_FONTS) &&
		 (!(fontinfo_p->SFflag & SFTerminalFont))) ||
		((screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font > 0) && 
		 (fontinfo_p->SFnumglyph > screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font)))
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "(p9000_font_is_font_downloadable)\n"
						   "{\n"
						   "\t# font-type not downloadable by option.\n"
						   "\tfont type = %s\n"
						   "\tnumber of glyphs = %ld\n"
						   "}\n",
						   (fontinfo_p->SFflag & SFTerminalFont) ?
						   "Terminal Font.\n" : "NonTerminal Font.\n",
						   fontinfo_p->SFnumglyph);
		}
#endif

		return (SI_FAIL);

	}

	return (SI_SUCCEED);
}

/*
 * Utility macro.
 */

#define ACCUMULATE_BITS(destination_bits_p,							\
						current_bit_position,						\
						source_bits_p,								\
						source_width)								\
if ((DEFAULT_SYSTEM_LONG_WORD_SIZE - current_bit_position) >=		\
	source_width)													\
{																	\
	unsigned long _tmp =											\
		((*(source_bits_p)) << (32 - (source_width))) >>			\
		(32 - (source_width) - (current_bit_position));				\
	*(destination_bits_p) |= _tmp;									\
	(current_bit_position) += (source_width);						\
	if (current_bit_position == DEFAULT_SYSTEM_LONG_WORD_SIZE)		\
	{																\
		current_bit_position = 0;									\
		(destination_bits_p)++;										\
	}																\
	ASSERT((current_bit_position) >= 0 &&							\
		   (current_bit_position) < DEFAULT_SYSTEM_LONG_WORD_SIZE);	\
}																	\
else																\
{																	\
	unsigned long _tmp =											\
		((*(source_bits_p)) << (32 - (source_width))) >>			\
		(32 - (source_width));										\
																	\
	*(destination_bits_p)++ |=										\
		(_tmp << (current_bit_position));							\
	*(destination_bits_p) |=										\
		(_tmp >> (32 - (current_bit_position)));					\
	(current_bit_position) =										\
		(source_width - (32 - (current_bit_position)));				\
	ASSERT((current_bit_position) >= 0 &&							\
		   (current_bit_position) <= DEFAULT_SYSTEM_LONG_WORD_SIZE);	\
}


/*
 * p9000_font_download_font
 *
 * Download a font from the SI layer into the SDD's system memory.
 */

STATIC SIBool
p9000_font_download_font(
	SIint32 font_index,
	SIFontInfoP font_info_p,
	SIGlyphP glyph_list_p)
{
	struct p9000_font	*font_p;
	int					max_glyph_size_in_bits;	/* number of bits in glyph */
	int 				max_glyph_size;	/* number of long words in glyph */

	const SIGlyphP		last_si_glyph_p = /* fence */
							glyph_list_p + font_info_p->SFnumglyph;
	SIGlyphP			si_glyph_p;
	struct p9000_glyph	*glyph_p;
	unsigned long		*glyph_packed_bits_p;

	
#if (defined(__DEBUG__))
	const unsigned int           p9000_debug_stamp = P9000_FONT_DEBUG_STAMP;
#endif

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_FONT_STATE, font_state_p));

	ASSERT(sizeof(unsigned long) == 4);

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_font, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_font_download_font)\n"
					   "{\n"
					   "\tfont_index = %ld\n"
					   "\tfont_info_p = %p\n"
					   "\tglyph_list_p = %p\n"
					   "}\n",
					   font_index, (void *) font_info_p,
					   (void *) glyph_list_p);
	}
#endif
	

	if ((font_index < 0) || 
		(font_index >=
		 font_state_p->max_number_of_downloadable_fonts))
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "(p9000_font_download_font)\n"
						   "{\n"
						   "\t# index out of bounds.\n"
						   "\tfont_index = %ld\n"
						   "}\n",
						   font_index);
		}
#endif

		return (SI_FAIL);

	}

	if (font_info_p->SFnumglyph <= 0)
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "(p9000_font_download_font)\n"
						   "{\n"
						   "\t# negative or zero number of glyphs\n"
						   "\tnumber of glyphs = %ld\n"
						   "}\n",
						   font_info_p->SFnumglyph);
		}
	#endif

			return (SI_FAIL);

	}
	
	ASSERT ((font_state_p->font_list_pp[font_index]) == NULL);

	/*
	 * Allocate zeroed memory for one p9000_font structure and
	 * tuck away the pointer in the p9000 font state structure.
	 */

	font_p = allocate_and_clear_memory(sizeof(struct p9000_font));

	font_state_p->font_list_pp[font_index] = font_p;

	/*
	 * Copy the fontinfo structure.
	 */

	font_p->si_font_info = *font_info_p;

	ASSERT(font_info_p->SFnumglyph > 0);

	/*
	 * Allocate space for the p9000_glyph structures .
	 */

	font_p->font_glyphs_p = 
		allocate_and_clear_memory(font_info_p->SFnumglyph *
								  sizeof(struct p9000_glyph));

	/*
	 * Compute the space required to hold the largest character in the
	 * font.
	 */

	max_glyph_size_in_bits = 
		(((font_info_p->SFmax.SFrbearing -
		   font_info_p->SFmin.SFlbearing) *
		  (font_info_p->SFmax.SFascent +
		   font_info_p->SFmax.SFdescent)));

	ASSERT(((max_glyph_size_in_bits +
			 DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) & 
			~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) >=
		   DEFAULT_SYSTEM_LONG_WORD_SIZE);
	
	max_glyph_size = 
		(((max_glyph_size_in_bits + DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) & 
		  ~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) >> 
		 DEFAULT_SYSTEM_LONG_WORD_SIZE_SHIFT);

	if ((max_glyph_size <= P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE) &&
		(font_info_p->SFflag & SFTerminalFont))
	{
		max_glyph_size = P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE;
	}
	
	/*
	 * Update glyph size information in the font state.
	 */
		
	font_p->max_glyph_size = max_glyph_size;

#if (defined(__DEBUG__))

	/*
	 * Allocate space for the glyph bits.
	 */

	font_p->font_glyph_packed_bits_p = 
		allocate_and_clear_memory((max_glyph_size *
								   font_info_p->SFnumglyph *
								   sizeof (unsigned long))
								  + sizeof(p9000_debug_stamp));

	/*
	 * Place a magic marker in the memory.
	 */

	*((font_p->font_glyph_packed_bits_p + 
	   (max_glyph_size * font_info_p->SFnumglyph))) =
		   p9000_debug_stamp;

#else

	/*
	 * Allocate space for the glyph bits.
	 */

	font_p->font_glyph_packed_bits_p = 
	    allocate_and_clear_memory(max_glyph_size *
								  font_info_p->SFnumglyph *
								  sizeof(unsigned long));

#endif

	/*
	 * Now for each glyph in the font: fill in the p9000_glyph
	 * structure and pack the glyph bits. 
	 */

	for(si_glyph_p = glyph_list_p,
		glyph_p = font_p->font_glyphs_p,
		glyph_packed_bits_p = font_p->font_glyph_packed_bits_p;

		si_glyph_p < last_si_glyph_p;

		glyph_p++,
		si_glyph_p++,
		glyph_packed_bits_p += max_glyph_size)
	{

		int			glyph_width_in_pixels;
		int			glyph_height_in_pixels;
		int current_bit_position = 0;
		unsigned long *glyph_bits_row_start_p;
		unsigned long *packed_bits_p;
		int row_number;
		int glyph_row_step;
		
		
		/*
		 * Copy the SIGlyph structure.
		 */

		glyph_p->si_glyph = 
			*si_glyph_p;


		/*
		 * Compute useful accelarators.
		 */
		
		glyph_p->is_out_of_font_bounds = 
			((si_glyph_p->SFascent > font_p->si_font_info.SFlascent) ||
			 (si_glyph_p->SFdescent > font_p->si_font_info.SFldescent)) ? 
				 TRUE : FALSE;

		/*
		 * set the pointer to the packed bits.
		 */

		packed_bits_p = glyph_packed_bits_p; 

		glyph_p->glyph_packed_bits_p = packed_bits_p;
		
		glyph_width_in_pixels = 
			si_glyph_p->SFrbearing - si_glyph_p->SFlbearing; 

		ASSERT(glyph_width_in_pixels ==
			   glyph_p->si_glyph.SFglyph.Bwidth);

		glyph_height_in_pixels = 
			si_glyph_p->SFascent + si_glyph_p->SFdescent; 
		
		ASSERT(glyph_height_in_pixels ==
			   glyph_p->si_glyph.SFglyph.Bheight);

		glyph_bits_row_start_p = 
			(unsigned long *) glyph_p->si_glyph.SFglyph.Bptr;

		glyph_row_step =
			(glyph_width_in_pixels +
			 DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) >>
			 DEFAULT_SYSTEM_LONG_WORD_SIZE_SHIFT;
		
		/*
		 * Loop over all the rows of the glyph bitmap packing the
		 * bits.
		 */

		for (row_number = 0; row_number < glyph_height_in_pixels;
			 row_number++, glyph_bits_row_start_p += glyph_row_step)
		{
			
			/*
			 * Loop over the words in each row of glyph bits.
			 */

			int number_of_whole_words_in_row = 
				(unsigned) glyph_width_in_pixels >>
					DEFAULT_SYSTEM_LONG_WORD_SIZE_SHIFT;

			register unsigned long *glyph_bits_p =
				glyph_bits_row_start_p; 
			
			ASSERT(number_of_whole_words_in_row >= 0);
			
			/*
			 * Loop over whole words first.
			 */

			for(; 
				number_of_whole_words_in_row--; 
				glyph_bits_p++)
			{
				ACCUMULATE_BITS(packed_bits_p,
								current_bit_position,
								glyph_bits_p,
								DEFAULT_SYSTEM_LONG_WORD_SIZE);
			}
			
			/*
			 * Do the next partial word.
			 */

			if (glyph_width_in_pixels &
				DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK)
			{
				
				ACCUMULATE_BITS(packed_bits_p,
								current_bit_position,
								glyph_bits_p,
								(glyph_width_in_pixels &
								 DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK));
			}
			
							
		}
		
		/*
		 * Mark the number of whole and partial words used up by the
		 * packing process.
		 */

		ASSERT(packed_bits_p >= glyph_p->glyph_packed_bits_p);

		glyph_p->glyph_whole_transfers = 
			(packed_bits_p - glyph_packed_bits_p);
		
		glyph_p->glyph_last_width =
			current_bit_position & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK;

		/*
		 *  last width should never be zero, if it is so then borrow
		 *  32 bits from whole words. But make sure that  it is not
		 *  a null glyph 
		 */

		if (glyph_p->glyph_last_width == 0 &&
			 glyph_p->glyph_whole_transfers > 0) 
		{
			glyph_p->glyph_last_width  =
				 DEFAULT_SYSTEM_LONG_WORD_SIZE;

			--glyph_p->glyph_whole_transfers;
		}

		ASSERT(glyph_p->glyph_whole_transfers >= 0);

		STAMP_OBJECT(P9000_GLYPH, glyph_p);

	}


	/*
	 * Set the pointers to the font draw functions.
	 */

	if (font_info_p->SFflag & SFTerminalFont)
	{

		/*
		 * use the terminal font drawing code.
		 */

		font_p->font_kind = P9000_FONT_KIND_TERMINAL_FONT;
		font_p->draw_glyphs =
				p9000_font_draw_terminal_glyphs;

	}
	else
	{
		/*
		 * Use the non-terminal font draw code.
		 */

		font_p->font_kind = P9000_FONT_KIND_NON_TERMINAL_FONT;
		font_p->draw_glyphs =
			p9000_font_draw_non_terminal_glyphs;
	}


	STAMP_OBJECT(P9000_FONT, font_p);
	
	ASSERT(IS_OBJECT_STAMPED(P9000_FONT_STATE, font_state_p));

	/* check for memory overruns */
	ASSERT(*((font_p->font_glyph_packed_bits_p + 
					  (max_glyph_size *
					   font_info_p->SFnumglyph))) ==
		   p9000_debug_stamp);

	return (SI_SUCCEED);
}

#undef ACCUMULATE_BITS

/*
 * p9000_font_free_font
 *
 * Free the memory used by a font.
 */

STATIC SIBool
p9000_font_free_font(SIint32 font_index)
{
	
	struct p9000_font	*font_p;

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_font, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_font_free_font)\n"
					   "{\n"
					   "\tfont_index = %ld\n"
					   "}\n",
					   font_index);
	}
#endif
	
	if ((font_index < 0) || 
		(font_index >= 
		 font_state_p->max_number_of_downloadable_fonts))
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "(p9000_font_free_font)\n"
						   "{\n"
						   "\t# index out of bounds.\n"
						   "}\n");
		}
#endif
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return (SI_FAIL);
	}

	font_p = font_state_p->font_list_pp[font_index];

	ASSERT(IS_OBJECT_STAMPED(P9000_FONT,font_p));

	ASSERT(font_p->font_kind == P9000_FONT_KIND_TERMINAL_FONT ||
		   font_p->font_kind == P9000_FONT_KIND_NON_TERMINAL_FONT);

	ASSERT(font_p->font_glyph_packed_bits_p);
	
	/*
	 * Free the space allocated for the packed bits;
	 */

	free_memory(font_p->font_glyph_packed_bits_p);

	/*
	 * Free the memory allocated for the p9000_glyph_structures.
	 */

	ASSERT(font_p->font_glyphs_p);
	
	free_memory(font_p->font_glyphs_p);

	ASSERT(font_p);
	
	free_memory(font_p);

	/*
	 * Mark this entry as being unused.
	 */

	font_state_p->font_list_pp[font_index] = NULL;

	ASSERT(IS_OBJECT_STAMPED(P9000_FONT_STATE, font_state_p));

	return (SI_SUCCEED);

}


/*
 * p9000_font_draw_glyphs
 *
 * SI's entry point for font drawing and stippling.  This routine will
 * clip the list of displayed glyphs to an approximate visible list of
 * glyphs, and pass the resultant clipped list down to the lower-level
 * drawing routine.  If the font draw style is "OPQStipple" (ie:
 * ImageText), the background rectangle of the text string is drawn
 * prior to calling the lower-level function.
 */

STATIC SIBool
p9000_font_draw_glyphs(
	const SIint32 font_index, 
	const SIint32 x,
	const SIint32 y,
	SIint32 glyph_count,
	SIint16 *glyph_list_p,
	SIint32 draw_method)
{
	int x_origin = x;			/* starting point of string */
	int end_x_origin;
	int string_width;
	unsigned short minterm;

	SIint16 *displayed_glyph_start_p;
	SIint16 *displayed_glyph_end_p;

	/*
	 * The font descriptor.
	 */
	struct p9000_font *font_p;
	struct p9000_glyph *font_glyphs_p;

	SIint16 *const last_glyph_p = glyph_list_p + glyph_count;

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	P9000_CURRENT_FONT_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	
	const int screen_clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int screen_clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int screen_clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;

	
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(P9000_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_font, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_font_draw_glyphs)\n"
					   "{\n"
					   "\tfont_index = %ld\n"
					   "\tx = %ld\n"
					   "\ty = %ld\n"
					   "\tglyph_count = %ld\n"
					   "\tglyph_list_p = %p\n"
					   "\tdraw_method = %ld\n"
					   "}\n",
					   font_index, x, y, glyph_count,
					   (void *) glyph_list_p, draw_method);
	}
	
	/*
	 * Dump the glyph list
	 */

	if (p9000_font_string_debug)
	{
		p9000_font_glyph_list_dump_helper(font_index, glyph_count, 
										 glyph_list_p);
	}
	
#endif

	/*
	 * Check if anything has to be drawn.
	 */

	if (glyph_count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Check for the validity of the font index.
	 */

	if (font_index < 0 || font_index >=
		font_state_p->max_number_of_downloadable_fonts) 
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "(p9000_font_draw_glyphs)\n"
						   "{\n"
						   "\t# index is out of bounds.\n"
						   "\tfont_index = %ld\n"
						   "}\n",
						   font_index);
		}
#endif

		return (SI_FAIL);

	}

	/*
	 * Update the clip registers to the graphics state if need be.
	 */

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	/*
	 * Determine the type of draw : opaque or transparent stippling.
	 */

	if (draw_method == 0)
	{
		draw_method =
			graphics_state_p->generic_state.si_graphics_state.SGstplmode;
	}
	
	/*
	 * Get the pointer to this font.
	 */

	font_p = *(font_state_p->font_list_pp + font_index);

	ASSERT(IS_OBJECT_STAMPED(P9000_FONT, font_p));

	/*
	 * Get the pointer to the glyph descriptions for this font.
	 */

	font_glyphs_p = font_p->font_glyphs_p;
	
	ASSERT(font_glyphs_p != NULL);
	
	/*
	 * Check for the y-coordinates visibility.
	 */

	if ((y - font_p->si_font_info.SFmax.SFascent > screen_clip_bottom) ||
		(y + font_p->si_font_info.SFmax.SFdescent <= screen_clip_top))
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "\t# Y check indicates nothing is to be drawn.\n"
						   "}\n");
		}
#endif

		return (SI_SUCCEED);

	}
	
	/*
	 * Compute clipping parameters.
	 */

	if (font_p->si_font_info.SFflag & SFTerminalFont)
	{

		/*
		 * Terminal glyphs.
		 */

		int width = font_p->si_font_info.SFmax.SFwidth;
		int left_extra_glyph_count = 0;
		int right_extra_glyph_count = 0;
		int x_start = x;		/* inclusive */
		int x_end = x_start + (width * glyph_count); /* exclusive */
		
		if (width < 0)
		{

			/*
			 * Right to left.
			 */

			int tmp;

			/*
			 * Swap start and end and adjust for the first character.
			 */

			tmp = x_start;
			x_start = x_end;
			x_end = tmp;
			
			x_start -= width;
			x_end += width;
			
		}

		if (x_start > screen_clip_right || x_end <= screen_clip_left )
		{

#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
			{
				(void) fprintf(debug_stream_p,
							   "\t#X-coord of string out of clip rectangle.\n"
							   "}\n");
			}
#endif

			return (SI_SUCCEED);

		}
		
		if (x_start < screen_clip_left)
		{
			left_extra_glyph_count = (screen_clip_left - x_start) /
				width;
		}

		if ((x_end - 1) > screen_clip_right)
		{
			right_extra_glyph_count = (x_end - screen_clip_right - 1) /
				width;
		}
		
		if (width > 0)
		{
			displayed_glyph_start_p = glyph_list_p +
				left_extra_glyph_count;
			displayed_glyph_end_p = glyph_list_p + glyph_count -
				right_extra_glyph_count;
		}
		else					/* width < 0 */
		{	
			displayed_glyph_start_p = glyph_list_p +
				right_extra_glyph_count;
			displayed_glyph_end_p = glyph_list_p + glyph_count -
				left_extra_glyph_count;
		}
		
		/*
		 * The string width parameter is not used for terminal fonts,
		 * as there is no need for a separate background rectangle
		 * fill.  Instead we do opaque stippling.
		 */

		string_width = 
			(displayed_glyph_end_p - displayed_glyph_start_p) * width;
		
		x_origin += 
			(displayed_glyph_start_p - glyph_list_p) * width;

	}
	else 
	{

		/*
		 * Non-terminal glyphs.
		 */
		
		if (draw_method == SGOPQStipple)
		{
			
			/*
			 * Compute the set of visible glyphs given the current clip region.
			 * The total width in pixels of the glyphs visible is returned.
			 */

			displayed_glyph_start_p = glyph_list_p;

			/*
			 * Look for the first visible glyph.
			 */

			do
			{

				register const struct p9000_glyph *glyph_p = 
					&(font_glyphs_p[*displayed_glyph_start_p]);
		
				if ((x_origin + 
					 glyph_p->si_glyph.SFrbearing - 1) >= 
					screen_clip_left)
				{
					if ((x_origin +
						 glyph_p->si_glyph.SFlbearing) <=
						screen_clip_right)
					{
						break;
					}
				}
			
				x_origin += glyph_p->si_glyph.SFwidth;
			
			} while (++displayed_glyph_start_p < last_glyph_p);
		
			/*
			 * Have we exhausted the glyph list already?
			 */

			if (displayed_glyph_start_p == last_glyph_p)
			{

				/*
				 * Nothing to draw.
				 */

#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
				{
					(void) fprintf(debug_stream_p,
								   "\t#No glyph visible in clip region.\n"
								   "}\n");
				}
#endif

				return (SI_SUCCEED);

			}
		
			/*
			 * Look for the last (partially?) visible character.
			 */
		
			displayed_glyph_end_p = displayed_glyph_start_p;
		
			end_x_origin = x_origin;

			do
			{
				register const struct p9000_glyph *const glyph_p = 
					&(font_glyphs_p[*displayed_glyph_end_p]);
			
				if (((end_x_origin + 
					  glyph_p->si_glyph.SFrbearing - 1) <
					 screen_clip_left) ||
					((end_x_origin +
					  glyph_p->si_glyph.SFlbearing) >
					 screen_clip_right))
				{
					break;
				}
			
				end_x_origin += glyph_p->si_glyph.SFwidth;

			} while (++displayed_glyph_end_p < last_glyph_p);
			

			/*
			 * Compute the string width.  This will overshoot, for clipped
			 * text, but the hardware clip will ensure that wrong does
			 * not actually occur.
			 *
			 * *BUG* This is incorrect for right to left text
			 */

			string_width = end_x_origin - x_origin;
		
			ASSERT(displayed_glyph_start_p <= displayed_glyph_end_p);
		
			/*
			 * Check if *something* is visible.
			 */

			if (displayed_glyph_start_p == displayed_glyph_end_p)
			{

#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
				{
					(void) fprintf(debug_stream_p,
								   "\t# No glyphs are displayable.\n"
								   "}\n");
				}
#endif

				return (SI_SUCCEED);

			}
		}
		else
		{
			/*
			 * There is little point in computing the background 
			 * rectangle dimensions for PolyText, as we never use
			 * it.  Draw all the glyphs.
			 */

			displayed_glyph_start_p = glyph_list_p;
			displayed_glyph_end_p = last_glyph_p;
			string_width = 0;
		}
	}

	/*
	 * Set the draw mode.
	 */

	if (draw_method == SGOPQStipple)
	{
	
		/*
		 * Synchronize with the foreground color, the background color
		 * and the planemask.
		 */

		P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));
		
		P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

		if (font_p->font_kind != P9000_FONT_KIND_TERMINAL_FONT)
		{
			
			int x_left = x_origin;
			int y_top = y - font_p->si_font_info.SFlascent;
			int x_right = x_origin + string_width;
			int y_bottom = y + font_p->si_font_info.SFldescent; 
		
			int status;
		
#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_font, INTERNAL))
			{
				(void) fprintf(debug_stream_p,
							   "(p9000_font_draw_glyphs)\n"
							   "{\n"
							   "\tImage Text.\n"
							   "\ttop left = (%d, %d)\n"
							   "\tbottom right = (%d, %d)\n"
							   "}\n",
							   x_left, y_top,
							   x_right, y_bottom);
			}
#endif

			/*
			 * Fill the background with the bg color.
			 */

			/*
			 * Draw a solid background rectangle.  Fill up the
			 * rectangle parameters. 
			 */
		
			/*
			 * x_top_left, y_top_left
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_0,
				P9000_PARAMETER_COORDINATE_XY_16,
				P9000_PARAMETER_COORDINATE_ABS,
				P9000_PACK_XY(x_left, y_top));
		
			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_XY_16,
				P9000_PARAMETER_COORDINATE_ABS,
				P9000_PACK_XY(x_left, y_bottom));
		
			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_2,
				P9000_PARAMETER_COORDINATE_XY_16,
				P9000_PARAMETER_COORDINATE_ABS,
				P9000_PACK_XY(x_right, y_bottom));
		
			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_3,
				P9000_PARAMETER_COORDINATE_XY_16,
				P9000_PARAMETER_COORDINATE_ABS,
				P9000_PACK_XY(x_right, y_top));
		
			/*
			 * Change the raster to GXcopy and draw the background rectangle.
			 */
		
			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
		
			P9000_STATE_SET_RASTER(register_state_p,
								   P9000_BACKGROUND_COLOR_MASK);

			do
			{
				status = P9000_INITIATE_QUAD_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

			/*
			 * We should never face any problem in software in this case,
			 * as clipping to visible coordinates has already been done on
			 * the glyphs being displayed.
			 */

			ASSERT(!(status & P9000_STATUS_QUAD_SOFTWARE));

			/*
			 * Setup the minterms for doing xparent stippling with
			 * the foreground color, with rop == GXcopy.
			 */

			minterm = 
				(P9000_SOURCE_MASK & P9000_FOREGROUND_COLOR_MASK) | 
					(~P9000_SOURCE_MASK & P9000_DESTINATION_MASK);
		}
		else
		{
			
			/*
			 * Terminal fonts can be opaque stippled.  The rop ==
			 * GXcopy in this case.
			 */
			
			minterm =
				(P9000_SOURCE_MASK & P9000_FOREGROUND_COLOR_MASK) | 
				(~P9000_SOURCE_MASK & P9000_BACKGROUND_COLOR_MASK);
		}
	}
	else						/* PolyText */
	{

		/*
		 * Synchronize with the foreground color and the planemask.
		 */

		P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));
		
		P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

		minterm = 
				(P9000_SOURCE_MASK &
				 P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
												  graphics_state_p)) | 
				(~P9000_SOURCE_MASK & P9000_DESTINATION_MASK);

	}
	
	/*
	 * Call the appropriate drawing function for this font.
	 */
	
	(*font_p->draw_glyphs)
		(font_p, displayed_glyph_start_p, 
		 displayed_glyph_end_p, x_origin, y, x_origin + string_width,
		 minterm, draw_method);

	return (SI_SUCCEED);

}

/*
 * p9000_font__initialize__
 *
 * Initialize the font module.
 */

function void
p9000_font__initialize__(SIScreenRec *si_screen_p,
						 struct p9000_options_structure *options_p)
{
	int glyph_max_width;
	int glyph_max_height;
	struct p9000_font_state *font_state_p;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	P9000_CURRENT_SCREEN_STATE_DECLARE();

	
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) screen_state_p));

	/*
	 * Create space for the font state and fill in control values.
	 */

	font_state_p = 
		allocate_and_clear_memory(sizeof(struct p9000_font_state));

	/*
	 * How many fonts the SDD should support at a given time.
	 */

	font_state_p->max_number_of_downloadable_fonts = 
		options_p->number_of_downloadable_fonts;

	/*
	 * Create space for font pointers.
	 */

	if (font_state_p->max_number_of_downloadable_fonts > 0)
	{
		font_state_p->font_list_pp =
			allocate_and_clear_memory(
			    font_state_p->max_number_of_downloadable_fonts * 
				sizeof(struct p9000_font *));
	}
	else	/* mark that we are not handling fonts */
	{
		flags_p->SIfontcnt = flags_p->SIavail_font = 0;
	}
	
	if (options_p->max_font_glyph_size != NULL &&
		sscanf(options_p->max_font_glyph_size, "%ix%i", &glyph_max_width,
			   &glyph_max_height) != 2)
	{
		(void) fprintf(stderr,
					   P9000_MESSAGE_CANNOT_PARSE_GLYPH_CACHE_SIZE,
					   options_p->max_font_glyph_size);
		glyph_max_width = P9000_DEFAULT_MAX_GLYPH_WIDTH;
		glyph_max_height = P9000_DEFAULT_MAX_GLYPH_HEIGHT;
	}
	else
	{
		glyph_max_width = P9000_DEFAULT_MAX_GLYPH_WIDTH;
		glyph_max_height = P9000_DEFAULT_MAX_GLYPH_HEIGHT;
	}
	

	font_state_p->max_supported_glyph_width = glyph_max_width;
	font_state_p->max_supported_glyph_height = glyph_max_height;
	

	STAMP_OBJECT(P9000_FONT_STATE, font_state_p);

	/*
	 * Tuck away pointer into the screen state.
	 */
	screen_state_p->font_state_p = font_state_p;

	/*
	 * Store parameters for SI.
	 */

	flags_p->SIfontcnt  =
		font_state_p->max_number_of_downloadable_fonts;
	
	flags_p->SIavail_font = 
		(FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL);

	functions_p->si_font_check = p9000_font_is_font_downloadable;
	functions_p->si_font_download = p9000_font_download_font;
	functions_p->si_font_free = p9000_font_free_font;
	functions_p->si_font_stplblt = p9000_font_draw_glyphs;

}
