/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/TLgetentry.c	1.3.5.3"
#ident  "$Header: TLgetentry.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];
extern entry_t *TLe_malloc();

ENTRY
TLgetentry( tid )
int tid;
{
	entry_t *e_malloc();
	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( NULL );

	return( (ENTRY)TLe_malloc() );
}

int
TLfreeentry( tid, entry )
entry_t *entry;
{
	void e_free();
	tid--;
	if( !TLt_valid( tid ) ) return( TLBADID );
	if( !entry )	return( TLARGS );
	TLe_free( entry );
	(void) Free( entry );
	return( TLOK );
}
