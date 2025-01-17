/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/my_fgets.c	1.1.1.2"
#ident  "$Header: my_fgets.c 1.1 91/07/03 $"

/*
		my_fgets(buffer, size, stream)

	Differs from fgets(3S) un that it honors the size.
*/

#include	"MS-DOS.h"

#include	<stdio.h>

char	*
my_fgets(buffer, size, stream)
char	*buffer;
int	size;
FILE	*stream;
{
	int	i;
	int	rc;

	/*
		If size == 0 return NULL
	*/
	if (size == 0)
		return(NULL);

	/*
		Loop on getc(3S).
	*/
	for (i = 0; (rc = getc(stream)) != EOF; ) {
		/*
			We read a byte from the stream

			If it is a newline, break.
		*/
		if (rc == '\n')
			break;

		/*
			The character read is not a newline.
			If we have read less than size - 1 bytes,
			store this one in the buffer.
		*/
		if (--size > 1)
			*(buffer + i++) = rc;
	}

	*(buffer + i++) = '\n';
	*(buffer + i) = '\0';

	return(buffer);
}
