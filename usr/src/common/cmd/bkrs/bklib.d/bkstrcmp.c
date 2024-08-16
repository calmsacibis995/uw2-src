/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkstrcmp.c	1.3.5.2"
#ident  "$Header: bkstrcmp.c 1.2 91/06/21 $"

extern int strcmp();

/* strcmp + handle NULL pointers */
int
bkstrcmp( a, b )
char *a, *b;
{
	if( a && b ) return( strcmp( a, b ) );
	if( a ) return( -1 );
	return( b != (char *)0 );
}
