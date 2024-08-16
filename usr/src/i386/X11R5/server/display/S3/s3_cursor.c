/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_cursor.c	1.11"

/***
 ***	NAME
 ***
 ***		s3_cursor.c : Cursor management module
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s3_cursor.h"
 ***
 ***
 ***			SIBool
 ***			s3_cursor_download ( SIint32 cursor_index,
 ***									SICursorP cursor_p )
 ***			SIBool
 ***			s3_cursor_turnon ( SIint32 cursor_index )
 ***			SIBool
 ***			s3_cursor_turnoff ( SIint32 cursor_index )
 ***			SIBool
 ***			s3_cursor_move ( SIint32 cursor_index, SIint32 x, SIint32 y )
 ***
 ***
 ***
 ***
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***		s3_cursor.c : Source.
 ***		s3_cursor.h : Interface.
 ***
 ***	SEE ALSO
 ***
 ***  SI definitions document
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
#include "s3_options.h"
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



PUBLIC

/***
 *** 	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include <string.h>


/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

enum s3_cursor_status_kind
{
	CURSOR_ON,
	CURSOR_OFF
};


struct s3_cursor_status
{
	enum s3_cursor_status_kind flag ;	/*is cursor on/off*/
	int	current_index ;		/*index of the current cursor (cursor_index)*/
	int	x;							/*X co-ord of the cursor*/
	int	y;							/*Y co-ord of the cursor*/
#if defined ( __DEBUG__ )
	int stamp ;
#endif
};

/*
 * hardware cursor specfic fields
 */
struct s3_cursor_hardware_cursor
{
	/*
	 * Vertical and horizontal offsets in to the 64x64 cursor bitmap
	 * for this cursor
	 */
	int horz_cursor_offset;
	int vert_cursor_offset;

	int	offscreen_x_location;
	int offscreen_y_location;

#if defined(__DEBUG__)
	int stamp ;
#endif
};

/*
 *software cursor structure
 */
struct s3_cursor_software_cursor
{
	struct omm_allocation *src_bitmap_p;
	struct omm_allocation *invsrc_bitmap_p;
	struct omm_allocation *save_area_p;
	int saved_width;
	int saved_height;
#if defined(__DEBUG__)
	int	valid_data;
	int stamp ;
#endif
};
struct s3_cursor
{
	/*
	 * Cursor attributes (hardware/software)
	 */
	int	cursor_width ;				
	int cursor_height; 
	int	foreground_color ;
	int	background_color ;

	/* Pointers to various cursor operation functions for this cursor*/
	SIBool (*move_function_p)    ( SIint32, SIint32, SIint32 ) ;
	SIBool (*turnon_function_p)  ( SIint32 ) ;
	SIBool (*turnoff_function_p) ( SIint32 ) ;

	struct s3_cursor_hardware_cursor
		*hardware_cursor_p;

	struct s3_cursor_software_cursor
		*software_cursor_p;
#if defined(__DEBUG__)
	int stamp ;
#endif

};



/*
** Module state
*/
struct s3_cursor_state
{
	
	/* type of the cursor S3_CURSOR_HARDWARE or S3_CURSOR_SOFTWARE*/
	int cursor_type;

	/*Max width of the hardware cursor */
	int hardware_cursor_max_width;	

	/*Max height */
	int hardware_cursor_max_height; 

	/*No of downloadable cursors*/
	int number_of_cursors ;			

	/*Status of the current cursor */
	struct s3_cursor_status current_cursor_status; 

	/*
	*The co-ords of the visible window (displayed area) with respect to
	*the virtual screen
	*/
	int	 visible_window_top_x;
	int	 visible_window_top_y;

	/* Pointer to cursors list */
	struct s3_cursor **cursor_list_pp;


#if defined ( __DEBUG__)
	int stamp ;
#endif

};

#if (defined(__DEBUG__))
export boolean s3_cursor_debug;
#endif

PRIVATE

/*
 * Includes
 */

#include "s3_asm.h"
#include "g_omm.h"
#include "s3_regs.h"
#include "s3_globals.h"
#include "s3_state.h"
#include "s3_options.h"

/*
 * We support two kinds of cursors
 */
#define	S3_CURSOR_HARDWARE	0
#define	S3_CURSOR_SOFTWARE	1


/*
 * status of the save area
 */
#define S3_CURSOR_VALID_DATA	1
#define S3_CURSOR_INVALID_DATA	0

#define	S3_SOFTWARE_CURSOR_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('S' << 3) + ('O' << 4) +\
	 ('F' << 5) + ('T' << 6) + ('W' << 7) + ('A' << 8) + ('R' << 9) +\
	 ('E' << 10) + ('_' << 11) + ('C' << 12) + ('U' << 13) + ('R' << 14) +\
	 ('S' << 15) + ('O' << 16) + ('R' << 17) + 0 )

#define	S3_HARDWARE_CURSOR_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('H' << 3) + ('A' << 4) +\
	 ('R' << 5) + ('D' << 6) + ('W' << 7) + ('A' << 8) + ('R' << 9) +\
	 ('E' << 10) + ('_' << 11) + ('C' << 12) + ('U' << 13) + ('R' << 14) +\
	 ('S' << 15) + ('O' << 16) + ('R' << 17) + ('_' << 18) + ('S' << 19) +\
	 ('T' << 20) + ('A' << 21) + ('M' << 22) + ('P' << 23) + 0 )

#define	S3_CURSOR_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('C' << 3) + ('U' << 4) +\
	 ('R' << 5) + ('S' << 6) + ('O' << 7) + ('R' << 8) + ('_' << 9) +\
	 ('S' << 10) + ('T' << 11) + ('A' << 12) + ('T' << 13) + ('E' << 14) +\
	 0 )
#define	S3_CURSOR_STAMP\
	 (('S' << 0) + ('3' << 1) + ('_' << 2) + ('C' << 3) + ('U' << 4) +\
	 ('R' << 5) + ('S' << 6) + ('O' << 7) + ('R' << 8) + ('_' << 9) +\
	 ('S' << 10) + ('T' << 11) + ('A' << 12) + ('M' << 13) + ('P' << 14) + 0 )

#define	S3_CURSOR_NAMED_ALLOCATION_ID	"CURSOR"

#define S3_CURSOR_NAMED_ALLOCATION_STRING_FORMAT\
		S3_CURSOR_NAMED_ALLOCATION_ID ":%d+%d+%d@%d+%d+%d"

#define S3_CURSOR_NAMED_ALLOCATION_STRING_TEMPLATE\
	"named-allocate(" S3_CURSOR_NAMED_ALLOCATION_ID\
	":%4.4dx%4.4dx%4.4d+%4.4d+%4.4d" ")"

#define	S3_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN	200

/*
** Each pixel of the hardware cursor is represented by two bits
*/
#define S3_HARDWARE_CURSOR_DEPTH	2

/***
 ***	Macros.
 ***/
#define S3_HARDWARE_CURSOR_MAX_WIDTH()\
	(s3_cursor_state_p->hardware_cursor_max_width)

#define S3_HARDWARE_CURSOR_MAX_HEIGHT()\
	(s3_cursor_state_p->hardware_cursor_max_height) 

#define S3_MAX_DISPLAYED_X_PIXELS() \
	(generic_current_screen_state_p->screen_displayed_width) 

#define S3_MAX_DISPLAYED_Y_PIXELS()\
	(generic_current_screen_state_p->screen_displayed_height) 

#define S3_MAX_VIRTUAL_X_PIXELS() \
	(generic_current_screen_state_p->screen_virtual_width) 

#define S3_MAX_VIRTUAL_Y_PIXELS()\
	(generic_current_screen_state_p->screen_virtual_height) 

#define S3_MAX_PHYSICAL_X_PIXELS() \
	(generic_current_screen_state_p->screen_physical_width) 

#define S3_MAX_PHYSICAL_Y_PIXELS()\
	(generic_current_screen_state_p->screen_physical_height) 

#define S3_SCREEN_DEPTH()\
	(generic_current_screen_state_p->screen_depth) 

#define	S3_CHIPSET_KIND()\
	(((struct s3_screen_state*)\
	  generic_current_screen_state_p)->\
	 chipset_kind) 

#define	S3_CURSOR_STATE_DECLARE()\
	struct s3_cursor_state  *s3_cursor_state_p=\
	(screen_state_p)->cursor_state_p


#define	S3_CURSOR_IS_VALID_INDEX(cursor_index)\
	(cursor_index >= 0) &&\
	(cursor_index<s3_cursor_state_p->number_of_cursors)


#define S3_CURSOR_ALL_PLANES_ENABLED\
	((1 << S3_SCREEN_DEPTH()) - 1) 

/*
 * Function for nullpadding the undefined bits in the cursor source
 * and inverse source bitmaps.
 */
STATIC void
s3_cursor_clear_extra_bits_and_copy(SICursorP si_cursor_p,
	SIbitmapP si_src_p, SIbitmapP si_invsrc_p,
	SIbitmapP si_mask_p)
{
	int total_bytes;
	char *src_p_1;
	char *mask_p_1;
	char *invsrc_p_1;
	char *src_p_2;
	char *mask_p_2;
	char *invsrc_p_2;
	
	ASSERT(si_cursor_p->SCsrc->BbitsPerPixel == 1);
	/*
	 * We know that the cursor bitmaps are always 1 bit deep
	 */
	total_bytes = (( si_cursor_p->SCsrc->Bwidth + 31 ) & ~31 ) >> 3;
	total_bytes  *= si_cursor_p->SCsrc->Bheight; 
	ASSERT(total_bytes > 0);

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
	for(;total_bytes > 0; --total_bytes)
	{
		*src_p_2 = *src_p_1 & *mask_p_1;
		*invsrc_p_2 = *invsrc_p_1 & *mask_p_1;
		*mask_p_2 = (*invsrc_p_2 | *src_p_2);

		++src_p_1;
		++mask_p_1;
		++invsrc_p_1;
		++src_p_2;
		++mask_p_2;
		++invsrc_p_2;
	}


}

