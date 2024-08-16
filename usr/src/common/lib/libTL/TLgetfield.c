/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/TLgetfield.c	1.3.5.6"
#ident  "$Header: TLgetfield.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

unsigned char *
TLgetfield( tid, entry, fieldname )
int tid;
entry_t *entry;
unsigned char *fieldname;
{
	register tbl_t *tptr;
	register offset;
	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( NULL );
	if( !entry || !fieldname ) return( NULL );

	tptr = TLtables + tid;

	if( !Strcmp( fieldname, TLCOMMENT ) )

		if ( E_NFIELDS(entry) )
			return(NULL);
		else
		if ( E_COMMENT(entry) == NULL )
			return( (unsigned char *) "");
		else
			return ( E_COMMENT(entry) );

	if( !Strcmp( fieldname, TLTRAILING ) )
		return( (E_NFIELDS(entry) == 0)? NULL: E_COMMENT(entry) );

	if( (offset = TLf_find( &(tptr->fieldnames), fieldname )) == -1 )
		return( NULL );
	if( offset > E_NFIELDS(entry) ) return( NULL );

	return( (unsigned char *)*E_GETFIELD( entry, offset ) );
}
