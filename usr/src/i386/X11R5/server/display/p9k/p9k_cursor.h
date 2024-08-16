/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_cursor.h	1.2"

#if (! defined(__P9K_CURSOR_INCLUDED__))

#define __P9K_CURSOR_INCLUDED__



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
extern enum debug_level p9000_cursor_debug;
#endif

extern void
p9000_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct p9000_options_structure * options_p)
;

extern void
p9000_cursor__vt_switch_in__(void)
;


#endif
