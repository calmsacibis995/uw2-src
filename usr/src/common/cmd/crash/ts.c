/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/ts.c	1.2"
#ident	"$Header: ts.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  tslwp, tsdptbl
 */

#include <a.out.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ts.h>
#include "crash.h"



struct syment *Tshash, *Tsdptbl, *Tsmaxumdpri;	/* namelist symbol */
tsdpent_t *tsdptbl;
struct tshash *tshp, *thashp, *tshphead;
struct tshash *tshashpbuf;
struct tslwp *tslwpp;
struct tslwp tslwpbuf;

/* get arguments for tslwp function */
int
gettslwp()
{
	int i,c;

	char *tshashhdg = " TMLFT CPUPRI UPRILIM UPRI UMDPRI NICE FLAGS DISPWAIT   LWPP    LSTATP		LPRIP		LFLAGP		SLPWAIT	NEXT     	PREV\n";

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}


	if(!Tshash)
		if(!(Tshash = symsrch("tshashp"))) 
			error("tshashp not found in symbol table\n");

	/* Get the address of the head of the hash table for TS class */
	readmem((long)Tshash->n_value, 1, -1, (char *)&thashp,
		sizeof(tshash_t *), "tshashp table address");

	tshashpbuf = (tshash_t *)malloc(TSHASHSZ * sizeof(struct tshash));

	/* Read in the heads of all the hash buckets */
	readmem((long)thashp, 1, -1, (char *)tshashpbuf,
		TSHASHSZ * sizeof(struct tshash), "tshashp table");

	fprintf(fp, "%s", tshashhdg);

	/* Loop through all the hash buckets */
	for (i=0; i < TSHASHSZ; i++) {
		/* Loop through all entries on the hash chain */
		tshp = &tshashpbuf[i];
		tshphead = &thashp[i]; /* Remember where we started. */
		tslwpp = (tslwp_t *)tshp->th_first;
		while (tslwpp != (tslwp_t *)tshphead) {
			readmem((long)tslwpp, 1, -1, (char *)&tslwpbuf,
				sizeof tslwpbuf, "tslwp_t struc");
			prtslwp();
			tslwpp = (tslwp_t *)tslwpbuf.ts_flink;
		}
	}
	free(tshashpbuf);
}




/* print the time sharing lwp table */
int
prtslwp()
{


	fprintf(fp, "  %4d    %2d      %2d    %2d    %2d    %2d   %#x    %2d   %.8x	%.8x 	%.8x 	%.8x 	%4d 	%.8x 	%.8x\n",
	tslwpbuf.ts_timeleft, tslwpbuf.ts_cpupri, tslwpbuf.ts_uprilim,
		tslwpbuf.ts_upri, tslwpbuf.ts_umdpri, tslwpbuf.ts_nice,
		tslwpbuf.ts_flags, tslwpbuf.ts_dispwait, tslwpbuf.ts_lwpp,
		tslwpbuf.ts_lstatp, tslwpbuf.ts_lprip, tslwpbuf.ts_lflagp,
		tslwpbuf.ts_sleepwait,
		tslwpbuf.ts_flink, tslwpbuf.ts_rlink);
}


/* get arguments for tsdptbl function */

int
gettsdptbl()
{
	int slot = -1;
	int all = 0;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	long addr = -1;
	short tsmaxumdpri;

	char *tsdptblhdg = "SLOT     GLOBAL PRIO     TIME QUANTUM     TQEXP     SLPRET     MAXWAIT     LWAIT\n\n";

	if(!Tsdptbl)
		if(!(Tsdptbl=symsrch("ts_dptbl")))
			error("ts_dptbl not found in symbol table\n");

	if(!Tsmaxumdpri)
		if(!(Tsmaxumdpri=symsrch("ts_maxumdpri")))
			error("ts_maxumdpri not found in symbol table\n");

	optind=1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	readmem((long)Tsmaxumdpri->n_value, 1, -1, (char *)&tsmaxumdpri, sizeof(short),
		"ts_maxumdpri");

	tsdptbl = (tsdpent_t *)malloc((tsmaxumdpri + 1) * sizeof(tsdpent_t));

	readmem((long)Tsdptbl->n_value, 1, -1, (char *)tsdptbl,
	    (tsmaxumdpri + 1) * sizeof(tsdpent_t), "ts_dptbl");

	fprintf(fp,"%s",tsdptblhdg);


	if(args[optind]) {
		all = 1;
		do {
			getargs(tsmaxumdpri + 1,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prtsdptbl(slot);
			else {
				if(arg1 >= 0 && arg1 < tsmaxumdpri + 1)
					prtsdptbl(slot);
				else
					fprintf(fp,"invalid time-sharing scheduler parameter table slot: %d\n",arg1);
			}
			slot = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else 
		for(slot = 0; slot < tsmaxumdpri + 1; slot++)
			prtsdptbl(slot);

	free(tsdptbl);
}

/* print the time sharing dispatcher parameter table */
int
prtsdptbl(slot)
int  slot;
{
	fprintf(fp,"%3d         %4d         %10ld        %3d       %3d        %5d       %3d\n",
	    slot, tsdptbl[slot].ts_globpri, tsdptbl[slot].ts_quantum,
	    tsdptbl[slot].ts_tqexp, tsdptbl[slot].ts_slpret,
	    tsdptbl[slot].ts_maxwait, tsdptbl[slot].ts_lwait);
}
