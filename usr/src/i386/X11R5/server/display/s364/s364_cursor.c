/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_cursor.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_cursor.c : Cursor management module
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s364_cursor.h"
 ***
 ***	DESCRIPTION
 ***
 ***			SIBool
 ***			s364_cursor_download ( SIint32 cursor_index,
 ***									SICursorP cursor_p )
 ***			SIBool
 ***			s364_cursor_turnon ( SIint32 cursor_index )
 ***			SIBool
 ***			s364_cursor_turnoff ( SIint32 cursor_index )
 ***			SIBool
 ***			s364_cursor_move ( SIint32 cursor_index, SIint32 x, SIint32 y )
 ***
 ***		This module defines the above said cursor SI entry points.
 ***	The entry points will be different for each type of cursor that
 ***	is supported by the SDD. Software cursor (used only by R4 server),
 ***	dac cursor and the chipset cursor (for Vision 864 based boards) are
 ***	the cursor types supported.
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***		s364_cursor.c : Source.
 ***		s364_cursor.h : Interface.
 ***
 ***	SEE ALSO
 ***
 ***  		SI definitions document
 ***		s364_mischw.c
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

enum s364_cursor_status_kind
{
	CURSOR_ON,
	CURSOR_OFF
};


struct s364_cursor_status
{
	enum s364_cursor_status_kind flag ;	/*is cursor on/off*/
	int	current_index ;		/*index of the current cursor (cursor_index)*/
	int	x;							/*X co-ord of the cursor*/
	int	y;							/*Y co-ord of the cursor*/
#if defined ( __DEBUG__ )
	int stamp ;
#endif
};

/*
 * chipset cursor specfic fields
 */
struct s364_chipset_cursor
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


struct s364_cursor
{
	int	cursor_width ;				
	int cursor_height; 

	int	foreground_color ;
	int	background_color ;

	/* Pointers to various cursor operation functions for this cursor*/

	SIBool (*move_function_p)    (SIint32, SIint32, SIint32);
	SIBool (*turnon_function_p)  (SIint32) ;
	SIBool (*turnoff_function_p) (SIint32) ;

	struct s364_chipset_cursor *chipset_cursor_p;

	/*
	 * Bitmaps for the current cursor
	 */

	SIbitmapP source_p;
	SIbitmapP inverse_source_p;
	SIbitmapP mask_p;

	/*
	 * Area to save the screen that's obscured by the cursor.
	 */ 
	SIbitmapP save_area_p;

#if defined(__DEBUG__)
	int stamp ;
#endif

};

struct  s364_dac_cursor
{

	int	cursor_width;				
	int cursor_height; 

	int	foreground_color;
	int	background_color;

	unsigned char *cursor_bits_p;

};


/*
** Module state
*/

struct s364_cursor_state
{

	int cursor_max_width;
	int cursor_max_height;

	/*No of downloadable cursors*/
	int number_of_cursors ;			

	/*Status of the current cursor */
	struct s364_cursor_status current_cursor_status; 


	/* Pointer to cursors list: Used only by software cursors */
	struct s364_cursor **cursor_list_pp;

	/* Used if cursor type is dac cursor */

	struct s364_dac_cursor *dac_cursor_p;


	/*
     * The co-ords of the visible window (displayed area) with 
	 * respect to the virtual screen.
     */

    int  visible_window_top_x;
    int  visible_window_top_y;	

#if defined ( __DEBUG__)
	int stamp ;
#endif

};

#if (defined(__DEBUG__))
export boolean s364_cursor_debug = 0;
#endif

PRIVATE

/*
 * Includes
 */

#include "generic.h"


#include "s364_asm.h"
#include "g_omm.h"
#include "s364_regs.h"
#include "s364_gbls.h"
#include "s364_state.h"
#include "s364_opt.h"

#include "s364_gs.h"
#include "s364_mischw.h"


/***
 ***	Macros.
 ***/

#define S364_CURSOR_STPLBLT_TRANSFER_THRO_PIXTRANS_WINDOW(COUNT, SRC_P)\
{                                                                           \
    register int tmp = (COUNT);                                             \
    register unsigned long *src_p = (SRC_P);                                \
    register unsigned long *dst_p =                                         \
        (unsigned long *)(register_base_address_p);                         \
    ASSERT((COUNT) <= S3_PIXTRANS_MMAP_WINDOW_SIZE_IN_LONG_WORDS);          \
    do                                                                      \
    {                                                                       \
        *dst_p++ = *src_p++;                                                \
    } while ( --tmp > 0);                                                   \
}


#define S364_CURSOR_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('6' << 3) + ('4' << 4) +\
 	('_' << 5) + ('C' << 6) + ('U' << 7) + ('R' << 8) + ('S' << 9) +\
	('O' << 10) + ('R' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) +\
 	('A' << 15) + ('M' << 16) + ('P' << 17) + 0 )

#define S364_CHIPSET_CURSOR_STAMP\
	(('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) +\
	('C' << 5) + ('H' << 6) + ('I' << 7) + ('P' << 8) + ('S' << 9) +\
	('E' << 10) + ('T' << 11) + ('_' << 12) + ('C' << 13) + ('U' << 14) +\
	('R' << 15) + ('S' << 16) + ('_' << 17) + ('S' << 18) + ('T' << 19) +\
	('A' << 20) + ('M' << 21) + ('P' << 22) + 0 )

#define S364_CURSOR_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('6' << 3) + ('4' << 4) +\
	('_' << 5) + ('C' << 6) + ('U' << 7) + ('R' << 8) + ('S' << 9) +\
	('O' << 10) + ('R' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) +\
	('A' << 15) + ('T' << 16) + ('E' << 17) + ('_' << 18) + ('S' << 19) +\
	('T' << 20) + ('A' << 21) + ('M' << 22) + ('P' << 23) + 0 )

#define S364_CURSOR_NAMED_ALLOCATION_ID   "CURSOR"

#define S364_CURSOR_NAMED_ALLOCATION_STRING_FORMAT\
        S364_CURSOR_NAMED_ALLOCATION_ID ":%d+%d+%d@%d+%d+%d"

#define S364_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN    200

/*
** Each pixel of the chipset cursor is represented by two bits
*/
#define S364_CHIPSET_CURSOR_DEPTH    2

#define S364_CHIPSET_CURSOR_MAX_WIDTH()\
    (cursor_state_p->cursor_max_width)

#define S364_CHIPSET_CURSOR_MAX_HEIGHT()\
    (cursor_state_p->cursor_max_height)

#define S364_MAX_DISPLAYED_X_PIXELS() \
	(screen_state_p->generic_state.screen_displayed_width) 

#define S364_MAX_DISPLAYED_Y_PIXELS()\
	(screen_state_p->generic_state.screen_displayed_height) 

#define S364_MAX_VIRTUAL_X_PIXELS() \
	(screen_state_p->generic_state.screen_virtual_width) 

#define S364_MAX_VIRTUAL_Y_PIXELS()\
	(screen_state_p->generic_state.screen_virtual_height) 

#define S364_MAX_PHYSICAL_X_PIXELS() \
	(screen_state_p->generic_state.screen_physical_width) 

#define S364_MAX_PHYSICAL_Y_PIXELS()\
	(screen_state_p->generic_state.screen_physical_height) 

#define S364_SCREEN_DEPTH()\
	(screen_state_p->generic_state.screen_depth) 


#define	S364_CURSOR_STATE_DECLARE()\
	struct s364_cursor_state  *cursor_state_p=\
	screen_state_p->cursor_state_p


#define	S364_CURSOR_IS_VALID_INDEX(cursor_index)\
	(cursor_index >= 0) &&\
	(cursor_index<cursor_state_p->number_of_cursors)



/*
 *
 * PURPOSE
 *
 * Function for nullpadding the undefined bits in the cursor source
 * and inverse source bitmaps.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
s364_cursor_clear_extra_bits_and_copy(SICursorP si_cursor_p,
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
 * Software cursor functions
 */

/*
 * s364_cursor_create_cursor
 *
 * PURPOSE
 *
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 */

STATIC int 
s364_cursor_create_cursor(struct s364_cursor *active_cursor_p,
	SICursor *cursor_p )
{

	int height;
	int	width;
	int total_bytes;
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

#if (defined( __DEBUG__ ))
	if(s364_cursor_debug)
	{
		(void)fprintf( debug_stream_p,
				"(s364_cursor_create_cursor){\n"
				"\tactive_cursor_p= %p\n"
				"cursor_p= %p\n"
				"}\n",
				(void*)active_cursor_p,
				(void*)cursor_p);
	}
#endif

	ASSERT((active_cursor_p != NULL)&&( cursor_p != NULL));
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,active_cursor_p));

	width = cursor_p->SCwidth;
	height = cursor_p->SCheight;
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

	(void) memcpy(active_cursor_p->source_p,
		cursor_p->SCsrc, sizeof(SIbitmap));
	(void) memcpy(active_cursor_p->inverse_source_p,
		cursor_p->SCinvsrc, sizeof(SIbitmap));
	(void) memcpy(active_cursor_p->mask_p,
		cursor_p->SCmask, sizeof(SIbitmap));

	/*
	 * Allocate temporary space for bitmaps	
	 */

	active_cursor_p->source_p->Bptr = allocate_memory(total_bytes);
	active_cursor_p->inverse_source_p->Bptr = allocate_memory(total_bytes);
	active_cursor_p->mask_p->Bptr = allocate_memory(total_bytes);
	
	/*
	 * NOTE : mask_bitmap is not being used in this function
	 */

	s364_cursor_clear_extra_bits_and_copy(cursor_p,
		active_cursor_p->source_p,
		active_cursor_p->inverse_source_p,
		active_cursor_p->mask_p);

    /*
     * Initialize the bitmap to store the screen area
     * obscured by the software cursor.
     */
	/*
	 * Free anything that's allocated previously.
	 */
	if( active_cursor_p->save_area_p != NULL)
	{
		if(active_cursor_p->save_area_p->Bptr != NULL)
		{
			free_memory(active_cursor_p->save_area_p->Bptr);
		}
		free_memory(active_cursor_p->save_area_p);
	}

    active_cursor_p->save_area_p =
        allocate_memory(sizeof(SIbitmap));

	if( active_cursor_p->save_area_p == NULL)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}

    active_cursor_p->save_area_p->Bwidth =
        cursor_state_p->cursor_max_width;
    active_cursor_p->save_area_p->Bheight =
        cursor_state_p->cursor_max_height;
    active_cursor_p->save_area_p->BbitsPerPixel =
        screen_state_p->generic_state.screen_depth;

    active_cursor_p->save_area_p->Bptr = allocate_memory(
		((cursor_state_p->cursor_max_width *
        screen_state_p->generic_state.screen_depth) >> 3) *
        cursor_state_p->cursor_max_height);
	
	if( active_cursor_p->save_area_p->Bptr == NULL)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}

	return 1;
}

