/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/vgroup.c	1.2.10.2"
#ident  "$Header: vgroup.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<grp.h>
#include	<users.h>

extern int valid_gname(), valid_gid();
extern long strtol();

/*
	validate a group name or number and return the appropriate
	group structure for it.
*/
int
valid_group( group, gptr )
char *group;
struct group **gptr;
{
	register gid_t gid;
	char *ptr;

	if( isalpha(*group) ) return( valid_gname( group, gptr ) );

	if( isdigit(*group) ) {

		gid = (gid_t) strtol(group, &ptr, (int) 10);
		if( *ptr ) return( INVALID );

		return( valid_gid( gid, gptr ) );

	}
	return( INVALID );
}
