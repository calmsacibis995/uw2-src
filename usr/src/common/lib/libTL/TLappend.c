/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/TLappend.c	1.4.5.2"
#ident  "$Header: TLappend.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

int
TLappend( tid, entryno, entry )
int tid;
entryno_t entryno;
entry_t	*entry;
{
	register rc;
	entry_t	*newentry, *TLe_malloc();

	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );

	if( !entry ) return( TLARGS );

	if( entryno < 0 || entryno > TLEND )
		return( TLBADENTRY );
	else if( entryno == TLBEGIN ) entryno = 0;

	/*
	else if( !IS_FROM_END(entryno) ) entryno--;
	*/

	if( TLe_diffformat( tid, entry ) ) return( TLDIFFFORMAT );

	/*
		TLe_find() insures that the entry, it it exists, is parsed and
		in main memory.
	*/
 	if( entryno != 0 && (rc = TLe_find( tid, entryno ) ) ) return( rc );

	/* Make our own copy of the entry */
	if( !(newentry = TLe_malloc()) ) return( TLNOMEMORY ); 
	if( (rc = TLe_copy( newentry, entry ) ) != TLOK ) {
		(void)TLfreeentry( tid, newentry );
		return( rc );
	}

	/* If entryno is relative to the end, make that calculation here */
	if( IS_FROM_END(entryno) )
		entryno = TLe_relative( tid, entryno );

	/* Add the entry into the table */
	if( (rc = TLe_add( tid, entryno + 1, newentry ) ) == TLOK )
		TLtables[ tid ].status |= MODIFIED;
	else (void)TLfreeentry( tid, newentry );

	return( rc );
}
