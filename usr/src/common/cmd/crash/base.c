/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/base.c	1.1"
#ident	"$Header: base.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function base.
 */

#include "stdio.h"
#include "crash.h"

/* get arguments for function base */
int
getbase()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do {
			prnum(args[optind++]);
		}while(args[optind]);
	}
	else longjmp(syn,0);
}


/* print results of function base */
int
prnum(string)
char *string;
{
	int i;
	long num;

	if(*string == '(') 
		num = eval(++string);
	else num = strcon(string,NULL);
	if(num == -1)
		return;
	fprintf(fp,"hex: %x\n",num);
	fprintf(fp,"decimal: %d\n",num);
	fprintf(fp,"octal: %o\n",num);
	fprintf(fp,"binary: ");
	for(i=0;num >= 0 && i < 32;i++,num<<=1);
	for(;i<32;i++,num<<=1)
		num < 0 ? fprintf(fp,"1") : fprintf(fp,"0");
	fprintf(fp,"\n");
}
