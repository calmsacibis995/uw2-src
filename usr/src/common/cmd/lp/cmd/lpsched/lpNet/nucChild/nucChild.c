/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpNet/nucChild/nucChild.c	1.3.1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/lp/cmd/lpsched/lpNet/nucChild/nucChild.c,v 1.4 1994/03/26 01:51:54 aclark Exp $"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "lp.h"
#include "lpNet.h"
#include "lpd.h"
/* #include "oam_def.h" */
#include "oam.h"
#include "errorMgmt.h"
#include "debug.h"

char	 	 Buf[BUFSIZ];	/* general use buffer */
char		*Netbuf;	/* buffer for reading network messages */

extern MESG	*lp_Md;

#if defined (__STDC__)
static	int	getUsrReqs(char *);
static	void	Execlisten(void);
static	void	Netlisten(void);
static	void	lpdNetExit(void);
static	void	lpdNetInit(void);
static	void	s_send_job(char *);
#else
static	int	getUsrReqs();
static	void	Execlisten();
static	void	Netlisten();
static	void	lpdNetExit();
static	void	lpdNetInit();
static	void	s_send_job();
#endif

/*
 * Main entry point
 */
void
#if defined (__STDC__)
nucChild(void)
#else
nucChild()
#endif
{
		char	msgbuf [MSGMAX];
		short	status;
	static	char	FnName [] = "nucChild";

	ENTRYP
	lpdNetInit();

	if (ProcessInfo.processRank == MasterChild)
	{
		if (mread (ProcessInfo.lpExecMsg_p,
		    msgbuf, sizeof (msgbuf)) == -1)
			TrapError (Fatal, Unix, FnName, "mread");

		if (mtype (msgbuf) != S_CHILD_SYNC)
			TrapError (Fatal, Internal, FnName, 
			"Bad message from lpExec.  Expected S_CHILD_SYNC.");

		if (getmessage (msgbuf, S_CHILD_SYNC, &status) == -1)
			TrapError (Fatal, Unix, FnName, "getmessage");

		if (status != MOK)
		{
			fatal ("Child services aborted.");
		}
	}

	Execlisten();

	lpdNetExit();
	/*NOTREACHED*/
}

static void
#if defined (__STDC__)
lpdNetInit(void)
#else
lpdNetInit()
#endif
{
	struct utsname	utsname;

	DEFINE_FNNAME ("lpdNetInit")
	ENTRYP
	Name = LPDNET;
	logit(LOG_INFO, "%s starting (%s)", Name, CONNECTED_TO_REMOTE ? 
								"passive" : 
								"active"  );
	(void)umask(0);
	Lhost = ProcessInfo.systemName_p;
	Rhost = SIP->systemName_p;
	lp_Md = ProcessInfo.lpExecMsg_p;
}

static void
#if defined (__STDC__)
lpdNetExit(void)
#else
lpdNetExit()
#endif
{
	DEFINE_FNNAME ("lpdNetExit")
	ENTRYP
	nucdone(0);
	/*NOTREACHED*/
}

/*
 * Process lpExec requests.
 */
