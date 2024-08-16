/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_cursor.c	1.2"
/***
 ***	NAME
 ***
 ***		p9k_cursor.c : Cursor management module
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9000_cursor.h"
 ***
 ***
 ***			SIBool
 ***			p9000_cursor_download ( SIint32 cursor_index,
 ***									SICursorP cursor_p )
 ***			SIBool
 ***			p9000_cursor_turnon ( SIint32 cursor_index )
 ***			SIBool
 ***			p9000_cursor_turnoff ( SIint32 cursor_index )
 ***			SIBool
 ***			p9000_cursor_move ( SIint32 cursor_index, SIint32 x, SIint32 y )
 ***
 ***
 ***
 ***
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***		p9000_cursor.c : Source.
 ***		p9000_cursor.h : Interface.
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
#include "stdenv.h"
#include <string.h>



/***
 ***	Constants.
 ***/
#define P9000_CURSOR_STAMP\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + ('0' << 4) +\
	 ('_' << 5) + ('C' << 6) + ('U' << 7) + ('R' << 8) + ('S' << 9) +\
	 ('O' << 10) + ('R' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) +\
	 ('A' << 15) + ('M' << 16) + ('P' << 17) + 0 )

#define P9000_CURSOR_STATE_STAMP\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + ('0' << 4) +\
	 ('_' << 5) + ('C' << 6) + ('U' << 7) + ('R' << 8) + ('S' << 9) +\
	 ('O' << 10) + ('R' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) +\
	 ('A' << 15) + ('T' << 16) + ('E' << 17) + 0 )


/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

enum p9000_cursor_status_kind
{
	CURSOR_ON,
	CURSOR_OFF
};


struct p9000_cursor_status
{
	enum p9000_cursor_status_kind flag ;	/*is cursor on/off*/
	int	current_index ;		/*index of the current cursor (cursor_index)*/
	int	x;							/*X co-ord of the cursor*/
	int	y;							/*Y co-ord of the cursor*/
#if defined ( __DEBUG__ )
	int stamp ;
#endif
};




struct p9000_cursor
{
	int	cursor_width ;				
	int cursor_height; 

	int	foreground_color ;
	int	background_color ;

	/* Pointers to various cursor operation functions for this cursor*/

	SIBool (*move_function_p)    (SIint32, SIint32, SIint32);
	SIBool (*turnon_function_p)  (SIint32) ;
	SIBool (*turnoff_function_p) (SIint32) ;

	/*
	 * Bitmaps for the current cursor
	 */

	SIbitmapP source_p;
	SIbitmapP inverse_source_p;
	SIbitmapP mask_p;

	struct omm_allocation *save_area_p;
	int saved_width;
	int saved_height;

#if defined(__DEBUG__)
	int stamp ;
#endif

};

struct  p9000_dac_cursor
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

struct p9000_cursor_state
{

	int cursor_max_width;
	int cursor_max_height;

	/*No of downloadable cursors*/
	int number_of_cursors ;			

	/*Status of the current cursor */
	struct p9000_cursor_status current_cursor_status; 


	/* Pointer to cursors list: Used only by software cursors */
	struct p9000_cursor **cursor_list_pp;

	/* Used if cursor type is dac cursor */

	struct p9000_dac_cursor *dac_cursor_p;

#if defined ( __DEBUG__)
	int stamp ;
#endif

};

#if (defined(__DEBUG__))
export enum debug_level p9000_cursor_debug;
#endif

PRIVATE

/*
 * Includes
 */

#include "g_omm.h"

#include "p9k_state.h"
#include "p9k_opt.h"
#include "p9k_regs.h"
#include "p9k_gbls.h"
#include "p9k_dacs.h"



/***
 ***	Macros.
 ***/

#define P9000_MAX_DISPLAYED_X_PIXELS() \
	(screen_state_p->generic_state.screen_displayed_width) 

#define P9000_MAX_DISPLAYED_Y_PIXELS()\
	(screen_state_p->generic_state.screen_displayed_height) 

