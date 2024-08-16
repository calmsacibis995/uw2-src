/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_cursor.c	1.5"

/***
 ***	NAME
 ***
 ***		mach_cursor.c : Cursor management  module
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_cursor.h"
 ***
 ***
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***		mach_cursor.c : Source.
 ***		mach_cursor.h : Interface.
 ***
 ***	SEE ALSO
 ***
 ***  		Programmer's Guide to the MACH 32 Registers
 ***  		SI definitions document
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

#ident	"@(#)mach:mach/m_cursor.c	1.2"

PUBLIC

/***
 *** 	Includes.
 ***/

#include <sidep.h>
#include "m_opt.h"
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

enum mach_cursor_status_kind
{
	CURSOR_ON,
	CURSOR_OFF
};


struct mach_cursor_status
{
	enum mach_cursor_status_kind flag ;	/*is cursor on/off*/
	int	current_index ;		/*index of the current cursor (cursor_index)*/
	int	x;							/*X co-ord of the cursor*/
	int	y;							/*Y co-ord of the cursor*/
#if defined ( __DEBUG__ )
	int stamp ;
#endif
} ;

struct offscreen_area
{
	int	x ;							
	int	y ;
};

/*
 *Software cursor structure
 */
struct software_cursor
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
struct mach_cursor_hardware_cursor
{

/*Most significant 4 bits of the addr of cursor location in offscreen mem */
	int	cursor_offset_hi ;			
/* lower order 16 bits of the location */
	int	cursor_offset_lo ;
	int horz_cursor_offset ;
	int vert_cursor_offset ;
	struct offscreen_area	memory_representation ;			
#if defined(__DEBUG__)
	int stamp ;
#endif
};

struct mach_cursor
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

	struct mach_cursor_hardware_cursor
		*hardware_cursor_p;

	struct software_cursor	
		*software_cursor_p;
	

#if defined(__DEBUG__)
	int stamp ;
#endif

};



/*
** Module state
*/
struct mach_cursor_state
{
	
	/* type of the cursor
		 MACH_CURSOR_HARDWARE or MACH_CURSOR_SOFTWARE*/
	int cursor_type;

	/*Max width of the hardware cursor */
	int hardware_cursor_max_width;	

	/*Max height */
	int hardware_cursor_max_height; 

	/*No of downloadable cursors*/
	int number_of_cursors ;			

	/*
	*The co-ords of the visible window (displayed area) with respect to
	*the virtual screen
	*/
	int	 visible_window_top_x;
	int	 visible_window_top_y;

	/*Status of the current cursor */
	struct mach_cursor_status current_cursor_status; 

	/* Pointer to cursors list */
	struct mach_cursor **cursor_list_pp;

	/* Pointer to omm descriptor*/
	struct omm_allocation * omm_block_p ;

	/* Byte swap function  for 4-bit mode */
	void (*swap_function_p) ( unsigned char *, int );


#if defined ( __DEBUG__)
	int stamp ;
#endif

};

/***
 ***	Variables.
 ***/

/*
**	Debugging variables.
*/

#if (defined(__DEBUG__))
export boolean mach_cursor_debug;
#endif

/*
*	Current module state.
*/

PRIVATE

/***
 *** 	Includes.
 ***/

#include <string.h>
#include "m_state.h"
#include "m_asm.h"
#include "g_omm.h"


/***
 ***	Constants.
 ***/

/*
 * We support two kinds of cursors
 */
#define	MACH_CURSOR_HARDWARE	0
#define	MACH_CURSOR_SOFTWARE	1

/*
 * status of the save area
 */
#define MACH_CURSOR_VALID_DATA	1
#define MACH_CURSOR_INVALID_DATA	0


#define MACH_CURSOR_BITBLT_LOCAL_BUFFER_SIZE\
	(2 * DEFAULT_MACH_MAX_FIFO_BLOCKING_FACTOR)

/*
 * Mask for reading/writing all planes
 */
#define MACH_CURSOR_ENABLE_ALL_PLANES\
	((1 << MACH_SCREEN_DEPTH()) - 1) 




#define MACH_CURSOR_STAMP \
	(( 'M' << 0 ) + ( 'A' << 1 ) + ( 'C' << 2 ) +\
	( 'H' << 3 ) + ( '_' << 4 ) + ( 'C' << 5 ) +\
	( 'U' << 6 ) + ( 'R' << 7 ) + ( 'S' << 8 ) +\
	( 'O' << 9 ) + ( 'R' << 10 ) + 0 )

#define MACH_CURSOR_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('C' << 5) + ('U' << 6) + ('R' << 7) +\
	 ('S' << 8) + ('O' << 9) + ('R' << 10) + ('_' << 11) +\
	 ('S' << 12) + ('T' << 13) + ('A' << 14) + ('T' << 15) +\
	 ('E' << 16) + ('_' << 17) + ('S' << 18) + ('T' << 19) +\
	 ('A' << 20) + ('M' << 21))

#define MACH_HARDWARE_CURSOR_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('H' << 5) + ('A' << 6) + ('R' << 7) +\
	 ('D' << 8) + ('W' << 9) + ('A' << 10) + ('R' << 11) +\
	 ('E' << 12) + ('_' << 13) + ('C' << 14) + ('U' << 15) +\
	 ('R' << 16) + ('S' << 17) + ('O' << 18))

#define MACH_SOFTWARE_CURSOR_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('S' << 5) + ('O' << 6) + ('F' << 7) +\
	 ('T' << 8) + ('W' << 9) + ('A' << 10) + ('R' << 11) +\
	 ('E' << 12) + ('_' << 13) + ('C' << 14) + ('U' << 15) +\
	 ('R' << 16) + ('S' << 17) + ('O' << 18))

#define	MACH_CURSOR_NAMED_ALLOCATION_ID	"CURSOR"

#define MACH_CURSOR_NAMED_ALLOCATION_STRING_TEMPLATE\
	MACH_CURSOR_NAMED_ALLOCATION_ID ":%d+%d+%d@%d+%d+%d"

#define	MACH_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN	200

/*
** Each pixel of the hardware cursor is represented by two bits
*/
#define MACH_HARDWARE_CURSOR_DEPTH	2

/***
 ***	Macros.
 ***/

#define	MACH_CURSOR_IS_PANNING_FEASIBLE()\
	 ((MACH_MAX_DISPLAYED_Y_PIXELS() < MACH_MAX_VIRTUAL_Y_PIXELS()) ||\
		(MACH_MAX_DISPLAYED_X_PIXELS() < MACH_MAX_VIRTUAL_X_PIXELS()))

#define MACH_HARDWARE_CURSOR_MAX_WIDTH()\
	(mach_cursor_state_p->hardware_cursor_max_width)

#define MACH_HARDWARE_CURSOR_MAX_HEIGHT()\
	(mach_cursor_state_p->hardware_cursor_max_height) 

#define MACH_MAX_DISPLAYED_X_PIXELS() \
	(generic_current_screen_state_p->screen_displayed_width) 

#define MACH_MAX_DISPLAYED_Y_PIXELS()\
	(generic_current_screen_state_p->screen_displayed_height) 

#define MACH_MAX_VIRTUAL_X_PIXELS() \
	(generic_current_screen_state_p->screen_virtual_width) 

#define MACH_MAX_VIRTUAL_Y_PIXELS()\
	(generic_current_screen_state_p->screen_virtual_height) 

#define MACH_MAX_PHYSICAL_X_PIXELS() \
	(generic_current_screen_state_p->screen_physical_width) 

#define MACH_MAX_PHYSICAL_Y_PIXELS()\
	(generic_current_screen_state_p->screen_physical_height) 

#define MACH_SCREEN_DEPTH()\
	(generic_current_screen_state_p->screen_depth) 

#define	MACH_CHIPSET_KIND()\
	(((struct mach_screen_state*)\
	  generic_current_screen_state_p)->\
	 chipset_kind) 

#define	MACH_CURSOR_CURSOR_STATE_DECLARE()\
	struct mach_cursor_state  *mach_cursor_state_p=\
	(screen_state_p)->cursor_state_p


#define	MACH_CURSOR_IS_VALID_INDEX(cursor_index)\
	(cursor_index >= 0) &&\
	(cursor_index<mach_cursor_state_p->number_of_cursors)

#define	MACH_CURSOR_CALCULATE_OFFSET_HI_LO(x,y,offset_hi, offset_lo)\
	{\
	int offset_in_mem;\
	if ( (y) !=  0 )\
	{\
			offset_in_mem=\
			((y)*(MACH_MAX_PHYSICAL_X_PIXELS()*MACH_SCREEN_DEPTH()))/32;\
			if ( (x) !=  0 )\
			{\
				offset_in_mem +=  ((x)*MACH_SCREEN_DEPTH())/32;\
			}\
	}\
	else\
	{\
			if ( (x) != 0 )\
			{\
				offset_in_mem =  ((x)*MACH_SCREEN_DEPTH())/32;\
			}\
			else\
			{\
				offset_in_mem = 0 ;\
			}\
	}\
	(offset_lo)= offset_in_mem & 0xFFFF ;\
	offset_in_mem = offset_in_mem >> 16 ;\
	(offset_hi) = offset_in_mem & 0xF ;\
}


#define	MACH_CURSOR_SET_CURSOR_COLOR( foreground_color, background_color )	  \
{																			  \
	if (screen_state_p->generic_state.screen_depth <= 8)					  \
	{																		  \
		mach_register_state_p->cursor_color_0 = (unsigned short)			  \
			(foreground_color);												  \
		mach_register_state_p->cursor_color_1 = (unsigned short)			  \
			(background_color);												  \
		outb(MACH_REGISTER_CURSOR_COLOR_0, foreground_color);				  \
		outb(MACH_REGISTER_CURSOR_COLOR_1, background_color);				  \
	}																		  \
	else																	  \
	{																		  \
		unsigned char fg_red_component, fg_green_component,					  \
			fg_blue_component;												  \
		unsigned char bg_red_component, bg_green_component,					  \
			bg_blue_component;												  \
 		SIVisual *si_visual_p =												  \
			screen_state_p->generic_state.screen_visuals_list_p[0].			  \
				si_visual_p;												  \
		fg_red_component = (foreground_color & si_visual_p->SVredmask) >>	  \
			si_visual_p->SVredoffset;										  \
		fg_green_component = (foreground_color & si_visual_p->SVgreenmask) >> \
			si_visual_p->SVgreenoffset;										  \
		fg_blue_component = (foreground_color & si_visual_p->SVbluemask) >>	  \
			si_visual_p->SVblueoffset;										  \
		bg_red_component = (background_color & si_visual_p->SVredmask) >>	  \
			si_visual_p->SVredoffset;										  \
		bg_green_component = (background_color & si_visual_p->SVgreenmask) >> \
			si_visual_p->SVgreenoffset;										  \
		bg_blue_component = (background_color & si_visual_p->SVbluemask) >>	  \
			si_visual_p->SVblueoffset;										  \
		outb(MACH_REGISTER_CURSOR_COLOR_0,									  \
			 mach_register_state_p->cursor_color_0 = fg_blue_component);	  \
		outb(MACH_REGISTER_CURSOR_COLOR_1,									  \
			 mach_register_state_p->cursor_color_1 = bg_blue_component);	  \
		outw(MACH_REGISTER_EXT_CURSOR_COLOR_0,								  \
			 mach_register_state_p->ext_cursor_color_0 =					  \
			 (fg_green_component | (fg_red_component << 8)));				  \
		outw(MACH_REGISTER_EXT_CURSOR_COLOR_1,								  \
			 mach_register_state_p->ext_cursor_color_1 =					  \
			 (bg_green_component | (bg_red_component << 8)));				  \
	}																		  \
}


#define	MACH_CURSOR_SET_OFFSET( h_offset, v_offset )\
{\
	mach_register_state_p->horz_cursor_offset =\
	(h_offset);\
	mach_register_state_p->vert_cursor_offset =\
	(v_offset);\
	outw(MACH_REGISTER_HORZ_CURSOR_OFFSET,h_offset);\
	outw(MACH_REGISTER_VERT_CURSOR_OFFSET,v_offset);\
}



