/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/search.c	1.2"
#ident	"$Header: search.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function search.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/immu.h"
#include "crash.h"

#define min(a,b) (a>b? b:a)
#define pageround(x) ((x + MMU_PAGESIZE) & ~(MMU_PAGESIZE -1))
#define BUFSIZE (MMU_PAGESIZE/sizeof(int))

/* get arguments for search function */
int
getsearch()
{
	long mask = 0xffffffff;
	int phys = 0;
	int proc = Cur_proc;
	int c;
	unsigned long pat,start,len;
	struct syment *sp;

	optind = 1;
	while((c = getopt(argcnt,args,"ps:w:m:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 's' :	proc = setproc();
					break;
			case 'm' :	if((mask = strcon(optarg,'h')) == -1)
						error("\n");
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		pat = strcon(args[optind++],'h');
		if(pat == -1)
			error("\n");
		if(args[optind]) {
			if(*args[optind] == '(') {
				if((start = eval(++args[optind])) == -1)
					error("\n");
			}
			else if(sp = symsrch(args[optind]))
				start = (long)sp->n_value;
			else if((start = strcon(args[optind],'h')) == -1)
				error("%s not found in symbol table\n",
					args[optind]);
			if(args[++optind]) {
				if((len = strcon(args[optind++],'h')) == -1)
					error("\n");
				prsearch(mask,pat,start,len,phys,proc);
			}
			else longjmp(syn,0);
		}
		else longjmp(syn,0);
	} else 
		longjmp(syn,0); 
}

/* print results of search */
int
prsearch(mask,pat,start,len,phys,proc)
long mask;
unsigned long pat,start,len;
int phys,proc;
{
	unsigned long buf[BUFSIZE];
	int i;
	unsigned n;
	long remainder;

	fprintf(fp,"MASK = %x, PATTERN = %x, START = %x, LENGTH = %x\n\n",
		mask,pat,start,len);
	while(len > 0)  {
		remainder = pageround(start) - start;
		n = min(remainder,len); 
		readmem((long)start,!phys,proc,
			(char *)buf,n,"buffer");
		for(i = 0; i<n/sizeof (int); i++)  
			if((buf[i] & mask) == (pat & mask)) {
				fprintf(fp,"MATCH AT %8x: %8x\n",start+
					i*sizeof (int),buf[i]);
			}
		start +=n;
		len -=n;
	}
}
