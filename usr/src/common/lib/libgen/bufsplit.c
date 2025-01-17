/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:bufsplit.c	1.3"
/*
	split buffer into fields delimited by tabs and newlines.
	Fill pointer array with pointers to fields.
	Return the number of fields assigned to the array[].
	The remainder of the array elements point to the end of the buffer.
  Note:
	The delimiters are changed to null-bytes in the buffer and array of
	pointers is only valid while the buffer is intact.
*/

#ifdef __STDC__
	#pragma weak bufsplit = _bufsplit
#endif
#include "synonyms.h"
#include <sys/types.h>
static char	*bsplitchar = "\t\n";	/* characters that separate fields */

size_t
bufsplit( buf, dim, array )
register char	*buf;		/* input buffer */
size_t	dim;		/* dimension of the array */
char	*array[];
{
	extern	char	*strrchr();
	extern	char	*strpbrk();
	register unsigned numsplit;
	register int	i;

	if( !buf )
		return 0;
	if ( !dim ^ !array)
		return 0;
	if( buf  &&  !dim  &&  !array ) {
		bsplitchar = buf;
		return 1;
	}
	numsplit = 0;
	while ( numsplit < dim ) {
		array[numsplit] = buf;
		numsplit++;
		buf = strpbrk(buf, bsplitchar);
		if (buf)
			*(buf++) = '\0';
		else
			break;
		if (*buf == '\0') {
			break;
		}
	}
	buf = strrchr( array[numsplit-1], '\0' );
	for (i=numsplit; i < dim; i++)
		array[i] = buf;
	return numsplit;
}
