/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkisjob.c	1.5.5.2"
#ident  "$Header: bkisjob.c 1.2 91/06/21 $"

#include <string.h>
#include <ctype.h>

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

/* Does this string have the right form for a restore jobid? */
int
is_rsjobid( string )
char *string;
{
	register len = strlen( string ), i;

	/*
		Valid form is 'rest-NA' where N is some number of digits and
		A is an alphabetic character.
	*/
	if ( len < 7 ) return ( FALSE );
	if( strncmp( string, "rest-", 5 ) ) return( FALSE );
	if( !isalpha( string[ len - 1 ] ) ) return( FALSE );
	for( i = 5; i < len - 1; i++ ) 
		if( !isdigit( string[ i ] ) ) return( FALSE );
	return( TRUE );
}

/* Does this string have the right form for a backup jobid? */
int
is_bkjobid( string )
char *string;
{
	register len = strlen( string ), i;

	/*
		Valid form is 'back-N' where N is some number of digits
	*/
	if( strncmp( string, "back-", 5 ) ) return( FALSE );
	for( i = 5; i < len; i++ ) 
		if( !isdigit( string[ i ] ) ) return( FALSE );
	return( TRUE );
}