/*
 * s3_cursor_copy_area
 * 
 * Used in saving the contents of saved areas from the displayed
 * screen and conversely for restoring the saved screen portions from
 * offscreen area to the displayed screen.
 */

STATIC void
s3_cursor_copy_area(int source_x, int source_y,
	int destination_x, int destination_y,
	int width, int height)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	/*
	 * Set clip rectangle
	 */
	S3_STATE_SET_CLIP_RECTANGLE(screen_state_p,0,0,
					S3_MAX_PHYSICAL_X_PIXELS(),
					S3_MAX_PHYSICAL_Y_PIXELS());
	screen_state_p->generic_state.screen_current_clip =
	    GENERIC_CLIP_NULL;
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);


	/*
	 *set write and read masks
	 */
	S3_STATE_SET_RD_MASK(screen_state_p,S3_CURSOR_ALL_PLANES_ENABLED);
	S3_STATE_SET_WRT_MASK(screen_state_p,S3_CURSOR_ALL_PLANES_ENABLED);


	S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
	S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_N);
#if (defined(__DEBUG__))
	if (s3_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_cursor_copy_area){ \n"
			"\tsource_x = %d\n"
			"\tsource_y = %d\n"
			"\tdestination_x = %d\n"
			"\tdestination_y = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"}\n",
			source_x, source_y,
			destination_x,
			destination_y,
			width, height);
	}
#endif
	
	
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_VIDEO_DATA);
	s3_asm_move_screen_bits(source_x, source_y, destination_x,
		destination_y, width, height,S3_ASM_TRANSFER_THRO_PLANE);

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	
}


/*
 * s3_cursor_software_cursor_download_helper
 */
STATIC int
s3_cursor_software_cursor_download_helper
	(SIbitmap *bitmap_p, int destination_x,
	int destination_y, int width,
	int height, unsigned short plane_mask)
{
	unsigned char *source_bits_p;
	unsigned char *inverted_bytes_p;
	int	transfers_per_line;
	int	source_step;
	S3_CURRENT_SCREEN_STATE_DECLARE();
	const unsigned char *tmp_byte_invert_table_p  = 
		screen_state_p->byte_invert_table_p;


#if defined(__DEBUG__)
	if( s3_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
			"(s3_cursor_software_cursor_download_helper){\n"
			"\tbitmap_p = %p\n"
			"\tdest_x = %d\n"
			"\tdest_y = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tplane_mask = %d\n"
			"}\n",
			(void*)bitmap_p,
			destination_x,
			destination_y,
			width,
			height,
			plane_mask);
	}					
#endif




	/*
	 * Setup rop for GXcopy
	 */
	S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
	S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_N);

	/*
	 * Make sure that we will write to only one plane
	 */
	ASSERT(!(plane_mask & (plane_mask - 1)));

	S3_STATE_SET_WRT_MASK(screen_state_p, plane_mask);

	S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, (short)destination_x,
		(short)destination_y,
	    destination_x + width,
	    destination_y + height);

	screen_state_p->generic_state.screen_current_clip =
	    GENERIC_CLIP_NULL;
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);

	/*
	 * The foreground color is set to all 1's; the write mask ensures
     * that only one plane of offscreen memory gets updated.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_CPU_DATA);
	S3_STATE_SET_FRGD_COLOR(screen_state_p, S3_CURSOR_ALL_PLANES_ENABLED);
	S3_STATE_SET_BKGD_COLOR(screen_state_p, 0);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

	/*
	 * Calculate no of pixtrans words to be transferred per line
	 */
	width = (width + screen_state_p->pixtrans_width - 1) &
		~(screen_state_p->pixtrans_width - 1);
	transfers_per_line = width >>
		screen_state_p->pixtrans_width_shift;

	source_step = ((bitmap_p->Bwidth + 31) & ~31)  >> 3;
	source_bits_p = (unsigned char*)bitmap_p->Bptr;

#if defined ( __DEBUG__ )
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s3_cursor_software_cursor_download_helper)\n{\n"
			"\tdata_p = %p\n"
			"\ttransfers_per_line = %d\n"
			"source_step = %d\n"
			"\n}\n",
			(void*)source_bits_p,
			transfers_per_line,
			source_step);
	}
#endif

	inverted_bytes_p =  
		screen_state_p->generic_state.screen_scanline_p;

	S3_WAIT_FOR_FIFO(5);
	

	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS));
	
	
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		(screen_state_p->cmd_flags) | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_DRAW |
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);

	S3_WAIT_FOR_FIFO(8);
	
	/*
	 * the following piece of code has been lifted from
	 * s3_bitblt.c:s3_memory_to_screen_stplblt
	 */ 
	do
	{
		register int i;
		register unsigned char *tmp_inverted_bytes_p;
		register unsigned char *tmp_source_p;
	
		tmp_inverted_bytes_p = inverted_bytes_p;
		tmp_source_p = source_bits_p;

		/*
	 	 * Invert all bytes in each line before pumping
		 */
		for (i=0; i < source_step; ++i)
		{
			*tmp_inverted_bytes_p++ = *(tmp_byte_invert_table_p +
				*tmp_source_p++);
		}
		(*screen_state_p->screen_write_bits_p)
			(screen_state_p->pixtrans_register,
			 transfers_per_line, 
			 (void *) inverted_bytes_p);
		source_bits_p += source_step;
	}while(--height > 0);
	
	
	return 1;
}


/*
 * s3_cursor_software_cursor_draw_helper
 *
 */

STATIC void
s3_cursor_software_cursor_draw_helper
	(int source_x, int source_y,
	int destination_x, int destination_y,
	int width, int height, int foreground_color,
	int background_color, unsigned short read_mask)
{
	unsigned short write_mask;
	int clip_ul_x;
	int clip_ul_y;
	S3_CURRENT_SCREEN_STATE_DECLARE();

#if defined(__DEBUG__)
	if( s3_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
				"(s3_cursor_software_cursor_draw_helper){\n"
				"\tsrc_x = %d\n"
				"\tsrc_y = %d\n"
				"\tdest_x = %d\n"
				"\tdest_y = %d\n"
				"\twidth = %d\n"
				"\theight = %d\n"
				"\tforeground_color = %d\n"
				"\tbackground_color = %d\n"
				"\tread_mask = %d\n"
				"}\n",
				source_x,source_y,
				destination_x,destination_y,
				width, height,
				foreground_color,
				background_color,
				read_mask);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	/*
	 * Check if clipping rectangle co-ordinates have to be changed.
	 * (This is done inorder to avoid drawing outside the visible area.)
	 */
	if (destination_x < 0)
	{
		clip_ul_x = 0;
	}
	else
	{
		clip_ul_x = destination_x;
	}

	if (destination_y < 0)
	{
		clip_ul_y = 0;
	}
	else
	{
		clip_ul_y = destination_y;
	}
	
	if (destination_x + width > S3_MAX_VIRTUAL_X_PIXELS() + 1)
	{
		width -= destination_x + width - S3_MAX_VIRTUAL_X_PIXELS();
	}

	if (destination_y + height > S3_MAX_VIRTUAL_Y_PIXELS() + 1)
	{
		height -= destination_y + height - S3_MAX_VIRTUAL_Y_PIXELS();
	}
	/*
	 * Set the clip rectangle
	 */
	S3_STATE_SET_CLIP_RECTANGLE(screen_state_p,
		(short)clip_ul_x,(short)clip_ul_y,
		destination_x + width,
		destination_y + height);
	screen_state_p->generic_state.screen_current_clip =
	    GENERIC_CLIP_NULL;
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);



#if (defined(__DEBUG__))
	if (s3_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
					"(s3_cursor_software_cursor_draw_helper){ \n"
					"\tsource_x = %d\n"
					"\tsource_y = %d\n"
					"\tdestination_x = %d\n"
					"\tdestination_y = %d\n"
					"\twidth = %d\n"
					"\theight = %d\n"
					"}\n",
					source_x, source_y, destination_x,
				    destination_y, width, height);
	}
#endif
	
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_VID_MEM);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);
	S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
	S3_STATE_SET_FRGD_COLOR(screen_state_p, foreground_color);
	/*
	 *set plane masks
	 */
	write_mask = S3_CURSOR_ALL_PLANES_ENABLED;
	
	
	S3_STATE_SET_RD_MASK(screen_state_p,read_mask); 
	S3_STATE_SET_WRT_MASK(screen_state_p,write_mask); 

	s3_asm_move_screen_bits(source_x, source_y, destination_x,
	  destination_y, width, height,
	  S3_ASM_TRANSFER_ACROSS_PLANE);
	
}


/*
 *s3_cursor_create_software_cursor
 */

STATIC int 
s3_cursor_create_software_cursor(struct s3_cursor *active_cursor_p,
	SICursor *cursor_p )
{
	int height;
	int	width;
	int dx;
	int dy;
	int total_bytes;
	unsigned short plane_mask;
	SIbitmap src_bitmap;
	SIbitmap invsrc_bitmap;
	SIbitmap mask_bitmap;

#if defined ( __DEBUG__ )
	if ( s3_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
				"(s3_cursor_create_software_cursor){\n"
				"\tactive_cursor_p= %p\n"
				"cursor_p= %p\n"
				"}\n",
				(void*)active_cursor_p,
				(void*)cursor_p);
	}
