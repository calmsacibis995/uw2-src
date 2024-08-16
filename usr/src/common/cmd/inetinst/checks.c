/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:checks.c	1.4"

/*
 *  checks.c 
 *  Routines for performing various checks:
 *	- Is this string a host?
 *	- Is this string a device?
 *	- Is this string a directory?
 *	- Is this a package?
 */

#include "inetinst.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 *  uname struct for getting hostname
 */
static struct utsname	my_uname={
	{""},
	{""},
	{""},
	{""},
	{""},
};				

/*
 *  Routine to handle an interruption (signal) from the user.
 *	INPUT	signal number (we are called by signal handler)
 *	OUTPUT	log fact that we are terminating due to user input.
 *	ACTION	log interruption, close log file, exit(IERR_INTR).
 */
void
clean_exit(int signo)
{
	char logbuf[IBUF_SIZE];

	if (signo == IERR_SUCCESS)
		sprintf(logbuf, catgets(mCat, MSG_SET, C_EXIT_OK, M_EXIT_OK));
	else if ((signo > 0) && (signo < IERR_MIN))
		sprintf(logbuf, catgets(mCat, MSG_SET, C_EXIT_SIG, M_EXIT_SIG),
			signo);
	else {
	switch(signo) {
		case IERR_USAGE:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_USAGE, M_COND_USAGE));
			break;
		case IERR_SYSTEM:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_SYSTEM, M_COND_SYSTEM));
			break;
		case IERR_BADFILE:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_BADFILE, M_COND_BADFILE));
			break;
		case IERR_BADFILE_SERVER:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_BADFILE_SERVER, M_COND_BADFILE_SERVER));
			break;
		case IERR_PERM:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_PERM, M_COND_PERM));
			break;
		case IERR_BADNET:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_BADNET, M_COND_BADNET));
			break;
		case IERR_BADPROTO:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_BADPROTO, M_COND_BADPROTO));
			break;
		case IERR_BADOPT:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_BADOPT, M_COND_BADOPT));
			break;
		case IERR_BADHOST:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_BADHOST, M_COND_BADHOST));
			break;
		case IERR_INTR:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_INTR, M_COND_INTR));
			break;
		case IERR_SOURCE_INVAL:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_SOURCE_INVAL, M_COND_SOURCE_INVAL));
			break;
		case IERR_TARGET_INVAL:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_TARGET_INVAL, M_COND_TARGET_INVAL));
			break;
		case IERR_CLOSED:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_CLOSED, M_COND_CLOSED));
			break;
		case IERR_SUCCESS:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_SUCCESS, M_COND_SUCCESS));
			break;
		default:
			sprintf(logbuf,
			catgets(mCat, MSG_SET, C_COND_UNK, M_COND_UNK));
			break;
	}
	}
	log(logbuf);
	log_close();
	exit(signo);
}

/*
 *  Routine to analyze the results of a system(3) call.  Logs the
 *  result in the logfile.  May be called from any routine using system(3).
 */
int
system_exit(int stat)
{
	char logbuf[IBUF_SIZE];

	if (WEXITSTATUS(stat) == 0)
		return(IERR_SUCCESS);
	else {
		if (WIFSIGNALED(stat) != 0)
			return(WTERMSIG(stat));
		else
			return(IERR_CLOSED);
	}
}

/*
 *  Routine to return the name of the invoking host
 *	INPUT	none
 *	OUTPUT	string pointing to node name
 *	ACTION	if utsname() had never been called, populate my_uname
 *		structure.
 *		Return the nodename part of the structure.
 */
char *
get_nodename()
{
	char logbuf[IBUF_SIZE];
	/*
	 *  If the nodename has never been found, find it,
	 *  otherwise return the found one.
	 */
	if (!strlen(my_uname.nodename)) {
		if (uname(&my_uname) < 0 ) {
			sprintf(logbuf, "%s:%d:%s\n", MCAT, C_ERR_UNAME, M_ERR_UNAME);
			pfmt(stderr, MM_ACTION, logbuf);
			exit(IERR_SYSTEM);
		}
	}

	return(my_uname.nodename);
}

/*
 *  eval_path() evaluates the path given it 
 *	INPUT	pathname and package
 *	OUTPUT	none
 *	ACTION	Returns one of
 *		ISTAT_PKGADD		PKGADD directory format
 *		ISTAT_CAT		PKGADD datastream format
 *		ISTAT_NONE		No pkg associated
 */
