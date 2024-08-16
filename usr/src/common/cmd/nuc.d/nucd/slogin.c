/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:slogin.c	1.14"

/******************************************************************************
 ******************************************************************************
 *
 *	SLOGIN.C
 *
 *	Single login thread
 *
 ******************************************************************************
 *****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <dirent.h>
#include <sys/nwctypes.h>
#include <netdir.h>
#include <sys/tiuser.h>
#include <sys/nucam_common.h>
#include <nw/ntypes.h>
#include <nw/nwalias.h>
#include <nw/nwerror.h>
#include <nw/nwclient.h>
#include <nw/nwconnec.h>
#include <nw/nwbindry.h>
#include <sys/utsname.h>
#include "sl_ipc.h"
#include <sys/signal.h>
#include <pfmt.h>
#include <locale.h>
#include <nct.h>

#ifndef	FAILURE
#define	FAILURE	-1
#endif
#ifndef	SUCCESS
#define	SUCCESS	0
#endif

typedef struct {
	int				uid;		/* unhashed user id */
	void			*next;		/* next link */
	void 			*prev;		/* previous link */
	SL_USER_INFO_T	uinfo;		/* hashed user info */
} slogin_rec_t;

slogin_rec_t		*slogin_recs = NULL;	/* ptr to table */

extern int			nuc_debug;
extern int			nuc_slogin;
extern void			SLCloseIPC ( void );
extern FILE			*log_fd;

slogin_rec_t * 		slogin_find_user ( int uid );
void 				slogin_add_user ( SL_USER_INFO_T *ui );
void 				slogin_del_user ( SL_USER_INFO_T *ui );
int 				slogin_authen_user ( SL_USER_INFO_T *ui );
void 				slogin_close ( void );

/******************************************************************************
 *
 *	slogin ( int arg )
 *
 *	single login
 *
 *	Entry :
 *		arg			passed from thread create -- nothing
 *
 *	Exit :
 *		0			cool
 *		x			error
 *
 *****************************************************************************/

int
slogin ( int arg )
{
	int			rc;
	SL_CMD_T	Cmd;

	/*
	 *	initialize things
	 */

	signal ( SIGTERM, slogin_close );	

	/*
	 *	wait for a request
	 */
	if (nuc_slogin)
		(void)pfmt (log_fd, MM_ERROR, ":28:nucd:slogin: Started\n");

	setuid( 0 );
	while ( 1 ) {
		memset ( &Cmd, 0, sizeof ( SL_CMD_T ) );	/* insure its zero */
		if ( rc = SLGetRequest( &Cmd ) ) {
			/*
			 *	something went wrong
			 */
			(void)pfmt (log_fd, MM_ERROR, 
				":29:nucd:slogin: Get Request Failure - %d\n", rc);
			break;
		}

		if (nuc_slogin)
			(void)pfmt (log_fd, MM_ERROR,
				":30:nucd:slogin: Get Request Success\n" );

		if ( nuc_slogin) 
			(void)pfmt (log_fd, MM_ERROR,
				":104:nucd:slogin: Command = 0x%x\n",Cmd.cmd );

		switch( Cmd.cmd ){
			case REGISTER_USER:
				slogin_add_user ( &Cmd.userInfo );
				Cmd.rc = 0;
				break;
			case UNREGISTER_USER:
				slogin_del_user ( &Cmd.userInfo );
				Cmd.rc = 0;
				break;
			case AUTHENTICATE_USER:
				Cmd.rc = slogin_authen_user ( &Cmd.userInfo );
				break;
			default:
				Cmd.rc = 0;
				break;
		}


		/*
		 *	ack the request up
		 */
		if ( rc = SLSendReply( &Cmd ) ) {
			/*
		 	 *	something went wrong
			 */
			if (nuc_slogin)
				(void)pfmt (log_fd, MM_ERROR,
					":105:nucd:slogin: Send Reply Failure - %d\n", rc);
			continue;
		}

		if (nuc_slogin)
			(void)pfmt (log_fd, MM_ERROR,
				":118:nucd:slogin: Send Reply Success\n");
	}
	setuid( 0 );
	SLCloseIPC ();
}

