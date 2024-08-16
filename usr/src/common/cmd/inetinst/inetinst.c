/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:inetinst.c	1.6"


/*
 *  in.inetinst
 *
 *  Daemon process, started from inetd, that manages software
 *  installation and distribution.
 *  Management console connects via TLI to in.inetinst on server
 *  who in turn connects to client.
 *  This daemon runs on both client and server.
 *  On Server:
 *	Takes direction from Management Console,
 *	Gives direction to Client.
 *	Transfers data to Client.
 *	Retrieves log info from Client.
 *  On Client:
 *	Takes direction from Server.
 *	Performs requested activity.
 *	Transmits log info to Server.
 */
#include "inetinst.h"
/* xyzzy */
#include <pwd.h>
#include <grp.h>
#include <priv.h>

/*
 *  The following #include statements may be omitted in the future;
 *  they are solely for the use of the t_sync() and ioctl()s in main().
 */
#include <nl_types.h>
#include <tiuser.h>
#include <stropts.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#include <netconfig.h>
#include <netdir.h>


static char keyword[IBUF_SIZE];	/* for scanning keywords from chat */
static char keyargs[IBUF_SIZE];	/* for scanning keyword args from chat */

int	do_svc_install();
int	do_svc_list();
int	do_svc_copy();
/* int	do_proxy_svr(); PROXY - NOT YET */

main(int argc, char *argv[])
{
	char	sndbuf[IBUF_SIZE];
	char	rcvbuf[IBUF_SIZE];
	char	logbuf[IBUF_SIZE];
	char	*requestor;		/* Machine that requested our svc */
	char	*service;		/* Service requested */
	int	(*do_svc)();		/* Function call for req'd svc */
	int	retval;			/* Return value for commands we run */
	int	pass=0;			/* Can we pass to next state? */
	struct	passwd	mypwent;	/* For storing passwd entries */
	struct	passwd	*mypwentp;	/* For storing passwd entries */
	struct	group	mygrent;	/* For storing group entries */
	struct	group	*mygrentp;	/* For storing group entries */

	/*
	 *  Set up for i18n
	 */
	setlocale(LC_ALL, "");
	mCat = catopen(MCAT, 0);
	setcat("inetinst.cat.m");
	setlabel("UX:inetinst");

	/*
	 *  As a security feature, make sure that the real and effective
	 *  UID and GID of this process are 'nobody', and that priveleges
	 *  are all removed.
	 */
	if ((mypwentp = getpwnam(NOBODY)) == (struct passwd *)NULL) {
		clean_exit(IERR_PERM);
	}
	memcpy(&mypwent, mypwentp, sizeof(struct passwd));
	retval = setuid(mypwent.pw_uid);
	if (retval == -1) {
		clean_exit(IERR_PERM);
	}

	if ((mygrentp = getgrnam(NOBODY)) == (struct group *)NULL) {
		clean_exit(IERR_PERM);
	}
	memcpy(&mygrent, mygrentp, sizeof(struct group));
	retval = setgid(mygrent.gr_gid);
	if (retval == -1) {
		clean_exit(IERR_PERM);
	}

	retval = procpriv(CLRPRV, (priv_t *)NULL, 0);
	if ((retval == -1) || (retval > 0)) {
		clean_exit(IERR_PERM);
	}

	/*
	 *  The following three lines of code may be deleted some time
	 *  in the future.  Due to a mistreatment by inetd and nwnetd of
	 *  TLI processes they spawn, servers spawned by these processes
	 *  must be aware that they are network programs.
	 */
	t_sync(0);
	ioctl(0, I_POP, "timod");
	ioctl(0, I_PUSH, "tirdwr");

	/*
	 *  Set up signal handling so that log will be dealt with
	 *  correctly on interruption.
	 */
	signal_setup();

	/*
	 *  Scan first line of chat.  Should be SMTP-style
	 *  "HELO requestor".  If not, fail with bad protocol.
	 */
	while(!pass) {
		if (fgets(rcvbuf, IBUF_SIZE, stdin) == (char *)EOF) {
			clean_exit(IERR_BADNET);
		}
		if (sscanf(rcvbuf, "%s %s", keyword, keyargs) != 2) {
			sprintf(sndbuf, catgets(mCat, MSG_SET, C_BADKEY_1, M_BADKEY_1), keyword, "HELO");
			netputs(sndbuf, stdout);
			continue;
		}
		if (strcmp(keyword, "HELO")) {
			sprintf(sndbuf, catgets(mCat, MSG_SET, C_BADKEY_1, M_BADKEY_1), keyword, "HELO");
			netputs(sndbuf, stdout);
			continue;
		} else {
			/*
			 *  Got what we need - we can go on now
			 */
			pass = 1;
			requestor = strdup(keyargs);
			sprintf(sndbuf, catgets(mCat, MSG_SET, C_HELLO, M_HELLO), requestor);
			netputs(sndbuf, stdout);
		}
	}
	/*
	 *  Get the default options taken care of
	 */
	set_default_options(requestor);
	print_all_options();

	pass = 0;

	/*
	 *  Scan second line of chat.  Should be SMTP-style
	 *  "SERVICE servicename".  If not, fail with bad protocol.
	 */
	while(!pass) {
		if (fgets(rcvbuf, IBUF_SIZE, stdin) == (char *)EOF) {
			log(catgets(mCat, MSG_SET, C_COND_BADPROTO, M_COND_BADPROTO));
			clean_exit(IERR_BADNET);
		}

		if (sscanf(rcvbuf, "%s %s", keyword, keyargs) != 2) {
			sprintf(sndbuf, catgets(mCat, MSG_SET, C_BADKEY_1, M_BADKEY_1), keyword, "SERVICE");
			netputs(sndbuf, stdout);
			continue;
		}

		if (strcmp(keyword, "SERVICE")) {
			sprintf(sndbuf, catgets(mCat, MSG_SET, C_BADKEY_1, M_BADKEY_1), keyword, "SERVICE");
			netputs(sndbuf, stdout);
			continue;
		} 
		/*
		 *  Make sure we support this service
		 */
		service = strdup(keyargs);
		if (!strcmp(service, ISVC_COPY) || !strcmp(service, ISVC_CAT))
			do_svc = do_svc_copy;
		else if (!strcmp(service, ISVC_LIST))
			do_svc = do_svc_list;
		else if (!strcmp(service, ISVC_INSTALL))
			do_svc = do_svc_install;
		/*
		else if (!strcmp(service, ISVC_PROXY))
			do_svc = do_proxy_svr;
		PROXY - NOT YET */
		else {
			sprintf(sndbuf, catgets(mCat, MSG_SET, C_BADKEY_SVC, M_BADKEY_SVC), service);
			netputs(sndbuf, stdout);
			continue;
		} 
		pass = 1;

		sprintf(sndbuf, catgets(mCat, MSG_SET, C_READY, M_READY), service);
		netputs(sndbuf, stdout);
		if (strcmp(service, ISVC_PROXY)) {
			get_options();
			print_all_options();
		}

		/*
		 *  Initialize logging for SERVER role and don't
		 *  forget to log who's in the CLIENT role.
		 */
		log_init(service);
		sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_REQUEST, M_LOG_REQUEST), requestor, service);
		log(logbuf);

		/*
		 *  Go perform the requested service
		 */
		retval = do_svc();

		/*
		 *  Close the log and clean up; it's time to go.
		 */
		sleep(2);
		clean_exit(retval);
	}
}

