/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/disp.c	1.2"

/*
 * This file contains code for the crash functions:  dispq
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/priocntl.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/engine.h>
#include "crash.h"

extern int Cur_eng;
extern engine_t * id_to_eng();

struct syment 	*Globalque;	/* symbol table entry for "global_rq" */
struct syment	*Nglobpris;	/* symbol table entry for "nglobpris" */
int		nglobpris;

runque_t	runquebuf;
runque_t	*global_rq;	
list_t		*dispqbuf;

STATIC const int localq_msk = 0x01;
STATIC const int globalq_msk = 0x02;

/* get arguments for dispq function */
int
getdispq()
{
	int slot = -1;
	int c, i;
	long arg1 = -1;
	long arg2 = -1;
	runque_t **rqpp, *rq;
	engine_t *eng;
	engine_t engine;
	int quemask = localq_msk | globalq_msk;

	char *dispqhdg = "SLOT     FLINK       RLINK\n\n";

	if(!Nglobpris && !(Nglobpris = symsrch("nglobpris")))
		error("nglobpris not found in symbol table\n");

	readmem((long)Nglobpris->n_value, 1, -1, (char *)&nglobpris,
		sizeof nglobpris, "nglobpris");

        if(!Globalque && !(Globalque = symsrch("global_rq")))
                error("global_rq not found in symbol table\n");

        /* Read the global runqueue address*/

        readmem((long)Globalque->n_value, 1, -1, (char *)&global_rq,
                sizeof(runque_t *), "global_rq address");

	/* Read in our engine_t structure in order to get our run queues */

	eng = id_to_eng(Cur_eng);
        readmem((unsigned long)eng,1,-1,(char *)&engine,sizeof(struct engine),
                                                 "engine structure");
	optind = 1;
	while((c = getopt(argcnt,args,"lgw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'l' :	quemask = localq_msk;
					break;
			case 'g' :	quemask = globalq_msk;
					break;
			default  :	longjmp(syn,0);
		}
	}

	/* Loop through all run queues */
	for (rqpp = engine.e_rqlist; (rq = *rqpp) != NULL; rqpp++) {

		if(rq == global_rq)
			/* If this is the Global Run Queue then say so */
			if(quemask & globalq_msk)
				fprintf(fp, "Global Run Queue:\n");
			else
				continue;
		else
			/* If this is a Local Run Queue then say so */
			if(quemask & localq_msk)
				fprintf(fp, "Local Run Queue:\n");
			else
				continue;

		/* Read in the runque_t structure */
		readmem((long)rq, 1, -1, (char *)&runquebuf,
	    		sizeof(runque_t), "runque struct");

		/* Allocate enough space to read in the entire table */
		/* of dispatch queue headers at once */

		dispqbuf = (list_t *)malloc(nglobpris * sizeof(list_t));
	
		/* Read in the entire table of dispq headers */

		readmem((long)runquebuf.rq_dispq, 1, -1, (char *)dispqbuf,
	    	nglobpris * sizeof(list_t), "dispq header table");

		fprintf(fp, "%s", dispqhdg);

		/* Print out all (or some) of the dispatch queue headers */
		if(args[optind]){
			do {
				getargs(nglobpris,&arg1,&arg2);
				if(arg1 == -1) 
					continue;
				if(arg2 != -1)
					for(slot = arg1; slot <= arg2; slot++)
						prdispq(slot);
				else {
					if(arg1 >=0 && arg1 < nglobpris)
						prdispq(arg1);
					else
						fprintf(fp,"invalid dispq slot: %d\n",arg1);
				}
				slot = arg1 = arg2 = -1;
			}while(args[++optind]);
		} else for(slot = 0; slot < nglobpris; slot++)
			prdispq(slot);

		fprintf(fp, "\n\n");
	}

	free(dispqbuf);
}

/* print dispq header table  */
int
prdispq(slot)
int slot;
{
	fprintf(fp, "%4d     %8x    %8x",
		slot, dispqbuf[slot].flink, dispqbuf[slot].rlink);

	if(dispqbuf[slot].flink != dispqbuf[slot].rlink)
		fprintf(fp, "***\n");
	else
		fprintf(fp, "\n");
}
