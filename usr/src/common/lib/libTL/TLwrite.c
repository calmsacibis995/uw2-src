/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/TLwrite.c	1.2.5.2"
#ident  "$Header: TLwrite.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

int
TLwrite( tid, entryno, newentry )
int tid;
entryno_t entryno;
entry_t	*newentry;
{
	register rc;
	entry_t *oldentry;
	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );

	if( !newentry ) return( TLARGS );

	if( T_EMPTY(tid) || entryno < 0 || entryno > TLEND )
		return( TLBADENTRY );
	else if( entryno == TLBEGIN )
		entryno = 1;

	if( TLe_diffformat( tid, newentry ) ) return( TLDIFFFORMAT );

	/*
		TLe_find() insures that the entry, it it exists, is parsed and
		in main memory.
	*/
 	if( (rc = TLe_find( tid, entryno ) ) ) return( rc );

	/* If entryno is relative to TLEND, calculate the absolute entryno */
	if( IS_FROM_END(entryno) )
		entryno = TLe_relative( tid, entryno );

	/* Now, get a pointer to the entry */
	if( (rc = TLe_getentry( tid, entryno, &oldentry ) ) != TLOK ) return( rc );

	/* Replace the old entry with the new */
	rc = TLe_copy( oldentry, newentry );

	if( rc == TLOK )
		TLtables[ tid ].status |= MODIFIED;
	return( rc );
}
