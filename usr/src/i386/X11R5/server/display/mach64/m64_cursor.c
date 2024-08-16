/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_cursor.c	1.11"

/***
 ***	NAME
 ***
 ***		m64_cursor.c : Cursor management module
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m64_cursor.h"
 ***
 ***
 ***			SIBool
 ***			m64_cursor_download ( SIint32 cursor_index,
 ***									SICursorP cursor_p )
 ***			SIBool
 ***			m64_cursor_turnon ( SIint32 cursor_index )
 ***			SIBool
 ***			m64_cursor_turnoff ( SIint32 cursor_index )
 ***			SIBool
 ***			m64_cursor_move ( SIint32 cursor_index, SIint32 x, SIint32 y )
 ***
 ***
 ***
 ***
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***		m64_cursor.c : Source.
 ***		m64_cursor.h : Interface.
 ***
 ***	SEE ALSO
 ***
 ***  		SI definitions document
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***/

PUBLIC

/***
 *** 	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"

/***
 ***	Constants.
 ***/

#if (defined(__DEBUG__))

#define M64_CURSOR_STAMP \
	(( 'M' << 0 ) + ( '6' << 1 ) + ( '4' << 2 ) + ( '_' << 3 ) +\
	 ( 'C' << 4 ) + ( 'U' << 5 ) + ( 'R' << 6 ) + ( 'S' << 7 ) +\
	 ( 'O' << 8 ) + ( 'R' << 9 ) + 0 )

#define M64_CURSOR_STATE_STAMP\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) +\
	 ('C' << 4) + ('U' << 5) + ('R' << 6) + ('S' << 7) +\
	 ('O' << 8) + ('R' << 9) + ('_' << 10) + ('S' << 11) +\
	 ('T' << 12) + ('A' << 13) + ('T' << 14) + ('E' << 15) +\
	 ('_' << 16) + ('S' << 17) + ('T' << 18) + ('A' << 19)+ ('P' << 20))

#define M64_HARDWARE_CURSOR_STAMP\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + \
	 ('_' << 3) + ('H' << 4) + ('A' << 5) + ('R' << 6) +\
	 ('D' << 7) + ('W' << 8) + ('A' << 9) + ('R' << 10) +\
	 ('E' << 11) + ('_' << 12) + ('C' << 13) + ('U' << 14) +\
	 ('R' << 15) + ('S' << 16) + ('O' << 17) + ('R' << 18))

#define M64_SOFTWARE_CURSOR_STAMP\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + \
	 ('_' << 3) + ('S' << 4) + ('O' << 5) + ('F' << 6) +\
	 ('T' << 7) + ('W' << 8) + ('A' << 9) + ('R' << 10) +\
	 ('E' << 11) + ('_' << 12) + ('C' << 13) + ('U' << 14) +\
	 ('R' << 15) + ('S' << 16) + ('O' << 17) + ('R' << 18))
#endif

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/
enum m64_cursor_status_kind
{
	CURSOR_ON,
	CURSOR_OFF
};

struct m64_cursor_status
{
	enum m64_cursor_status_kind flag ;	/*is cursor on/off*/
	int	current_index ;		/*index of the current cursor (cursor_index)*/
	int	x;							/*X coord of the cursor*/
	int	y;							/*Y coord of the cursor*/
#if defined ( __DEBUG__ )
	int stamp ;
#endif
} ;

struct offscreen_area
{
	int	x ;							
	int	y ;
};

struct m64_cursor_software_cursor
{
	/*
	 * Bitmaps for the current cursor
	 */
	SIbitmapP source_p;
	SIbitmapP inverse_source_p;
	SIbitmapP mask_p;

#if defined(__DEBUG__)
	int stamp ;
#endif
};

struct m64_cursor_hardware_cursor
{

	int	cursor_offset ;
	int horz_cursor_offset ;
	int vert_cursor_offset ;
	struct offscreen_area	memory_representation ;			
#if defined(__DEBUG__)
	int stamp ;
#endif
};

struct m64_cursor
{
	/*
	 * Cursor attributes
	 */
	int	cursor_width ;				
	int cursor_height; 
	int	foreground_color ;
	int	background_color ;

	/* Pointers to various cursor operation functions for this cursor*/
	SIBool (*move_function_p)    ( SIint32, SIint32, SIint32 ) ;
	SIBool (*turnon_function_p)  ( SIint32 ) ;
	SIBool (*turnoff_function_p) ( SIint32 ) ;

	struct m64_cursor_hardware_cursor *hardware_cursor_p;

	/*
	 * software cursor module.
	 */
	struct m64_cursor_software_cursor	*software_cursor_p;

#if defined(__DEBUG__)
	int stamp ;
#endif
};

/*
 * Module state
 */
struct m64_cursor_state
{
	
	/* type of the cursor M64_CURSOR_HARDWARE or M64_CURSOR_SOFTWARE*/
	int cursor_type;

	/*Max width of the software and hardware cursor */
	int cursor_max_width;	

	/*Max height */
	int cursor_max_height; 

	/*No of downloadable cursors*/
	int number_of_cursors ;			

	/*
	 * The coords of the visible window (displayed area) with respect to
	 * the virtual screen
	 */
	int	 visible_window_top_x;
	int	 visible_window_top_y;

	/*Status of the current cursor */
	struct m64_cursor_status current_cursor_status; 

	/* Pointer to cursors list */
	struct m64_cursor **cursor_list_pp;

	/* Pointer to omm descriptor*/
	struct omm_allocation * omm_block_p ;

	/* Pointer to the bitmap containing the obscured screen area*/
	SIbitmapP saved_screen_area_p;


#if defined ( __DEBUG__)
	int stamp ;
#endif
};

/***
 ***	Variables.
 ***/

/*
 * Debugging variables.
 */
#if (defined(__DEBUG__))
export boolean m64_cursor_debug = 0;
#endif


PRIVATE

/***
 *** 	Includes.
 ***/
/*
 * Includes
 */
#include "m64_asm.h"
#include "g_omm.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_gbls.h"
#include "m64_state.h"
#include "m64_mischw.h"
#include "m64_cmap.h"

/***
 ***	Constants.
 ***/

/*
 * We support two kinds of cursors
 */
#define	M64_CURSOR_HARDWARE	0
#define	M64_CURSOR_SOFTWARE	1

#define	M64_CURSOR_NAMED_ALLOCATION_ID	"CURSOR"

#define M64_CURSOR_NAMED_ALLOCATION_STRING_TEMPLATE\
	M64_CURSOR_NAMED_ALLOCATION_ID ":%d+%d+%d@%d+%d+%d"

#define	M64_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN	200

/*
** Each pixel of the hardware cursor is represented by two bits
*/
#define M64_HARDWARE_CURSOR_DEPTH	2

/*
 * Parameters for downloading cursor into offscreen memory as a stipple.
 */
#define M64_CURSOR_SOURCE_OFFSET						0
#define M64_CURSOR_DESTINATION_OFFSET					0
#define M64_CURSOR_SOURCE_STRIDE						4
#define M64_CURSOR_BITMAP_WIDTH_IN_BYTES				1024
#define M64_CURSOR_BITMAP_HEIGHT						1

/***
 ***	Macros.
 ***/

#define M64_HARDWARE_CURSOR_MAX_WIDTH()\
	(m64_cursor_state_p->cursor_max_width)

#define M64_HARDWARE_CURSOR_MAX_HEIGHT()\
	(m64_cursor_state_p->cursor_max_height) 

#define M64_SOFTWARE_CURSOR_MAX_WIDTH()\
	(m64_cursor_state_p->cursor_max_width)

#define M64_SOFTWARE_CURSOR_MAX_HEIGHT()\
	(m64_cursor_state_p->cursor_max_height) 

#define M64_MAX_DISPLAYED_X_PIXELS() \
	(generic_current_screen_state_p->screen_displayed_width) 

#define M64_MAX_DISPLAYED_Y_PIXELS()\
	(generic_current_screen_state_p->screen_displayed_height) 

#define M64_MAX_VIRTUAL_X_PIXELS() \
	(generic_current_screen_state_p->screen_virtual_width) 

#define M64_MAX_VIRTUAL_Y_PIXELS()\
	(generic_current_screen_state_p->screen_virtual_height) 

#define M64_MAX_PHYSICAL_X_PIXELS() \
	(generic_current_screen_state_p->screen_physical_width) 

#define M64_MAX_PHYSICAL_Y_PIXELS()\
	(generic_current_screen_state_p->screen_physical_height) 

#define M64_SCREEN_DEPTH()\
	(generic_current_screen_state_p->screen_depth) 

#define	M64_CURSOR_CURSOR_STATE_DECLARE()\
	struct m64_cursor_state  *m64_cursor_state_p=\
	(screen_state_p)->cursor_state_p

#define	M64_CURSOR_IS_VALID_INDEX(cursor_index)\
	(cursor_index >= 0) &&\
	(cursor_index < m64_cursor_state_p->number_of_cursors)


#define	M64_CURSOR_CALCULATE_OFFSET(x,y,offset)\
{\
	int offset_in_mem;\
	if ( (y) !=  0 )\
	{\
			offset_in_mem=\
			((y)*(M64_MAX_PHYSICAL_X_PIXELS()*M64_SCREEN_DEPTH()))/32;\
			if ( (x) !=  0 )\
			{\
				offset_in_mem +=  ((x)*M64_SCREEN_DEPTH())/32;\
			}\
	}\
	else\
	{\
			if ( (x) != 0 )\
			{\
				offset_in_mem =  ((x)*M64_SCREEN_DEPTH())/32;\
			}\
			else\
			{\
				offset_in_mem = 0 ;\
			}\
	}\
	offset = offset_in_mem;\
}

/*
 * Remember that the visual numbered 0 is the screen default visual.
 */