#define	MACH_CURSOR_SET_OFFSET_HI_LO( offset_hi, offset_lo)\
{\
	mach_register_state_p->cursor_offset_lo =\
	(offset_lo);\
	mach_register_state_p->cursor_offset_hi =\
	(offset_hi);\
	outw(MACH_REGISTER_CURSOR_OFFSET_LO,\
		 offset_lo);\
	outw(MACH_REGISTER_CURSOR_OFFSET_HI,\
			offset_hi) ;\
}

#define	MACH_CURSOR_SET_CURSOR_ON()\
{\
	mach_register_state_p->cursor_offset_hi |=\
	MACH_CURSOR_OFFSET_HI_CURSOR_ENA;\
	outw(MACH_REGISTER_CURSOR_OFFSET_HI,\
			mach_register_state_p->cursor_offset_hi);\
}

#define	MACH_CURSOR_SET_CURSOR_OFF()\
{\
	mach_register_state_p->cursor_offset_hi &=\
	~MACH_CURSOR_OFFSET_HI_CURSOR_ENA;\
	outw(MACH_REGISTER_CURSOR_OFFSET_HI,\
			mach_register_state_p->cursor_offset_hi);\
}
#define	MACH_CURSOR_SET_CURSOR_POSN(x,y)\
{\
	mach_register_state_p->horz_cursor_posn =(unsigned short)(((x) >0)?(x):0);\
	mach_register_state_p->vert_cursor_posn =(unsigned short)(((y)>0)?(y):0);\
	outw(MACH_REGISTER_HORZ_CURSOR_POSN,\
		mach_register_state_p->horz_cursor_posn );\
	outw(MACH_REGISTER_VERT_CURSOR_POSN,\
			mach_register_state_p->vert_cursor_posn ) ;\
}

#define	MACH_CURSOR_SET_CRT_OFFSET_HI_LO(offset_hi,offset_lo)\
{\
	mach_register_state_p->crt_offset_hi = (unsigned short)(offset_hi);\
	mach_register_state_p->crt_offset_lo = (unsigned short)(offset_lo);\
	MACH_WAIT_FOR_FIFO(2);\
	outw(MACH_REGISTER_CRT_OFFSET_HI,(offset_hi));\
	outw(MACH_REGISTER_CRT_OFFSET_LO,(offset_lo));\
}





/***
 ***	Functions.
 ***/

#if defined(__DEBUG__)
STATIC void
print_bitmap ( SIbitmap *bm )	
{
	SIArray arr;
	SIint32 w ;
	int i,j;

	

	if(mach_cursor_debug)	
	{
		arr = bm->Bptr ;

		for ( i=0; i<bm->Bheight; ++i )
		{
			w = *arr ;
			for ( j= 0;  j <bm->Bwidth; ++j )
			{
				if ( w&1)
				{
					(void) fprintf ( debug_stream_p,"1");
				}
				else
				{
					(void) fprintf ( debug_stream_p,"0");
				}
				w  = w  >> 1 ;
			}
			(void) fprintf ( debug_stream_p,"\n");
		++arr ;
		}
		(void) fprintf ( debug_stream_p,"\n\n");
	}
}/*print_bitmap*/
#endif

/*
**mach_cursor_no_operation
*/

STATIC SIBool
mach_cursor_no_operation(void)
{
	return (SI_SUCCEED);
}/*mach_cursor_no_operation*/


/*
 * mach_cursor_screen_to_screen_bitblt
 * 
 * Used in saving the contents of saved areas from the displayed
 * screen and conversely for restoring the saved screen portions from
 * offscreen area to the displayed screen.
 */

STATIC void
mach_cursor_screen_to_screen_bitblt(int source_x, int source_y,
			int destination_x, int destination_y,
			int width, int height)
{
	unsigned short dp_config;
	MACH_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(!MACH_IS_IO_ERROR());
	
	if (width <= 0 || height <= 0 || 
		MACH_IS_X_OUT_OF_BOUNDS(source_x) ||
		MACH_IS_X_OUT_OF_BOUNDS(destination_x) ||
		MACH_IS_Y_OUT_OF_BOUNDS(source_y) || 
		MACH_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return;
	}

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	/*
	 * Set clip rectangle
	 */
	MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p,0,0,
					MACH_MAX_PHYSICAL_X_PIXELS(),
					MACH_MAX_PHYSICAL_Y_PIXELS());
	screen_state_p->generic_state.screen_current_clip =
	    GENERIC_CLIP_NULL;
	MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);


	/*
	 *set write and read masks
	 */
	MACH_STATE_SET_RD_MASK(screen_state_p,MACH_CURSOR_ENABLE_ALL_PLANES);
	MACH_STATE_SET_WRT_MASK(screen_state_p,MACH_CURSOR_ENABLE_ALL_PLANES);

	/*
	 * program the dp_config registers.
	 */
	dp_config = screen_state_p->dp_config_flags |
				MACH_DP_CONFIG_WRITE |
				MACH_DP_CONFIG_ENABLE_DRAW |
        		MACH_DP_CONFIG_LSB_FIRST |
				MACH_DP_CONFIG_FG_COLOR_SRC_BLIT;
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	MACH_STATE_SET_FG_ROP(screen_state_p,MACH_MIX_FN_PAINT);
	MACH_STATE_SET_BG_ROP(screen_state_p,MACH_MIX_FN_PAINT);

#if (defined(__DEBUG__))
	if (mach_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_cursor_screen_to_screen_bitblt){ \n"
"\tsource_x = %d\n"
"\tsource_y = %d\n"
"\tdestination_x = %d\n"
"\tdestination_y = %d\n"
"\twidth = %d\n"
"\theight = %d\n"
"\tdp_config = 0x%x\n"
"}\n",
					 source_x, source_y,
					 destination_x,
					 destination_y,
					 width, height,
					 dp_config);
	}
#endif
	
	
	mach_asm_move_screen_bits(source_x, source_y, destination_x,
		destination_y, width, height);
	
	/*
	 * This blit has destroyed the pattern register contents.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,MACH_INVALID_PATTERN_REGISTERS);

	ASSERT(!MACH_IS_IO_ERROR());
	
	
}/*mach_cursor_screen_to_screen_bitblt*/

/*
 * mach_cursor_memory_to_screen_stplblt
 *
 * Used for downloading the cursor bitmaps onto offscreen locations.
 * 
 */

STATIC int
mach_cursor_memory_to_screen_stplblt( SIbitmap *bitmap_p, int destination_x,
	int destination_y, int width,
	int height, unsigned short plane_mask)
{
	unsigned char *data_p;
	int	transfers_per_line;
	int	source_step;
	unsigned short dp_config;
	MACH_CURRENT_SCREEN_STATE_DECLARE();


#if defined(__DEBUG__)
	if( mach_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
"(mach_cursor_memory_to_screen_stplblt){\n"
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



	if (MACH_IS_X_OUT_OF_BOUNDS(destination_x) ||
	    MACH_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return 1;
	}

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	dp_config = screen_state_p->dp_config_flags|  
		(MACH_DP_CONFIG_MONO_SRC_HOST |
		 MACH_DP_CONFIG_READ_MODE_MONO_DATA|
		 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		 MACH_DP_CONFIG_ENABLE_DRAW | 
		 MACH_DP_CONFIG_WRITE);

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	MACH_STATE_SET_FG_ROP(screen_state_p,MACH_MIX_FN_PAINT);
	MACH_STATE_SET_BG_ROP(screen_state_p,MACH_MIX_FN_PAINT);

	ASSERT(!(plane_mask & (plane_mask - 1)));
	
	MACH_STATE_SET_WRT_MASK(screen_state_p, plane_mask);

	MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, (short)destination_x,
		(short)destination_y,
	    destination_x + width,
	    destination_y + height);
	screen_state_p->generic_state.screen_current_clip =
	    GENERIC_CLIP_NULL;
	MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);

	/*
	 * The foreground color is set to all 1's; the write mask ensures
     * that only one plane of offscreen memory gets updated.
	 */
	MACH_STATE_SET_FRGD_COLOR(screen_state_p, MACH_CURSOR_ENABLE_ALL_PLANES);
	MACH_STATE_SET_BKGD_COLOR(screen_state_p, 0);

	ASSERT(!MACH_IS_IO_ERROR());

	/*
	 * Calculate no of pixtrans words to be transferred per line
	 */
	width = (width + screen_state_p->pixtrans_width - 1) &
		~(screen_state_p->pixtrans_width - 1);
	transfers_per_line = width >>
		screen_state_p->pixtrans_width_shift;

	source_step = ((bitmap_p->Bwidth + 31) & ~31)  >> 3;
	data_p = (unsigned char*)bitmap_p->Bptr;

#if defined ( __DEBUG__ )
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_memory_to_screen_stplblt)\n{\n"
"\tdata_p = %p\n"
"\ttransfers_per_line = %d\n"
"source_step = %d\n"
"\n}\n",
			(void*)data_p,
			transfers_per_line,
			source_step);
	}
#endif


	/* 
	 * set the blit parameters that dont change for every line
	 * these are SRC_Y_DIR, CUR_X, DEST_X_START and DEST_Y_END
	 */
	MACH_WAIT_FOR_FIFO(6);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);			/* top to bottom */
	outw(MACH_REGISTER_CUR_X, destination_x);
	outw(MACH_REGISTER_DEST_X_START, destination_x);
	outw(MACH_REGISTER_DEST_X_END, (destination_x + width));
	outw(MACH_REGISTER_CUR_Y, destination_y);
	outw(MACH_REGISTER_DEST_Y_END, (destination_y + height)); 	
								/* stplblt initiatior */

	/*
	 * This code has been duplicated from
	 * m_bitblt.c:(mach_memory_to_screen_stplblt).
	 */
	MACH_WAIT_FOR_FIFO(16);
	
	switch(screen_state_p->bus_width)
	{
	case MACH_BUS_WIDTH_16:
		
		while (height--)
		{
			unsigned char *tmp_source_p = data_p; 
			unsigned char local_buffer[MACH_CURSOR_BITBLT_LOCAL_BUFFER_SIZE];

			register int tmp = transfers_per_line;

			ASSERT(MACH_CURSOR_BITBLT_LOCAL_BUFFER_SIZE >=
				   mach_graphics_engine_fifo_blocking_factor);
			
			/*
			 * Invert the stipple bits and pump
			 */
			while (tmp >= mach_graphics_engine_fifo_blocking_factor)
			{
				register int i;
				register unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert stipple bits
				 */
				for(i = 0; i <
					mach_graphics_engine_fifo_blocking_factor; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
					
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				/*
				 * pump to pixtrans
				 */
				MACH_WAIT_FOR_FIFO(mach_graphics_engine_fifo_blocking_factor);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 mach_graphics_engine_fifo_blocking_factor,
					 local_buffer);
				tmp -= mach_graphics_engine_fifo_blocking_factor;
			}

			/*
			 * Do whatever words remain.
			 */
			if (tmp > 0)
			{
				int i;
				unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert screen bits
				 */
				for(i = 0; i < tmp; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
					
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				MACH_WAIT_FOR_FIFO(tmp);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 tmp,
					 local_buffer);
			}
			
			data_p += source_step;
		}
		break;

	case MACH_BUS_WIDTH_8:

		while (height--)
		{
			unsigned char *tmp_source_p = data_p; 
			unsigned char local_buffer[MACH_CURSOR_BITBLT_LOCAL_BUFFER_SIZE];

			int tmp = transfers_per_line;

			ASSERT(MACH_CURSOR_BITBLT_LOCAL_BUFFER_SIZE >=
				   mach_graphics_engine_fifo_blocking_factor);
			
			/*
			 * Invert the stipple bits and pump
			 */
			while (tmp >= mach_graphics_engine_fifo_blocking_factor)
			{
				int i;
				unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert stipple bits
				 */
				for(i = 0; i <
					mach_graphics_engine_fifo_blocking_factor; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				/*
				 * pump to pixtrans
				 */
				MACH_WAIT_FOR_FIFO(mach_graphics_engine_fifo_blocking_factor);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 mach_graphics_engine_fifo_blocking_factor,
					 local_buffer);
				tmp -= mach_graphics_engine_fifo_blocking_factor;
			}

			/*
			 * Do whatever words remain.
			 */
			if (tmp > 0)
			{
				int i;
				unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert screen bits
				 */
				for(i = 0; i < tmp; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				MACH_WAIT_FOR_FIFO(tmp);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 tmp,
					 local_buffer);
			}
			
			data_p += source_step;
		}
		
		break;
	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}/*switch*/
	
	/*
	 * This blit has destroyed the pattern register contents.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,MACH_INVALID_PATTERN_REGISTERS);

	ASSERT(!MACH_IS_IO_ERROR());
	return 1;


}/*mach_cursor_memory_to_screen_stplblt*/


