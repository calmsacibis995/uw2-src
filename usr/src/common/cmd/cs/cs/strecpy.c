/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/strecpy.c	1.1.1.5"
#ident	"$Header: $"

#include	"uucp.h"

/*
	strecpy(output, input, except)
	strccpy copys the input string to the output string expanding
	any non-graphic character with the C escape sequence.
	Esacpe sequences produced are those defined in "The C Programming
	Language" pages 180-181.
	Characters in the except string will not be expanded.
*/

GLOBAL char *
strecpy( pout, pin, except )
register char	*pout;
register char	*pin;
char	*except;
{
	register unsigned	c;
	register char		*output;

	output = pout;
	while( (c = (unsigned char)*pin++) != 0 ) {
		if( !iswprint( (unsigned char)c )  &&  ( !except  ||  !strchr( except, c ) ) ) {
			*pout++ = '\\';
			switch( c ) {
			case '\n':
				*pout++ = 'n';
				continue;
			case '\t':
				*pout++ = 't';
				continue;
			case '\b':
				*pout++ = 'b';
				continue;
			case '\r':
				*pout++ = 'r';
				continue;
			case '\f':
				*pout++ = 'f';
				continue;
			case '\v':
				*pout++ = 'v';
				continue;
			case '\\':
				continue;
			default:
				sprintf( pout, "%.3o", c );
				pout += 3;
				continue;
			}
		}
		if( c == '\\'  &&  ( !except  ||  !strchr( except, c ) ) )
			*pout++ = '\\';
		*pout++ = (char) c;
	}
	*pout = '\0';
	return  output;
}