#endif

	ASSERT((active_cursor_p != NULL)&&( cursor_p != NULL));
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,active_cursor_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SOFTWARE_CURSOR,
		active_cursor_p->software_cursor_p));

	height = cursor_p->SCheight;
	width = cursor_p->SCwidth;
	active_cursor_p->cursor_width = width;
	active_cursor_p->cursor_height = height;

	active_cursor_p->foreground_color =
		cursor_p->SCfg;
	active_cursor_p->background_color =
		cursor_p->SCbg;

	/*
	 * Filter all extra (set) bits from source and inverse source bitmaps
	 * by applying mask to it.
	 */
	total_bytes = (( cursor_p->SCsrc->Bwidth + 31 ) & ~31 ) >> 3;
	total_bytes  *= cursor_p->SCsrc->Bheight; 
	ASSERT(total_bytes > 0 );

	/*
	 * Make a local copy of the bitmaps as we need to pass bitmap
	 * structures down to some other functions, and the SCsrc and
	 * SCinvscr bitmaps are not supposed to be modified by us.
	 */
	(void) memcpy(&src_bitmap,cursor_p->SCsrc, sizeof(SIbitmap));
	(void) memcpy(&invsrc_bitmap,cursor_p->SCinvsrc, sizeof(SIbitmap));
	(void) memcpy(&mask_bitmap,cursor_p->SCmask, sizeof(SIbitmap));

	/*
	 * Allocate temporary space for bitmaps	
	 */
	src_bitmap.Bptr = allocate_memory(total_bytes);
	invsrc_bitmap.Bptr = allocate_memory(total_bytes);
	mask_bitmap.Bptr = allocate_memory(total_bytes);
	
	/*
	 * NOTE : mask_bitmap is not being used in this function
	 */
	s3_cursor_clear_extra_bits_and_copy(cursor_p,&src_bitmap,&invsrc_bitmap,
		&mask_bitmap);

	if(active_cursor_p->software_cursor_p->src_bitmap_p != NULL)
	{
		(void) omm_free(active_cursor_p->
						software_cursor_p->src_bitmap_p);
	}
	
	active_cursor_p->software_cursor_p->src_bitmap_p =
	omm_allocate(width, height, cursor_p->SCsrc->BbitsPerPixel,
		 OMM_LONG_TERM_ALLOCATION);

	
	if( active_cursor_p->software_cursor_p->src_bitmap_p == NULL)
	{
#if defined(__DEBUG__)
		if(s3_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
				"(s3_cursor_create_software_cursor){\n"
				"\t#OMM failed!\n"
				"\t#Request:\n"
				"\t width = %d\n"
				"\t height = %d\n"
				"}\n",
				width,
				height);
		}
#endif
		return 0;
	}

	if(OMM_LOCK(active_cursor_p->software_cursor_p->src_bitmap_p))
	{ 
		dx = 	active_cursor_p->software_cursor_p->src_bitmap_p->x; 
		dy = 	active_cursor_p->software_cursor_p->src_bitmap_p->y; 
		plane_mask = active_cursor_p->software_cursor_p->
			src_bitmap_p->planemask;

		if (s3_cursor_software_cursor_download_helper
				(&src_bitmap,
				dx, dy,width,height,plane_mask) == 0)
		{
			OMM_UNLOCK(active_cursor_p->software_cursor_p->src_bitmap_p);

			(void) omm_free(active_cursor_p->software_cursor_p->src_bitmap_p);
			active_cursor_p->software_cursor_p->src_bitmap_p = NULL;
			free_memory(src_bitmap.Bptr);
			return 0;
		}
		OMM_UNLOCK(active_cursor_p->software_cursor_p->src_bitmap_p);
		free_memory(src_bitmap.Bptr);
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}

	if(active_cursor_p->software_cursor_p->invsrc_bitmap_p != NULL)
	{
		(void) omm_free(active_cursor_p->software_cursor_p->invsrc_bitmap_p); 
	}

	active_cursor_p->software_cursor_p->invsrc_bitmap_p =
		omm_allocate(width, height, cursor_p->SCinvsrc->BbitsPerPixel,
					 OMM_LONG_TERM_ALLOCATION);
	if( active_cursor_p->software_cursor_p->invsrc_bitmap_p == NULL)
	{

#if defined(__DEBUG__)
		if(s3_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
				"(s3_cursor_create_software_cursor){\n"
				"\t#OMM failed!\n"
				"\t#Request:\n"
				"\t width = %d\n"
				"\t height = %d\n"
				"}\n",
				width,
				height);
		}
#endif
		return 0;
	}
	/*
	 * fetch transfer parameters
	 */
	if (OMM_LOCK(active_cursor_p->software_cursor_p->invsrc_bitmap_p)) 
	{ 
		dx = active_cursor_p->software_cursor_p->invsrc_bitmap_p->x; 
		dy = active_cursor_p->software_cursor_p->invsrc_bitmap_p->y; 
		plane_mask = active_cursor_p->software_cursor_p->
			invsrc_bitmap_p->planemask;

		if (s3_cursor_software_cursor_download_helper
				(&invsrc_bitmap,
				dx, dy,width,height,plane_mask) == 0)
		{
			OMM_UNLOCK(active_cursor_p->software_cursor_p->invsrc_bitmap_p);
			(void) omm_free(active_cursor_p->software_cursor_p->
							invsrc_bitmap_p);
			active_cursor_p->software_cursor_p->invsrc_bitmap_p =
				NULL;
			free_memory(invsrc_bitmap.Bptr);
			free_memory(mask_bitmap.Bptr);
			return 0;
		}
		OMM_UNLOCK(active_cursor_p->software_cursor_p->invsrc_bitmap_p);
		free_memory(invsrc_bitmap.Bptr);
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}

	/*
	 * Allocate offscreen memory for saving obscured portion of the screen
	 */

	/*
	 * free previously allocated area
	 */
	if( active_cursor_p->software_cursor_p->save_area_p != NULL)
	{
		(void) omm_free(active_cursor_p->software_cursor_p->save_area_p);
	}

	active_cursor_p->software_cursor_p->save_area_p =
		omm_allocate(width, height,
		 S3_SCREEN_DEPTH(), OMM_LONG_TERM_ALLOCATION);
	if(	active_cursor_p->software_cursor_p->save_area_p == NULL)
	{
		(void) omm_free(active_cursor_p->
						software_cursor_p->src_bitmap_p);
		active_cursor_p->software_cursor_p->src_bitmap_p = NULL;		
		(void) omm_free(active_cursor_p->
						software_cursor_p->invsrc_bitmap_p); 
		active_cursor_p->software_cursor_p->invsrc_bitmap_p = NULL;
		return 0;
	}
	/*
	 * invalidate the save area
	 */
#if defined(__DEBUG__)
	active_cursor_p->software_cursor_p->valid_data =
		 S3_CURSOR_INVALID_DATA;
#endif

	return 1;

}

/*
 *s3_cursor_save_obscured_screen 
 */

STATIC int
s3_cursor_save_obscured_screen(int cursor_index)  
{
	int dx;
	int dy;
	int height;
	int width;
	int x1;
	int y1;
	int x2;
	int y2;
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	struct s3_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
					  "(s3_cursor_save_obscured_screen){"
					  "\n"
					  "\tcursor_index= %d\n}\n",
					  cursor_index);
	}
#endif
	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,active_cursor_p));

	/*
	 * determine the co-ordinates of the rectangular area that
	 * has to be saved: (x1,y1,x2,y2)
	 */
	x1 = s3_cursor_state_p->current_cursor_status.x;
	y1 = s3_cursor_state_p->current_cursor_status.y;
	x2 = s3_cursor_state_p->current_cursor_status.x +
		active_cursor_p->cursor_width;
	y2 = s3_cursor_state_p->current_cursor_status.y +
		active_cursor_p->cursor_height;

	/*
	 * Check if it is outside the visible area and adjust co-ords
	 * if required
	 */
	 /*
	  *NOTE: x2 and y2 are exclusive
	  */
	 x2 = (x2 > S3_MAX_VIRTUAL_X_PIXELS() + 1) ?
			 (S3_MAX_VIRTUAL_X_PIXELS() + 1) : x2;
	 y2 = (y2 > S3_MAX_VIRTUAL_Y_PIXELS() + 1) ?
			 (S3_MAX_VIRTUAL_Y_PIXELS() + 1) : y2;

	/*
	 * Calculate width and height of the rectangular save area
	 */
	width = x2 - x1;
	height = y2 - y1;
	active_cursor_p->software_cursor_p->saved_width = width;
	active_cursor_p->software_cursor_p->saved_height = height;

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s3_cursor_save_obscured_screen){\n"
			"\t#Co-ords of the rectangular area to be saved\n"
			"\tul.x = %d\n"
			"\tul.y = %d\n"
			"\tlr.x = %d\n"
			"\tlr.y = %d\n"
			"\twidth = %d\n"
			"\thgt = %d\n}\n",
			x1,y1,x2,
			y2,width,height);
	}
#endif
	
	if(active_cursor_p->software_cursor_p->save_area_p  == NULL)
	{
		return 0;
	}

	if (OMM_LOCK(active_cursor_p->software_cursor_p->save_area_p))  
	{
		dx = active_cursor_p->software_cursor_p->save_area_p->x;  
		dy = active_cursor_p->software_cursor_p->save_area_p->y;  
		s3_cursor_copy_area(x1, y1, dx,dy,width,height);
		OMM_UNLOCK(active_cursor_p->software_cursor_p->save_area_p);

#if defined(__DEBUG__)
		active_cursor_p->software_cursor_p->valid_data =
			 S3_CURSOR_VALID_DATA;
#endif
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
 *s3_cursor_restore_obscured_screen 
 */
STATIC int
s3_cursor_restore_obscured_screen ( int cursor_index )
{
	int sx;
	int sy;
	int	height;
	int width;
	int	x;
	int	y;
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	struct s3_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s3_cursor_restore_obscured_screen){\n"
			"\tcursor_index= %d\n"
			"}\n",
			cursor_index);
	 }
#endif

	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,active_cursor_p));
	ASSERT(active_cursor_p->software_cursor_p->valid_data ==
		 S3_CURSOR_VALID_DATA);

	height = active_cursor_p->software_cursor_p->saved_height; 
	width = active_cursor_p->software_cursor_p->saved_width; 
	if (OMM_LOCK(active_cursor_p->software_cursor_p->save_area_p))
	{
		sx = active_cursor_p->software_cursor_p->save_area_p->x;
		sy = active_cursor_p->software_cursor_p->save_area_p->y;
		x = s3_cursor_state_p->current_cursor_status.x;
		y = s3_cursor_state_p->current_cursor_status.y;
		s3_cursor_copy_area(sx, sy, x, y, width,height);
#if (defined(__DEBUG__))
		active_cursor_p->software_cursor_p->valid_data =
			 S3_CURSOR_INVALID_DATA;
#endif
		OMM_UNLOCK(active_cursor_p->software_cursor_p->save_area_p);
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
 *s3_cursor_draw_software_cursor 
 */
STATIC int
s3_cursor_draw_software_cursor(int cursor_index)
{
	int sx;
	int sy;
	int width;
	int height;
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	struct s3_cursor *active_cursor_p;
	unsigned short plane_mask;

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
				"(s3_cursor_draw_software_cursor){\n"
				"\tcursor_index= %d\n}\n",
				cursor_index);
	}
