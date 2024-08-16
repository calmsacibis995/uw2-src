/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/misc.c	1.3"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/misc.c,v 1.4 1994/10/06 22:05:57 nick Exp $";
#endif

/*
 * BEGIN_MANUAL_ENTRY( rprinter (daemon), \
 *                     api/utils/rprinter/rprinter )
 *			A Portable NetWare deamon which emulates the RPRINTER
 *			DOS TSR with extensions added
 *
 * SYNOPSIS
 *			rprinter [-d main_console_output] [-x debug_file] [-h]
 *
 * INPUT
 *			-d main_console_output
 *								sets what types of messages will be sent
 *								to the main console while rprinter is
 *								running. Each letter of the parameter
 *								following the '-d' has reference to a
 *								type of message: 'n' = normal,
 *								'v' = verbose, 'w' = warning,
 *								'e' = error and 'd' = debug. If the
 *								parameter consists of only 'no', it
 *								means no output to the main console.
 *								(default: 'new' ie. normal, error
 *								and warning messages)
 *
 *			-x debug_file		writes messages of all types to the
 *								debug_file path; each message is
 *								time-stamped (default: no debug output
 *								file)
 *
 *			-h					sends to standard output a help message
 *								about how to run rprinter.
 *
 * DESCRIPTION
 *			This utility allows NetWare print jobs in any NetWare
 *			file-server print queue to be printed on any Portable
 *			NetWare host printer by way of any NetWare PServer on the
 *			network.
 *
 *			The extensions added to this RPrinter which go beyond the
 *			DOS RPRINTER are:
 *				- the ability by way of the control_file of having
 *				  rprinter handle more than one PServer/printer
 *				  pair
 *
 * SEE ALSO
 *			PServer, pconsole
 *
 * END_MANUAL_ENTRY ( 3/18/91 )
 */

#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "rprinter.h"
#include "inform.h"
#include "main_proto.h"
#include "upconfig_proto.h"
#include "misc_proto.h"
#include "config_proto.h"

#define DEFAULT_MESSAGE_TYPES		MSG_NORMAL | MSG_WARN | MSG_ERROR


extern char	*optarg;
extern int	 opterr;
extern int	 optind;



static void setConsoleMessageTypes(
	char	 types[],
	uint16	*msgTypesToInform);

/*
 * BEGIN_MANUAL_ENTRY( Initialize, \
 *                     api/utils/rprinter/initial )
 *			RPRINTER initialization routine
 *
 * SYNOPSIS
 *			Initialize( argc, argv )
 *			int   argc;
 *			char *argv[];
 *
 * INPUT
 *			argc				passed directly from 'main'
 *
 *			argv				passed directly from 'main'
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *			main, ShutDown, InformInit
 *
 * END_MANUAL_ENTRY ( 3/18/91 )
 */
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

void
NPDaemon(void)
{
	struct rlimit limits;

	if (fork())
		exit(0); /* exit the parent */
#ifdef DEBUG
	errno = 0;
#endif
	setsid();
/*
	getrlimit( RLIMIT_NOFILE, &limits );
	limits.rlim_cur = 40;
	setrlimit( RLIMIT_NOFILE, &limits );
	getrlimit( RLIMIT_NOFILE, &limits );
*/	
#ifdef  DEBUG
	printf("Setsid() status = %d -- %s\n", errno, errno ? strerror(errno) : "OK");
#endif
}


void InitConsole(int NPdebugFlag, int NPverboseFlag)
{
	/* Must be done after NPDaemon() */ 
	(void)close(0);
	(void)close(2);
	(void)open("/", O_RDONLY);
	(void)dup2(0, 2);
	if (!(NPdebugFlag || NPverboseFlag)) {
		(void)close(1);
		(void)dup2(0, 1);	/* no stdout unless NPdebugFlag */
	}
}


void
Initialize(
	int		  argc,
	char	 *argv[])
{
	int		c;
	int		NPdebug = 0;
	int		NPverbose = 0;
	uint16	msgTypesToInform = DEFAULT_MESSAGE_TYPES;
	char	*invalidSwitch = 0;
	char	*debugOutput = 0;

	sigset( SIGINT, ShutDown );
	sigset( SIGTERM, ShutDown );
	sigset( SIGQUIT, ShutDown );
	sigset( SIGHUP, ShutDown);

	opterr = 0;
	while( ( c = getopt( argc, argv, "d:hivx:" ) ) != EOF ) {
		switch( c ) {
			case 'd':
				setConsoleMessageTypes( optarg, &msgTypesToInform );
				NPdebug = 1;
				break;
			case 'h':
			case 'i':
				printf( argv[0] );
				printf( " [-d main_console_output]" );
				printf( " [-x debug_file] [-h]\n\n" );
				printf( "\tmain_console_output: 'n' = normal," );
				printf( " 'e' = error, 'w' = warning\n" );
				printf( "\t\t'v' = verbose and 'd' = debug" );
				printf( " -- (default: 'new')\n\n" );
				exit( 0 );
				/*NOTREACHED*/
			case 'v':
				msgTypesToInform |= MSG_VERBOSE;	
				NPverbose = 1;
				break;
			case 'x':
				debugOutput = optarg;
				break;
			case '?':
				invalidSwitch = argv[optind - 1];
				break;
		}
	}

	NPDaemon();		/* Daemonize the Process */

	/* Setup I/O and error messages  */
	/*(Should be done after NPDaemon)*/ 
	InitConsole(NPdebug, NPverbose); 


	InitConfig();


	InformInit( msgTypesToInform, debugOutput );
	Inform( (RPrinterInfo_t *) 0, RPMSG_STARTING, MSG_VERBOSE );

	if (invalidSwitch)
		InformWithStr( (RPrinterInfo_t *) 0, RPMSG_INVALID_SWITCH,
			invalidSwitch, MSG_ERROR );


	

}


int
IsTimeForCheckup(void)
{
	time_t currTime;
	static time_t timeForCheckup;

	currTime = time( (long *) 0 );

	if (timeForCheckup <= currTime) {
		timeForCheckup = currTime + StatusCheckInterval();
		return TRUE;
	} else {
		return FALSE;
	}
}


static void
setConsoleMessageTypes(
	char	 types[],
	uint16	*msgTypesToInform)
{
	char *cp;

	*msgTypesToInform = 0;
	if (strcmp( types, "no" ))
		for (cp = types; *cp; cp++)
			switch (*cp) {
				case 'n':
					*msgTypesToInform |= MSG_NORMAL;
					break;
				case 'w':
					*msgTypesToInform |= MSG_WARN;
					break;
				case 'e':
					*msgTypesToInform |= MSG_ERROR;
					break;
				case 'v':
					*msgTypesToInform |= MSG_VERBOSE;
					break;
				case 'd':
					*msgTypesToInform |= MSG_DEBUG;
					break;
				default:
					break;
			}
}
int
IncOpenFiles(void)
{
	return(0);
}
int
DecOpenFiles(void)
{
	return(0);
}
