/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_font.c	1.2"

/***
 ***	NAME
 ***
 ***		s364_font.c : Font handling code for the S364 display
 ***	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s364_font.h"
 ***
 ***	DESCRIPTION
 ***	
 ***	This file contains the font handling code for the S3_X64 class
 ***	of chipsets.  All kinds of fonts are handled and are rendered
 ***	from system memory. Small terminal fonts are special cased, if
 ***	a word can hold more than one glyph width then suitable number 
 ***	of glyphs are assembled and drawn at a time, this can be controlled
 ***	by an option.
 ***	Image text in case of terminal fonts is drawn using opaque stippling
 ***	and for non-terminal fonts the background rectangle is drawn and
 ***	the glyphs are rendered in transparency mode.
 ***	
 ***	
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

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s364_font_debug = FALSE;
#endif

PRIVATE

/***
 ***	Private declarations.
 ***/
/*
 * Cached glyph information.
 *
 */

struct s364_glyph
{

	/*
	 * Saved SI glyph information
	 */

	SIGlyph si_glyph;

	/*
	 * Pointer to raw bits for this glyph
	 */

	char *glyph_bits_p;

	/*
	 * Count of words to transfer for this glyph
	 */

	int transfer_length;

	boolean is_null_glyph;

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/***
 ***	Includes.
 ***/

#include "g_state.h"
#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_gs.h"
#include "s364_asm.h"
#include "s364_state.h"

/***
 ***	Constants.
 ***/

#define	S364_FONT_STATE_STAMP											\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('F' << 3) + ('O' << 4) +	\
	 ('N' << 5) + ('T' << 6) + ('_' << 7) + ('S' << 8) + ('T' << 9) +	\
	 ('A' << 10) + ('T' << 11) + ('E' << 12) + 0 )

#define	S364_GLYPH_STAMP												\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('G' << 3) + ('L' << 4) +	\
	 ('Y' << 5) + ('P' << 6) + ('H' << 7) + ('_' << 8) + ('S' << 9) +	\
	 ('T' << 10) + ('A' << 11) + ('M' << 12) + ('P' << 13) + 0 )

#define	S364_FONT_STAMP												\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('F' << 3) + ('O' << 4) +	\
	 ('N' << 5) + ('T' << 6) + ('_' << 7) + ('S' << 8) + ('T' << 9) +	\
	 ('A' << 10) + ('M' << 11) + ('P' << 12) + 0 )
	
/*
 * Flags controlling the drawing.
 */

#define S364_FONT_DO_POLYTEXT 			(0x0001 << 0U)
#define S364_FONT_DO_IMAGETEXT			(0x0001 << 1U)

/***
 ***	Macros.
 ***/

#define S364_CURRENT_FONT_STATE_DECLARE()			\
	struct s364_font_state *font_state_p = 			\
		screen_state_p->font_state_p


#define	S364_FONT_RENDER_GLYPH(REG_BASE,GLYPH_BITS_P,WORD_COUNT)	\
{																	\
	int *data_p = (int*)(GLYPH_BITS_P);								\
	int *fence_p = data_p + (WORD_COUNT);							\
	do																\
	{																\
		*((volatile int*)REG_BASE) = *data_p++;						\
	}while(data_p < fence_p);										\
}

#define	S364_FONT_RENDER_TERMINAL_GLYPHS(REG_BASE,							\
		GLYPH_LIST_START_P,GLYPH_LIST_END_P,								\
		X_ORIGIN,Y_POSITION,FONT_GLYPH_BITS_P,								\
		BYTES_PER_GLYPH,WIDTH)												\
{																			\
	while(GLYPH_LIST_START_P < GLYPH_LIST_END_P)							\
	{																		\
		S3_INLINE_WAIT_FOR_FIFO(3);											\
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 		\
			(unsigned short)Y_POSITION, unsigned short);					\
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 		\
			(unsigned short)X_ORIGIN, unsigned short);						\
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,			\
			S3_CMD_WRITE | 													\
			S3_CMD_PX_MD_ACROSS_PLANE |										\
			S3_CMD_DRAW |													\
			S3_CMD_DIR_TYPE_AXIAL | 										\
			S3_CMD_AXIAL_X_LEFT_TO_RIGHT |									\
			S3_CMD_AXIAL_X_MAJOR |											\
			S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |									\
			S3_CMD_USE_WAIT_YES |											\
			S3_CMD_BUS_WIDTH_32 |											\
			S3_CMD_LSB_FIRST |												\
			S3_CMD_TYPE_RECTFILL,											\
			unsigned short);												\
		S3_WAIT_FOR_ALL_FIFO_FREE();										\
		S364_FONT_RENDER_GLYPH(REG_BASE,									\
			(FONT_GLYPH_BITS_P + *(GLYPH_LIST_START_P) *					\
			BYTES_PER_GLYPH),												\
			(BYTES_PER_GLYPH >> 2));										\
		X_ORIGIN += WIDTH;													\
		++GLYPH_LIST_START_P;												\
	}																		\
}



/***
 ***	Types.
 ***/

/*
 * Font kinds.
 */

#define DEFINE_FONT_TYPES()						\
	DEFINE_FONT_TYPE(NULL),						\
	DEFINE_FONT_TYPE(TERMINAL_FONT),			\
	DEFINE_FONT_TYPE(NON_TERMINAL_FONT),		\
	DEFINE_FONT_TYPE(PACKED_TERMINAL_FONT),		\
	DEFINE_FONT_TYPE(COUNT)

enum s364_font_kind
{
#define DEFINE_FONT_TYPE(NAME)\
		S364_FONT_KIND_ ## NAME
		DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE
};

/*
 * Structure of a downloaded font.
 */

struct s364_font
{

	/*
	 * SI's font information
	 */

	SIFontInfo si_font_info;
	
	/*
	 * The type of this font.
	 */

	enum s364_font_kind font_kind;
	
	/*
	 * Pointer to the array of s3_glyph structures.
	 */

