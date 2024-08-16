/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/lck.c	1.4"
#ident	"$Header: lck.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function lck.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/var.h>
#include <a.out.h>
#include <sys/proc.h>
#include <sys/vnode.h>
/*
#include <sys/fs/vx_inode.h>
*/
#include <sys/flock.h>
#include <sys/metrics.h>
#include "crash.h"

static struct syment *Sleeplcks;		/* namelist symbol */
struct syment *S_s5hinode;		/* pointers */
struct syment *S_sfs_ihead;		/* pointers */
extern struct syment *S_vxfs_fshead;
struct syment *S_mets;

struct 	mets		mets;
ulong_t	flckinfo[4];

struct procid {				/* effective and sys ids */
	pid_t epid;
	long sysid;
	int valid;
};
struct procid *procptr;			/* pointer to procid structure */
extern char *malloc();

/* get effective and sys ids into table */
int
getprocid()
{
	struct proc *prp, prbuf;
	static int lckinit = 0;
	register i;
	proc_t *slot_to_proc();

	if(lckinit == 0) {
		procptr = (struct procid *)malloc((unsigned)
			(sizeof (struct procid) * vbuf.v_proc));
		lckinit = 1;
	}

	for (i = 0; i < vbuf.v_proc; i++) {
		prp = slot_to_proc(i);
		if (prp == NULL)
			procptr[i].valid = 0;
		else {
			readmem((long)prp,1, -1,(char *)&prbuf,sizeof (proc_t),
				"proc table");
			procptr[i].epid = prbuf.p_epid;
			procptr[i].sysid = prbuf.p_sysid;
			procptr[i].valid = 1;
		}
	}
}

/* find process with same id and sys id */
int
findproc(pid,sysid)
pid_t pid;
short sysid;
{
	int slot;

	for (slot = 0; slot < vbuf.v_proc; slot++) 
		if ((procptr[slot].valid) &&
		    (procptr[slot].epid == pid) &&
		    (procptr[slot].sysid == sysid))
			return slot;
	return(-1);
}


/* get arguments for lck function */
int
getlcks()
{
	int	totalflcks;
	int	inusedflcks;
	int slot = -1, all;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;

	if (!S_mets)
		if (!(S_mets = symsrch("m")))
			error("m not found in the symbol table\n");
	if(!Sleeplcks)
		if(!(Sleeplcks = symsrch("sleeplcks")))
			error("sleeplcks not found in symbol table\n");
	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	getprocid();
	readmem((long)S_mets->n_value,1,-1,(char *)&mets,
		sizeof mets,"mets table");
	
	fprintf(fp,"\nAdministrative Info:\n");
	fprintf(fp,"Currently_in_use  Total_used\n");
	fprintf(fp,"     %5d             %5d\n\n",
		mets.mets_files.msf_flck[MET_INUSE],
		mets.mets_files.msf_flck[MET_TOTAL]);
	if(args[optind]) {
		fprintf(fp,"TYP WHENCE      START        LEN\n\tPROC     EPID    SYSID WAIT     PREV     NEXT\n\n");
			do {
				if((addr = strcon(args[optind], 'h')) == -1)
					error("\n");
				prlcks(phys,addr);
			}while(args[++optind]);
	} else
		prilcks();
}


