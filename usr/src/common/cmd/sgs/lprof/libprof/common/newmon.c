/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/newmon.c	1.5"
/*LINTLIBRARY*/
/*
 *	Assumptions:
 *	1) I am assuming that _newmon() will be called before the 1st .init
 *	   and after the last .fini - always, independent of whether the
 *	   code is dynamic or static.
 *	4) I am assuming that 600 functions in the call count array is
 *	   sufficient for most programs, more will be allocated if necessary.
 *
 *	Environment variable PROFDIR added such that:
 *		If PROFDIR doesn't exist, "dmon.out" is produced as before.
 *		If PROFDIR = NULL, no profiling output is produced.
 *		If PROFDIR = string, "string/pid.progname" is produced,
 *		  where name consists of argv[0] suitably massaged.
 *	Routines:
 *		(global) _newmon	initalize, cleanup for profiling
 *		(global) _mcount	function call counter
 *		(global) _mcountNewent	call count entry manager (in libc)
 *		(global) _mnewblock	call count block allocator
 *
 *	_newmon(), coordinating with _mcount(), _mcountNewent() and
 *	 _mnewblock(), maintains a series of blocks of prof-profiling 
 *	information.  These blocks are added in response to calls to
 *	monitor() (explicitly or via mcrt[1n]) and, via _mcount()'s
 *	calls to _mcountNewent() thence to _mnewblock().
 *
 *	The first time _newmon() is called, it sets up the information
 *	by allocating and initializing the first *_SOentry.
 *
 *	For each fcn, the first time it calls _mcount(), mcount calls
 *	_mcountNewent() which parcels out the fcn call count entries
 *	from the current block, until they are exausted; then it calls
 *	_mnewblock().
 *
 *	_mnewblock() allocates a call count block and links it to the call
 *	count linked list.  Each new _mnewblock() block or user block,
 *	is added to the list as it comes in, FIFO.
 *
 *	When _newmon() is called to close up shop, it writes out
 *	a summarizing header, ALL the fcn call counts and all the
 *	 specified execution histograms.
 *
 *	NOTE - no block may be freed, until _newmon() is called to clean up!!!!
 *
 */
/*
#include "inc/synonyms.h"
#include "inc/shlib.h"
*/
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dprof.h"


#define PROFDIR	"PROFDIR"

char **___Argv = NULL; /* initialized to argv array by mcrt1 (if loaded) */

	/* countbase and countlimit are used to parcel out
	 * the pc,count cells from the current block one at 
	 * a time to each profiled function, the first time 
	 * that function is called.
	 * When countbase reaches 0, _mnewent() calls
	 * _mnewblock() to link in a new block.
	 *
	 * Only _newmon/_mcount/_mcountNewent/_mnewblock() should
	 * change these.
	 */

WORD    _out_tcnt;      /* holder for histogram out of bounds pcs */
SOentry	*_curr_SO = NULL;	/* pointer to a _SOentry */
SOentry	*_act_SO = NULL;	/* pointer to link list of active _SOentry's */
SOentry	*_inact_SO = NULL;	/* pointer to link list of deleted _SOentry's */
SOentry	*_last_SO = NULL;	/* pointer to link list of deleted _SOentry's */

/* declare local variables */
static char	*dmon_out;
extern Cntb	*_cntbuffer;	/* points to current call count buffer */
extern int	countbase; 
static struct sigaction psig;


extern void	_dprofil();
#if DEBUG
void *printSOentry();
#endif

/* declare local functions */



void 
_newmon(int (*alowpc)(), int (*ahighpc)(), WORD *buffer, int bufsize)
/* int	(*alowpc)(),	- gets used by newmon
 *	(*ahighpc)();	boundaries of text to be monitored- gets used by newmon
 * WORD	*buffer;	ptr to space for monitor data (WORDs)- ignored
 * int	bufsize;	size of above space (in WORDs)- gets used by newmon
*/