#define P9000_MAX_VIRTUAL_X_PIXELS() \
	(screen_state_p->generic_state.screen_virtual_width) 

#define P9000_MAX_VIRTUAL_Y_PIXELS()\
	(screen_state_p->generic_state.screen_virtual_height) 

#define P9000_MAX_PHYSICAL_X_PIXELS() \
	(screen_state_p->generic_state.screen_physical_width) 

#define P9000_MAX_PHYSICAL_Y_PIXELS()\
	(screen_state_p->generic_state.screen_physical_height) 

#define P9000_SCREEN_DEPTH()\
	(screen_state_p->generic_state.screen_depth) 


#define	P9000_CURSOR_STATE_DECLARE()\
	struct p9000_cursor_state  *cursor_state_p=\
	(screen_state_p)->cursor_state_p


#define	P9000_CURSOR_IS_VALID_INDEX(cursor_index)\
	(cursor_index >= 0) &&\
	(cursor_index<cursor_state_p->number_of_cursors)



/*
 * Function for nullpadding the undefined bits in the cursor source
 * and inverse source bitmaps.
 */

STATIC void
p9000_cursor_clear_extra_bits_and_copy(SICursorP si_cursor_p,
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
 * p9000_cursor_copy_area
 * 
 * Used in saving the contents of saved areas from the displayed
 * screen and conversely for restoring the saved screen portions from
 * offscreen area to the displayed screen.
 */

STATIC void
p9000_cursor_copy_area(int source_x, int source_y,
	int destination_x, int destination_y,
	int width, int height)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	unsigned int raster;
	unsigned int status;

#if defined ( __DEBUG__ )
	if (DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING))
	{
		(void)fprintf( debug_stream_p,
				"(p9000_cursor_copy_area){\n"
				"\tsource_x = %d\n"
				"\tsource_y = %d\n"
				"\tdest_x = %d\n"
				"\tdest_y = %d\n"
				"\twidth = %d\n"
				"\theight = %d\n"
				"}\n",
				source_x, source_y,
				destination_x, destination_y,
				width, height);
	}
#endif

	/*
	 * Do a screen to screen from source to destination
	 */
	
	/*
	 * First  program the following
	 * pmask = 0xff
	 * raster = GXcopy
	 */

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	P9000_REGISTER_SET_PLANE_MASK(0xff);

	raster = screen_state_p->blit_minterm_table_p[GXcopy];
	
	P9000_STATE_SET_RASTER(register_state_p, raster);

	P9000_REGISTER_SET_CLIPPING_RECTANGLE(destination_x,destination_y,
		destination_x + width, destination_y + height);
	
	
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(source_x,source_y));

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY((source_x + width-1),
		(source_y + height-1)));
	 
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(destination_x,destination_y));

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY((destination_x + width-1),
		(destination_y + height-1)));
	 

	do
	{
		status = P9000_INITIATE_BLIT_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

	ASSERT(!(status &  P9000_STATUS_BLIT_SOFTWARE));

	/*
	 * Restore planemask
	 */

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	P9000_REGISTER_SET_PLANE_MASK(register_state_p->pmask);
	P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);

}




/*
 *p9000_cursor_create_cursor
 */

STATIC int 
p9000_cursor_create_cursor(struct p9000_cursor *active_cursor_p,
	SICursor *cursor_p )
{
	int height;
	int	width;
	int total_bytes;
	P9000_CURRENT_SCREEN_STATE_DECLARE();

#if defined ( __DEBUG__ )
	if (DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING))
	{
		(void)fprintf( debug_stream_p,
				"(p9000_cursor_create_cursor){\n"
				"\tactive_cursor_p= %p\n"
				"cursor_p= %p\n"
				"}\n",
				(void*)active_cursor_p,
				(void*)cursor_p);
	}
