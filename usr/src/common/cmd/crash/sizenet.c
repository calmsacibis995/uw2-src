/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/sizenet.c	1.1"
#ident	"$Header: sizenet.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function size.  The RFS and
 * Streams tables and sizes are listed here to allow growth and not
 * overrun the compiler symbol table.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/stream.h>
#include <sys/strsubr.h>
#include <sys/list.h>

#ifdef notdef
#include <sys/rf_messg.h>
#include <sys/rf_comm.h>
#include <sys/nserve.h>
#include <sys/rf_cirmgr.h> 
#include <sys/rf_adv.h>
#endif

#include "crash.h"


struct sizenetable {
	char *name;
	char *symbol;
	unsigned size;
};
struct sizenetable sizntab[] = {
	"datab","dblock",sizeof (struct datab),
	"dblk","dblock",sizeof (struct datab),
	"dblock","dblock",sizeof (struct datab),
	"linkblk","linkblk",sizeof (struct linkblk),
	"mblk","mblock",sizeof (struct msgb),
	"mblock","mblock",sizeof (struct msgb),
	"msgb","mblock",sizeof (struct msgb),
	"queue","queue",sizeof (struct queue),
	"stdata","streams",sizeof (struct stdata),
	"streams","streams",sizeof (struct stdata),
	NULL,NULL,NULL
};	


/* get size from size table */
unsigned
getsizenetab(name)
char *name;
{
	unsigned size = 0;
	struct sizenetable *st;

	for(st = sizntab; st->name; st++) {
		if(!(strcmp(st->name,name))) {
			size = st->size;
			break;
		}
	}
	return(size);
}

/* print size */
int
prsizenet(name)
char *name;
{
	struct sizenetable *st;
	int i;

	if(!strcmp("",name)) {
		for(st = sizntab,i = 0; st->name; st++,i++) {
			if(!(i & 3))
				fprintf(fp,"\n");
			fprintf(fp,"%-15s",st->name);
		}
		fprintf(fp,"\n");
	}
}

/* get symbol name and size */
int
getnetsym(name,symbol,size)
char *name;
char *symbol;
unsigned *size;
{
	struct sizenetable *st;

	for(st = sizntab; st->name; st++) 
		if(!(strcmp(st->name,name))) {
			strcpy(symbol,st->symbol);
			*size = st->size;
			break;
		}
}