/*
 * s364_cursor_save_obscured_screen 
 *
 * PURPOSE
 *
 *	Saves the rectangular area of the screen that is to be obscured by
 * the screen in the data structure before drawing the cursor in that
 * screen area.
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 *
 */

STATIC int
s364_cursor_save_obscured_screen(int cursor_index)  
{
	int x1;
	int y1;
	int x2;
	int y2;
	int width;
	int height;
	SIbitmapP save_area_p;
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	S364_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	struct s364_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
					  "(s364_cursor_save_obscured_screen){"
					  "\n"
					  "\tcursor_index= %d\n}\n",
					  cursor_index);
	}
#endif

	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,active_cursor_p));

	save_area_p = active_cursor_p->save_area_p;

	/*
	 * determine the co-ordinates of the rectangular area that
	 * has to be saved: (x1,y1,x2,y2)
	 */

	x1 = cursor_state_p->current_cursor_status.x;
	y1 = cursor_state_p->current_cursor_status.y;
	x2 = cursor_state_p->current_cursor_status.x +
		active_cursor_p->cursor_width;
	y2 = cursor_state_p->current_cursor_status.y +
		active_cursor_p->cursor_height;

	/*
	 * Check if it is outside the visible area and adjust co-ords
	 * if required
	 */
	 /*
	  *NOTE: x2 and y2 are exclusive
	  */

	 x2 = (x2 > S364_MAX_VIRTUAL_X_PIXELS() + 1) ?
			 (S364_MAX_VIRTUAL_X_PIXELS() + 1) : x2;
	 y2 = (y2 > S364_MAX_VIRTUAL_Y_PIXELS() + 1) ?
			 (S364_MAX_VIRTUAL_Y_PIXELS() + 1) : y2;

	if (x1 < 0)
	{
		x1 = 0;
	}
	
	if (y1 < 0)
	{
		y1 = 0; 
	}

	/*
	 * Calculate width and height of the rectangular save area
	 */

	width = x2 - x1;
	height = y2 - y1;

	save_area_p->Bwidth = width;
	save_area_p->Bheight = height;

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s364_cursor_save_obscured_screen){\n"
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
	
	ASSERT((width > 0) && (height > 0));
    ASSERT((x1 >= 0) && (y1 >= 0));

	if (save_area_p->Bptr)
	{
        int     		stride = screen_state_p->framebuffer_stride;
        int     		depth_shift = screen_state_p->generic_state.
							screen_depth_shift;
        unsigned long   *tmp_frame_buffer_p;
        unsigned int    source_offset;
        unsigned int    destination_step;
            				
							
		destination_step = (((unsigned)(save_area_p->Bwidth 
							<< depth_shift) >> 3U)+ 3) & ~3;

        
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
     	 * Wait for the GE to be idle before switching to LFB.
     	 */
    	S3_WAIT_FOR_GE_IDLE();
    	S3_DISABLE_MMAP_REGISTERS();	

        /*
         * Copy the screen area into a bitmap.
         * The screen area will be saved at the offset (0,0)
         * of the bitmap.
         */
        screen_state_p->transfer_pixels_p(tmp_frame_buffer_p,
            (unsigned long *)(save_area_p->Bptr),
            source_offset , 0,
            stride, destination_step , width, height,
            save_area_p->BbitsPerPixel,
            GXcopy,
			(~0U >> (32U - screen_state_p->generic_state.screen_depth)),
            screen_state_p->pixels_per_long_shift, (-1));

		S3_ENABLE_MMAP_REGISTERS();

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
 * s364_cursor_restore_obscured_screen 
 *
 * PURPOSE
 *
 *	Restore the screen area saved when the cursor goes away from that 
 * area.
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 *
 */

STATIC int
s364_cursor_restore_obscured_screen (int cursor_index)
{
	int	x;
	int	y;
	int width;
	int	height;
	int source_step;
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	int depth_shift = screen_state_p->generic_state.screen_depth_shift;
	struct s364_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s364_cursor_restore_obscured_screen){\n"
			"\tcursor_index= %d\n"
			"}\n",
			cursor_index);
	 }
#endif

	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,active_cursor_p));

	width = active_cursor_p->save_area_p->Bwidth; 
	height = active_cursor_p->save_area_p->Bheight; 

	x = cursor_state_p->current_cursor_status.x;
	y = cursor_state_p->current_cursor_status.y;

	if ( x < 0)
	{
		x = 0;
	}

	if (y < 0)
	{
		y = 0;
	}	

	source_step = ((unsigned)( (width + ((32 >> depth_shift) - 1)) &
        			~((32 >> depth_shift) - 1)) 
					<< depth_shift) >> 5U;

	S3_REGISTER_SET_SCISSOR_REGISTERS(x, y, x+width-1, y+height-1);
	
	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;

	if(active_cursor_p->save_area_p->Bptr)
	{
		s364_asm_transfer_pixels_through_pixtrans_helper(
                    (unsigned long *)active_cursor_p->save_area_p->Bptr,
                    x,
                    y,
                    source_step,
                    width,
                    height,
                    active_cursor_p->save_area_p->BbitsPerPixel,
                    S3_MIX_FN_N,
					(~0U >> (32U - screen_state_p->generic_state.
							screen_depth)),
                    S3_CMD_PX_MD_THRO_PLANE,
					(-1));

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
 * 
 * PURPOSE
 *
 *	This routine does memory to screen stippling of the cursor pattern.
 * Any arbitrary pixel offset into the cursor bitmap can be given in 
 * order to draw partial cursor when moved to the edges.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
s364_cursor_stplblt(SIbitmapP source_bitmap_p, 
	SIint32 source_x, SIint32 source_y,
	SIint32 destination_x, SIint32 destination_y,
	SIint32 width, SIint32 height)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long		*source_bits_p;
	unsigned int		source_step;
	unsigned int		number_of_pixtrans_words_per_width;
	int					delta;
	const unsigned char *byte_invert_table_p = 
		screen_state_p->byte_invert_table_p;
	unsigned char 		*inverted_bytes_p =
		screen_state_p->generic_state.screen_scanline_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	
#if (defined(__DEBUG__))
	if (s364_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_cursor_stplblt)\n"
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

	/*
	 * Set only the top and bottom scissors to the virtual, if required.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_WAIT_FOR_FIFO(2);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_T_INDEX | 0x0000,
			unsigned short);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_B_INDEX |
			((screen_state_p->generic_state.screen_virtual_height - 1) & 
			S3_MULTIFUNC_VALUE_BITS),unsigned short);
	}

	S3_WAIT_FOR_FIFO(5);

	/*
	 * Set the left and the right ends of the clipping rectangle.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(destination_x & S3_MULTIFUNC_VALUE_BITS)), unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		((destination_x + width -1) & S3_MULTIFUNC_VALUE_BITS)), 
		unsigned short);
	
	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;

	/*
	 * The following registers are already programmed at this point.
	 * WRITE_MASK (programmed in select state.)
	 * FG_COLOR (programmed in draw_cursor apropriately)
	 */

	/*
	 * Set the pixel control register to use cpu data for selecting
	 * the foreground and background mix registers.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_CPU_DATA, unsigned short);

	/*
	 * Rop is GXcopy, always in our case.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		(S3_MIX_FN_N | S3_CLR_SRC_FRGD_COLOR), unsigned short);

	/*
	 * For transparent case program BG_ROP to be DST.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
		S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);

	/*
	 * Align the source to the previous long word boundary.
	 * Adjust the destination and the width appropriately.
	 * The pixtrans register has a width of 32.
	 */
	if ((delta = (source_x & 31)) != 0)
	{
		source_x &= ~31;
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
	 * pixtrans words. Also compute the number of pixtrans long 
	 * words per width.
	 */
	width =	(width + 31) & ~31;
	number_of_pixtrans_words_per_width = (unsigned)width >> 5U;

#if (defined(__DEBUG__))
	if (s364_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_cursor_stplblt)\n"
			"{\n"
			"\tsource_bitmap_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\trounded width = %ld\n"
			"\tnumber of pixtrans words per width = %ld\n"
			"}\n",
			(void *) source_bitmap_p, source_x, source_y,
			destination_x, destination_y, width,
			number_of_pixtrans_words_per_width);
	}
#endif

	ASSERT((width > 0) && (height > 0));

	S3_WAIT_FOR_FIFO(5);

	/*
	 * Set up the GE registers for the memory to screen transfers.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
		(unsigned short)destination_x, unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
		(unsigned short)destination_y, unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(unsigned short)(width - 1) , unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);

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
		S3_CMD_LSB_FIRST |
		S3_CMD_TYPE_RECTFILL,
		unsigned short);

	/*
	 * Wait for the initiator to get executed.
	 */
	S3_WAIT_FOR_ALL_FIFO_FREE();

	/*
	 * Start pumping host data to the pixtrans.
	 */
	do
	{
		register int i = 0;
		register unsigned char *tmp_source_p = (unsigned char *)source_bits_p;
		register unsigned char *tmp_inverted_bytes_p = inverted_bytes_p;

		ASSERT(!(width & 31));

		/*
		 * Invert all bytes in each line before pumping
		 */
		do
		{
			*tmp_inverted_bytes_p++ = *(byte_invert_table_p + *tmp_source_p++);
		} while ( ++i < ((unsigned)width >> 3U));

		S364_CURSOR_STPLBLT_TRANSFER_THRO_PIXTRANS_WINDOW(
			number_of_pixtrans_words_per_width, 
			(unsigned long *)inverted_bytes_p);

		source_bits_p += source_step;

	} while(--height > 0);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

#if (defined(__DEBUG__))	
	S3_WAIT_FOR_GE_IDLE();
#endif
	/*
	 * Restore the pixel control.
	 */
	S3_WAIT_FOR_FIFO(1);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);

	return;
}

/*
 *
 * PURPOSE
 *
 *	Stipples the source and mask cursor bitmaps one over the other
 * from the memory to the specified position in the screen.
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 *
 */

