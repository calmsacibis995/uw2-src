/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/netselect/netselect.c	1.9.13.11"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*       PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*       Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*       (c) 1986,1987,1988.1989  Sun Microsystems, Inc
*       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*       (c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/

#include <sys/types.h>

#define fopen _fopen	/* Create a prototype for _fopen in <stdio.h> */
#include <stdio.h>
#undef fopen 

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <netconfig.h>
#include "netcspace.h"
#include "netsel_mt.h"

#define FAILURE  (unsigned)(-1L)

/*
 *	Local routines used by the library procedures
 */

static int blank();
static int comment();
static struct netconfig *fgetnetconfig();
static void free_netcf();
static unsigned long getflag();
static char **getlookups();
static struct netconfig **getnetlist();
static unsigned long getnlookups();
static char *gettoken();
static unsigned long getvalue();
static void shift1left();

/*
 *	Access routines used by library procedures
 *	to get per-thread shared data
 */

static int *get_nc_error();
static int *get_linenum();
static int *get_fieldnum();
static char *get_retstr();
static char **get_savep();

/*
 *	Static external variables used by the library procedures:
 *	
 *	netpp - points to the beginning of the list of netconfig
 *		pointers used by setnetconfig() and setnetpath().
 *
 *	finalpp - points to the last member of the list of netconfig
 *		pointers used by setnetconfig().
 *
 *	num_calls - number of times setnetpath() and setnetconfig()
 *		    are called
 *
 *	linenum - the current line number of the /etc/netconfig
 *		  file (used for debugging and for nc_perror()).
 *
 *	fieldnum - the current field number of the current line
 *	 	   of /etc/netconfig (used for debugging and for
 *		   nc_perror()).
 *
 *	nc_error - the error condition encountered.
 */

static struct netconfig **netpp = NULL; /* protected by _netselect_list_lock */
static struct netconfig **finalpp;	/* protected by _netselect_list_lock */
static int num_calls = 0;	     /* protected by _netselect_counter_lock */

/*
 *	setnetconfig() has the effect of "initializing" the
 *	network configuration database.   It reads in the
 *	netconfig entries (if not already read in) and returns
 *	a handle to the first entry (to be used in subsequent
 *	calls to getnetconfig()).
 */

void *
setnetconfig()
{
	NCONF_HANDLE *retp;
	struct netconfig **netpp0;	/* local copy of netpp */
	int *nc_errorp;

	/* Allocate handle for netconfig list */
	if ((retp = (NCONF_HANDLE *)malloc(sizeof(NCONF_HANDLE))) == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return(NULL);
	}

	/*
	 * Both the counter and the list must be locked during initialization.
	 * Otherwise, there could be a memory leak.  
	 * The following example will make the need for two locks clearer.
	 * Two threads start; both increment the counter; the first one 
	 * locks the list but fails to initialize it; the second one initializes
	 * the list, gets a handle, finishes, calls endnetconfig(), which
	 * decrements the counter to 1 but doesn't free the list; the other
	 * thread, by chance, only now gets to decrement the counter to 0--too
	 * late to trigger the freeing of the list.
	 * The solution is to prevent the second thread from incrementing the
	 * counter until the first thread has either successfully initialized
	 * the list or decremented the counter.
	 * Two locks are necessary, since other functions just need to
	 * read netpp and should not be blocked unless netpp is being set.
	 */

	/*
	 * To avoid deadlock when two locks are to be held at the same time,
	 * the locks must always be acquired in the same order:
	 *
	 * set _netselect_counter_lock first.
	 * set _netselect_list_lock second.
	 */

	/*
	 * Get the counter lock and keep it until the final value
	 * of num_calls has been set.
	 */
	MUTEX_LOCK(&_netselect_counter_lock);

	/* Get current head of netconfig list */
	netpp0 = netpp;
	/* Is netconfig list empty? */
	if(netpp0 == NULL) {
		RW_WRLOCK(&_netselect_list_lock);
		/* Is netconfig list still empty? 
		 * (Another thread may have gotten here first.) */
		if(netpp == NULL) {
		/* Read netconfig entries into memory */
			netpp = getnetlist();
		}
		netpp0 = netpp;
		RW_UNLOCK(&_netselect_list_lock);
		if (netpp0 == NULL) {
			free(retp);
			/* Release the counter lock last, so no deadlocking. */
			MUTEX_UNLOCK(&_netselect_counter_lock);
			return(NULL);
		}
	}

	/* Now we know for sure that the netconfig list is not empty
	 * and that we have a handle. */

	/* Increment the counter of active calls and release the lock */
	num_calls ++;
	MUTEX_UNLOCK(&_netselect_counter_lock);

	/* Initialize handle to point to the list. */
	retp->nc_head = retp->nc_curr = netpp0;

	return((void *)retp);
}

/*
 *	endnetconfig() frees up all data allocated by setnetconfig()
 *	but only if this is the last call in a potentially nested
 *	sequence of "setnetconfig() - endnetconfig()" calls.
 */

int
endnetconfig(vdata)
void  *vdata;
{
	/*
	 *	The argument is really a NCONF_HANDLE;  cast it here
	 */
	NCONF_HANDLE *nconf_handlep = (NCONF_HANDLE *)vdata;
	struct netconfig **tpp; /* traverses the array of netcf structures */
	struct netconfig **netpp0; /* local copy of netpp */
	int *nc_errorp;		/* pointer to per-thread error variable */

	/*
	 * Both the counter and the list must be locked during cleanup.
	 * Otherwise, there could be a memory leak.  See the example in
	 * setnetconfig().
	 */

	/*
	 * To avoid deadlock when two locks are to be held at the same time,
	 * the locks must always be acquired in the same order:
	 *
	 * set _netselect_counter_lock first.
	 * set _netselect_list_lock second.
	 */

	/* Get current head of netconfig list */
	netpp0 = netpp;
	/* Is netconfig list or handle empty? */
	if(netpp0 == NULL || nconf_handlep == NULL) {
			if ((nc_errorp = get_nc_error()) == NULL) {
				return (-1);
			}
			*nc_errorp = NC_NOSET;
			return(-1);
		}
	/* The handle is not empty and must be freed */
	free(nconf_handlep);

	/* The netconfig list was not empty and may need to be freed.
	 * Make sure we're the only call left before freeing list. */
	MUTEX_LOCK(&_netselect_counter_lock);
	if (--num_calls == 0) {
		/* Check whether the list is still not empty. */
		RW_WRLOCK(&_netselect_list_lock);
		if (netpp != NULL) {
			/* Free the storage to which the list members point */
			for (tpp = netpp; *tpp; tpp++) {
				free_netcf(*tpp);
			}
			free(netpp);
			netpp = NULL;
		}
		RW_UNLOCK(&_netselect_list_lock);
	}
	MUTEX_UNLOCK(&_netselect_counter_lock);
	return(0);
}

/*
 *	getnetconfig() returns the current entry in the list
 *	of netconfig structures.  It uses the nconf_handlep argument
 *	to determine the current entry. If setnetconfig() was not
 *	called previously to set up the list, return failure.
 */

struct netconfig *
getnetconfig(vdata)
void *vdata;
{
	/* The argument is really a NCONF_HANDLE;  cast it here */
	NCONF_HANDLE *nconf_handlep = (NCONF_HANDLE *)vdata;

	struct netconfig *retp;  /* holds the return value */
	struct netconfig **netpp0;  /* local copy of pointer to list */
	struct netconfig **finalpp0;  /* local copy of ptr to final ptr */
	int *nc_errorp;		/* pointer to per-thread error variable */

	/*
	 * Test that this thread is calling this function appropriately,
	 * by verifying that the list has not been freed and that the handle
	 * points to one of the struct netconfig pointers in the list.
	 * These tests do not guarantee that the call is correct.
	 * However, no other thread will be changing netpp, assuming that
	 * setnetconfig() has been called and has incremented num_calls.
	 * Therefore, there is no need to lock the list for long.
	 */

	/* 
	 * Get current head and final non-null member of netconfig list.
	 * Need read lock to make sure that netpp and finalpp are in synch.
	 */
	RW_RDLOCK(&_netselect_list_lock);
	netpp0 = netpp;
	finalpp0 = finalpp;
	RW_UNLOCK(&_netselect_list_lock);

	/* Does list exist? */
	if (netpp0 == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOSET;
		return(NULL);
	}

	/* Does handle point to a pointer to a struct netconfig in the list? */
	if (nconf_handlep == NULL
	 || nconf_handlep->nc_curr < netpp0
	 || nconf_handlep->nc_curr > finalpp0) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_BAD_HANDLE;
		return(NULL);
	}

	retp = *(nconf_handlep->nc_curr);
	if (retp != NULL)
		++(nconf_handlep->nc_curr);
	return(retp);
}