#endif

	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,active_cursor_p));

	width = active_cursor_p->cursor_width; 
	height = active_cursor_p->cursor_height; 

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s3_cursor_draw_software_cursor){\n"
			"\tdraw_width = %d\n"
			"\tdraw_height = %d\n"
			"}\n",
			width,
			height);
	}
#endif



	if(OMM_LOCK(active_cursor_p->software_cursor_p->src_bitmap_p)) 
	{
		sx =  active_cursor_p->software_cursor_p->src_bitmap_p->x; 
		sy =  active_cursor_p->software_cursor_p->src_bitmap_p->y; 

		plane_mask =
			 active_cursor_p->software_cursor_p->src_bitmap_p->planemask;

		s3_cursor_software_cursor_draw_helper(sx, sy,
			s3_cursor_state_p->current_cursor_status.x,
			s3_cursor_state_p->current_cursor_status.y, 
			width,
			height,
			active_cursor_p->foreground_color,
			active_cursor_p->background_color,
			plane_mask);

		OMM_UNLOCK(active_cursor_p->software_cursor_p->src_bitmap_p);
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);

		return 0;
	}

	/*
	** Copy invsrc
	*/
	if (OMM_LOCK(active_cursor_p->software_cursor_p->invsrc_bitmap_p)) 
	{
		sx =  active_cursor_p->software_cursor_p->invsrc_bitmap_p->x;
		sy =  active_cursor_p->software_cursor_p->invsrc_bitmap_p->y; 

		plane_mask =
			 active_cursor_p->software_cursor_p->invsrc_bitmap_p->planemask;

		s3_cursor_software_cursor_draw_helper(sx,sy,
			s3_cursor_state_p->current_cursor_status.x,
			s3_cursor_state_p->current_cursor_status.y, 
			width,
			height,
			active_cursor_p->background_color,
			active_cursor_p->foreground_color,
			plane_mask);

		OMM_UNLOCK(active_cursor_p->software_cursor_p->invsrc_bitmap_p);
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
 * s3_software_cursor_turnon  
 */
STATIC SIBool
s3_software_cursor_turnon(SIint32 cursor_index)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct s3_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
			"(s3_cursor_software_turnon){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif
	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,active_cursor_p));
#endif

	if (s3_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		return SI_SUCCEED;
	}

	if (s3_cursor_save_obscured_screen(cursor_index) == 0)
	{
		return SI_FAIL;
	}

	if (s3_cursor_draw_software_cursor(cursor_index) == 0)
	{
		return SI_FAIL;
	}


	s3_cursor_state_p->current_cursor_status.flag = CURSOR_ON;
	s3_cursor_state_p->current_cursor_status.current_index = cursor_index;

	return SI_SUCCEED;
}


/*
 * s3_software_cursor_turnoff
 */
STATIC SIBool
s3_software_cursor_turnoff(SIint32 cursor_index)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct s3_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
				"(s3_software_cursor_turnoff){\n"
				"\tcursor_index= %d\n}\n",
				(int)cursor_index);
	 }
#endif

	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,active_cursor_p));
#endif

	if (s3_cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		return SI_SUCCEED;
	}
	else
	{
		s3_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
	}

 	if (s3_cursor_restore_obscured_screen(cursor_index) == 0)
	{
		return SI_FAIL;
	}
	else
	{
		return SI_SUCCEED;
	}
}


/*
 *s3_software_cursor_move
 */

STATIC SIBool
s3_software_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	struct s3_cursor *active_cursor_p;
	int	cursor_width;
	int	cursor_height;

#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
				"(s3_software_cursor_move){\n"
				"\tcursor_index= %d\n"
				"\tx = %d\n"
				"\ty= %d\n}\n",
				(int)cursor_index,
				(int)x, (int)y);
	 }
#endif

		
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,active_cursor_p));

	if (s3_cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		s3_cursor_state_p->current_cursor_status.x = x;
		s3_cursor_state_p->current_cursor_status.y = y;
		return SI_SUCCEED;
	}


	(*active_cursor_p->turnoff_function_p)( cursor_index);

	s3_cursor_state_p->current_cursor_status.x = x;
	s3_cursor_state_p->current_cursor_status.y = y;
	s3_cursor_state_p->current_cursor_status.current_index = cursor_index;

	return (*active_cursor_p->turnon_function_p)(cursor_index);
} 


/*
 *s3_software_cursor_download
 */
STATIC SIBool
s3_software_cursor_download( SIint32 cursor_index, SICursorP cursor_p )
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	struct s3_cursor  *active_cursor_p;
	

#if defined(__DEBUG__)
	if (s3_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
				"(s3_cursor_software_download){\n"
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
	ASSERT ( S3_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR, active_cursor_p));

	/*
	 * If the cursor being downloaded is currently in use and turned ON
	 * then turn it OFF, download the new cursor and then turn it ON
	 */
	if (s3_cursor_state_p->current_cursor_status.current_index ==
		 cursor_index 
		 && s3_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		(*active_cursor_p->turnoff_function_p)(cursor_index);

		if (s3_cursor_create_software_cursor(active_cursor_p,cursor_p)
			 == 0 )
		{
			return SI_FAIL;
		}
		return (*active_cursor_p->turnon_function_p)(cursor_index);
	}
	else
	{
		return (
			(s3_cursor_create_software_cursor(active_cursor_p,cursor_p) ==
			 0) ? SI_FAIL : SI_SUCCEED);
	}
}




/********************************************************************
 * Hardware cursor routines
 ********************************************************************/
/*
 * s3_cursor_hardware_cursor_download_helper 
 */ 
STATIC void
s3_cursor_hardware_cursor_download_helper(
	 unsigned char *cursor_bitmap_p,
	 int destination_x, int destination_y)
{

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	int	transfer_height;
	int	transfer_width; 
	int	pixtrans_words_count;
	int	total_pixels;
	int	dummy_pixels_count;

	/*
	 * The following drawing control parameters have to be set:
	 *  WRT_MASK 		: all planes enabled
	 *  COLOR_SOURCE 	: FRGD_MIX
	 *  COLOR			: CPU  data
	 * 	ALU Function	: New
	 *	Clipping rect	: to physical memory dimensions
	 */ 	
	
	ASSERT(!S3_IS_FIFO_OVERFLOW());

	S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
		S3_MAX_PHYSICAL_X_PIXELS(), S3_MAX_PHYSICAL_Y_PIXELS());

	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;
	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);

	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);

	S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
	S3_STATE_SET_WRT_MASK(screen_state_p,((1 << S3_SCREEN_DEPTH()) - 1));
	
	total_pixels = 
		((S3_HARDWARE_CURSOR_MAX_WIDTH()*S3_HARDWARE_CURSOR_DEPTH) /
			S3_SCREEN_DEPTH()) * S3_HARDWARE_CURSOR_MAX_HEIGHT();

	transfer_height = total_pixels / S3_MAX_PHYSICAL_X_PIXELS();

	/*
	 * In some cases the cursor might take some full
	 * lines following by an incomplete line, in such  cases
	 * we have to transfer dummy pixels to fill the rest of the
	 * line.
	 */
	if (total_pixels % S3_MAX_PHYSICAL_X_PIXELS())
	{
		dummy_pixels_count = 
			S3_MAX_PHYSICAL_X_PIXELS() -
			(total_pixels % S3_MAX_PHYSICAL_X_PIXELS());
		++transfer_height;
		transfer_width = S3_MAX_PHYSICAL_X_PIXELS();
	}
	else
	{
		transfer_width = total_pixels;
		transfer_height = 1;
		dummy_pixels_count = 0;
	}


	pixtrans_words_count =
	  ((total_pixels * S3_SCREEN_DEPTH()) / screen_state_p->pixtrans_width);

#if defined(__DEBUG__)
	if (s3_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
		"s3_cursor_hardware_cursor_download_helper\n{\n"
		"\tdestination_x = %d\n"
		"\tdestination_y = %d\n"
		"\ttransfer_width = %d\n"
		"\ttransfer_height = %d\n"
		"\tdummy_pixels_count	= %d\n"
		"\ttotal_pixels = %d\n"
		"\tpixtrans_words_count = %d\n"
		"}\n",
		destination_x,
		destination_y,
		transfer_width,
		transfer_height,
		dummy_pixels_count,
		total_pixels,
		pixtrans_words_count);
	}
#endif

	S3_WAIT_FOR_FIFO(5);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
		 destination_x);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
		 destination_y);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		 transfer_width - 1);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
		((transfer_height - 1) & S3_MULTIFUNC_VALUE_BITS));
		
	
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_THRO_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);
	
	ASSERT(pixtrans_words_count > 0);

		
	S3_WAIT_FOR_FIFO(8);
	(*screen_state_p->screen_write_bits_p)
		(screen_state_p->pixtrans_register,
		 pixtrans_words_count, 
		 (void *) cursor_bitmap_p);

	if (dummy_pixels_count)
	{
		pixtrans_words_count =
	  	((dummy_pixels_count * S3_SCREEN_DEPTH()) /
		screen_state_p->pixtrans_width);

		(*screen_state_p->screen_write_bits_p)
			(screen_state_p->pixtrans_register,
			 pixtrans_words_count, 
			 (void *) cursor_bitmap_p);
	}

	
	
	ASSERT(!S3_IS_FIFO_OVERFLOW());
}

/*
 *s3_cursor_generate_hardware_cursor
 */