/*
**mach_cursor_screen_to_screen_stplblt
*/

STATIC void
mach_cursor_screen_to_screen_stplblt( int source_x, int source_y,
	int destination_x, int destination_y,
	int width, int height, int foreground_color,
	int background_color, unsigned short read_mask)
{
	unsigned short dp_config;
	unsigned short write_mask;
	int clip_ul_x;
	int clip_ul_y;
	MACH_CURRENT_SCREEN_STATE_DECLARE();

#if defined(__DEBUG__)
	if( mach_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
"(mach_cursor_screen_to_screen_stplblt){\n"
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
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(!MACH_IS_IO_ERROR());
	
	if (width <= 0 || height <= 0 || 
		MACH_IS_X_OUT_OF_BOUNDS(source_x) ||
		MACH_IS_X_OUT_OF_BOUNDS(destination_x) ||
		MACH_IS_Y_OUT_OF_BOUNDS(source_y) || 
		MACH_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return ;
	}

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

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
	
	if (destination_x + width > MACH_MAX_VIRTUAL_X_PIXELS())
	{
		width -= destination_x + width - MACH_MAX_VIRTUAL_X_PIXELS();
	}

	if (destination_y + height > MACH_MAX_VIRTUAL_Y_PIXELS())
	{
		height -= destination_y + height - MACH_MAX_VIRTUAL_Y_PIXELS();
	}
	/*
	 * Set the clip rectangle
	 */
	MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p,
					(short)clip_ul_x,(short)clip_ul_y,
					destination_x + width,
					destination_y + height);
	screen_state_p->generic_state.screen_current_clip =
	    GENERIC_CLIP_NULL;
	MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);


	/*
	 * program the dp_config register.
	 */
	dp_config = screen_state_p->dp_config_flags |
				MACH_DP_CONFIG_WRITE |
				MACH_DP_CONFIG_ENABLE_DRAW |
				MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR|
				MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR|
				MACH_DP_CONFIG_MONO_SRC_BLIT;
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

#if (defined(__DEBUG__))
	if (mach_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_cursor_screen_to_screen_stplblt){ \n"
"\tsource_x = %d\n"
"\tsource_y = %d\n"
"\tdestination_x = %d\n"
"\tdestination_y = %d\n"
"\twidth = %d\n"
"\theight = %d\n"
"\tdp_config = 0x%x\n"
"}\n",
					source_x, source_y, destination_x,
				    destination_y, width, height, dp_config);
	}
#endif
	
	MACH_STATE_SET_BG_ROP(screen_state_p, MACH_MIX_FN_LEAVE_ALONE);
	MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);

	MACH_STATE_SET_FRGD_COLOR(screen_state_p, foreground_color);

	/*
	 *set plane masks
	 */
	write_mask = MACH_CURSOR_ENABLE_ALL_PLANES;
	
	
	MACH_STATE_SET_RD_MASK(screen_state_p,read_mask); 
	MACH_STATE_SET_WRT_MASK(screen_state_p,write_mask); 

	mach_asm_move_screen_bits(source_x, source_y, destination_x,
							  destination_y, width, height);
	
	/*
	 * This blit has destroyed the pattern register contents.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,MACH_INVALID_PATTERN_REGISTERS);

	ASSERT(!MACH_IS_IO_ERROR());
	

}/*mach_cursor_screen_to_screen_stplblt*/


/*
**mach_cursor_software_new_cursor
*/

STATIC int 
mach_cursor_software_new_cursor(struct mach_cursor *active_cursor_p,
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
	unsigned char *tmp_src_p;
	unsigned char *tmp_invsrc_p;
	unsigned char *tmp_mask_p;
	unsigned char *tmp_result_src_p;
	unsigned char *tmp_result_invsrc_p;

#if defined ( __DEBUG__ )
	if ( mach_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
"(mach_cursor_software_new_cursor){\n"
"\tactive_cursor_p= %p\n"
"cursor_p= %p\n"
"}\n",
		    (void*)active_cursor_p,
		    (void*)cursor_p);
	}
#endif

	ASSERT((active_cursor_p != NULL)&&( cursor_p != NULL));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,active_cursor_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SOFTWARE_CURSOR,
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
	src_bitmap.Bptr = allocate_memory(total_bytes);
	invsrc_bitmap.Bptr = allocate_memory(total_bytes);
	
	tmp_mask_p = (unsigned char*)cursor_p->SCmask->Bptr;
	tmp_src_p = (unsigned char*)cursor_p->SCsrc->Bptr;
	tmp_invsrc_p = (unsigned char*)cursor_p->SCinvsrc->Bptr;
	tmp_result_src_p = (unsigned char*)src_bitmap.Bptr ;
	tmp_result_invsrc_p = (unsigned char*)invsrc_bitmap.Bptr ;
	
	/* 
	 * Create the new source and inverse source bitmaps.
	 */
	for(; total_bytes > 0; --total_bytes)
	{
		*tmp_result_src_p = *tmp_mask_p & *tmp_src_p;
		*tmp_result_invsrc_p = *tmp_mask_p & *tmp_invsrc_p;

		++tmp_result_src_p;
		++tmp_result_invsrc_p;
		++tmp_mask_p;
		++tmp_src_p;
		++tmp_invsrc_p;
	}

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
		if(mach_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
"(mach_cursor_software_new_cursor){\n"
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

		if (mach_cursor_memory_to_screen_stplblt(&src_bitmap,
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
		if(mach_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
"(mach_cursor_software_new_cursor){\n"
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

		if (mach_cursor_memory_to_screen_stplblt(&invsrc_bitmap,
			dx, dy,width,height,plane_mask) == 0)
		{
			OMM_UNLOCK(active_cursor_p->software_cursor_p->invsrc_bitmap_p);
			(void) omm_free(active_cursor_p->software_cursor_p->
							invsrc_bitmap_p);
			active_cursor_p->software_cursor_p->invsrc_bitmap_p =
				NULL;
			free_memory(invsrc_bitmap.Bptr);
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
		 MACH_SCREEN_DEPTH(), OMM_LONG_TERM_ALLOCATION);
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
		 MACH_CURSOR_INVALID_DATA;
#endif

	return 1;

}/*mach_cursor_software_new_cursor*/


/*
** mach_cursor_generate_hardware_cursor
*/
STATIC void
mach_cursor_generate_hardware_cursor(
	SIbitmap *mask_p,
	SIbitmap *src_p,
	unsigned short *result_p)
{
	int i;
	int j;
	int balance ;
	int word_cnt = 0;
	int	 shift=0 ;
	int src_word, mask_word;
	short result_word ;
	int bitmask ;
	SIArray src ;
	SIArray mask ;
	int cursor_width ;
	int inc ;
	int effective_width = 0 ;
	int effective_height = 0 ;
	unsigned short *resultant_p = result_p ;
	int no_of_words_per_cursor_line;

#if (defined(__DEBUG__))
	int	number_of_bytes ;
#endif

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();

	/* Each pixel in the resultant map is represented by two  bits
	**  src_bit		mask_bit		resultant_bits
	**  ----------------------------------------------------
	**	1			1				00 		Foreground color
	**	0			1				01		Background color
	**	0			0				10		Transparent
	**  ----------------------------------------------------
	*/

#define	FOREGROUND_COLOR	0x0000		

#define	BACKGROUND_COLOR	0x0001

#define TRANSPARENT			0x0002


	ASSERT((mask_p != NULL) && (src_p != NULL) && (result_p != NULL)) ;

	ASSERT((mask_p->Bwidth == src_p->Bwidth)&&\
			(mask_p->Bheight == src_p->Bheight) ) ;

	ASSERT((mask_p->BbitsPerPixel == 1) && (src_p->BbitsPerPixel == 1) );


#if defined(__DEBUG__)
	number_of_bytes = (((mask_p->Bwidth + 31 ) & ~31 ) >> 3)*mask_p->Bheight ;

	if	( mach_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
"(mach_cursor_generate_hardware_cursor){\n"
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

	if (mask_p->Bwidth > MACH_HARDWARE_CURSOR_MAX_WIDTH())
	{
		inc = (((mask_p->Bwidth + 31 ) & ~31 ) >> 3)/4;

		inc -= ((MACH_HARDWARE_CURSOR_MAX_WIDTH()/32)-1) ;

#if defined ( __DEBUG__ )
		if ( mach_cursor_debug )
		{
			(void)fprintf ( debug_stream_p,
"(mach_cursor_generate_hardware_cursor)\n"
"{\n"
"\t# Truncating cursor\n"
"\tinc = %d\n"
"}\n",
			    inc);
		}
#endif
	}

	effective_height = ((mask_p->Bheight>MACH_HARDWARE_CURSOR_MAX_HEIGHT())?
	    MACH_HARDWARE_CURSOR_MAX_HEIGHT():mask_p->Bheight);

	effective_width = ((mask_p->Bwidth>MACH_HARDWARE_CURSOR_MAX_WIDTH())?
	    MACH_HARDWARE_CURSOR_MAX_WIDTH():mask_p->Bwidth);

	no_of_words_per_cursor_line = (MACH_HARDWARE_CURSOR_MAX_WIDTH() * 
									MACH_HARDWARE_CURSOR_DEPTH)/16 ;

	if ((MACH_HARDWARE_CURSOR_MAX_WIDTH()*MACH_HARDWARE_CURSOR_DEPTH) % 16)
	{
		++no_of_words_per_cursor_line;
	}
	

	for (i = 1; i <= effective_height; ++i)
	{
		src_word = *src ;

		mask_word = *mask ;

		result_word = 0 ;

		word_cnt = 0 ;

		if (cursor_width < MACH_HARDWARE_CURSOR_MAX_WIDTH())
		{
			result_p += (((MACH_HARDWARE_CURSOR_MAX_WIDTH() - cursor_width) * 
						MACH_HARDWARE_CURSOR_DEPTH) / 16);

			balance = (((MACH_HARDWARE_CURSOR_MAX_WIDTH() - cursor_width) *
						MACH_HARDWARE_CURSOR_DEPTH) % 16);

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

			shift += MACH_HARDWARE_CURSOR_DEPTH;

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
			** each word(short) in resultant map can hold
			** only 8 pixels
			*/

			if ((shift == 16) && (j<effective_width) )
			{
				/*
				** resultant row spans over multiple words
				*/
				result_p[word_cnt] = result_word ;

				word_cnt += 1;

				shift = 0 ;

				result_word = 0 ;

			}
		}/*for*/

		result_p[word_cnt] = result_word ;

		if (cursor_width < MACH_HARDWARE_CURSOR_MAX_WIDTH())
		{
			result_p += no_of_words_per_cursor_line - 
						((MACH_HARDWARE_CURSOR_MAX_WIDTH() - cursor_width) * 
							MACH_HARDWARE_CURSOR_DEPTH) / 16;
		}
		else
		{
			result_p += no_of_words_per_cursor_line ;
		}

		if (mask_p->Bwidth > MACH_HARDWARE_CURSOR_MAX_WIDTH())
		{
			src += inc ;
			mask += inc ;
		}
		else
		{
			++src ;
			++mask ;
		}

	}/*for*/

	if ( mach_cursor_state_p->swap_function_p != NULL )
	{
		(*mach_cursor_state_p->swap_function_p)
		    ( (unsigned char *)resultant_p,
		    (MACH_HARDWARE_CURSOR_MAX_HEIGHT()*
		    MACH_HARDWARE_CURSOR_MAX_WIDTH()*MACH_HARDWARE_CURSOR_DEPTH)/8 ) ;
	}

}/*mach_cursor_generate_hardware_cursor*/


/*
**mach_cursor_memory_to_screen_copy_hardware_cursor
*/

STATIC void
mach_cursor_memory_to_screen_copy_hardware_cursor(unsigned short *source_p, 
									int destination_x, 
									int destination_y )
{

	int memory_height = 0 ;			
	unsigned short dp_config;		
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	int	 width = MACH_HARDWARE_CURSOR_MAX_WIDTH() ;
	int	 height = MACH_HARDWARE_CURSOR_MAX_HEIGHT() ;
	int src_width_in_screen_pixels;
	int numwords;
	int last_line;
	int src_total_pixels;
	int src_full_lines ;
	unsigned short wrt_mask_val;
	
	int	top_x;
	int top_y;
	int bot_x;
	int bot_y;


	ASSERT(source_p!=NULL);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
	    generic_current_screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

	ASSERT(!MACH_IS_IO_ERROR());


#if (defined(__DEBUG__))
	if (mach_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_cursor_memory_to_screen_copy_hardware_cursor)\n"
"{\n"
"\tsource_p = %p\n"
"\tdestin_x = %d\n"
"\tdestin_y = %d\n"
"\twidth = %d\n"
"\theight = %d\n"
"}\n",
		    (void *) source_p,
		    destination_x,
		    destination_y,
		    width,
		    height);
	}
#endif


	if (MACH_IS_X_OUT_OF_BOUNDS(destination_x) ||
	    MACH_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return;
	}

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	dp_config = screen_state_p->dp_config_flags|  
		(MACH_DP_CONFIG_FG_COLOR_SRC_HOST |
		 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		 MACH_DP_CONFIG_ENABLE_DRAW | 
		 MACH_DP_CONFIG_MONO_SRC_ONE |
		 MACH_DP_CONFIG_WRITE);

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	MACH_STATE_SET_FG_ROP(screen_state_p,MACH_MIX_FN_PAINT);

	
	wrt_mask_val = MACH_CURSOR_ENABLE_ALL_PLANES;
	MACH_STATE_SET_WRT_MASK(screen_state_p, wrt_mask_val);

	src_width_in_screen_pixels =  (width * MACH_HARDWARE_CURSOR_DEPTH) /
									 (MACH_SCREEN_DEPTH());

	src_total_pixels = src_width_in_screen_pixels * height ;

	src_full_lines = src_total_pixels / MACH_MAX_PHYSICAL_X_PIXELS();

	last_line = src_total_pixels % MACH_MAX_PHYSICAL_X_PIXELS();

	if ((last_line == 0) && (src_full_lines > 0))
	{
		last_line = MACH_MAX_PHYSICAL_X_PIXELS ();

		src_full_lines--;
	}

	/*
	 * Memory height and screen height are the same
	 */

	memory_height = 
		(src_full_lines + (( last_line > 0 ) ? 1 : 0));

	/*
	 * Total number of words
	 */

	numwords = (src_total_pixels*MACH_SCREEN_DEPTH())/16;

	/*
	 * Number of pixtrans words per line
	 */

	numwords = numwords / memory_height;

	/*
	 * Workaround for 1280x1024 mode bug, the chipset goes into a
	 * vague state when the clip rectangle is set and not restored
	 * while downloading the cursor.
	 * Save the initial clip rectangle coordinates and restore them
	 * while exiting from the function
	 */ 
	
    top_x =	screen_state_p->register_state.ext_scissor_l ;
	top_y = screen_state_p->register_state.ext_scissor_t;
	bot_x = screen_state_p->register_state.ext_scissor_r + 1;
	bot_y = screen_state_p->register_state.ext_scissor_b + 1;

	MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 
		(unsigned short) destination_y,
	    (unsigned short) MACH_MAX_PHYSICAL_X_PIXELS(),
	    (unsigned short) destination_y + memory_height);

	screen_state_p->generic_state.screen_current_clip =
	    GENERIC_CLIP_NULL;

	MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);

	ASSERT(!MACH_IS_IO_ERROR());


#if defined ( __DEBUG__ )
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_memory_to_screen_copy_hardware_cursor)\n{\n"
"\tSource_p= %p\n"
"\tdp_config=0x%x\n"
"\tscreen depth= %d\n"
"\tnumwords= %d\n"
"\tsrc_width_in_screen_pixels = %d\n"
"\tsrc_total_pixels = %d\n"
"\tfull_lines = %d\n"
"\tlast_line= %d\n"
"\tmemory_height= %d\n"
"\n}\n",
		    (void*)source_p,
		    dp_config,
		    MACH_SCREEN_DEPTH(),
		    numwords,
		    src_width_in_screen_pixels,
		    src_total_pixels,
		    src_full_lines,
		    last_line,
		    memory_height);
	}
