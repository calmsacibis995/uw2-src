/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/lookup_dev.c	1.1.1.2"
#ident  "$Header: lookup_dev.c 1.1 91/07/03 $"

/*
			lookup_device(handle)

	Search for an entry in our device table using
	the passed handle. Return its index - if found.
	Return -1 on failure.
*/
#include	"MS-DOS.h"

int
lookup_device(handle)
int	handle;
{
	int	index;

	/*
		Look for our specified handle.
	*/
	for (index = 0; index < MAX_DEVICES; index++)
		if (TABLE.handle == handle)
			break;

	/*
		If we reached end of our table, no match
	*/
	if (index == MAX_DEVICES)
		index = -1;

	return(index);
}