/*
 *	getnetconfig() searches the netconfig database for a
 *	given network id.  Returns a pointer to the netconfig
 *	structure or a NULL if not found.
 */

struct netconfig *
getnetconfigent(netid)
char *netid;
{
	struct netconfig *cfp; /* holds each entry in NETCONFIG */
	FILE *fp;	       /* file stream for NETCONFIG     */
	int *nc_errorp;		/* pointer to per-thread error variable */

	if ((fp = _fopen(NETCONFIG, "r")) == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_OPENFAIL;
		return(NULL);
	}

	while ((cfp = fgetnetconfig(fp)) && strcmp(netid, cfp->nc_netid)) {
		free_netcf(cfp);
	}

	(void) fclose(fp);
	return(cfp);
}

/*
 *	freenetconfigent frees the data allocated by getnetconfigent()
 */

void
freenetconfigent(netp)
struct netconfig *netp;
{
	free_netcf(netp);
}

/*
 *	getnetlist() reads the netconfig file and creates a
 *	NULL-terminated list of entries.
 *	Returns the pointer to the head of the list or a NULL
 *	on failure.
 */

static struct netconfig **
getnetlist()
{
	char  line[BUFSIZ];         /* holds each line of NETCONFIG        */
	FILE *fp;	            /* file stream for NETCONFIG           */
	struct netconfig **listpp;  /* the beginning of the netconfig list */
	struct netconfig **tpp;     /* used to traverse the netconfig list */
	int num_validentries;	    /* the number of valid entries in file */
	int count;		    /* the number of entries in file       */
	int *nc_errorp;		    /* pointer to per-thread error variable*/
	int *linenump;		    /* pointer to per-thread line counter  */

	if ((fp = _fopen(NETCONFIG, "r")) == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_OPENFAIL;
		return(NULL);
	}

	/*
	 *	Set count to the number of non-blank and comment lines
	 * 	in the NETCONFIG file plus 1 (since a NULL entry will
	 *	terminate the list).
	 */

	count = 1;
	while (fgets(line, BUFSIZ, fp)) {
		if (!(blank(line) || comment(line))) {
			++count;
		}
	}
	rewind(fp);

	if ((listpp = (struct netconfig **)
		      malloc(count * sizeof(struct netconfig *))) == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return(NULL);
	}

	/*
	 *	The following loop fills in the list (loops until
	 *	fgetnetconfig() returns a NULL) and counts the
	 *	number of entries placed in the list.  Note that
	 *	when the loop is completed, the last entry in the
	 *	list will contain a NULL (signifying the end of
	 *	the list).
	 */

	num_validentries = 0;
	if ((linenump = get_linenum()) == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return (NULL);
	}
	*linenump = 0;
	for (tpp = listpp; *tpp = fgetnetconfig(fp); tpp++) {
		num_validentries ++;
	}
	(void) fclose(fp);

	/*
	 *	If the number of valid entries is not the same
	 *	as the number of lines in the file, then some of the
	 *	lines did not parse correctly (so free up the
	 *	space and return NULL).  Note that count must be
	 *	decremented, since it is the number of entries + 1.
	 */

	finalpp = listpp + num_validentries;
	if (num_validentries != --count) {
		for (tpp = listpp; *tpp; tpp++) {
			free_netcf(*tpp);
		}
		free(listpp);
		listpp = NULL;
		finalpp = NULL;
	}
	return(listpp);
}

