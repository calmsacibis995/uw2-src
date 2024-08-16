/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_cursor.h	1.4"

#if (! defined(__S3_CURSOR_INCLUDED__))

#define __S3_CURSOR_INCLUDED__



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
extern boolean s3_cursor_debug;
#endif

extern void
s3_cursor__vt_switch_out__(void)
;

extern void
s3_cursor__vt_switch_in__(void)
;

extern void
s3_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct s3_options_structure * options_p)
;

extern char *
s3_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct s3_options_structure * options_p)
;


#endif
