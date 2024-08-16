/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_cursor.h	1.1"

#if (! defined(__M_CURSOR_INCLUDED__))

#define __M_CURSOR_INCLUDED__



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
extern boolean mach_cursor_debug;
#endif

/*
*	Current module state.
*/

extern void
mach_cursor__vt_switch_out__(void)
;

extern void
mach_cursor__vt_switch_in__(void)
;

extern void
mach_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct mach_options_structure * options_p)
;

extern char *
mach_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct mach_options_structure * options_p)
;


#endif