/* print lock information relative to all inodes (default) */
int
prilcks()
{
	struct	filock	*slptr,fibuf;
	long iptr;
	int active = 0;
	int free = 0;
	int sleep = 0;
	int next,prev;
	long addr;
	int i;
	extern int vxfs_lck();

	fprintf(fp,"Active Locks:\n");
	fprintf(fp,"      INO         TYP WHENCE      START        LEN\n\tPROC     EPID    SYSID WAIT     PREV     NEXT\n");


	if (S_s5hinode = symsrch("hinode")) {
		if(S_s5hinode->n_value != 0)
			active += practlcks(S_s5hinode, 0, "s5");
	}

	if (S_sfs_ihead = symsrch("sfs_ihead")) {
		if(S_sfs_ihead->n_value != 0)
			active += practlcks(S_sfs_ihead, 0, "ufs/sfs");
	}
	if (S_vxfs_fshead = symsrch("vxfs_fshead")) {
		if (S_vxfs_fshead->n_value != 0) {
			active += vxfs_lck();
		}
	}

	fprintf(fp,"\nSleep  Locks:\n");
	fprintf(fp,"TYP WHENCE      START        LEN\n\tLPROC     EPID    SYSID BPROC     EPID    SYSID     PREV     NEXT\n");
	readmem((long)Sleeplcks->n_value,1,-1,(char *)&slptr,
		sizeof slptr,"sleep lock information table");
	while (slptr) {
		readmem((long)slptr,1,-1,(char *)&fibuf,sizeof fibuf,
			"sleep lock information table slot");
		++sleep;
		if(fibuf.set.l_type == F_RDLCK) 
			fprintf(fp," r  ");
		else if(fibuf.set.l_type == F_WRLCK) 
			fprintf(fp," w  ");
		else fprintf(fp," ?  ");
		fprintf(fp,"%6d %10ld %10ld\n\t%5d %8d %8d %5d %8d %8d %8x %8x\n\n",
			fibuf.set.l_whence,
			fibuf.set.l_start,
			fibuf.set.l_len,
			findproc(fibuf.set.l_pid,fibuf.set.l_sysid),
			fibuf.set.l_pid,
			fibuf.set.l_sysid,
			findproc(fibuf.stat.blk.pid,fibuf.stat.blk.sysid),
			fibuf.stat.blk.pid,
			fibuf.stat.blk.sysid,
			fibuf.prev,
			fibuf.next);
		if (fibuf.next == fibuf.prev)
			break;
		slptr = fibuf.next;
	}

	fprintf(fp,"\nSummary From Actual Lists:\n");
	fprintf(fp," TOTAL    ACTIVE  SLEEP\n");
	fprintf(fp," %4d    %4d    %4d\n",
		active+sleep,
		active,
		sleep);
}    

int
prlcks(phys,addr)
int phys;
long addr;
{
	struct filock fibuf;

	readmem(addr,!phys,-1,(char *)&fibuf,sizeof fibuf,"frlock");
	fprintf(fp," %c%c%c",
	(fibuf.set.l_type == F_RDLCK) ? 'r' : ' ',
	(fibuf.set.l_type == F_WRLCK) ? 'w' : ' ',
	(fibuf.set.l_type == F_UNLCK) ? 'u' : ' ');
	fprintf(fp,"%6d %10ld %10ld\n\t%4d %8d %8d %4x %8x %8x\n",
		fibuf.set.l_whence,
		fibuf.set.l_start,
		fibuf.set.l_len,
		findproc(fibuf.set.l_pid,fibuf.set.l_sysid),
		fibuf.set.l_pid,
		fibuf.set.l_sysid,
		fibuf.stat.wakeflg,
		fibuf.prev,
		fibuf.next);
}

int
practlcks(sfsh,getifilock,itype)
struct syment *sfsh;
struct filock *getifilock();
char *itype;
{
	int active = 0;
	extern int s5lck();
	extern int sfslck();

	if (strcmp(itype, "s5") == 0) {
		active += s5lck();

	} else if (strcmp(itype, "ufs/sfs") == 0) {
		 active += sfslck();

	}
	return(active);
}

int
print_lock(actptr, addr, itype)
struct filock *actptr;
long addr;
char *itype;
{
	struct	filock	fibuf, *flp, *nflp;
	int active = 0;

	for (flp = actptr; flp != NULL; flp = nflp) {
		readmem((long)flp,1,-1,(char *)&fibuf,
			sizeof fibuf,"filock information");
		nflp = fibuf.next;
		++active;
		fprintf(fp,"%x(%-7s) ",addr,itype);
		if(fibuf.set.l_type == F_RDLCK) 
			fprintf(fp," r  ");
		else if(fibuf.set.l_type == F_WRLCK) 
			fprintf(fp," w  ");
		else fprintf(fp," ?  ");
		fprintf(fp,"%6d %10ld %10ld\n\t%4d %8d %8d %4x %8x %8x\n\n",
			fibuf.set.l_whence,
			fibuf.set.l_start,
			fibuf.set.l_len,
			findproc(fibuf.set.l_pid,fibuf.set.l_sysid),
			fibuf.set.l_pid,
			fibuf.set.l_sysid,
			fibuf.stat.wakeflg,
			fibuf.prev,
			fibuf.next);
		if (fibuf.next == fibuf.prev)
			break;
	}
	return(active);
}