	struct s364_glyph *font_glyphs_p;

	char *font_glyph_bits_p;
	int bytes_per_glyph;

	/*
	 * Used incase of packed terminal fonts.
	 */

	int *font_packed_glyph_bits_p;

	/*
	 * Number of terminal glyph widths that can be packed into a
	 * word.
	 */ 

	int number_of_glyph_lines_per_word;

	/*
	 * Pointer to draw function for this font.
	 */

	void (*draw_glyphs)
		(struct s364_font *const font_p,
		 short *glyph_start_p, 
		 short *glyph_end_p,
		 int x_origin,
		 int y_origin,
		 int string_width,
		 int draw_mode);

#if (defined(__DEBUG__))
	int stamp;
#endif

};

/*
 * The state of the font handling code.
 */

struct s364_font_state
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

	struct s364_font **font_list_pp;
	
#if (defined(__DEBUG__))
	int stamp;
#endif

};

/***
 ***	Functions.
 ***/


/*
 * PURPOSE
 *  This is a generalized routine for handling all kinds of font glyphs.
 * This is called for rendering non-terminal glyphs.
 *
 * RETURN VALUE
 *
 * None
 *
 */


STATIC void
s364_font_draw_non_terminal_glyphs(
	struct s364_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	struct s364_glyph *const font_glyphs_p = font_p->font_glyphs_p;


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));


#if (defined(__DEBUG__))
	if (s364_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_font_draw_non_terminal_glyphs) {\n"
			"\tfont_p = %p\n"
			"\tglyph_list_start_p = %p\n"
			"\tglyph_list_end_p = %p\n"
			"\tx_origin = %d\n"
			"\ty_origin = %d\n"
			"\tstring_width = %d\n"
			"\tdraw_flags = 0x%x\n"
			"}\n",
			(void *) font_p,
			(void *) glyph_list_start_p,
			(void *) glyph_list_end_p,
			x_origin, y_origin,
			string_width, draw_flags);
	}
#endif


	/*
 	 * Set cpu data as the color source
	 */

	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_CPU_DATA, unsigned short);

	do
	{
		struct s364_glyph *const glyph_p =
				 font_glyphs_p + *glyph_list_start_p;
		const short lbearing	= glyph_p->si_glyph.SFlbearing;
		const short rbearing	= glyph_p->si_glyph.SFrbearing;
		const short descent	= glyph_p->si_glyph.SFdescent; 
		const short ascent	= glyph_p->si_glyph.SFascent; 


#define	GLYPH_WIDTH		(rbearing - lbearing)
#define	GLYPH_HEIGHT 	(ascent + descent)
#define	X_LEFT			(x_origin + lbearing)
#define	Y_TOP			(y_origin - ascent)


		ASSERT(IS_OBJECT_STAMPED(S364_GLYPH, glyph_p));


		/*
		 * There could be some glyphs with null width and height.
		 * ignore them. Stipple the other glyphs through the host
		 * data registers.
		 */

		if (glyph_p->is_null_glyph == FALSE)
		{

			ASSERT(glyph_p->transfer_length > 0);


			S3_WAIT_FOR_ALL_FIFO_FREE();

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
				(unsigned short)(GLYPH_WIDTH - 1),
				 unsigned short);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
				((GLYPH_HEIGHT - 1) & S3_MULTIFUNC_VALUE_BITS),
				unsigned short);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
				(unsigned short)X_LEFT,
				unsigned short);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				(unsigned short)Y_TOP,
				unsigned short);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD,
				S3_CMD_WRITE | 
				S3_CMD_PX_MD_ACROSS_PLANE |
				S3_CMD_DRAW |
				S3_CMD_DIR_TYPE_AXIAL | 
				S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
				S3_CMD_USE_WAIT_YES |
				S3_CMD_BUS_WIDTH_32 |
				S3_CMD_LSB_FIRST |
				S3_CMD_TYPE_RECTFILL,
				unsigned short);

			S364_FONT_RENDER_GLYPH(register_base_address_p,
				(glyph_p->glyph_bits_p),
				(glyph_p->transfer_length));

		}

		/*
		 * Adjust the origin for the next glyph
		 */

		x_origin += glyph_p->si_glyph.SFwidth;


	} while (++glyph_list_start_p < glyph_list_end_p);

#undef	X_LEFT
#undef	Y_TOP
#undef	GLYPH_WIDTH
#undef	GLYPH_HEIGHT

	/*
	 * Restore the pixel control.
	 */

	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);
}

/*
 * PURSPOSE
 *   
 * This routine is called for rendering large terminal  fonts.
 *
 * RETURN
 * None
 */

STATIC void
s364_font_draw_terminal_glyphs(
	struct s364_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	const int width = font_p->si_font_info.SFmax.SFwidth;
	const int height = font_p->si_font_info.SFmax.SFdescent + 
		font_p->si_font_info.SFmax.SFascent;
	const int y_position =
		 (y_origin - font_p->si_font_info.SFmax.SFascent);
	const int bytes_per_glyph = font_p->bytes_per_glyph;
	const char * font_glyph_bits_p = font_p->font_glyph_bits_p;





	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));


#if (defined(__DEBUG__))
	if (s364_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_font_draw_terminal_glyphs) {\n"
			"\tfont_p = %p\n"
			"\tglyph_list_start_p = %p\n"
			"\tglyph_list_end_p = %p\n"
			"\tx_origin = %d\n"
			"\ty_origin = %d\n"
			"\tstring_width = %d\n"
			"\tdraw_flags = 0x%x\n"
			"}\n",
			(void *) font_p,
			(void *) glyph_list_start_p,
			(void *) glyph_list_end_p,
			x_origin, y_origin,
			string_width, draw_flags);
	}