STATIC int
s364_cursor_draw_cursor(int cursor_index)
{
	int x;
	int y;
	int source_x;
	int source_y;
	int width;
	int height;
	struct s364_cursor *active_cursor_p;
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();


	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,active_cursor_p));

	width = active_cursor_p->cursor_width; 
	height = active_cursor_p->cursor_height; 

	x = cursor_state_p->current_cursor_status.x;
	y = cursor_state_p->current_cursor_status.y;

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

	if ((x + width) >= S364_MAX_DISPLAYED_X_PIXELS())
	{
		width -= 
			(x + width -  S364_MAX_DISPLAYED_X_PIXELS());
	}
	
	if ((y + height) >= S364_MAX_DISPLAYED_Y_PIXELS())
	{
		height -= 
			(y + height - S364_MAX_DISPLAYED_Y_PIXELS());
	}

	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
		active_cursor_p->foreground_color, unsigned long);

	s364_cursor_stplblt(active_cursor_p->source_p, source_x, source_y, x, y, 
						width, height);
		
	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
		active_cursor_p->background_color, unsigned long);

	s364_cursor_stplblt(active_cursor_p->inverse_source_p, source_x, 
		source_y, x, y, width, height);
					
	/*
	 * Set the foreground color to the graphics state value. 
	 */
	S3_WAIT_FOR_FIFO(1);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
		enhanced_cmds_p->frgd_color, unsigned long);

	return 1;

}

/*
 * s364_cursor_turnon - SI entry point.
 *
 * PURPOSE
 *
 *	The software cursor on the screen is turned on by saving the 
 * screen are to be obscured and then drawing the cursor pattern.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_cursor_turnon(SIint32 cursor_index)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct s364_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
			"(s364_cursor_turnon){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif

	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,active_cursor_p));
#endif

	if (cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		return SI_SUCCEED;
	}

	s364_cursor_save_obscured_screen(cursor_index);

	s364_cursor_draw_cursor(cursor_index);


	cursor_state_p->current_cursor_status.flag = CURSOR_ON;
	cursor_state_p->current_cursor_status.current_index = cursor_index;

	return SI_SUCCEED;
}


/*
 * s364_cursor_turnoff - SI entry point
 *
 * PURPOSE
 *
 *  The cursor is turned off by redrawing the cursor area that was 
 * obscured by the cursor.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_cursor_turnoff(SIint32 cursor_index)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct s364_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
				"(s364_cursor_turnoff){\n"
				"\tcursor_index= %d\n}\n",
				(int)cursor_index);
	 }
#endif

	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,active_cursor_p));
#endif

	if (cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		return SI_SUCCEED;
	}
	else
	{
		cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
	}

 	s364_cursor_restore_obscured_screen(cursor_index);
	
	return SI_SUCCEED;
}


/*
 * s364_cursor_move - SI entry point
 *
 * PURPOSE
 *
 *	Software cursor move function. The new cursor position is updated
 * and turn on is called which in turn draws the cursor in the new
 * position. If the status is CURSOR OFF, then the cursor in not drawn.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	struct s364_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
				"(s364_cursor_move){\n"
				"\tcursor_index= %d\n"
				"\tx = %d\n"
				"\ty= %d\n}\n",
				(int)cursor_index,
				(int)x, (int)y);
	 }
#endif

		
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,active_cursor_p));

	if (cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		cursor_state_p->current_cursor_status.x = x;
		cursor_state_p->current_cursor_status.y = y;
		return SI_SUCCEED;
	}


	(*active_cursor_p->turnoff_function_p)( cursor_index);

	cursor_state_p->current_cursor_status.x = x;
	cursor_state_p->current_cursor_status.y = y;
	cursor_state_p->current_cursor_status.current_index = cursor_index;

	return (*active_cursor_p->turnon_function_p)(cursor_index);
} 

/*
 * s364_cursor_download - SI entry point
 *
 * PURPOSE
 *
 * Software cursor downloaded into the cursor data structures
 * after clearing unnecessary bits in the SI cursor bitmap and null
 * padding.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	struct s364_cursor  *active_cursor_p;
	

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
				"(s364_cursor_download){\n"
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

	ASSERT ( S364_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR, active_cursor_p));

	/*
	 * If the cursor being downloaded is currently in use and turned ON
	 * then turn it OFF, download the new cursor and then turn it ON
	 */

	if (cursor_state_p->current_cursor_status.current_index ==
		 cursor_index 
		 && cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		(*active_cursor_p->turnoff_function_p)(cursor_index);

		if (s364_cursor_create_cursor(active_cursor_p,cursor_p)
			 == 0 )
		{
			return SI_FAIL;
		}
		return (*active_cursor_p->turnon_function_p)(cursor_index);
	}
	else
	{
		return (
			(s364_cursor_create_cursor(active_cursor_p,cursor_p) ==
			 0) ? SI_FAIL : SI_SUCCEED);
	}
}


/*********************************************************************
 *
 * CHIPSET cursor routines.
 *
 *********************************************************************/

/*
 * s364_cursor_chipset_cursor_download_helper 
 *
 * PURPOSE
 *
 *	Does memory to screen bitblt - This is used for downloading the
 * cursor pattern in the offscreen memory. 
 *
 * RETURN VALUE
 *
 *		None.
 */ 
STATIC void
s364_cursor_chipset_cursor_download_helper(
	 unsigned char *cursor_bitmap_p,
	 int destination_x, int destination_y)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
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
	 *
	 * Of these, WRT_MASK, FOREGROUND_COLOR and BACKGROUND_COLOR 
	 * are already programmed.
	 * And FRGD_MIX is selected as the color source, by default.
	 */ 	
	
	ASSERT(!S3_IS_FIFO_OVERFLOW());

	S364_STATE_SET_CLIP_RECTANGLE_TO_VIDEO_MEMORY();

	S3_WAIT_FOR_FIFO(2);

    S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
		(~0U >> (32U - screen_state_p->generic_state.screen_depth)),
		unsigned short);

    S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		(S3_MIX_FN_N | S3_CLR_SRC_CPU_DATA),
        unsigned short);

	total_pixels = 
		((S364_CHIPSET_CURSOR_MAX_WIDTH()*S364_CHIPSET_CURSOR_DEPTH) /
			S364_SCREEN_DEPTH()) * S364_CHIPSET_CURSOR_MAX_HEIGHT();

	pixtrans_words_count =
	  ((total_pixels * S364_SCREEN_DEPTH()) / 32);

	/*
	 * Add the offset in the x direction, till we reach a 1K boundary.
	 * This is required since we are blitting a rectangle.
	 */

	total_pixels += destination_x;

	transfer_height = total_pixels / S364_MAX_PHYSICAL_X_PIXELS();

	/*
	 * In some cases the cursor might take some full
	 * lines following by an incomplete line, in such  cases
	 * we have to transfer dummy pixels to fill the rest of the
	 * line.
	 */
	if (total_pixels % S364_MAX_PHYSICAL_X_PIXELS())
	{
		dummy_pixels_count = 
			S364_MAX_PHYSICAL_X_PIXELS() -
			(total_pixels % S364_MAX_PHYSICAL_X_PIXELS());
		++transfer_height;
	}
	else
	{
		dummy_pixels_count = 0;
	}

	if(total_pixels > S364_MAX_PHYSICAL_X_PIXELS())
	{
		transfer_width = S364_MAX_PHYSICAL_X_PIXELS();
	}
	else
	{
		transfer_width = total_pixels;
		transfer_height = 1;
	}


#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
		"s364_cursor_chipset_cursor_download_helper\n{\n"
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
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
		 0, unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
		 destination_y, unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		 (transfer_width - 1), unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
		((transfer_height - 1) & S3_MULTIFUNC_VALUE_BITS),
		unsigned short);
		
	
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		(S3_CMD_LSB_FIRST |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_WAIT_YES | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_THRO_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM),
		unsigned short);
	
	ASSERT(pixtrans_words_count > 0);

		
	S3_WAIT_FOR_ALL_FIFO_FREE();

	/*
	 * Put dummy pixels in the given line 'destination_y' till
	 * 'destination_x' which is a 1K boundary is reached.
	 */
	if (destination_x > 0)
	{
		register int i ;
		register unsigned long *dst_p = 
				(unsigned long *)(register_base_address_p);

	  	i = ((destination_x * S364_SCREEN_DEPTH()) / 32);

		do 
		{
			*dst_p++ = 0;
		} while(--i > 0);
	}

	/*
	 * Pump the actual cursor pattern.
	 */
	{
		register int i = pixtrans_words_count;
		register unsigned long *src_p = (unsigned long *)(cursor_bitmap_p);
		register unsigned long *dst_p = 
				(unsigned long *)(register_base_address_p);

		do 
		{
			*dst_p++ = *src_p++;
		} while(--i > 0);
	}

	if (dummy_pixels_count)
	{
		register int i ;
		register unsigned long *dst_p = 
				(unsigned long *)(register_base_address_p);

	  	i = ((dummy_pixels_count * S364_SCREEN_DEPTH()) / 32);

		do 
		{
			*dst_p++ = 0;
		} while(--i > 0);
	}

 	S3_WAIT_FOR_FIFO(1);
    S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
        enhanced_cmds_p->write_mask, unsigned long);
	
	ASSERT(!S3_IS_FIFO_OVERFLOW());
}