static void
#if defined (__STDC__)
Execlisten(void)
#else
Execlisten()
#endif
{
	struct pollfd 	 pollfd;
	int		 timeout;
	int		 n;

	DEFINE_FNNAME ("Netlisten")
	ENTRYP
	TRACEd (ProcessInfo.lpExec)
	pollfd.fd = ProcessInfo.lpExec;
	pollfd.events = POLLIN;

	/*
	**	A timeout of zero is actually
	**	too quick; so we will protect users
	**	and set a zero to one
	*/
	if (SIP->timeout == 0)
		SIP->timeout = 1;

	/*
	**	The first time through the loop we will set timeout
	**	to -1.  This is not a problem since there must be some
	**	event there for us to process.
	*/

	timeout = -1;

	for(;;) {
		if ((n = poll(&pollfd, 1, timeout)) == -1)
			continue;
		if (!n) {
			logit(LOG_INFO, "%s timing-out", Name);
			EXITP
			return;			/* timed-out */
		}

		timeout = (SIP->timeout * 60000);

		switch (pollfd.revents) {
		case 0:		/* shouldn't happen */
			break;

		case POLLIN:
			if ((n = mread(lp_Md, Msg, MSGMAX)) <= 0) {
				logit(LOG_WARNING, "mread() returned %d", n);
				break;
			}
			switch (n = getmessage(Msg, I_GET_TYPE)) {
			case S_SHUTDOWN:
				logit(LOG_INFO, "%s shutting down", Name);
				(void)mdisconnect(lp_Md);
				lp_Md = NULL;
				EXITP
				return;

			case S_SEND_JOB:
				s_send_job(Msg);
				break;

			default:	/* should we just return? */
				logit(LOG_ERR, "unexpected message: %d", n);
				break;
			}
			break;

		default:		/* something amiss */
			logit(LOG_INFO, "%s received%s%s on lpexec pipe", Name,
				pollfd.revents & POLLHUP ? " POLLHUP" : "",
				pollfd.revents & POLLERR ? " POLLERR" : "");
			EXITP
			return;
		}
	}
}

/*
 * Process S_SEND_JOB request.
 * All service routines will return NULL in the event of an error;
 * otherwise, they return a response message.
 */
static void
#if defined (__STDC__)
s_send_job(char *msg)
#else
s_send_job(msg)
char	*msg;
#endif
{
	char		*sysname, *rqfname, *ret_msg;
	short		 ftype, msgsize;
	short		 status;
	int		 type;
 	int		 i;

	if ((type = getmessage(msg, S_SEND_JOB, &sysname,
				 		&ftype,
						&rqfname,
						&msgsize,
						&msg)) >= 0) {
		logit(LOG_DEBUG, "S_SEND_JOB(ftype=%d, request=\"%s\")", 
								ftype, rqfname);
		if (!STREQU(Rhost, sysname)) {
			logit(LOG_ERR, "s_send_job: wrong system: %s", sysname);
			nucdone(2);
			/*NOTREACHED*/
		}
		if (contimeout()) {
			msg = NULL;
			goto out;
		}
		switch (type = getmessage(msg, I_GET_TYPE)) {

		case S_PRINT_REQUEST:
 			i = 0;
 			alarm(0);  /* turn off timer for print requests? */
 			while (!(ret_msg = s_nuc_print_request(msg))) {
 				if (SIP->retry < 0)
 					break;
 				closeRemote();
 				if (++i == 2)
 					logit(LOG_INFO,
				"waiting for remote queue to be enabled");
 				if (SIP->retry > 0)
 					(void)sleep(SIP->retry*60);
 				if (!openRemote())
 					break;
 			}
			msg = ret_msg;
			break;

		case S_CANCEL:
			msg = s_nuc_cancel(msg);
			break;

		case S_GET_STATUS:
			msg = s_nuc_get_status(msg);
			break;

		default:
			logit(LOG_ERR, "s_send_job: bad type = %d", type);
			nucdone(2);
			/*NOTREACHED*/
		}
		alarm(0);
	} else {
		if (type < 0)
			logit(LOG_ERR, "badly formed S_SEND_JOB message");
		msg = NULL;
	}

out:
	r_send_job(msg ? MOK : MTRANSMITERR, msg);
	closeRemote();
}

/*
 * Exit routine
 */
void
#if defined (__STDC__)
nucdone(int rc)
#else
nucdone(rc)
int	rc;
#endif
{
	if (lp_Md) {	
		if (ProcessInfo.processRank == MasterChild)
			(void)mputm(lp_Md, S_SHUTDOWN, MOK);
		(void)mdisconnect(lp_Md);
	}
	DisconnectSystem(CIP);
	FreeConnectionInfo(&CIP);
	(void)fclose(stderr);		/* may not actually be open */
	logit(LOG_INFO, "%s exiting, status=%d", Name, rc);
	exit(rc);
	/*NOTREACHED*/
}