STATIC void
s3_cursor_generate_hardware_cursor( SIbitmapP  si_mask_p,
	SIbitmapP si_source_p, void *cursor_bitmap_p)
{

	int	destination_step;
	int	source_step;
	int	effective_cursor_width;
	int	effective_cursor_height;
	int	effective_no_of_words_per_line;
	unsigned short *mask_p;
	unsigned short *source_p;
	unsigned short *tmp_image_p;
	void *tmp_mask_p;
	void *tmp_source_p;
	int	i;
	int j;
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	const unsigned char *tmp_byte_invert_table_p  = 
		screen_state_p->byte_invert_table_p;
	

	ASSERT(si_mask_p && si_source_p && cursor_bitmap_p );
	ASSERT(si_mask_p->Bwidth == si_source_p->Bwidth);
	ASSERT(si_mask_p->Bheight == si_source_p->Bheight);
	ASSERT(si_mask_p->BbitsPerPixel == 1);
	
	/*
	 * if height/width is more than 64 pixels truncate it
	 */
	if (si_mask_p->Bwidth > S3_HARDWARE_CURSOR_MAX_WIDTH())
	{
		effective_cursor_width = S3_HARDWARE_CURSOR_MAX_WIDTH();
	}
	else
	{
		effective_cursor_width = si_mask_p->Bwidth;
	}

	if (si_mask_p->Bheight > S3_HARDWARE_CURSOR_MAX_HEIGHT())
	{
		effective_cursor_height = S3_HARDWARE_CURSOR_MAX_HEIGHT();
	}
	else
	{
		effective_cursor_height = si_mask_p->Bheight;
	}
	/*
	 * Alternating 'xor' and 'and' words
	 */
	destination_step = (S3_HARDWARE_CURSOR_MAX_WIDTH() / 8) * 2;  /*bytes*/

	source_step = ((si_mask_p->Bwidth + 31) & ~31) >> 3;		/*bytes*/

	/*
	 * Effective number of short words per line
	 */
	effective_no_of_words_per_line =
		 ((effective_cursor_width + 31) & ~31) / 16;

#if defined(__DEBUG__)
	if (s3_cursor_debug)
	{
		(void)fprintf (debug_stream_p,
		"s3_cursor_generate_hardware_cursor)\n{\n"
		"\teffective_cursor_width = %d\n"
		"\teffective_cursor_height = %d\n"
		"\tdestination_step = %d\n"
		"\tsource_step = %d\n"
		"\teffective_no_of_words_per_line = %d\n"
		"}\n",
		effective_cursor_width,
		effective_cursor_height,
		destination_step,
		source_step,
		effective_no_of_words_per_line);
	}
#endif
	tmp_source_p  = si_source_p->Bptr;
	tmp_mask_p  = si_mask_p->Bptr;

	for(i=0; i <effective_cursor_height; ++i)
	{
		unsigned short mask_word;
		unsigned short source_word;
		
		source_p = (unsigned short*)tmp_source_p;
		mask_p = (unsigned short*)tmp_mask_p;
		tmp_image_p = cursor_bitmap_p;

		for(j=0;j<effective_no_of_words_per_line; ++j)
		{
			mask_word = *mask_p;
			source_word = *source_p;

			
			if (effective_cursor_width > j * 16) 
			{
				if (effective_cursor_width < (j * 16 + 15))
				{
					short mask = (effective_cursor_width % 16);
					mask = (1 << mask) - 1;
					mask_word  = mask_word & mask;
					source_word = source_word & mask;
				}
				/*
				 * Invert each byte
				 */
				((unsigned char*)&mask_word)[0] =
				 tmp_byte_invert_table_p[((unsigned char*)&mask_word)[0]];
				((unsigned char*)&mask_word)[1] =
				 tmp_byte_invert_table_p[((unsigned char*)&mask_word)[1]];

				((unsigned char*)&source_word)[0] =
				 tmp_byte_invert_table_p[((unsigned char*)&source_word)[0]];
				((unsigned char*)&source_word)[1] =
				 tmp_byte_invert_table_p[((unsigned char*)&source_word)[1]];

				*tmp_image_p++ = mask_word;
				*tmp_image_p++ = source_word;
			}
			else
			{
					*tmp_image_p++ = 0;
					*tmp_image_p++ = 0;
			}
			++mask_p;
			++source_p;
		}

		cursor_bitmap_p  = (char*)cursor_bitmap_p +  destination_step;
		tmp_source_p = (char*)tmp_source_p + source_step;
		tmp_mask_p = (char*)tmp_mask_p + source_step;
	}
}
	

/*
**s3_cursor_create_hardware_cursor
*/
STATIC int 
s3_cursor_create_hardware_cursor(struct s3_cursor *active_cursor_p,
					 SICursor *si_cursor_p)
{
	int height ;
	int	width ;
	int	bitmap_size;
	unsigned char *cursor_bitmap_p;
	int  total_bytes;
	SIbitmap src_bitmap;
	SIbitmap invsrc_bitmap;
	SIbitmap mask_bitmap;

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();

#if defined ( __DEBUG__ )
	if ( s3_cursor_debug )
	{
		(void)fprintf	( debug_stream_p,
"(s3_cursor_create_hardware_cursor){\n"
"\tactive_cursor_p= %p\n"
"\tsi_cursor_p= %p}\n",
		    (void*)active_cursor_p,
		    (void*)si_cursor_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p));
	ASSERT((active_cursor_p != NULL) && ( si_cursor_p != NULL ));
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR, active_cursor_p));

	height = si_cursor_p->SCheight ;

	width = si_cursor_p->SCwidth ;



	total_bytes = (( si_cursor_p->SCsrc->Bwidth + 31 ) & ~31 ) >> 3;
	total_bytes  *= si_cursor_p->SCsrc->Bheight; 
	ASSERT(total_bytes > 0 );

	(void) memcpy(&mask_bitmap,si_cursor_p->SCmask, sizeof(SIbitmap));
	(void) memcpy(&src_bitmap,si_cursor_p->SCsrc, sizeof(SIbitmap));
	(void) memcpy(&invsrc_bitmap,si_cursor_p->SCinvsrc, sizeof(SIbitmap));

	mask_bitmap.Bptr = allocate_and_clear_memory(total_bytes);
	src_bitmap.Bptr = allocate_and_clear_memory(total_bytes);
	invsrc_bitmap.Bptr = allocate_and_clear_memory(total_bytes);

	/*
	 * NOTE: invsrc_bitmap is not used in this function
	 */
	s3_cursor_clear_extra_bits_and_copy(si_cursor_p,
		&src_bitmap,&invsrc_bitmap,
		&mask_bitmap);

	/*
	 * Hardware cursor is 2 bit deep
	 */
	bitmap_size = ((S3_HARDWARE_CURSOR_MAX_WIDTH() *
		 S3_HARDWARE_CURSOR_MAX_HEIGHT()) / 8) * 2; /*bytes*/
	cursor_bitmap_p =allocate_memory(bitmap_size); 
	/*
	** Set all pixels to transparent
	*/
	(void)memset(cursor_bitmap_p,0,bitmap_size);
		

	s3_cursor_generate_hardware_cursor(&mask_bitmap, 
		 &src_bitmap,
		 cursor_bitmap_p);
	free_memory(mask_bitmap.Bptr);
	free_memory(src_bitmap.Bptr);
	free_memory(invsrc_bitmap.Bptr);

	active_cursor_p->cursor_height = 
	    ((height > S3_HARDWARE_CURSOR_MAX_HEIGHT() ) ?
		 S3_HARDWARE_CURSOR_MAX_HEIGHT() : height);

	active_cursor_p->cursor_width = 
	    ((width > S3_HARDWARE_CURSOR_MAX_WIDTH() ) ?
		 S3_HARDWARE_CURSOR_MAX_WIDTH() : width);

	active_cursor_p->foreground_color =
		si_cursor_p->SCfg;

	active_cursor_p->background_color =
		si_cursor_p->SCbg;

	active_cursor_p->hardware_cursor_p->horz_cursor_offset = 0;
	active_cursor_p->hardware_cursor_p->vert_cursor_offset = 0;

	s3_cursor_hardware_cursor_download_helper(cursor_bitmap_p, 
		active_cursor_p->hardware_cursor_p->offscreen_x_location,
		active_cursor_p->hardware_cursor_p->offscreen_y_location);

	free_memory(cursor_bitmap_p) ;

	return 1;
}


/*
 * s3_cursor_draw_hardware_cursor
 */
STATIC int
s3_cursor_draw_hardware_cursor(int	cursor_index)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	struct s3_cursor *active_cursor_p ;
	int	start_addr;
	int	x;
	int	y;
	

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p));
	
	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));

	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR, active_cursor_p));


	S3_DISABLE_HARDWARE_CURSOR(screen_state_p);

	S3_SET_HARDWARE_CURSOR_COLORS(screen_state_p,
	    active_cursor_p->foreground_color,
		active_cursor_p->background_color);


	start_addr =
		(active_cursor_p->hardware_cursor_p->offscreen_y_location * 
		((S3_MAX_PHYSICAL_X_PIXELS()*S3_SCREEN_DEPTH()) / 8)) / 1024;

	S3_SET_HARDWARE_CURSOR_STORAGE_START_ADDRESS(screen_state_p,
		 (unsigned short)start_addr);

	

	/*
	 * Take care of -ve x,y positions
	 */
	x = s3_cursor_state_p->current_cursor_status.x;
	y = s3_cursor_state_p->current_cursor_status.y;

	if ( x < 0)
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset  = (-x) & 0xFE;
		S3_SET_HARDWARE_CURSOR_HORZ_DISPLAY_OFFSET(screen_state_p,
	  	active_cursor_p->hardware_cursor_p->horz_cursor_offset);
		x = 0;
	}

	if ( y < 0)
	{
		active_cursor_p->hardware_cursor_p->vert_cursor_offset  = (-y) & 0xFE;
		S3_SET_HARDWARE_CURSOR_VERT_DISPLAY_OFFSET(screen_state_p,
	  	active_cursor_p->hardware_cursor_p->vert_cursor_offset);
		y = 0;
	}
	S3_SET_HARDWARE_CURSOR_POSITION(screen_state_p,x,y);

	S3_ENABLE_HARDWARE_CURSOR(screen_state_p);