#endif

	/*
	 * Call the helper to transfer bits to offscreen memory.
	 */
	mach_asm_transfer_helper (
	    (unsigned char*)source_p,
	    numwords*2,
	    numwords,
	    memory_height,
	    destination_x,
	    destination_y,
	    MACH_MAX_PHYSICAL_X_PIXELS(),
	    memory_height,
	    screen_state_p->screen_write_pixels_p,
	    screen_state_p->pixtrans_register,
	    MACH_ASM_TRANSFER_TO_VIDEO_MEMORY
	    );

	/*
	 * This blit has destroyed the pattern register contents.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);

	ASSERT(!MACH_IS_IO_ERROR());
	/*
	 * Restore the clip rectangle to the original  one
	 */
	MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 
	    (unsigned short) top_x, 
		(unsigned short) top_y,
	    (unsigned short) bot_x,
	    (unsigned short) bot_y);


}/*mach_cursor_memory_to_screen_copy_hardware_cursor*/

/*
**mach_cursor_hardware_new_cursor
*/
STATIC int 
mach_cursor_hardware_new_cursor(struct mach_cursor *active_cursor_p,
					 SICursor *si_cursor_p)
{
	int height ;
	int	width ;
	int dx ;
	int dy ;
	unsigned short *hardware_cursor_p ;

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	MACH_CURSOR_CURSOR_STATE_DECLARE();

#if defined ( __DEBUG__ )
	if ( mach_cursor_debug )
	{
		(void)fprintf	( debug_stream_p,
"(mach_cursor_hardware_new_cursor){\n"
"\tactive_cursor_p= %p\n"
"\tsi_cursor_p= %p}\n",
		    (void*)active_cursor_p,
		    (void*)si_cursor_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p));
	ASSERT((active_cursor_p != NULL) && ( si_cursor_p != NULL ));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR, active_cursor_p));

	height = si_cursor_p->SCheight ;

	width = si_cursor_p->SCwidth ;


	/*
	 * The representation of the HW cursor in the offscreen mem is
	 * different. 
	 * Two bits represent one pixel
	 */


	hardware_cursor_p = allocate_memory
			 ((MACH_HARDWARE_CURSOR_MAX_WIDTH() * MACH_HARDWARE_CURSOR_DEPTH *
			  MACH_HARDWARE_CURSOR_MAX_HEIGHT())/8);
	/*
	** Set all pixels to transparent
	*/
	(void) memset(hardware_cursor_p,0xaa,((MACH_HARDWARE_CURSOR_MAX_WIDTH() * 
									MACH_HARDWARE_CURSOR_DEPTH *
									MACH_HARDWARE_CURSOR_MAX_HEIGHT())  / 8));

	mach_cursor_generate_hardware_cursor(si_cursor_p->SCmask, 
										 si_cursor_p->SCsrc,
										 hardware_cursor_p);

	active_cursor_p->cursor_height = 
	    ((height > MACH_HARDWARE_CURSOR_MAX_HEIGHT() ) ?
		 MACH_HARDWARE_CURSOR_MAX_HEIGHT() : height);

	active_cursor_p->cursor_width = 
	    ((width > MACH_HARDWARE_CURSOR_MAX_WIDTH() ) ?
		 MACH_HARDWARE_CURSOR_MAX_WIDTH() : width);

	active_cursor_p->foreground_color =
		si_cursor_p->SCfg;

	active_cursor_p->background_color =
		si_cursor_p->SCbg;

	dx = active_cursor_p->hardware_cursor_p->memory_representation.x;

	dy = active_cursor_p->hardware_cursor_p->memory_representation.y;

	MACH_CURSOR_CALCULATE_OFFSET_HI_LO(dx, dy, 
		active_cursor_p->hardware_cursor_p->cursor_offset_hi,
	    active_cursor_p-> hardware_cursor_p->cursor_offset_lo);

	if (si_cursor_p->SCheight > MACH_HARDWARE_CURSOR_MAX_HEIGHT())
	{
		active_cursor_p->hardware_cursor_p->vert_cursor_offset =  0;
	}
	else
	{
		active_cursor_p->hardware_cursor_p->vert_cursor_offset =  
		    MACH_HARDWARE_CURSOR_MAX_HEIGHT() - si_cursor_p->SCheight;
	}

	if (si_cursor_p->SCwidth > MACH_HARDWARE_CURSOR_MAX_WIDTH())
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset =
		    0;
	}
	else
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset =
		    MACH_HARDWARE_CURSOR_MAX_WIDTH() - si_cursor_p->SCwidth;
	}

	mach_cursor_memory_to_screen_copy_hardware_cursor(
							  hardware_cursor_p, dx, dy);

	free_memory (hardware_cursor_p) ;

	return 1;
}/*mach_cursor_hardware_new_cursor*/

/*
**mach_cursor_new_cursor
*/

STATIC int
mach_cursor_new_cursor(int cursor_index, SICursorP cursor_p)
{
	struct mach_cursor *active_cursor_p ;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE() ;
	int mach_cursor_hardware_new_cursor ( struct mach_cursor *active_cursor_p,
		 SICursor *si_cursor_p );

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_new_cursor){\n"
"\tcursor_index= %d\n"
"\tcursor_p= %p}\n",
		    cursor_index,
		    (void*)cursor_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p));
	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));
	ASSERT(cursor_p != NULL);
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR, active_cursor_p));

	if (mach_cursor_state_p->cursor_type == MACH_CURSOR_SOFTWARE)
	{
		return mach_cursor_software_new_cursor(active_cursor_p, cursor_p);
	}
	else
	{
		return mach_cursor_hardware_new_cursor(active_cursor_p, cursor_p);
	}
}/*mach_cursor_new_cursor*/


/*
**mach_cursor_save_obscured_screen 
*/

STATIC int
mach_cursor_save_obscured_screen(int cursor_index)  
{
	int dx;
	int dy;
	int height;
	int width;
	int x1;
	int y1;
	int x2;
	int y2;
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	struct mach_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
"(mach_cursor_save_obscured_screen){"
"\n"
"\tcursor_index= %d\n}\n",
		cursor_index);
	}
#endif
	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,active_cursor_p));

	/*
	 * determine the co-ordinates of the rectangular area that
	 * has to be saved: (x1,y1,x2,y2)
	 */
	x1 = mach_cursor_state_p->current_cursor_status.x;
	y1 = mach_cursor_state_p->current_cursor_status.y;
	x2 = mach_cursor_state_p->current_cursor_status.x +
		active_cursor_p->cursor_width;
	y2 = mach_cursor_state_p->current_cursor_status.y +
		active_cursor_p->cursor_height;

	/*
	 * Check if it is outside the visible area and adjust co-ords
	 * if required
	 */
	 x2 = (x2 > MACH_MAX_VIRTUAL_X_PIXELS()) ?
			 MACH_MAX_VIRTUAL_X_PIXELS() : x2;
	 y2 = (y2 > MACH_MAX_VIRTUAL_Y_PIXELS()) ?
			 MACH_MAX_VIRTUAL_Y_PIXELS() : y2;

	/*
	 * Calculate width and height of the rectangular save area
	 */
	width = x2 - x1;
	height = y2 - y1;
	active_cursor_p->software_cursor_p->saved_width = width;
	active_cursor_p->software_cursor_p->saved_height = height;

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_save_obscured_screen){\n"
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
	
	if(active_cursor_p->software_cursor_p->save_area_p  == NULL)
	{
		return 0;
	}

	if (OMM_LOCK(active_cursor_p->software_cursor_p->save_area_p))  
	{
		dx = active_cursor_p->software_cursor_p->save_area_p->x;  
		dy = active_cursor_p->software_cursor_p->save_area_p->y;  
		mach_cursor_screen_to_screen_bitblt(x1, y1, dx,dy,width,height);
		OMM_UNLOCK(active_cursor_p->software_cursor_p->save_area_p);

#if defined(__DEBUG__)
		active_cursor_p->software_cursor_p->valid_data =
			 MACH_CURSOR_VALID_DATA;
#endif
		return 1;
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}
}/*mach_cursor_save_obscured_screen*/