#define M64_CURSOR_SET_CURSOR_COLOR(FG_COLOR,BG_COLOR)						\
{                                                                           \
    switch (generic_current_screen_state_p->screen_visuals_list_p[0].		\
		si_visual_p->SVtype) 												\
	{                                                                       \
		case PseudoColor:													\
		case StaticColor:													\
		{                                                                   \
			int cmap_index = generic_current_screen_state_p->				\
				screen_current_graphics_state_p->si_graphics_state.SGcmapidx;\
			int visual_index = generic_current_screen_state_p->				\
				screen_current_graphics_state_p->si_graphics_state.SGvisualidx;\
			struct generic_colormap *current_colormap_p =					\
				&(generic_current_screen_state_p->							\
				screen_colormaps_pp[visual_index][cmap_index]);				\
			unsigned short *fg_rgb_values_p, *bg_rgb_values_p;				\
			fg_rgb_values_p=&(current_colormap_p->rgb_values_p[(FG_COLOR)*3]);\
			bg_rgb_values_p=&(current_colormap_p->rgb_values_p[(BG_COLOR)*3]);\
			m64_cursor_set_cursor_color(FG_COLOR, BG_COLOR,fg_rgb_values_p,	\
				bg_rgb_values_p, generic_current_screen_state_p->			\
				screen_visuals_list_p[0].si_visual_p->SVtype);				\
			screen_state_p->dac_state_p->set_hardware_cursor_color(			\
				fg_rgb_values_p,bg_rgb_values_p);							\
		}                                                                   \
		break;																\
	case TrueColor:															\
		{                                                                   \
			unsigned short fg_rgb_values_p[3], bg_rgb_values_p[3];          \
			SIVisual *si_visual_p =                                         \
				screen_state_p->generic_state.screen_visuals_list_p[0].     \
					si_visual_p;                                            \
			fg_rgb_values_p[0] = ((FG_COLOR) & si_visual_p->SVredmask) >>   \
				si_visual_p->SVredoffset;                                   \
			fg_rgb_values_p[1] = ((FG_COLOR) & si_visual_p->SVgreenmask) >>	\
				si_visual_p->SVgreenoffset;                                 \
			fg_rgb_values_p[2] = ((FG_COLOR) & si_visual_p->SVbluemask) >>	\
				si_visual_p->SVblueoffset;                                  \
			bg_rgb_values_p[0] = ((BG_COLOR) & si_visual_p->SVredmask) >> 	\
				si_visual_p->SVredoffset;                                   \
			bg_rgb_values_p[1] = ((BG_COLOR) & si_visual_p->SVgreenmask) >>	\
				si_visual_p->SVgreenoffset;                                 \
			bg_rgb_values_p[2] = ((BG_COLOR) & si_visual_p->SVbluemask) >> 	\
				si_visual_p->SVblueoffset;                                  \
			m64_cursor_set_cursor_color(FG_COLOR, BG_COLOR,fg_rgb_values_p,	\
				bg_rgb_values_p, generic_current_screen_state_p->			\
				screen_visuals_list_p[0].si_visual_p->SVtype);				\
			screen_state_p->dac_state_p->set_hardware_cursor_color(			\
				fg_rgb_values_p,bg_rgb_values_p);							\
		}                                                                   \
		break;																\
	case DirectColor:														\
		{                                                                   \
			unsigned short fg_rgb_values_p[3], bg_rgb_values_p[3];          \
			SIVisual *si_visual_p =                                         \
				screen_state_p->generic_state.screen_visuals_list_p[0].     \
					si_visual_p;                                            \
			int cmap_index = generic_current_screen_state_p->				\
				screen_current_graphics_state_p->si_graphics_state.SGcmapidx;\
			int visual_index = generic_current_screen_state_p->				\
				screen_current_graphics_state_p->si_graphics_state.SGvisualidx;\
			struct generic_colormap *current_colormap_p =					\
				&(generic_current_screen_state_p->							\
				screen_colormaps_pp[visual_index][cmap_index]);				\
			int red_index, green_index, blue_index;							\
			int dac_rgb_width = (screen_state_p->options_p->dac_rgb_width ==\
				M64_OPTIONS_DAC_RGB_WIDTH_8 ? 8:6);							\
			red_index = ((FG_COLOR) & si_visual_p->SVredmask) >>   			\
				si_visual_p->SVredoffset;                                   \
			fg_rgb_values_p[0] = (*((current_colormap_p->rgb_values_p) +	\
				red_index*3) >> (16 - dac_rgb_width)) &						\
				((1 << dac_rgb_width) - 1);									\
			green_index = ((FG_COLOR) & si_visual_p->SVgreenmask) >>		\
				si_visual_p->SVgreenoffset;                                 \
			fg_rgb_values_p[1] = (*((current_colormap_p->rgb_values_p) +	\
				green_index*3 + 1) >> (16 - dac_rgb_width)) &				\
				((1 << dac_rgb_width) -1);									\
			blue_index = ((FG_COLOR) & si_visual_p->SVbluemask) >>			\
				si_visual_p->SVblueoffset;                                  \
			fg_rgb_values_p[2] = (*((current_colormap_p->rgb_values_p) + 	\
				blue_index*3 + 2) >> (16 - dac_rgb_width)) &				\
				((1 << dac_rgb_width) -1);									\
			red_index = ((BG_COLOR) & si_visual_p->SVredmask) >>   			\
				si_visual_p->SVredoffset;                                   \
			bg_rgb_values_p[0] = (*((current_colormap_p->rgb_values_p) +	\
				red_index*3) >> (16 - dac_rgb_width)) &						\
				((1 << dac_rgb_width) -1);									\
			green_index = ((BG_COLOR) & si_visual_p->SVgreenmask) >>		\
				si_visual_p->SVgreenoffset;                                 \
			bg_rgb_values_p[1] = (*((current_colormap_p->rgb_values_p) +	\
				green_index*3 + 1) >> (16 - dac_rgb_width)) &				\
				((1 << dac_rgb_width) -1);									\
			blue_index = ((BG_COLOR) & si_visual_p->SVbluemask) >>			\
				si_visual_p->SVblueoffset;                                  \
			bg_rgb_values_p[2] = (*((current_colormap_p->rgb_values_p) + 	\
				blue_index*3 + 2) >> (16 - dac_rgb_width)) &				\
				((1 << dac_rgb_width) -1);									\
			m64_cursor_set_cursor_color(FG_COLOR, BG_COLOR,fg_rgb_values_p,	\
				bg_rgb_values_p, generic_current_screen_state_p->			\
				screen_visuals_list_p[0].si_visual_p->SVtype);				\
			screen_state_p->dac_state_p->set_hardware_cursor_color(			\
				fg_rgb_values_p,bg_rgb_values_p);							\
		}                                                                   \
		break;																\
		default:															\
		/*CONSTANTCONDITION*/ 												\
		ASSERT(0);															\
		break;																\
	}																		\
}

#define M64_CURSOR_DISABLE_HARDWARE_CURSOR()								\
	screen_state_p->dac_state_p->disable_hardware_cursor()

#define M64_CURSOR_ENABLE_HARDWARE_CURSOR()									\
	screen_state_p->dac_state_p->enable_hardware_cursor()

#define	M64_CURSOR_IS_PANNING_FEASIBLE()\
	 ((M64_MAX_DISPLAYED_Y_PIXELS() < M64_MAX_VIRTUAL_Y_PIXELS()) ||\
		(M64_MAX_DISPLAYED_X_PIXELS() < M64_MAX_VIRTUAL_X_PIXELS()))

/***
 ***	Types.
 ***/

/***
 ***	Variables.
 ***/

/***
 *** 	Functions.
 ***/

/*
 * Set the cursor colors on the chipset.
 */
STATIC void
m64_cursor_set_cursor_color(int fg_color, int bg_color, 
	unsigned short *fg_rgb_values_p, unsigned short *bg_rgb_values_p, 
	SIint32 visual_type)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	if( visual_type == PseudoColor ||  visual_type == StaticColor )
	{
			register_base_address_p[M64_REGISTER_CUR_CLR0_OFFSET] = fg_color;
			register_base_address_p[M64_REGISTER_CUR_CLR1_OFFSET] = bg_color;
	}
	else
	{
		int shift[3] = {0, 0, 0};
		/*
		 * Remember visual 0 is the screen default visual.
		 */
		switch(screen_state_p->generic_state.screen_visuals_list_p[0].dac_flags)
		{
			case M64_VISUAL_TRUE_COLOR_16_555:
			case M64_VISUAL_DIRECT_COLOR_16_555:
				shift[0] = shift[1] = shift[2] = 3U;
				break;
			case M64_VISUAL_TRUE_COLOR_16_565:
			case M64_VISUAL_DIRECT_COLOR_16_565:
				shift[0] = shift[2] = 3U;
				shift[1] = 2U;
				break;
			default:
				break;
		}
		register_base_address_p[M64_REGISTER_CUR_CLR0_OFFSET] = 
			((fg_rgb_values_p[0] << shift[0] ) << 24) |
			((fg_rgb_values_p[1] << shift[1] ) << 16) |
			((fg_rgb_values_p[2] << shift[2] ) << 8 );

		register_base_address_p[M64_REGISTER_CUR_CLR1_OFFSET] = 
			((bg_rgb_values_p[0] << shift[0] ) << 24) |
			((bg_rgb_values_p[1] << shift[1] ) << 16) |
			((bg_rgb_values_p[2] << shift[2] ) << 8 );
	}

	return;
}

/*
 * m64_cursor__vt_switch_out__
 * 
 * Called when the X server is about to lose control of its virtual
 * terminal.  The cursor module needs to turn off the hardware cursor
 * before the VT switch occurs, in order to remove it from interfering
 * with the display.
 * Note that we don't save the cursor area as the responsibilty of
 * saving offscreen memory rests with the vt switch code in the "m64"
 * module (m64.c).
 */
function void
m64_cursor__vt_switch_out__(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;

	/*
	 * Save the cursor position related registers.
	 */
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
	register_values_p[M64_REGISTER_CUR_OFFSET_OFFSET] = 
		register_base_address_p[M64_REGISTER_CUR_OFFSET_OFFSET]; 
	register_values_p[M64_REGISTER_CUR_HORZ_VERT_OFF_OFFSET] = 
		register_base_address_p[M64_REGISTER_CUR_HORZ_VERT_OFF_OFFSET]; 
	register_values_p[M64_REGISTER_CUR_HORZ_VERT_POSN_OFFSET] = 
		register_base_address_p[M64_REGISTER_CUR_HORZ_VERT_POSN_OFFSET]; 

	return;
}

/*
 * m64_cursor__vt_switch_in__
 *
 * Called when the X server is going to switch into a virtual
 * terminal.  In the cursor module we need to re-enable the hardware
 * cursor if it was in use.  The contents of the offscreen location
 * are assumed to have been restored previous to this invocation.
 */

function void
m64_cursor__vt_switch_in__(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	int fg_color, bg_color;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

	/*
	 * Restore the cursor position related registers.
	 */
	register_base_address_p[M64_REGISTER_CUR_OFFSET_OFFSET] = 
		register_values_p[M64_REGISTER_CUR_OFFSET_OFFSET] & CUR_OFFSET_BITS; 
	register_base_address_p[M64_REGISTER_CUR_HORZ_VERT_OFF_OFFSET] = 
		register_values_p[M64_REGISTER_CUR_HORZ_VERT_OFF_OFFSET] &
			CUR_HORZ_VERT_OFF_BITS; 
	register_base_address_p[M64_REGISTER_CUR_HORZ_VERT_POSN_OFFSET] = 
		register_values_p[M64_REGISTER_CUR_HORZ_VERT_POSN_OFFSET] &
			CUR_HORZ_VERT_POSN_BITS; 
	
	/*
	 * Restore the cursor color.
	 */
	fg_color = 
		screen_state_p->cursor_state_p->cursor_list_pp[0]->foreground_color;
	bg_color = 
		screen_state_p->cursor_state_p->cursor_list_pp[0]->background_color;

	M64_CURSOR_SET_CURSOR_COLOR(fg_color,bg_color)

	/*
	 * enable the cursor.
	 */
	M64_CURSOR_ENABLE_HARDWARE_CURSOR();
}


/*
 * Software Cursor Routines.
 */

/*
 * Function for nullpadding the undefined bits in the cursor source
 * and inverse source bitmaps.
 */
STATIC void
m64_cursor_clear_extra_bits_and_copy(SICursorP si_cursor_p,
	SIbitmapP si_src_p, SIbitmapP si_invsrc_p,
	SIbitmapP si_mask_p)
{
	int bitmap_width;
	int bitmap_height;
	int bitmap_step_1;
	char *src_p_1;
	char *mask_p_1;
	char *invsrc_p_1;
	char *src_p_2;
	char *mask_p_2;
	char *invsrc_p_2;
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	
	ASSERT(si_cursor_p->SCsrc->BbitsPerPixel == 1);
	/*
	 * We know that the cursor bitmaps are always 1 bit deep
	 * Cursors of width and heights greater than default will
	 * be clipped to the default width and height.
	 */
	bitmap_width = 
		(((si_cursor_p->SCsrc->Bwidth > M64_SOFTWARE_CURSOR_MAX_WIDTH() ?
		M64_SOFTWARE_CURSOR_MAX_WIDTH() : si_cursor_p->SCsrc->Bwidth) + 31 ) & 
		~31 ) >> 3;

	bitmap_height = 
		(si_cursor_p->SCsrc->Bheight > M64_SOFTWARE_CURSOR_MAX_HEIGHT() ?
		M64_SOFTWARE_CURSOR_MAX_HEIGHT() : si_cursor_p->SCsrc->Bheight); 

	bitmap_step_1 = ((si_cursor_p->SCsrc->Bwidth + 31) & ~31) >> 3;

	ASSERT((bitmap_width > 0) && (bitmap_height > 0));

#if defined ( __DEBUG__ )
	if ( m64_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
			"(m64_cursor_clear_extra_bits_and_copy)\n"
			"{\n"
			"\tbitmap_width= %d\n"
			"\tbitmap_height= %d\n"
			"}\n",
			bitmap_width, bitmap_height);
	}
