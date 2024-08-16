/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:oh/detab.c	1.5.3.3"

#include <stdio.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "typetab.h"
#include "detabdefs.h"

/* the Object Detection Function Table for this FMLI session */
struct odft_entry Detab[MAXODFT];