/******************************************************************************
 *
 *	slogin_find_user ( int uid )
 *
 *	Find a uid in the linked list
 *
 *	Entry :
 *		uid			uid to find
 *
 *	Exit :
 *		*			ptr to entry
 *		NULL		no entry found
 *
 *****************************************************************************/

slogin_rec_t *
slogin_find_user ( int uid )
{
	slogin_rec_t			*p;

	p = slogin_recs;
	while ( p ) {
		if ( p->uid == uid ) {
			/*
			 *	found it
			 */
			return ( p );
		}
		p = p->next;
	} 
	return ( NULL );
}

/******************************************************************************
 *
 *	slogin_add_user ( SL_USER_INFO_T *ui )
 *
 *	Add a user to the list
 *
 *	Entry :
 *		*ui			ptr to user info
 *
 *	Exit :
 *		Nothing
 *
 *****************************************************************************/

void
slogin_add_user ( SL_USER_INFO_T *ui )
{
	slogin_rec_t	*p;
	int				i;

	Munge ( ui, sizeof ( SL_USER_INFO_T ) );	/* unpack */

	if (nuc_slogin) 
		(void)pfmt (log_fd, MM_ERROR,
			":119:nucd:slogin: Adding UID %d\n",ui->uid );

	if (slogin_find_user (ui->uid)) {
		/*
		 *	dude is already logged in
		 */
		if (nuc_slogin) 
			(void)pfmt (log_fd, MM_ERROR,
				":120:nucd:slogin: Already in the data base\n");
		return;
	}

	/*
	 *	doesn't exist -- add him
	 */

	if ( ( p = (slogin_rec_t *)malloc ( sizeof ( slogin_rec_t ) ) ) == NULL ) {
		/*
		 *	Unable to malloc memory
		 */
		if (nuc_slogin) 
			(void)pfmt (log_fd, MM_ERROR,
				":126:nucd:slogin: Unable to allocate memory\n");
		return;
	}

	p->uid = ui->uid;
	
	memcpy ( &p->uinfo, ui, sizeof ( SL_USER_INFO_T ) );
	for( i = (strlen( p->uinfo.password ) - 1); i>=0; i-- )
		*(p->uinfo.password + i) = toupper( *(p->uinfo.password + i) );
	Munge ( &p->uinfo, sizeof ( SL_USER_INFO_T ) );	/* pack */

	/*
	 *	add into list
	 */
	p->next = slogin_recs;
	p->prev = NULL;
	if ( slogin_recs ) {
		/*
		 *	Had something
		 */
		slogin_recs -> prev = p;
	}
	slogin_recs = p;
	if ( nuc_slogin) 
		(void)pfmt (log_fd, MM_ERROR,
			":127:nucd:slogin: Added '%s'\n",ui->userName);
	return;
}

/******************************************************************************
 *
 *	slogin_del_user ( SL_USER_INFO_T *ui )
 *
 *	Delete a user from the list
 *
 *	Entry :
 *		*ui			ptr to user info
 *
 *	Exit :
 *		Nothing
 *
 *****************************************************************************/

void
slogin_del_user ( SL_USER_INFO_T *ui )
{
	slogin_rec_t	*p;

	if ( nuc_slogin) 
		(void)pfmt (log_fd, MM_ERROR,
			":128:nucd:slogin: Deleting UID %d\n",ui->uid );

	if ( ( p = slogin_find_user ( ui->uid ) ) == NULL ) {
		/*
		 *	not in the list
		 */
		if ( nuc_slogin) 
			(void)pfmt (log_fd, MM_ERROR, ":129:nucd:slogin: Not in list\n");
		return;
	}

	/*
	 *	exists -- toast him
	 */
	if ( slogin_recs == p )
		slogin_recs = p->next;
	if ( p->prev ) 
		((slogin_rec_t *)p->prev)->next = p->next;
	if ( p->next )
		((slogin_rec_t *)p->next)->prev = p->prev;

	free ( p );

	if ( nuc_slogin) 
		(void)pfmt (log_fd, MM_ERROR, ":136:nucd:slogin: Deleted\n");

	return;
}

/******************************************************************************
 *
 *	slogin_authen_user ( SL_USER_INFO_T *ui )
 *
 *	Authenticate a user
 *
 *	Entry :
 *		*ui			ptr to user info record
 *					(ui->userName = server name)
 *
 *	Exit :
 *		0			cool -- authenticated
 *		x			error code
 *
 *****************************************************************************/

