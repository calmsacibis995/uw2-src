/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/size.c	1.1"
#ident	"$Header: size.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  size, findslot, and
 * findaddr.  The size table for RFS and Streams structures is located in
 * sizenet.c
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/buf.h>

#ifdef notdef
#include <sys/callo.h>
#endif

#include <sys/conf.h>
#include <sys/fstyp.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/flock.h>
#include <sys/immu.h>
#include <vm/vm_hat.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/proc.h>
#include <sys/termios.h>
#include <sys/stream.h>
#include <sys/strtty.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fs/s5inode.h>
#include <sys/fs/snode.h>
#include <sys/fs/fifonode.h>
#include <sys/procfs.h>
#include <sys/uio.h>
#include <fs/procfs/prdata.h>
#include "crash.h"

struct sizetable {
	char *name;
	char *symbol;
	unsigned size;
};

struct sizetable siztab[] = {
	"buf","buf",sizeof (struct buf),
#ifdef notdef
	"callo","callout",sizeof (struct callo),
	"callout","callout",sizeof (struct callo),
#endif
	"file","file",sizeof (struct file),
#ifdef notdef
	"flckinfo","flckinfo",sizeof (struct flckinfo),
#endif
	"fifonode","fifonode",sizeof (struct fifonode),
	"filock","flox",sizeof (struct filock),
	"flox","flox",sizeof (struct filock),
	"inode","inode",sizeof (struct inode),
	"pp","pp",sizeof (struct page),
#ifdef notdef
	"prnode","prnode",sizeof (struct prnode),
#endif
	"proc","proc",sizeof (struct proc),
	"snode","snode",sizeof (struct snode),
	"tty","tty",sizeof (struct strtty),	
	"vfs","vfs",sizeof (struct vfs),	
	"vfssw","vfssw",sizeof (struct vfssw),	
	"vnode","vnode",sizeof (struct vnode),	
	NULL,NULL,NULL
};	


/* get size from size tables */
unsigned
getsizetab(name)
char *name;
{
	unsigned size = 0;
	struct sizetable *st;
	extern unsigned getsizenetab();

	for(st = siztab; st->name; st++) {
		if(!(strcmp(st->name,name))) {
			size = st->size;
			break;
		}
	if(!size)
		size = getsizenetab(name);
	}
	return(size);
}

/* get arguments for size function */
int
getsize()
{
	int c;
	char *all = "";
	int hex = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"xw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'x' : 	hex = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do{
			prtsize(args[optind++],hex);
		}while(args[optind]);
	}
	else prtsize(all,hex);
}

/* print size */
int
prtsize(name,hex)
char *name;
int hex;
{
	unsigned size;
	struct sizetable *st;
	int i;

	if(!strcmp("",name)) {
		for(st = siztab,i = 0; st->name; st++,i++) {
			if(!(i & 3))
				fprintf(fp,"\n");
			fprintf(fp,"%-15s",st->name);
		}
		prsizenet(name);
	}
	else {
		size = getsizetab(name);
		if(size) {
			if(hex)
				fprintf(fp,"0x%x\n",size);
			else fprintf(fp,"%d\n",size);
		}
		else error("%s does not match in sizetable\n",name);
	}
}
	

/* get arguments for findaddr function */
int
getfindaddr()
{
	int c;
	int slot;
	char *name;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		name = args[optind++];
		if(args[optind]) {
			if((slot = (int)strcon(args[optind],'d')) == -1)
				error("\n");
			prfindaddr(name,slot);
		}
		else longjmp(syn,0);
	}
	else longjmp(syn,0);
}

/* print address */
int
prfindaddr(name,slot)
char *name;
int slot;
{
	unsigned size = 0;
	struct syment *sp;
	struct sizetable *st;
	char symbol[10];

	symbol[0] = '\0';
	for(st = siztab; st->name; st++) 
		if(!(strcmp(st->name,name))) {
			strcpy(symbol,st->symbol);
			size = st->size;
			break;
		}
	if(symbol[0] == '\0') 
		getnetsym(name,symbol,&size);
	if(symbol[0] == '\0')
		error("no match for %s in sizetable\n",name);
	if(!(sp = symsrch(symbol)))
		error("no match for %s in symbol table\n",name);
	fprintf(fp,"%8x\n",sp->n_value + size * slot);
}

/* get arguments for findslot function */
int
getfindslot()
{
	int c;
	long addr;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do{
			if((addr = strcon(args[optind++],'h')) == -1)
				continue;
			prfindslot(addr);
		}while(args[optind]);
	}
	else longjmp(syn,0);
}

/* print table and slot */
int
prfindslot(addr)
long addr;
{
	struct syment *sp;
	int slot,offset;
	unsigned size;
	extern char *strtbl;
	extern short N_DATA,N_BSS;
	extern struct syment *findsym();
	char *name;

	if(!(sp = findsym((unsigned long)addr)))
		error("no symbol match for %8x\n",addr);
	name = (char *) sp->n_offset;
	size = getsizetab(name);
	if(!size)
		error("%s does not match in sizetable\n",name);
	slot = (addr - sp->n_value)/size;
	offset = (addr - sp->n_value)%size;
	fprintf(fp,"%s",name);
	fprintf(fp,", slot %d, offset %d\n",slot,offset);
}
