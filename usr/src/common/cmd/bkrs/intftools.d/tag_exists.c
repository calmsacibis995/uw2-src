/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/tag_exists.c	1.2.5.2"
#ident  "$Header: tag_exists.c 1.2 91/06/21 $"

#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <table.h>
#include <bkreg.h>
#define TRUE 1
#define FALSE 0

/* Program returns 0 if tag exists in table, non-zero if tag does not exist in */
/* the table.  The first argument is the tag, the second is the table. */
/* Note: table must exist when this program is called (i.e., caller must */
/* verify that table file exists. */
main( argc, argv )
int argc;
char *argv[];
{
	char *table;
	unsigned char *tag;

	int tid;

	struct TLdesc description;
	struct TLsearch TLsearches[ 2 ];
	struct TLsearch *tls;

	void exit();

	if( argc != 3 )
		exit( 1 );
	tag = (unsigned char *)argv[1];
	table = argv[2];

	strncpy( (char *)&description, "", sizeof( struct TLdesc ) );
	description.td_format = R_BKREG_F;
	if( TLopen( &tid, table, &description, O_RDONLY ) != TLOK )
		exit( -1 );
	
	tls = TLsearches;
	tls->ts_fieldname = R_TAG;
	tls->ts_pattern = tag;
	tls->ts_operation = (int(*)() )TLEQ;
	tls++;
	tls->ts_fieldname = (unsigned char *)0;

	if( TLsearch1( tid, TLsearches, TLBEGIN, TLEND, TRUE ) == TLFAILED )
		exit( 1 ); /* FALSE for the shell */
	else
		exit( 0 ); /* TRUE (no error) for the shell */
}