#endif

	ASSERT((active_cursor_p != NULL)&&( cursor_p != NULL));
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR,active_cursor_p));

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

	p9000_cursor_clear_extra_bits_and_copy(cursor_p,
		active_cursor_p->source_p,
		active_cursor_p->inverse_source_p,
		active_cursor_p->mask_p);

	/*
	 * Allocate offscreen memory for saving obscured portion of the screen
	 */

	/*
	 * free previously allocated area
	 */

	if(active_cursor_p->save_area_p != NULL)
	{
		(void) omm_free(active_cursor_p->save_area_p);
	}

	active_cursor_p->save_area_p =
		omm_allocate(width, height,screen_state_p->generic_state.screen_depth,
		OMM_LONG_TERM_ALLOCATION);

	if(	active_cursor_p->save_area_p == NULL)
	{
		return 0;
	}

	return 1;
}

/*
 *p9000_cursor_save_obscured_screen 
 */

STATIC int
p9000_cursor_save_obscured_screen(int cursor_index)  
{
	int dx;
	int dy;
	int x1;
	int y1;
	int x2;
	int y2;
	int width;
	int height;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();
	struct p9000_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf( debug_stream_p,
					  "(p9000_cursor_save_obscured_screen){"
					  "\n"
					  "\tcursor_index= %d\n}\n",
					  cursor_index);
	}
#endif

	ASSERT(P9000_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR,active_cursor_p));

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

	 x2 = (x2 > P9000_MAX_VIRTUAL_X_PIXELS() + 1) ?
			 (P9000_MAX_VIRTUAL_X_PIXELS() + 1) : x2;
	 y2 = (y2 > P9000_MAX_VIRTUAL_Y_PIXELS() + 1) ?
			 (P9000_MAX_VIRTUAL_Y_PIXELS() + 1) : y2;

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

	active_cursor_p->saved_width = width;
	active_cursor_p->saved_height = height;

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,INTERNAL) )
	{
		(void)fprintf ( debug_stream_p,
			"(p9000_cursor_save_obscured_screen){\n"
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
	
	if(active_cursor_p->save_area_p  == NULL)
	{
		return 0;
	}

	if (OMM_LOCK(active_cursor_p->save_area_p))  
	{
		dx = active_cursor_p->save_area_p->x;  
		dy = active_cursor_p->save_area_p->y;  
		p9000_cursor_copy_area(x1, y1, dx,dy,width,height);
		OMM_UNLOCK(active_cursor_p->save_area_p);

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
 *p9000_cursor_restore_obscured_screen 
 */

STATIC int
p9000_cursor_restore_obscured_screen (int cursor_index)
{
	int sx;
	int sy;
	int	x;
	int	y;
	int width;
	int	height;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();
	struct p9000_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf ( debug_stream_p,
			"(p9000_cursor_restore_obscured_screen){\n"
			"\tcursor_index= %d\n"
			"}\n",
			cursor_index);
	 }
#endif

	ASSERT(P9000_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR,active_cursor_p));

	height = active_cursor_p->saved_height; 
	width = active_cursor_p->saved_width; 

	if (OMM_LOCK(active_cursor_p->save_area_p))
	{
		sx = active_cursor_p->save_area_p->x;
		sy = active_cursor_p->save_area_p->y;
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

		p9000_cursor_copy_area(sx, sy, x, y, width,height);
		OMM_UNLOCK(active_cursor_p->save_area_p);
		return 1;
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}
}

STATIC void
p9000_cursor_stplblt(SIbitmapP source_bitmap_p,
	 int source_x, int source_y,
	 int destination_x, int destination_y,
	 int width, int height)
{
	int source_step;
	int full_words_count = 0;
	int leading_bits_count = 0;
	int leading_bits_shift;
	int trailing_bits_count = 0;
	unsigned char *source_bits_p;

	const unsigned int swap_flag = 
		P9000_ADDRESS_SWAP_HALF_WORDS |
		P9000_ADDRESS_SWAP_BYTES |
		P9000_ADDRESS_SWAP_BITS;

 	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	


	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_cursor_stplblt)"
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

	if (width <= 0 || height <= 0)
	{
		return;
	}



	source_step = ((source_bitmap_p->Bwidth + 31) & ~31)  >> 3;
	
	/*
	 * Points to a long word, the first pixel to drawn for that
	 * scanline may be at an offset inside the long word
	 */

	source_bits_p = (unsigned char *) source_bitmap_p->Bptr +
					 (source_y * source_step) +  ((source_x & ~31) >> 3);


	if (source_x & 31)
	{
		leading_bits_count =  (32 - (source_x & 31));
		leading_bits_shift = (source_x & 31);
	}
	else
	{
		leading_bits_count = leading_bits_shift = 0;
	}
	

	if (leading_bits_count > width)
	{
		leading_bits_count = width;
		full_words_count = 0;
		trailing_bits_count = 0;
	}
	else
	{
		trailing_bits_count = (source_x + width) & 31;
		full_words_count = (width - leading_bits_count -
							trailing_bits_count) >> 5;
	}
	
	ASSERT(full_words_count >= 0);
	
	ASSERT((full_words_count*32 + leading_bits_count +
			trailing_bits_count) == width);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		destination_x);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(destination_x,destination_y));
	 

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		destination_x + width);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_ABS,1);

	


	
	/*
	 *Wait for engine idle before writing the first in a series of
	 *pixel1 commands
	 */
	
	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();


	do
	{
		register int count = full_words_count;
		register unsigned long *data_p = 
			(unsigned long*)source_bits_p;
		

		if(leading_bits_count > 0)
		{
			unsigned long tmp = (*data_p++) >> leading_bits_shift;
			
			ASSERT(leading_bits_count < 32);
			ASSERT(leading_bits_shift < 32 && leading_bits_shift > 0);
			
			P9000_PIXEL1_COMMAND(tmp,leading_bits_count,
				swap_flag);
		}
		
		if (count > 0)
		{
			do
			{
				P9000_PIXEL1_COMMAND(*data_p++, 32, swap_flag);

			}while( --count > 0);
		}

	
		if (trailing_bits_count > 0)
		{
			P9000_PIXEL1_COMMAND(*data_p, trailing_bits_count,
								 swap_flag);
		}

		source_bits_p += source_step;

	} while(--height > 0);
}