#endif


	if (font_p->font_glyphs_p->is_null_glyph == TRUE)
	{
		return;
	}


	S3_WAIT_FOR_FIFO(3);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_CPU_DATA,
		unsigned short);


	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(unsigned short)(width - 1),
		unsigned short);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS),
		unsigned short);
	
	S364_FONT_RENDER_TERMINAL_GLYPHS(
		register_base_address_p,
		glyph_list_start_p,glyph_list_end_p,	
		x_origin, y_position, font_glyph_bits_p,
		bytes_per_glyph,width);
		
	/*
	 * Restore the pixel control.
	 */

	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);
}


/*
 * PURPOSE
 *
 * This routine is used to render small terminal font glyphs.
 * Adjacent glyphs are packed  and rendered.
 *
 * RETURN
 *
 * None
 */



STATIC void
s364_font_draw_packed_terminal_glyphs(
	struct s364_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	const char * font_glyph_bits_p = font_p->font_glyph_bits_p;
	const int * font_packed_glyph_bits_p =
			 (int*)font_p->font_packed_glyph_bits_p;
	const int width = font_p->si_font_info.SFmax.SFwidth;
	const int height = font_p->si_font_info.SFmax.SFdescent + 
		font_p->si_font_info.SFmax.SFascent;
	const int y_position =
		 (y_origin - font_p->si_font_info.SFmax.SFascent);
	const int bytes_per_glyph = font_p->bytes_per_glyph;
	const int words_per_glyph = bytes_per_glyph >> 2;
	const int	glyph_lines_per_word =
		 font_p->number_of_glyph_lines_per_word;
	int	packed_transfer_count = 
		(glyph_list_end_p - glyph_list_start_p) / glyph_lines_per_word;



	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(font_packed_glyph_bits_p);

#if (defined(__DEBUG__))
	if (s364_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_font_draw_packed_terminal_glyphs) {\n"
			"\tfont_p = %p\n"
			"\tglyph_list_start_p = %p\n"
			"\tglyph_list_end_p = %p\n"
			"\tx_origin = %d\n"
			"\ty_origin = %d\n"
			"\tstring_width = %d\n"
			"\tglyph_lines_per_word = %d\n"
			"\tdraw_flags = 0x%x\n"
			"}\n",
			(void *) font_p,
			(void *) glyph_list_start_p,
			(void *) glyph_list_end_p,
			x_origin, y_origin,
			string_width,
			glyph_lines_per_word,
			draw_flags);
	}
#endif


	/*
	 * Set the pixel control register to use cpu data for selecting
	 * the foreground and background mix registers.
	 */

	S3_WAIT_FOR_FIFO(3);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_CPU_DATA, unsigned short);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(unsigned short)(width*glyph_lines_per_word - 1),
		unsigned short);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS),
		unsigned short);


	if (packed_transfer_count > 0) 
	{

#define	GLYPH_LINE(GLYPH_NO,SHIFT)										\
		(*(g_bits_p[GLYPH_NO] + tmp_height)) <<	(SHIFT)

		switch(glyph_lines_per_word)
		{
			case 5:
				ASSERT(width == 6);
				do
				{
					int tmp_height = 0;
					int *g_bits_p[5];

					g_bits_p[0] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[0]);

					g_bits_p[1] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[1]);

					g_bits_p[2] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[2]);

					g_bits_p[3] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[3]);

					g_bits_p[4] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[4]);

					S3_INLINE_WAIT_FOR_FIFO(3);

					S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						(unsigned short)y_position, unsigned short);

					S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
						(unsigned short)x_origin, unsigned short);

					S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
						S3_CMD_WRITE | 
						S3_CMD_PX_MD_ACROSS_PLANE |
						S3_CMD_DRAW |
						S3_CMD_DIR_TYPE_AXIAL | 
						S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
						S3_CMD_AXIAL_X_MAJOR |
						S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
						S3_CMD_USE_WAIT_YES |
						S3_CMD_BUS_WIDTH_32 |
						S3_CMD_TYPE_RECTFILL,
						unsigned short);

					S3_WAIT_FOR_ALL_FIFO_FREE();


						
					do
					{

#define	GLYPH0_OFFSET	26
#define	GLYPH1_OFFSET	20
#define	GLYPH2_OFFSET	14
#define	GLYPH3_OFFSET	 8
#define	GLYPH4_OFFSET	 2

						*((volatile int*)register_base_address_p) =
							GLYPH_LINE(0,GLYPH0_OFFSET) | 
							GLYPH_LINE(1,GLYPH1_OFFSET) | 
							GLYPH_LINE(2,GLYPH2_OFFSET) | 
							GLYPH_LINE(3,GLYPH3_OFFSET) | 
							GLYPH_LINE(4,GLYPH4_OFFSET); 

#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
#undef	GLYPH4_OFFSET

					}while(++tmp_height < height);


					/*
					 * Adjust the origin for the next glyph
					 */

					x_origin += (width*glyph_lines_per_word);
					glyph_list_start_p += glyph_lines_per_word;
				}while(--packed_transfer_count > 0);
				break;

			case 4:
				ASSERT(width == 8 || width == 7);