/*
**mach_cursor_restore_obscured_screen 
*/
STATIC int
mach_cursor_restore_obscured_screen ( int cursor_index )
{
	int sx;
	int sy;
	int	height;
	int width;
	int	x;
	int	y;
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	struct mach_cursor *active_cursor_p;

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_restore_obscured_screen){\n"
"\tcursor_index= %d\n"
"}\n",
		cursor_index);
	 }
#endif

	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,active_cursor_p));
	ASSERT(active_cursor_p->software_cursor_p->valid_data ==
		 MACH_CURSOR_VALID_DATA);

	height = active_cursor_p->software_cursor_p->saved_height; 
	width = active_cursor_p->software_cursor_p->saved_width; 
	if (OMM_LOCK(active_cursor_p->software_cursor_p->save_area_p))
	{
		sx = active_cursor_p->software_cursor_p->save_area_p->x;
		sy = active_cursor_p->software_cursor_p->save_area_p->y;
		x = mach_cursor_state_p->current_cursor_status.x;
		y = mach_cursor_state_p->current_cursor_status.y;
		mach_cursor_screen_to_screen_bitblt(sx, sy, x, y, width,height);
#if (defined(__DEBUG__))
		active_cursor_p->software_cursor_p->valid_data =
			 MACH_CURSOR_INVALID_DATA;
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
}/*mach_cursor_restore_obscured_screen*/


/*
**mach_cursor_draw_software_cursor 
*/
STATIC int
mach_cursor_draw_software_cursor(int cursor_index)
{
	int sx;
	int sy;
	int width;
	int height;
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	struct mach_cursor *active_cursor_p;
	unsigned short plane_mask;

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_draw_software_cursor){\n"
"\tcursor_index= %d\n}\n",
		cursor_index);
	}
#endif

	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,active_cursor_p));

	width = active_cursor_p->cursor_width; 
	height = active_cursor_p->cursor_height; 

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_draw_software_cursor){\n"
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
		mach_cursor_screen_to_screen_stplblt(
						sx, sy,
						mach_cursor_state_p->current_cursor_status.x,
						mach_cursor_state_p->current_cursor_status.y, 
						width,
						height,
						active_cursor_p->foreground_color,
						active_cursor_p->background_color,
						plane_mask
						);
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

		mach_cursor_screen_to_screen_stplblt(
						sx, sy,mach_cursor_state_p->current_cursor_status.x,
						mach_cursor_state_p->current_cursor_status.y, 
						width,
						height,
						active_cursor_p->background_color,
						active_cursor_p->foreground_color,
						plane_mask
						);
		OMM_UNLOCK(active_cursor_p->software_cursor_p->invsrc_bitmap_p);
		return 1;
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return 0;
	}
}/*mach_cursor_draw_software_cursor*/

/*
** mach_cursor_software_turnon  
*/
STATIC SIBool
mach_cursor_software_turnon(SIint32 cursor_index)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct mach_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf(debug_stream_p,
"(mach_cursor_software_turnon){\n"
"\tcursor_index= %d\n"
"}\n",
		(int)cursor_index);
	 }
#endif
	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,active_cursor_p));
#endif

	if (mach_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		return SI_SUCCEED;
	}
	if (mach_cursor_save_obscured_screen(cursor_index) == 0)
	{
		return SI_FAIL;
	}
	if (mach_cursor_draw_software_cursor(cursor_index) == 0)
	{
		return SI_FAIL;
	}

	mach_cursor_state_p->current_cursor_status.flag = CURSOR_ON;
	mach_cursor_state_p->current_cursor_status.current_index = cursor_index;

	return SI_SUCCEED;
}/*mach_cursor_software_turnon*/


/*
 * mach_cursor_software_turnoff
 */
STATIC SIBool
mach_cursor_software_turnoff(SIint32 cursor_index)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();

#if (defined(__DEBUG__))
	struct mach_cursor *active_cursor_p;
#endif

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_software_turnoff){\n"
"\tcursor_index= %d\n}\n",
		(int)cursor_index);
	 }
#endif

	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));

#if (defined(__DEBUG__))
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,active_cursor_p));
#endif

	if (mach_cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		return SI_SUCCEED;
	}
	else
	{
		mach_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
	}

 	if (mach_cursor_restore_obscured_screen(cursor_index) == 0)
	{
		return SI_FAIL;
	}
	else
	{
		return SI_SUCCEED;
	}
}/*mach_cursor_software_turnoff*/


/*
**mach_cursor_software_move
*/

STATIC SIBool
mach_cursor_software_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	struct mach_register_state *mach_register_state_p =
		&(screen_state_p->register_state);
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	struct mach_cursor *active_cursor_p;
	int	cursor_width;
	int	cursor_height;
	int cursor_turned_off = 0;
	int	crtoffset_change_required = 0;

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf( debug_stream_p,
"(mach_cursor_software_move){\n"
"\tcursor_index= %d\n"
"\tx = %d\n"
"\ty= %d\n}\n",
		(int)cursor_index,
		(int)x, (int)y);
	 }
#endif

		
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index];
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,active_cursor_p));

	if (mach_cursor_state_p->current_cursor_status.flag == CURSOR_OFF)
	{
		mach_cursor_state_p->current_cursor_status.x = x;
		mach_cursor_state_p->current_cursor_status.y = y;
		return SI_SUCCEED;
	}

	/*
	 * Check if panning will be needed 
	 */
	if (MACH_CURSOR_IS_PANNING_FEASIBLE())
	{
		cursor_height = active_cursor_p->cursor_height;
		cursor_width  = active_cursor_p->cursor_width;

		if ( x + cursor_width > 
				mach_cursor_state_p->visible_window_top_x +
					 MACH_MAX_DISPLAYED_X_PIXELS())
		{
			mach_cursor_state_p->visible_window_top_x = 
				(x + cursor_width > MACH_MAX_VIRTUAL_X_PIXELS()) ?
					(MACH_MAX_VIRTUAL_X_PIXELS() -
						 MACH_MAX_DISPLAYED_X_PIXELS())	:
					(x + cursor_width - MACH_MAX_DISPLAYED_X_PIXELS());
			crtoffset_change_required = 1;
		}
		else
		{
			if (x < mach_cursor_state_p->visible_window_top_x &&
					mach_cursor_state_p->visible_window_top_x > 0)
			{
				if ( x >= 0)
				{
					mach_cursor_state_p->visible_window_top_x = x;
				}
				else
				{
					mach_cursor_state_p->visible_window_top_x =0;
				}
					
				crtoffset_change_required = 1;
			}
		}
		if ( y + cursor_height > 
				mach_cursor_state_p->visible_window_top_y +
					 MACH_MAX_DISPLAYED_Y_PIXELS())
		{
			mach_cursor_state_p->visible_window_top_y = 
				(y + cursor_height > MACH_MAX_VIRTUAL_Y_PIXELS()) ?
					(MACH_MAX_VIRTUAL_Y_PIXELS() -
						 MACH_MAX_DISPLAYED_Y_PIXELS())	:
					(y + cursor_height - MACH_MAX_DISPLAYED_Y_PIXELS());

			crtoffset_change_required = 1;
		}
		else
		{
			if (y < mach_cursor_state_p->visible_window_top_y &&
					mach_cursor_state_p->visible_window_top_y > 0)
			{

				if ( y >= 0)
				{
					mach_cursor_state_p->visible_window_top_y = y;
				}
				else
				{
					mach_cursor_state_p->visible_window_top_y =0;
				}
				crtoffset_change_required = 1;
			}
		}
		if (crtoffset_change_required == 1)
		{
			int offset_hi;
			int offset_lo;

			ASSERT( (mach_cursor_state_p->visible_window_top_x >= 0) &&
				(mach_cursor_state_p->visible_window_top_y >= 0));

			MACH_CURSOR_CALCULATE_OFFSET_HI_LO(
				mach_cursor_state_p->visible_window_top_x,
				mach_cursor_state_p->visible_window_top_y,
				offset_hi,
				offset_lo);
#if defined(__DEBUG__)
			if (mach_cursor_debug)
			{
				(void)fprintf(debug_stream_p,
"(mach_cursor_software_move){\n"
"\twindow_top_x = %d\n"
"\twindow_top_y = %d\n"
"}",
				mach_cursor_state_p->visible_window_top_x,
				mach_cursor_state_p->visible_window_top_y);
			}
#endif
						
		 	if( (mach_register_state_p->crt_offset_hi !=
				 	(unsigned short)offset_hi)|| 
	 		 	(mach_register_state_p->crt_offset_lo !=
					 	(unsigned short)offset_lo))
		 	{
				MACH_WAIT_FOR_VERTICAL_BLANK();
				MACH_CURSOR_SET_CRT_OFFSET_HI_LO(offset_hi,offset_lo);
				(void) mach_cursor_software_turnoff(cursor_index);
				cursor_turned_off =1;
			}
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

	mach_cursor_state_p->current_cursor_status.x = x;
	mach_cursor_state_p->current_cursor_status.y = y;
	mach_cursor_state_p->current_cursor_status.current_index = cursor_index;

	return (*active_cursor_p->turnon_function_p)(cursor_index);
} /*mach_cursor_software_move*/


/*
**mach_cursor_software_download
*/


STATIC SIBool
mach_cursor_software_download( SIint32 cursor_index, SICursorP cursor_p )
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	struct mach_cursor  *active_cursor_p;
	

#if defined(__DEBUG__)
	if (mach_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_software_download){\n"
"\tcursor_index= %d\n"
"\tcursor_p= %p\n"
"\tcursor_width= %d\n"
"\tcursor_height= %d\n}\n",
		(int)cursor_index,
		(void*)cursor_p,
		(int)cursor_p->SCwidth,
		(int)cursor_p->SCheight);
	}
#if 0
		print_bitmap(cursor_p->SCsrc);
		print_bitmap(cursor_p->SCmask);
		print_bitmap(cursor_p->SCinvsrc);
#endif
	
	
#endif
	ASSERT ( MACH_CURSOR_IS_VALID_INDEX(cursor_index));
	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR, active_cursor_p));

	/*
	 * If the cursor being downloaded is currently in use and turned ON
	 * then turn it OFF, download the new cursor and then turn it ON
	 */
	if (mach_cursor_state_p->current_cursor_status.current_index ==
		 cursor_index 
		 && mach_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		(*active_cursor_p->turnoff_function_p)(cursor_index);

		if (mach_cursor_new_cursor(cursor_index, cursor_p) == 0 )
		{
			return SI_FAIL;
		}
		return (*active_cursor_p->turnon_function_p)(cursor_index);
	}
	else
	{
		return ((mach_cursor_new_cursor(cursor_index,cursor_p) == 0)
				? SI_FAIL : SI_SUCCEED);
	}
}/*mach_cursor_software_download*/


/*
 * mach_cursor_swap_bytes.
 *
 * In 4 bit mode the cursor bits need to be written in a fashion
 * different from that given in the programmers guide. 
 */