STATIC int
p9000_cursor_draw_cursor(int cursor_index)
{
	int x;
	int y;
	int source_x;
	int source_y;
	int width;
	int height;
	unsigned int raster;
	struct p9000_cursor *active_cursor_p;
	SIbitmapP inverse_source_p = NULL;
	SIbitmapP source_p;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();


	ASSERT(P9000_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR,active_cursor_p));

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

	if (x + width >= P9000_MAX_DISPLAYED_X_PIXELS())
	{
		width -= 
			(x + width -  P9000_MAX_DISPLAYED_X_PIXELS());
	}
	
	if (y + height >= P9000_MAX_DISPLAYED_Y_PIXELS())
	{
		height -= 
			(y + height - P9000_MAX_DISPLAYED_Y_PIXELS());
	}

	source_p = active_cursor_p->source_p;
	inverse_source_p = active_cursor_p->inverse_source_p;

	
	/*
	 * raster for transparent stippling under GXcopy
	 */

	raster = 
		(P9000_SOURCE_MASK &
		screen_state_p->fg_color_minterm_table_p[GXcopy]) | 
		(~P9000_SOURCE_MASK & P9000_DESTINATION_MASK);

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	P9000_REGISTER_SET_PLANE_MASK(0xff);
	P9000_STATE_SET_RASTER(register_state_p, raster);

	P9000_REGISTER_SET_FOREGROUND_COLOR(active_cursor_p->foreground_color);

	P9000_REGISTER_SET_CLIPPING_RECTANGLE(x,y,x+width,y+height);

	/*
	 * Stipple the source bitmap
	 */

	p9000_cursor_stplblt(source_p,source_x,source_y,x,y,width,height);


	/*
	 * Stipple using the inverse source bitmap
	 */

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	P9000_REGISTER_SET_FOREGROUND_COLOR(active_cursor_p->background_color);

	p9000_cursor_stplblt(inverse_source_p,
		source_x,source_y,x,y,width,height);


	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	P9000_REGISTER_SET_PLANE_MASK(register_state_p->pmask);
	P9000_REGISTER_SET_FOREGROUND_COLOR(register_state_p->fground);

	P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);

	return 1;

}

