/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkutils.c	1.4.5.3"
#ident  "$Header: bkutils.c 1.2 91/06/21 $"

#include	<stdio.h>
#include	<ctype.h>
#include	<errno.h>
#include	<table.h>
#include	<backup.h>
#include	<bkreg.h>

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

extern char *optname;
extern char *brcmdname;
extern int nflags;

/*
	Check the combination of flags given on the command line - make sure
	that there are exactly enough and not too many.

	Each option has its own bit. 
*/
int
validcombination( flag, given, f_allowed, f_reqd )
char flag;
unsigned given, f_allowed, f_reqd;
{
	unsigned too_few, too_many;
	register i, offset, error = FALSE;
	too_few = (~given & f_reqd);
	too_many = (given & ~f_allowed);
	for( i = 1, offset = 0; i < (1<<nflags); i <<= 1, offset++ ) {
		if( (i & (too_few)) /* && !(i & f_allowed) */ ) {
			(void) fprintf( stderr, "%s: the -%c option requires the -%c option.\n",
				brcmdname, flag, optname[ offset ] );
			error = TRUE;
		}
		if( i & too_many ) {
			(void) fprintf( stderr,
				"%s: the -%c option does not use the -%c option.\n",
				brcmdname, flag, optname[ offset ] );
			error = TRUE;
		}
	}
	return( !error );
}



/* call TLassign and then log errors */
void
bkTLassign( tid, entry, fieldname, value, errstr )
int tid;
ENTRY *entry;
unsigned char *fieldname, *value;
char *errstr;	/* bklog message inclusion -- should be calling subrtn name */
{
	register int rc;

	if ( (rc = TLassign( tid, entry, fieldname, value )) != TLOK ) {
		brlog( "%s(): TLassign() returned error %d", errstr, rc);
	}
	return;
}