#if defined(__DEBUG__)
	if (s3_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"(s3_cursor_draw_hardware_cursor){\n"
			"\t#Cursor has been drawn\n"
			"\tx = %d\n"
			"\ty = %d\n"
			"\thorz offset = %d\n"
			"\tvert offset = %d\n"
			"}\n",
			s3_cursor_state_p->current_cursor_status.x,
			s3_cursor_state_p->current_cursor_status.y,
	  		active_cursor_p->hardware_cursor_p->horz_cursor_offset,
	  		active_cursor_p->hardware_cursor_p->vert_cursor_offset);
	}
#endif
	return 1;
	
}

/*
 * s3_cursor__vt_switch_out__
 * 
 * Called when the X server is about to lose control of its virtual
 * terminal.  The cursor module needs to turn off the hardware cursor
 * before the VT switch occurs, in order to remove it from interfering
 * with the display.
 * Note that we don't save the cursor area as the responsibilty of
 * saving offscreen memory rests with the vt switch code in the "s3"
 * module (s3.c).
 */
function void
s3_cursor__vt_switch_out__(void)
{

	S3_CURRENT_SCREEN_STATE_DECLARE();
	struct s3_register_state *s3_register_state_p =
		&(screen_state_p->register_state);
	S3_CURSOR_STATE_DECLARE();


#if defined(__DEBUG__)
	if(s3_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
"(s3_cursor__vt_switch_out__){\n"
"\t# VT switch out\n"
"}\n");
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	/*
	 * for now check to see if s3_cursor_state_p is not null
	 */
	if((s3_cursor_state_p == NULL) ||
	   (s3_cursor_state_p->cursor_type != S3_CURSOR_HARDWARE))
	{
		return;
	}

	/*
	 * Turnoff the hardware-cursor
	 */
	S3_DISABLE_HARDWARE_CURSOR(screen_state_p);
}/*s3_cursor_vt_switch__out__*/

/*
 * s3_cursor__vt_switch_in__
 *
 * Called when the X server is going to switch into a virtual
 * terminal.  In the cursor module we need to re-enable the hardware
 * cursor if it was in use.  The contents of the offscreen location
 * are assumed to have been restored previous to this invocation.
 */

function void
s3_cursor__vt_switch_in__(void)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	struct s3_register_state *s3_register_state_p =
		&(screen_state_p->register_state);
	S3_CURSOR_STATE_DECLARE ();

#if defined(__DEBUG__)
	if(s3_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
"(s3_cursor_vt_switch__in__){\n"
"\t# VT switch in\n"
"}\n" );
	}
#endif

	if((s3_cursor_state_p == NULL) ||
	   (s3_cursor_state_p->cursor_type != S3_CURSOR_HARDWARE))
	{
		return;
	}

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p));

	/*
	 * Turnon the hardware-cursor 
	 */
	if(s3_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		S3_ENABLE_HARDWARE_CURSOR(screen_state_p);
	}

}/*s3_cursor_vt_switch__in__*/

/*
 *s3_hardware_cursor_download
 */

STATIC SIBool
s3_hardware_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE ();
	struct s3_cursor *active_cursor_p ;

#if defined(__DEBUG__)
	if (s3_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
			"(s3_hardware_cursor_download){\n"
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
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p));
	
	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));

	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR, active_cursor_p));

	/*
	 * If the cursor being downloaded is the current cursor, turn it
	 * off, reload the bits.
	 */
	if (s3_cursor_state_p->current_cursor_status.current_index ==
		cursor_index  && 
		s3_cursor_state_p->current_cursor_status.flag == CURSOR_ON )
	{
		/*
		 * Turn off existing cursor.
		 */
		(*active_cursor_p->turnoff_function_p) ( cursor_index );

		/*
		 * Create the new cursor.
		 */
		if (s3_cursor_create_hardware_cursor(active_cursor_p,cursor_p) == 0)
		{
			return SI_FAIL ;
		}
		
		active_cursor_p =
			s3_cursor_state_p->cursor_list_pp[cursor_index];

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
		return ((s3_cursor_create_hardware_cursor(active_cursor_p,cursor_p) 
				== 1) ? SI_SUCCEED : SI_FAIL);
	}
}


/*
**s3_hardware_cursor_turnon  
*/
STATIC SIBool
s3_hardware_cursor_turnon ( SIint32 cursor_index )
{

	S3_CURRENT_SCREEN_STATE_DECLARE();

	S3_CURSOR_STATE_DECLARE ();


#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s3_hardware_cursor_turnon){\n"
			"\tcursor_index= %ld\n"
			"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p));
	
	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));

	if (s3_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
#if defined(__DEBUG__)
		if (s3_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
				"(s3_hardware_cursor_turnon){\n"
				"\t#Cursor already turned on\n"
				"\n}\n");
		}
#endif


		return SI_SUCCEED ;
	}

	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,
		 s3_cursor_state_p->cursor_list_pp[cursor_index]));

	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
	if (s3_cursor_draw_hardware_cursor(cursor_index) == 0)
	{
		return SI_FAIL ;
	}
	s3_cursor_state_p->current_cursor_status.flag = CURSOR_ON ;

	s3_cursor_state_p->current_cursor_status.current_index = cursor_index ;

	return SI_SUCCEED ;
}

/*
**s3_hardware_cursor_turnoff
*/
STATIC SIBool
s3_hardware_cursor_turnoff ( SIint32 cursor_index )
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	
	struct s3_register_state *s3_register_state_p =
		&(screen_state_p->register_state);
	S3_CURSOR_STATE_DECLARE ();


#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s3_hardware_cursor_turnoff){\n"
			"\tcursor_index= %ld\n"
			"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p));
	
	ASSERT(S3_CURSOR_IS_VALID_INDEX(cursor_index));

	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR,
		s3_cursor_state_p->cursor_list_pp[cursor_index]));

	s3_cursor_state_p->current_cursor_status.flag = CURSOR_OFF ;
	/*
	 ** disable cursor 
	 */
	S3_DISABLE_HARDWARE_CURSOR(screen_state_p);
	


	return SI_SUCCEED ;
}

/*
**s3_hardware_cursor_move
*/
STATIC SIBool
s3_hardware_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	
	S3_CURSOR_STATE_DECLARE();
	struct s3_cursor *active_cursor_p;
	int	x_offset;
	int	y_offset;
	int	cursor_height;
	int	cursor_width;
	int	crtoffset_change_required;

	
	s3_cursor_state_p->current_cursor_status.x = x;
	s3_cursor_state_p->current_cursor_status.y = y;
	if ( s3_cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		return SI_SUCCEED;
	}
	active_cursor_p = s3_cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR, active_cursor_p));
	/*
	 * Check if panning will be needed 
	 */
	if (screen_state_p->is_panning_to_virtual_screen_feasible == TRUE)
	{

		cursor_height = active_cursor_p->cursor_height;
		cursor_width  = active_cursor_p->cursor_width;

		if ( x + cursor_width > 
				s3_cursor_state_p->visible_window_top_x +
					 S3_MAX_DISPLAYED_X_PIXELS())
		{
			s3_cursor_state_p->visible_window_top_x = 
				(x + cursor_width > S3_MAX_VIRTUAL_X_PIXELS()) ?
					(S3_MAX_VIRTUAL_X_PIXELS() -
						 S3_MAX_DISPLAYED_X_PIXELS())	:
					(x + cursor_width - S3_MAX_DISPLAYED_X_PIXELS());
			x -=  s3_cursor_state_p->visible_window_top_x;
			s3_cursor_state_p->current_cursor_status.x = x ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (x < s3_cursor_state_p->visible_window_top_x &&
					s3_cursor_state_p->visible_window_top_x > 0)
			{
				if ( x >= 0)
				{
					s3_cursor_state_p->visible_window_top_x = x;
					x = 0;
					s3_cursor_state_p->current_cursor_status.x = x ;
				}
				else
				{
					x += s3_cursor_state_p->visible_window_top_x; 
					s3_cursor_state_p->current_cursor_status.x = x ;
					s3_cursor_state_p->visible_window_top_x =0;
				}
					
				crtoffset_change_required = 1;
			}
			else
			{
				x -= s3_cursor_state_p->visible_window_top_x; 
				s3_cursor_state_p->current_cursor_status.x = x ;
			}
		}
		if ( y + cursor_height > 
				s3_cursor_state_p->visible_window_top_y +
					 S3_MAX_DISPLAYED_Y_PIXELS())
		{
			s3_cursor_state_p->visible_window_top_y = 
				(y + cursor_height > S3_MAX_VIRTUAL_Y_PIXELS()) ?
					(S3_MAX_VIRTUAL_Y_PIXELS() -
						 S3_MAX_DISPLAYED_Y_PIXELS())	:
					(y + cursor_height - S3_MAX_DISPLAYED_Y_PIXELS());

			y -=  s3_cursor_state_p->visible_window_top_y;
			s3_cursor_state_p->current_cursor_status.y = y ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (y < s3_cursor_state_p->visible_window_top_y &&
					s3_cursor_state_p->visible_window_top_y > 0)
			{

				if ( y >= 0)
				{
					s3_cursor_state_p->visible_window_top_y = y;
					y = 0;
					s3_cursor_state_p->current_cursor_status.y = y ;
				}
				else
				{
					y += s3_cursor_state_p->visible_window_top_y; 
					s3_cursor_state_p->current_cursor_status.y = y ;
					s3_cursor_state_p->visible_window_top_y =0;
				}
				crtoffset_change_required = 1;
			}
			else
			{
				y -= s3_cursor_state_p->visible_window_top_y; 
				s3_cursor_state_p->current_cursor_status.y = y ;
			}
		}
		if (crtoffset_change_required == 1)
		{
			S3_SET_DISPLAY_START_ADDRESS(screen_state_p,
				s3_cursor_state_p->visible_window_top_x,
				s3_cursor_state_p->visible_window_top_y);
			

#if defined(__DEBUG__)
			if (s3_cursor_debug)
			{
				(void)fprintf(debug_stream_p,
					"(s3_hardware_cursor_move){\n"
					"\twindow_top_x = %d\n"
					"\twindow_top_y = %d\n"
					"}",
					s3_cursor_state_p->visible_window_top_x,
					s3_cursor_state_p->visible_window_top_y);
			}
#endif
						
		}

	}
	

	if (x < 0)
	{
		x_offset =  (-x) & 0xFE;
		x = 0;
	}
	else
	{
		x_offset = 0;
	}

	if (y < 0)
	{
		y_offset = (-y) & 0xFE;
		y = 0;
	}
	else
	{
		y_offset = 0;
	}

	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);

	S3_SET_HARDWARE_CURSOR_HORZ_DISPLAY_OFFSET(screen_state_p,
		(unsigned short)x_offset);
	S3_SET_HARDWARE_CURSOR_VERT_DISPLAY_OFFSET(screen_state_p,
		(unsigned short)y_offset);
	S3_SET_HARDWARE_CURSOR_POSITION(screen_state_p,(unsigned short)x,
		(unsigned short)y);
	
	active_cursor_p->hardware_cursor_p->horz_cursor_offset = x_offset;
	active_cursor_p->hardware_cursor_p->vert_cursor_offset = y_offset;
	return SI_SUCCEED ;

}


