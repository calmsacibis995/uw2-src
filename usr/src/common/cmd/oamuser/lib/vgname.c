/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/vgname.c	1.2.10.4"
#ident  "$Header: vgname.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<grp.h>
#include	<userdefs.h>
#include	<users.h>

#define nisname(n) (*n == '+' || *n == '-')

extern struct group *nis_getgrnam();

extern unsigned int strlen();

/*
 * validate string given as group name.
 */
int
valid_gname( group, gptr )
char *group;
struct group **gptr;
{
	register struct group *t_gptr;
	register char *ptr = group;

	if( !group || !*group || (int) strlen(nisname(group)?(group+1):group) >= MAXGLEN )
		return( INVALID );

	for( ; *ptr != NULL; ptr++ ) 
		if( !isprint(*ptr) || (*ptr == ':') )
			return( INVALID );

	if( t_gptr = nis_getgrnam( group ) ) {
		if( gptr ) *gptr = t_gptr;
		return( NOTUNIQUE );
	}

	return( UNIQUE );
}
