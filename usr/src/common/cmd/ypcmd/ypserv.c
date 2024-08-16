/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:ypserv.c	1.5.5.11"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * This contains the mainline code for the YP server.  Data
 * structures which are process-global are also in this module.  
 */

#include "ypsym.h"
#include <sys/file.h>
#include <sys/signal.h>
#include <rpc/rpc.h>
#include <unistd.h>
#include <pfmt.h>
#include <locale.h>

#include "ypdefs.h"

#define register_failed \
	gettxt(":5", "Unable to register service for %s\n")
static bool silent = TRUE;
static char logfile[] = "/var/yp/ypserv.log";

static void ypexit();
static void ypinit();
static void ypdispatch();
static void ypget_command_line_args();
static void signalHandler();

extern char *gettxt();

void logprintf();

/*
 * External refs to functions named only by the dispatchers.
 */
extern void ypdomain();
extern void ypmatch();
extern void ypfirst();
extern void ypnext();
extern void ypxfr();
extern void ypall();
extern void ypmaster();
extern void yporder();
extern void ypmaplist();

/*
 * Other external refs to functions
 */
extern int select();
extern long fork();
extern void exit();
extern int access();
extern int close();
extern long setpgrp();
extern int gethostname();
extern int gettimeofday();
extern void *sysvconfig();

/*
 * This is the main line code for the yp server.
 */
main(argc, argv)
	int argc;
	char **argv;
{
 
	(void)setlocale(LC_ALL,"");
	(void)setcat("uxypserv");
	(void)setlabel("UX:ypserv");
 

	if (geteuid() != 0) {
		(void) pfmt(stderr, MM_STD, ":6:must be root to run %s\n", argv[0]);
		exit(1);
	}

 	ypinit(argc, argv); 			/* Set up shop */

	svc_run();

}

void
dezombie()
{
	int pid;
	int wait_status;
	
	pid = wait (&wait_status);

        sigset(SIGCHLD,(void (*)())dezombie);
	return;
}

/*
 * Does startup processing for the yp server.
 */
static void
ypinit(argc, argv)
	int argc;
	char **argv;
{
	int pid;
	int t;

	ypget_command_line_args(argc, argv);


	signal (SIGQUIT, signalHandler);
	signal (SIGINT, signalHandler);
	signal (SIGHUP, signalHandler);
	signal (SIGTERM, signalHandler);

	if (silent) {
		
		pid = (int) fork();
		
		if (pid == -1) {
			logprintf(
			     gettxt(":9", "ypinit fork failure.\n"));
			ypexit();
		}
	
		if (pid != 0) {
			exit(0);
		}
	
		if (access(logfile, W_OK) == -1) {
			(void) freopen("/dev/null", "w", stderr);
			(void) freopen("/dev/null", "w", stdout);
		} else {
			(void) freopen(logfile, "a", stderr);
			(void) freopen(logfile, "a", stdout);
		}
		(void) freopen("/dev/null", "r", stdin);

		for (t = 3; t < 20; t++) {
			(void) close(t);
		}
	
		(void) setpgrp();
	}

	sigset(SIGHUP, (void (*)())sysvconfig);
	sigset(SIGCHLD, (void (*)())dezombie);


	/* prepare to answer questions about system v filesystem aliases */
	sysvconfig();
	
	if (!svc_create(ypdispatch, YPPROG, YPVERS, "netpath") ) {
		logprintf(register_failed, "udp");
		ypexit();
	}


}

/*
 * This picks up any command line args passed from the process invocation.
 */
static void
ypget_command_line_args(argc, argv)
	int argc;
	char **argv;
{
	argv++;

	while (--argc) {
		
		if ((*argv)[0] == '-') {

			switch ((*argv)[1]) {
				case 'v': {
					silent = FALSE;
				}
				default: {
					;
				}
			}
				
		}
	}
}

/*
 * This dispatches to server action routines based on the input procedure
 * number.  ypdispatch is called from the RPC function svc_getreq.
 */
static void
ypdispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{


	switch (rqstp->rq_proc) {

	case YPPROC_NULL:

		if (!svc_sendreply(transp, xdr_void, 0) ) {
			logprintf(
			    gettxt(":11", "Can't reply to rpc call.\n"));
		}

		break;

	case YPPROC_DOMAIN:
		ypdomain(transp, TRUE);
		break;

	case YPPROC_DOMAIN_NONACK:
		ypdomain(transp, FALSE);
		break;

	case YPPROC_MATCH:
		ypmatch(rqstp, transp);
		break;

	case YPPROC_FIRST:
		ypfirst(rqstp, transp);
		break;

	case YPPROC_NEXT:
		ypnext(rqstp, transp);
		break;

	case YPPROC_XFR:
		ypxfr(rqstp, transp, YPPROC_XFR);
		break;

	case YPPROC_NEWXFR:
		ypxfr(rqstp, transp, YPPROC_NEWXFR);
		break;

	case YPPROC_CLEAR:
		ypclr_current_map();
		
		if (!svc_sendreply(transp, xdr_void, 0) ) {
			logprintf(
			    ":11:Can't reply to rpc call.\n");
		}

		break;

	case YPPROC_ALL:
		ypall(rqstp, transp);
		break;

	case YPPROC_MASTER:
		ypmaster(transp);
		break;

	case YPPROC_ORDER:
		yporder(transp);
		break;

	case YPPROC_MAPLIST:
		ypmaplist(transp);
		break;

	default:
		svcerr_noproc(transp);
		break;

	}

	return;
}

static void
ypexit()
{
	(void) fflush(stderr);
	exit(1);
}
/*
 * This constructs a logging record.
 */
/*VARARGS*/
void
logprintf(arg1,arg2,arg3,arg4,arg5,arg6,arg7)
char *arg1; 
int arg2, arg3, arg4, arg5, arg6, arg7;
{
	struct timeval t;

	if (silent) {
		(void) gettimeofday(&t, (struct timezone *) NULL);
		fseek(stderr,0,2);
		(void) pfmt(stderr, MM_NOSTD | MM_NOGET, "%19.19s: ",
			ctime(&t.tv_sec));
	}
	(void) pfmt(stderr, MM_NOSTD | MM_NOGET, arg1,arg2,arg3,
						arg4,arg5,arg6,arg7);
	fflush(stderr);
}
/*
 * This does the cleanup and process exit.
 */
static void
signalHandler(sig)
int sig;
{
	logprintf(gettxt(":24", "Received signal %d, exiting!\n"), sig);
	(void) rpcb_unset(YPPROG, YPVERS, (struct netconfig *)NULL);
	exit(0);
}