#endif
	/*
	 * Setup source pointers
	 */
	src_p_1 = (char*)si_cursor_p->SCsrc->Bptr;
	invsrc_p_1 = (char*)si_cursor_p->SCinvsrc->Bptr;
	mask_p_1 = (char*)si_cursor_p->SCmask->Bptr;

	/*
	 * Setup destination pointers
	 */
	src_p_2 = (char*)si_src_p->Bptr;
	invsrc_p_2 = (char*)si_invsrc_p->Bptr;
	mask_p_2 = (char*)si_mask_p->Bptr;

	/*
	 * Copy bytes from source to destination masking all
	 * extra set bits
	 */
	ASSERT((bitmap_width > 0) && (bitmap_height > 0));
	for(;bitmap_height > 0; --bitmap_height)
	{
		char *tmp_src_p_1 = src_p_1;
		char *tmp_mask_p_1 = mask_p_1;
		char *tmp_invsrc_p_1 = invsrc_p_1;
		int tmp_bitmap_width = bitmap_width;

		for(;tmp_bitmap_width > 0; --tmp_bitmap_width)
		{
			*src_p_2 = *tmp_src_p_1 & *tmp_mask_p_1;
			*invsrc_p_2 = *tmp_invsrc_p_1 & *tmp_mask_p_1;
			*mask_p_2 = (*invsrc_p_2 | *src_p_2);
			++src_p_2;
			++mask_p_2;
			++invsrc_p_2;
			++tmp_src_p_1;
			++tmp_mask_p_1;
			++tmp_invsrc_p_1;
		}
		src_p_1 += bitmap_step_1;
		mask_p_1 += bitmap_step_1;
		invsrc_p_1 += bitmap_step_1;
	}

	return;
}

/*
 * m64_cursor_software_new_cursor
 */

STATIC int 
m64_cursor_software_new_cursor(struct m64_cursor *active_cursor_p,
	SICursor *cursor_p )
{
	int height;
	int	width;
	int total_bytes;
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();

#if defined ( __DEBUG__ )
	if ( m64_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
			"(m64_cursor_create_cursor)\n"
			"{\n"
			"\tactive_cursor_p= %p\n"
			"\tcursor_p= %p\n"
			"}\n",
			(void*)active_cursor_p,
			(void*)cursor_p);
	}
#endif

	ASSERT((active_cursor_p != NULL)&&( cursor_p != NULL));
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,active_cursor_p));

	height = cursor_p->SCheight;
	width = cursor_p->SCwidth;
	active_cursor_p->cursor_height = 
	    ((height > M64_SOFTWARE_CURSOR_MAX_HEIGHT() ) ?
		 M64_SOFTWARE_CURSOR_MAX_HEIGHT() : height);

	active_cursor_p->cursor_width = 
	    ((width > M64_SOFTWARE_CURSOR_MAX_WIDTH() ) ?
		 M64_SOFTWARE_CURSOR_MAX_WIDTH() : width);

	active_cursor_p->foreground_color = cursor_p->SCfg;
	active_cursor_p->background_color = cursor_p->SCbg;

	/*
	 * Filter all extra (set) bits from source and inverse source bitmaps
	 * by applying mask to it.
	 */

	total_bytes = 
		(((cursor_p->SCsrc->Bwidth > M64_SOFTWARE_CURSOR_MAX_WIDTH() ?
		M64_SOFTWARE_CURSOR_MAX_WIDTH() : cursor_p->SCsrc->Bwidth)+ 31 ) & ~31 )
		>> 3;
	total_bytes  *= 
		(cursor_p->SCsrc->Bheight > M64_SOFTWARE_CURSOR_MAX_HEIGHT() ?
		M64_SOFTWARE_CURSOR_MAX_HEIGHT() : cursor_p->SCsrc->Bheight); 

	ASSERT(total_bytes > 0 );

	(void) memcpy(active_cursor_p->software_cursor_p->source_p,
		cursor_p->SCsrc, sizeof(SIbitmap));
	(void) memcpy(active_cursor_p->software_cursor_p->inverse_source_p,
		cursor_p->SCinvsrc, sizeof(SIbitmap));
	(void) memcpy(active_cursor_p->software_cursor_p->mask_p,
		cursor_p->SCmask, sizeof(SIbitmap));

	/*
	 * Allocate temporary space for bitmaps	
	 */

	active_cursor_p->software_cursor_p->source_p->Bptr = 
		allocate_memory(total_bytes);
	active_cursor_p->software_cursor_p->inverse_source_p->Bptr = 
		allocate_memory(total_bytes);
	active_cursor_p->software_cursor_p->mask_p->Bptr = 
		allocate_memory(total_bytes);
	
	/*
	 * NOTE : mask_bitmap is not being used in this function
	 */

	m64_cursor_clear_extra_bits_and_copy(cursor_p,
		active_cursor_p->software_cursor_p->source_p,
		active_cursor_p->software_cursor_p->inverse_source_p,
		active_cursor_p->software_cursor_p->mask_p);

	return 1;
}

/*
 * m64_cursor_software_download
 */
STATIC SIBool
m64_cursor_software_download( SIint32 cursor_index, SICursorP cursor_p )
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	struct m64_cursor  *active_cursor_p;
	

#if defined(__DEBUG__)
	if (m64_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
			"(m64_cursor_software_download)\n"
			"{\n"
			"\tcursor_index= %d\n"
			"\tcursor_p= %p\n"
			"\tcursor_width= %d\n"
			"\tcursor_height= %d\n}\n",
			(int)cursor_index,
			(void*)cursor_p,
			(int)cursor_p->SCwidth,
			(int)cursor_p->SCheight);
	}
#endif
	ASSERT ( M64_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR, active_cursor_p));

	/*
	 * If the cursor being downloaded is currently in use and turned ON
	 * then turn it OFF, download the new cursor and then turn it ON
	 */
	if ((m64_cursor_state_p->current_cursor_status.current_index ==
		 cursor_index) && 
		 (m64_cursor_state_p->current_cursor_status.flag == CURSOR_ON))
	{
		(*active_cursor_p->turnoff_function_p)(cursor_index);

		if (m64_cursor_software_new_cursor(active_cursor_p, cursor_p) == 0 )
		{
			return SI_FAIL;
		}
		return (*active_cursor_p->turnon_function_p)(cursor_index);
	}
	else
	{
		return ((m64_cursor_software_new_cursor(active_cursor_p,cursor_p) == 0)
				? SI_FAIL : SI_SUCCEED);
	}
}

/*
 * m64_cursor_save_obscured_screen 
 */

STATIC int
m64_cursor_save_obscured_screen(int cursor_index)  
{
	int height;
	int width;
	int x1;
	int y1;
	int x2;
	int y2;
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	struct m64_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
"(m64_cursor_save_obscured_screen){"
"\n"
"\tcursor_index= %d\n}\n",
		cursor_index);
	}
#endif
	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,active_cursor_p));

	/*
	 * determine the co-ordinates of the rectangular area that
	 * has to be saved: (x1,y1,x2,y2)
	 */
	x1 = m64_cursor_state_p->current_cursor_status.x;
	y1 = m64_cursor_state_p->current_cursor_status.y;
	x2 = m64_cursor_state_p->current_cursor_status.x +
		active_cursor_p->cursor_width;
	y2 = m64_cursor_state_p->current_cursor_status.y +
		active_cursor_p->cursor_height;

	/*
	 * Check if it is outside the visible area and adjust co-ords
	 * if required
	 */
	 x1 = (x1 < 0) ? 0 : x1;
	 y1 = (y1 < 0) ? 0 : y1;
	 x2 = (x2 > M64_MAX_VIRTUAL_X_PIXELS()) ?
			 M64_MAX_VIRTUAL_X_PIXELS() : x2;
	 y2 = (y2 > M64_MAX_VIRTUAL_Y_PIXELS()) ?
			 M64_MAX_VIRTUAL_Y_PIXELS() : y2;

	/*
	 * Calculate width and height of the rectangular save area
	 */
	width = x2 - x1;
	height = y2 - y1;
	m64_cursor_state_p->saved_screen_area_p->Bwidth = width;
	m64_cursor_state_p->saved_screen_area_p->Bheight = height;

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_save_obscured_screen){\n"
"\t#Co-ords of the rectangular area to be saved\n"
"\tul.x = %d\n"
"\tul.y = %d\n"
"\tlr.x = %d\n"
"\tlr.y = %d\n"
"\twidth = %d\n"
"\thgt = %d\n}\n",
		x1,y1,x2,y2,width,height);
	}
#endif
	
	if (m64_cursor_state_p->saved_screen_area_p == NULL)  
	{
		return 0;
	}

	ASSERT((width > 0) && (height > 0));
	ASSERT((x1 >= 0) && (y1 >= 0));
	if (m64_cursor_state_p->saved_screen_area_p)  
	{
		int 	stride = screen_state_p->framebuffer_stride;
		int		depth_shift = screen_state_p->generic_state.screen_depth_shift;
		unsigned long   *tmp_frame_buffer_p; 
		unsigned int	source_offset;
		unsigned int	destination_step = 
			(((unsigned)(m64_cursor_state_p->saved_screen_area_p->
			Bwidth << depth_shift) >> 3U)+ 3) & ~3;	

		/*
		 * compute the source offset from the base.
		 */
		source_offset = x1 + (y1 * (((unsigned)stride << 3U) >> depth_shift));

		/*
		 * Pointer to the beginning of the screen area to be saved.
		 */
		tmp_frame_buffer_p = ((unsigned long *) framebuffer_p) + 
			((unsigned)(source_offset << depth_shift) >> 5U); 

		/*
		 * Convert source offset to offset into first longword.
		 */
		source_offset = source_offset & ((32 >> depth_shift ) - 1);

		/*
		 * Copy the screen area into a bitmap.
		 * The screen area will be saved at the offset (0,0)
		 * of the bitmap.
		 */
		screen_state_p->transfer_pixels_p(tmp_frame_buffer_p, 
			(unsigned long *)m64_cursor_state_p->saved_screen_area_p->Bptr,
			source_offset , 0,
			stride, destination_step , width, height, 
			m64_cursor_state_p->saved_screen_area_p->BbitsPerPixel,
			GXcopy,
			(~0U >> (32U - screen_state_p->generic_state.screen_depth)),
			screen_state_p->pixels_per_long_shift);

		return 1;
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}
}


/*
 * m64_cursor_restore_obscured_screen 
 */
STATIC int
m64_cursor_restore_obscured_screen ( int cursor_index )
{
	int	height;
	int width;
	int	x;
	int	y;
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	struct m64_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_restore_obscured_screen){\n"
"\tcursor_index= %d\n"
"}\n",
		cursor_index);
	 }