int
eval_path(char *path, char *package)
{
	struct stat statbuf;		/* Buffer for stat() */
	FILE	*fptr;			/* Tmp file pointer to check data */
	char	*ptr;			/* Tmp char pointer to check data */
	char tmppath[IBUF_SIZE];	/* Tmp buffer to play with paths */
	char logbuf[IBUF_SIZE];		/* Logging meessage buffer. */

	/*
	 *  Do the stat() so we can tell if the path exists.
	 */
	if (stat(path, &statbuf) < 0) {
		/*
		 *  If the path does not exist, there's a chance that it's
		 *  an entry in the device.tab file.
		 */
		return(ISTAT_NONE);
	}

	/*
	 *  If path is a directory, check to see if the first package
	 *  specified is a directory or a datastream
	 */
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
		if ((ptr = (char *)strchr(package, ' ')) != NULL) {
			*(ptr) = '\0';
		}
		sprintf(tmppath, "%s/%s", path, package);
		if (stat(tmppath, &statbuf) < 0) {
			return(ISTAT_NONE);
		}

		/*
		 *  If path/package is a regular file, check if its
		 *  a package datastream.
		 */
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
			/*
			 *  If the device contains a PaCkAgE DaTaStReAm header,
			 *  it contains a package.
			 */
			if ((fptr = fopen(tmppath, "r")) == NULL) {
				return(ISTAT_NONE);
			}
			fgets(tmppath, IBUF_SIZE, fptr);
			fclose(fptr);

			/*
			 *  Not all files we want to pkgcat are really
			 *  pkg datastreams, so return ISTAT_CAT on all flat
			 *  files.  This way cpio archives et al can be
			 *  pkgcopied across the net.
			 *  LEAVE THIS CHECK IN HERE IN CASE THE CODE NEEDS
			 *  TO BE RESTORED TO CHECKING FOR PKG DATASTREAMS.

			if (!strncmp(tmppath, "# PaCkAgE DaTaStReAm", 20)) {
				strcat(path, "/");
				strcat(path, package);
				*(package)='\0';
				return(ISTAT_CAT);
			}
			*
			*/

			return(ISTAT_CAT);
		}

		return(ISTAT_PKGADD);
	}

	/*
	 *  If path is a char device, see if its a pkg
	 */
	if (((statbuf.st_mode & S_IFMT) == S_IFCHR)||((statbuf.st_mode & S_IFMT) == S_IFREG)) {
		/*
		 *  If the device contains a PaCkAgE DaTaStReAm header,
		 *  it contains a package.
		 */
		if ((fptr = fopen(path, "r")) == NULL) {
			return(ISTAT_NONE);
		}
		fgets(tmppath, IBUF_SIZE, fptr);
		fclose(fptr);
		if (!strncmp(tmppath, "# PaCkAgE DaTaStReAm", 20)) {
			return(ISTAT_CAT);
		}
		return(ISTAT_NONE);
	}
	return(ISTAT_NONE);
}

/*
 *  ishost() determines whether the object passed it is a host
 *	INPUT	string
 *	OUTPUT	none
 *	ACTION	Returns 1 if the object is a host, 
 *		Returns 0 if the object is not a host.
 */
int
ishost(char *object)
{
	if (gethostbyname(object) == (struct hostent *)NULL)
		return(0);
	return(1);
}

/*
 *  Routine to parse a location specification of a source or target.
 *	INPUT	string representing location specification, address of
 *		buffer for holding host and device that are found.
 *	OUTPUT	none
 *	ACTION	parse string, determine semantics of location
 *		specification, and set the passed in addresses of
 *		host and device to their proper values.
 */
int
parse_location(char *location, char **host, char **device)
{
	char	*ptr;			/* Temp pointer for strings */
	char	logbuf[IBUF_SIZE];	/* Buffer for log messages */
	int	pass=0;			/* Can we move on? */

	/*
	 *  See if there's a : seperator in the source
	 */
	if ((ptr = (char *)strchr(location, ':')) != NULL) {
		/*
		 *  If the : is at the beginning, this is a dev
		 */
		if (ptr == location) {
			*host = strdup(get_nodename());
			*device = strdup(strtok(location, ":"));
		/*
		 *  If the : is at the end, this is a host
		 */
		} else if (ptr == (location + strlen(location) - 1)) {
			*host = strdup(strtok(location, ":"));
			*device = strdup(ISPOOL_DIR);
		} else {
		/*
		 *  If the : is in the middle, this is full specification
		 */
			*host = strdup(strtok(location, ":"));
			*device = strdup(strtok(NULL, ":"));
		}
	} else {
		/*
		 *  If there is no : then this is an invalid source
		 *  specification.
		 */
		return(IERR_SOURCE_INVAL);
	}

	return(0);
}

/*
 *  Routine to set up default signal handling.  On any interruption,
 *  we call clean_exit (declared above).
 */
void
signal_setup()
{
	int	my_signals[]={
		SIGHUP, SIGINT, SIGQUIT, SIGILL,
		SIGTRAP, SIGIOT, SIGABRT, SIGEMT, 
		SIGFPE, SIGBUS, SIGSEGV, SIGALRM,
		SIGTERM, SIGPWR, SIGXCPU, SIGXFSZ,
		SIGPIPE,
		0
	};
	int i;
	sigset_t my_sigset;
	struct sigaction my_sigaction;	/* Action to take on signal */

	sigemptyset(&my_sigset);
	for (i=0; my_signals[i] != 0; i++)
		sigaddset(&my_sigset, my_signals[i]);

	/*
	 *  Initialize set of signals to catch.  Since the catcher
	 *  just logs the signal, cleans up the logfile, and exits,
	 *  we want to catch just about every conceivable signal.
	 */
	my_sigaction.sa_handler = clean_exit;
	my_sigaction.sa_mask = my_sigset;

	for (i=0; my_signals[i] != 0; i++)
		sigaction(my_signals[i], &my_sigaction, NULL);
}
