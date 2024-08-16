/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/lookup_drv.c	1.1.1.2"
#ident  "$Header: lookup_drv.c 1.1 91/07/03 $"

/*
		lookup_drive(handle)

	This routine returns the index into the hardware table
	for the passed drive spcification.

	Returns index on success, -1 on error.
*/

#include	"MS-DOS.h"

#include	<stdio.h>

lookup_drive(local_drive)
char	local_drive;
{
	int	index;

	for (index = 0; HARDWARE.device_letter != '\0' && index < MAX_DEVICES; index++)
		if (HARDWARE.device_letter == local_drive)
			break;

	if (index == MAX_DEVICES) {
		(void) fprintf(stderr, "lookup_drive(): Error Drive '%c' invalid\n", local_drive);
		return(-1);
	}

	return(index);
}