#endif

	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,active_cursor_p));

	width = m64_cursor_state_p->saved_screen_area_p->Bwidth; 
	height = m64_cursor_state_p->saved_screen_area_p->Bheight; 

	ASSERT((width > 0) && (height > 0));
	if (m64_cursor_state_p->saved_screen_area_p)
	{
		int 	stride = screen_state_p->framebuffer_stride;
		int		depth_shift = screen_state_p->generic_state.screen_depth_shift;
		unsigned long   *tmp_frame_buffer_p; 
		unsigned int	destination_offset;
		unsigned int	source_step =
			(((unsigned)(m64_cursor_state_p->saved_screen_area_p->
			Bwidth << depth_shift) >> 3U)+ 3) & ~3;	

		x = (m64_cursor_state_p->current_cursor_status.x < 0) ?
			0 : m64_cursor_state_p->current_cursor_status.x;
		y = (m64_cursor_state_p->current_cursor_status.y < 0) ?
			0 : m64_cursor_state_p->current_cursor_status.y;

		ASSERT((x >= 0) && (y >= 0));
		/*
		 * compute the offset from in pixels from the base.
		 */
		destination_offset = x + 
			(y * (((unsigned)stride << 3U) >> depth_shift));
		tmp_frame_buffer_p = ((unsigned long *) framebuffer_p) + 
			((unsigned)(destination_offset << depth_shift) >> 5U); 
		/*
		 * Convert destination offset to offset into first longword.
		 */
		destination_offset = destination_offset & ((32U >> depth_shift) - 1 );

		/*
		 * Copy the the saved bitmap onto the screen.
		 */
		screen_state_p->transfer_pixels_p(
			(unsigned long *)m64_cursor_state_p->saved_screen_area_p->Bptr,
			tmp_frame_buffer_p, 
			0 , destination_offset,
			source_step, stride, width, height, 
			m64_cursor_state_p->saved_screen_area_p->BbitsPerPixel,
			GXcopy,
			(~0U >> (32U - screen_state_p->generic_state.screen_depth)),
			screen_state_p->pixels_per_long_shift);

		return 1;
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}
}

/*
 * m64_cursor_stplblt
 */
STATIC void
m64_cursor_stplblt(SIbitmapP source_bitmap_p,
	 int source_x, int source_y,
	 int destination_x, int destination_y,
	 int width, int height)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	unsigned long	*source_bits_p;
	unsigned int	source_step;
	unsigned int	number_of_host_data_words_per_width;
	int				delta;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	
#if (defined(__DEBUG__))
	if (m64_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_cursor_stplblt)\n"
			"{\n"
			"\tsource_bitmap_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n"
			"}\n",
			(void *) source_bitmap_p, source_x, source_y,
			destination_x, destination_y, width, height);
	}
#endif

	ASSERT((height > 0) || (width > 0));
	ASSERT((destination_x < M64_MAX_VIRTUAL_X_PIXELS()) && 
		(destination_y < M64_MAX_VIRTUAL_Y_PIXELS()));

	/*
	 * Transparent stippling.
	 * For transparent case program BG_ROP to be DST.
	 */
	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) =
		(*(register_values_p + M64_REGISTER_DP_MIX_OFFSET) & 
		~DP_MIX_DP_BKGD_MIX) | DP_MIX_GXnoop;

	/*
	 * Set the clipping rectangle to correspond to the actual drawing area.
	 * Invalidate the clipping rectangle.
	 */
	M64_WAIT_FOR_FIFO(2);
	*(register_base_address_p + M64_REGISTER_SC_LEFT_RIGHT_OFFSET) = 
		destination_x | 
		((destination_x + width - 1) << SC_LEFT_RIGHT_SC_RIGHT_SHIFT);
	*(register_base_address_p + M64_REGISTER_SC_TOP_BOTTOM_OFFSET) =
		destination_y | 
		((destination_y + height - 1) << SC_TOP_BOTTOM_SC_BOTTOM_SHIFT);
	screen_state_p->generic_state.screen_current_clip = 
		M64_INVALID_CLIP_RECTANGLE;

	/*
	 * Align the source to the previous long word boundary.
	 * Adjust the destination and the width appropriately.
	 */
	/*CONSTANTCONDITION*/
	ASSERT(HOST_DATA_REGISTER_WIDTH == 32);
	if ((delta = (source_x & (HOST_DATA_REGISTER_WIDTH - 1))) != 0)
	{
		source_x &= ~(HOST_DATA_REGISTER_WIDTH - 1);
		destination_x -= delta;
		width += delta;
	}

	/*
	 * compute and round off the source step to a long word boundary.
	 */
	source_step  = (((unsigned)(source_bitmap_p->Bwidth)+ 31) & ~31) >> 5U;	
												
	/*
	 * compute the source pointer for the new (source_x,source_y).
	 */
	source_bits_p = (unsigned long *)source_bitmap_p->Bptr +
		(source_y * source_step) + ((unsigned)source_x >> 5U);

	/*
	 * Now adjust width so that we write an integral number of 
	 * host data words. Also compute the number of host data long 
	 * words per width.
	 */
	width =	(width + HOST_DATA_REGISTER_WIDTH - 1) & 
		~(HOST_DATA_REGISTER_WIDTH - 1);
	number_of_host_data_words_per_width = (unsigned)width >> 5U;

	/*
	 * Program the gui trajectory control register for a normal blit.
	 * Program the DP_PIX_WID register to select host_pix_wid to 1bpp.
	 * Program the DP_SRC register to select monochrome host data as the source.
	 * Program the destination gui engine registers and initiate the blit.
	 */
	M64_WAIT_FOR_FIFO(5);

	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) =
		register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET];

	*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] & 
		~DP_PIX_WID_DP_HOST_PIX_WID;

	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BKGD_COLOR) |
		(DP_SRC_FRGD_COLOR <<  DP_SRC_DP_FRGD_SRC_SHIFT) |
		(DP_SRC_DP_MONO_SRC_HOST_DATA << DP_SRC_DP_MONO_SRC_SHIFT);

	*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = destination_y |
		((unsigned)destination_x << DST_Y_X_DST_X_SHIFT); 
	ASSERT(height > 0 && width > 0);
	*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = height | 
		((unsigned)width << DST_HEIGHT_WID_DST_WID_SHIFT);

	/*
	 * Wait for the initiator to get executed.
	 */
	M64_WAIT_FOR_FIFO(16);
	do
	{
		register unsigned long *tmp_source_p = source_bits_p; 
		register int tmp = number_of_host_data_words_per_width;

		/*
		 * Pump the stipple bits host_data_transfer_blocking_factor longs
		 * at a time.
		 */
		while(tmp >= screen_state_p->host_data_transfer_blocking_factor)
		{
			register int i = screen_state_p->host_data_transfer_blocking_factor;
				
			tmp -= i;

			/*
			 * pump to host data register.
			 */
			M64_WAIT_FOR_FIFO(i);
			do
			{
				*(register_base_address_p + M64_REGISTER_HOST_DATA0_OFFSET) =
					*tmp_source_p++;
			} while (--i);
		}

		/*
		 * Do whatever long words remain.
		 */
		if (tmp > 0)
		{
			/*
			 * pump to host data register.
			 */
			M64_WAIT_FOR_FIFO(tmp);

			do
			{
				*(register_base_address_p + M64_REGISTER_HOST_DATA0_OFFSET) =
					*tmp_source_p++;
			}while (--tmp);
		}
		source_bits_p += source_step;
	} while (--height);

	M64_WAIT_FOR_FIFO(2);
	register_base_address_p[M64_REGISTER_DP_PIX_WID_OFFSET] =
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET];
	register_base_address_p[M64_REGISTER_DP_MIX_OFFSET] =
		register_values_p[M64_REGISTER_DP_MIX_OFFSET];

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif

	return;
}

/*
 * m64_cursor_draw_software_cursor 
 */
STATIC int
m64_cursor_draw_software_cursor(int cursor_index)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	unsigned long saved_frgd_clr = 
		register_values_p[M64_REGISTER_DP_FRGD_CLR_OFFSET];
	int x;
	int y;
	int source_x;
	int source_y;
	int width;
	int height;
	struct m64_cursor *active_cursor_p;
	SIbitmapP inverse_source_p = NULL;
	SIbitmapP source_p;


	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,active_cursor_p));

	width = active_cursor_p->cursor_width; 
	height = active_cursor_p->cursor_height; 

	x = m64_cursor_state_p->current_cursor_status.x;
	y = m64_cursor_state_p->current_cursor_status.y;

	source_x = 0;
	source_y = 0;

	if (x < 0)
	{
		source_x = -x;
		width += x;
		x = 0;
	}
	
	if (y < 0)
	{
		source_y = -y;
		height += y;
		y = 0;
	}
	if (x + width  >= M64_MAX_VIRTUAL_X_PIXELS())
	{
		width -= (x + width -  M64_MAX_VIRTUAL_X_PIXELS());
	}
	
	if (y + height >= M64_MAX_VIRTUAL_Y_PIXELS())
	{
		height -= (y + height - M64_MAX_VIRTUAL_Y_PIXELS());
	}

	source_p = active_cursor_p->software_cursor_p->source_p;
	inverse_source_p = active_cursor_p->software_cursor_p->inverse_source_p;

	/*
	 * save the register values of the registers dp_frgd_clr
	 * and dp_bkgd_clr.
	 * First set the dp_frgd_clr to the foreground color of the cursor.
	 */
	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_DP_FRGD_CLR_OFFSET) =
		active_cursor_p->foreground_color;
	
	/*
	 * Stipple the source bitmap
	 */

	m64_cursor_stplblt(source_p,source_x,source_y,x,y,width,height);

	/*
	 * set the dp_frgd_clr to the background color of the cursor.
	 */
	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_DP_FRGD_CLR_OFFSET) =
		active_cursor_p->background_color;

	/*
	 * Stipple using the inverse source bitmap
	 */

	m64_cursor_stplblt(inverse_source_p,source_x,source_y,x,y,width,height);

	/*
	 * restore the dp_frgd_clr register to its saved value.
	 */
	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_DP_FRGD_CLR_OFFSET) = 
		saved_frgd_clr;

	return 1;

}

/*
 * m64_cursor_software_turnon  
 */
STATIC SIBool
m64_cursor_software_turnon(SIint32 cursor_index)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct m64_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
			"(m64_cursor_software_turnon){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif
	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,active_cursor_p));
#endif

	if (m64_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		return SI_SUCCEED;
	}
	if (m64_cursor_save_obscured_screen(cursor_index) == 0)
	{
		return SI_FAIL;
	}
	if (m64_cursor_draw_software_cursor(cursor_index) == 0)
	{
		return SI_FAIL;
	}

	m64_cursor_state_p->current_cursor_status.flag = CURSOR_ON;
	m64_cursor_state_p->current_cursor_status.current_index = cursor_index;

	return SI_SUCCEED;
}

/*
 * m64_cursor_software_turnoff
 */
STATIC SIBool
m64_cursor_software_turnoff(SIint32 cursor_index)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct m64_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(m64_cursor_software_turnoff){\n"
			"\tcursor_index= %d\n}\n",
			(int)cursor_index);
	 }
#endif

	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,active_cursor_p));
#endif

	if (m64_cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		return SI_SUCCEED;
	}
	else
	{
		m64_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
	}

 	if (m64_cursor_restore_obscured_screen(cursor_index) == 0)
	{
		return SI_FAIL;
	}
	else
	{
		return SI_SUCCEED;
	}
}


/*
 * m64_cursor_software_move
 */

STATIC SIBool
m64_cursor_software_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	struct m64_cursor *active_cursor_p;
	int	cursor_width;
	int	cursor_height;
	int cursor_turned_off = 0;
	int	crtoffset_change_required = 0;

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
"(m64_cursor_software_move){\n"
"\tcursor_index= %d\n"
"\tx = %d\n"
"\ty= %d\n}\n",
		(int)cursor_index,
		(int)x, (int)y);
	 }