/*
 *	fgetnetconfig() parses a line of the netconfig file into
 *	a netconfig structure.  It returns a pointer to the
 *	structure of success and a NULL on failure or EOF.
 */

static struct netconfig *
fgetnetconfig(fp)
FILE *fp;
{
	register char *linep;	      /* pointer to a line in the file     */
	struct netconfig *netconfigp; /* holds the new netconfig structure */
	char  *tokenp;		      /* holds a token from the line       */
	char  *retvalp;		      /* the return value of fgets()       */
	int *nc_errorp;		   /* pointer to per-thread error variable */
	int *linenump;		   /* pointer to per-thread line counter   */
	int *fieldnump;		   /* pointer to per-thread field counter  */

	if (((linep = malloc(BUFSIZ)) == NULL)
	 || ((netconfigp = (struct netconfig *)malloc(sizeof(struct netconfig)))	     == NULL)) {
		if (linep) free(linep);
		if (netconfigp) free(netconfigp);
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return(NULL);
	}

	/*
	 *	skip past blank lines and comments.
	 */

	if ((linenump = get_linenum()) == NULL) {
		free(linep);
		free(netconfigp);
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return (NULL);
	}
	while (retvalp = fgets(linep, BUFSIZ, fp)) {
		(*linenump)++;
		if (!(blank(linep) || comment(linep))) {
			break;
		}
	}
	if (retvalp == NULL) {
		free(linep);
		free(netconfigp);
		return(NULL);
	}

	if ((fieldnump = get_fieldnum()) == NULL) {
		free(linep);
		free(netconfigp);
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return (NULL);
	}
	*fieldnump = 0;
	if (((netconfigp->nc_netid = gettoken(linep)) == NULL)
	 || ((tokenp = gettoken(NULL)) == NULL)
	 || ((netconfigp->nc_semantics = getvalue(tokenp, nc_semantics))
	      == FAILURE)
	 || ((tokenp = gettoken(NULL)) == NULL)
	 || ((netconfigp->nc_flag = getflag(tokenp)) == FAILURE)
	 || ((netconfigp->nc_protofmly = gettoken(NULL)) == NULL)
	 || ((netconfigp->nc_proto = gettoken(NULL)) == NULL)
	 || ((netconfigp->nc_device = gettoken(NULL)) == NULL)
	 || ((tokenp = gettoken(NULL)) == NULL)
	 || ((netconfigp->nc_nlookups = getnlookups(tokenp)) == FAILURE)
	 || ((netconfigp->nc_lookups = getlookups(tokenp)) == NULL
	     && netconfigp->nc_nlookups > 0)) {
		free(linep);
		free(netconfigp);
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_BADLINE;
		return(NULL);
	}
	return(netconfigp);
}

