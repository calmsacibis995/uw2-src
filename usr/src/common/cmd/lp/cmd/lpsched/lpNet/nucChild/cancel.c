/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpNet/nucChild/cancel.c	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/lp/cmd/lpsched/lpNet/nucChild/cancel.c,v 1.2 1994/03/26 01:51:50 aclark Exp $"

#include <stdio.h>
#include <string.h>
#include "msgs.h"
#include "lp.h"
#include "lpd.h"
#include <pwd.h>
#include "printers.h"

#include <sys/nwportable.h>
#include <nw/nwcalls.h>
#include <cpscommon.h>

#if defined (__STDC__)
static	char	* r_cancel(short, short, char *);
#else
static	char	* r_cancel();
#endif


/*
 * Cancel job previously submitted to NetWare file server
 * (S_CANCEL processing)
 */
char *
#if defined (__STDC__)
s_nuc_cancel(char *msg)
#else
s_nuc_cancel(msg)
char	*msg;
#endif
{
	QMS_INFO_T	qmsInfo;
	opaque_t	*qmsHandle;
	int		authentication_attempts = 0;
	NWCCODE		ccode;
	short		status;
	char		*dest;
	char		*user;
	char		*reqid;
	char		*PrintServer;
	char		*PrintQueue = NULL;
	char		*tmp_user;
	struct passwd	*pw;
	uid_t		uid, saved_uid;
	int		pid;
	char		*printer_name;
	PRINTER		*pr = NULL;

	saved_uid = geteuid();

	/* destination field should never be empty */
	(void)getmessage(msg, S_CANCEL, &dest, &user, &reqid, &printer_name);

	logit(LOG_DEBUG, "S_CANCEL(\"%s\", \"%s\", \"%s\", \"%s\")",
		dest, user, reqid, printer_name);

	if (reqid == NULL) {
		status = MNOINFO;
		goto out;
	}
	if (PrintServer = strchr(dest, '!')) {
		*PrintServer = NULL;
		PrintQueue = strdup(dest);	/* free this one */
		*PrintServer++ = '!';		/* don't free this one */
	}
	else {
		PrintQueue = strdup(dest);	/* free this one */
		PrintServer = NULL;
	}

	qmsInfo.qmsServer = Rhost;
	qmsInfo.qmsQueue = PrintQueue;
	qmsInfo.pServer = PrintServer;

	if (!(pr = getprinter(printer_name))) {
		status = MNOINFO;
		goto out;
	}
	if (pr->user)
		user = pr->user;
	else
	/*
	**  Set effective UID to match real UID of user that sent print job.
	**  Users can only cancel their own print jobs.
	*/
	if (tmp_user = strchr(user, '!'))
		user = tmp_user + 1;

	if ((pw = getpwnam(user)) == NULL) {
		logit(LOG_WARNING, "can't find %s in password file", user);
		status = MSEELOG;
		goto out;
	}
	uid = pw->pw_uid;
	logit(LOG_DEBUG, "changing effective UID to %d", uid);
	if (seteuid(uid) == -1) {
		logit(LOG_WARNING, "can't change effective UID to %d", uid);
		status = MSEELOG;
		goto out;
	}
	if (NWcpsAttachQMS(&qmsInfo, &qmsHandle) != SUCCESS) {
		logit(LOG_WARNING, "can't attach to file server \"%s\"", Rhost);
		status = MNOATTACH;
		goto out;
	}

	logit(LOG_DEBUG, "%s attached to \"%s\" queue on \"%s\" for print server \"%s\"",
		Name, PrintQueue, Rhost, PrintServer ? PrintServer : "ANY");

	while ((ccode = NWcpsDeleteJob(qmsHandle, atoi(reqid))) != 0) {

		/* set status even though lpsched ignores it */

		switch (ccode) {

		    case ERR_NO_Q_JOB:	/* no queue job */

			logit(LOG_DEBUG, "no such job as \"%s\"", reqid);
			status = M2LATE;
			goto detach;

		    case ERR_NO_Q_JOB_RIGHTS:	/* no right to delete job */

			logit(LOG_DEBUG, "can't delete \"%s\"", reqid);
			status = MNORIGHTS;
			goto detach;

		    case ERR_NO_Q_RIGHTS: /* not authenticated to server */

			if (++authentication_attempts >
			    MAX_AUTHENTICATION_ATTEMPTS) {
				logit(LOG_WARNING, "Login to %s failed", Rhost);
				status = MNOLOGIN;
				goto detach;
			}
	
			pid = 0;
			authenticateToServer(Rhost, user, &uid, &pid);
			break;

		    default:
			logit(LOG_WARNING, "NWcpsDeleteJob failed, return value = %x",
			    ccode);
			status = MSEELOG;
			goto detach;
		}
	}
	logit(LOG_DEBUG, "job %s deleted from queue \"%s\" on \"%s\"",
		reqid, PrintQueue, Rhost);
	status = MOK;

detach:
	NWcpsDetachQMS(qmsHandle);
	logit(LOG_DEBUG, "%s detached from queue \"%s\" on \"%s\"",
		Name, PrintQueue, Rhost);
out:
	(void) seteuid(saved_uid);
	if (pr)
		freeprinter(pr);
	if (PrintQueue)
		free(PrintQueue);
	return(r_cancel(MOK, status, reqid));
}

static char *
#if defined (__STDC__)
r_cancel(short status1, short status2, char *reqid)
#else
r_cancel(status1, status2, reqid)
short	 status1;
short	 status2;
char	*reqid;
#endif
{
	logit(LOG_DEBUG, "R_CANCEL(%d, %d, \"%s\")", status1, status2, NB(reqid));
	if (putmessage(Msg, R_CANCEL, status1, status2, reqid) < 0)
		return(NULL);
	else
		return(Msg);
}