#endif

		
	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,active_cursor_p));

	if (m64_cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		m64_cursor_state_p->current_cursor_status.x = x;
		m64_cursor_state_p->current_cursor_status.y = y;
		return SI_SUCCEED;
	}

	/*
	 * Check if panning will be needed 
	 */
	if (M64_CURSOR_IS_PANNING_FEASIBLE())
	{
		cursor_height = active_cursor_p->cursor_height;
		cursor_width  = active_cursor_p->cursor_width;

		if ( x + cursor_width > 
				m64_cursor_state_p->visible_window_top_x +
					 M64_MAX_DISPLAYED_X_PIXELS())
		{
			m64_cursor_state_p->visible_window_top_x = 
				(x + cursor_width > M64_MAX_VIRTUAL_X_PIXELS()) ?
					(M64_MAX_VIRTUAL_X_PIXELS() -
						 M64_MAX_DISPLAYED_X_PIXELS())	:
					(x + cursor_width - M64_MAX_DISPLAYED_X_PIXELS());
			crtoffset_change_required = 1;
		}
		else
		{
			if (x < m64_cursor_state_p->visible_window_top_x &&
					m64_cursor_state_p->visible_window_top_x > 0)
			{
				if ( x >= 0)
				{
					m64_cursor_state_p->visible_window_top_x = x;
				}
				else
				{
					m64_cursor_state_p->visible_window_top_x =0;
				}
					
				crtoffset_change_required = 1;
			}
		}
		if ( y + cursor_height > 
				m64_cursor_state_p->visible_window_top_y +
					 M64_MAX_DISPLAYED_Y_PIXELS())
		{
			m64_cursor_state_p->visible_window_top_y = 
				(y + cursor_height > M64_MAX_VIRTUAL_Y_PIXELS()) ?
					(M64_MAX_VIRTUAL_Y_PIXELS() -
						 M64_MAX_DISPLAYED_Y_PIXELS())	:
					(y + cursor_height - M64_MAX_DISPLAYED_Y_PIXELS());

			crtoffset_change_required = 1;
		}
		else
		{
			if (y < m64_cursor_state_p->visible_window_top_y &&
					m64_cursor_state_p->visible_window_top_y > 0)
			{

				if ( y >= 0)
				{
					m64_cursor_state_p->visible_window_top_y = y;
				}
				else
				{
					m64_cursor_state_p->visible_window_top_y =0;
				}
				crtoffset_change_required = 1;
			}
		}
		if (crtoffset_change_required == 1)
		{
			int offset;

			ASSERT( (m64_cursor_state_p->visible_window_top_x >= 0) &&
				(m64_cursor_state_p->visible_window_top_y >= 0));

			M64_CURSOR_CALCULATE_OFFSET(
				m64_cursor_state_p->visible_window_top_x,
				m64_cursor_state_p->visible_window_top_y,
				offset);
#if defined(__DEBUG__)
			if (m64_cursor_debug)
			{
				(void)fprintf(debug_stream_p,
"(m64_cursor_software_move){\n"
"\twindow_top_x = %d\n"
"\twindow_top_y = %d\n"
"}",
				m64_cursor_state_p->visible_window_top_x,
				m64_cursor_state_p->visible_window_top_y);
			}
#endif
						
			outl(M64_IO_REGISTER_CRTC_OFF_PITCH, 
				(*(register_values_p + M64_REGISTER_CRTC_OFF_PITCH_OFFSET) &
				CRTC_OFF_PITCH_CRTC_PITCH) | (offset >> 1));
			(void) m64_cursor_software_turnoff(cursor_index);
			cursor_turned_off =1;
		}

	}

	/*
	 * if the cursor is ON, the cursor is first turned off, the
	 * new position is set and then cursor is turned on
	 */
	if(!cursor_turned_off)
	{
		(*active_cursor_p->turnoff_function_p)( cursor_index);
	}

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(m64_cursor_software_move){\n"
			"\tcursor_index= %ld\n"
			"\t(%d,%d)---->(%ld,%ld)\n"
			"}\n",
		    cursor_index,
			m64_cursor_state_p->current_cursor_status.x,
			m64_cursor_state_p->current_cursor_status.y,
		    x,
		    y);
	}
#endif

	m64_cursor_state_p->current_cursor_status.x = x;
	m64_cursor_state_p->current_cursor_status.y = y;
	m64_cursor_state_p->current_cursor_status.current_index = cursor_index;

	return (*active_cursor_p->turnon_function_p)(cursor_index);
}



/*
 * Hardware Cursor Routines.
 */

/*
 * m64_cursor_generate_hardware_cursor
 */
STATIC void
m64_cursor_generate_hardware_cursor(
	SIbitmap *mask_p,
	SIbitmap *src_p,
	unsigned long *result_p)
{
	int i;
	int j;
	int balance ;
	int long_word_cnt = 0;
	int	 shift=0 ;
	int src_word, mask_word;
	unsigned long result_word ;
	int bitmask ;
	SIArray src ;
	SIArray mask ;
	int cursor_width ;
	int inc ;
	int effective_width = 0 ;
	int effective_height = 0 ;
	unsigned long *resultant_p = result_p ;
	int no_of_long_words_per_cursor_line;

#if (defined(__DEBUG__))
	int	number_of_bytes ;
#endif

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();

	/* Each pixel in the resultant map is represented by two  bits
	**  src_bit		mask_bit		resultant_bits
	**  ----------------------------------------------------
	**	1			1				00 		Foreground color
	**	0			1				01		Background color
	**	0			0				10		Transparent
	**  ----------------------------------------------------
	*/

#define	FOREGROUND_COLOR	0x00000000		

#define	BACKGROUND_COLOR	0x00000001

#define TRANSPARENT			0x00000002


	ASSERT((mask_p != NULL) && (src_p != NULL) && (result_p != NULL)) ;

	ASSERT((mask_p->Bwidth == src_p->Bwidth)&&\
			(mask_p->Bheight == src_p->Bheight) ) ;

	ASSERT((mask_p->BbitsPerPixel == 1) && (src_p->BbitsPerPixel == 1) );


#if defined(__DEBUG__)
	number_of_bytes = (((mask_p->Bwidth + 31 ) & ~31 ) >> 3)*mask_p->Bheight ;

	if	( m64_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
"(m64_cursor_generate_hardware_cursor){\n"
"\tmask_p= %p\n"
"\tsrc_p= %p\n"
"\tresult_p= %p\n"
"\tnumber_of_bytes= %d\n"
"}\n",
		    (void*)mask_p,
		    (void*)src_p,
		    (void*)result_p,
		    number_of_bytes);
	}
#endif

	src = src_p->Bptr ;

	mask = mask_p->Bptr ;

	bitmask = 0x01;

	cursor_width = mask_p->Bwidth ;

	if (mask_p->Bwidth > M64_HARDWARE_CURSOR_MAX_WIDTH())
	{
		inc = (((mask_p->Bwidth + 31 ) & ~31 ) >> 3)/4;

		inc -= ((M64_HARDWARE_CURSOR_MAX_WIDTH()/32)-1) ;

#if defined ( __DEBUG__ )
		if ( m64_cursor_debug )
		{
			(void)fprintf ( debug_stream_p,
"(m64_cursor_generate_hardware_cursor)\n"
"{\n"
"\t# Truncating cursor\n"
"\tinc = %d\n"
"}\n",
			    inc);
		}
#endif
	}

	effective_height = ((mask_p->Bheight>M64_HARDWARE_CURSOR_MAX_HEIGHT())?
	    M64_HARDWARE_CURSOR_MAX_HEIGHT():mask_p->Bheight);

	effective_width = ((mask_p->Bwidth>M64_HARDWARE_CURSOR_MAX_WIDTH())?
	    M64_HARDWARE_CURSOR_MAX_WIDTH():mask_p->Bwidth);

	no_of_long_words_per_cursor_line = (M64_HARDWARE_CURSOR_MAX_WIDTH() * 
		M64_HARDWARE_CURSOR_DEPTH)/32 ;

	if ((M64_HARDWARE_CURSOR_MAX_WIDTH()*M64_HARDWARE_CURSOR_DEPTH) % 32)
	{
		++no_of_long_words_per_cursor_line;
	}
	

	for (i = 1; i <= effective_height; ++i)
	{
		src_word = *src ;

		mask_word = *mask ;

		result_word = 0 ;

		long_word_cnt = 0 ;

		if (cursor_width < M64_HARDWARE_CURSOR_MAX_WIDTH())
		{
			result_p += (((M64_HARDWARE_CURSOR_MAX_WIDTH() - cursor_width) * 
						M64_HARDWARE_CURSOR_DEPTH) / 32);

			balance = (((M64_HARDWARE_CURSOR_MAX_WIDTH() - cursor_width) *
						M64_HARDWARE_CURSOR_DEPTH) % 32);

			shift = balance;
		}
		else
		{
			shift = 0 ;
		}

		for (j = 1; j<=effective_width; ++j)
		{

			if ( src_word & bitmask )
			{
				if ( (mask_word & bitmask) == bitmask )
				{
					result_word |= (FOREGROUND_COLOR<<shift) ;
				}
				else
				{
					result_word |= (TRANSPARENT<<shift) ;
				}
			}
			else
			{
				if ( (mask_word & bitmask) == bitmask )
				{
					result_word |= (BACKGROUND_COLOR<<shift) ;
				}
				else
				{
					result_word |= (TRANSPARENT<<shift) ;
				}

			}

			shift += M64_HARDWARE_CURSOR_DEPTH;

			if ( (j%32==0) && (j<effective_width) )
			{
				/*
				** row spans over multiple longs
				*/
				++src ;
				++mask ;
				src_word = *src ;
				mask_word = *mask ;

			}
			else
			{
				src_word = src_word>>1 ;
				mask_word = mask_word>>1 ;

			}

			/*
			** each long word in resultant map can hold
			** only 16 pixels
			*/

			if ((shift == 32) && (j<effective_width) )
			{
				/*
				** resultant row spans over multiple words
				*/
				result_p[long_word_cnt] = result_word ;

				long_word_cnt += 1;

				shift = 0 ;

				result_word = 0 ;

			}
		}/*for*/

		result_p[long_word_cnt] = result_word ;

		if (cursor_width < M64_HARDWARE_CURSOR_MAX_WIDTH())
		{
			result_p += no_of_long_words_per_cursor_line - 
						((M64_HARDWARE_CURSOR_MAX_WIDTH() - cursor_width) * 
							M64_HARDWARE_CURSOR_DEPTH) / 32;
		}
		else
		{
			result_p += no_of_long_words_per_cursor_line ;
		}

		if (mask_p->Bwidth > M64_HARDWARE_CURSOR_MAX_WIDTH())
		{
			src += inc ;
			mask += inc ;
		}
		else
		{
			++src ;
			++mask ;
		}

	}
}

/*
 * m64_cursor_hardware_new_cursor
 */