/*
 *	setnetpath() has the effect of "initializing" the
 *	NETPATH variable.  It reads in the netconfig entries (if not
 *	already read in), creates a list corresponding to the entries
 *	in the NETPATH variable (or the "visible" entries of netconfig
 *	if NETPATH is not set), and returns a pointer to the
 *	first value in the list.
 */

void *
setnetpath()
{
	int count;	            /* the number of entries in NETPATH     */
	char valid_netpath[BUFSIZ]; /* holds the valid entries in NETPATH   */
	char templine[BUFSIZ];	    /* has value of NETPATH when scanning   */
	struct netconfig **curr_pp; /* scans the list from NETPATH          */
	struct netconfig **tpp;     /* scans the list from netconfig file   */
	struct netconfig **netpp0;  /* local copy of pointer to list 	    */
	char *netpath;		    /* value of NETPATH from environment    */
	char *netid;		    /* holds a component of NETPATH         */
	register char *tp;	    /* used to scan NETPATH string          */
	register char *ntp;	    /* used to condense NETPATH string      */
	NCONF_HANDLE *retp;	    /* the return value                     */
	int *nc_errorp;		    /* pointer to per-thread error variable */
	struct netconfig **rnetpp;  /* pointer to return pointer */

	/* Allocate handle for netconfig list */
	if ((retp = (NCONF_HANDLE *)malloc(sizeof(NCONF_HANDLE))) == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return(NULL);
	}

	/*
	 * Both the counter and the list must be locked during initialization.
	 * Otherwise, there could be a memory leak.  See the example in
	 * setnetconfig().
	 */

	/*
	 * To avoid deadlock when two locks are to be held at the same time,
	 * the locks must always be acquired in the same order:
	 *
	 * set _netselect_counter_lock first.
	 * set _netselect_list_lock second.
	 */

	/* Get the counter lock and keep it set! */
	MUTEX_LOCK(&_netselect_counter_lock);

	/* Get current head of netconfig list */
	netpp0 = netpp;
	/* Is netconfig list empty? */
	if(netpp0 == NULL) {
		RW_WRLOCK(&_netselect_list_lock);
		/* Is netconfig list still empty? 
		 * (Another thread may have gotten here first.) */
		if(netpp == NULL) {
		/* Read netconfig entries into memory */
			netpp = getnetlist();
		}
		netpp0 = netpp;
		RW_UNLOCK(&_netselect_list_lock);
		if (netpp0 == NULL) {
			free(retp);
			/* Release the counter lock last, so no deadlocking. */
			MUTEX_UNLOCK(&_netselect_counter_lock);
			return(NULL);
		}
	}

	/* Now we know for sure that the netconfig list is not empty
	 * and that we have a handle. 
	 * However, we still need to parse the NETPATH and make a list of
	 * pointers to the corresponding struct netconfigs.
	 * If space for this list cannot be allocated, then we must
	 * decrement the counter and return NULL.
	 * Therefore, _netselect_counter_lock must remain set.
	 */

	/*
	 *	Get the valid entries of the NETPATH variable (and
	 *	count the number of entries while doing it).
	 *
	 *	This is done every time the procedure is called, just
	 *	in case NETPATH has changed from call to call.
	 */

	count = 0;
	valid_netpath[0] = '\0';
	if ((netpath = getenv(NETPATH)) == NULL
	 || netpath[0] == '\0') {

		/*
		 *	Since there is no NETPATH variable or it is empty,
		 *	the valid NETPATH consists of all "visible"
		 *	netids from the netconfig database.
		 */

		for (tpp = netpp0; *tpp; tpp++) {
			if ((*tpp)->nc_flag & NC_VISIBLE) {
				(void)strcat(valid_netpath, (*tpp)->nc_netid);
				(void)strcat(valid_netpath, ":");
				count ++;
			}
		}
	} else {

		/*
		 *	Copy the value of NETPATH (since '\0's will be
		 *	put into the string) and create the valid NETPATH
		 *	(by throwing away all netids not in the database).
		 *	If an entry appears more than once, it *will* be
		 *	listed twice in the list of valid netpath entries.
		 */

		(void)strcpy(templine, netpath);

		/*
		 *	Skip all leading ':'s
		 */

		tp = templine;
		while (*tp && *tp == ':')
			tp++;

		/*
		 *	Set the first token and scan to the next.
		 *	Allow literal ':' and '\'
		 */

		netid = tp;
		while (*tp && *tp != ':')
			if (*tp == '\\' && (*(tp+1) == ':' || *(tp+1) == '\\')){
				for (ntp = tp++; *(ntp+1); ntp++)
					*ntp = *(ntp+1);
				*ntp = *(ntp+1);	/* NULL */
			} else
				tp++;
		if (*tp)
			*tp++ = '\0';
		while (*tp == ':')
			tp++;

		while (*netid) {
			for (tpp = netpp0; *tpp; tpp++) {
				if (!strcmp(netid, (*tpp)->nc_netid)) {
					(void)strcat(valid_netpath,
						     (*tpp)->nc_netid);
					(void)strcat(valid_netpath, ":");
					count ++;
					break;
				}
			}
			/*
			 *	Set netid and scan to the next token
			 */

			netid = tp;
			while (*tp && *tp != ':')
				if (*tp == '\\'
				 && (*(tp+1) == ':' || *(tp+1) == '\\')) {
					for (ntp = tp++; *(ntp+1); ntp++)
						*ntp = *(ntp+1);
					*ntp = *(ntp+1);	/* NULL */
				} else
					tp++;
			if (*tp)
				*tp++ = '\0';
			while (*tp == ':')
				tp++;
		}
	}

	/*
	 *	Get space to hold the valid list (+1 for the NULL)
	 */

	if ((curr_pp = rnetpp
	      = (struct netconfig **)
	        malloc(++count * sizeof(struct netconfig *))) == NULL) {
		/*
		 * Abort the initialization for this call.
		 * All storage that we allocated must be freed.
		 */
		if (curr_pp) free(curr_pp);
		if (num_calls == 0) {
			/* This is the only active call, so free the list. */
			RW_WRLOCK(&_netselect_list_lock);
			for (tpp = netpp; *tpp; tpp++) {
				free_netcf(*tpp);
			}
			free(netpp);
			netpp = NULL;
			RW_UNLOCK(&_netselect_list_lock);
		}
		/* Release the counter lock last, so no deadlocking. */
		MUTEX_UNLOCK(&_netselect_counter_lock);
		/* Free the handle (to which no pointers have been added) */
		free(retp);
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return(NULL);
	}
	/*
	 * Now we know we also have a pointer to the right size list
	 * of pointers to struct netconfigs.  We are sure to return 
	 * success, so now it is safe to increment the counter of
	 * active calls and to release _netselect_counter_lock.
	 */

	num_calls ++;
	MUTEX_UNLOCK(&_netselect_counter_lock);

	/*
	 *	Populate the NETPATH list, ending it with a NULL.
	 *	Each entry in the list points to the structure in the
	 *	"netpp" list (the entry must exist in the list, otherwise
	 *	it wouldn't appear in valid_netpath[]).
	 */

	netid = tp = valid_netpath;
	while (*tp && *tp != ':')
		if (*tp == '\\' && (*(tp+1) == ':' || *(tp+1) == '\\')) {
			for (ntp = tp++; *(ntp+1); ntp++)
				*ntp = *(ntp+1);
			*ntp = *(ntp+1);	/* NULL */
		} else
			tp++;
	if (*tp)
		*tp++ = '\0';
	while (*netid) {
		for (tpp = netpp0; *tpp; tpp++) {
			if (!strcmp(netid, (*tpp)->nc_netid)) {
				*curr_pp++ = *tpp;
				break;
			}
		}
		netid = tp;
		while (*tp && *tp != ':')
			if (*tp == '\\' && (*(tp+1) == ':' || *(tp+1) == '\\')){
				for (ntp = tp++; *(ntp+1); ntp++)
					*ntp = *(ntp+1);
				*ntp = *(ntp+1);	/* NULL */
			} else
				tp++;
		if (*tp)
			*tp++ = '\0';
	}
	*curr_pp = NULL;

	/*
	 *	Return the pointer to the first entry in the list
	 */

	retp->nc_curr = retp->nc_head = rnetpp;
	return((void *)retp);
}