{
	register char *s;
	char	*lowpc = (char *)alowpc;
	char	*highpc = (char *)ahighpc;
	int	ssiz;
	SOentry *tmpSO;

#ifdef DEBUG
	printf("monitor: lowpc = 0x%x, highpc = 0x%x\n", lowpc, highpc);
	printf("         buffer = 0x%x, bufsize = %d\n",  buffer, bufsize);
#endif

	if (lowpc == NULL) {		 /* true only at the end */
					/* if nothing collected ? */
		_dprofil((unsigned int)0); /* turn off profiling signal */
		if ( !_writeBlocks() )
			perror(dmon_out);
		return;
	}
	/*
	 * Ok - they want to submit a block for immediate use, for
	 *	function call count consumption, and execution profile
	 *	histogram computation.
	 * Next thing - get name to use. If PROFDIR is NULL, let's
	 *	get out now - they want No Profiling done.
	 *
	 * Otherwise:
	 * Set the ratio of buffer size to text size.
	 * Call profil and return.
	 */

	if ((s = getenv(PROFDIR)) == NULL)  /* PROFDIR not in environment */
		dmon_out = MON_OUT;	    /* use default "mon.out" */
	else if (*s == '\0')		    /* value of PROFDIR is NULL */
		return;			    /* no profiling on this run */
	else {				    /* construct "PROFDIR/pid.progname" */
		register int n;
		register pid_t pid;
		register char *name;
		size_t len;

		len = strlen(s);
		/* 15 is space for /pid.mon.out\0, if necessary */
		if ((dmon_out = malloc(len + strlen(___Argv[0]) + 15)) == NULL) {
			perror("no space for profile program name");
			return;
		}
		strcpy(dmon_out, s);
		name = dmon_out + len;
		*name++ = '/';		    /* two slashes won't hurt */

		if ((pid = getpid()) <= 0)  /* extra test just in case */
			pid  = 1;	    /* getpid returns something inappropriate */
		else if (pid >=10000)
			pid %= 10000; 
		for (n = 10000; n > pid; n /= 10)
			;		    /* suppress leading zeros */
		for ( ; ; n /= 10) {
			*name++ = pid/n + '0';
			if (n == 1)
			    break;
			pid %= n;
		}
		*name++ = '.';

		if (___Argv != NULL)	    /* mcrt0.s executed */
			if ((s = strrchr(___Argv[0], '/')) != NULL)
				strcpy(name, s + 1);
			else
				strcpy(name, ___Argv[0]);
		else
			strcpy(name, MON_OUT);
	}

	tmpSO = (SOentry *)buffer;
	ssiz = sizeof(SOentry)/sizeof(WORD);
	buffer += ssiz;
	bufsize -= ssiz;
	_cntbuffer = (Cntb *)buffer;
	ssiz = sizeof(Cntb)/sizeof(WORD);
	/* get space for the call counts */
	buffer += ssiz;		/* move ptr past call count buffer */
	bufsize -= ssiz;

	/* fill in information for the a.out */
	tmpSO->tcnt = buffer;
	tmpSO->baseaddr = (long) lowpc;
	tmpSO->textstart = (long) lowpc; /* same as baseaddr for a.out */
	tmpSO->endaddr = (long) highpc;
	tmpSO->size = bufsize;
	tmpSO->ccnt = 0;
	tmpSO->SOpath = "";
	tmpSO->next_SO = 0;
	tmpSO->prev_SO = 0;
		
	/* turn on profiling signal */
	 _dprofil(1); 

	/* set up global pointers for profiling shared objects if any */
	_act_SO = tmpSO;
	_inact_SO = NULL;
	_curr_SO = _act_SO;
	_last_SO = _act_SO;

	_SOin(0);
	return;
}


	/* writeBlocks() - write accumulated profiling info, std fmt.
	 *
	 * This routine collects the function call counts, and the
	 * last specified profil buffer, and writes out one combined
	 * 'pseudo-block', as expected by current and former versions
	 * of prof.
	 *
	 * Note that the output file will have the following structure:
	 *	|-------------------------------|
	 *	| 1st SOentry = 0, + _out_tcnt	|
	 *	| rest of SOentrys		|
	 *	|-------------------------------|
	 *	| all call counts 3 pcs data	|
	 *	|   # calls, fptr, & SOentry ptr|
	 *	|-------------------------------|
	 *	| hist for SOentry #1		|
	 *	| hist for SOentry #2		|
	 *	|  .				|
	 *	|  .				|
	 *	|  .				|
	 *	|_______________________________|
	 */