/*
 * p9000_cursor_turnon
 */

STATIC SIBool
p9000_cursor_turnon(SIint32 cursor_index)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct p9000_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf(debug_stream_p,
			"(p9000_cursor_turnon){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif

	ASSERT(P9000_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR,active_cursor_p));
#endif

	if (cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		return SI_SUCCEED;
	}

	p9000_cursor_save_obscured_screen(cursor_index);

	p9000_cursor_draw_cursor(cursor_index);


	cursor_state_p->current_cursor_status.flag = CURSOR_ON;
	cursor_state_p->current_cursor_status.current_index = cursor_index;

	return SI_SUCCEED;
}


/*
 * p9000_cursor_turnoff
 */

STATIC SIBool
p9000_cursor_turnoff(SIint32 cursor_index)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct p9000_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf ( debug_stream_p,
				"(p9000_cursor_turnoff){\n"
				"\tcursor_index= %d\n}\n",
				(int)cursor_index);
	 }
#endif

	ASSERT(P9000_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR,active_cursor_p));
#endif

	if (cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		return SI_SUCCEED;
	}
	else
	{
		cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
	}

 	p9000_cursor_restore_obscured_screen(cursor_index);
	
	return SI_SUCCEED;
}


/*
 *p9000_cursor_move
 */

STATIC SIBool
p9000_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();
	struct p9000_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf( debug_stream_p,
				"(p9000_cursor_move){\n"
				"\tcursor_index= %d\n"
				"\tx = %d\n"
				"\ty= %d\n}\n",
				(int)cursor_index,
				(int)x, (int)y);
	 }
#endif

		
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR,active_cursor_p));

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
 *p9000_cursor_download
 */

STATIC SIBool
p9000_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();
	struct p9000_cursor  *active_cursor_p;
	

#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING))
	{
		(void)fprintf ( debug_stream_p,
				"(p9000_cursor_download){\n"
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

	ASSERT ( P9000_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR, active_cursor_p));

	/*
	 * If the cursor being downloaded is currently in use and turned ON
	 * then turn it OFF, download the new cursor and then turn it ON
	 */

	if (cursor_state_p->current_cursor_status.current_index ==
		 cursor_index 
		 && cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		(*active_cursor_p->turnoff_function_p)(cursor_index);

		if (p9000_cursor_create_cursor(active_cursor_p,cursor_p)
			 == 0 )
		{
			return SI_FAIL;
		}
		return (*active_cursor_p->turnon_function_p)(cursor_index);
	}
	else
	{
		return (
			(p9000_cursor_create_cursor(active_cursor_p,cursor_p) ==
			 0) ? SI_FAIL : SI_SUCCEED);
	}
}

STATIC int
p9000_cursor_initialize_cursors(int no_of_cursors)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();
	int i ;
	struct p9000_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(P9000_CURSOR_STATE, cursor_state_p)) ;

	cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct p9000_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct p9000_cursor));
		STAMP_OBJECT(P9000_CURSOR, cursor_state_p->cursor_list_pp[i]);

		cursor_p = cursor_state_p->cursor_list_pp[i];
		cursor_p->turnoff_function_p = p9000_cursor_turnoff;
		cursor_p->turnon_function_p = p9000_cursor_turnon;
		cursor_p->move_function_p = p9000_cursor_move;


		cursor_p->source_p = allocate_memory(sizeof(SIbitmap));
		cursor_p->inverse_source_p = allocate_memory(sizeof(SIbitmap));
		cursor_p->mask_p = allocate_memory(sizeof(SIbitmap));

		cursor_p->save_area_p = NULL;

	}
	return 1;
	

}


/*
 * DAC cursor functions
 */