/*************************************************************************
 * Hardware/Software cursor initialization routines
 *************************************************************************/


STATIC int
s3_cursor_initialize_software_cursors(int no_of_cursors)
{

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	int i ;
	struct s3_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p)) ;

	s3_cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct s3_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		s3_cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct s3_cursor));
		STAMP_OBJECT(S3_CURSOR, s3_cursor_state_p->cursor_list_pp[i]);

		cursor_p = s3_cursor_state_p->cursor_list_pp[i];
		cursor_p->turnoff_function_p = s3_software_cursor_turnoff;
		cursor_p->turnon_function_p = s3_software_cursor_turnon;
		cursor_p->move_function_p = s3_software_cursor_move;

		cursor_p->software_cursor_p= 
		 allocate_and_clear_memory(sizeof(struct s3_cursor_software_cursor));

		STAMP_OBJECT(S3_SOFTWARE_CURSOR, cursor_p->software_cursor_p);
		cursor_p->software_cursor_p->src_bitmap_p = NULL;
		cursor_p->software_cursor_p->invsrc_bitmap_p = NULL;
		cursor_p->software_cursor_p->save_area_p = NULL;

	}
	return 1;
	

}/*s3_cursor_initialize_software_cursors*/

/*
** s3_cursor_initialize_hardware_cursors
*/
STATIC int
s3_cursor_initialize_hardware_cursors(int no_of_cursors)
{

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURSOR_STATE_DECLARE();
	int i ;
	int total_pixels ;
	int	full_lines ;
	int last_line ;
	struct omm_allocation *omm_block_p;
	struct s3_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(S3_CURSOR_STATE, s3_cursor_state_p)) ;
	
	omm_block_p = omm_named_allocate(S3_CURSOR_NAMED_ALLOCATION_ID);
	if (omm_block_p  == NULL)
	{
		(void) fprintf(stderr, 
			S3_HW_CURSOR_OFFSCREEN_ALLOCATION_FAILED_MESSAGE);
		return 0;
	}

	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, omm_block_p));
	

#if defined ( __DEBUG__ )
	if (s3_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
			"(s3_cursor_initialize_hardware_cursors)"
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
		((S3_HARDWARE_CURSOR_MAX_WIDTH() * S3_HARDWARE_CURSOR_DEPTH) /
		 S3_SCREEN_DEPTH()) * S3_HARDWARE_CURSOR_MAX_HEIGHT();

	full_lines = total_pixels / S3_MAX_PHYSICAL_X_PIXELS ();

	last_line = total_pixels % S3_MAX_PHYSICAL_X_PIXELS ();

	if (last_line > 0)
	{
		++full_lines ;
	}

	s3_cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct s3_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		s3_cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct s3_cursor));
		STAMP_OBJECT(S3_CURSOR, s3_cursor_state_p->cursor_list_pp[i]);

		cursor_p = s3_cursor_state_p->cursor_list_pp[i];

		cursor_p->turnoff_function_p = s3_hardware_cursor_turnoff;

		cursor_p->turnon_function_p = s3_hardware_cursor_turnon;

		cursor_p->move_function_p = s3_hardware_cursor_move;

		cursor_p->hardware_cursor_p= 
		 allocate_and_clear_memory(sizeof(struct s3_cursor_hardware_cursor));

		STAMP_OBJECT(S3_HARDWARE_CURSOR, cursor_p->hardware_cursor_p);

		cursor_p->hardware_cursor_p->offscreen_x_location = 
			omm_block_p->x ;
		cursor_p->hardware_cursor_p->offscreen_y_location = 
			omm_block_p->y+ (full_lines*i);

#if defined ( __DEBUG__ )
		if ( s3_cursor_debug )
		{
			(void) fprintf(debug_stream_p,
			"(s3_cursor_initialize_hardware_cursors)"
			"{\n"
			"\tcursor_index = %d"
			"\tOffscreen loc ( %d, %d)"
			"}\n",
			 i,
			 cursor_p->hardware_cursor_p->offscreen_x_location,
			 cursor_p->hardware_cursor_p->offscreen_y_location);
		}
#endif
	}

	return 1;

}

/*
 * s3_cursor__initialize__
 */