static int 
_writeBlocks() {
	int 	fd;		/* file descriptor */
	int 	ok;		/* ok? if write occurs */
	int 	i = 0, total = 0;	/* random counters */
	int 	tblsize = 0;
	int 	hdrsize;
	char 	*nametbl;
	SOentry *tmpSO;		/* temp SOentry ptr */
	SOentry *SOlist;	
	Cntb	*tmpCntb;	/* temp Cntb pointer */
	SOentry *sum;		/* pointer to output header */
	char	*end;
	const SOentry mt = {0};	/* empty SOentry for initialization */

	/* open the output file */
	if ( (fd = creat(dmon_out, 0666)) < 0 )
		return 0;
	/* the summary header needs to be 1 bigger than the # of SOentries
	 * so that the first entry can be 0 for old prof.  Note that
	 * the header is the SOentries, but that the pointer to each
	 * next_SO should be modified to point to the beginning of
	 * that SOentry in memory.
	*/
	SOlist = _act_SO;
	_act_SO = NULL;
	_last_SO->next_SO = _inact_SO;
	/* need to know how big a header space is needed */
	for( tmpSO = SOlist; tmpSO != NULL; tmpSO = tmpSO->next_SO){
		total++;
		tblsize += strlen(tmpSO->SOpath) +1;
	}

	hdrsize = (total + 1) * sizeof(SOentry);
	if ((sum = (SOentry *)malloc(hdrsize)) == 0) {
		perror("writeBlocks - can't malloc header");
		return;
	}
	if ((nametbl = (char *)malloc(sizeof(char) * tblsize)) == 0) {
		perror("writeBlocks - can't malloc name table");
		return;
	}

	/* zero out the 1st entry */
	sum[0] = mt;
	sum[0].size = total ; 
	sum[0].baseaddr = (unsigned long) _out_tcnt;
	sum[0].ccnt = tblsize;

	/* and copy the rest to sum */
	/* change next_SO to point to itself rather than next entry */
	/* in the linked list */
	i=1;
	end = nametbl;
	for( (tmpSO = SOlist); tmpSO != NULL; tmpSO = tmpSO->next_SO){
		int len = strlen(tmpSO->SOpath) +1;

		sum[i] = *tmpSO;
		if (tmpSO->SOpath)
			strcpy(end,tmpSO->SOpath);
		sum[i].SOpath = (char *) (end - nametbl);
		end += len;
		sum[i].next_SO = tmpSO;
#ifdef DEBUG
		printf("make sure structure gets copied correctly\n");
		printSOentry(tmpSO);
		printSOentry(&sum[i]);
#endif
		i++;
	}

	if (write(fd, (const void *)sum, hdrsize ) !=  hdrsize ){
		perror("newmon.c(writeBlocks) - trouble writing header");
		return;
	}
	if (write(fd, nametbl, sizeof(char) * tblsize) !=  sizeof(char) * tblsize ){
		perror("newmon.c(writeBlocks) - trouble writing name table");
		return;
	}
	/* if the hdr went out ok, write out call count area */
		/* write out the count array */
	if ((ok = write(fd, _cntbuffer->cnts, (FCNTOT-countbase)*sizeof(Cnt))) == (FCNTOT-countbase)*sizeof(Cnt))
	{
		if (_cntbuffer->next != (Cntb *)0)
		{
			for ((tmpCntb = _cntbuffer->next); tmpCntb != NULL; tmpCntb = tmpCntb->next) /* more than one buffer */
			{
				if ((ok = write(fd, tmpCntb->cnts, sizeof(tmpCntb->cnts))) != sizeof(tmpCntb->cnts))
				{
					perror("newmon.c(writeBlocks) - trouble writing call counts");
					return;
				}
			}
		}
	}
	else
		{
			perror("newmon.c(writeBlocks) - trouble writing call counts");
			return;
		}
	
	/* write out the histogram areas */
	if (ok) {
		for((tmpSO = SOlist); tmpSO != 0; tmpSO = tmpSO->next_SO) 
			if ((ok = write(fd, tmpSO->tcnt, 
			     tmpSO->size *sizeof(WORD))) 
			     != tmpSO->size * sizeof(WORD)) {
				perror("newmon.c(writeBlocks)");
				return;
			}
	}
	(void) close(fd);
	return( ok );	/* indicate success */
}