STATIC SIBool
p9000_dac_cursor_download(SIint32 cursor_index, SICursorP cursor_p)
{
	int h;
	int  wsrc;
	int rounded_width;
	register int i;
	register int j;
	int	 bitmap_size;
	int  total_bytes;
	int effective_cursor_width;
	unsigned char end_mask;
	SIbitmap src_bitmap;
	SIbitmap invsrc_bitmap;
	SIbitmap mask_bitmap;
	unsigned char *mask_bits_p = NULL;
	unsigned char *src_bits_p = NULL;
	unsigned char *plane0, *plane1;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();

	STATIC const unsigned char 
	inverted_byte_value_table[] =
	{
		0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50,
		0xd0, 0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8,
		0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04,
		0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4,
		0x34, 0xb4, 0x74, 0xf4, 0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c,
		0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82,
		0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32,
		0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
		0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 0x06, 0x86, 0x46,
		0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6,
		0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e,
		0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1,
		0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71,
		0xf1, 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99,
		0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25,
		0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
		0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d,
		0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3,
		0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 0x0b,
		0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb,
		0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67,
		0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 0x0f, 0x8f,
		0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f,
		0xbf, 0x7f, 0xff
	};



#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING))
	{
		(void)fprintf ( debug_stream_p,
				"(p9000_dac_cursor_download){\n"
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
	 * BT485 cursor is 2 bit deep
	 */
	
	bitmap_size = (( cursor_state_p->cursor_max_width *
		 cursor_state_p->cursor_max_height) / 8) * 2; /*bytes*/


	memset(cursor_state_p->dac_cursor_p->cursor_bits_p,0,bitmap_size); 

	plane0 = cursor_state_p->dac_cursor_p->cursor_bits_p;
	plane1 = cursor_state_p->dac_cursor_p->cursor_bits_p +
		 (bitmap_size / 2);



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

	(void) p9000_cursor_clear_extra_bits_and_copy(cursor_p,
		&src_bitmap,&invsrc_bitmap,
		&mask_bitmap);

	src_bits_p = (unsigned char*) src_bitmap.Bptr ;
    mask_bits_p = (unsigned char*) mask_bitmap.Bptr ;

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
	if (DEBUG_LEVEL_MATCH(p9000_cursor,INTERNAL))
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

	
	free_memory(mask_bitmap.Bptr);
	free_memory(src_bitmap.Bptr);
	free_memory(invsrc_bitmap.Bptr);
	

	cursor_state_p->dac_cursor_p->cursor_height =
	    ((cursor_p->SCheight > cursor_state_p->cursor_max_height) ?
		 cursor_state_p->cursor_max_height : cursor_p->SCheight);

	cursor_state_p->dac_cursor_p->cursor_width =
	    ((cursor_p->SCwidth > cursor_state_p->cursor_max_width) ?
		cursor_state_p->cursor_max_width : cursor_p->SCwidth);

	return SI_SUCCEED;
}

STATIC SIBool
p9000_dac_cursor_turnon(SIint32 cursor_index)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_DAC_FUNCTIONS_DECLARE();
	P9000_CURSOR_STATE_DECLARE();

	ASSERT(cursor_state_p->dac_cursor_p);

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf(debug_stream_p,
			"(p9000_dac_cursor_turnon){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif

	/*
 	 * Turn the existing cursor off
	 */

	(*dac_functions_p->turnoff_cursor_p)();

	/*
	 * Download the new cursor bits into the DAC cursor RAM
	 */

	(*dac_functions_p->download_cursor_p)(
		cursor_state_p->dac_cursor_p->cursor_bits_p);

	/*
	 * Program  the DAC cursor palette
	 */ 

	(*dac_functions_p->set_cursor_color_p)(
		cursor_state_p->dac_cursor_p->foreground_color,
		cursor_state_p->dac_cursor_p->background_color);

	/*
	 * Reprogram the cursor location 
	 */
		
	(*dac_functions_p->move_cursor_p)(cursor_state_p->current_cursor_status.x,
		cursor_state_p->current_cursor_status.y);


	/*
	 * Turn the new cursor on
	 */

	(*dac_functions_p->turnon_cursor_p)();

	/*
	 * Mark the cursor as on
	 */

	cursor_state_p->current_cursor_status.flag = CURSOR_ON;

	return SI_TRUE;


}

STATIC SIBool
p9000_dac_cursor_turnoff(SIint32 cursor_index)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();
	P9000_DAC_FUNCTIONS_DECLARE();

	ASSERT(cursor_state_p->dac_cursor_p);
#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf(debug_stream_p,
			"(p9000_dac_cursor_turnoff){\n"
			"\tcursor_index= %d\n"
			"}\n",
			(int)cursor_index);
	 }