STATIC
void
mach_cursor_swap_bytes(unsigned char * bytes_p, int nbytes)
{
	int i ;
	unsigned char tmp ;

	/*
	** 4 bit mode: byte ordering of the downloaded cursor bitmap
	** | 1 | 3 | 2 | 4 | 5 | 7 |  6 | 8 | 9 |......
	*/

# if defined ( __DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_swap_bytes)"
"{\n"
"\tbytes_p= %p\n"
"\tnbytes= %d\n"
"}\n",
		    (void*)bytes_p,
		    nbytes);
	}
#endif

	for ( i=1; i<nbytes; i += 4 )
	{
		tmp = bytes_p[i] ;
		bytes_p[i] =bytes_p[i+1];
		bytes_p[i+1]= tmp ;
	}

}/*mach_cursor_swap_bytes*/





/*
**mach_cursor_draw_hardware_cursor
*/

STATIC int
mach_cursor_draw_hardware_cursor(int cursor_index)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();

	struct mach_register_state *mach_register_state_p =
		&(screen_state_p->register_state);
	MACH_CURSOR_CURSOR_STATE_DECLARE();

	struct mach_cursor *active_cursor_p ;


#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_draw_hardware_cursor){\n"
"\tcursor_index= %d\n"
"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p));
	
	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));

	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR, active_cursor_p));


	MACH_CURSOR_SET_CURSOR_COLOR(
	    active_cursor_p->foreground_color,\
		active_cursor_p->background_color);

	MACH_CURSOR_SET_OFFSET(active_cursor_p->hardware_cursor_p->\
				   horz_cursor_offset,
				   active_cursor_p->hardware_cursor_p->vert_cursor_offset);

	MACH_CURSOR_SET_OFFSET_HI_LO(active_cursor_p->hardware_cursor_p->\
			 cursor_offset_hi,
			 active_cursor_p->hardware_cursor_p->cursor_offset_lo);

	MACH_CURSOR_SET_CURSOR_ON();

	MACH_CURSOR_SET_CURSOR_POSN(mach_cursor_state_p->current_cursor_status.x,\
								mach_cursor_state_p->current_cursor_status.y);

#if defined ( __DEBUG__ )
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_draw_hardware_cursor){\n"
"\tcursor_offset_hi= 0x%x\n"
"\tcusor_offset_lo= 0x%x\n"
"\tvert_cursor_offset= %d\n"
"\thorz_cursor_offset= %d\n"
"\thorz_cursor_posn= %d\n"
"\tvert_cursor_posn= %d\n"
"\tforeground_color = %d\n"
"\tbackground_color = %d\n"
"}\n",
		    mach_register_state_p->cursor_offset_hi,
		    mach_register_state_p->cursor_offset_lo,
		    mach_register_state_p->vert_cursor_offset,
		    mach_register_state_p->horz_cursor_offset,
		    mach_register_state_p->horz_cursor_posn,
		    mach_register_state_p->vert_cursor_posn,
		    mach_register_state_p->cursor_color_0,
		    mach_register_state_p->cursor_color_1);
	}
#endif
	return 1 ;
}/*mach_cursor_draw_hardware_cursor*/



/*
**mach_cursor_hardware_turnon  
*/
STATIC SIBool
mach_cursor_hardware_turnon ( SIint32 cursor_index )
{

	MACH_CURRENT_SCREEN_STATE_DECLARE();

	MACH_CURSOR_CURSOR_STATE_DECLARE ();


#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_hardware_turnon){\n"
"\tcursor_index= %ld\n"
"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p));
	
	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));

	if (mach_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		return SI_SUCCEED ;
	}

	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,
		 mach_cursor_state_p->cursor_list_pp[cursor_index]));

	if (mach_cursor_draw_hardware_cursor(cursor_index) == 0)
	{
		return SI_FAIL ;
	}
	mach_cursor_state_p->current_cursor_status.flag = CURSOR_ON ;

	mach_cursor_state_p->current_cursor_status.current_index = cursor_index ;

	return SI_SUCCEED ;
}/*mach_cursor_hardware_turnon*/

/*
**mach_cursor_hardware_turnoff
*/
STATIC SIBool
mach_cursor_hardware_turnoff ( SIint32 cursor_index )
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	struct mach_register_state *mach_register_state_p =
		&(screen_state_p->register_state);
	MACH_CURSOR_CURSOR_STATE_DECLARE ();


#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_hardware_turnoff){\n"
"\tcursor_index= %ld\n"
"}\n",
		    cursor_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p));
	
	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));

	/*
	 * Older X servers, repeatedly call cursor_off ...
	 */
	ASSERT(screen_state_p->generic_state.screen_server_version_number <
		   X_SI_VERSION1_1 || 
		   mach_cursor_state_p->current_cursor_status.flag != CURSOR_OFF);

	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR,
		mach_cursor_state_p->cursor_list_pp[cursor_index]));

	mach_cursor_state_p->current_cursor_status.flag = CURSOR_OFF ;
	/*
	 ** disable cursor 
	 */
	MACH_CURSOR_SET_CURSOR_OFF();

	return SI_SUCCEED ;
}/*mach_cursor_hardware_turnoff*/

/*
**mach_cursor_hardware_move
*/
STATIC SIBool
mach_cursor_hardware_move(SIint32 cursor_index, SIint32 x, SIint32 y)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	struct mach_register_state *mach_register_state_p =
		&(screen_state_p->register_state);
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	struct mach_cursor *active_cursor_p;
	int oldx ;
	int oldy ;
	int cursor_height ;
	int cursor_width;
	int crtoffset_change_required = 0;

	oldx = mach_cursor_state_p->current_cursor_status.x ;
	oldy = mach_cursor_state_p->current_cursor_status.y ;

	mach_cursor_state_p->current_cursor_status.x = x ;
	mach_cursor_state_p->current_cursor_status.y = y ;

#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_hardware_move){\n"
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

	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index] ;
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR, active_cursor_p));

	/*
	 * Check if panning will be needed 
	 */
	if (MACH_CURSOR_IS_PANNING_FEASIBLE())
	{
		cursor_height = active_cursor_p->cursor_height;
		cursor_width  = active_cursor_p->cursor_width;

		if ( x + cursor_width > 
				mach_cursor_state_p->visible_window_top_x +
					 MACH_MAX_DISPLAYED_X_PIXELS())
		{
			mach_cursor_state_p->visible_window_top_x = 
				(x + cursor_width > MACH_MAX_VIRTUAL_X_PIXELS()) ?
					(MACH_MAX_VIRTUAL_X_PIXELS() -
						 MACH_MAX_DISPLAYED_X_PIXELS())	:
					(x + cursor_width - MACH_MAX_DISPLAYED_X_PIXELS());
			x -=  mach_cursor_state_p->visible_window_top_x;
			mach_cursor_state_p->current_cursor_status.x = x ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (x < mach_cursor_state_p->visible_window_top_x &&
					mach_cursor_state_p->visible_window_top_x > 0)
			{
				if ( x >= 0)
				{
					mach_cursor_state_p->visible_window_top_x = x;
					x = 0;
					mach_cursor_state_p->current_cursor_status.x = x ;
				}
				else
				{
					x += mach_cursor_state_p->visible_window_top_x; 
					mach_cursor_state_p->current_cursor_status.x = x ;
					mach_cursor_state_p->visible_window_top_x =0;
				}
					
				crtoffset_change_required = 1;
			}
			else
			{
				x -= mach_cursor_state_p->visible_window_top_x; 
				mach_cursor_state_p->current_cursor_status.x = x ;
			}
		}
		if ( y + cursor_height > 
				mach_cursor_state_p->visible_window_top_y +
					 MACH_MAX_DISPLAYED_Y_PIXELS())
		{
			mach_cursor_state_p->visible_window_top_y = 
				(y + cursor_height > MACH_MAX_VIRTUAL_Y_PIXELS()) ?
					(MACH_MAX_VIRTUAL_Y_PIXELS() -
						 MACH_MAX_DISPLAYED_Y_PIXELS())	:
					(y + cursor_height - MACH_MAX_DISPLAYED_Y_PIXELS());

			y -=  mach_cursor_state_p->visible_window_top_y;
			mach_cursor_state_p->current_cursor_status.y = y ;
			crtoffset_change_required = 1;
		}
		else
		{
			if (y < mach_cursor_state_p->visible_window_top_y &&
					mach_cursor_state_p->visible_window_top_y > 0)
			{

				if ( y >= 0)
				{
					mach_cursor_state_p->visible_window_top_y = y;
					y = 0;
					mach_cursor_state_p->current_cursor_status.y = y ;
				}
				else
				{
					y += mach_cursor_state_p->visible_window_top_y; 
					mach_cursor_state_p->current_cursor_status.y = y ;
					mach_cursor_state_p->visible_window_top_y =0;
				}
				crtoffset_change_required = 1;
			}
			else
			{
				y -= mach_cursor_state_p->visible_window_top_y; 
				mach_cursor_state_p->current_cursor_status.y = y ;
			}
		}
		if (crtoffset_change_required == 1)
		{
			int offset_hi;
			int offset_lo;

			ASSERT( (mach_cursor_state_p->visible_window_top_x >= 0) &&
				(mach_cursor_state_p->visible_window_top_y >= 0));

			MACH_CURSOR_CALCULATE_OFFSET_HI_LO(
				mach_cursor_state_p->visible_window_top_x,
				mach_cursor_state_p->visible_window_top_y,
				offset_hi,
				offset_lo);
#if defined(__DEBUG__)
			if (mach_cursor_debug)
			{
				(void)fprintf(debug_stream_p,
"(mach_cursor_hardare_move){\n"
"\twindow_top_x = %d\n"
"\twindow_top_y = %d\n"
"}",
				mach_cursor_state_p->visible_window_top_x,
				mach_cursor_state_p->visible_window_top_y);
			}
#endif
						
		 	if( (mach_register_state_p->crt_offset_hi !=
				 	(unsigned short)offset_hi) || 
	 		 	(mach_register_state_p->crt_offset_lo !=
					 	(unsigned short)offset_lo))
		 	{
				MACH_WAIT_FOR_VERTICAL_BLANK();
				MACH_CURSOR_SET_CRT_OFFSET_HI_LO(offset_hi,offset_lo);
			}
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
		MACH_CURSOR_SET_CURSOR_POSN(x,y);
		return SI_SUCCEED ;
	}

	
	/*
	 ** is the cursor at the left corner of the screen
	 ** or top of the screen
	 */
	if(x < 0)
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset = 
		    MACH_HARDWARE_CURSOR_MAX_WIDTH() - 
			active_cursor_p->cursor_width + (-x);
		x = 0;
	}
	else
	{
		active_cursor_p->hardware_cursor_p->horz_cursor_offset = 
		    MACH_HARDWARE_CURSOR_MAX_WIDTH() - 
			active_cursor_p->cursor_width;
	}

	if (y < 0)
	{
		int x_offset;
		/*
		 ** Change offset hi and offset lo to point to the
		 ** topmost line of the visible cursor
		 */

		x_offset  = ((-y) * (MACH_HARDWARE_CURSOR_MAX_WIDTH()) /
					 (MACH_SCREEN_DEPTH()/MACH_HARDWARE_CURSOR_DEPTH ));
		MACH_CURSOR_CALCULATE_OFFSET_HI_LO(x_offset,
		   active_cursor_p->hardware_cursor_p->memory_representation.y,
		   active_cursor_p-> hardware_cursor_p->cursor_offset_hi,
		   active_cursor_p->hardware_cursor_p->cursor_offset_lo);

		active_cursor_p->hardware_cursor_p->vert_cursor_offset =
		    MACH_HARDWARE_CURSOR_MAX_HEIGHT() -
		    active_cursor_p->cursor_height + (-y);
		y=0;
	}
	else
	{
		MACH_CURSOR_CALCULATE_OFFSET_HI_LO(active_cursor_p->\
			hardware_cursor_p->memory_representation.x,
			active_cursor_p->hardware_cursor_p->memory_representation.y,
			active_cursor_p->hardware_cursor_p->cursor_offset_hi,
			active_cursor_p->hardware_cursor_p->cursor_offset_lo);

		active_cursor_p->hardware_cursor_p->vert_cursor_offset =  
		    MACH_HARDWARE_CURSOR_MAX_HEIGHT() -
		    active_cursor_p->cursor_height;

	}

	if (mach_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{

		/*
		 * If the cursor goes off screen at the top.
		 */
		if (((mach_register_state_p->cursor_offset_hi) !=
			 (active_cursor_p->hardware_cursor_p->cursor_offset_hi|
			  MACH_CURSOR_OFFSET_HI_CURSOR_ENA)  )||
		    (mach_register_state_p->cursor_offset_lo !=
			 active_cursor_p->hardware_cursor_p->cursor_offset_lo))
		{
			int ret;
			/*
			 ** Logically modifying the offset_hi and offset_lo
			 ** registers should suffice, but modifying these two
			 ** registers resets other cursor related registers
			 ** also. So program all cursor related  registers
			 */
			ret = mach_cursor_draw_hardware_cursor(cursor_index);
			return ret;
		}
		else if ((active_cursor_p->hardware_cursor_p->horz_cursor_offset !=
				  mach_register_state_p->horz_cursor_offset) ||
				 (mach_register_state_p->vert_cursor_offset != 
				  active_cursor_p->hardware_cursor_p->vert_cursor_offset))
		{
			/*
			 * If the cursor moves within the screen boundaries.
			 */
			MACH_CURSOR_SET_OFFSET(active_cursor_p->
				hardware_cursor_p->horz_cursor_offset,
				active_cursor_p->hardware_cursor_p->vert_cursor_offset);
		}
		MACH_CURSOR_SET_CURSOR_POSN(x,y);
	}

	return SI_SUCCEED ;

}/*mach_cursor_hardware_move*/