#define		RENDER_4_GLYPHS()								\
{															\
	int tmp_height = 0;										\
	int *g_bits_p[4];										\
	g_bits_p[0] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[0]);								\
	g_bits_p[1] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[1]);								\
	g_bits_p[2] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[2]);								\
	g_bits_p[3] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[3]);								\
	S3_INLINE_WAIT_FOR_FIFO(3);								\
	S3_UPDATE_MMAP_REGISTER(								\
		S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 				\
		(unsigned short)y_position, unsigned short);		\
	S3_UPDATE_MMAP_REGISTER(								\
		S3_ENHANCED_COMMAND_REGISTER_CUR_X, 				\
		(unsigned short)x_origin, unsigned short);			\
	S3_UPDATE_MMAP_REGISTER(								\
		S3_ENHANCED_COMMAND_REGISTER_CMD,					\
		S3_CMD_WRITE | 										\
		S3_CMD_PX_MD_ACROSS_PLANE |							\
		S3_CMD_DRAW |										\
		S3_CMD_DIR_TYPE_AXIAL | 							\
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |						\
		S3_CMD_AXIAL_X_MAJOR |								\
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |						\
		S3_CMD_USE_WAIT_YES |								\
		S3_CMD_BUS_WIDTH_32 |								\
		S3_CMD_TYPE_RECTFILL,								\
		unsigned short);									\
	S3_WAIT_FOR_ALL_FIFO_FREE();							\
	do														\
	{														\
		*((volatile int*)register_base_address_p) =			\
		GLYPH_LINE(0,GLYPH0_OFFSET) | 						\
		GLYPH_LINE(1,GLYPH1_OFFSET) | 						\
		GLYPH_LINE(2,GLYPH2_OFFSET) | 						\
		GLYPH_LINE(3,GLYPH3_OFFSET);						\
	}while(++tmp_height < height);							\
	x_origin += (width*glyph_lines_per_word);				\
	glyph_list_start_p += glyph_lines_per_word;				\
}

				if (width == 8)
				{
#define	GLYPH0_OFFSET	24
#define	GLYPH1_OFFSET	16
#define	GLYPH2_OFFSET	 8
#define	GLYPH3_OFFSET	 0
					do
					{
						RENDER_4_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
				}
				else
				{
#define	GLYPH0_OFFSET	25
#define	GLYPH1_OFFSET	18
#define	GLYPH2_OFFSET	11
#define	GLYPH3_OFFSET	 4
					do
					{
						RENDER_4_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
				}
				break;

			case 3:

#define		RENDER_3_GLYPHS()								\
{															\
	int tmp_height = 0;										\
	int *g_bits_p[3];										\
	g_bits_p[0] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[0]);								\
	g_bits_p[1] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[1]);								\
	g_bits_p[2] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[2]);								\
	S3_INLINE_WAIT_FOR_FIFO(3);								\
	S3_UPDATE_MMAP_REGISTER(								\
		S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 				\
		(unsigned short)y_position, unsigned short);		\
	S3_UPDATE_MMAP_REGISTER(								\
		S3_ENHANCED_COMMAND_REGISTER_CUR_X, 				\
		(unsigned short)x_origin, unsigned short);			\
	S3_UPDATE_MMAP_REGISTER(								\
		S3_ENHANCED_COMMAND_REGISTER_CMD,					\
		S3_CMD_WRITE | 										\
		S3_CMD_PX_MD_ACROSS_PLANE |							\
		S3_CMD_DRAW |										\
		S3_CMD_DIR_TYPE_AXIAL | 							\
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |						\
		S3_CMD_AXIAL_X_MAJOR |								\
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |						\
		S3_CMD_USE_WAIT_YES |								\
		S3_CMD_BUS_WIDTH_32 |								\
		S3_CMD_TYPE_RECTFILL,								\
		unsigned short);									\
	S3_WAIT_FOR_ALL_FIFO_FREE();							\
	do														\
	{														\
		*((volatile int*)register_base_address_p) =			\
		GLYPH_LINE(0,GLYPH0_OFFSET) | 						\
		GLYPH_LINE(1,GLYPH1_OFFSET) | 						\
		GLYPH_LINE(2,GLYPH2_OFFSET); 						\
	}while(++tmp_height < height);							\
	x_origin += (width*glyph_lines_per_word);				\
	glyph_list_start_p += glyph_lines_per_word;				\
}
				ASSERT(width == 10 || width == 9);

				if (width == 10)
				{
#define	GLYPH0_OFFSET	22
#define	GLYPH1_OFFSET	12
#define	GLYPH2_OFFSET	2
					do
					{
						RENDER_3_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
				}
				else
				{
#define	GLYPH0_OFFSET	23
#define	GLYPH1_OFFSET	14
#define	GLYPH2_OFFSET	5
					do
					{
						RENDER_3_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
				}

				break;
#undef	GLYPH_LINE

			default:
				ASSERT(glyph_lines_per_word >= 5);

				do
				{
					int tmp_height = 0;
					int *g_bits_p[5];

					g_bits_p[0] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[0]);

					g_bits_p[1] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[1]);

					g_bits_p[2] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[2]);

					g_bits_p[3] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[3]);

					g_bits_p[4] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[4]);

					S3_INLINE_WAIT_FOR_FIFO(3);

					S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						(unsigned short)y_position, unsigned short);

					S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
						(unsigned short)x_origin, unsigned short);

					S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
						S3_CMD_WRITE | 
						S3_CMD_PX_MD_ACROSS_PLANE |
						S3_CMD_DRAW |
						S3_CMD_DIR_TYPE_AXIAL | 
						S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
						S3_CMD_AXIAL_X_MAJOR |
						S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
						S3_CMD_USE_WAIT_YES |
						S3_CMD_BUS_WIDTH_32 |
						S3_CMD_TYPE_RECTFILL,
						unsigned short);

					S3_WAIT_FOR_ALL_FIFO_FREE();

#define	GLYPH_LINE(GLYPH_NO)										\
			(*(g_bits_p[GLYPH_NO] + tmp_height)) <<					\
				(32 - ((GLYPH_NO+1) * width))
						
					do
					{

						*((volatile int*)register_base_address_p) =
						GLYPH_LINE(0) | 
						GLYPH_LINE(1) | 
						GLYPH_LINE(2) | 
						GLYPH_LINE(3) | 
						GLYPH_LINE(4); 

					}while(++tmp_height < height);

					/*
					 * Adjust the origin for the next glyph
					 */

					x_origin += (width*glyph_lines_per_word);
					glyph_list_start_p += glyph_lines_per_word;

				}while(--packed_transfer_count > 0);
			break;

#undef	GLYPH_LINE
		}
	}

	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(unsigned short)(width - 1),unsigned short);


	/*
	 * Draw the remaining glyphs one at a time
	 */

	S364_FONT_RENDER_TERMINAL_GLYPHS(register_base_address_p,
		glyph_list_start_p,glyph_list_end_p,
		x_origin,y_position,font_glyph_bits_p,
		bytes_per_glyph,width);

	/*
	 * Restore the pixel control.
	 */

	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);

