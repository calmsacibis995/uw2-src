/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:do_svc.c	1.3"

/*
 *  This file contains the service routines for software management:
 *	parse_location() - parse a location specification
 *	do_svc_list()    - perform listing of software
 *	do_svc_copy()    - perform data transfer for copy
 *	do_svc_install() - perform data transfer for install
 *	NOT YET do_proxy_svr()   - execute a command by proxy
 *	NOT YET do_proxy_cli()   - request proxy execution of a command
 */

#include "inetinst.h"
#include <string.h>

/*
 *  Routine for doing package list from server
 *	INPUT:	Name of requestor host
 *	OUTPUT: None (except logging events)
 *	ACTION: List available software on this host or
 *	run proxy command (NOT YET)
 */
int
do_svc_list()
{
	FILE	*cmdpipe;		/* pointer for popen() */
	char	*src_host;		/* Host in source specification */
	char	*src_device;		/* Device in source specification */
	char	*package;		/* Package arg (':'-separated) */
	char	*ptr;			/* Temp pointer for strings */
	char	cmdbuf[IBUF_SIZE];	/* Command buffer for system() */
	char	logbuf[IBUF_SIZE];	/* Command buffer for log() */
	int	retval;			/* Return value from pkglist command */

	/*
	 *  First thing to do before sending output is to let the
	 *  client know that something's coming.
	 */
	sprintf(cmdbuf, "%s\n", IMSG_DATA_YES);
	netputs(cmdbuf, stdout);

	/*
	 *  We need first to split apart the source into a
	 *  host and directory.  If the specifcation is invalid,
	 *  discard it.
	 */
	sprintf(cmdbuf, "%s", get_option(IOPT_SOURCE));
	if ((retval=parse_location(cmdbuf, &(src_host), &(src_device))!=0)){
		log(catgets(mCat, MSG_SET, C_ERR_SOURCE, M_ERR_SOURCE));
		clean_exit(IERR_SOURCE_INVAL);
	}

	/*
	 *  Now that we know the source, log that info
	 */
	sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_SOURCE, M_LOG_SOURCE),
		src_host,src_device);
	log(logbuf);

	/*
	 *  Now separate out the :-separated package list into
	 *  a format the local tools understand.
	 */
	package = strdup(get_option(IOPT_PACKAGE));
	while ((ptr = strchr(package, ':')) != NULL) {
		*(ptr) = ' ';
	}

	/*
	 *  Log the command we're about to run
	 */
	sprintf(logbuf,"%s -d %s %s\n", PKGINFO, src_device, package);
	log(logbuf);

	/*
	 *  Run the command
	 */
	sprintf(cmdbuf,"echo go | %s -d %s %s 2>&1",
	  PKGINFO, src_device, package);

	if ((cmdpipe = popen(cmdbuf, "r")) == NULL) {
		log(catgets(mCat, MSG_SET, C_ERR_POPEN, M_ERR_POPEN));
		return(IERR_SYSTEM);
	}
	while(fgets(logbuf, IBUF_SIZE, cmdpipe) != NULL) {
		log(logbuf);
		netputs(logbuf, stdout);
	}
	retval = pclose(cmdpipe);
	retval=system_exit(retval);
	return retval;
}

/*
 *  Routine for doing package copy from server
 *	INPUT:	requestor
 *	OUTPUT: None (except logging events)
 *	ACTION: Parse OPTIONS and perform package copy
 */