/*
 * s364_cursor_generate_chipset_cursor
 *
 * PURPOSE
 *
 * 	Routine to convert the cursor pattern from the SI's format to
 * the one required by the chipset.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_cursor_generate_chipset_cursor( SIbitmapP  si_mask_p,
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
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	const unsigned char *tmp_byte_invert_table_p  = 
		screen_state_p->byte_invert_table_p;
	

	ASSERT(si_mask_p && si_source_p && cursor_bitmap_p );
	ASSERT(si_mask_p->Bwidth == si_source_p->Bwidth);
	ASSERT(si_mask_p->Bheight == si_source_p->Bheight);
	ASSERT(si_mask_p->BbitsPerPixel == 1);
	
	/*
	 * if height/width is more than 64 pixels truncate it
	 */
	if (si_mask_p->Bwidth > S364_CHIPSET_CURSOR_MAX_WIDTH())
	{
		effective_cursor_width = S364_CHIPSET_CURSOR_MAX_WIDTH();
	}
	else
	{
		effective_cursor_width = si_mask_p->Bwidth;
	}

	if (si_mask_p->Bheight > S364_CHIPSET_CURSOR_MAX_HEIGHT())
	{
		effective_cursor_height = S364_CHIPSET_CURSOR_MAX_HEIGHT();
	}
	else
	{
		effective_cursor_height = si_mask_p->Bheight;
	}
	/*
	 * Alternating 'xor' and 'and' words
	 */
	destination_step = (S364_CHIPSET_CURSOR_MAX_WIDTH() / 8) * 2;  /*bytes*/

	source_step = ((si_mask_p->Bwidth + 31) & ~31) >> 3;		/*bytes*/

	/*
	 * Effective number of short words per line
	 */
	effective_no_of_words_per_line =
		 ((effective_cursor_width + 31) & ~31) / 16;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf (debug_stream_p,
		"s364_cursor_generate_chipset_cursor)\n{\n"
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
 * s364_cursor_create_chipset_cursor
 *
 * PURPOSE
 *
 * 	Process the SI cursor bitmap and update the data struture.
 * The cursor bitmap is also downloaded in the offscreen memory.
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 *
 */
STATIC int 
s364_cursor_create_chipset_cursor(struct s364_cursor *active_cursor_p,
					 SICursor *si_cursor_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

	int height ;
	int	width ;
	int	bitmap_size;
	int total_bytes;
	unsigned char *cursor_bitmap_p;
	SIbitmap src_bitmap;
	SIbitmap invsrc_bitmap;
	SIbitmap mask_bitmap;


#if defined ( __DEBUG__ )
	if ( s364_cursor_debug )
	{
		(void)fprintf	( debug_stream_p,
"(s364_cursor_create_chipset_cursor){\n"
"\tactive_cursor_p= %p\n"
"\tsi_cursor_p= %p}\n",
		    (void*)active_cursor_p,
		    (void*)si_cursor_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR_STATE, cursor_state_p));
	ASSERT((active_cursor_p != NULL) && ( si_cursor_p != NULL ));
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR, active_cursor_p));

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
	s364_cursor_clear_extra_bits_and_copy(si_cursor_p,
		&src_bitmap,&invsrc_bitmap,
		&mask_bitmap);

	/*
	 * Hardware cursor is 2 bit deep
	 */
	bitmap_size = ((S364_CHIPSET_CURSOR_MAX_WIDTH() *
		 S364_CHIPSET_CURSOR_MAX_HEIGHT()) / 8) * 2; /*bytes*/
	cursor_bitmap_p =allocate_memory(bitmap_size); 
	/*
	** Set all pixels to transparent
	*/
	(void)memset(cursor_bitmap_p,0,bitmap_size);
		

	s364_cursor_generate_chipset_cursor(&mask_bitmap, 
		 &src_bitmap,
		 cursor_bitmap_p);

	free_memory(mask_bitmap.Bptr);
	free_memory(src_bitmap.Bptr);
	free_memory(invsrc_bitmap.Bptr);

	active_cursor_p->cursor_height = 
	    ((height > S364_CHIPSET_CURSOR_MAX_HEIGHT() ) ?
		 S364_CHIPSET_CURSOR_MAX_HEIGHT() : height);

	active_cursor_p->cursor_width = 
	    ((width > S364_CHIPSET_CURSOR_MAX_WIDTH() ) ?
		 S364_CHIPSET_CURSOR_MAX_WIDTH() : width);

	active_cursor_p->foreground_color =
		si_cursor_p->SCfg;

	active_cursor_p->background_color =
		si_cursor_p->SCbg;

	active_cursor_p->chipset_cursor_p->horz_cursor_offset = 0;
	active_cursor_p->chipset_cursor_p->vert_cursor_offset = 0;

	s364_cursor_chipset_cursor_download_helper(cursor_bitmap_p, 
		active_cursor_p->chipset_cursor_p->offscreen_x_location,
		active_cursor_p->chipset_cursor_p->offscreen_y_location);

	free_memory(cursor_bitmap_p) ;

	return 1;
}


/*
 * s364_cursor_draw_chipset_cursor
 *
 * PURPOSE
 *
 *	The cursor registers in the chipset are programmed and the cursor
 * is enabled. This draws the cursor pattern on the screen.
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 *
 */
STATIC int
s364_cursor_draw_chipset_cursor(int	cursor_index)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	struct s364_cursor *active_cursor_p ;
	int	start_addr;
	int	x;
	int	y;
	

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR_STATE, cursor_state_p));
	
	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));

	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR, active_cursor_p));


	S364_DISABLE_CHIPSET_CURSOR(screen_state_p);

	S364_SET_CHIPSET_CURSOR_COLORS(screen_state_p,
	    active_cursor_p->foreground_color,
		active_cursor_p->background_color);


	start_addr =
		((active_cursor_p->chipset_cursor_p->offscreen_y_location * 
		((S364_MAX_PHYSICAL_X_PIXELS() * S364_SCREEN_DEPTH()) / 8)) +
		active_cursor_p->chipset_cursor_p->offscreen_x_location)
		/ 1024;

	S364_SET_CHIPSET_CURSOR_STORAGE_START_ADDRESS(screen_state_p,
		 (unsigned short)start_addr);

	

	/*
	 * Take care of -ve x,y positions
	 */
	x = cursor_state_p->current_cursor_status.x;
	y = cursor_state_p->current_cursor_status.y;

	if ( x < 0)
	{
		active_cursor_p->chipset_cursor_p->horz_cursor_offset  = (-x) & 0xFE;
		S364_SET_CHIPSET_CURSOR_HORZ_DISPLAY_OFFSET(screen_state_p,
	  		active_cursor_p->chipset_cursor_p->horz_cursor_offset);
		x = 0;
	}

	if ( y < 0)
	{
		active_cursor_p->chipset_cursor_p->vert_cursor_offset  = (-y) & 0xFE;
		S364_SET_CHIPSET_CURSOR_VERT_DISPLAY_OFFSET(screen_state_p,
	  		active_cursor_p->chipset_cursor_p->vert_cursor_offset);
		y = 0;
	}
	S364_SET_CHIPSET_CURSOR_POSITION(screen_state_p,x,y);

	S364_ENABLE_CHIPSET_CURSOR(screen_state_p);
#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"(s364_cursor_draw_chipset_cursor){\n"
			"\t#Cursor has been drawn\n"
			"\tx = %d\n"
			"\ty = %d\n"
			"\thorz offset = %d\n"
			"\tvert offset = %d\n"
			"}\n",
			cursor_state_p->current_cursor_status.x,
			cursor_state_p->current_cursor_status.y,
	  		active_cursor_p->chipset_cursor_p->horz_cursor_offset,
	  		active_cursor_p->chipset_cursor_p->vert_cursor_offset);
	}
#endif
	return 1;
	
}

/*
 * Chipset cursor ENTRY points.
 */

/*
 * s364_chipset_cursor_download - SI entry point
 *
 * PURPOSE
 *
 *	Download the cursor to the offscreen memory after processing the
 * SI cursor bitmap.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_chipset_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE ();
	struct s364_cursor *active_cursor_p ;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
			"(s364_chipset_cursor_download){\n"
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
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR_STATE, cursor_state_p));
	
	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));

	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR, active_cursor_p));

	/*
	 * If the cursor being downloaded is the current cursor, turn it
	 * off, reload the bits.
	 */
	if (cursor_state_p->current_cursor_status.current_index ==
		cursor_index  && 
		cursor_state_p->current_cursor_status.flag == CURSOR_ON )
	{
		/*
		 * Turn off existing cursor.
		 */
		(*active_cursor_p->turnoff_function_p) ( cursor_index );

		/*
		 * Create the new cursor.
		 */
		if (s364_cursor_create_chipset_cursor(active_cursor_p,cursor_p) == 0)
		{
			return SI_FAIL ;
		}
		
		active_cursor_p =
			cursor_state_p->cursor_list_pp[cursor_index];

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
		return ((s364_cursor_create_chipset_cursor(active_cursor_p,cursor_p) 
				== 1) ? SI_SUCCEED : SI_FAIL);
	}
}


/*
 * s364_chipset_cursor_turnon   - SI entry point
 *
 * PURPOSE
 *
 *	Turn on the cursor by forcing a draw. If the cursor is OFF already,
 * nothing is done.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_chipset_cursor_turnon ( SIint32 cursor_index )
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE ();

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s364_chipset_cursor_turnon){\n"
			"\tcursor_index= %ld\n"
			"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR_STATE, cursor_state_p));
	
	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));

	if (cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
#if defined(__DEBUG__)
		if (s364_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
				"(s364_chipset_cursor_turnon){\n"
				"\t#Cursor already turned on\n"
				"\n}\n");
		}
#endif


		return SI_SUCCEED ;
	}

	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,
		 cursor_state_p->cursor_list_pp[cursor_index]));

#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif

	if (s364_cursor_draw_chipset_cursor(cursor_index) == 0)
	{
		return SI_FAIL ;
	}
	cursor_state_p->current_cursor_status.flag = CURSOR_ON ;

	cursor_state_p->current_cursor_status.current_index = cursor_index ;

	return SI_SUCCEED ;
}

/*
 * s364_chipset_cursor_turnoff - SI entry point
 *
 * PURPOSE
 *
 *	Turn off the cursor by setting the cursor disable bit in CR 45.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_chipset_cursor_turnoff ( SIint32 cursor_index )
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE ();

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
			"(s364_chipset_cursor_turnoff){\n"
			"\tcursor_index= %ld\n"
			"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR_STATE, cursor_state_p));
	
	ASSERT(S364_CURSOR_IS_VALID_INDEX(cursor_index));

	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR,
		cursor_state_p->cursor_list_pp[cursor_index]));

	cursor_state_p->current_cursor_status.flag = CURSOR_OFF ;
	/*
	 ** disable cursor 
	 */
	S364_DISABLE_CHIPSET_CURSOR(screen_state_p);
	
	return SI_SUCCEED ;
}

