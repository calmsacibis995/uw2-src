/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "sh_config.h"
#include "edit.h"

#ident	"@(#)debugger:libedit/common/defs.c	1.1"

#ifdef MULTIBYTE
#include "national.h"
/*
 * These are default values.  They can be changed with CSWIDTH
 */

char int_charsize[] =
{
	1, CCS1_IN_SIZE, CCS2_IN_SIZE, CCS3_IN_SIZE,	/* input sizes */
	1, CCS1_OUT_SIZE, CCS2_OUT_SIZE, CCS3_OUT_SIZE	/* output widths */
};
#else
char int_charsize[] =
{
	1, 0, 0, 0,	/* input sizes */
	1, 0, 0, 0	/* output widths */
};
#endif /* MULTIBYTE */