int
do_svc_copy()
{
	char	*src_host;		/* Host in source specification */
	char	*src_device;		/* Device in source specification */
	char	*trg_host;		/* Host in source specification */
	char	*trg_device;		/* Device in source specification */
	char	*target;		/* Target specification */
	char	*package;		/* Package arg (':'-separated) */
	char	*ptr;			/* Temp pointer for strings */
	char	*requestor;		/* Who requested this service */
	char	cmdbuf[IBUF_SIZE];	/* Command buffer for system() */
	char	logbuf[IBUF_SIZE];	/* Command buffer for log() */
	int	retval;			/* Return value from pkglist command */

	/*
	 *  We need first to split apart the source and target into a
	 *  host and directory.  If the specifcation is invalid,
	 *  discard it.
	 */
	sprintf(cmdbuf, "%s", get_option(IOPT_SOURCE));
	if ((retval=parse_location(cmdbuf, &(src_host), &(src_device))!=0)){
		log(catgets(mCat, MSG_SET, C_ERR_SOURCE, M_ERR_SOURCE));
		clean_exit(IERR_SOURCE_INVAL);
	}
	sprintf(cmdbuf, "%s", get_option(IOPT_TARGET));
	if ((retval=parse_location(cmdbuf, &(trg_host), &(trg_device))!=0)){
		log(catgets(mCat, MSG_SET, C_ERR_TARGET, M_ERR_TARGET));
		clean_exit(IERR_TARGET_INVAL);
	}

	/*
	 *  Now that we know the source, log that info
	 */
	sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_SOURCE, M_LOG_SOURCE),
		src_host,src_device);
	log(logbuf);

	/*
	 *  Now separate out the :-separated package list into
	 *  a format the local tools understand.
	 */
	package = strdup(get_option(IOPT_PACKAGE));
	while ((ptr = strchr(package, ':')) != NULL) {
		*(ptr) = ' ';
	}

	/*
	 *  If the target is the same as the requestor, we'll do the
	 *  transfer.
	 *  Otherwise, we need to run a proxy to another machine. (NOT YET)
	 */
	if (strcmp(get_option(IOPT_REQUESTOR), trg_host)) {
		/*
		 *  This next code block was added to disable proxy commands.
		 *  to re-enable this feature, remove this block and fix the
		 *  comments in the surrounding code.
		 */
		netputs("Target host did not request service; giving up.\n", stdout);
		log(catgets(mCat, MSG_SET, C_ERR_PROXY, M_ERR_PROXY));
		clean_exit(IERR_BADHOST);
		}
		/*
		 *  End of code block to disable proxy commands
		 */


		/*
		 *  If no package is specified, we can't run a proxy
		 *  command; intercation will be required, and we can't
		 *  provide it remotely.
		 * /
		if (*(package) == ' ') {
			sprintf(logbuf, "PROXY cannot be run, because no package is specified.\n");
			log(logbuf);
			sprintf(cmdbuf, "%s\n", IMSG_DATA_NO);
			netputs(cmdbuf, stdout);
			return(IERR_USAGE);
		}

		sprintf(logbuf, "PROXY <%s -s %s:%s -t %s:%s %s>\n", PKGCOPY,
			src_host, src_device, trg_host, trg_device, package);
		log(logbuf);
		
		/*
		 *  See if we need to run the proxy command verbose.
		 * /
		memset(logbuf, '\0', IBUF_SIZE);
		sprintf(cmdbuf, "%s", get_option(IOPT_VERBOSE));
		if (strncmp(cmdbuf, "0",1)) {
			sprintf(cmdbuf, "%s -v -s %s:%s -t %s:%s %s\n", PKGCOPY,
			  src_host, src_device, trg_host, trg_device, package);
		} else {
			sprintf(cmdbuf, "%s -s %s:%s -t %s:%s %s\n", PKGCOPY,
			  src_host, src_device, trg_host, trg_device, package);
		}

		retval = do_proxy_cli(trg_host, cmdbuf);
		return(retval);
	/* } PROXY - NOT YET */

	/*
	 *  Do the pkginfo for the selected source.
	 *  If we're not local, we need to run another net command.
	 */
	retval = eval_path(src_device, package);
	if (retval == ISTAT_NONE) {
		/*
		 *  First thing to do before sending output is to let the
		 *  client know that nothing's coming.
		 */
		sprintf(cmdbuf, "%s\n", IMSG_DATA_NO);
		netputs(cmdbuf, stdout);
		sprintf(logbuf, catgets(mCat, MSG_SET, C_ERR_NOTFOUND,
		  M_ERR_NOTFOUND), src_device, package);
		log(logbuf);
		return(IERR_BADFILE);
	} else {
		/*
		 *  First thing to do before sending output is to let the
		 *  client know that something's coming.
		 */
		sprintf(cmdbuf, "%s\n", IMSG_DATA_YES);
		netputs(cmdbuf, stdout);
	}
	if (retval == ISTAT_PKGADD) {
		/*
		 *  Log the command we're about to run
		 */
		sprintf(logbuf,"%s -s %s - %s\n", PKGTRANS, src_device, package);
		log(logbuf);

		/*
		 *  Do the pkgtrans, using stdin as the datastream
		 */
		sprintf(cmdbuf,"echo go | %s -s %s - %s", PKGTRANS,  src_device, package);
		retval=system(cmdbuf);
		retval=system_exit(retval);
		return retval;
	}
	if (retval == ISTAT_CAT) {
		/*
		 *  Log the command we're about to run
		 */
		sprintf(logbuf,"%s %s/%s\n", CAT, src_device, package);
		log(logbuf);

		/*
		 *  Do the package transfer, using stdin as the datastream
		 */
		sprintf(cmdbuf,"%s %s/%s", CAT,  src_device, package);
		retval=system(cmdbuf);
		retval=system_exit(retval);
		return retval;
	}
}

/*
 *  Routine for doing package install from server
 *	INPUT:	none
 *	OUTPUT: None (except logging events)
 *	ACTION: Parse OPTIONS and perform package install
 */