/*
 * s364_chipset_cursor_move - SI entry point
 *
 * PURPOSE
 *
 *	Update the cursor position registers. The cursor offset registers
 * are also updated in order to take care of the condition(partial 
 * visibility) when the cursor is moved to edges of the cursor.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_chipset_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	
	S364_CURSOR_STATE_DECLARE();
	struct s364_cursor *active_cursor_p;
	int	x_offset;
	int	y_offset;
	int	cursor_height;
	int	cursor_width;
	int	crtoffset_change_required;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"(s364_chipset_cursor_move){\n"
			"\tcursor index = %d\n"
			"\t			  x = %d\n"
			"\t		      y = %d\n"
			"}",
			cursor_index,
			x,
			y);
	}
#endif
	
	cursor_state_p->current_cursor_status.x = x;
	cursor_state_p->current_cursor_status.y = y;
	
	if ( cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		return SI_SUCCEED;
	}

	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR, active_cursor_p));


#ifdef PANNING
	/*
	 * Check if panning will be needed 
	 */
	if (screen_state_p->is_panning_to_virtual_screen_feasible == TRUE)
	{

		cursor_height = active_cursor_p->cursor_height;
		cursor_width  = active_cursor_p->cursor_width;

		if ( x + cursor_width > 
				cursor_state_p->visible_window_top_x +
					 S364_MAX_DISPLAYED_X_PIXELS())
		{
			cursor_state_p->visible_window_top_x = 
				(x + cursor_width > S364_MAX_VIRTUAL_X_PIXELS()) ?
					(S364_MAX_VIRTUAL_X_PIXELS() -
						 S364_MAX_DISPLAYED_X_PIXELS())	:
					(x + cursor_width - S364_MAX_DISPLAYED_X_PIXELS());
			x -=  cursor_state_p->visible_window_top_x;
			cursor_state_p->current_cursor_status.x = x ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (x < cursor_state_p->visible_window_top_x &&
					cursor_state_p->visible_window_top_x > 0)
			{
				if ( x >= 0)
				{
					cursor_state_p->visible_window_top_x = x;
					x = 0;
					cursor_state_p->current_cursor_status.x = x ;
				}
				else
				{
					x += cursor_state_p->visible_window_top_x; 
					cursor_state_p->current_cursor_status.x = x ;
					cursor_state_p->visible_window_top_x =0;
				}
					
				crtoffset_change_required = 1;
			}
			else
			{
				x -= cursor_state_p->visible_window_top_x; 
				cursor_state_p->current_cursor_status.x = x ;
			}
		}
		if ( y + cursor_height > 
				cursor_state_p->visible_window_top_y +
					 S364_MAX_DISPLAYED_Y_PIXELS())
		{
			cursor_state_p->visible_window_top_y = 
				(y + cursor_height > S364_MAX_VIRTUAL_Y_PIXELS()) ?
					(S364_MAX_VIRTUAL_Y_PIXELS() -
						 S364_MAX_DISPLAYED_Y_PIXELS())	:
					(y + cursor_height - S364_MAX_DISPLAYED_Y_PIXELS());

			y -=  cursor_state_p->visible_window_top_y;
			cursor_state_p->current_cursor_status.y = y ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (y < cursor_state_p->visible_window_top_y &&
					cursor_state_p->visible_window_top_y > 0)
			{

				if ( y >= 0)
				{
					cursor_state_p->visible_window_top_y = y;
					y = 0;
					cursor_state_p->current_cursor_status.y = y ;
				}
				else
				{
					y += cursor_state_p->visible_window_top_y; 
					cursor_state_p->current_cursor_status.y = y ;
					cursor_state_p->visible_window_top_y =0;
				}
				crtoffset_change_required = 1;
			}
			else
			{
				y -= cursor_state_p->visible_window_top_y; 
				cursor_state_p->current_cursor_status.y = y ;
			}
		}
		if (crtoffset_change_required == 1)
		{
			S364_SET_DISPLAY_START_ADDRESS(screen_state_p,
				cursor_state_p->visible_window_top_x,
				cursor_state_p->visible_window_top_y);
			

#if defined(__DEBUG__)
			if (s364_cursor_debug)
			{
				(void)fprintf(debug_stream_p,
					"(s364_chipset_cursor_move){\n"
					"\twindow_top_x = %d\n"
					"\twindow_top_y = %d\n"
					"}",
					cursor_state_p->visible_window_top_x,
					cursor_state_p->visible_window_top_y);
			}
#endif
						
		}

	}
#endif
	

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

#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif

	S364_SET_CHIPSET_CURSOR_HORZ_DISPLAY_OFFSET(screen_state_p,
		(unsigned short)x_offset);
	S364_SET_CHIPSET_CURSOR_VERT_DISPLAY_OFFSET(screen_state_p,
		(unsigned short)y_offset);
	S364_SET_CHIPSET_CURSOR_POSITION(screen_state_p,(unsigned short)x,
		(unsigned short)y);
	
	active_cursor_p->chipset_cursor_p->horz_cursor_offset = x_offset;
	active_cursor_p->chipset_cursor_p->vert_cursor_offset = y_offset;
	return SI_SUCCEED ;

}

/*
 *
 * PURPOSE
 *
 *	Patch in the cursor entry points in the private data structure for
 * the number of downloadable cursors. Initialization of the cursor
 * data structures is also done here.
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 *
 */

STATIC int
s364_cursor_initialize_cursors(int no_of_cursors)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	int i ;
	struct s364_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR_STATE, cursor_state_p)) ;

	cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct s364_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct s364_cursor));
		STAMP_OBJECT(S364_CURSOR, cursor_state_p->cursor_list_pp[i]);

		cursor_p = cursor_state_p->cursor_list_pp[i];
		cursor_p->turnoff_function_p = s364_cursor_turnoff;
		cursor_p->turnon_function_p = s364_cursor_turnon;
		cursor_p->move_function_p = s364_cursor_move;


		cursor_p->source_p = allocate_memory(sizeof(SIbitmap));
		cursor_p->inverse_source_p = allocate_memory(sizeof(SIbitmap));
		cursor_p->mask_p = allocate_memory(sizeof(SIbitmap));

		cursor_p->save_area_p = NULL;

	}
	return 1;
}
/*
 * s364_cursor_initialize_chipset_cursors
 *
 * PURPOSE
 *
 *	Patch in the cursor entry points in the private data structure for
 * the number of downloadable cursors. Initialization of the cursor
 * data structures is also done here.
 *
 * RETURN VALUE
 *
 *	0 on failure
 *	1 on success
 *
 */
STATIC int
s364_cursor_initialize_chipset_cursors(int no_of_cursors)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	int i ;
	int start_byte_address;
	struct omm_allocation *omm_block_p;
	struct s364_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(S364_CURSOR_STATE, cursor_state_p)) ;
	
	omm_block_p = omm_named_allocate(S364_CURSOR_NAMED_ALLOCATION_ID);
	if (omm_block_p  == NULL)
	{
		(void) fprintf(stderr, 
			S364_CHIPSET_CURSOR_OFFSCREEN_ALLOCATION_FAILED_MESSAGE);
		return 0;
	}

	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, omm_block_p));
	

#if defined ( __DEBUG__ )
	if (s364_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
			"(s364_cursor_initialize_chipset_cursors)"
			"{\n"
			"\tOM Block at ( %d, %d)\n"
			"\tno_of_cursors = %d\n"
			"}\n",
		    omm_block_p->x,
		    omm_block_p->y,
		    no_of_cursors);
	}
#endif
 
	start_byte_address = ((S364_MAX_VIRTUAL_Y_PIXELS() * 
			(S364_MAX_PHYSICAL_X_PIXELS() * S364_SCREEN_DEPTH() / 8) 
			+ 1023) / 1024) * 1024;


	cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct s364_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct s364_cursor));
		STAMP_OBJECT(S364_CURSOR, cursor_state_p->cursor_list_pp[i]);

		cursor_p = cursor_state_p->cursor_list_pp[i];

		cursor_p->turnoff_function_p = s364_chipset_cursor_turnoff;

		cursor_p->turnon_function_p = s364_chipset_cursor_turnon;

		cursor_p->move_function_p = s364_chipset_cursor_move;

		cursor_p->chipset_cursor_p= 
			allocate_and_clear_memory(sizeof(struct s364_chipset_cursor));

		STAMP_OBJECT(S364_CHIPSET_CURSOR, cursor_p->chipset_cursor_p);

		/*
		 * Chipset cursor is 64 bits wide and 64 bits high. Refer 
		 * Vision864 manual. Hence, the size of the cursor is 1024 bytes.
		 */
		start_byte_address += i * 1024;

		cursor_p->chipset_cursor_p->offscreen_x_location = start_byte_address % 
			(S364_MAX_PHYSICAL_X_PIXELS() * S364_SCREEN_DEPTH() / 8);
		
		cursor_p->chipset_cursor_p->offscreen_y_location = start_byte_address / 
			(S364_MAX_PHYSICAL_X_PIXELS() * S364_SCREEN_DEPTH() / 8);

#if defined ( __DEBUG__ )
		if ( s364_cursor_debug )
		{
			(void) fprintf(debug_stream_p,
			"(s364_cursor_initialize_chipset_cursors)"
			"{\n"
			"\tcursor_index = %d"
			"\tOffscreen loc ( %d, %d)"
			"}\n",
			 i,
			 cursor_p->chipset_cursor_p->offscreen_x_location,
			 cursor_p->chipset_cursor_p->offscreen_y_location);
		}
#endif
	}

	/*
	 * The cursor offset has to be made zero as the default value
	 * is undefined. This is the offset into the cursor pattern.
	 */
	S364_SET_CHIPSET_CURSOR_HORZ_DISPLAY_OFFSET(screen_state_p, 0);

	S364_SET_CHIPSET_CURSOR_VERT_DISPLAY_OFFSET(screen_state_p, 0);

	return 1;

}

/*
 * DAC cursor functions
 */

/*
 * s364_bt485_generate_hardware_cursor
 *
 * PURPOSE
 *
 * 	Routine to convert the cursor pattern from the SI's format to
 * the one required by the Bt485 DAC.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_bt485_generate_hardware_cursor(
	SICursorP cursor_p,
	SIbitmap *src_bitmap_p,
	SIbitmap *mask_bitmap_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	int bitmap_size;
	int h;
	int wsrc;
	int rounded_width;
	int effective_cursor_width;
	register int i;
	register int j;
	unsigned char end_mask;
	unsigned char *mask_bits_p = NULL;
	unsigned char *src_bits_p = NULL;
	unsigned char *plane0, *plane1;
	
	const unsigned char *inverted_byte_value_table =
		screen_state_p->byte_invert_table_p;

    bitmap_size = (( cursor_state_p->cursor_max_width *
         cursor_state_p->cursor_max_height) / 8) * 2; /*bytes*/

    plane0 = cursor_state_p->dac_cursor_p->cursor_bits_p;
    plane1 = cursor_state_p->dac_cursor_p->cursor_bits_p + (bitmap_size / 2);

	src_bits_p = (unsigned char*) src_bitmap_p->Bptr ;
    mask_bits_p = (unsigned char*) mask_bitmap_p->Bptr ;

	h = cursor_p->SCheight;

	if (h > cursor_state_p->cursor_max_height)
	{
		h = cursor_state_p->cursor_max_height;
	}

	effective_cursor_width = cursor_p->SCwidth ;

	if (effective_cursor_width > cursor_state_p->cursor_max_width)
	{
		effective_cursor_width = cursor_state_p->cursor_max_width;
	}

    rounded_width = ((cursor_p->SCwidth + 31) & ~31) >> 3;

	/*
	 * Round it off to the nearest byte
	 */

	wsrc = ((effective_cursor_width  + 7) & ~7) >> 3;

	end_mask = (1 << (effective_cursor_width & 7)) - 1;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"\teffective_cursor_width = %d\n"
			"\twsrc = %d\n"
			"\tend_mask = %x\n"
			"\trounded_width = %d\n",
			effective_cursor_width,
			wsrc,
			end_mask,
			rounded_width);
	}