#undef GLYPHS_PER_WORD
}


/*
 * PURPOSE
 *  This routine clips the string to be drawn before calling a 
 *	font-type dependent routine that will do the actual rendering.
 *
 *  RETURN
 * 
 * SI_SUCCEED: If glyphs successfully rendered
 * SI_FAIL: otherwise
 */

STATIC SIBool
s364_font_draw_glyphs( const SIint32 font_index, 
	const SIint32 x, const SIint32 y,
	SIint32 glyph_count, SIint16 *glyph_list_p, SIint32 draw_method)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_CURRENT_FONT_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	
	/*
	 * The font descriptor.
	 */

	struct s364_font 	*font_p;
	struct s364_glyph 	*font_glyphs_p;

	/*
	 * parameters for the glyphs to be rendered.
	 */

	const SIint16 		*last_glyph_p = glyph_list_p + glyph_count;
	SIint16 			*displayed_glyph_start_p;
	SIint16 			*displayed_glyph_end_p;
	int 				end_x_origin;
	int 				string_width;
	int 				draw_flags;

	/* 
	 * starting point of string to be rendered.
	 */

	int 				x_origin = x;

	/*
	 * current clip bounds.
	 */

	const int clip_left = screen_state_p->generic_state.screen_clip_left;
	const int clip_right = screen_state_p->generic_state.screen_clip_right;
	const int clip_top = screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom = screen_state_p->generic_state.screen_clip_bottom;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (s364_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_font_draw_glyphs)\n"
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
#endif 


	if (glyph_count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (font_index < 0 || font_index >=
		font_state_p->max_number_of_downloadable_fonts) 
	{
		return (SI_FAIL);
	}

	if ((x < 0) || (y < 0))
	{
		return (SI_FAIL);
	}

	/*
	 * Determine the type of draw : opaque or transparent stippling.
	 */

	if (draw_method == 0)
	{
		draw_method =
			graphics_state_p->generic_state.si_graphics_state.SGstplmode;
	}

	/*
	 * Set the draw flags.
	 */

	draw_flags = (draw_method == SGStipple) ? S364_FONT_DO_POLYTEXT : 
		S364_FONT_DO_IMAGETEXT;
	
	if ((draw_method == SGStipple) &&
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode
		 != SGFillSolidFG) &&
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode
		 != SGFillSolidBG))
	{
		/*
		 * We can't handle polytext requests with the fill style set
		 * to tiling or stippling.
		 */

		return (SI_FAIL);
	}
		
	font_p = *(font_state_p->font_list_pp + font_index);

	ASSERT(IS_OBJECT_STAMPED(S364_FONT, font_p));

	font_glyphs_p = font_p->font_glyphs_p;
	
	ASSERT(font_glyphs_p != NULL);
	
	/*
	 * Check for the y-coordinates visibility.
	 */

	if ((y - font_p->si_font_info.SFmax.SFascent > clip_bottom) ||
		(y + font_p->si_font_info.SFmax.SFdescent <= clip_top))
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Compute clipping parameters.
	 */

	if (font_p->si_font_info.SFflag & SFTerminalFont)
	{
		/*
		 * Terminal glyphs.
		 *
		 */

		int width = font_p->si_font_info.SFmax.SFwidth;
		int left_extra_glyph_count = 0;
		int right_extra_glyph_count = 0;
		int x_start = x;
		int x_end = x_start + (width * glyph_count);
		
		if (width < 0)
		{
			/*
			 * Right to left
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

		if (x_start > clip_right || x_end <= clip_left )
		{
			return (SI_SUCCEED);
		}
		
		if (x_start < clip_left)
		{
			left_extra_glyph_count = (clip_left - x_start) / width;
		}

		if ((x_end - 1)  > clip_right)
		{
			right_extra_glyph_count = (x_end - clip_right - 1) / width;
		}
		
		if (width > 0)
		{
			displayed_glyph_start_p = glyph_list_p + left_extra_glyph_count;
			displayed_glyph_end_p = glyph_list_p + glyph_count -
				right_extra_glyph_count;
		}
		else					/* width < 0 */
		{	
			displayed_glyph_start_p = glyph_list_p + right_extra_glyph_count;
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
		
		x_origin +=  (displayed_glyph_start_p - glyph_list_p) * width;

	}
	else
	{
		
		/*
		 * Non-terminal glyphs.
		 */
		
		/*
		 * Compute the set of visible glyphs given the current clip region,
		 * ensure that these glyphs are downloaded into offscreen memory
		 * (if the usage of offscreen memory is allowed).
		 * The total width in pixels of the glyphs visible is returned.
		 */

		displayed_glyph_start_p = glyph_list_p;

		/*
		 * Look for the first visible glyph.
		 */

		do
		{
			register const struct s364_glyph *glyph_p = 
				&(font_glyphs_p[*displayed_glyph_start_p]);
		
			if ((x_origin + glyph_p->si_glyph.SFrbearing - 1) >= clip_left)
			{
				if ((x_origin + glyph_p->si_glyph.SFlbearing) <= clip_right)
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

			return (SI_SUCCEED);
		}
		
		/*
		 * Look for the last (partially?) visible character.
		 */

		displayed_glyph_end_p = displayed_glyph_start_p;
		
		end_x_origin = x_origin;

		do
		{
			register const struct s364_glyph *const glyph_p = 
				&(font_glyphs_p[*displayed_glyph_end_p]);
			
			if (((end_x_origin + glyph_p->si_glyph.SFrbearing - 1) <
				 	clip_left) ||
				((end_x_origin + glyph_p->si_glyph.SFlbearing) > 
					clip_right))
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
			return (SI_SUCCEED);
		}
	}

	
	if (string_width == 0)
	{
		return SI_SUCCEED;
	}

	
	S364_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP();
	

	if (font_p->font_kind == S364_FONT_KIND_NON_TERMINAL_FONT)
	{
		if (draw_flags == S364_FONT_DO_IMAGETEXT)
		{
		
			S3_WAIT_FOR_FIFO(6);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
				(S3_CLR_SRC_BKGD_COLOR | 
				S3_MIX_FN_N),unsigned short);
				
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X,
				(string_width > 0 ? x_origin : x_origin - 1),
				unsigned short);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				y - font_p->si_font_info.SFlascent,
				unsigned short);


			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((font_p->si_font_info.SFldescent + 
				  font_p->si_font_info.SFlascent - 1) & 
				 S3_MULTIFUNC_VALUE_BITS),
				unsigned short);


			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
				((string_width > 0 ? string_width : - string_width) - 1),
				unsigned short);
				

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				(S3_CMD_WRITE |
				S3_CMD_PX_MD_THRO_PLANE |  
				S3_CMD_DRAW |
				S3_CMD_DIR_TYPE_AXIAL | 
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_X_MAJOR |
				(string_width > 0 ? S3_CMD_AXIAL_X_LEFT_TO_RIGHT :
				S3_CMD_AXIAL_X_RIGHT_TO_LEFT) |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
				S3_CMD_BUS_WIDTH_32 |
				S3_CMD_TYPE_RECTFILL),
				unsigned short);

			/*
			 * Now we can do transparent stippling in GXcopy mode
			 */

			S3_WAIT_FOR_FIFO(2);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
				(S3_CLR_SRC_FRGD_COLOR | S3_MIX_FN_N),unsigned short);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
				S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);

		}
		else
		{

			S3_WAIT_FOR_FIFO(2);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
				(enhanced_cmds_p->frgd_mix | S3_CLR_SRC_FRGD_COLOR),
				unsigned short);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
				S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);
		}

	}
	else
	{

		if (draw_flags == S364_FONT_DO_IMAGETEXT)
		{
			S3_WAIT_FOR_FIFO(2);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
				(S3_MIX_FN_N | S3_CLR_SRC_FRGD_COLOR),
				unsigned short);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
				(S3_MIX_FN_N | S3_CLR_SRC_BKGD_COLOR),
				unsigned short);

		}
		else
		{
			S3_WAIT_FOR_FIFO(2);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
				(enhanced_cmds_p->frgd_mix | S3_CLR_SRC_FRGD_COLOR),
				unsigned short);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
				S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);

		}
	}


	ASSERT(font_p->draw_glyphs);
	
	(*font_p->draw_glyphs) (font_p,
		 displayed_glyph_start_p, 
		 displayed_glyph_end_p,
		 x_origin, y,
		 string_width, draw_flags);

	return (SI_SUCCEED);
}