/*
 *  Routine to parse all of the TARGET options until we get a
 *  DATA or OPTIONS keyword.
 */
void
get_options()
{
	char	netbuf[IBUF_SIZE];
	char	tag[IBUF_SIZE];
	char	value[IBUF_SIZE];
	int	numscanned;
	int	pass=0;			/* Can we pass to next state? */

	/*
	 *  Scan next line of chat.  Should be OPTIONS or DATA.
	 *  If not, fail with bad protocol.
	 */
	while(!pass) {
		if (fgets(netbuf, IBUF_SIZE, stdin) == (char *)EOF) {
			log(catgets(mCat, MSG_SET, C_COND_BADNET, M_COND_BADNET));
			clean_exit(IERR_BADNET);
		}

		numscanned = sscanf(netbuf, "%s %s", keyword, keyargs);
		if (numscanned == 1) {
			/*
			 *  If this was a request for DATA, we need to go
			 *  back and send data.
			 */
			if (!strcmp(keyword, "DATA")) {
				return;
			}

			if (strcmp(keyword, "OPTIONS")) {
				sprintf(netbuf, catgets(mCat, MSG_SET, C_BADKEY_2, M_BADKEY_2), keyword, "SERVICE", "DATA");
				netputs(netbuf, stdout);
				continue;
			}
			pass = 1;
		} else {
			sprintf(netbuf, catgets(mCat, MSG_SET, C_BADKEY_2, M_BADKEY_2), keyword, "SERVICE", "DATA");
			netputs(netbuf, stdout);
			continue;
		}
	}

	/*
	 *  Now we can get down to the business of parsing
	 *  options.
	 *  Scan next line of chat.  Should be just options
	 *  unless it's the DATA header.
	 */
	pass = 0;
	sprintf(netbuf, catgets(mCat, MSG_SET, C_STARTOPT, M_STARTOPT), "DATA");
	netputs(netbuf, stdout);
	while (!pass) {
		if (fgets(netbuf, IBUF_SIZE, stdin) == (char *)EOF) {
			log(catgets(mCat, MSG_SET, C_COND_BADNET, M_COND_BADNET));
			clean_exit(IERR_BADNET);
		}

		/*
		 *  If we scan in < 2 args, then check to see if we're moving
		 *  on to the data section.  If not, it's an error.
		 */
		numscanned = sscanf(netbuf, "%s %s", keyword, keyargs);
		if (numscanned < 2) {
			if (!strcmp(keyword, "DATA")) {
				return;
			}
			sprintf(netbuf, catgets(mCat, MSG_SET, C_BADKEY_OPT, M_BADKEY_OPT), keyword);
			netputs(netbuf, stdout);
			continue;
		}
		/*
		 *  If we scanned in 2 arguments, then we need to assign them
		 *  to the appropriate location.
		 */
		if (set_option(keyword, keyargs) == 0) {
			sprintf(netbuf, "OK\n");
			netputs(netbuf, stdout);
		}
		continue;
	}
}