STATIC int 
m64_cursor_hardware_new_cursor(struct m64_cursor *active_cursor_p,
					 SICursor *si_cursor_p)
{
	int height ;
	int	width ;
	int dx ;
	int dy ;
	unsigned long *hardware_cursor_data_p ;
	unsigned long *tmp_frame_buffer_p; 

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();

#if defined ( __DEBUG__ )
	if ( m64_cursor_debug )
	{
		(void)fprintf	( debug_stream_p,
"(m64_cursor_hardware_new_cursor){\n"
"\tactive_cursor_p= %p\n"
"\tsi_cursor_p= %p}\n",
		    (void*)active_cursor_p,
		    (void*)si_cursor_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR_STATE, m64_cursor_state_p));
	ASSERT((active_cursor_p != NULL) && ( si_cursor_p != NULL ));
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR, active_cursor_p));

	height = si_cursor_p->SCheight ;

	width = si_cursor_p->SCwidth ;

	/*
	 * The representation of the HW cursor in the offscreen mem is
	 * different. 
	 * Two bits represent one pixel
	 */
	hardware_cursor_data_p = allocate_memory
		((M64_HARDWARE_CURSOR_MAX_WIDTH() * M64_HARDWARE_CURSOR_DEPTH *
		M64_HARDWARE_CURSOR_MAX_HEIGHT())/8);

	/*
	 * Set all pixels to transparent
 	 */
	(void) memset((unsigned char *)hardware_cursor_data_p, 0xaa,
		((M64_HARDWARE_CURSOR_MAX_WIDTH() * M64_HARDWARE_CURSOR_DEPTH *
		M64_HARDWARE_CURSOR_MAX_HEIGHT())  / 8));

	m64_cursor_generate_hardware_cursor(
		si_cursor_p->SCmask,si_cursor_p->SCsrc,hardware_cursor_data_p);

	active_cursor_p->cursor_height = 
	    ((height > M64_HARDWARE_CURSOR_MAX_HEIGHT() ) ?
		 M64_HARDWARE_CURSOR_MAX_HEIGHT() : height);

	active_cursor_p->cursor_width = 
	    ((width > M64_HARDWARE_CURSOR_MAX_WIDTH() ) ?
		 M64_HARDWARE_CURSOR_MAX_WIDTH() : width);

	active_cursor_p->foreground_color = si_cursor_p->SCfg;
	active_cursor_p->background_color = si_cursor_p->SCbg;

	dx = active_cursor_p->hardware_cursor_p->memory_representation.x;
	dy = active_cursor_p->hardware_cursor_p->memory_representation.y;

	/*
	 * set the cursor-offset (in long words).
	 */
	M64_CURSOR_CALCULATE_OFFSET(dx, dy,
	   active_cursor_p->hardware_cursor_p->cursor_offset);

	if (si_cursor_p->SCheight > M64_HARDWARE_CURSOR_MAX_HEIGHT())
	{
		active_cursor_p->hardware_cursor_p->vert_cursor_offset =  0;
	}
	else
	{
		active_cursor_p->hardware_cursor_p->vert_cursor_offset =  
		    M64_HARDWARE_CURSOR_MAX_HEIGHT() - si_cursor_p->SCheight;
	}

	if (si_cursor_p->SCwidth > M64_HARDWARE_CURSOR_MAX_WIDTH())
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset = 0;
	}
	else
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset =
		    M64_HARDWARE_CURSOR_MAX_WIDTH() - si_cursor_p->SCwidth;
	}

	tmp_frame_buffer_p = ((unsigned long *) framebuffer_p) + 
		active_cursor_p->hardware_cursor_p->cursor_offset;

	/*
	 * call the asm helper to transfer the hardware cursor data 
	 * generated to the offscreen.Since there will be 64x2x64 (1024 bytes)
	 * hardcode the width to 1024, height to 1, depth 8bpp, rop GXcopy.
	 * Source offset and destination offset are 0.
	 */
	screen_state_p->transfer_pixels_p(hardware_cursor_data_p,
		tmp_frame_buffer_p, 
		M64_CURSOR_SOURCE_OFFSET,
		M64_CURSOR_DESTINATION_OFFSET,
		M64_CURSOR_SOURCE_STRIDE,
		screen_state_p->framebuffer_stride,
		M64_CURSOR_BITMAP_WIDTH_IN_BYTES,
		M64_CURSOR_BITMAP_HEIGHT,
		8, GXcopy, 0xFF, 2); 

	free_memory (hardware_cursor_data_p) ;
	return 1;
}

/*
 *m64_hardware_cursor_download
 */

STATIC SIBool
m64_hardware_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE ();

	struct m64_cursor *active_cursor_p ;

#if defined(__DEBUG__)
	if (m64_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_hardware_download){\n"
"\tcursor_index= %d\n"
"\tcursor_p= %p\n"
"\tcursor_width= %d\n"
"\tcursor_height= %d\n"
"}\n",
		    (int)cursor_index,
		    (void*)cursor_p,
		    (int)cursor_p->SCwidth,
		    (int)cursor_p->SCheight);
	}
#endif
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR_STATE, m64_cursor_state_p));
	
	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));
	ASSERT(cursor_p != NULL);

	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR, active_cursor_p));


	/*
	 * If the cursor being downloaded is the current cursor, turn it
	 * off, reload the bits.
	 */
	if (m64_cursor_state_p->current_cursor_status.current_index ==
		cursor_index  && 
		m64_cursor_state_p->current_cursor_status.flag == CURSOR_ON )
	{
		/*
		 * Turn off existing cursor.
		 */
		(*active_cursor_p->turnoff_function_p) ( cursor_index );

		/*
		 * Create the new cursor.
		 */
		if (m64_cursor_hardware_new_cursor(active_cursor_p, cursor_p) == 0)
		{
			return SI_FAIL ;
		}
		
		active_cursor_p =
			m64_cursor_state_p->cursor_list_pp[cursor_index];

		/*
		 * Turn on the new cursor.
		 */
		return (*active_cursor_p->turnon_function_p) (cursor_index) ;
	}
	else
	{
		/*
		 * This cursor is at a different index than that currently on
		 * display.
		 */
		return ((m64_cursor_hardware_new_cursor(active_cursor_p,cursor_p) == 1)
				? SI_SUCCEED : SI_FAIL);
	}
}

/*
 * m64_cursor_draw_hardware_cursor
 */

STATIC int
m64_cursor_draw_hardware_cursor(int cursor_index)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;

	struct m64_cursor *active_cursor_p ;

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_draw_hardware_cursor){\n"
"\tcursor_index= %d\n"
"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR_STATE, m64_cursor_state_p));
	
	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));

	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR, active_cursor_p));

	M64_CURSOR_SET_CURSOR_COLOR(
	    active_cursor_p->foreground_color,active_cursor_p->background_color);

	/*
	 * set the cursor offset register.
	 * check if its in 64 bit word offset.
	 */
	outl(M64_IO_REGISTER_CUR_OFFSET,
		((active_cursor_p->hardware_cursor_p->cursor_offset) >> 1));
	/*
	 * set the horizontal and vertical offset register.
	 */
	outl(M64_IO_REGISTER_CUR_HORZ_VERT_OFF,
		(active_cursor_p->hardware_cursor_p->horz_cursor_offset |
		(active_cursor_p->hardware_cursor_p->vert_cursor_offset) << 16));

	/*
	 * set the cursor position.
	 */
	outl(M64_IO_REGISTER_CUR_HORZ_VERT_POSN,
	((m64_cursor_state_p->current_cursor_status.x > 0 ?
		m64_cursor_state_p->current_cursor_status.x : 0) |
	((m64_cursor_state_p->current_cursor_status.y > 0 ?
		m64_cursor_state_p->current_cursor_status.y : 0) << 16)));

	M64_CURSOR_ENABLE_HARDWARE_CURSOR();

#if defined ( __DEBUG__ )
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_draw_hardware_cursor){\n"
"\tcusor_offset= 0x%x\n"
"\tvert_cursor_offset= %d\n"
"\thorz_cursor_offset= %d\n"
"\tforeground_color = %d\n"
"\tbackground_color = %d\n"
"}\n",
		    active_cursor_p->hardware_cursor_p->cursor_offset,
		    active_cursor_p->hardware_cursor_p->vert_cursor_offset,
		    active_cursor_p->hardware_cursor_p->horz_cursor_offset,
		    active_cursor_p->foreground_color,
		    active_cursor_p->background_color);
	}
#endif
	return 1 ;
}/*m64_cursor_draw_hardware_cursor*/

/*
 * m64_hardware_cursor_turnon  
 */
STATIC SIBool
m64_hardware_cursor_turnon ( SIint32 cursor_index )
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE ();

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_hardware_turnon){\n"
"\tcursor_index= %ld\n"
"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR_STATE, m64_cursor_state_p));
	
	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));

	if (m64_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		return SI_SUCCEED ;
	}

	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,
		 m64_cursor_state_p->cursor_list_pp[cursor_index]));

	if (m64_cursor_draw_hardware_cursor(cursor_index) == 0)
	{
		return SI_FAIL ;
	}
	m64_cursor_state_p->current_cursor_status.flag = CURSOR_ON ;

	m64_cursor_state_p->current_cursor_status.current_index = cursor_index ;

	return SI_SUCCEED ;
}

/*
 * m64_hardware_cursor_turnoff
 */
STATIC SIBool
m64_hardware_cursor_turnoff ( SIint32 cursor_index )
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE ();
	unsigned long *register_values_p = screen_state_p->register_state.registers;

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_hardware_turnoff){\n"
"\tcursor_index= %ld\n"
"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR_STATE, m64_cursor_state_p));
	
	ASSERT(M64_CURSOR_IS_VALID_INDEX(cursor_index));

	/*
	 * Older X servers, repeatedly call cursor_off ...
	 */
	ASSERT(screen_state_p->generic_state.screen_server_version_number <
		   X_SI_VERSION1_1 || 
		   m64_cursor_state_p->current_cursor_status.flag != CURSOR_OFF);

	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR,
		m64_cursor_state_p->cursor_list_pp[cursor_index]));

	m64_cursor_state_p->current_cursor_status.flag = CURSOR_OFF ;

	M64_CURSOR_DISABLE_HARDWARE_CURSOR();

	return SI_SUCCEED ;
}

/*
 * m64_hardware_cursor_move
 */
STATIC SIBool
m64_hardware_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	struct m64_cursor *active_cursor_p;
	int oldx ;
	int oldy ;
	int cursor_height ;
	int cursor_width;
	int crtoffset_change_required = 0;

	oldx = m64_cursor_state_p->current_cursor_status.x ;
	oldy = m64_cursor_state_p->current_cursor_status.y ;

	m64_cursor_state_p->current_cursor_status.x = x ;
	m64_cursor_state_p->current_cursor_status.y = y ;

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_hardware_move){\n"
"\tcursor_index= %ld\n"
"\t(%d,%d)---->(%ld,%ld)\n"
"}\n",
		    cursor_index,
		    oldx,
		    oldy,
		    x,
		    y);
	}
