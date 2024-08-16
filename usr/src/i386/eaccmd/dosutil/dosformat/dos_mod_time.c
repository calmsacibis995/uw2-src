/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/dos_mod_time.c	1.1.1.2"
#ident  "$Header: dos_mod_time.c 1.1 91/07/03 $"

/*
			dos_mod_time(displacement)

	Retrieves the modification time from the current sector_buffer,
	at the passed displacement.  Returns a character string of 
	the format hh:mm.
*/

char	_dmt_ret_value[6];

#include	<stdio.h>

#include	"MS-DOS.h"

char	*
dos_mod_time(i)
int	i;	/* displacement */
{
	char	mode;
	int	hours = (sector_buffer[TIME + i + 1]) >> 3;

	if (hours == 0) {
		hours = 12;
		mode = 'a';
	}
	else if (hours > 12) {
		mode = 'p';
		hours -= 12;
	}
	else mode = 'a';

	(void) sprintf(_dmt_ret_value, "%2d:%02d%c", hours, ((sector_buffer[TIME + i + 1] & 0x07) << 3) | ((sector_buffer[TIME + i] & 0xE0) >> 5), mode);

	return(_dmt_ret_value);
}
