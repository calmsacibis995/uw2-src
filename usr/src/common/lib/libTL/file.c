/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/file.c	1.10.5.2"
#ident  "$Header: file.c 1.2 91/06/25 $"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <table.h>
#include <internal.h>

/* How much to read at a time */
#define	NTIMES	4
#define	TLBSIZE	(TL_MAXLINESZ / NTIMES)

extern tbl_t TLtables[];
extern unsigned char TLgenbuf[];

int
TLfl_getentry( tid, buffer, count )
int tid, count;
unsigned char *buffer;
{
	tbl_t *tptr = TLtables + tid;
	register unsigned char *to = buffer, *from = TLgenbuf, eoe;
	register rc, nchars, nread, ntimes, done = FALSE, escape = FALSE;

	eoe = tptr->description.td_eoe;


	/* Break up reading the entry into NTIMES reads */
	for( ntimes = 0, nread = 0; !done && ntimes < NTIMES; ntimes++ ) {

		if( (rc = Read( tptr->file.fid, from, TLBSIZE ) ) < 0 )
			return( TLFAILED );

		if( rc == 0 ) 
			return( TLEOF );

		nread += rc;

		while( rc-- > 0 && !(done = (*from == eoe && !escape)) ) {

			if (escape)  /*handle pending escape */
				{
				escape=FALSE; /*this char must be copied */
				*to++ = *from++;
				continue;
				}

			if( *from == '\\' ) 
			{
			escape = TRUE;  /*save the escape*/
				/* Is this the last char. read so far? */
				if (rc == 0)
					{
					*to++ = *from++;
					break;
					}  /*end of if rc == 0 */
			
	
			}  /*end of if  from == escape*/

			*to++ = *from++;
		} /*  end of while */

	}

	if( !done ) return( TLTOOLONG );
	
	*to = '\0';

	nchars = to - buffer + 1;

	/* reset fileptr to end of entry */
	if( nchars != nread ) 
		(void) lseek( tptr->file.fid, nchars - nread, 1 );

	return( nchars );
}

TLfl_lock( fid )
int fid;
{
	return( ( Lseek( fid, 0, 0 ) == -1 || lockf( fid, F_LOCK, 0 ) == -1 )?
		TLFAILED: TLOK );
}

int 
TLfl_unlock( fid )
int fid;
{
	return( ( Lseek( fid, 0, 0 ) == -1 || lockf( fid, F_ULOCK, 0 ) == -1 )?
		TLFAILED: TLOK );
}
		
int
TLfl_copy( to, from )
int to, from;
{
	unsigned char buffer[ 2048 ];
	register count = 0;
	(void) Lseek( to, 0, 0 );
	(void) Lseek( from, 0, 0 );
	while( (count != -1) && ((count = Read( from, buffer, 2048 )) > 0) )
		count = Write( to, buffer, count );
	return( count == -1? TLFAILED: TLOK );
}
