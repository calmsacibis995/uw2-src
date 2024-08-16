/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_stpl.h	1.2"

#if (! defined(__P9K_STPL_INCLUDED__))

#define __P9K_STPL_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"
#include <sidep.h>
#include "p9k_opt.h"

/***
 ***	Constants.
 ***/

#define P9000_STIPPLE_STATE_STAMP							\
(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + 		\
 ('0' << 4) + ('_' << 5) + ('S' << 6) + ('T' << 7) + 		\
 ('I' << 8) + ('P' << 9) + ('P' << 10) + ('L' << 11) + 		\
 ('E' << 12) + ('_' << 13) + ('S' << 14) + ('T' << 15) + 	\
 ('A' << 16) + ('M' << 17) + ('P' << 18))

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 * A stipple state.
 */

struct p9000_stipple_state
{
	SIbitmapP si_stipple_p;

	/*
	 * flag to indicate if the stipple is `small'
	 */
	
	boolean is_small_stipple;
	

	/*
	 * Downloaded stipple width and height.
	 */

	int downloaded_stipple_width;
	int downloaded_stipple_height;

	/*
	 * Reduced stipple bits 
	 */
	
	unsigned long reduced_si_stipple_bits[P9000_DEFAULT_SMALL_STIPPLE_HEIGHT*
		(((P9000_DEFAULT_SMALL_STIPPLE_WIDTH +
		 DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) &
		 ~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK)>>5)];

	/*JK: better to add an #error directive to check for the same
	  thing in defaults.h */

	/*
	 * Expanded stipple bits for small stipples.
	 */
	
	unsigned long
		small_stipple_bits[P9000_DEFAULT_PATTERN_REGISTER_COUNT];
	
	
	/*
	 * The draw function for this stipple.
	 */

	SIBool (*stipple_function_p)
		(struct p9000_stipple_state *stipple_state_p,
			int count, SIRectOutlineP rect_p);
	
	
#if (defined(__DEBUG__))
	int stamp;
#endif

};

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern enum debug_level p9000_stipple_debug ;
#endif

/*
 *	Current module state.
 */

extern void
p9000_stipple_small_stipple_helper(int count, SIRectOutlineP rect_p)
;

extern void
p9000_stipple_download_stipple(
	struct p9000_screen_state *screen_state_p,
	struct p9000_stipple_state *stipple_state_p,
	SIbitmapP si_bitmap_p)
;

extern void
p9000_stipple__gs_change__(void)
;

extern void
p9000_stipple__initialize__(
	SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
;


#endif