int
do_svc_install()
{
	char	*src_host;		/* Host in source specification */
	char	*src_device;		/* Device in source specification */
	char	*trg_host;		/* Host in source specification */
	char	*trg_device;		/* Device in source specification */
	char	*package;		/* Package arg (':'-separated) */
	char	*ptr;			/* Temp pointer for strings */
	char	cmdbuf[IBUF_SIZE];	/* Command buffer for system() */
	char	logbuf[IBUF_SIZE];	/* Command buffer for log() */
	int	retval;			/* Return value from pkglist command */

	/*
	 *  We need first to split apart the source and target into
	 *  host and directory.  If the specifcation is invalid,
	 *  discard it.
	 */
	sprintf(cmdbuf, "%s", get_option(IOPT_SOURCE));
	if ((retval=parse_location(cmdbuf, &(src_host), &(src_device))!=0)){
		log(catgets(mCat, MSG_SET, C_ERR_SOURCE, M_ERR_SOURCE));
		clean_exit(IERR_SOURCE_INVAL);
	}
	sprintf(cmdbuf, "%s", get_option(IOPT_TARGET));
	if ((retval=parse_location(cmdbuf, &(trg_host), &(trg_device))!=0)){
		log(catgets(mCat, MSG_SET, C_ERR_TARGET, M_ERR_TARGET));
		clean_exit(IERR_TARGET_INVAL);
	}

	/*
	 *  Now that we know the source, log that info
	 */
	sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_SOURCE, M_LOG_SOURCE),
		src_host,src_device);
	log(logbuf);
	sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_TARGET, M_LOG_TARGET),
		src_host,src_device);
	log(logbuf);

	/*
	 *  Now separate out the :-separated package list into
	 *  a format the local tools understand.
	 */
	package = strdup(get_option(IOPT_PACKAGE));
	while ((ptr = strchr(package, ':')) != NULL) {
		*(ptr) = ' ';
	}

	/*
	 *  If the target is the same as the requestor, we'll do the
	 *  install.  Otherwise, we need to run a proxy to the target
	 *  machine so it can pull the requested software.
	 * /
	if (strcmp(get_option(IOPT_REQUESTOR), trg_host)) {
		/*
		 *  If no package is specified, we can't run a proxy
		 *  command; intercation will be required, and we can't
		 *  provide it remotely.
		 * /
		if (*(package) == ' ') {
			sprintf(logbuf, "PROXY cannot be run, because no package is specified.\n");
			log(logbuf);
			sprintf(cmdbuf, "%s\n", IMSG_DATA_NO);
			netputs(cmdbuf, stdout);
			return(IERR_USAGE);
		}

		/*
		 *  Target is not the same as requestor.  We need to run
		 *  a proxy command so that the target requests the
		 *  software from us.
		 * /
		sprintf(logbuf, "PROXY <%s -n -s %s:%s -t %s:%s %s>\n",
		  PKGINSTALL, src_host, src_device,
		  trg_host, trg_device, package);
		log(logbuf);
		
		/*
		 *  See if we need to run the proxy command verbose.
		 * /
		sprintf(cmdbuf, "%s", get_option(IOPT_VERBOSE));
		if (strncmp(cmdbuf, "0",1)) {
			sprintf(cmdbuf, "%s -v -n -s %s:%s -t %s:%s %s\n",
			  PKGINSTALL, src_host, src_device,
			  trg_host, trg_device, package);
		} else {
			sprintf(cmdbuf, "%s -n -s %s:%s -t %s:%s %s\n",
			  PKGINSTALL, src_host, src_device,
			  trg_host, trg_device, package);
		}

		/*
		 *  Become the client of the proxy operation, and ask the
		 *  target to request installation from this machine.
		 * /
		retval = do_proxy_cli(trg_host, cmdbuf);
		return(retval);
	} PROXY - NOT YET */

	retval = eval_path(src_device, package);
	if (retval == ISTAT_NONE) {
		/*
		 *  First thing to do before sending output is to let the
		 *  client know that nothing's coming.
		 */
		sprintf(cmdbuf, "%s\n", IMSG_DATA_NO);
		netputs(cmdbuf, stdout);
		sprintf(logbuf, catgets(mCat, MSG_SET, C_ERR_NOTFOUND,
		  M_ERR_NOTFOUND), src_device, package);
		log(logbuf);
		return(IERR_BADFILE);
	} else {
		/*
		 *  First thing to do before sending output is to let the
		 *  client know that something's coming.
		 */
		sprintf(cmdbuf, "%s\n", IMSG_DATA_YES);
		netputs(cmdbuf, stdout);
	}
	if (retval == ISTAT_PKGADD) {
		/*
		 *  Log the command we're about to run
		 */
		sprintf(logbuf,"%s -s %s - %s\n", PKGTRANS,
		  src_device, package);
		log(logbuf);

		/*
		 *  Do the pkgtrans, using stdin as the datastream
		 */
		sprintf(cmdbuf,"echo go | %s -s %s - %s", PKGTRANS,
		  src_device, package);
		retval=system(cmdbuf);
		retval=system_exit(retval);
		return retval;
	}
	if (retval == ISTAT_CAT) {
		/*
		 *  Log the command we're about to run
		 */
		sprintf(logbuf,"%s %s\n", CAT, src_device);
		log(logbuf);

		/*
		 *  Do the package transfer, using stdin as the datastream
		 */
		sprintf(cmdbuf,"%s %s", CAT,  src_device);
		retval=system(cmdbuf);
		retval=system_exit(retval);
		return retval;
	}
}

