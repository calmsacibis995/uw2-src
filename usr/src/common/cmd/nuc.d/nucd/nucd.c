/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:nucd.c	1.16"
/******************************************************************************
 ******************************************************************************
 *
 *	NUCD.C
 *
 *	All-In-One NetWare Unix Client Daemon
 *
 *	Parameters :
 *			-n			don't daemonize
 *			-d			debug on for everything
 *			-l<file>	log file to use
 *			-s 			single login debug
 *
 *	Notes :
 *		- nucam	 		: auto mounter
 *		- nwlogin	 	: another part of the auto mounter
 *		- unmount 		: auto-unmounter
 *		- message	 	: message thingie
 *		- slogin		: single login
 *
 ******************************************************************************
 *****************************************************************************/

#define	MESSAGE_THREAD		0x00		/* message thread ID */
#define	UNMOUNT_THREAD		0x01		/* unmount thread ID */
#define	NUCAM_THREAD		0x02		/* auto mounter thread ID */
#define	NWLOGIN_THREAD		0x03		/* login thread ID */
#define	SLOGIN_THREAD		0x04		/* single login thread ID */
#define	SIGHUP_THREAD		0x05		/* Handle IPX M_HANGUP thread ID */
#define	MAX_THREADS			0x06		/* max threads to allocate */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <thread.h>

#include <pfmt.h>
#include <locale.h>

thread_t	nucd_thread [ MAX_THREADS ];	/*	threads to use	*/
FILE		*log_fd;						/*	Log file		*/

/*
 *	various command options
 */

int			nuc_debug;			/* 1 if debugging turned on */
int			nuc_daemon;			/* 1 if should daemonize */
int			nuc_slogin;			/* 1 if only single login */

extern	char *optarg;

long	unmount_age_limit;	/* minimum inactivity time before unmount */

int 		main ( int argc, char **argv );
void 		daemonize ( char *processName );
int 		thread_create ( void *start_addr, thread_t *thread_ptr, int arg );

/*
 *	various threads
 */
int 		message ( int arg );
int 		unmount ( int arg );
int 		nucam ( int arg );
int 		nwlogin ( int arg );
int 		slogin ( int arg );
int 		sighup ( int arg );

/******************************************************************************
 *
 *	main ( int argc, 
 *	       char **argv )
 *
 *	Main entry
 *
 *	Entry :
 *		argc		arg count
 *		argv		ptr to arg stuff :
 *
 *					d		debug on
 *					n		don't daemonize
 *					u###	unmount age limit
 *
 *	Exit :
 *		0			cool
 *		x			error
 *
 *****************************************************************************/

