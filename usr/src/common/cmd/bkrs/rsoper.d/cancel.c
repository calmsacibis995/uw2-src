/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/rsoper.d/cancel.c	1.4.5.3"
#ident  "$Header: cancel.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<errno.h>
#include	<table.h>
#include	<rsstatus.h>
#include	<errors.h>
#include        <bktypes.h>        

extern long strtol();
extern char *br_get_rsstatlog_path();
extern int f_exists();
extern void bkerror();
extern void brlog();
extern int atoi();
extern void m_send_msg();
extern char *uid_to_uname();

/*
	Cancel a particular restore request. Success flag tells whether or not
	user gets a 'complete' vs. 'cancelled' indication.
*/
int
cancel( jobid, success )
argv_t *jobid;         
int success;
{
	register rc, entryno;
	char *path, *uid_p;
	int tid;
	uid_t uid;
	ENTRY eptr;
	TLdesc_t descr;
	TLsearch_t sarray[ 2 ];
 	int i;

	path = (char *)br_get_rsstatlog_path();

	descr.td_fs = descr.td_eoe = descr.td_comment = (unsigned char)'\0';
	if( !f_exists( path ) ) {
		bkerror( stderr, ERROR8, jobid );
		return( 0 );
	} else descr.td_format = (unsigned char *)R_RSSTATUS_F;

	if( (rc = TLopen( &tid, path, &descr, O_RDWR ) ) != TLOK
		&& rc != TLDIFFFORMAT ) {
		if( rc = TLBADFS )
			TLclose( tid );
		if( rc == TLFAILED ) 
			brlog( "TLopen of restore status table %s fails: errno %ld",
				path, errno );
		else brlog( "TLopen of status table %s returns %d",
			path, rc );
		bkerror( stderr, ERROR12 );
		return( 0 );
	}

        for(i=0; (*jobid)[i]; i++) {   
	/* Now find out the entryno of the entry just written */
	sarray[ 0 ].ts_fieldname = RST_JOBID;
        sarray[ 0 ].ts_pattern = (unsigned char *)(*jobid)[i];      	
	sarray[ 0 ].ts_operation = (int (*)())TLEQ;
	sarray[ 1 ].ts_fieldname = (unsigned char *)0;

	if( (entryno = TLsearch1( tid, sarray, TLEND, TLBEGIN, TL_AND )) < 0 ) {
		/* Not found */
                bkerror( stderr, ERROR8, (*jobid)[i] );     
                continue;                                   	
	}

	/* Get a new entry structure */
	if( !(eptr = TLgetentry( tid )) ) {
                bkerror( stderr, ERROR7, (*jobid)[i] );    
                continue;                                  		
	}

	if( TLread( tid, entryno, eptr ) != TLOK ) {
                bkerror( stderr, ERROR7, (*jobid)[i] );  
                continue;                                		
	}

	if( !(uid_p = (char *)TLgetfield( tid, eptr, RST_UID ) ) ) {
		bkerror( stderr, ERROR9 );
                continue;     	
	}
	uid = strtol( uid_p, (char **)0, 10 );

	if( (rc = TLdelete( tid, entryno )) == TLOK
		&& (rc = TLsync( tid )) == TLOK )  
                m_send_msg( (*jobid)[i], uid_to_uname( uid ), 
			(success? "has been removed": "has been cancelled") );
        else                                                              
                bkerror( stderr, ERROR11, (success? "remove": "cancel") );
        }                                                                 

	(void) TLclose( tid );
	return( 1 );
}