#endif

	active_cursor_p = m64_cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR, active_cursor_p));

	/*
	 * Check if panning will be needed 
	 */
	if (M64_CURSOR_IS_PANNING_FEASIBLE())
	{
		cursor_height = active_cursor_p->cursor_height;
		cursor_width  = active_cursor_p->cursor_width;

		if ( x + cursor_width > 
				m64_cursor_state_p->visible_window_top_x +
					 M64_MAX_DISPLAYED_X_PIXELS())
		{
			m64_cursor_state_p->visible_window_top_x = 
				(x + cursor_width > M64_MAX_VIRTUAL_X_PIXELS()) ?
					(M64_MAX_VIRTUAL_X_PIXELS() -
						 M64_MAX_DISPLAYED_X_PIXELS())	:
					(x + cursor_width - M64_MAX_DISPLAYED_X_PIXELS());
			x -=  m64_cursor_state_p->visible_window_top_x;
			m64_cursor_state_p->current_cursor_status.x = x ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (x < m64_cursor_state_p->visible_window_top_x &&
					m64_cursor_state_p->visible_window_top_x > 0)
			{
				if ( x >= 0)
				{
					m64_cursor_state_p->visible_window_top_x = x;
					x = 0;
					m64_cursor_state_p->current_cursor_status.x = x ;
				}
				else
				{
					x += m64_cursor_state_p->visible_window_top_x; 
					m64_cursor_state_p->current_cursor_status.x = x ;
					m64_cursor_state_p->visible_window_top_x =0;
				}
					
				crtoffset_change_required = 1;
			}
			else
			{
				x -= m64_cursor_state_p->visible_window_top_x; 
				m64_cursor_state_p->current_cursor_status.x = x ;
			}
		}
		if ( y + cursor_height > 
				m64_cursor_state_p->visible_window_top_y +
					 M64_MAX_DISPLAYED_Y_PIXELS())
		{
			m64_cursor_state_p->visible_window_top_y = 
				(y + cursor_height > M64_MAX_VIRTUAL_Y_PIXELS()) ?
					(M64_MAX_VIRTUAL_Y_PIXELS() -
						 M64_MAX_DISPLAYED_Y_PIXELS())	:
					(y + cursor_height - M64_MAX_DISPLAYED_Y_PIXELS());

			y -=  m64_cursor_state_p->visible_window_top_y;
			m64_cursor_state_p->current_cursor_status.y = y ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (y < m64_cursor_state_p->visible_window_top_y &&
					m64_cursor_state_p->visible_window_top_y > 0)
			{

				if ( y >= 0)
				{
					m64_cursor_state_p->visible_window_top_y = y;
					y = 0;
					m64_cursor_state_p->current_cursor_status.y = y ;
				}
				else
				{
					y += m64_cursor_state_p->visible_window_top_y; 
					m64_cursor_state_p->current_cursor_status.y = y ;
					m64_cursor_state_p->visible_window_top_y =0;
				}
				crtoffset_change_required = 1;
			}
			else
			{
				y -= m64_cursor_state_p->visible_window_top_y; 
				m64_cursor_state_p->current_cursor_status.y = y ;
			}
		}
		if (crtoffset_change_required == 1)
		{
			int offset;

			ASSERT( (m64_cursor_state_p->visible_window_top_x >= 0) &&
				(m64_cursor_state_p->visible_window_top_y >= 0));

			M64_CURSOR_CALCULATE_OFFSET(
				m64_cursor_state_p->visible_window_top_x,
				m64_cursor_state_p->visible_window_top_y,
				offset);
#if defined(__DEBUG__)
			if (m64_cursor_debug)
			{
				(void)fprintf(debug_stream_p,
"(m64_cursor_hardare_move){\n"
"\twindow_top_x = %d\n"
"\twindow_top_y = %d\n"
"}",
				m64_cursor_state_p->visible_window_top_x,
				m64_cursor_state_p->visible_window_top_y);
			}
#endif
						
			outl(M64_IO_REGISTER_CRTC_OFF_PITCH, 
				(*(register_values_p + M64_REGISTER_CRTC_OFF_PITCH_OFFSET) &
				CRTC_OFF_PITCH_CRTC_PITCH) | (offset >> 1));
		}
	}

	/*
	 **If the previous move was within the visble region( i.e the full cursor
	 ** was visble)  and if this move is also within visble region 
	 ** then  boundary checking, updating various cursor
	 ** related registers is not necessary, just update the cursor position
	 ** registers and return
	 */

	if ((oldx >= 0)  && (oldy >= 0 ) && (x >= 0 ) && ( y >= 0 ))
	{
		/*
		 * set the cursor position.
		 */
		outl(M64_IO_REGISTER_CUR_HORZ_VERT_POSN,
		((m64_cursor_state_p->current_cursor_status.x > 0 ?
			m64_cursor_state_p->current_cursor_status.x : 0) |
		((m64_cursor_state_p->current_cursor_status.y > 0 ?
			m64_cursor_state_p->current_cursor_status.y : 0) << 16)));
		return SI_SUCCEED ;
	}

	
	/*
	 ** is the cursor at the left corner of the screen
	 ** or top of the screen
	 */
	if(x < 0)
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset = 
		    M64_HARDWARE_CURSOR_MAX_WIDTH() - 
			active_cursor_p->cursor_width + (-x);
		x = 0;
	}
	else
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset = 
		    M64_HARDWARE_CURSOR_MAX_WIDTH() - 
			active_cursor_p->cursor_width;
	}

	if (y < 0)
	{
		int x_offset;
		/*
		 ** Change offset hi and offset lo to point to the
		 ** topmost line of the visible cursor
		 */

		x_offset  = ((-y) * (M64_HARDWARE_CURSOR_MAX_WIDTH()) /
					 (M64_SCREEN_DEPTH()/M64_HARDWARE_CURSOR_DEPTH ));
		M64_CURSOR_CALCULATE_OFFSET(x_offset,
		   active_cursor_p->hardware_cursor_p->memory_representation.y,
		   active_cursor_p->hardware_cursor_p->cursor_offset);

		active_cursor_p->hardware_cursor_p->vert_cursor_offset =
		    M64_HARDWARE_CURSOR_MAX_HEIGHT() -
		    active_cursor_p->cursor_height + (-y);
		y=0;
	}
	else
	{
		M64_CURSOR_CALCULATE_OFFSET(active_cursor_p->\
			hardware_cursor_p->memory_representation.x,
			active_cursor_p->hardware_cursor_p->memory_representation.y,
			active_cursor_p->hardware_cursor_p->cursor_offset);

		active_cursor_p->hardware_cursor_p->vert_cursor_offset =  
		    M64_HARDWARE_CURSOR_MAX_HEIGHT() -
		    active_cursor_p->cursor_height;

	}

	if (m64_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		/*
		 * set the cursor offset.
		 */
		outl(M64_IO_REGISTER_CUR_OFFSET,
			((active_cursor_p->hardware_cursor_p->cursor_offset) >> 1));

		/*
		 * set the horizontal and vertical offset register.
		 */
		outl(M64_IO_REGISTER_CUR_HORZ_VERT_OFF,
			(active_cursor_p->hardware_cursor_p->horz_cursor_offset |
			(active_cursor_p->hardware_cursor_p->vert_cursor_offset) << 16));
		/*
		 * set the cursor position.
		 */
		outl(M64_IO_REGISTER_CUR_HORZ_VERT_POSN,
		((m64_cursor_state_p->current_cursor_status.x > 0 ?
			m64_cursor_state_p->current_cursor_status.x : 0) |
		((m64_cursor_state_p->current_cursor_status.y > 0 ?
			m64_cursor_state_p->current_cursor_status.y : 0) << 16)));
	}

	return SI_SUCCEED ;

}/*m64_cursor_hardware_move*/

/*
 * m64_cursor_make_named_allocation_string 
 */
function char *
m64_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct m64_options_structure * options_p)
{
	int total_pixels ;
	int lines ;
	int last_line ;
	char *string_p ;
	int hardware_cursor_max_width;
	int hardware_cursor_max_height;
	int number_of_cursors ;
	int start_y ;
	char tmp_buf[M64_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN+1] ;

	if(options_p->cursor_type != M64_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
	{
		return((char *)NULL);
	}

	if(options_p->cursor_max_size == NULL)
	{
		hardware_cursor_max_width = DEFAULT_M64_DOWNLOADABLE_CURSOR_WIDTH;

		hardware_cursor_max_height = DEFAULT_M64_DOWNLOADABLE_CURSOR_HEIGHT;
	}
	else if ((sscanf(options_p->cursor_max_size,"%ix%i",
		 &hardware_cursor_max_width, 
		 &hardware_cursor_max_height) != 2) ||
		 (hardware_cursor_max_width <= 0) ||
		 (hardware_cursor_max_height <= 0))
	{
		(void) fprintf(stderr, M64_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
					   options_p->cursor_max_size);
		
		hardware_cursor_max_width = DEFAULT_M64_DOWNLOADABLE_CURSOR_WIDTH;

		hardware_cursor_max_height = DEFAULT_M64_DOWNLOADABLE_CURSOR_HEIGHT;
	}


	number_of_cursors = (options_p->number_of_downloadable_cursors >  0) ?
			options_p->number_of_downloadable_cursors :
			DEFAULT_M64_NUMBER_OF_DOWNLOADABLE_CURSORS;

	total_pixels = (hardware_cursor_max_width*2/(M64_SCREEN_DEPTH()))* 
	    hardware_cursor_max_height;

	lines = total_pixels / M64_MAX_PHYSICAL_X_PIXELS () ;

	last_line = total_pixels % M64_MAX_PHYSICAL_X_PIXELS () ;


	if ( last_line > 0 )
	{
		++lines ;
	}

	/*
	 * The offscreen mem for the cursor will be allocated
	 * at the beginning of the offscreen area
	 */

	start_y = M64_MAX_VIRTUAL_Y_PIXELS() + 1;

	(void)sprintf(tmp_buf, M64_CURSOR_NAMED_ALLOCATION_STRING_TEMPLATE,
			M64_MAX_PHYSICAL_X_PIXELS(),
			number_of_cursors * lines,
			M64_SCREEN_DEPTH(), 0, start_y,0);

	string_p = allocate_memory(strlen(tmp_buf) + 1);

	(void) strcpy (string_p, tmp_buf);

#if defined(__DEBUG__)
	if (m64_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
"(m64_cursor_make_named_allocation_string){\n"
"\topt_string=%s\n"
"}\n",
					 string_p);
	}
#endif

	return string_p;
}

STATIC int
m64_cursor_initialize_software_cursors(int no_of_cursors)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	int i ;
	struct m64_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR_STATE, m64_cursor_state_p)) ;

#if defined ( __DEBUG__ )
	if (m64_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
			"(m64_cursor_initialize_software_cursors)"
			"{\n"
			"\tno_of_cursors = %d\n"
			"}\n",
		    no_of_cursors);
	}
#endif
	m64_cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct m64_cursor* ) * 
								  no_of_cursors); 

	/*
	 * Initialize the bitmap to store the screen area 
	 * obscured by the software cursor.
	 */
	m64_cursor_state_p->saved_screen_area_p =
		allocate_memory(sizeof(SIbitmap));

	m64_cursor_state_p->saved_screen_area_p->Bwidth =
		M64_SOFTWARE_CURSOR_MAX_WIDTH();
	m64_cursor_state_p->saved_screen_area_p->Bheight =
		M64_SOFTWARE_CURSOR_MAX_HEIGHT();
	m64_cursor_state_p->saved_screen_area_p->BbitsPerPixel =
		screen_state_p->generic_state.screen_depth;

	m64_cursor_state_p->saved_screen_area_p->Bptr =
		allocate_memory(((M64_SOFTWARE_CURSOR_MAX_WIDTH()*
		screen_state_p->generic_state.screen_depth) >> 3)*
		M64_SOFTWARE_CURSOR_MAX_HEIGHT());

	for ( i=0; i < no_of_cursors; ++i )
	{

		m64_cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct m64_cursor));
		STAMP_OBJECT(M64_CURSOR, m64_cursor_state_p->cursor_list_pp[i]);

		cursor_p = m64_cursor_state_p->cursor_list_pp[i];
		cursor_p->turnoff_function_p = m64_cursor_software_turnoff;
		cursor_p->turnon_function_p = m64_cursor_software_turnon;
		cursor_p->move_function_p = m64_cursor_software_move;

		cursor_p->software_cursor_p= allocate_and_clear_memory(
			sizeof(struct m64_cursor_software_cursor));

		cursor_p->software_cursor_p->source_p = 
			allocate_memory(sizeof(SIbitmap));
		cursor_p->software_cursor_p->inverse_source_p = 
			allocate_memory(sizeof(SIbitmap));
		cursor_p->software_cursor_p->mask_p = 
			allocate_memory(sizeof(SIbitmap));

		STAMP_OBJECT(M64_SOFTWARE_CURSOR, cursor_p->software_cursor_p);

	}
	
#if defined ( __DEBUG__ )
		if ( m64_cursor_debug )
		{
			(void) fprintf(debug_stream_p,
"(m64_cursor_initialize_software_cursors)"
"{\n"
"\tcursor_index = %d"
"}\n",
			    i);
		}
#endif

	return 1;
}

/*
 * m64_cursor_initialize_hardware_cursors
 */
STATIC int
m64_cursor_initialize_hardware_cursors(int no_of_cursors)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURSOR_CURSOR_STATE_DECLARE();
	int i ;
	int total_pixels ;
	int	full_lines ;
	int last_line ;
	struct omm_allocation *omm_block_p;
	struct m64_cursor *cursor_p ;

	ASSERT(IS_OBJECT_STAMPED(M64_CURSOR_STATE, m64_cursor_state_p));

	omm_block_p = omm_named_allocate(M64_CURSOR_NAMED_ALLOCATION_ID);
	if (omm_block_p  == NULL)
	{
		return 0;
	}

	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, omm_block_p));
	
	m64_cursor_state_p->omm_block_p = omm_block_p;

#if defined ( __DEBUG__ )
	if (m64_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
"(m64_cursor_initialize_hardware_cursors)"
"{\n"
"\tOM Block at ( %d, %d)\n"
"\tno_of_cursors = %d\n"
"}\n",
		    omm_block_p->x,
		    omm_block_p->y,
		    no_of_cursors);
	}