/*
 * PURPOSE
 * Check if a font can be downloaded into the SDD.
 *
 * RETURN
 *
 * SI_SUCCEED: If  the font described in fontinfo_p can be downloaded.
 * SI_FAIL: If the font can't be handled by the SDD.
 */

STATIC SIBool
s364_font_is_font_downloadable(
	SIint32 font_index, 
	SIFontInfoP fontinfo_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_FONT_STATE, font_state_p));
	
#if (defined(__DEBUG__))
	if (s364_font_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(s364_font_is_font_downloadable)\n"
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
		if (s364_font_debug)
		{
			(void) fprintf(debug_stream_p,
				   "(s364_font_is_font_downloadable)\n"
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
	 	   S364_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS) &&
		 (fontinfo_p->SFflag & SFTerminalFont)) ||
	    (!(screen_state_p->options_p->fontdraw_options & 
	 	   S364_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS) &&
		 (!(fontinfo_p->SFflag & SFTerminalFont))) ||
		((screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font > 0) && 
		 (fontinfo_p->SFnumglyph > screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font)))
	{

#if (defined(__DEBUG__))
		if (s364_font_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s364_font_is_font_downloadable)\n"
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
 * PURPOSE
 * Download a font from the SI layer into the SDD's system memory.
 *
 * RETURN
 *
 * SI_FAIL or SI_SUCCEED
 */

STATIC SIBool
s364_font_download_font( SIint32 font_index, SIFontInfoP font_info_p, 
	SIGlyphP glyph_list_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_FONT_STATE_DECLARE();
	struct s364_font *font_p = NULL;

	const int max_glyph_height_in_pixels = 
		font_info_p->SFmax.SFascent + 
		font_info_p->SFmax.SFdescent;

	const int max_glyph_width_in_pixels = 
		font_info_p->SFmax.SFrbearing - 
		font_info_p->SFmin.SFlbearing;

	const SIGlyphP	last_si_glyph_p = 
		glyph_list_p + font_info_p->SFnumglyph;
	
	SIGlyphP current_si_glyph_p = NULL;

	const unsigned int	glyph_count =
		 font_info_p->SFnumglyph;

	struct s364_glyph	*current_s364_glyph_p = NULL;
	enum s364_font_kind	font_kind = S364_FONT_KIND_NULL;



	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_FONT_STATE, font_state_p));
	ASSERT(screen_state_p->generic_state.screen_depth != 24);

#if (defined(__DEBUG__))
	if (s364_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_font_download_font)\n"
			"{\n"
			"\tfont_index = %ld\n"
			"\tfont_info_p = %p\n"
			"\tglyph_list_p = %p\n"
			"}\n",
			font_index,
			(void *) font_info_p,
			(void *) glyph_list_p);
	}