/*
 * mach_cursor__vt_switch_out__
 * 
 * Called when the X server is about to lose control of its virtual
 * terminal.  The cursor module needs to turn off the hardware cursor
 * before the VT switch occurs, in order to remove it from interfering
 * with the display.
 * Note that we don't save the cursor area as the responsibilty of
 * saving offscreen memory rests with the vt switch code in the "mach"
 * module (mach.c).
 */
function void
mach_cursor__vt_switch_out__(void)
{

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	struct mach_register_state *mach_register_state_p =
		&(screen_state_p->register_state);
	MACH_CURSOR_CURSOR_STATE_DECLARE ();


#if defined(__DEBUG__)
	if(mach_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_cursor__vt_switch_out__){\n"
"\t# VT switch out\n"
"}\n");
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

	/*
	 * for now check to see if mach_cursor_state_p is not null
	 */
	if((mach_cursor_state_p == NULL) ||
	   (mach_cursor_state_p->cursor_type != MACH_CURSOR_HARDWARE))
	{
		return;
	}

	/*
	 * Turnoff the hardware-cursor
	 */
	MACH_CURSOR_SET_CURSOR_OFF();

}/*mach_cursor_vt_switch__out__*/

/*
 * mach_cursor__vt_switch_in__
 *
 * Called when the X server is going to switch into a virtual
 * terminal.  In the cursor module we need to re-enable the hardware
 * cursor if it was in use.  The contents of the offscreen location
 * are assumed to have been restored previous to this invocation.
 */

function void
mach_cursor__vt_switch_in__(void)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	struct mach_register_state *mach_register_state_p =
		&(screen_state_p->register_state);
	MACH_CURSOR_CURSOR_STATE_DECLARE ();

#if defined(__DEBUG__)
	if(mach_cursor_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_cursor_vt_switch__in__){\n"
"\t# VT switch in\n"
"}\n" );
	}
#endif

	if((mach_cursor_state_p == NULL) ||
	   (mach_cursor_state_p->cursor_type != MACH_CURSOR_HARDWARE))
	{
		return;
	}

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p));

	/*
	 * Turnon the hardware-cursor 
	 */
	if(mach_cursor_state_p->current_cursor_status.flag == CURSOR_ON)
	{
		MACH_CURSOR_SET_CURSOR_ON();
	}

}/*mach_cursor_vt_switch__in__*/


/*
**mach_cursor_hardware_download
*/
STATIC SIBool
mach_cursor_hardware_download(SIint32 cursor_index, SICursorP cursor_p)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	MACH_CURSOR_CURSOR_STATE_DECLARE ();

	struct mach_cursor *active_cursor_p ;

#if defined(__DEBUG__)
	if (mach_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_hardware_download){\n"
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
	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p));
	
	ASSERT(MACH_CURSOR_IS_VALID_INDEX(cursor_index));

	active_cursor_p = mach_cursor_state_p->cursor_list_pp[cursor_index] ;

	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR, active_cursor_p));


	/*
	 * If the cursor being downloaded is the current cursor, turn it
	 * off, reload the bits.
	 */
	if (mach_cursor_state_p->current_cursor_status.current_index ==
		cursor_index  && 
		mach_cursor_state_p->current_cursor_status.flag == CURSOR_ON )
	{
		/*
		 * Turn off existing cursor.
		 */
		(*active_cursor_p->turnoff_function_p) ( cursor_index );

		/*
		 * Create the new cursor.
		 */
		if (mach_cursor_new_cursor(cursor_index, cursor_p) == 0)
		{
			return SI_FAIL ;
		}
		
		active_cursor_p =
			mach_cursor_state_p->cursor_list_pp[cursor_index];

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
		return ((mach_cursor_new_cursor(cursor_index,cursor_p) == 1)
				? SI_SUCCEED : SI_FAIL);
	}
}/*mach_cursor_hardware_download*/      


/*
**mach_cursor_initialize_software_cursors
*/ 

STATIC void
mach_cursor_initialize_software_cursors(int no_of_cursors)
{

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	int i ;
	struct mach_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p)) ;

	mach_cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct mach_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		mach_cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct mach_cursor));
		STAMP_OBJECT(MACH_CURSOR, mach_cursor_state_p->cursor_list_pp[i]);

		cursor_p = mach_cursor_state_p->cursor_list_pp[i];
		cursor_p->turnoff_function_p = mach_cursor_software_turnoff;
		cursor_p->turnon_function_p = mach_cursor_software_turnon;
		cursor_p->move_function_p = mach_cursor_software_move;

		cursor_p->software_cursor_p= 
		 allocate_and_clear_memory(sizeof(struct software_cursor));

		STAMP_OBJECT(MACH_SOFTWARE_CURSOR, cursor_p->software_cursor_p);
		cursor_p->software_cursor_p->src_bitmap_p = NULL;
		cursor_p->software_cursor_p->invsrc_bitmap_p = NULL;
		cursor_p->software_cursor_p->save_area_p = NULL;

	}


}/*mach_cursor_initialize_software_cursors*/

/*
** mach_cursor_initialize_hardware_cursors
*/
STATIC int
mach_cursor_initialize_hardware_cursors(int no_of_cursors)
{

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURSOR_CURSOR_STATE_DECLARE();
	int i ;
	int total_pixels ;
	int	full_lines ;
	int last_line ;
	struct omm_allocation *omm_block_p;
	struct mach_cursor *cursor_p ;


	ASSERT(IS_OBJECT_STAMPED(MACH_CURSOR_STATE, mach_cursor_state_p)) ;
	
	omm_block_p = omm_named_allocate(MACH_CURSOR_NAMED_ALLOCATION_ID);
	if (omm_block_p  == NULL)
	{
		return 0;
	}

	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, omm_block_p));
	
	mach_cursor_state_p->omm_block_p = omm_block_p;

