/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/TLsync.c	1.2.5.2"
#ident  "$Header: TLsync.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

int
TLsync( tid )
int tid;
{
	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );

	/* Write out new table */
	return( TLt_sync( tid ) );
}