#endif


	if ((font_index < 0) || 
		(font_index >= font_state_p->max_number_of_downloadable_fonts))
	{
		return (SI_FAIL);
	}

	if (font_info_p->SFnumglyph <= 0)
	{
		return (SI_FAIL);
	}
	
	ASSERT ((font_state_p->font_list_pp[font_index]) == NULL);

	/*
	 * Allocate zeroed memory for one s364_font structure and
	 * tuck away the pointer in the s364 font state structure.
	 */

	font_p = allocate_and_clear_memory(sizeof(struct s364_font));

	font_state_p->font_list_pp[font_index] = font_p;

	/*
	 * Copy the fontinfo structure.
	 */

	font_p->si_font_info = *font_info_p;

	ASSERT(font_info_p->SFnumglyph > 0);


	font_kind = (font_info_p->SFflag & SFTerminalFont) ? 
		S364_FONT_KIND_TERMINAL_FONT : S364_FONT_KIND_NON_TERMINAL_FONT;

	if ((screen_state_p->options_p->fontdraw_options &
		S364_OPTIONS_FONTDRAW_OPTIONS_ASSEMBLE_GLYPHS) &&
		(font_kind == S364_FONT_KIND_TERMINAL_FONT))
	{
		font_kind = S364_FONT_KIND_PACKED_TERMINAL_FONT;
	}
		

	/*
	 * Allocate space for the s364_glyph structures.
	 */

	font_p->font_glyphs_p = allocate_and_clear_memory(
		glyph_count * sizeof(struct s364_glyph));

	current_si_glyph_p = glyph_list_p;
	current_s364_glyph_p = font_p->font_glyphs_p;

	{
		const unsigned int bytes_per_glyph =
			((unsigned)((max_glyph_width_in_pixels + 31) & ~31) >> 3U) *
				  max_glyph_height_in_pixels;

		char *current_glyph_bits_p  = NULL;
		int *current_packed_glyph_bits_p  = NULL;


		current_glyph_bits_p = 
		font_p->font_glyph_bits_p = 
			allocate_and_clear_memory(bytes_per_glyph *
				glyph_count);


		font_p->bytes_per_glyph =
			bytes_per_glyph;


		if (font_kind == S364_FONT_KIND_PACKED_TERMINAL_FONT)
		{
			/*
			 * See if we can pack more than one glyph's line
			 * in a long word. If not mark font_kind as
			 * S364_FONT_KIND_TERMINAL_FONT.
			 */

			font_p->number_of_glyph_lines_per_word = 
				32 / max_glyph_width_in_pixels;
				
			if (font_p->number_of_glyph_lines_per_word > 2)
			{
				current_packed_glyph_bits_p = 
					font_p->font_packed_glyph_bits_p = 
					allocate_and_clear_memory(
					(sizeof(int) * max_glyph_height_in_pixels *
					glyph_count));
			}
			else
			{
				font_kind = S364_FONT_KIND_TERMINAL_FONT;
			}


		}

		while (current_si_glyph_p < last_si_glyph_p)
		{
			int			glyph_width_in_pixels;
			int			glyph_height_in_pixels;
			int			numbytes_per_glyph_line;
			int			numbytes_per_glyph_bitmap_line;
			
			/*
			 * Copy the SIGlyph structure after allocating space for it.
			 */

			current_s364_glyph_p->si_glyph = *current_si_glyph_p;

			glyph_width_in_pixels = 
				current_si_glyph_p->SFrbearing -
				 current_si_glyph_p->SFlbearing; 

			glyph_height_in_pixels = 
				current_si_glyph_p->SFascent +
				current_si_glyph_p->SFdescent; 

			numbytes_per_glyph_line = 
				(unsigned)((glyph_width_in_pixels + 31) & ~31) >> 3U;

			numbytes_per_glyph_bitmap_line = 
				(unsigned)((glyph_width_in_pixels + 31) & ~31) >> 3U;

			current_s364_glyph_p->is_null_glyph = TRUE;

			if ((glyph_height_in_pixels > 0) && ( glyph_width_in_pixels > 0)) 
			{
				/* 
				 * for copying the glyph bits.
				 */

				unsigned char *dst_p = NULL;
				unsigned char *src_p = NULL;
				int	i;

				/*
				 * Valid glyph if width/height are both > 0.
				 */

				current_s364_glyph_p->is_null_glyph = FALSE;

				/*
				 * Compute the total word transfers for rendering this 
				 * glyph from system memory.
				 */

				current_s364_glyph_p->transfer_length = 
					(numbytes_per_glyph_line * 
					glyph_height_in_pixels) >> 2;

				ASSERT(glyph_width_in_pixels == 
					current_si_glyph_p->SFglyph.Bwidth);

				/*
				 * pointer to the bits.
				 */

				current_s364_glyph_p->glyph_bits_p = current_glyph_bits_p;  

				/*
				 * Copy the glyph bits. Pack them to so that unnecessary
				 * bytes are not copied.
				 */

			  	dst_p = (unsigned char *)current_glyph_bits_p;
				src_p = (unsigned char *)current_si_glyph_p->SFglyph.Bptr;

				while(glyph_height_in_pixels--)
				{
					unsigned char *tmp_p;

					i = numbytes_per_glyph_line;
					tmp_p = src_p;

					while(i--)
					{
						*dst_p++ = 
							screen_state_p->byte_invert_table_p[*tmp_p++];
					}

					src_p += numbytes_per_glyph_bitmap_line;
				}

				if (font_kind == S364_FONT_KIND_PACKED_TERMINAL_FONT)
				{
					int tmp_bits = 0;
					int packed_data = 0;
					int full_bytes = glyph_width_in_pixels / 8;
					int partial_bits_count = glyph_width_in_pixels % 8;
					int partial_bits_mask = 
						(1 << partial_bits_count) - 1;
					int height = 
						current_si_glyph_p->SFascent +
						current_si_glyph_p->SFdescent; 
					unsigned char *src_p = 
					 (unsigned char *)current_si_glyph_p->SFglyph.Bptr;
					int *dst_p =
						current_packed_glyph_bits_p;
					int glyph_line_mask =
						 (1 << glyph_width_in_pixels) - 1;

					
					while(height)
					{
						int tmp_full_bytes = full_bytes;
						unsigned char *tmp_p = src_p;


						while(tmp_full_bytes > 0)
						{
							tmp_bits = 
								*tmp_p++;

							tmp_bits = 
								screen_state_p->byte_invert_table_p[tmp_bits];

							packed_data <<=  8;
							packed_data |= tmp_bits & 0xFF;
							--tmp_full_bytes;
						}

						if (partial_bits_count > 0)
						{
							tmp_bits = 
								*tmp_p;

							tmp_bits = 
								screen_state_p->byte_invert_table_p[tmp_bits];

							tmp_bits >>= (8 - partial_bits_count);

							packed_data <<= partial_bits_count;

							packed_data |=
								 tmp_bits & partial_bits_mask;

						}

						--height;
						*dst_p++ = packed_data & glyph_line_mask;
						src_p += numbytes_per_glyph_bitmap_line;
					}
			
				}
			}
				
			STAMP_OBJECT(S364_GLYPH, current_s364_glyph_p);

			/*
			 * Move over to the next glyph.
			 */

			current_glyph_bits_p += bytes_per_glyph;
			++current_s364_glyph_p;
			++current_si_glyph_p;

			if (font_kind == S364_FONT_KIND_PACKED_TERMINAL_FONT)
			{
				current_packed_glyph_bits_p +=
					 max_glyph_height_in_pixels;
			}
		}

	}



	switch (font_kind)
	{
		case S364_FONT_KIND_TERMINAL_FONT:
			font_p->font_kind =
				 S364_FONT_KIND_TERMINAL_FONT;
			font_p->draw_glyphs =
				 s364_font_draw_terminal_glyphs;
		break;
		case S364_FONT_KIND_NON_TERMINAL_FONT:
			font_p->font_kind =
				 S364_FONT_KIND_NON_TERMINAL_FONT;
			font_p->draw_glyphs =
				 s364_font_draw_non_terminal_glyphs;
		break;
		case S364_FONT_KIND_PACKED_TERMINAL_FONT:
			font_p->font_kind =
				 S364_FONT_KIND_PACKED_TERMINAL_FONT;
			font_p->draw_glyphs =
				 s364_font_draw_packed_terminal_glyphs;
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			return(SI_FAIL);
	}

	STAMP_OBJECT(S364_FONT, font_p);

	ASSERT(IS_OBJECT_STAMPED(S364_FONT_STATE, font_state_p));

	return(SI_SUCCEED);

}

