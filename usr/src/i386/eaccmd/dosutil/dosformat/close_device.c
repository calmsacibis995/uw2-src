/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/close_device.c	1.1.1.2"
#ident  "$Header: close_device.c 1.1 91/07/03 $"

/* #define		DEBUG		1	/* */

/*
		close_device(handle)

	Use this to close the device when finished.

	Return 0 on success, -1 on failure.
*/

#include	"MS-DOS.h"

#include	<stdio.h>

close_device(handle)
int	handle;
{
	int	index;

#ifdef DEBUG
	(void) fprintf(stderr, "close_device(): DEBUG - Closing down Handle: %d\n", handle);
#endif

	if ((index = lookup_device(handle)) == -1) {
#ifdef DEBUG
		(void) fprintf(stderr, "close_device(): Error - Handle %d not found in device table\n", handle);
#endif
		return(-1);
	}

	if (close(handle) == -1) {
#ifdef DEBUG
		(void) fprintf(stderr, "close_device(): Failed to close device\n");
		perror("	Reason");
#endif
		return(-1);
	}

	TABLE.handle = 0;

	(void) free(TABLE.our_fat);

#ifdef DEBUG
	(void) fprintf(stderr, "close_device(): DEBUG - Handle: %d closed\n", handle);
#endif

	return(0);
}