/* _search() gets the pc and checks if it's in range of any _SOentry
*  update _curr_SO & return pointer to _SOentry if found,
*  otherwise return NULL
*/

SOentry 
*_search(unsigned long pc)
{
	SOentry	*tmpSO;	/* temp pointer to SOentry */

#ifdef DEBUG
	printf("in _search... received pc = 0x%x\n", pc);
#endif
	/* search thru the linked list of _SOentry's */
	for ( tmpSO=_act_SO; tmpSO != NULL; tmpSO=tmpSO->next_SO)
	{
		if ( ( pc >= tmpSO->textstart) && ( pc < tmpSO->endaddr) )
		{
#ifdef DEBUG
			printf("  found match: textstart = 0x%x", tmpSO->textstart);
			printf("  endaddr = 0x%x\n", tmpSO->endaddr);
			printf("  returning from  _search... %s\n", tmpSO->SOpath);
#endif

			_curr_SO=tmpSO;		/* update _curr_SO */

			return(tmpSO);
		}
	}
	return((SOentry *)NULL);
}

/* mnewblock() - allocate and link in a new call count block.
 *
 * This routine, called by _mcountNewent(), allocates a new block
 * containing the function call count array, and resets countbase
 * and _cntbuffer pointer.
 *
 * This routine cannot be called recursively, since (each) mcount
 * has a local lock which prevents recursive calls to mcountNewent.
 * See mcountNewent for more details.
 * 
 */

void 
_mnewblock()
{
	Cntb	*ncntbuf;	/* temporary Cntb pointer */

	/* temporarily turn off SIGPROF signal */
	sigemptyset(&psig.sa_mask);
	sigaddset(&psig.sa_mask, SIGPROF);
	sigprocmask(SIG_BLOCK, &psig.sa_mask, 0);
	/* get space for new buffer, malloc returns NULL on failure */
	ncntbuf = (Cntb *)malloc(sizeof(Cntb));
	if (ncntbuf == NULL)
	{
		perror("mcount(mnewblock)");
		return;
	}

	/* link new call count buffer to old call count buffer */
	ncntbuf->next = _cntbuffer;

	/* reset countbase and _cntbuffer */
	_cntbuffer = ncntbuf;
	countbase  = FCNTOT;
	/* turn back on SIGPROF signal */
	sigprocmask(SIG_UNBLOCK, &psig.sa_mask, 0);
}

#ifdef DEBUG
void *printSOentry(SOentry *ptr)
{
	printf("This entry contains: ");
	printf("SO_path  %s\n", ptr->SOpath);
	printf("		     baseaddr  0x%x\n", ptr->baseaddr);
	printf("		     txtstart  0x%x\n", ptr->textstart);
	printf("		     endaddr   0x%x\n", ptr->endaddr);
	printf("		     ccnt      %d\n",   ptr->ccnt);
	printf("		     tcnt      0x%x\n", ptr->tcnt);
	printf("		     size      %d\n",   ptr->size);
	printf("		     next_SO   0x%x\n", ptr->next_SO);
}
#endif