/*
 * PURPOSE
 * 
 * Free up all kinds of SDD resources used by the font corresponding to
 * font_index.
 *
 * RETURN
 *
 * SI_FAIL or SI_SUCCEED
 */

STATIC SIBool
s364_font_free_font(SIint32 font_index)
{
	struct s364_font		*font_p;

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (s364_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_font_free_font)\n"
			"{\n"
			"\tfont_index = %ld\n"
			"}\n",
			font_index);
	}
#endif
	
	if ((font_index < 0) || 
		(font_index >= font_state_p->max_number_of_downloadable_fonts))
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return (SI_FAIL);
	}

	font_p = font_state_p->font_list_pp[font_index] ;

	ASSERT(IS_OBJECT_STAMPED(S364_FONT,font_p));
	
	if (font_p->font_glyph_bits_p)
	{
		/*
		 * Free the space allocated for the font glyph bits.
		 */

		free_memory(font_p->font_glyph_bits_p);
	}

	/*
	 * Free the memory allocated for the s364_glyph_structures.
	 */

	ASSERT(font_p->font_kind == S364_FONT_KIND_TERMINAL_FONT ||
		font_p->font_kind == S364_FONT_KIND_NON_TERMINAL_FONT ||
		font_p->font_kind == S364_FONT_KIND_PACKED_TERMINAL_FONT);
		
	if (font_p->font_kind == S364_FONT_KIND_PACKED_TERMINAL_FONT)
	{
		(void)free_memory(font_p->font_packed_glyph_bits_p);
	}

	ASSERT(font_p->font_glyphs_p);
	free_memory(font_p->font_glyphs_p);

	ASSERT(font_p);
	
	free_memory(font_p);

	font_state_p->font_list_pp[font_index] = NULL;

	ASSERT(IS_OBJECT_STAMPED(S364_FONT_STATE, font_state_p));

	return (SI_SUCCEED);
}

/*
 * PURPOSE
 * Initialize the font module.
 * RETURN
 * None
 */



function void
s364_font__initialize__(SIScreenRec *si_screen_p,
						 struct s364_options_structure *options_p)
{
	int glyph_max_width;
	int glyph_max_height;
	struct s364_font_state *font_state_p;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	S364_CURRENT_SCREEN_STATE_DECLARE();

	
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) screen_state_p));

	/*
	 * Create space for the font state and fill in control values.
	 */

	font_state_p = 
		allocate_and_clear_memory(sizeof(struct s364_font_state));

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
				sizeof(struct s364_font *));
	}
	else	/* mark that we are not handling fonts */
	{
		flags_p->SIfontcnt = flags_p->SIavail_font = 0;
	}
	
	if (options_p->glyph_cache_size != NULL &&
		sscanf(options_p->glyph_cache_size, "%ix%i", &glyph_max_width,
			   &glyph_max_height) != 2)
	{
		(void) fprintf(stderr,
					   S364_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE,
					   options_p->glyph_cache_size);
		glyph_max_width = DEFAULT_S364_MAX_GLYPH_WIDTH;
		glyph_max_height = DEFAULT_S364_MAX_GLYPH_HEIGHT;
	}
	else
	{
		glyph_max_width = DEFAULT_S364_MAX_GLYPH_WIDTH;
		glyph_max_height = DEFAULT_S364_MAX_GLYPH_HEIGHT;
	}
	

	font_state_p->max_supported_glyph_width = glyph_max_width;
	font_state_p->max_supported_glyph_height = glyph_max_height;
	

	STAMP_OBJECT(S364_FONT_STATE, font_state_p);

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

	functions_p->si_font_check = s364_font_is_font_downloadable;
	functions_p->si_font_download = s364_font_download_font;
	functions_p->si_font_free = s364_font_free_font;
	functions_p->si_font_stplblt = s364_font_draw_glyphs;

}
