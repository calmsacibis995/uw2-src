#ident	"@(#)xpr:devices/terminfo/models.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "xpr_term.h"

/*
 * Most models can't do better than "enlarge()" in scaling
 * vertically.
 */
struct model		models[] = {
	{ init_image_to_cells, image_to_cells, 1, 0 },
	{ init_image_to_bits,  image_to_bits,  0, 0 },
	{ init_image_to_bits,  image_to_bits,  0, 0 },
};

int			nmodels	 = 3;