/*
 *	endnetpath() frees up all of the memory allocated by setnetpath().
 *	It returns -1 (error) if setnetpath was never called.
 */

int
endnetpath(vdata)
void *vdata;
{
	/*
	 *	The argument is really a NCONF_HANDLE;  cast it here
	 */
	NCONF_HANDLE *nconf_handlep = (NCONF_HANDLE *)vdata;
	struct netconfig **tpp; /* traverses the array of netcf structures */
	struct netconfig **netpp0; /* local copy of netpp */
	int *nc_errorp;		/* pointer to per-thread error variable */

	/*
	 * Both the counter and the list must be locked during cleanup.
	 * Otherwise, there could be a memory leak.  See the example in
	 * setnetconfig().
	 */

	/*
	 * To avoid deadlock when two locks are to be held at the same time,
	 * the locks must always be acquired in the same order:
	 *
	 * set _netselect_counter_lock first.
	 * set _netselect_list_lock second.
	 */

	/* Get current head of netconfig list */
	netpp0 = netpp;
	/* Is netconfig list or handle empty? */
	if(netpp0 == NULL || nconf_handlep == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (-1);
		*nc_errorp = NC_NOSET;
		return(-1);
	}

	/*
	 * The handle and the storage to which it points are not empty.
	 * They must be freed in any case.
	 */
	free(nconf_handlep->nc_head);
	free(nconf_handlep);

	/* The netconfig list was not empty and may need to be freed.
	 * Make sure we're the only call left before freeing list. */
	MUTEX_LOCK(&_netselect_counter_lock);
	if (--num_calls == 0) {
		/* Check whether the list is still not empty. */
		RW_WRLOCK(&_netselect_list_lock);
		if (netpp != NULL) {
			/* Free the storage to which the list members point */
			for (tpp = netpp; *tpp; tpp++) {
				free_netcf(*tpp);
			}
			free(netpp);
			netpp = NULL;
		}
		RW_UNLOCK(&_netselect_list_lock);
	}
	MUTEX_UNLOCK(&_netselect_counter_lock);
	return(0);
}

