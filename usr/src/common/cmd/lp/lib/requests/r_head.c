/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/requests/r_head.c	1.5.5.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "sys/types.h"

#include "lp.h"
#include "requests.h"

struct {
	char			*v;
	short			len;
}			reqheadings[RQ_MAX] = {

#define	ENTRY(X)	X, sizeof(X)-1

	ENTRY("C "),	/* RQ_COPIES */
	ENTRY("D "),	/* RQ_DEST */
	ENTRY("F "),	/* RQ_FILE */
	ENTRY("f "),	/* RQ_FORM */
	ENTRY("H "),	/* RQ_HANDL */
	ENTRY("N "),	/* RQ_NOTIFY */
	ENTRY("O "),	/* RQ_OPTS */
	ENTRY("P "),	/* RQ_PRIOR */
	ENTRY("p "),	/* RQ_PGES */
	ENTRY("S "),	/* RQ_CHARS */
	ENTRY("T "),	/* RQ_TITLE */
	ENTRY("Y "),	/* RQ_MODES */
	ENTRY("t "),	/* RQ_TYPE */
	ENTRY("U "),	/* RQ_USER */
	ENTRY("r "),	/* RQ_RAW */
	ENTRY("a "),	/* RQ_FAST */
	ENTRY("s "),	/* RQ_STAT */
/*	ENTRY("x "),	/* reserved (slow filter) */
/*	ENTRY("y "),	/* reserved (fast filter) */
/*	ENTRY("z "),	/* reserved (printer name) */

#undef	ENTRY

};
