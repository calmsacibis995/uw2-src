/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_cursor.h	1.3"

#if (! defined(__M64_CURSOR_INCLUDED__))

#define __M64_CURSOR_INCLUDED__



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
extern boolean m64_cursor_debug ;
#endif


extern void
m64_cursor__vt_switch_out__(void)
;

extern void
m64_cursor__vt_switch_in__(void)
;

extern char *
m64_cursor_make_named_allocation_string (SIScreenRec *si_screen_p,
						  struct m64_options_structure * options_p)
;

extern void
m64_cursor__initialize__(SIScreenRec *si_screen_p,
						  struct m64_options_structure * options_p)
;


#endif
