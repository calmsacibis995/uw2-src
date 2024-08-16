/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/init.c	1.1.1.9"
#ident "$Header: init.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash initialization.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <memory.h>
#include <sys/fs/s5dir.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/immu.h>
#include <sys/var.h>
#include <sys/ioctl.h>
#include <sys/sysi86.h>
#include <sys/plocal.h>
#include <sys/vmparam.h>
#include <sys/user.h>
#include <sys/resource.h>
#include "crash.h"

int	nmlst_tstamp ,		/* timestamps for namelist and cdumpfiles */
	dmp_tstamp ;

int Cur_eng = (-1);		/* Engine whose "Eyes" we see the world thru */
int Cur_lwp = (-1);		/* currnet lwp slot number */

#ifdef __STDC__
struct syment dummyent = { (long)0, (long)0, (long)0, (short)0,
				(unsigned short)0, (char)0, (char)0 };
#else
struct syment dummyent;
#endif

extern char *strtbl ;		/* pointer to string table in symtab.c */
extern char *dumpfile;		
extern char *namelist;

extern int active;		/* flag for active system */
extern long cr_vtop();
extern long lseek();
extern char *malloc();
struct syment *sp;		/* pointer to symbol table */
struct user *ubp;		/* pointer to ublock buffer */
struct rlimits *rlimitbp;	/* pointer to rlimit buffer */
char * ublock;			/* pointer to whole user area user+stack */
struct syment *L;		/* pointer to local structure */
struct plocal * getcureng();	
int Nengine=0;

/* namelist symbol pointers */
extern struct syment *Vnode, *Vfs, *V, *Vfssw, *Curproc;	
struct syment *Snode, *Sndd, *Rcvd, *Nrcvd, *Ngrps;
struct syment *Spec_vnodeops, *Fifo_vnodeops, *Prvnodeops;

struct syment *Trap, *Nmi, *Systrap, *Sigclean, *Stext, *Etext,*Evt_process;
struct syment *Panic_data;
vaddr_t *Structret_funcs;

struct plocal l;

/* initialize buffers, symbols, and global variables for crash session */
int
init(rwflag)
int rwflag;
{
	int offset ;
	struct syment	*ts_symb = NULL;
	extern void sigint();
	
	/* open dump file, if error print */
	if((mem = open(dumpfile, rwflag)) < 0)
		fatal("cannot open dump file %s\n",dumpfile);
	/*
	 * Set a flag if the dumpfile is of an active system.
	 */
	if(strcmp(dumpfile,"/dev/mem") == 0)
		active = 1;


#ifndef __STDC__
	memset(&dummyent,0,sizeof(dummyent));
#endif
	rdsymtab();			/* open and read the symbol table */

	if(!(V = symsrch("v")))
		fatal("var structure not found in symbol table\n");
	if(!(Vfs = symsrch("rootvfs")))
		fatal("vfs not found in symbol table\n");
	if(!(Vfssw = symsrch("vfssw")))
		fatal("vfssw not found in symbol table\n");

	if(!(Trap = symsrch("trap")))
		fatal("trap not found in symbol table\n");
	if(!(Nmi = symsrch("nmi")))
		fatal("nmi not found in symbol table\n");
	if(!(Systrap = symsrch("systrap")))
		fatal("systrap not found in symbol table\n");

	if(!(Sigclean = symsrch("sigclean")))
		fatal("sigclean not found in symbol table\n");

	if(!(L = symsrch("l")))
               	fatal("plocal structure (l) not found in symbol table\n");

	if(!(Stext = symsrch("stext"))) {
		if(!(Stext = symsrch("_stext"))) {
               		fatal("cant find start of text\n");
		}
	}

	if(!(Etext = symsrch("etext"))) {
		if(!(Etext = symsrch("_etext"))) {
               		fatal("cant find end of text\n");
		}
	}

	if(!(Evt_process = symsrch("evt_process"))){
               	fprintf(fp,"cant find symbol evt_process\n");
	}
	

	if(!(Panic_data = symsrch("panic_data")))
		fatal("panic_data not found in symbol table\n");

	if(!(Snode = symsrch("spectable")))
		error("snode table not found\n");

	if(!(Spec_vnodeops = symsrch("spec_vnodeops")))
		error("spec_vnodeops not found\n");
	if(!(Fifo_vnodeops = symsrch("fifo_vnodeops")))
		error("fifo_vnodeops not found\n");
	if(!(Prvnodeops = symsrch("prvnodeops")))
		Prvnodeops = &dummyent;
	if(!(Ngrps = symsrch("ngroups_max")))
		error("ngroups_max not found in symbol table\n");
	if(!active)
		findmemregs();

	readmem((long)V->n_value,1,-1,(char *)&vbuf,
		sizeof vbuf,"var structure");

	/* Allocate ublock buffer */
	ublock = (char*)malloc((unsigned)(USIZE*MMU_PAGESIZE));
	/*ubp = (user_t *) (ublock+ (int)((int)(U->n_value) -(int)KVUBLK));*/
	ubp = UBLOCK_TO_UAREA(ublock);

	/* Allocate rlimits structure */
	rlimitbp= (struct rlimits *)malloc(sizeof(struct rlimits));

	make_engmap();	/* call before get_context() */
	get_context();	/* local info for current processor */
	pr_context();

	/* setup break signal handling */
	if (signal(SIGINT,sigint) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
}