#endif
		

	for (i = 0; i < cursor_state_p->cursor_max_height; i++)
	{
		for (j = 0; j < (cursor_state_p->cursor_max_width >> 3); j++)
		{
			unsigned char mask, source;

			if (i < h && j < wsrc)
			{
				source = *src_bits_p++;
				mask = *mask_bits_p++;

				if ((j  == wsrc -1) && end_mask) 
				{
					source  &= end_mask;
					mask &= end_mask;
				}

				source = inverted_byte_value_table[source];
				mask = inverted_byte_value_table[mask];

				*plane0++ = source & mask;
				*plane1++ = mask;
			}
			else
			{
				if (j < rounded_width)
				{
					++src_bits_p;
					++mask_bits_p;
				}

				*plane0++ = 0x00;
				*plane1++ = 0x00;
			}
		}

		while (j++ < rounded_width)
		{
			++src_bits_p;
			++mask_bits_p;
		}
	}
}

/*
 * s364_ti3025_generate_hardware_cursor
 *
 * PURPOSE
 *
 * 	Routine to convert the cursor pattern from the SI's format to
 * the one required by the Ti3025 DAC.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_ti3025_generate_hardware_cursor(
	SICursorP cursor_p,
	SIbitmap *src_bitmap_p,
	SIbitmap *mask_bitmap_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

	int h;
	int wsrc;
	int rounded_width;
	int effective_cursor_width;
	register int i;
	register int j;
	unsigned char end_mask;
	unsigned char *mask_bits_p = NULL;
	unsigned char *src_bits_p = NULL;
	unsigned char *cursor_pattern_p;

	/* Each pixel in the resultant map is represented by two  bits
	**  src_bit		mask_bit		resultant_bits
	**  ----------------------------------------------------
	**	1			1				11 		Foreground color
	**	0			1				10		Background color
	**	0			0				00		Transparent
	**  ----------------------------------------------------
	*/

    cursor_pattern_p = cursor_state_p->dac_cursor_p->cursor_bits_p;

	src_bits_p = (unsigned char*) src_bitmap_p->Bptr ;
    mask_bits_p = (unsigned char*) mask_bitmap_p->Bptr ;

	h = cursor_p->SCheight;

	if (h > cursor_state_p->cursor_max_height)
	{
		h = cursor_state_p->cursor_max_height;
	}

	effective_cursor_width = cursor_p->SCwidth ;

	if (effective_cursor_width > cursor_state_p->cursor_max_width)
	{
		effective_cursor_width = cursor_state_p->cursor_max_width;
	}

	/*
	 * Round it off to the nearest byte
	 */

    rounded_width = ((cursor_p->SCwidth + 31) & ~31) >> 3;

	wsrc = ((effective_cursor_width  + 7) & ~7) >> 3;

	end_mask = (1 << (effective_cursor_width & 7)) - 1;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"\teffective_cursor_width = %d\n"
			"\twsrc = %d\n"
			"\tend_mask = %x\n",
			effective_cursor_width,
			wsrc,
			end_mask);
	}
#endif
		
	for (i = 0; i < cursor_state_p->cursor_max_height; i++)
	{
		for (j = 0; j < (cursor_state_p->cursor_max_width >> 3); j++)
		{
			unsigned char mask, source;

			if ((i < h ) && (j < wsrc))
			{
				mask = *mask_bits_p++;
				source = *src_bits_p++ & mask;

				if ((j == (wsrc - 1)) && end_mask) 
				{
					source &= end_mask;
					mask &= end_mask;
				}

    			*cursor_pattern_p++ =     
						((mask&0x01) ? 0x80 : 0) | ((source&0x01) ? 0x40 : 0) | 
						((mask&0x02) ? 0x20 : 0) | ((source&0x02) ? 0x10 : 0) | 
						((mask&0x04) ? 0x08 : 0) | ((source&0x04) ? 0x04 : 0) | 
						((mask&0x08) ? 0x02 : 0) | ((source&0x08) ? 0x01 : 0);

			    *cursor_pattern_p++ = 
						((mask&0x10) ? 0x80 : 0) | ((source&0x10) ? 0x40 : 0) | 
						((mask&0x20) ? 0x20 : 0) | ((source&0x20) ? 0x10 : 0) | 
						((mask&0x40) ? 0x08 : 0) | ((source&0x40) ? 0x04 : 0) | 
						((mask&0x80) ? 0x02 : 0) | ((source&0x80) ? 0x01 : 0);

			}
			else
			{
				if (j < rounded_width)
				{
					++src_bits_p;
					++mask_bits_p;
				}
				*cursor_pattern_p++ = 0x00; 
				*cursor_pattern_p++ = 0x00;
			}
		}
		/*
		 * if we still have more bytes on this line (j < wsrc),
		 * we have to ignore the rest of the line.
		 */
		if (i < h) 
		{
			while (j++ < rounded_width) 
			{
				mask_bits_p++;
				src_bits_p++;
			}
		}
	}
}
/*
 * PURPOSE
 *
 *	Dac cursor download processes the SI cursor bitmap and calls the 
 * dac specific routine to convert to the format required for that
 * dac. The actual download into the DAC cursor ram will be done at
 * turnon time.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_dac_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();

	int	 bitmap_size;
	int  total_bytes;
	SIbitmap src_bitmap;
	SIbitmap invsrc_bitmap;
	SIbitmap mask_bitmap;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
				"(s364_dac_cursor_download){\n"
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

	cursor_state_p->dac_cursor_p->foreground_color =
		cursor_p->SCfg;

	cursor_state_p->dac_cursor_p->background_color =
		cursor_p->SCbg;

	/*
	 * DAC Cursor is 2 bit deep
	 */
	
	bitmap_size = (( cursor_state_p->cursor_max_width *
		 cursor_state_p->cursor_max_height) / 8) * 2; /*bytes*/


	memset(cursor_state_p->dac_cursor_p->cursor_bits_p,0,bitmap_size); 

	total_bytes = (( cursor_p->SCsrc->Bwidth + 31 ) & ~31 ) >> 3;
	total_bytes  *= cursor_p->SCsrc->Bheight; 

	ASSERT(total_bytes > 0 );

	(void) memcpy(&mask_bitmap,cursor_p->SCmask, sizeof(SIbitmap));
	(void) memcpy(&src_bitmap,cursor_p->SCsrc, sizeof(SIbitmap));
	(void) memcpy(&invsrc_bitmap,cursor_p->SCinvsrc, sizeof(SIbitmap));

	mask_bitmap.Bptr = allocate_and_clear_memory(total_bytes);
	src_bitmap.Bptr = allocate_and_clear_memory(total_bytes);
	invsrc_bitmap.Bptr = allocate_and_clear_memory(total_bytes);

	/*
	 * NOTE: invsrc_bitmap is not used in this function
	 */

	(void) s364_cursor_clear_extra_bits_and_copy(cursor_p,
		&src_bitmap,&invsrc_bitmap,
		&mask_bitmap);

	if(dac_state_p->dac_kind == S364_DAC_BT485KPJ110 ||
		dac_state_p->dac_kind == S364_DAC_BT485KPJ135)
	{
		/*
		 * BT 485 dac cursor
		 */
		s364_bt485_generate_hardware_cursor(cursor_p, &src_bitmap, 
			&mask_bitmap);
	}
	else if(dac_state_p->dac_kind == S364_DAC_PTVP3025_135MDN ||
			dac_state_p->dac_kind == S364_DAC_PTVP3025_175MDN ||
			dac_state_p->dac_kind == S364_DAC_PTVP3025_200MDN)
	{
		/*
		 * TI 3025 dac cursor
		 */
		s364_ti3025_generate_hardware_cursor(cursor_p, &src_bitmap, 
			&mask_bitmap);
	}
    else 
	{
		/*
		 * DAC cursor not supported for other dacs yet.
		 * Should we return SI_FAIL (This condition should never occur)
		 */
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}
   
	cursor_state_p->dac_cursor_p->cursor_height =
	    ((cursor_p->SCheight > cursor_state_p->cursor_max_height) ?
		 cursor_state_p->cursor_max_height : cursor_p->SCheight);

	cursor_state_p->dac_cursor_p->cursor_width =
	    ((cursor_p->SCwidth > cursor_state_p->cursor_max_width) ?
		cursor_state_p->cursor_max_width : cursor_p->SCwidth);

	free_memory(mask_bitmap.Bptr);
	free_memory(src_bitmap.Bptr);
	free_memory(invsrc_bitmap.Bptr);
	
	return SI_SUCCEED;
}
/*
 * PURPOSE
 *
 *	The cursor pattern is actully downloaded into the cursor ram, the
 * cursor colors are programmed and the cursor is enabled.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_dac_cursor_turnon(SIint32 cursor_index)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

	ASSERT(cursor_state_p->dac_cursor_p);

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
			"(s364_dac_cursor_turnon){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif

	/*
 	 * Turn the existing cursor off
	 */

	(*dac_state_p->disable_hardware_cursor)();

	/*
	 * Download the new cursor bits into the DAC cursor RAM
	 */

	(*dac_state_p->download_hardware_cursor)(
		cursor_state_p->dac_cursor_p->cursor_bits_p);

	/*
	 * Program  the DAC cursor palette
	 */ 

	(*dac_state_p->set_hardware_cursor_color)(
		cursor_state_p->dac_cursor_p->foreground_color,
		cursor_state_p->dac_cursor_p->background_color);

	/*
	 * Reprogram the cursor location 
	 */
		
	(*dac_state_p->move_hardware_cursor)(
		cursor_state_p->current_cursor_status.x,
		cursor_state_p->current_cursor_status.y);


	/*
	 * Turn the new cursor on
	 */

	(*dac_state_p->enable_hardware_cursor)();

	/*
	 * Mark the cursor as on
	 */

	cursor_state_p->current_cursor_status.flag = CURSOR_ON;

	return SI_TRUE;


}
/*
 * PURPOSE
 *
 *	The cursor is disabled. This is done by calling the dac specific
 * disable cursor function.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_dac_cursor_turnoff(SIint32 cursor_index)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();

	ASSERT(cursor_state_p->dac_cursor_p);
#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
			"(s364_dac_cursor_turnoff){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif


	(*dac_state_p->disable_hardware_cursor)();

	cursor_state_p->current_cursor_status.flag = CURSOR_OFF;

	return SI_TRUE;

}

/*
 * PURPOSE
 *
 *	Program the cursor position registers with the new x, y values.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_dac_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

	ASSERT(cursor_state_p->dac_cursor_p);

#if defined(__DEBUG__)
	if ( s364_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
				"(s364_dac_cursor_move){\n"
				"\tcursor_index= %d\n"
				"\tx = %d\n"
				"\ty= %d\n}\n",
				(int)cursor_index,
				(int)x, (int)y);
	 }
#endif

	
	(*dac_state_p->move_hardware_cursor)(x,y);

	cursor_state_p->current_cursor_status.x = x;
	cursor_state_p->current_cursor_status.y = y;

	return SI_TRUE;

}


/*
 * s364_cursor__initialize__
 *
 * PURPOSE
 *
 * Initializing the cursor module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */

