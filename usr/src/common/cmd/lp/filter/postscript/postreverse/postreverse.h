/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:filter/postscript/postreverse/postreverse.h	1.1.5.3"
#ident	"$Header: $"

/*
 *
 * Definitions used by the page reversal utility program.
 *
 * An array of type Pages is used to keep track of the starting and ending byte
 * offsets for the pages we've been asked to print.
 *
 */

typedef struct {

	long	start;			/* page starts at this byte offset */
	long	stop;			/* and ends here */
	int	empty;			/* dummy page if TRUE */

} Pages;

/*
 *
 * Some of the non-integer functions in postreverse.c.
 *
 */

char	*copystdin();

