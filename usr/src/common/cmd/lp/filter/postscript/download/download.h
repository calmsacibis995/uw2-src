/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:filter/postscript/download/download.h	1.2.5.4"
#ident	"$Header: $"
/*
 *
 * The font data for a printer is saved in an array of the following type.
 *
 */

typedef struct map {

	char	*font;			/* a request for this PostScript font */
	char	*file;			/* means copy this unix file */
	int	downloaded;		/* TRUE after *file is downloaded */

} Map;

Map	*allocate();

#define DOT	'.'
#define PFB_EXT	".PFB"
#define PFB2PFA	"/usr/X/bin/pfb2pfa"