function void
s364_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
{
	struct s364_cursor_state  *cursor_state_p;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();


#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364__cursor__initialize__){\n"
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
			S364_MAX_DISPLAYED_X_PIXELS(),
			S364_MAX_DISPLAYED_Y_PIXELS(),
			S364_MAX_VIRTUAL_X_PIXELS(),
			S364_MAX_VIRTUAL_Y_PIXELS(),
			S364_MAX_PHYSICAL_X_PIXELS(),
			S364_MAX_PHYSICAL_Y_PIXELS());
	}
#endif


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

	cursor_state_p =
	    allocate_and_clear_memory(sizeof(struct s364_cursor_state));

	STAMP_OBJECT(S364_CURSOR_STATE, cursor_state_p);

	flags_p->SIcurscnt = (options_p->number_of_downloadable_cursors) ?
	    options_p->number_of_downloadable_cursors :
	    DEFAULT_S364_NUMBER_OF_DOWNLOADABLE_CURSORS;

	/*
	 * Get the cursor size that one is supposed to support.
	 */

	if (options_p->cursor_max_size == NULL)
	{
		cursor_state_p->cursor_max_width =
			DEFAULT_S364_DOWNLOADABLE_CURSOR_WIDTH;

		cursor_state_p->cursor_max_height =
			DEFAULT_S364_DOWNLOADABLE_CURSOR_HEIGHT;
	}
	else
	{
		if((sscanf(options_p->cursor_max_size,"%ix%i",
			&cursor_state_p->cursor_max_width, 
			&cursor_state_p->cursor_max_height) != 2) ||
		    (cursor_state_p->cursor_max_width <= 0) ||
		    (cursor_state_p->cursor_max_height <= 0))
		{
			(void) fprintf(stderr, S364_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
						   options_p->cursor_max_size);

			cursor_state_p->cursor_max_width =
				DEFAULT_S364_DOWNLOADABLE_CURSOR_WIDTH;

			cursor_state_p->cursor_max_height =
				DEFAULT_S364_DOWNLOADABLE_CURSOR_HEIGHT;
		}
	}

	/*
	 * Set SI's idea of the best cursor width.
	 */
	
	flags_p->SIcurswidth =
		cursor_state_p->cursor_max_width;
	flags_p->SIcursheight =
		cursor_state_p->cursor_max_height;

	/*
	 * The cursor mask is valid for software cursors only.
	 */

	flags_p->SIcursmask = DEFAULT_S364_DOWNLOADABLE_CURSOR_MASK;

	/*
	 * Default handlers.
	 */

	functions_p->si_hcurs_download = 
		(SIBool (*)(SIint32, SICursorP)) generic_no_operation_succeed;
	functions_p->si_hcurs_turnon = 
		(SIBool (*)(SIint32)) generic_no_operation_succeed;
	functions_p->si_hcurs_turnoff = 
		(SIBool (*)(SIint32)) generic_no_operation_succeed;
	functions_p->si_hcurs_move = 
		(SIBool (*)(SIint32, SIint32, SIint32))
		 generic_no_operation_succeed;

	/*
	 * If the user has asked for software cursor or the dac
	 * cannot support cursor then switch to software (FAKEHDWR)
	 * cursor else TRUEHDWR
	 */


	if((options_p->cursor_type == S364_OPTIONS_CURSOR_TYPE_AUTO_CONFIGURE ||
		options_p->cursor_type == S364_OPTIONS_CURSOR_TYPE_DAC_CURSOR) &&
		(dac_state_p->dac_private_flags & 
		DAC_PRIVATE_FLAGS_CAN_USE_DAC_CURSOR))
	{
		int bitmap_size;
		/*
	 	 * DAC cursor support:
		 * Since we can have only one cursor in the DAC RAM at any
	 	 * time we will mark no of downloadable cursors as 1
	 	 */

		flags_p->SIcursortype = CURSOR_TRUEHDWR;
		flags_p->SIcurscnt = 1;
		
		functions_p->si_hcurs_download = s364_dac_cursor_download;
		functions_p->si_hcurs_turnon = s364_dac_cursor_turnon;
		functions_p->si_hcurs_turnoff = s364_dac_cursor_turnoff;
		functions_p->si_hcurs_move = s364_dac_cursor_move;
		
		(*dac_state_p->get_hardware_cursor_max_size)(
			&cursor_state_p->cursor_max_width,	
			&cursor_state_p->cursor_max_height);

		flags_p->SIcurswidth =
			cursor_state_p->cursor_max_width;
		flags_p->SIcursheight =
			cursor_state_p->cursor_max_height;

		cursor_state_p->dac_cursor_p = 
			allocate_and_clear_memory(sizeof(struct s364_dac_cursor));


		bitmap_size = (( cursor_state_p->cursor_max_width *
			 cursor_state_p->cursor_max_height) / 8) * 2; /*bytes*/

		cursor_state_p->dac_cursor_p->cursor_bits_p  =
			allocate_and_clear_memory(bitmap_size); 

		screen_state_p->cursor_state_p 
			= cursor_state_p ;
	}
	else if ((
		options_p->cursor_type == S364_OPTIONS_CURSOR_TYPE_AUTO_CONFIGURE ||
		options_p->cursor_type == S364_OPTIONS_CURSOR_TYPE_CHIPSET_CURSOR) &&
		(screen_state_p->chipset_kind == 
		S364_CHIPSET_KIND_S3_CHIPSET_VISION864) &&
		( S364_MAX_VIRTUAL_Y_PIXELS() < (S364_MAX_PHYSICAL_Y_PIXELS() - 1)) &&
		(S364_SCREEN_DEPTH() <= 8))
	{
#if defined(__DEBUG__)
		if( s364_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
				"(s364_cursor__initialize__){\n"
				"\t#Switching to chipset cursor\n"
				"}\n");
		}
#endif
		flags_p->SIcursortype = CURSOR_TRUEHDWR;

		/*
		 * Cursor manipulation functions.
		 */
		functions_p->si_hcurs_download = s364_chipset_cursor_download;
		functions_p->si_hcurs_turnon = s364_chipset_cursor_turnon;
		functions_p->si_hcurs_turnoff = s364_chipset_cursor_turnoff;
		functions_p->si_hcurs_move = s364_chipset_cursor_move;

		/*
		 * Used for pixel panning
		 */
		cursor_state_p->visible_window_top_x = 0;
		cursor_state_p->visible_window_top_y = 0;
		
		/*
		 * Assign the cursor state.
		 */
		screen_state_p->cursor_state_p = cursor_state_p ;

		/*
		 * Allocate off-screen memory etc.
		 */

		if (s364_cursor_initialize_chipset_cursors(flags_p->SIcurscnt) == 0)
		{
			/*
			 * Allocation failed : Undo all work done 
			 */

#if (defined(__DEBUG__))
			if (s364_cursor_debug)
			{
				(void) fprintf(debug_stream_p,
					"(s364_cursor__initialize__)\n"
					"{\n"
					"\t# Initialization of chipset cursors failed.\n"
					"}\n");
			
			}
#endif

			flags_p->SIcursortype = CURSOR_FAKEHDWR;
	
			if(screen_state_p->generic_state.screen_server_version_number >=
				X_SI_VERSION1_1)  
			{
				return;
			}
			else
			{

				/*
				 * Cursor manipulation functions.
				 */
				functions_p->si_hcurs_download = s364_cursor_download;
				functions_p->si_hcurs_turnon = s364_cursor_turnon;
				functions_p->si_hcurs_turnoff = s364_cursor_turnoff;
				functions_p->si_hcurs_move = s364_cursor_move;

				/*
				 * Assign the cursor state.
				 */
				screen_state_p->cursor_state_p = cursor_state_p ;

				(void) s364_cursor_initialize_cursors(flags_p->SIcurscnt); 
			}
		}
	}
	else
	{

		flags_p->SIcursortype = CURSOR_FAKEHDWR;

		/*
		 * For R5 and above we don't get called for software cursor
		 */
		if(screen_state_p->generic_state.screen_server_version_number >=
			X_SI_VERSION1_1)  
		{
			return;
		}

		/*
		 * Cursor manipulation functions.
		 */

		functions_p->si_hcurs_download = s364_cursor_download;
		functions_p->si_hcurs_turnon = s364_cursor_turnon;
		functions_p->si_hcurs_turnoff = s364_cursor_turnoff;
		functions_p->si_hcurs_move = s364_cursor_move;

		/*
		 * Assign the cursor state.
		 */

		screen_state_p->cursor_state_p = cursor_state_p ;

		(void) s364_cursor_initialize_cursors(flags_p->SIcurscnt); 
	}

	cursor_state_p->number_of_cursors = flags_p->SIcurscnt;
	cursor_state_p->current_cursor_status.x = -1;
	cursor_state_p->current_cursor_status.y = -1;
	cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
	cursor_state_p->current_cursor_status.current_index = -1;

	return;
}

/*
 * PURPOSE
 *
 *		Initialization to be done when the virtual terminal used
 * by the X server is (re) entered. 
 * For dac cursor, force a turnon which inturn downloads and enables
 *		the cursor if the cursor status is ON.
 * For chipset cursor, enable the cursor.
 * For software cursor, nothing needs to be done since the framebuffer 
 *		is saved and restored.
 *
 * RETURN VALUE
 *
 *		None.
 */