/*
 *	getnetpath() returns the current entry in the list
 *	from the NETPATH variable.  If setnetpath() was not called
 *	previously to set up the list, return NULL.
 */

struct netconfig *
getnetpath(vdata)
void *vdata;
{
	/* The argument is really a NCONF_HANDLE;  cast it here. */
	NCONF_HANDLE *nconf_handlep = (NCONF_HANDLE *)vdata;

	struct netconfig *retp;       /* holds the return value 	      */
	int *nc_errorp;		      /* pointer to per-thread error variable */

	/*
	 * Test that this thread is calling this function appropriately.
	 * These tests do not guarantee that the call is correct.
	 * However, no other thread will be changing netpp, assuming that
	 * setnetpath() has been called and has incremented num_calls.
	 * Therefore, there is no need to lock the list for long.
	 */

	/* Does handle point to a pointer to a struct netconfig in sublist? */
	if (nconf_handlep == NULL
	 || nconf_handlep->nc_curr == NULL) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_BAD_HANDLE;
		return(NULL);
	}

	retp = *(nconf_handlep->nc_curr);
	if (retp != NULL)
		++(nconf_handlep->nc_curr);
	return(retp);
}

/*
 *	free_netcf() simply frees up the memeory allocated for the
 *	given netcf structure.
 */

static void
free_netcf(netcfp)
struct netconfig *netcfp;
{
	free(netcfp->nc_lookups);
	free(netcfp->nc_netid);
	free(netcfp);
}

/*
 *	blank() returns true if the line is a blank line, 0 otherwise
 */

static int
blank(cp)
char *cp;
{
	while (*cp && isspace(*cp)) {
		cp++;
	}
	return(*cp == '\0');
}

/*
 *	comment() returns true if the line is a comment, 0 otherwise.
 */

static int
comment(cp)
char *cp;
{
	while (*cp && isspace(*cp)) {
		cp ++;
	}
	return(*cp == '#');
}

/*
 *	getvalue() searches for the given string in the given array,
 *	and return the integer value associated with the string.
 */

static unsigned long
getvalue(cp, nc_data)
char *cp;
struct nc_data nc_data[];
{
	int i;	/* used to index through the given struct nc_data array */

	for (i = 0; nc_data[i].string; i++) {
		if (!strcmp(nc_data[i].string, cp)) {
			break;
		}
	}
	return(nc_data[i].value);
}

/*
 *	getflag() creates a bitmap of the one-character flags in
 *	the given string.  It uses nc_flags array to get the values.
 */

static unsigned long
getflag(cp)
char *cp;
{
	int i;	                 /* indexs through the nc_flag array */
	unsigned long mask = 0;  /* holds bitmask of flags           */

	while (*cp) {
		for (i = 0; nc_flag[i].string; i++) {
			if (*nc_flag[i].string == *cp) {
				mask |= nc_flag[i].value;
				break;
			}
		}
		cp ++;
	}
	return(mask);
}