/*
 *  Routine for executing proxy command from server (proxy started
 *  by in.inetinst)
 *	INPUT:	none
 *	OUTPUT: None (except logging events)
 *	ACTION: Get proxy command from client and execute it.
 * /
int
do_proxy_svr()
{
	FILE	*cmdpipe;		/* Pipe FILE ptr for proxy */
	char	netbuf[IBUF_SIZE];	/* Buffer for network I/O */
	char	cmdbuf[IBUF_SIZE];	/* Command buffer for system() */
	char	logbuf[IBUF_SIZE];	/* Command buffer for log() */
	int	retval;			/* Return value from pkglist command */

	/*
	 *  Get the command we are supposed to run from the net connection.
	 * /
	if (fgets(netbuf, IBUF_SIZE, stdin) == (char *)EOF) {
		log("bad read on net connection\n");
		clean_exit(IERR_BADNET);
	}
	sprintf(logbuf, "PROXY <%s>\n", netbuf);
	log(logbuf);
	sprintf(logbuf, "Got your PROXY command\n");
	netputs(logbuf, stdout);

	/*
	 *  Execute proxy command
	 * /
	if ((cmdpipe = popen(netbuf, "r")) == NULL) {
		log("Could not execute popen()\n");
		return(IERR_SYSTEM);
	}
	while(fgets(logbuf, IBUF_SIZE, cmdpipe) != NULL) {
		log(logbuf);
		netputs(logbuf, stdout);
	}
	retval = pclose(cmdpipe);
	if (retval == 0) retval=IERR_SUCCESS;
	return(retval);
} PROXY - NOT YET */

/*
 *  Routine for executing proxy command from client (proxy started
 *  by one of the do_svc routines)
 *	INPUT:	host, command
 *	OUTPUT: None (except logging events)
 *	ACTION: We were the server - we now become the client.
 *		Open up connection to next server, negotiate and send
 *		the proxy command.  All output we get goes to stdout,
 *		which is sent to the original client.
 * /
int
do_proxy_cli(char *trg_host, char *command)
{
	FILE	*cmdpipe;		/* Pipe FILE ptr for proxy */
	char	netbuf[IBUF_SIZE];	/* Buffer for network I/O */
	char	cmdbuf[IBUF_SIZE];	/* Command buffer for system() */
	char	logbuf[IBUF_SIZE];	/* Command buffer for log() */
	int	orig_stdin_fd;		/* Stdin when we were server */
	int	orig_stdout_fd;		/* Stdout when we were server */
	int	retval;			/* Return value from pkglist command */
	extern int errno;

	/*
	 *  dup the original stdin and stdout so we can get the
	 *  output from the proxy command back to them that needs it
	 * /
	if ((orig_stdin_fd = dup(0)) < 0) {
		sprintf(logbuf, "Couldn't dup stdin; errno %d\n", errno);
		log(logbuf);
	}
	if ((orig_stdout_fd = dup(1)) < 0) {
		sprintf(logbuf, "Couldn't dup stdout; errno %d\n", errno);
		log(logbuf);
	}

	/*
	 *  Open up the TLI connection
	 * /
	inetinst_connect(trg_host);

	/*
	 *  Initialize session with HELO; get response
	 * /
	sprintf(netbuf, "HELO %s\n", get_nodename());
	netsendrcv(netbuf, 1);

	/*
	 *  Request SERVICE; get response
	 * /
	sprintf(netbuf, "SERVICE proxy\n");
	netsendrcv(netbuf, 1);

	/*
	 *  Now send command; get response.  When we send the command
	 *  to be run by proxy, stderr needs to be sent back via the same
	 *  channel as stout so the management concole gets all output.
	 * /
	sprintf(netbuf, "%s 2>&1\n", command);
	netsendrcv(netbuf, 1);

	/*
	 *  Get output from proxy command
	 * /
	while(fgets(logbuf, IBUF_SIZE, stdin) != NULL) {
		log(logbuf);
		write(orig_stdout_fd, logbuf, strlen(logbuf));
	}

	return(0);
} PROXY - NOT YET */
