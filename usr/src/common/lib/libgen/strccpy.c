/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:strccpy.c	1.5"

#ifdef __STDC__
	#pragma weak strccpy = _strccpy
	#pragma weak strcadd = _strcadd
#endif
#include "synonyms.h"

/*
	strccpy(output, input)
	strccpy copys the input string to the output string compressing
	any C-like escape sequences to the real character.
	Esacpe sequences recognized are those defined in "The C Programming
	Language" pages 180-181.
	strccpy returns the output argument.

	strcadd(output, input)
	Identical to strccpy() except returns address of null-byte at end
	of output.  Useful for concatenating strings.
*/

char *strcadd();

char *
strccpy(pout, pin)
char *pout;
const char *pin;
{
	(void)strcadd( pout, pin );
	return  (pout);
}


char *
strcadd(pout, pin)
register char *pout;
register const char *pin;
{
	register char c;
	int count;
	int wd;

	while (c = *pin++) {
		if (c == '\\')
			switch (c = *pin++) {
			case 'n':
				*pout++ = '\n';
				continue;
			case 't':
				*pout++ = '\t';
				continue;
			case 'b':
				*pout++ = '\b';
				continue;
			case 'r':
				*pout++ = '\r';
				continue;
			case 'f':
				*pout++ = '\f';
				continue;
			case 'v':
				*pout++ = '\v';
				continue;
			case 'a':
				*pout++ = '\007';
				continue;
			case '\\':
				*pout++ = '\\';
				continue;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				wd = c - '0';
				count = 0;
				while ((c = *pin++) >= '0' && c <= '7') {
					wd <<= 3;
					wd |= (c - '0');
					if (++count > 1) {   /* 3 digits max */
						pin++;
						break;
					}
				}
				*pout++ = wd;
				--pin;
				continue;
			case 'x':
				wd = 0;
				count = 0;
				while ((c = *pin++) && count++ < 2)
				{
					int bias;
					switch( c)
					{
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
						    bias = 10 - 'a'; break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
						    bias = 10 - 'A'; break;
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
						    bias = -'0'; 
						    break;
						default:
						    bias = 0; 
						    break;
					}
					if (bias)
					{
						wd = (wd << 4) + (bias + c);
					}
					else
					{
						break; /* exit while loop */
					}
				}
				*pout++ = wd;
				--pin;
				continue;
			default:
				*pout++ = c;
				continue;
		}
		*pout++ = c;
	}
	*pout = '\0';
	return (pout);
}