/*
 *	getlookups() creates and returns an array of string representing
 *	the directory lookup libraries, given as a comma-seperated list
 *	in the argument "cp".
 */

static char **
getlookups(cp)
char *cp;
{
	unsigned long num;     /* holds the number of entries in the list   */
	char **listpp;	       /* the beginning of the list of dir routines */
	register char **tpp;   /* traverses the list, populating it         */

	num = getnlookups(cp);
	if (num == 0
	 || (listpp = (char **)malloc((num + 1) * sizeof(char *))) == NULL) {
		return(NULL);
	}

	tpp = listpp;
	while (num--) {
		*tpp  = cp;

		/*
		 *	Travserse the string looking for the next entry
		 *	of the list (i.e, where the ',' or end of the
	 	 *	string appears).  If a "\" is found, shift the
		 *	token over 1 to the left (taking the next char
		 *	literally).
		 */

		while (*cp && *cp != ',') {
			if (*cp == '\\' && *(cp + 1)) {
				shift1left(cp);
			}
			cp ++;
		}
		if (*cp)
			*cp++ ='\0';
		tpp ++;
	}
	*tpp = NULL;
	return(listpp);
}

/*
 *	getnlookups() returns the number of entries in a comma-separated
 *	string of tokens.  A "-" means no strings are present.
 */

static unsigned long
getnlookups(cp)
char *cp;
{
	unsigned long count;	/* the number of tokens in the string */

	if (!strcmp(cp, "-")) {
		return(0);
	}

	count = 1;
	while (*cp) {
		if (*cp == ',') {
			count++;
		}

		/*
		 *	If a "\" is in the string, take the next character
		 *	literally.  Onlly skip the character if "\" is
		 *	not the last character of the token.
		 */

		if (*cp == '\\' && *(cp + 1)) {
			cp ++;
		}
		cp ++;
	}
	return(count);
}

/*
 *	gettoken() behaves much like strtok(), except that
 *	it knows about escaped space characters (i.e., space characters
 *	preceeded by a '\' are taken literally).
 */

static char *
gettoken(cp)
char	*cp;
{
	char	*p;		/* the beginning of the new token           */
	char	*retp;		/* the token to be returned                 */
	char	**savepp;	/* pointer to per-thread position in string */
	int	*fieldnump;	/* pointer to per-thread field counter      */
	int 	*nc_errorp;     /* pointer to per-thread error variable     */

	if (((savepp = get_savep()) == NULL)
	 || ((fieldnump = get_fieldnum()) == NULL)) {
		if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
		*nc_errorp = NC_NOMEM;
		return (NULL);
	}
	(*fieldnump)++;

	/*
	 *	Determine if first or subsequent call
	 */

	p = (cp == NULL)? *savepp: cp;

	/*
	 *	Return if no tokens remain.
	 */

	if (p == 0) {
		return(NULL);
	}

	while (isspace(*p))
		p++;

	if (*p == '\0') {
		return(NULL);
	}

	/*
	 *	Save the location of the token and then skip past it
	 */

	retp = p;
	while (*p) {
		if (isspace(*p)) {
			break;
		}
		/*
		 *	Only process the escape of the space separator;
		 *	since the token may contain other separators,
		 *	let the other routines handle the escape of
		 *	specific characters in the token.
		 */

		if (*p == '\\' && *(p + 1) != '\n' && isspace(*(p + 1))) {
			shift1left(p);
		}
		p ++;
	}
	if(*p == '\0') {
		*savepp = 0;	/* indicate this is last token */
	} else {
		*p = '\0';
		*savepp = ++p;
	}
	return(retp);
}

/*
 *	shift1left() moves all characters in the string over 1 to
 *	the left.
 */

static void
shift1left(p)
char *p;
{
	for (; *p; p++)
		*p = *(p + 1);
}

