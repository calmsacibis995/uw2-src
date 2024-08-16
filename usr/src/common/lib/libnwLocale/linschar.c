/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:linschar.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/linschar.c,v 1.1 1994/09/26 17:20:46 rebekah Exp $"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

/* This function inserts a character at a given position in a string */
/* The size of the character inserted is returned */

#if (defined WIN32 || defined N_PLAT_UNIX)
#define _fmemmove memmove
#endif

#define	NWL_EXCLUDE_FILE 1
#define	NWL_EXCLUDE_TIME 1

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

int N_API NWLInsertChar(
	char N_FAR * src,	/* pointer to where char is to be inserted */
	char N_FAR *insertableChar) /* pointer to char to be inserted */
{
	size_t	len;
	char N_FAR *temp;
	size_t	charSize;

	if (src == NULL || insertableChar == NULL)
		return (0);

	/* find number of bytes to move */
	len =0;
	temp = src;
	while (*temp++ != '\0')
		len++;

	len++; /* Copy Null terminator too */

	/* Get size of character to be inserted */
	temp = src;
	charSize= NWCharType((unsigned char) *insertableChar);
	temp +=charSize;

	/* make room to insert the char by moving it up size bytes */
	_fmemmove(temp, src, len);

	/* Now copy over the char into the hole we just made */
	NWstrncpy(src, insertableChar, charSize);

	return(charSize);
}
