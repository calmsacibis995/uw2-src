/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/hp2648/linemod.c	1.2"
#ident	"$Header: $"

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */


#include "hp2648.h"

linemod( line )
char	*line;
{
	putchar('Z'); 
	handshake();
	putchar(ESC); 
	putchar(GRAPHIC);
	putchar(MODE);
	if ( *(line) == 's' ) {
		if ( *(++line) == 'o' ) {
			/*
			 * solid mode 1
			 */
			putchar( '1' );
			putchar( 'b' );
			goto done;
		}
		else if ( *(line) == 'h' ) {
			/*
			 * shortdashed mode 4
			 */
			putchar( '6' );
			putchar( 'b' );
			goto done;
		}
	}
	else if ( *(line) == 'd' ) {
		if ( *(++line) == 'o' && *(++line) == 't' ) {
			if ( *(++line) == 't' ) {
				/*
				 * dotted mode 2
				 */
				putchar( '7' );
				putchar( 'b' );
				goto done;
			}
			else if ( *(line) == 'd' ) {
				/*
				 * dotdashed mode 3
				 */
				putchar( '8' );
				putchar( 'b' );
				goto done;
			}
		}
	}
	else if ( *(line) == 'l' ) {
		/*
		 * longdashed mode 5
		 */
		putchar( '5' );
		putchar( 'b' );
		goto done;
	}
	putchar( '1' );				/* default to solid */
	putchar( 'b' );				/* default to solid */
done:
	putchar( 'Z' );
	handshake();
	putchar(ESC); 
	putchar(GRAPHIC);
	putchar(PLOT);
	putchar(BINARY);
	buffcount = 4;
	return;
}
