/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_cursor.h	1.2"
#if (! defined(__S364_CURSOR_INCLUDED__))

#define __S364_CURSOR_INCLUDED__



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
extern boolean s364_cursor_debug ;
#endif

extern void
s364_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
;

extern void
s364_cursor__vt_switch_in__(void)
;

extern char *
s364_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
;


#endif