int
main ( int argc, char **argv )
{
	uid_t		uid;
	int			c;
	boolean_t	unmount_thread, xauto_thread, message_thread,
				auto_mounter_thread, slogin_thread, sighup_thread;

	unmount_thread = xauto_thread = message_thread =
		auto_mounter_thread = slogin_thread = sighup_thread = B_TRUE;

	/*
	 *	internationalization crap
	 */

	setlocale(LC_ALL, "");
	(void)setcat("uvlnuc");
	setlabel("UX:nucd");

	/*
	 * If the log file already exists delete it.
	 */
	unlink ("/var/netware/nucd.log");

	/*
	 * Open the log file in append mode.
	 */
	if ((log_fd = fopen ("/var/netware/nucd.log", "a")) == NULL) {
		(void)pfmt ( stderr, MM_ERROR, ":398: Cannot open log file.\n");
		exit (1);
	}

	if ((chmod ("/var/netware/nucd.log", S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0){
		(void)pfmt ( stderr, MM_ERROR, ":399: Cannot chmod log file.\n");
		exit (1);
	}

	/*
	 * Use unbuffered I/O on the log file.
	 */
	setbuf (log_fd, (char *)NULL);

	/*
	 *	parse up parameters passed in
	 */
	nuc_debug = 0;		/* no debugging */
	nuc_daemon = 1;		/* make me a daemon */
	nuc_slogin = 0;		/* everything */
	unmount_age_limit = 600;	/* default unmount age limit (10 min) */

	while (( c = getopt ( argc, argv, "dnu:USFAMXs" ) ) != EOF ) {

		switch ( c ) {
			case	'd' :
				/*
				 *	turn ON debug options
				 */
				nuc_debug = 1;
				break;

			case	'n' :
				/*
				 *	turn OFF daemonize 
				 */
				nuc_daemon = 0;
				break;
	
			case	'u' :
				/*
				 *	new age limit
				 */
				unmount_age_limit = strtol ( optarg, NULL, 0 );
				break;

			case	's' :
				/*
				 *	single login
				 */
				nuc_slogin = 1;
				break;
			
			case	'U' :
				unmount_thread = B_FALSE;
				break;

			case	'S' :
				slogin_thread = B_FALSE;
				break;

			case	'A' :
				auto_mounter_thread = B_FALSE;
				break;

			case	'M' :
				message_thread = B_FALSE;
				break;
			
			case	'X' :
				xauto_thread = B_FALSE;

		}
	}

	/*
	 *	initial checks and daemonization
	 */
	uid = getuid ();
	if (uid != 0) {
		/*
		 * NUCAM daemon must be started by the super user.
		 */
		(void)pfmt ( log_fd, MM_ERROR, ":131: Must be root.\n");
		exit (1);
	}

	if ( nuc_debug ) 
		(void)pfmt ( log_fd, MM_NOSTD, ":132: nucd: Starting\n");


	if ( nuc_daemon ) {
		/*
		 *	wants me to be a daemon
		 */
		daemonize ( "mr nuc daemon" );
	}

	/*
	 *	do random library initialization
	 */

	(void)initreq ( NULL, NULL );

	/*
	 *	Create all the threads
	 */

	if (message_thread)
		/* message handler */
		thread_create ( (void *)message, &nucd_thread[MESSAGE_THREAD], 0 );

	if (unmount_thread)
		/* auto unmount */
		thread_create ( (void *)unmount, &nucd_thread[UNMOUNT_THREAD],
			unmount_age_limit );

	if (auto_mounter_thread)
		/* auto mounter */
		thread_create ( (void *)nucam, &nucd_thread[NUCAM_THREAD], 0 );

	if (xauto_thread)
		/* auto-login */
		thread_create ( (void *)nwlogin, &nucd_thread[NWLOGIN_THREAD], 0 );

	if (slogin_thread)
		/* single login */
		thread_create ( (void *)slogin, &nucd_thread[SLOGIN_THREAD], 0 );

	if (sighup_thread)
		/* handle IPX M_HANGUP */
		thread_create ( (void *)sighup, &nucd_thread[SIGHUP_THREAD], 0 );

	/*
	 *	go to sleep for a while
	 */

	while ( 1 ) {
		sleep ( 60 );
	}
}

/******************************************************************************
 *
 *	daemonize ( char *processName )
 *
 *	Fork off a program as a daemon and jettison everything
 *
 *	Entry :
 *		*processName		ptr to name of process
 *
 *	Exit :
 *		Nothing			(exits if something goes wrong)
 *
 *****************************************************************************/

void
daemonize ( char *processName )
{
	switch(fork1 ()) {
		case -1:
			/*
			 * Can't fork
			 */
			(void)pfmt (log_fd, MM_ERROR,
				":133: %s: can't daemonize\n", processName);
			exit (-1);
			
		case 0:
			/*
			 * Go into the back ground, and become silent
			 */
			(void) setpgrp ();
			if ( !(nuc_debug | nuc_slogin) ) {
				/*
				 *	close all my FDs
				 */
				close (0);
				close (1);
				close (2);
			}
			break;
		default:
			/*
			 * Release from the shell.
			 */
			exit (0);
	}
}

/******************************************************************************
 *
 *	thread_create ( void *start_addr, 
 *			thread_t *thread_ptr, 
 *			int arg )
 *
 *	Create a thread -- print out any error messages if needed
 *
 *	Entry :
 *		*start_addr		starting address of thread
 *		*thread_ptr		ptr to var that thread address is ret
 *		arg				arg to pass to start routine
 *
 *	Exit :
 *		0				cool
 *		x				error
 *
 *****************************************************************************/

int
thread_create ( void *start_addr, thread_t *thread_ptr, int arg )
{
	int				ret;

	if ( nuc_debug ) {
		/*
		 *	tell the user 
		 */
		(void)pfmt ( log_fd, MM_NOSTD,
			":134: Creating Thread - Start Addr = 0x%x  Arg = 0x%x\n",
			start_addr, arg );
	}

	if ( ret = thr_create ( NULL, 0, (void *(*))((void *)start_addr),
			(void *)arg, THR_BOUND, thread_ptr ) ) {
		/*
		 *	something went wrong :
		 */
		(void)pfmt ( log_fd, MM_ERROR, ":5: Error creating thread - " );

		switch ( ret ) {
			case	ENOMEM :
				(void)pfmt ( log_fd, MM_NOSTD, ":135:No Memory\n");
				break;
			case	EINVAL :
				(void)pfmt ( log_fd, MM_NOSTD, ":7:Invalid Parameter\n");
				break;
			case	EAGAIN :
				(void)pfmt ( log_fd, MM_NOSTD, ":8:Resource Limit Exceeded\n");
				break;
			case	EFAULT :
				(void)pfmt ( log_fd, MM_NOSTD, ":9:Illegal Address\n");
				break;
			default :
				(void)pfmt ( log_fd, MM_NOSTD, ":10:Error = %d\n",ret );
		}
		return ( ret );
	}

	/*
	 *	cool
	 */
		
	return ( 0 );
}
