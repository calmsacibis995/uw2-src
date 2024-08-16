/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)stand:i386at/standalone/boot/at386/gets.c	1.6"
#ident	"$Header: $"

#include <sys/types.h>

/*
 * bgets():	Sort of like gets(), but not quite.
 *		Read from the console (using getchar) into string str,
 *		until a carriage return or until n-1 characters are read.
 *		Null terminate the string, and return.
 *		This all is made complicated by the fact that we must
 *		do our own echoing during input.
 *		N.B.: Returns the *number of characters in str*.
 */

int
bgets( str, n )
char	*str;
int	n;
{
	int 	c;
	int	t;
	char	*p;

	p = str;
	c = 0;

	while ( (t = bgetchar()) != '\r' ) {
		bputchar(t);
		if ( t == '\b' ) {
			if ( c ) {
				bprintf(" \b");
				c--; p--;
			} else
				bputchar(' ');
			continue;
		}
		if (c < n - 1) {
			*p++ = t; 
			c++;
		}
	}
	*p = '\0';
	bputchar('\r');
	bputchar('\n');

	return(c);
}


/*
 * bfgets():	Sort of like fgets(), but not quite.
 *		Read data from the open inode starting at offset, 
 *		stopping when a	newline is encounted, or n-1 characters 
 *		have been read, or EOF is reached. The string is then
 *		null terminated. 
 *		N.B.: Returns the *number of characters in str*.
 */
 
int
bfgets( str, n, offset)
char	*str;
int	n;
int	offset;
{
	unsigned long	count, i;
	unsigned long	stat;

	BL_file_lseek(offset,&stat);

	BL_file_read( str, 0, n, &count, &stat );

	if ( count == 0 )
		return(0);

	for ( i = 0; i < count; i++ ) {
		if ( str[i] == '\n' )
			break;
	}
	str[i] = '\0';

	return(i);
}