int
slogin_authen_user ( SL_USER_INFO_T *ui )
{
	slogin_rec_t	*p;
	ccode_t			ccode;
	NWCONN_HANDLE	connID;
	CONNECT_INFO	connInfo;
	int				myAttach = 0;
	int				origuid;
	char			*server = (char*)ui->userName;
	NWCConnString	nwcserver;
	boolean_t		nwattached = B_FALSE;

	if (nuc_slogin) 
		(void)pfmt (log_fd, MM_ERROR,
			":137:nucd:slogin: Authenticating UID %d\n",ui->uid );

	if ((p = slogin_find_user (ui->uid)) == NULL) {
		/*
		 *	not in the list
		 */
		if (nuc_slogin) 
			(void)pfmt (log_fd, MM_ERROR, ":129:nucd:slogin: Not in list\n");
		return (1); 
	}

	/*
	 *	be the other user
	 */
	origuid = geteuid ();
	if (seteuid (ui->uid)) {
		/*
	 	 *	unable to be this guy
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":138:nucd:slogin: Unable to seteuid to %d\n",ui->uid);
		return (1);
	}

	/*
	 * Is there a connID for this user?
	 */
	if (GetServerConnID (server, &connID) != 0) {
		/*
		 * Need to attach to get the connID.
		 */
		ccode = NWAttach( server, &connID, 0 );
		if ((ccode == SUCCESS) || (ccode == NWERR_ALREADY_ATTACHED)) {
			nwattached = B_TRUE;
		} else {
			/*
		 	 *	unable to attach to server
		 	 */
			if (nuc_debug)
				(void)pfmt (log_fd, MM_ERROR,
					":139:nucd:slogin: Unable to attach to '%s'\n",server );
			if ( nuc_slogin) 
				(void)pfmt (log_fd, MM_ERROR,
					":147:nucd:slogin: Unable to authenticate to '%s'\n",
					server);
			(void) seteuid (origuid);
			return ( FAILURE );
		}

	}

	/*
	 * 	Got the serverConnID.  Is the connection authenticated?
	 */
	if (nwisauthen (connID) == FALSE) {
		/*
		 * Not authenticated.  Need to authenticate to continue. Get the 
		 * NetWare userid name.
		 */
		Munge (&p->uinfo, sizeof (SL_USER_INFO_T));	/* unmunge record */
		if (nuc_slogin) 
			(void)pfmt (log_fd, MM_ERROR,
				":149:nucd:slogin: authenticating '%s' to server '%s'\n",
				p->uinfo.userName, server);
		ccode = NWLoginToFileServer (connID, (pnstr8)p->uinfo.userName,
						OT_USER, (pnstr8)p->uinfo.password);
		Munge (&p->uinfo, sizeof (SL_USER_INFO_T));	/* re-munge record */
		if (ccode != 0) {
			/*
			 *	unable to authenticate
			 */
			if (nuc_debug) 
				(void)pfmt (log_fd, MM_ERROR,
					":147:nucd:slogin: Unable to authenticate to '%s'\n",
					server);
			if (nuc_slogin) 
				(void)pfmt (log_fd, MM_ERROR,
					":147:nucd:slogin: Unable to authenticate to '%s'\n",
					server);
			(void) seteuid ( origuid );
			if (nwattached)
				NWDetach (connID);
			return(FAILURE);
		}
	}

	/*
	 *	we be authenticated
	 */
	if (seteuid ( origuid ) > 0) {
		/*
		 *	failed to switch back to original
		 */
		if (nuc_debug)
			(void)pfmt ( log_fd, MM_ERROR,
				":210:nucd:slogin: setuid to original failed\n");
		if (nwattached)
			NWDetach (connID);
		return ( FAILURE );
	}
	if ( nuc_debug) 
		(void)pfmt ( log_fd, MM_ERROR,
			":266:nucd:slogin: Authenticated successfully\n" );

	return ( SUCCESS );
}

/******************************************************************************
 *
 *	slogin_close ( void )
 *
 *	Close everything and exit
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		Nothing (well, something, but..)
 *
 *****************************************************************************/

void
slogin_close ( void )
{
	SLCloseIPC ();
	exit ( 0 );
}
