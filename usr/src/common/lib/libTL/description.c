/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/description.c	1.2.5.2"
#ident  "$Header: description.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

void
TLd_free( desc )
TLdesc_t *desc;
{
	if( desc->td_format ) {
		Free( desc->td_format );
		desc->td_format = NULL;
	}
}

/*
	Field sep, end of entry, and comment characters must be distinct
	or the description is ambiguous. They cannot be equal to backslash,
	either.
*/
int
TLd_ambiguous( d )
TLdesc_t *d;
{
	if( d->td_fs == '\\' || d->td_eoe == '\\' || d->td_comment == '\\' )
		return( 1 );
	if( d->td_fs ) {
		if( d->td_eoe && d->td_fs == d->td_eoe ) return( 1 );
		if( d->td_comment && d->td_fs == d->td_comment ) return( 1 );
	}
	if( d->td_eoe && d->td_comment )
		if( d->td_eoe == d->td_comment ) return( 1 );
	return( 0 );
}