function void
s364_cursor__vt_switch_in__(void)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

	/*
	 * Is the cursor, a DAC cursor ?
	 */
	if (cursor_state_p->dac_cursor_p == NULL)
	{
		/*
		 * Is the cursor being used a chipset cursor?
		 */
		if (cursor_state_p->cursor_list_pp[cursor_state_p->
			current_cursor_status.current_index]->chipset_cursor_p != NULL)
		{
			S364_ENABLE_CHIPSET_CURSOR(screen_state_p);
		}
	}
	else
	{
		/*
		 * The cursor RAM on the DAC could have changed, so force a download
		 * and a turnon
		 */
	
		if (cursor_state_p->current_cursor_status.flag == CURSOR_ON)
		{
			s364_dac_cursor_turnon(0);
		}
	}
	
	return;
}



/*
 * s364_cursor_make_named_allocation_string 
 *
 * PURPOSE
 *
 *		Routine to form a named allocate string to be given to omm
 * for allocating offscreen memory in 1K boundary for the number of
 * downloadable chipset cursors.
 *
 * RETURN VALUE
 *
 *		NULL on failure  (or)
 *		Pointer to the named allocate string.
 *
 */

function char *
s364_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
{

	int total_pixels ;
	int lines ;
	int last_line ;
	int cursor_max_width;
	int cursor_max_height;
	int number_of_cursors ;
	int start_x ;
	int start_y ;
	int start_byte_address;
	char tmp_buf[S364_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN+1] ;
	char *string_p ;

	S364_CURRENT_SCREEN_STATE_DECLARE();

	/*
	 * No need for named allocation for chipset cursor 
	 * if there is no memory below the display area.
	 * Also see s364_cursor_initialize function.
	 */
	if (S364_MAX_VIRTUAL_Y_PIXELS() >= (S364_MAX_PHYSICAL_Y_PIXELS() - 1))
	{
#if defined (__DEBUG__)
		if ( s364_cursor_debug )
		{
			(void)fprintf(debug_stream_p,
				"(s364_cursor_make_named_allocation_string){\n" 
				"\tReturning null\n"
				"}\n");
		}
#endif
		return(NULL);
	}

	if(options_p->cursor_max_size == NULL)
	{
		cursor_max_width = DEFAULT_S364_DOWNLOADABLE_CURSOR_WIDTH;

		cursor_max_height = DEFAULT_S364_DOWNLOADABLE_CURSOR_HEIGHT;
	}
	else if ((sscanf(options_p->cursor_max_size,"%ix%i",
			 &cursor_max_width, 
			 &cursor_max_height) != 2) ||
			 (cursor_max_width <= 0) ||
			 (cursor_max_height <= 0))
	{
		(void) fprintf(stderr, S364_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
					   options_p->cursor_max_size);
		
		cursor_max_width = DEFAULT_S364_DOWNLOADABLE_CURSOR_WIDTH;

		cursor_max_height = DEFAULT_S364_DOWNLOADABLE_CURSOR_HEIGHT;
	}

	start_byte_address = ((S364_MAX_VIRTUAL_Y_PIXELS() * 
			(S364_MAX_PHYSICAL_X_PIXELS() * S364_SCREEN_DEPTH() / 8) 
			+ 1023) / 1024) * 1024;

	start_x = start_byte_address % 
			(S364_MAX_PHYSICAL_X_PIXELS() * S364_SCREEN_DEPTH() / 8);

	start_y = start_byte_address / 
			(S364_MAX_PHYSICAL_X_PIXELS() * S364_SCREEN_DEPTH() / 8);

	number_of_cursors = (options_p->number_of_downloadable_cursors >  0) ?
			options_p->number_of_downloadable_cursors :
			DEFAULT_S364_NUMBER_OF_DOWNLOADABLE_CURSORS;

	total_pixels = (cursor_max_width*2/(S364_SCREEN_DEPTH()))* 
	    cursor_max_height;

	/*
	 * Need to add 'start_x' which lies at the 1K boundary in the line 
	 * 'start_y', to the 'total_pixels' in order to allocate integral 
	 * number of lines of width S364_MAX_PHYSICAL_X_PIXELS each.
	 */
	
	total_pixels = total_pixels * number_of_cursors + start_x;

	lines = total_pixels / S364_MAX_PHYSICAL_X_PIXELS () ;

	last_line = total_pixels % S364_MAX_PHYSICAL_X_PIXELS () ;
	
	if ( last_line > 0 )
	{
		++lines ;
	}


#if 0
	/*
	 * If it is a 1280x1024 mode then allocate starting from the
	 * first line after displayed y lines. This is because in 1280 modes
	 * the HW cursor works only if downloaded byte position is the lcm of
	 * 1280 and 1024.
	 */
	if (S364_MAX_VIRTUAL_Y_PIXELS() == 1024)
	{
		start_y = S364_MAX_VIRTUAL_Y_PIXELS();
	}
	else
	{
		/*
		 * for all other cases we will allocate at the end of
		 * the offscreen area
		 */
		start_y =  S364_MAX_PHYSICAL_Y_PIXELS() -
			 (number_of_cursors * lines) - 1;
		/*
		 * For some reason chipset cursor has problems while downloading
		 * at the end of a 4MB/3MB boundary. Hack to make it download at end
		 * of display area.
		 */
		if (S364_MAX_PHYSICAL_Y_PIXELS() > 3000)
		{
			start_y = S364_MAX_VIRTUAL_Y_PIXELS();
		}
	}
#endif

	(void)sprintf(tmp_buf, S364_CURSOR_NAMED_ALLOCATION_STRING_FORMAT,
			S364_MAX_PHYSICAL_X_PIXELS(),
			lines,
			S364_SCREEN_DEPTH(), 0, start_y, 0 );

	string_p = allocate_memory(strlen(tmp_buf) + 1);

	(void) strcpy (string_p, tmp_buf);

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"(s364_cursor_make_named_allocation_string){\n"
			"\topt_string=%s\n"
			"}\n", string_p);
	}
#endif

	return (string_p);
}

#ifdef FUTURE_USE_ONLY
/*
 * PURPOSE
 *
 *	Dac cursor download processes the SI cursor bitmap and calls the 
 * dac specific routine to convert to the format required for that
 * dac. The actual download into the DAC cursor ram will be done at
 * turnon time.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_dac_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	int width;
	int height;
	int rounded_width;
	register int i;
	register int j;
	int	 bitmap_size;
	int  total_bytes;
	SIbitmap src_bitmap;
	SIbitmap invsrc_bitmap;
	SIbitmap mask_bitmap;
	unsigned char *mask_bits_p = NULL;
	unsigned char *src_bits_p = NULL;
	unsigned char *plane0, *plane1;
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURSOR_STATE_DECLARE();

	const unsigned char *inverted_byte_value_table
					= screen_state_p->byte_invert_table_p;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
				"(s364_dac_cursor_download){\n"
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

	cursor_state_p->dac_cursor_p->foreground_color =
		cursor_p->SCfg;

	cursor_state_p->dac_cursor_p->background_color =
		cursor_p->SCbg;

	total_bytes = (( cursor_p->SCsrc->Bwidth + 31 ) & ~31 ) >> 3;
	total_bytes  *= cursor_p->SCsrc->Bheight; 

	ASSERT(total_bytes > 0 );

	(void) memcpy(&mask_bitmap,cursor_p->SCmask, sizeof(SIbitmap));
	(void) memcpy(&src_bitmap,cursor_p->SCsrc, sizeof(SIbitmap));
	(void) memcpy(&invsrc_bitmap,cursor_p->SCinvsrc, sizeof(SIbitmap));

	mask_bitmap.Bptr = allocate_and_clear_memory(total_bytes);
	src_bitmap.Bptr = allocate_and_clear_memory(total_bytes);
	invsrc_bitmap.Bptr = allocate_and_clear_memory(total_bytes);

	/*
	 * NOTE: invsrc_bitmap is not used in this function
	 */

	(void) s364_cursor_clear_extra_bits_and_copy(cursor_p,
		&src_bitmap,&invsrc_bitmap,
		&mask_bitmap);

	src_bits_p = (unsigned char*) src_bitmap.Bptr ;
    mask_bits_p = (unsigned char*) mask_bitmap.Bptr ;

	height = cursor_p->SCheight;

	if (height > cursor_state_p->cursor_max_height)
	{
		height = cursor_state_p->cursor_max_height;
	}

	width = cursor_p->SCwidth ;

	if (width > cursor_state_p->cursor_max_width)
	{
		width = cursor_state_p->cursor_max_width;
	}
	
	cursor_state_p->dac_cursor_p->cursor_width = width;

	cursor_state_p->dac_cursor_p->cursor_height = height;

#if defined(__DEBUG__)
	if (s364_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
			"\twidth = %d\n",
			width);
	}
#endif
	
	/*
	 * Later on (when adding support for other dac cursors) make this a 
	 * function specific to the dac.
	 *
	 * 	 void s364_dac_xxx_create_dac_cursor(unsigned char *source_bits_p,
	 *										unsigned char *mask_bits_p)
	 *
	 */

    rounded_width = ((width + 31) & ~31) >> 3;

	bitmap_size = (( cursor_state_p->cursor_max_width *
		 cursor_state_p->cursor_max_height) / 8) * 2; /*bytes*/

	plane0 = cursor_state_p->dac_cursor_p->cursor_bits_p;
	plane1 = cursor_state_p->dac_cursor_p->cursor_bits_p +
		 (bitmap_size / 2);

	for (i = 0; i < cursor_state_p->cursor_max_height; i++)
	{
		for (j = 0; j < (cursor_state_p->cursor_max_width >> 3); j++)
		{
			unsigned char source, mask;

			source = *src_bits_p++;
			mask = *mask_bits_p++;

			if ((i < height) && (j < rounded_width))
			{
				source = inverted_byte_value_table[source];
				mask = inverted_byte_value_table[mask];

				*plane0++ = source & mask;
				*plane1++ = mask;
			}
			else
			{
				*plane0++ = 0;
				*plane1++ = 0;
			}
		}

		/*
		 * There are more bytes in this line still. ( j < rounded_width).
		 * Ignore them as it has exceeded the maximum.
		 */
		while (j++ < rounded_width)
		{
			++src_bits_p;
			++mask_bits_p;
		}
	}

	
	free_memory(mask_bitmap.Bptr);
	free_memory(src_bitmap.Bptr);
	free_memory(invsrc_bitmap.Bptr);
	
	return SI_SUCCEED;
}

#endif