char *
nc_sperror()
{
	int *nc_errorp;		   /* pointer to per-thread error variable */
	int *linenump;		   /* pointer to per-thread line counter   */
	int *fieldnump;		   /* pointer to per-thread field counter  */
	char *retstrp;		   /* pointer to per-thread return string  */

	if ((retstrp = get_retstr()) == NULL) return (NULL);
	if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
	switch (*nc_errorp) {
	   case NC_NOERROR:
		(void)strcpy(retstrp, gettxt("uxnsl:31", "No Error"));
		break;
	   case NC_NOMEM:
		(void)sprintf(retstrp,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "netselect");
		break;
	   case NC_NOSET:
		(void)strcpy(retstrp,
		    gettxt("uxnsl:164",
	       "routine called before calling setnetpath() or setnetconfig()"));
		break;
	   case NC_OPENFAIL:
		(void)strcpy(retstrp,
		    gettxt("uxnsl:165", "cannot open /etc/netconfig"));
		break;
	   case NC_BADLINE:
		if ((linenump = get_linenum()) == NULL) {
			if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
			*nc_errorp = NC_NOMEM;
			return (NULL);
		}
		else if ((fieldnump = get_fieldnum()) == NULL) {
			if ((nc_errorp = get_nc_error()) == NULL) return (NULL);
			*nc_errorp = NC_NOMEM;
			return (NULL);
		}
		(void)sprintf(retstrp,
		      gettxt("uxnsl:166",
			  "error in /etc/netconfig: field %d of line %d"),
		      *fieldnump, *linenump);
		break;
	   case NC_BAD_HANDLE:
		(void)sprintf(retstrp,
		    gettxt("uxnsl:167", "bad netconfig handle"));
		break;

	   default:
		(void)sprintf(retstrp,
		      gettxt("uxnsl:168",
			  "network selection unknown error %d"),
		      *nc_errorp);
		break;
	}
	return(retstrp);
}

void
nc_perror(string)
char *string;
{
	if(strlen(string))
		fprintf(stderr, "%s: %s\n", string, nc_sperror());
	else
		fprintf(stderr, "%s\n", nc_sperror());
}

static int *
get_nc_error()
{
	static int nc_error = NC_NOERROR;

#ifdef _REENTRANT
	struct _netsel_tsd *key_tbl;
	int *nc_errorp;

	if (FIRST_OR_NO_THREAD) 
		return (&nc_error);

	if ((key_tbl = (struct _netsel_tsd *)
			_mt_get_thr_specific_storage(_netselect_key,
						     NETSEL_KEYTBL_SIZE))
	    == NULL)
		return (NULL);
	if (key_tbl->ncerror_p == NULL) {
		if ((nc_errorp = (int *)calloc(1, sizeof(int))) == NULL)
			return (NULL);
		key_tbl->ncerror_p = nc_errorp;
		*nc_errorp = NC_NOERROR;
	}
	return((int *)key_tbl->ncerror_p);
#else
	return (&nc_error);
#endif /* _REENTRANT */
	
}

static int *
get_linenum()
{
	static int linenum = 0;

#ifdef _REENTRANT
	struct _netsel_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) 
		return (&linenum);

	if ((key_tbl = (struct _netsel_tsd *)
		       _mt_get_thr_specific_storage(_netselect_key,
						    NETSEL_KEYTBL_SIZE))
	    == NULL)
		return (NULL);
	if (key_tbl->linenum_p == NULL) {
		key_tbl->linenum_p = calloc(1, sizeof(int));
	}
	return((int *)key_tbl->linenum_p);
#else
	return (&linenum);
#endif /* _REENTRANT */
}

static int *
get_fieldnum()
{
	static int fieldnum = 0;

#ifdef _REENTRANT
	struct _netsel_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) 
		return (&fieldnum);

	if ((key_tbl = (struct _netsel_tsd *)
		       _mt_get_thr_specific_storage(_netselect_key,
						    NETSEL_KEYTBL_SIZE))
	    == NULL)
		return (NULL);
	if (key_tbl->fieldnum_p == NULL) {
		key_tbl->fieldnum_p = calloc(1, sizeof(int));
	}
	return((int *)key_tbl->fieldnum_p);
#else
	return (&fieldnum);
#endif /* _REENTRANT */
}

static char *
get_retstr()
{
	static char retstr[BUFSIZ];

#ifdef _REENTRANT
	struct _netsel_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) 
		return (&retstr[0]);

	if ((key_tbl = (struct _netsel_tsd *)
		       _mt_get_thr_specific_storage(_netselect_key,
						    NETSEL_KEYTBL_SIZE))
	    == NULL)
		return (NULL);
	if (key_tbl->retstr_p == NULL) {
		key_tbl->retstr_p = calloc(1, BUFSIZ);
	}
	return((char *)key_tbl->retstr_p);
#else
	return (&retstr[0]);
#endif /* _REENTRANT */
}

static char **
get_savep()
{
	static char	*savep;	   /* the place where we left off    */

#ifdef _REENTRANT
	struct _netsel_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) 
		return (&savep);

	if ((key_tbl = (struct _netsel_tsd *)
		       _mt_get_thr_specific_storage(_netselect_key,
						    NETSEL_KEYTBL_SIZE))
	    == NULL)
		return (NULL);
	if (key_tbl->savep_p == NULL) {
		key_tbl->savep_p = calloc(1, sizeof(char *));
	}
	return((char **)key_tbl->savep_p);
#else
	return (&savep);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_netsel_ncerror(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

void
_free_netsel_linenum(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

void
_free_netsel_fieldnum(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

void
_free_netsel_retstr(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

void
_free_netsel_savep(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */
