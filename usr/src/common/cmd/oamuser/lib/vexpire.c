/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/vexpire.c	1.5.10.2"
#ident  "$Header: vexpire.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<time.h>
#include	<users.h>

extern long p_getdate();

/*
	Validate an expiration date string
*/
int
valid_expire( string, expire )
char *string;
time_t *expire;
{
	time_t tmp, now;
	struct tm *tm;

	if( !(tmp = (time_t) p_getdate( string ) ) )
		return( INVALID );

	now = time( (long *)0 );
	
	/* Make a time_t for midnight tonight */
	tm = localtime( &now );
	now -= tm->tm_hour * 60*60 + tm->tm_min * 60 + tm->tm_sec;
	now += 24 * 60 * 60;

	if( tmp < now ) return( INVALID );

	if( expire ) *expire = now;

	return( UNIQUE );
}
