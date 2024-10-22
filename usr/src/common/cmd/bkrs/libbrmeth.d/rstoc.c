/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/rstoc.c	1.4.5.2"
#ident  "$Header: rstoc.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<time.h>
#include	<backup.h>
#include	<bkmsgs.h>
#include	<bkrs.h>

extern int brtype;
extern pid_t bkdaemonpid;

extern unsigned int strlen();
extern char *strcpy();
extern int bkm_send();

int
rstocname( tocname )
char *tocname;
{
	bkdata_t msg;

	switch( brtype ) {

	case RESTORE_T:

		if( !tocname ) return( BRBADARGS );

		if( strlen( tocname ) > BKFNAME_SZ )
			return( BRTOOBIG );

		(void) strcpy( msg.rst_o_c.tocname, tocname );

		if( bkm_send( bkdaemonpid, RSTOC, &msg ) == -1 )
			return( BRFATAL );
		break;

	case BACKUP_T:
		break;

	default:
		return( BRNOTINITIALIZED );
	}
	return( BRSUCCESS );
}

