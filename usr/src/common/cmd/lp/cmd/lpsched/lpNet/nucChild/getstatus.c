/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpNet/nucChild/getstatus.c	1.4"
#ident	"$Header: $"

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "msgs.h"
#include "lp.h"
#include "lpd.h"
#include "pwd.h"
#include "printers.h"

#include <sys/nwportable.h>
#include <nw/nwcalls.h>
#include <cpscommon.h>
#include <cpsservice.h>

#if defined (__STDC__)
static	char	* r_get_status(int, char *);
static	int	  parseJob(char *, char **, int *, char **, char **);
#else
static	char	* r_get_status();
static	int	  parseJob();
#endif


/*
 * Gather job status from NetWare file server
 * (S_GET_STATUS processing)
 */
char *
#if defined (__STDC__)
s_nuc_get_status(char *msg)
#else
s_nuc_get_status(msg)
char	*msg;
#endif
{

	/*
	 * NetWare allows queries based on user in addition to queries 
	 * based on jobid.  Perhaps we can take advantage of this
	 * in the future.
	 */

	FILE		*fp;
	GENERIC_STATUS_T *genericStatus;
	QMS_INFO_T	qmsInfo;
	QMS_SERVICE_T	*qmsHandle;
	QUEUE_STATE_T	queueState;

	mode_t		omask;

	uid_t		uid, saved_uid, tmp_uid;
	uint32		numberOfJobs;
	int 		i, pid;
	int		authentication_attempts = 0;
	NWCCODE		ccode;
	short		status;
	char		*p;
	char		*ClientName;
	char		*filename;
	char		*PrintServer;
	char		*PrintQueue = NULL;
	char		user_name[32] = "";
	char		*tmp_user;
	struct passwd	*pw;
	char		*printer_name;
	PRINTER		*pr = NULL;
    nuint8		binderyAccessLevel;
	nuint32		objectID;

	saved_uid = geteuid();

	(void)getmessage(msg, S_GET_STATUS, &Printer, &filename, &printer_name);
	logit(LOG_DEBUG, "S_GET_STATUS(\"%s\", \"%s\", \"%s\")",
		Printer, filename, printer_name);

	if (PrintServer = strchr(Printer, '!')) {
		*PrintServer = NULL;
		PrintQueue = strdup(Printer);	/* free this one */
		*PrintServer++ = '!';		/* don't free this one */
	}
	else {
		PrintQueue = strdup(Printer);	/* free this one */
		PrintServer = NULL;
	}

	omask = umask(077);
	fp = fopen(filename, "w");		/* put status info in this file */
	(void)umask(omask);

	if (!fp) {
		logit(LOG_WARNING,
		      "s_get_status: can't open \"%s\": %s", filename, PERROR); 
		status = MNOINFO;
		goto out;
	}

	qmsInfo.qmsServer = Rhost;
	qmsInfo.qmsQueue = PrintQueue;
	qmsInfo.pServer = PrintServer;

	if (findAuthenticatedUser(Rhost, &uid) != SUCCESS) {
		if (!(pr = getprinter(printer_name))) {
			status = MNOINFO;
			goto out;
		}
		if (pr->user) {
			logit(LOG_DEBUG, "using %s", pr->user);

			if ((pw = getpwnam(pr->user)) == NULL) {
				logit(LOG_WARNING, "can't find %s in password file",
					pr->user);
				status = MSEELOG;
				goto out;
			}
			uid = pw->pw_uid;
		}
		else {
			logit(LOG_DEBUG, "will try to find workstation owner");

			if (getWorkStationOwner(&uid, &tmp_user) != SUCCESS) {
				logit(LOG_WARNING, "can't determine workstation owner");
				status = MSEELOG;
				goto out;
			}
		}
	}

	logit(LOG_DEBUG, "changing UID to %d", uid);

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

	/*
	 * Information about all jobs in the file server's queue is stored
	 * in the qmsHandle structure.  We could call NWcpsStatusQueueJob
	 * to get even more information about a particular job.
	 */

	while ((ccode = NWcpsGetQueueInfo(qmsHandle, &numberOfJobs, &queueState))
								 != SUCCESS) {
	    switch(ccode) {

		case ERR_NO_Q_RIGHTS:

		    /*
		     *  Distinguish between having no queue rights
		     *  and not being logged in.
		     */

		    if (callAPI(GET_BINDERY_ACCESS_LEVEL, qmsHandle->connID,
										&binderyAccessLevel, &objectID))
		    {
			logit(LOG_WARNING, "NWGetBinderyAccessLevel failed");
			status = MSEELOG;
			goto detach;
		    }
		    if ((int)(binderyAccessLevel & 0x0f)
				>= (int)BS_LOGGED_READ )
		    {
			logit(LOG_DEBUG, "no queue rights");
			status = MNORIGHTS;
			goto detach;
		    }

		    if (++authentication_attempts >
					MAX_AUTHENTICATION_ATTEMPTS) {
			logit(LOG_WARNING, "Login to %s failed", Rhost);
			status = MNOLOGIN;
			goto detach;
		    }

		    if (pr == NULL) {
			if (!(pr = getprinter(printer_name))) {
			    status = MNOINFO;
			    goto detach;
			}
		    }
		    if (pr->user) {
			logit(LOG_DEBUG, "using %s", pr->user);

			if ((pw = getpwnam(pr->user)) == NULL) {
				logit(LOG_WARNING, "can't find %s in password file",
					pr->user);
				status = MSEELOG;
				goto detach;
			}
			tmp_uid = pw->pw_uid;
			strncpy(user_name, pr->user, sizeof(user_name));
		    }
		    else {
			logit(LOG_DEBUG, "will try to find workstation owner");

			if (getWorkStationOwner(&tmp_uid, &tmp_user) != SUCCESS) {
			    logit(LOG_WARNING, "can't determine workstation owner");
			    status = MSEELOG;
			    goto detach;
			}
			strncpy(user_name, tmp_user, sizeof(user_name));
		    }

		    /*
		    **  Too late to change process UID since we're
		    **  already attached to the file server.
		    */
		    if (tmp_uid != uid) {
			status = MNOINFO;
			goto detach;
		    }
			pid = 0;	
		    authenticateToServer(Rhost, user_name, &uid, &pid);
		    break;

	        default:
		    logit(LOG_WARNING,
			"NWcpsGetQueueInfo failed, return value = %x", ccode);
		    status = MSEELOG;
		    goto detach;
	    }
	}

	logit(LOG_DEBUG, "queue state is %d, number of jobs is %d",
		queueState, numberOfJobs);

	/*
	 * The algorithm here is to get a list of remote jobids from the file
	 * server and to query the server for the status of each job.
	 * NWcpsGetQueueInfo allocates space for no more than 250 jobids.
	 * This could be a problem because jobs are assumed to have been
	 * printed and are removed from the local queue if they don't appear
	 * in the status reply from the remote printer.
	 * If the file server has more than 250 jobs in the queue and we
	 * submit job 251 then it will be removed from the local
	 * queue before it is printed on the remote machine.
	 *
	 * Also, it might make more sense to ask form the status of just the
	 * jobs we have spooled to the remote system rather than the status
	 * of all print jobs on the remote system.
	 */

	for (i = 0; i < numberOfJobs; i++) {
	    if (qmsHandle->cachedStatuses[i].entryAvailable) {

		/* spoolFile field contains description */
		if (p = strchr((char *)qmsHandle->cachedStatuses[i].spoolFile,
									'@')) {
		    ClientName = ++p;
		    if (p = strchr(ClientName, ':')) {
				*p = NULL; /* okay to trash buffer */

				/* user:rank:jobid:host */

				(void)fprintf(fp, "%s:%d:%d:%s\n",
					qmsHandle->cachedStatuses[i].userName,
					qmsHandle->cachedStatuses[i].priority,
					qmsHandle->cachedStatuses[i].jobNumber,
					ClientName);

				logit(LOG_DEBUG, "%s:%d:%d:%s",
					qmsHandle->cachedStatuses[i].userName,
					qmsHandle->cachedStatuses[i].priority,
					qmsHandle->cachedStatuses[i].jobNumber,
					ClientName);
		    }
		}
	    }
	}
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
	if (fp)
		fclose(fp);
	msg = r_get_status(status, Printer);
	return(msg);
}

static char *
#if defined (__STDC__)
r_get_status(int status, char * printer)
#else
r_get_status(status, printer)
int	status;
char	*printer;
#endif
{
	logit(LOG_DEBUG, "R_GET_STATUS(%d, \"%s\")", status, printer);
	if (putmessage(Msg, R_GET_STATUS, status, printer) < 0)
		return(NULL);
	else
		return(Msg);
}