#if defined ( __DEBUG__ )
	if (mach_cursor_debug)
	{
		(void)fprintf ( debug_stream_p,
"(mach_cursor_initialize_hardware_cursors)"
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
		((MACH_HARDWARE_CURSOR_MAX_WIDTH() * MACH_HARDWARE_CURSOR_DEPTH) /
		 MACH_SCREEN_DEPTH()) * MACH_HARDWARE_CURSOR_MAX_HEIGHT();

	full_lines = total_pixels / MACH_MAX_PHYSICAL_X_PIXELS ();

	last_line = total_pixels % MACH_MAX_PHYSICAL_X_PIXELS ();

	if (last_line > 0)
	{
		++full_lines ;
	}

	mach_cursor_state_p->cursor_list_pp =
	    allocate_and_clear_memory(sizeof(struct mach_cursor* ) * 
								  no_of_cursors); 

	for ( i=0; i < no_of_cursors; ++i )
	{

		mach_cursor_state_p->cursor_list_pp[i] =
		    allocate_and_clear_memory(sizeof(struct mach_cursor));
		STAMP_OBJECT(MACH_CURSOR, mach_cursor_state_p->cursor_list_pp[i]);

		cursor_p = mach_cursor_state_p->cursor_list_pp[i];

		cursor_p->turnoff_function_p = mach_cursor_hardware_turnoff;

		cursor_p->turnon_function_p = mach_cursor_hardware_turnon;

		cursor_p->move_function_p = mach_cursor_hardware_move;

		cursor_p->hardware_cursor_p= 
		 allocate_and_clear_memory(sizeof(struct mach_cursor_hardware_cursor));

		STAMP_OBJECT(MACH_HARDWARE_CURSOR, cursor_p->hardware_cursor_p);

		cursor_p->hardware_cursor_p->memory_representation.x = 
			omm_block_p->x ;

		cursor_p->hardware_cursor_p->memory_representation.y = 
			omm_block_p->y+ (full_lines*i);

#if defined ( __DEBUG__ )
		if ( mach_cursor_debug )
		{
			(void) fprintf(debug_stream_p,
"(mach_cursor_initialize_hardware_cursors)"
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

}/*mach_cursor_initialize_hardware_cursors*/

/*
** mach_cursor_initialize__
*/
function void
mach_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct mach_options_structure * options_p)
{
	struct mach_cursor_state  *mach_cursor_state_p;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	MACH_CURRENT_SCREEN_STATE_DECLARE();


#if defined(__DEBUG__)
	if ( mach_cursor_debug )
	{
		(void) fprintf(debug_stream_p,
"(mach__cursor__initialize__){\n"
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
						MACH_MAX_DISPLAYED_X_PIXELS(),
						MACH_MAX_DISPLAYED_Y_PIXELS(),
						MACH_MAX_VIRTUAL_X_PIXELS(),
						MACH_MAX_VIRTUAL_Y_PIXELS(),
						MACH_MAX_PHYSICAL_X_PIXELS(),
						MACH_MAX_PHYSICAL_Y_PIXELS());
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
	mach_cursor_state_p =
	    allocate_and_clear_memory(sizeof(struct mach_cursor_state));

	STAMP_OBJECT(MACH_CURSOR_STATE, mach_cursor_state_p);

	flags_p->SIcurscnt = 
	    (options_p->number_of_downloadable_cursors) ?
	    options_p->number_of_downloadable_cursors :
	    DEFAULT_MACH_NUMBER_OF_DOWNLOADABLE_CURSORS;

	/*
	 * Get the cursor size that one is supposed to support.
	 */
	if (options_p->cursor_max_size == NULL)
	{
		mach_cursor_state_p->hardware_cursor_max_width =
			DEFAULT_MACH_DOWNLOADABLE_CURSOR_WIDTH;

		mach_cursor_state_p->hardware_cursor_max_height =
			DEFAULT_MACH_DOWNLOADABLE_CURSOR_HEIGHT;

#if defined (__DEBUG__)
			if (mach_cursor_debug)
			{
				(void)fprintf ( debug_stream_p,
"(mach_cursor__initialize__){\n"
"\tUsing default cursor dimensions\n"
"\n}\n"
					);
			}
#endif
	}
	else
	{
		if((sscanf(options_p->cursor_max_size,"%ix%i",
					&mach_cursor_state_p->hardware_cursor_max_width, 
					&mach_cursor_state_p->hardware_cursor_max_height) != 2) ||
		   (mach_cursor_state_p->hardware_cursor_max_width <= 0) ||
		   (mach_cursor_state_p->hardware_cursor_max_height <= 0))
		{
#if defined (__DEBUG__)
			if ( mach_cursor_debug )
			{
				(void)fprintf ( debug_stream_p,
"(mach_cursor__initialize__){\n"
"\tUsing default cursor dimensions\n"
"\n}\n"
					);
			}
#endif
			(void) fprintf(stderr, MACH_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
						   options_p->cursor_max_size);

			mach_cursor_state_p->hardware_cursor_max_width =
				DEFAULT_MACH_DOWNLOADABLE_CURSOR_WIDTH;

			mach_cursor_state_p->hardware_cursor_max_height =
				DEFAULT_MACH_DOWNLOADABLE_CURSOR_HEIGHT;
		}
#if defined (__DEBUG__)
		else
		{
			if ( mach_cursor_debug )
			{
				(void) fprintf(debug_stream_p,
"(mach_cursor__initialize__){\n"
"\tMax cursor width  = %d"
"\tMax cursor height = %d"
"\n}\n",
							   mach_cursor_state_p->hardware_cursor_max_width,
							   mach_cursor_state_p->hardware_cursor_max_height
							   );
			}
		}
#endif
	}

	/*
	 * Set SI's idea of the best cursor width.
	 */
	
	flags_p->SIcurswidth =
		mach_cursor_state_p->hardware_cursor_max_width;
	flags_p->SIcursheight =
		mach_cursor_state_p->hardware_cursor_max_height;

	/*
	 * The cursor mask is valid for software cursors only.
	 */
	flags_p->SIcursmask = DEFAULT_MACH_DOWNLOADABLE_CURSOR_MASK;

	/*
	 * Default handlers.
	 */
	functions_p->si_hcurs_download = 
		(SIBool (*)(SIint32, SICursorP)) mach_cursor_no_operation;
	functions_p->si_hcurs_turnon = 
		(SIBool (*)(SIint32)) mach_cursor_no_operation;
	functions_p->si_hcurs_turnoff = 
		(SIBool (*)(SIint32)) mach_cursor_no_operation;
	functions_p->si_hcurs_move = 
		(SIBool (*)(SIint32, SIint32, SIint32)) mach_cursor_no_operation;

	if (screen_state_p->chipset_kind == MACH_CHIPSET_ATI_38800) 
	{
		/*
		 * If the user has explicitly asked for a hardware cursor,
		 * warn him/her, but continue.  This will result in  no
		 * cursor being visible (ie: all calls will return SI_SUCCEED)
		 */
		if (options_p->cursor_type ==
		    MACH_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR )
		{
			(void) fprintf(stderr,
						   MACH_NO_HARDWARE_CURSOR_FOR_MACH8_MESSAGE);
			flags_p->SIcursortype = CURSOR_TRUEHDWR;
			return;
		}
		else /* Software cursor or auto-configure */
		{
			flags_p->SIcursortype = CURSOR_FAKEHDWR;
		}
	}
	else
	{
		/* 
		 * The MACH32 and above support
		 * hardware cursors. 
		 */
		if (options_p->cursor_type ==
			MACH_OPTIONS_CURSOR_TYPE_SOFTWARE_CURSOR)
		{
			flags_p->SIcursortype = CURSOR_FAKEHDWR;
		}
		else	/*auto-configure or hardware cursor requested by the  user*/
		{
			flags_p->SIcursortype = CURSOR_TRUEHDWR;
			/*
			 * Hardware cursor does not work for 16bpp mode. use software
			 * cursor till such time this is fixed.
			 */
			if (screen_state_p->generic_state.screen_depth == 16)
			{
				flags_p->SIcursortype = CURSOR_FAKEHDWR;
			}
		}
	}
	
	if(flags_p->SIcursortype == CURSOR_TRUEHDWR)
	{
#if defined(__DEBUG__)
		if( mach_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
"(mach_cursor__initialize__){\n"
"\t#Switching to hardware cursor\n"
"}\n"
							);
		}
#endif

		/*
		 * Set up SI's parameters.
		 */

		flags_p->SIcursortype = CURSOR_TRUEHDWR;
		mach_cursor_state_p->cursor_type = MACH_CURSOR_HARDWARE;

		/*
		 * Cursor manipulation functions.
		 */

		functions_p->si_hcurs_download = mach_cursor_hardware_download;
		functions_p->si_hcurs_turnon = mach_cursor_hardware_turnon;
		functions_p->si_hcurs_turnoff = mach_cursor_hardware_turnoff;
		functions_p->si_hcurs_move = mach_cursor_hardware_move;

		mach_cursor_state_p->number_of_cursors = flags_p->SIcurscnt;

		mach_cursor_state_p->current_cursor_status.x = -1;
		mach_cursor_state_p->current_cursor_status.y = -1;
		mach_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
		mach_cursor_state_p->current_cursor_status.current_index = -1;

		/*
		 * Used for pixel panning
		 */
		mach_cursor_state_p->visible_window_top_x = 0;
		mach_cursor_state_p->visible_window_top_y = 0;

		/*
		 * Do we need to post process the cursor bits?
		 */
		if (options_p->cursor_byte_swap ==
			MACH_OPTIONS_CURSOR_BYTE_SWAP_ENABLED)
		{
			mach_cursor_state_p->swap_function_p = mach_cursor_swap_bytes;
		}
		else if (options_p->cursor_byte_swap ==
				 MACH_OPTIONS_CURSOR_BYTE_SWAP_DISABLED)
		{
			mach_cursor_state_p->swap_function_p = NULL;
		}
		else					/* auto-configure */
		{
			if (screen_state_p->generic_state.screen_depth == 4)
			{
				mach_cursor_state_p->swap_function_p =
					mach_cursor_swap_bytes;
			}
			else				/* 8 bit and above */
			{
				mach_cursor_state_p->swap_function_p = NULL;
			}
		}
		
		/*
		 * Assign the cursor state.
		 */
		(screen_state_p)->cursor_state_p 
			= mach_cursor_state_p ;

		/*
		 * Allocate off-screen memory etc.
		 */

		if (mach_cursor_initialize_hardware_cursors(flags_p->SIcurscnt) == 0)
		{
			/*
			 * Allocation failed : Undo all work done 
			 */

#if (defined(__DEBUG__))
			if (mach_cursor_debug)
			{
				(void) fprintf(debug_stream_p,
"(mach_cursor__initialize__)\n"
"{\n"
"\t# Initialization of hardware cursors failed.\n"
"}\n");
			
			}
#endif

			flags_p->SIcursortype = CURSOR_FAKEHDWR;
			functions_p->si_hcurs_download = 
				(SIBool (*)(SIint32, SICursorP)) mach_cursor_no_operation;
			functions_p->si_hcurs_turnon = 
				(SIBool (*)(SIint32)) mach_cursor_no_operation;
			functions_p->si_hcurs_turnoff = 
				(SIBool (*)(SIint32)) mach_cursor_no_operation;
			functions_p->si_hcurs_move = 
				(SIBool (*)(SIint32, SIint32, SIint32))
					 mach_cursor_no_operation;

			return ;
		}
	}
	else /*setup for software cursor*/
	{
#if defined(__DEBUG__)
		if( mach_cursor_debug)
		{
			(void)fprintf(debug_stream_p,
"(mach_cursor__initialize__){\n"
"\t#Switching to software cursor\n"
"}\n"
							);
		}
#endif
		mach_cursor_state_p->cursor_type = MACH_CURSOR_SOFTWARE;

		/*
		 * For testing software cursor on R5
		 */
		/*flags_p->SIcursortype = CURSOR_TRUEHDWR;*/

		flags_p->SIcursortype = CURSOR_FAKEHDWR;

		/*
		 * Cursor manipulation functions.
		 */
		functions_p->si_hcurs_download = mach_cursor_software_download;
		functions_p->si_hcurs_turnon = mach_cursor_software_turnon;
		functions_p->si_hcurs_turnoff = mach_cursor_software_turnoff;
		functions_p->si_hcurs_move = mach_cursor_software_move;

		mach_cursor_state_p->number_of_cursors = flags_p->SIcurscnt;

		mach_cursor_state_p->current_cursor_status.x = -1;
		mach_cursor_state_p->current_cursor_status.y = -1;
		mach_cursor_state_p->current_cursor_status.flag = CURSOR_OFF;
		mach_cursor_state_p->current_cursor_status.current_index = -1;
		/*
		 * Used for pixel panning
		 */
		mach_cursor_state_p->visible_window_top_x = 0;
		mach_cursor_state_p->visible_window_top_y = 0;
		/*
		 * Assign the cursor state.
		 */
		(screen_state_p)->cursor_state_p 
			= mach_cursor_state_p ;
		mach_cursor_initialize_software_cursors(flags_p->SIcurscnt); 
	}
		
}/*mach_cursor__initialize__*/

/*
**mach_cursor_make_named_allocation_string 
*/

function char *
mach_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct mach_options_structure * options_p)
{

	int total_pixels ;
	int lines ;
	int last_line ;
	char *string_p ;
	int hardware_cursor_max_width;
	int hardware_cursor_max_height;
	int number_of_cursors ;
	int start_y ;
	char tmp_buf[MACH_CURSOR_NAMED_ALLOCATION_STRING_MAXLEN+1] ;


	/*
	 * No need for named allocation for cursor if hardware cursor is
	 * not requested or the chip is MACH8
	 */
	if ((options_p->cursor_type ==
		 MACH_OPTIONS_CURSOR_TYPE_SOFTWARE_CURSOR) ||
		(MACH_CHIPSET_KIND() == MACH_CHIPSET_ATI_38800))
	{
#if defined (__DEBUG__)
		if ( mach_cursor_debug )
		{
			(void)fprintf(debug_stream_p,
"(mach_cursor_make_named_allocation_string){\n" 
"\tReturning null\n"
"}\n");
		}
#endif
		return NULL;
	}

	if(options_p->cursor_max_size == NULL)
	{
		hardware_cursor_max_width =
		    DEFAULT_MACH_DOWNLOADABLE_CURSOR_WIDTH;

		hardware_cursor_max_height =
		    DEFAULT_MACH_DOWNLOADABLE_CURSOR_HEIGHT;
	}
	else if ((sscanf(options_p->cursor_max_size,"%ix%i",
					 &hardware_cursor_max_width, 
					 &hardware_cursor_max_height) != 2) ||
			 (hardware_cursor_max_width <= 0) ||
			 (hardware_cursor_max_height <= 0))
	{
		(void) fprintf(stderr, MACH_CANNOT_PARSE_CURSOR_SIZE_MESSAGE,
					   options_p->cursor_max_size);
		
		hardware_cursor_max_width =
		    DEFAULT_MACH_DOWNLOADABLE_CURSOR_WIDTH;

		hardware_cursor_max_height =
		    DEFAULT_MACH_DOWNLOADABLE_CURSOR_HEIGHT;
	}


	number_of_cursors = 
	    (options_p->number_of_downloadable_cursors >  0) ?
			options_p->number_of_downloadable_cursors :
			DEFAULT_MACH_NUMBER_OF_DOWNLOADABLE_CURSORS;

	total_pixels = 
		(hardware_cursor_max_width*2/(MACH_SCREEN_DEPTH()))* 
	    hardware_cursor_max_height;

	lines = total_pixels / MACH_MAX_PHYSICAL_X_PIXELS () ;

	last_line = total_pixels % MACH_MAX_PHYSICAL_X_PIXELS () ;


	if ( last_line > 0 )
	{
		++lines ;
	}

	/*
	 * The offscreen mem for the cursor will be allocated
	 * at the beginning of the offscreen area
	 */

	start_y = MACH_MAX_VIRTUAL_Y_PIXELS() + 1;

	(void)sprintf(tmp_buf, MACH_CURSOR_NAMED_ALLOCATION_STRING_TEMPLATE,
			MACH_MAX_PHYSICAL_X_PIXELS(),
			number_of_cursors * lines,
			MACH_SCREEN_DEPTH(), 0, start_y,0);

	string_p = allocate_memory(strlen(tmp_buf) + 1);

	(void) strcpy (string_p, tmp_buf);

#if defined(__DEBUG__)
	if (mach_cursor_debug)
	{
		(void)fprintf(debug_stream_p,
"(mach_cursor_make_named_allocation_string){\n"
"\topt_string=%s\n"
"}\n",
					 string_p);
	}
#endif

	return string_p;

}/*mach_cursor_make_named_allocation_string*/