function void
s3_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct s3_options_structure * options_p)
{
	struct s3_cursor_state  *s3_cursor_state_p;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIConfigP config_p = si_screen_p->cfgPtr;
	S3_CURRENT_SCREEN_STATE_DECLARE();


#if defined(__DEBUG__)
	if ( s3_cursor_debug )
	{
		(void) fprintf(debug_stream_p,
			"(s3__cursor__initialize__){\n"
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
			S3_MAX_DISPLAYED_X_PIXELS(),
			S3_MAX_DISPLAYED_Y_PIXELS(),
			S3_MAX_VIRTUAL_X_PIXELS(),
			S3_MAX_VIRTUAL_Y_PIXELS(),
			S3_MAX_PHYSICAL_X_PIXELS(),
			S3_MAX_PHYSICAL_Y_PIXELS());
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
	s3_cursor_state_p =
	    allocate_and_clear_memory(sizeof(struct s3_cursor_state));

	STAMP_OBJECT(S3_CURSOR_STATE, s3_cursor_state_p);

	flags_p->SIcurscnt = (options_p->number_of_downloadable_cursors) ?
	    options_p->number_of_downloadable_cursors :
	    DEFAULT_S3_NUMBER_OF_DOWNLOADABLE_CURSORS;

	/*
	 * Get the cursor size that one is supposed to support.
	 */
	if (options_p->cursor_max_size == NULL)
	{
		s3_cursor_state_p->hardware_cursor_max_width =
			DEFAULT_S3_DOWNLOADABLE_CURSOR_WIDTH;

		s3_cursor_state_p->hardware_cursor_max_height =
			DEFAULT_S3_DOWNLOADABLE_CURSOR_HEIGHT;

#if defined (__DEBUG__)
			if (s3_cursor_debug)
			{
				(void)fprintf ( debug_stream_p,
					"(s3_cursor__initialize__){\n"
					"\tUsing default cursor dimensions\n"
					"\n}\n");
			}
#endif
	}
	else
	{
		if((sscanf(options_p->cursor_max_size,"%ix%i",
			&s3_cursor_state_p->hardware_cursor_max_width, 
			&s3_cursor_state_p->hardware_cursor_max_height) != 2) ||
		    (s3_cursor_state_p->hardware_cursor_max_width <= 0) ||
		    (s3_cursor_state_p->hardware_cursor_max_height <= 0))
		{
#if defined (__DEBUG__)
			if ( s3_cursor_debug )
			{
				(void)fprintf ( debug_stream_p,
					"(s3_cursor__initialize__){\n"
					"\tUsing default cursor dimensions\n"
					"\n}\n");
			}
#endif
			(void) fprintf(stderr, S3_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
						   options_p->cursor_max_size);

			s3_cursor_state_p->hardware_cursor_max_width =
				DEFAULT_S3_DOWNLOADABLE_CURSOR_WIDTH;

			s3_cursor_state_p->hardware_cursor_max_height =
				DEFAULT_S3_DOWNLOADABLE_CURSOR_HEIGHT;
		}
#if defined (__DEBUG__)
		else
		{
			if ( s3_cursor_debug )
			{
				(void) fprintf(debug_stream_p,
					"(s3_cursor__initialize__){\n"
					"\tMax cursor width  = %d"
					"\tMax cursor height = %d"
					"\n}\n",
					s3_cursor_state_p->hardware_cursor_max_width,
					s3_cursor_state_p->hardware_cursor_max_height);
			}
		}
#endif
	}

	/*
	 * Set SI's idea of the best cursor width.
	 */
	
	flags_p->SIcurswidth =
		s3_cursor_state_p->hardware_cursor_max_width;
	flags_p->SIcursheight =
		s3_cursor_state_p->hardware_cursor_max_height;

	/*
	 * The cursor mask is valid for software cursors only.
	 */
	flags_p->SIcursmask = DEFAULT_S3_DOWNLOADABLE_CURSOR_MASK;

	/*
	 * Default handlers.
	 */
	functions_p->si_hcurs_download = 
		(SIBool (*)(SIint32, SICursorP)) s3_no_operation_succeed;
	functions_p->si_hcurs_turnon = 
		(SIBool (*)(SIint32)) s3_no_operation_succeed;
	functions_p->si_hcurs_turnoff = 
		(SIBool (*)(SIint32)) s3_no_operation_succeed;
	functions_p->si_hcurs_move = 
		(SIBool (*)(SIint32, SIint32, SIint32)) s3_no_operation_succeed;

	/*
	 * Hardware cursor does not work for the following cases:
	 * 1. Dac kind bt485 in mux mode.
	 * 2. depth greater than 16.
	 * 3. if there is no space (atleast 2 lines) below the displayed area.
	 *
	 * JUN 11
	 * 4. if the video memory configuration used is 512K.
	 */
	if((options_p->cursor_type == S3_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR) &&
	   (S3_SCREEN_DEPTH() <= 8) && 
	   (config_p->videoRam != 512) &&
	   (((screen_state_p->dac_kind == S3_DAC_BT485)&&
		(S3_MAX_DISPLAYED_X_PIXELS() >=1280)) ? 0 : 1) &&
	   ( S3_MAX_VIRTUAL_Y_PIXELS() < S3_MAX_PHYSICAL_Y_PIXELS() - 1))
	{
#if defined(__DEBUG__)
		if( s3_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
				"(s3_cursor__initialize__){\n"
				"\t#Switching to hardware cursor\n"
				"}\n");
		}
#endif
		flags_p->SIcursortype = CURSOR_TRUEHDWR;
		s3_cursor_state_p->cursor_type = S3_CURSOR_HARDWARE;
		/*
		 * Cursor manipulation functions.
		 */
		functions_p->si_hcurs_download = s3_hardware_cursor_download;
		functions_p->si_hcurs_turnon = s3_hardware_cursor_turnon;
		functions_p->si_hcurs_turnoff = s3_hardware_cursor_turnoff;
		functions_p->si_hcurs_move = s3_hardware_cursor_move;

		s3_cursor_state_p->number_of_cursors = flags_p->SIcurscnt;

		s3_cursor_state_p->current_cursor_status.x = -1;
		s3_cursor_state_p->current_cursor_status.y = -1;
		s3_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
		s3_cursor_state_p->current_cursor_status.current_index = -1;
		/*
		 * Used for pixel panning
		 */
		s3_cursor_state_p->visible_window_top_x = 0;
		s3_cursor_state_p->visible_window_top_y = 0;
		
		/*
		 * Assign the cursor state.
		 */
		(screen_state_p)->cursor_state_p = s3_cursor_state_p ;

		/*
		 * Allocate off-screen memory etc.
		 */

		if (s3_cursor_initialize_hardware_cursors(flags_p->SIcurscnt) == 0)
		{
			/*
			 * Allocation failed : Undo all work done 
			 */

#if (defined(__DEBUG__))
			if (s3_cursor_debug)
			{
				(void) fprintf(debug_stream_p,
					"(s3_cursor__initialize__)\n"
					"{\n"
					"\t# Initialization of hardware cursors failed.\n"
					"}\n");
			
			}
#endif

			if(screen_state_p->generic_state.screen_server_version_number >=
				X_SI_VERSION1_1)  
			{
				flags_p->SIcursortype = CURSOR_FAKEHDWR;
				functions_p->si_hcurs_download = 
					(SIBool (*)(SIint32, SICursorP)) s3_no_operation_succeed;
				functions_p->si_hcurs_turnon = 
					(SIBool (*)(SIint32)) s3_no_operation_succeed;
				functions_p->si_hcurs_turnoff = 
					(SIBool (*)(SIint32)) s3_no_operation_succeed;
				functions_p->si_hcurs_move = 
				(SIBool (*)(SIint32, SIint32, SIint32))s3_no_operation_succeed;
			}
			else
			{
				flags_p->SIcursortype = CURSOR_FAKEHDWR;
				s3_cursor_state_p->cursor_type = S3_CURSOR_SOFTWARE;
				/*
				 * Cursor manipulation functions.
				 */
				functions_p->si_hcurs_download = s3_software_cursor_download;
				functions_p->si_hcurs_turnon = s3_software_cursor_turnon;
				functions_p->si_hcurs_turnoff = s3_software_cursor_turnoff;
				functions_p->si_hcurs_move = s3_software_cursor_move;

				s3_cursor_state_p->number_of_cursors = flags_p->SIcurscnt;

				s3_cursor_state_p->current_cursor_status.x = -1;
				s3_cursor_state_p->current_cursor_status.y = -1;
				s3_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
				s3_cursor_state_p->current_cursor_status.current_index = -1;

				/*
				 * Assign the cursor state.
				 */
				(screen_state_p)->cursor_state_p = s3_cursor_state_p ;
				(void) s3_cursor_initialize_software_cursors(
					flags_p->SIcurscnt); 
			}

			return ;
		}
	}
	else /*setup for software cursor*/
	{
	
		flags_p->SIcursortype = CURSOR_FAKEHDWR;
#if defined(__DEBUG__)
		if( s3_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
			"(s3_cursor__initialize__){\n"
			"\t#Switching to software cursor\n"
			"}\n");
		}
#endif

		if(screen_state_p->generic_state.screen_server_version_number >=
		   	X_SI_VERSION1_1)  
		{
			functions_p->si_hcurs_download = 
				(SIBool (*)(SIint32, SICursorP)) s3_no_operation_succeed;
			return;
		}
		s3_cursor_state_p->cursor_type = S3_CURSOR_SOFTWARE;


		/*
		 * Cursor manipulation functions.
		 */
		functions_p->si_hcurs_download = s3_software_cursor_download;
		functions_p->si_hcurs_turnon = s3_software_cursor_turnon;
		functions_p->si_hcurs_turnoff = s3_software_cursor_turnoff;
		functions_p->si_hcurs_move = s3_software_cursor_move;

		s3_cursor_state_p->number_of_cursors = flags_p->SIcurscnt;

		s3_cursor_state_p->current_cursor_status.x = -1;
		s3_cursor_state_p->current_cursor_status.y = -1;
		s3_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
		s3_cursor_state_p->current_cursor_status.current_index = -1;

		/*
		 * Assign the cursor state.
		 */
		(screen_state_p)->cursor_state_p 
			= s3_cursor_state_p ;
		(void) s3_cursor_initialize_software_cursors(flags_p->SIcurscnt); 
	}
}

/*
 * s3_cursor_make_named_allocation_string 
 */

function char *
s3_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct s3_options_structure * options_p)
{

	int total_pixels ;
	int lines ;
	int last_line ;
	char *string_p ;
	int hardware_cursor_max_width;
	int hardware_cursor_max_height;
	int number_of_cursors ;
	int start_y ;
	char tmp_buf[S3_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN+1] ;

	/*
	 * No need for named allocation for cursor if hardware cursor is
	 * not requested or if there no memory below the display area.
	 * Also see s3_cursor_initialize function.
	 */
	if ((options_p->cursor_type == S3_OPTIONS_CURSOR_TYPE_SOFTWARE_CURSOR) || 
	    (S3_MAX_VIRTUAL_Y_PIXELS() >= S3_MAX_PHYSICAL_Y_PIXELS() - 1))
	{
#if defined (__DEBUG__)
		if ( s3_cursor_debug )
		{
			(void)fprintf(debug_stream_p,
				"(s3_cursor_make_named_allocation_string){\n" 
				"\tReturning null\n"
				"}\n");
		}
#endif
		return(NULL);
	}

	if(options_p->cursor_max_size == NULL)
	{
		hardware_cursor_max_width = DEFAULT_S3_DOWNLOADABLE_CURSOR_WIDTH;

		hardware_cursor_max_height = DEFAULT_S3_DOWNLOADABLE_CURSOR_HEIGHT;
	}
	else if ((sscanf(options_p->cursor_max_size,"%ix%i",
			 &hardware_cursor_max_width, 
			 &hardware_cursor_max_height) != 2) ||
			 (hardware_cursor_max_width <= 0) ||
			 (hardware_cursor_max_height <= 0))
	{
		(void) fprintf(stderr, S3_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
					   options_p->cursor_max_size);
		
		hardware_cursor_max_width = DEFAULT_S3_DOWNLOADABLE_CURSOR_WIDTH;

		hardware_cursor_max_height = DEFAULT_S3_DOWNLOADABLE_CURSOR_HEIGHT;
	}


	number_of_cursors = (options_p->number_of_downloadable_cursors >  0) ?
			options_p->number_of_downloadable_cursors :
			DEFAULT_S3_NUMBER_OF_DOWNLOADABLE_CURSORS;

	total_pixels = (hardware_cursor_max_width*2/(S3_SCREEN_DEPTH()))* 
	    hardware_cursor_max_height;

	lines = total_pixels / S3_MAX_PHYSICAL_X_PIXELS () ;

	last_line = total_pixels % S3_MAX_PHYSICAL_X_PIXELS () ;


	if ( last_line > 0 )
	{
		++lines ;
	}


	/*
	 * If it is a 1280x1024 mode then allocate starting from the
	 * first line after displayed y lines. This is because in 1280 modes
	 * the HW cursor works only if downloaded byte position is the lcm of
	 * 1280 and 1024.
	 */
	if (S3_MAX_VIRTUAL_Y_PIXELS() == 1024)
	{
		start_y = S3_MAX_VIRTUAL_Y_PIXELS();
	}
	else
	{
		/*
		 * for all other cases we will allocate at the end of
		 * the offscreen area
		 */
		start_y =  S3_MAX_PHYSICAL_Y_PIXELS() -
			 (number_of_cursors * lines) - 1;
		/*
		 * For some reason hardware cursor has problems while downloading
		 * at the end of a 4MB/3MB boundary. Hack to make it download at end
		 * of display area.
		 */
		if (S3_MAX_PHYSICAL_Y_PIXELS() > 3000)
		{
			start_y = S3_MAX_VIRTUAL_Y_PIXELS();
		}
	}

	(void)sprintf(tmp_buf, S3_CURSOR_NAMED_ALLOCATION_STRING_FORMAT,
			S3_MAX_PHYSICAL_X_PIXELS(),
			number_of_cursors * lines,
			S3_SCREEN_DEPTH(), 0, start_y, 0 );

	string_p = allocate_memory(strlen(tmp_buf) + 1);

	(void) strcpy (string_p, tmp_buf);

#if defined(__DEBUG__)
	if (s3_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"(s3_cursor_make_named_allocation_string){\n"
			"\topt_string=%s\n"
			"}\n", string_p);
	}
#endif

	return (string_p);
}