#endif
	total_pixels =  
		((M64_HARDWARE_CURSOR_MAX_WIDTH() * M64_HARDWARE_CURSOR_DEPTH) /
		 M64_SCREEN_DEPTH()) * M64_HARDWARE_CURSOR_MAX_HEIGHT();

	full_lines = total_pixels / M64_MAX_PHYSICAL_X_PIXELS ();

	last_line = total_pixels % M64_MAX_PHYSICAL_X_PIXELS ();

	if (last_line > 0)
	{
		++full_lines ;
	}

	m64_cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct m64_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		m64_cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct m64_cursor));
		STAMP_OBJECT(M64_CURSOR, m64_cursor_state_p->cursor_list_pp[i]);

		cursor_p = m64_cursor_state_p->cursor_list_pp[i];

		cursor_p->turnoff_function_p = m64_hardware_cursor_turnoff;

		cursor_p->turnon_function_p = m64_hardware_cursor_turnon;

		cursor_p->move_function_p = m64_hardware_cursor_move;

		cursor_p->hardware_cursor_p= 
		 allocate_and_clear_memory(sizeof(struct m64_cursor_hardware_cursor));

		STAMP_OBJECT(M64_HARDWARE_CURSOR, cursor_p->hardware_cursor_p);

		cursor_p->hardware_cursor_p->memory_representation.x = 
			omm_block_p->x ;

		cursor_p->hardware_cursor_p->memory_representation.y = 
			omm_block_p->y+ (full_lines*i);

#if defined ( __DEBUG__ )
		if ( m64_cursor_debug )
		{
			(void) fprintf(debug_stream_p,
"(m64_cursor_initialize_hardware_cursors)"
"{\n"
"\tcursor_index = %d"
"\tOffscreen loc ( %d, %d)"
"}\n",
			    i,
			    cursor_p->hardware_cursor_p->memory_representation.x,
			    cursor_p->hardware_cursor_p->memory_representation.y);
		}
#endif
	}

	return 1;
}

/*
 * m64_cursor__initialize__
 */
function void
m64_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct m64_options_structure * options_p)
{
	struct m64_cursor_state  *m64_cursor_state_p;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	M64_CURRENT_SCREEN_STATE_DECLARE();

#if defined(__DEBUG__)
	if ( m64_cursor_debug )
	{
		(void) fprintf(debug_stream_p,
"(m64__cursor__initialize__){\n"
"\tsi_screen_p= %p\n"
"\toptions_p= %p\n"
"\tmax_displayed_x = %d\n"
"\tmax_displayed_y = %d\n"
"\tmax_virtual_x = %d\n"
"\tmax_virtual_y = %d\n"
"\tmax_physical_x  = %d\n"
"\tmax_phyical_y   = %d\n"
"}\n",
					   (void*) si_screen_p,
					   (void*) options_p,
						M64_MAX_DISPLAYED_X_PIXELS(),
						M64_MAX_DISPLAYED_Y_PIXELS(),
						M64_MAX_VIRTUAL_X_PIXELS(),
						M64_MAX_VIRTUAL_Y_PIXELS(),
						M64_MAX_PHYSICAL_X_PIXELS(),
						M64_MAX_PHYSICAL_Y_PIXELS());
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));

	/*
	 *  Check if some other layer is planning to handle the cursor
	 */

	if(screen_state_p->cursor_state_p != NULL)
	{
		return;
	}

	/*
	 * Space for the state of the cursor module.
	 */
	m64_cursor_state_p =
	    allocate_and_clear_memory(sizeof(struct m64_cursor_state));

	STAMP_OBJECT(M64_CURSOR_STATE, m64_cursor_state_p);

	flags_p->SIcurscnt = (options_p->number_of_downloadable_cursors) ?
	    options_p->number_of_downloadable_cursors :
	    DEFAULT_M64_NUMBER_OF_DOWNLOADABLE_CURSORS;

	/*
	 * Get the cursor size that one is supposed to support.
	 */
	if (options_p->cursor_max_size == NULL)
	{
		m64_cursor_state_p->cursor_max_width =
			DEFAULT_M64_DOWNLOADABLE_CURSOR_WIDTH;

		m64_cursor_state_p->cursor_max_height =
			DEFAULT_M64_DOWNLOADABLE_CURSOR_HEIGHT;

#if defined (__DEBUG__)
			if (m64_cursor_debug)
			{
				(void)fprintf ( debug_stream_p,
"(m64_cursor__initialize__){\n"
"\tUsing default cursor dimensions\n"
"\n}\n"
					);
			}
#endif
	}
	else
	{
		if((sscanf(options_p->cursor_max_size,"%ix%i",
			&m64_cursor_state_p->cursor_max_width, 
			&m64_cursor_state_p->cursor_max_height) != 2) ||
		   (m64_cursor_state_p->cursor_max_width <= 0) ||
		   (m64_cursor_state_p->cursor_max_height <= 0))
		{
#if defined (__DEBUG__)
			if ( m64_cursor_debug )
			{
				(void)fprintf ( debug_stream_p,
"(m64_cursor__initialize__){\n"
"\tUsing default cursor dimensions\n"
"\n}\n"
					);
			}
#endif
			/*
			(void) fprintf(stderr, M64_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
						   options_p->cursor_max_size);
			*/

			m64_cursor_state_p->cursor_max_width =
				DEFAULT_M64_DOWNLOADABLE_CURSOR_WIDTH;

			m64_cursor_state_p->cursor_max_height =
				DEFAULT_M64_DOWNLOADABLE_CURSOR_HEIGHT;
		}
#if defined (__DEBUG__)
		else
		{
			if ( m64_cursor_debug )
			{
				(void) fprintf(debug_stream_p,
"(m64_cursor__initialize__){\n"
"\tMax cursor width  = %d"
"\tMax cursor height = %d"
"\n}\n",
							   m64_cursor_state_p->cursor_max_width,
							   m64_cursor_state_p->cursor_max_height
							   );
			}
		}
#endif
	}

	/*
	 * Set SI's idea of the best cursor width.
	 */
	flags_p->SIcurswidth =
		m64_cursor_state_p->cursor_max_width;
	flags_p->SIcursheight =
		m64_cursor_state_p->cursor_max_height;

	/*
	 * The cursor mask is valid for software cursors only.
	 */
	flags_p->SIcursmask = DEFAULT_M64_DOWNLOADABLE_CURSOR_MASK;

	/*
	 * Default handlers.
	 */
	functions_p->si_hcurs_download = 
		(SIBool (*)(SIint32, SICursorP)) m64_no_operation_succeed;
	functions_p->si_hcurs_turnon = 
		(SIBool (*)(SIint32)) m64_no_operation_succeed;
	functions_p->si_hcurs_turnoff = 
		(SIBool (*)(SIint32)) m64_no_operation_succeed;
	functions_p->si_hcurs_move = 
		(SIBool (*)(SIint32, SIint32, SIint32)) m64_no_operation_succeed;

	/* 
	 * The M64 supports hardware cursors. 
	 */
	if (options_p->cursor_type == M64_OPTIONS_CURSOR_TYPE_SOFTWARE_CURSOR ||
	    (strcmp(screen_state_p->dac_state_p->dac_name, "M64_DAC_STG1702") == 0
		&& screen_state_p->generic_state.screen_depth == 32))
	{
		flags_p->SIcursortype = CURSOR_FAKEHDWR;
	}
	else	
	{
		flags_p->SIcursortype = CURSOR_TRUEHDWR;
	}
	
	if(flags_p->SIcursortype == CURSOR_TRUEHDWR)
	{
#if defined(__DEBUG__)
		if( m64_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
"(m64_cursor__initialize__){\n"
"\t#Switching to hardware cursor\n"
"}\n"
							);
		}
#endif

		/*
		 * Set up SI's parameters.
		 */

		flags_p->SIcursortype = CURSOR_TRUEHDWR;
		m64_cursor_state_p->cursor_type = M64_CURSOR_HARDWARE;

		/*
		 * Cursor manipulation functions.
		 */

		functions_p->si_hcurs_download = m64_hardware_cursor_download;
		functions_p->si_hcurs_turnon = m64_hardware_cursor_turnon;
		functions_p->si_hcurs_turnoff = m64_hardware_cursor_turnoff;
		functions_p->si_hcurs_move = m64_hardware_cursor_move;

		m64_cursor_state_p->number_of_cursors = flags_p->SIcurscnt;

		m64_cursor_state_p->current_cursor_status.x = -1;
		m64_cursor_state_p->current_cursor_status.y = -1;
		m64_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
		m64_cursor_state_p->current_cursor_status.current_index = -1;

		/*
		 * Used for pixel panning
		 */
		m64_cursor_state_p->visible_window_top_x = 0;
		m64_cursor_state_p->visible_window_top_y = 0;


		/*
		 * Assign the cursor state.
		 */
		(screen_state_p)->cursor_state_p = m64_cursor_state_p ;

		/*
		 * Allocate off-screen memory etc.
		 */

		if (m64_cursor_initialize_hardware_cursors(flags_p->SIcurscnt) == 0)
		{
			/*
			 * Allocation failed : Undo all work done 
			 */

#if (defined(__DEBUG__))
			if (m64_cursor_debug)
			{
				(void) fprintf(debug_stream_p,
"(m64_cursor__initialize__)\n"
"{\n"
"\t# Initialization of hardware cursors failed.\n"
"}\n");
			
			}
#endif

			flags_p->SIcursortype = CURSOR_FAKEHDWR;
			functions_p->si_hcurs_download = 
				(SIBool (*)(SIint32, SICursorP)) m64_no_operation_succeed;
			functions_p->si_hcurs_turnon = 
				(SIBool (*)(SIint32)) m64_no_operation_succeed;
			functions_p->si_hcurs_turnoff = 
				(SIBool (*)(SIint32)) m64_no_operation_succeed;
			functions_p->si_hcurs_move = 
				(SIBool (*)(SIint32, SIint32, SIint32))
					 m64_no_operation_succeed;
			return ;
		}
	}
	else  /* software cursor */
	{
#if defined(__DEBUG__)
		if( m64_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
"(m64_cursor__initialize__){\n"
"\t#Switching to software cursor\n"
"}\n"
							);
		}
#endif

		m64_cursor_state_p->cursor_type = M64_CURSOR_SOFTWARE;

		/*
		 * For R5 and above we don't get called for software cursor

		if(screen_state_p->generic_state.screen_server_version_number >=
			X_SI_VERSION1_1)  
		{
			functions_p->si_hcurs_download = 
				(SIBool (*)(SIint32, SICursorP))
				 m64_no_operation_succeed;
			return;
		}
		 */

		/*
		 * Cursor manipulation functions.
		 */

		functions_p->si_hcurs_download = m64_cursor_software_download;
		functions_p->si_hcurs_turnon = m64_cursor_software_turnon;
		functions_p->si_hcurs_turnoff = m64_cursor_software_turnoff;
		functions_p->si_hcurs_move = m64_cursor_software_move;

		m64_cursor_state_p->number_of_cursors = flags_p->SIcurscnt;

		m64_cursor_state_p->current_cursor_status.x = -1;
		m64_cursor_state_p->current_cursor_status.y = -1;
		m64_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
		m64_cursor_state_p->current_cursor_status.current_index = -1;

		/*
		 * Used for pixel panning
		 */
		m64_cursor_state_p->visible_window_top_x = 0;
		m64_cursor_state_p->visible_window_top_y = 0;


		/*
		 * Assign the cursor state.
		 */
		(screen_state_p)->cursor_state_p = m64_cursor_state_p ;

		m64_cursor_initialize_software_cursors(flags_p->SIcurscnt);

		
		return;
	}
}