#endif


	(*dac_functions_p->turnoff_cursor_p)();

	cursor_state_p->current_cursor_status.flag = CURSOR_OFF;

	return SI_TRUE;

}

STATIC SIBool
p9000_dac_cursor_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_DAC_FUNCTIONS_DECLARE();
	P9000_CURSOR_STATE_DECLARE();

	ASSERT(cursor_state_p->dac_cursor_p);

#if defined(__DEBUG__)
	if ( DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING) )
	{
		(void)fprintf( debug_stream_p,
				"(p9000_dac_cursor_move){\n"
				"\tcursor_index= %d\n"
				"\tx = %d\n"
				"\ty= %d\n}\n",
				(int)cursor_index,
				(int)x, (int)y);
	 }
#endif

	
	(*dac_functions_p->move_cursor_p)(x,y);

	cursor_state_p->current_cursor_status.x = x;
	cursor_state_p->current_cursor_status.y = y;

	return SI_TRUE;

}


/*
 * p9000_cursor__initialize__
 */

function void
p9000_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct p9000_options_structure * options_p)
{
	struct p9000_cursor_state  *cursor_state_p;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_DAC_FUNCTIONS_DECLARE();


#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000_cursor,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000__cursor__initialize__){\n"
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
			P9000_MAX_DISPLAYED_X_PIXELS(),
			P9000_MAX_DISPLAYED_Y_PIXELS(),
			P9000_MAX_VIRTUAL_X_PIXELS(),
			P9000_MAX_VIRTUAL_Y_PIXELS(),
			P9000_MAX_PHYSICAL_X_PIXELS(),
			P9000_MAX_PHYSICAL_Y_PIXELS());
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
	    allocate_and_clear_memory(sizeof(struct p9000_cursor_state));

	STAMP_OBJECT(P9000_CURSOR_STATE, cursor_state_p);

	flags_p->SIcurscnt = (options_p->number_of_downloadable_cursors) ?
	    options_p->number_of_downloadable_cursors :
	    P9000_DEFAULT_NUMBER_OF_DOWNLOADABLE_CURSORS;

	/*
	 * Get the cursor size that one is supposed to support.
	 */

	if (options_p->cursor_max_size == NULL)
	{
		cursor_state_p->cursor_max_width =
			P9000_DEFAULT_DOWNLOADABLE_CURSOR_WIDTH;

		cursor_state_p->cursor_max_height =
			P9000_DEFAULT_DOWNLOADABLE_CURSOR_HEIGHT;

	}
	else
	{
		if((sscanf(options_p->cursor_max_size,"%ix%i",
			&cursor_state_p->cursor_max_width, 
			&cursor_state_p->cursor_max_height) != 2) ||
		    (cursor_state_p->cursor_max_width <= 0) ||
		    (cursor_state_p->cursor_max_height <= 0))
		{
			(void) fprintf(stderr, P9000_MESSAGE_CANNOT_PARSE_CURSOR_SIZE,
						   options_p->cursor_max_size);

			cursor_state_p->cursor_max_width =
				P9000_DEFAULT_DOWNLOADABLE_CURSOR_WIDTH;

			cursor_state_p->cursor_max_height =
				P9000_DEFAULT_DOWNLOADABLE_CURSOR_HEIGHT;
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

	flags_p->SIcursmask = P9000_DEFAULT_DOWNLOADABLE_CURSOR_MASK;

	/*
	 * Default handlers.
	 */

	functions_p->si_hcurs_download = 
		(SIBool (*)(SIint32, SICursorP)) p9000_global_no_operation_succeed;
	functions_p->si_hcurs_turnon = 
		(SIBool (*)(SIint32)) p9000_global_no_operation_succeed;
	functions_p->si_hcurs_turnoff = 
		(SIBool (*)(SIint32)) p9000_global_no_operation_succeed;
	functions_p->si_hcurs_move = 
		(SIBool (*)(SIint32, SIint32, SIint32))
		 p9000_global_no_operation_succeed;

	/*
	 * If the user has asked for software cursor or the dac
	 * cannot support cursor then switch to software (FAKEHDWR)
	 * cursor else TRUEHDWR
	 */


	if ((options_p->cursor_type == P9000_OPTIONS_CURSOR_TYPE_SOFTWARE_CURSOR) ||
		((*dac_functions_p->can_support_cursor_p)() == FALSE))
	{
		flags_p->SIcursortype = CURSOR_FAKEHDWR;

		/*
		 * For R5 and above we don't get called for software cursor
		 */

		if(screen_state_p->generic_state.screen_server_version_number >=
			X_SI_VERSION1_1)  
		{
			functions_p->si_hcurs_download = 
				(SIBool (*)(SIint32, SICursorP))
				 p9000_global_no_operation_succeed;
			return;
		}

		/*
		 * Cursor manipulation functions.
		 */

		functions_p->si_hcurs_download = p9000_cursor_download;
		functions_p->si_hcurs_turnon = p9000_cursor_turnon;
		functions_p->si_hcurs_turnoff = p9000_cursor_turnoff;
		functions_p->si_hcurs_move = p9000_cursor_move;



		/*
		 * Assign the cursor state.
		 */

		screen_state_p->cursor_state_p 
			= cursor_state_p ;

		(void) p9000_cursor_initialize_cursors(flags_p->SIcurscnt); 
	}
	else
	{
		int bitmap_size;
		/*
	 	 * DAC cursor support:
		 * Since we can have only one cursor in the DAC RAM at any
	 	 * time we will mark no of downloadable cursors as 1
	 	 */

		flags_p->SIcursortype = CURSOR_TRUEHDWR;
		flags_p->SIcurscnt = 1;
		
		functions_p->si_hcurs_download = p9000_dac_cursor_download;
		functions_p->si_hcurs_turnon = p9000_dac_cursor_turnon;
		functions_p->si_hcurs_turnoff = p9000_dac_cursor_turnoff;
		functions_p->si_hcurs_move = p9000_dac_cursor_move;
		
		(*dac_functions_p->get_cursor_max_size_p)(
			&cursor_state_p->cursor_max_width,	
			&cursor_state_p->cursor_max_height);

		flags_p->SIcurswidth =
			cursor_state_p->cursor_max_width;
		flags_p->SIcursheight =
			cursor_state_p->cursor_max_height;

		cursor_state_p->dac_cursor_p = 
			allocate_and_clear_memory(sizeof(struct p9000_dac_cursor));


		bitmap_size = (( cursor_state_p->cursor_max_width *
			 cursor_state_p->cursor_max_height) / 8) * 2; /*bytes*/

		cursor_state_p->dac_cursor_p->cursor_bits_p  =
			allocate_and_clear_memory(bitmap_size); 

		screen_state_p->cursor_state_p 
			= cursor_state_p ;
	}

	cursor_state_p->number_of_cursors = flags_p->SIcurscnt;
	cursor_state_p->current_cursor_status.x = -1;
	cursor_state_p->current_cursor_status.y = -1;
	cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
	cursor_state_p->current_cursor_status.current_index = -1;
}

/*
 * VT restore  function
 */

function void
p9000_cursor__vt_switch_in__(void)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURSOR_STATE_DECLARE();

	/*
	 * If we are not using DAC cursor then return
	 */
	if (cursor_state_p->dac_cursor_p == NULL)
	{
		return;
	}

	/*
	 * The cursor RAM on the DAC could have changed, so force a download
	 * and a turnon
	 */

	if (cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		p9000_dac_cursor_turnon(0);
	}
	
}
