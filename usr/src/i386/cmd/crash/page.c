/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/page.c	1.2"
#ident "$Header: page.c 1.2 91/09/04 $"

/*
 * This file contains code for the crash functions: page, as, and ptbl.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sysmacros.h>
#include <sys/fs/s5dir.h>
#include <sys/tss.h>
#include <sys/immu.h>
#include <sys/vnode.h>
#include <sys/vmparam.h>
#include <vm/vm_hat.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/proc.h>
#include "crash.h"


/* namelist symbol pointers */
extern struct syment *V;		/* ptr to var structure */

extern u_long vtop();
extern proc_t *slot_to_proc();

extern pte_t *kpd_start;

void prsegs();

#define MAX_PFN	0xFFFFF

/*
 * On pagepool algorithms see "mem/vm_page.c"
 */
static uint_t num_chunks;
static struct pp_chunk  *pagepool;	/* Pagepool chunks	*/
static struct pp_chunk *pp_first;	/* First chunk in list	*/

static int
load_pagepool()
{
	struct syment    *Pagepool, *N_pp_chunk;
	struct pp_chunk **chunkpp;
	int i;
	vaddr_t addr;

	if (!(N_pp_chunk = symsrch("n_pp_chunk")))
		error("n_pp_chunk not found in symbol table\n");

	readmem((long)N_pp_chunk->n_value, 1, -1, (char *)&num_chunks,
				sizeof(u_int));

	if (debugmode > 0)
		fprintf(fp, "number of pagepool chunks = %d\n", num_chunks);

	if (!(Pagepool = symsrch("pagepool")))
		error("pagepool not found in symbol table\n");

	pagepool = (struct pp_chunk *)malloc(num_chunks * 
						sizeof(struct pp_chunk));
	if (pagepool == NULL)
		error("malloc for pagepool failed\n");

	if (debugmode > 0)
		fprintf(fp, "pagepool pointer = %lx\n", Pagepool->n_value);

	/* get the pointer value */
	readmem(Pagepool->n_value, 1, -1, (char *)&addr, sizeof(vaddr_t),
		"page pool pointer");

	readmem(addr, 1, -1, pagepool, num_chunks * sizeof(struct pp_chunk),
		"page pool table");

	/*
	 * Setup the ordered list
	 */
	for (i = 0; i < num_chunks; i++){
		if (pagepool[i].pp_epage == NULL && 
			pagepool[i].pp_page  == NULL)
			break;

		chunkpp = &pp_first;
		while (*chunkpp != NULL) {
			if ((*chunkpp)->pp_epage - (*chunkpp)->pp_page >
			   pagepool[i].pp_epage - pagepool[i].pp_page)
					break;

			chunkpp = &(*chunkpp)->pp_next;
		}
		pagepool[i].pp_next = *chunkpp;
		*chunkpp = &pagepool[i];
	}

	if(debugmode > 0)
		dump_pagepool();

	return(0);
}

static int
dump_pagepool()
{
	struct pp_chunk *chp;
	int i;

	fprintf(fp,"PAGEPOOL	   addr      pfn     epfn     page    epage     next\n");
	for(i=0, chp = pagepool; i < num_chunks; i++, chp++){
		fprintf(fp,"Pagepool[%2d]:%c %08x %8x %8x %08x %08x %08x\n",
			i,chp == pp_first ? '*' : ' ',chp,
			chp->pp_pfn, chp->pp_epfn,
			chp->pp_page,chp->pp_epage,
			chp->pp_next);
	}

	return(0);
}

u_int
page_pptonum(pp)
	page_t	*pp;
{
	struct pp_chunk *chp;

	if(pp_first == NULL && load_pagepool() < 0)
		return((u_int)-1);

	chp = pp_first;

	while (pp < chp->pp_page || pp >= chp->pp_epage) {
		if (num_chunks == 1 || ((chp = chp->pp_next) == NULL))
			return((u_int)-1);
	}

	return((u_int)((pp-chp->pp_page)*(PAGESIZE/MMU_PAGESIZE))+chp->pp_pfn);
}

page_t *
page_numtopp(pfn)
	u_int	pfn;
{
	struct pp_chunk *chp;

	if(pp_first == NULL && load_pagepool() < 0)
		return((page_t *)NULL);

	chp = pp_first;

	while (pfn < chp->pp_pfn || pfn >= chp->pp_epfn) {
		if (num_chunks == 1 || ((chp = chp->pp_next) == NULL))
			return((page_t *)NULL);
	}

	return(&chp->pp_page[(pfn - chp->pp_pfn) / (PAGESIZE/MMU_PAGESIZE)]);
}

/* get arguments for page function */
int
getpage()
{
	u_int pfn = (u_int)-1;
	u_int all = 0;
	u_int phys = 0;
	u_long addr = (ulong_t)-1;
	u_long arg1 = (ulong_t)-1;
	u_long arg2 = (ulong_t)-1;
	int c;

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

	if(args[optind]) {
		all = 1;
		do {
			getargs((int)MAX_PFN+1,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(pfn = arg1; pfn <= (u_int)arg2; pfn++)
					prpage(all,pfn,phys,addr);
			else {
				if(arg1 >= 0 && arg1 <= MAX_PFN)
					pfn = arg1;
				else
					addr = arg1;
				prpage(all,pfn,phys,addr);
			}
			pfn = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	} else
		for(pfn = 0; pfn <= MAX_PFN; pfn++)
			prpage(all,pfn,phys,addr);
}

/* print page structure table */
int
prpage(all,pfn,phys,addr)
u_int all,pfn,phys;
u_long addr;
{
	struct page pagebuf;
	u_int next, prev;
	u_int vpnext, vpprev;
	u_int hash;

	if (!Virtmode)
		phys = 1;
	if (addr == (u_long)-1) {
		if ((addr = (u_long)page_numtopp(pfn)) == 0)
			return;
		phys = 0;
	} else if (pfn == (u_int)-1 && !phys)
		pfn = page_pptonum((page_t *)addr);

	readmem(addr,!phys,-1,
		(char *)&pagebuf,sizeof pagebuf,"page structure table");

	/* check page flags */
	if ((*((ushort *)&pagebuf) == 0) && !all)
		return;

	fprintf(fp,"pfn = %05x  ", pfn);
	fprintf(fp,"pp = %08x", addr);

	fprintf(fp,"  vnode =  0x%08x  offset = 0x%08x\n",
		pagebuf.p_vnode, pagebuf.p_offset);

	fprintf(fp,"hash pointer = 0x%08x ", pagebuf.p_hash);
	fprintf(fp," vpprev = 0x%08x ", pagebuf.p_vpprev);
	fprintf(fp," vpnext = 0x%08x  \n", pagebuf.p_next);
	fprintf(fp," p_next = 0x%08x  p_prev = 0x%08x\n", pagebuf.p_next,
							  pagebuf.p_prev);
	fprintf(fp," p_activecnt = 0x%08x p_timestamp = 0x%08x\n",
			pagebuf.p_activecnt, pagebuf.p_timestamp); 
	fprintf(fp, " p_uselock = 0x%08x, p_mapping = 0x%08x\n",
			pagebuf.p_uselock, pagebuf.p_mapping);

	fprintf(fp,"%s%s%s%s%s\n",
		pagebuf.p_free    ? "FREE "    : "not-free ",
		pagebuf.p_pageout ? "PAGEOUT " : "no-pageout ",
		pagebuf.p_mod     ? "MOD "     : "not-dirty ",
		pagebuf.p_invalid ? "INVALID " : "valid ",
		pagebuf.p_physdma ? "DMAable " : "not-DMAable ");

	fprintf(fp, "nio = %d p_type = %d p_chidx = %d\n", pagebuf.p_nio,
	            pagebuf.p_type, pagebuf.p_chidx);

	fprintf(fp, "\n");
}

/* get arguments for ptbl function */
int
getptbl()
{
	int proc = Cur_proc;
	int all = 0;
	int phys = 0;
	u_long addr = (ulong_t)-1;
	int c;
	struct proc prbuf;
	struct as asbuf;
	int count = 1;

	optind = 1;
	while((c = getopt(argcnt,args,"epw:s:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 's' :	proc = setproc();
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		if ((addr = (u_long)strcon(args[optind++],'h')) == (u_long)-1)
			error("\n");
		if (args[optind]) 
			if ((count = strcon(args[optind],'d')) == -1)
				error("\n");
		prptbl(phys, addr, count, (u_long)-1, proc);
	} else {
	  	if (!all) {
			procntry(proc, &prbuf);
			if (prbuf.p_as == 0)
				error("invalid address space for proc %d\n",
					proc);
			readmem((long)prbuf.p_as, 1, -1, (char *)&asbuf, 
				sizeof asbuf, "as");
			prptbls(all, proc, prbuf.p_as, &asbuf.a_hat);
		} else { /* print all procs */
			for (proc = 0; proc < vbuf.v_proc; proc++) {
				if (slot_to_proc(proc) == 0)
					continue;
				procntry(proc, &prbuf);
				if (prbuf.p_as == 0) {
					fprintf(fp, 
						"\ninvalid address space for proc %d\n",
						proc);
					continue;
				}
				readmem((long)prbuf.p_as, 1, -1,
					(char *)&asbuf, sizeof asbuf, "as");
				prptbls(all, proc, prbuf.p_as, &asbuf.a_hat);
			}
		}
	}
}

/* print all of a proc's page tables */
int
prptbls(all,proc,as,hatp)
int all;
u_int proc;
struct as *as;
struct hat *hatp;
{
	u_long	ptapaddr;
	hatpt_t	ptapbuf;
	u_long	pt_addr, base;

	fprintf(fp, "Page Tables for Process %d AS %lx\n", proc, as);

	if ((ptapaddr = (u_long)hatp->hat_pts) == 0)
		return;

	do {
		readmem(ptapaddr, 1, proc, (char *)&ptapbuf, sizeof(ptapbuf),
			"hatpt structure");

		fprintf(fp,
		"\nHATP 0x%08x: addr 0x%08x pde 0x%08x aec %d locks %d\n\n",
			ptapaddr,
			base = (ptapbuf.hatpt_pdtep - kpd_start) << PTNUMSHFT,
			ptapbuf.hatpt_pde.pg_pte,
			ptapbuf.hatpt_aec,
			ptapbuf.hatpt_locks);

		if (ptapbuf.hatpt_as != as) {
			fprintf(fp, "WARNING - hatpt was not pointing to the correct as struct: 0x%8x\n",
				ptapbuf.hatpt_as);
			fprintf(fp, "          hatpt list traversal aborted.\n");
			break;
		}

		/* locate page table */
		pt_addr = pfntophys(ptapbuf.hatpt_pde.pgm.pg_pfn);
		prptbl(1, pt_addr, NPGPT, base, proc);
	} while ((ptapaddr = (u_long)ptapbuf.hatpt_forw) != (u_long)hatp->hat_pts);
}

/* print page table */
static int
prptbl(phys,addr,count,base,proc)
int phys;
u_int count,proc;
u_long addr,base;
{
	pte_t	ptebuf;
	u_int	i;

	if (count > NPGPT)
		count = NPGPT;

	fprintf(fp, "Page Table Entries\n");
	if (base != (u_long)-1)
		fprintf(fp, "SLOT     VADDR    PFN   FLAGS\n");
	else
		fprintf(fp, "SLOT    PFN   FLAGS\n");

	for (i = 0; i < count; i++) {
		readmem(addr,!phys,proc,(char *)&ptebuf,sizeof(ptebuf),
			"page table");
		addr += sizeof(ptebuf);
		if (ptebuf.pgm.pg_pfn == 0)
			continue;
		fprintf(fp, "%4u", i);
		if (base != (u_long)-1)
			fprintf(fp, "  %08x", base + pfntophys(i));
		fprintf(fp, " %6x   %s%s%s%s%s\n",
			ptebuf.pgm.pg_pfn,
			ptebuf.pgm.pg_ref   ? "ref "   : "",	
			ptebuf.pgm.pg_rw    ? "w "     : "",	
			ptebuf.pgm.pg_us    ? "us "    : "",	
			ptebuf.pgm.pg_mod   ? "mod "   : "",	
			ptebuf.pgm.pg_v     ? "v "     : "");	
	}
}

/* get arguments for as function */
int
getas()
{
	struct var varbuf;
	int slot = -1;
	int proc = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	u_long addr = (ulong_t)-1;
	u_long arg1 = (ulong_t)-1;
	u_long arg2 = (ulong_t)-1;
	int c;
	char *heading = "PROC  SEGS       SEGLAST           SIZE         RSS   LOCKED_RSS          WSS    WHENAGED    HAT_PTS\n";

	optind = 1;
	while((c = getopt(argcnt,args,"efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}

	if (!full)
		fprintf(fp,"%s",heading);

	if (args[optind]) {
		do {
			getargs(vbuf.v_proc,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for (proc = arg1; proc <= arg2; proc++)
					pras(all,full,proc,phys,addr,heading);
			else {
				if (arg1 >= 0 && arg1 < vbuf.v_proc)
					proc = arg1;
				else
					addr = arg1;
				pras(all,full,proc,phys,addr,heading);
			}
			addr = proc = arg1 = arg2 = -1;
		} while(args[++optind]);
	} else if (all) {
		for (proc = 0; proc < vbuf.v_proc; proc++) 
			pras(all,full,proc,phys,addr,heading);
	} else
		pras(all,full,Cur_proc,phys,addr,heading);
}


/* print address space structure */
int
pras(all,full,slot,phys,addr,heading)
int all,full,slot,phys,addr;
char *heading;
{
	struct proc prbuf, *procaddr;
	struct as asbuf;

	if(addr == -1)
		procaddr = slot_to_proc(slot);
	else
		procaddr = (struct proc *) addr;

	if (procaddr) {
		readmem((long)procaddr,1, -1,(char *)&prbuf,sizeof prbuf,
		    "proc table");
	} else {
		return;
	}

	if (full)
		fprintf(fp,"\n%s",heading);

	fprintf(fp, "%4d  ", slot);

	if (prbuf.p_as == NULL) {
		fprintf(fp, "- no address space.\n");
		return;
	}

	readmem((long)(prbuf.p_as),1,-1,
		(char *)&asbuf,sizeof asbuf,"as structure");

	fprintf(fp,"0x%08x 0x%08x  0x%08x   %9d    %9d    %9d  0x%08x 0x%08x  \n",
		asbuf.a_segs,
		asbuf.a_seglast,
		asbuf.a_size,
		asbuf.a_rss,
		asbuf.a_lockedrss,
		asbuf.a_wss,
		asbuf.a_whenaged,
		asbuf.a_hat.hat_pts);

	if (full) { 
		prsegs(prbuf.p_as, (struct as *)&asbuf, phys);
	}
}


/* print list of seg structures */
void
prsegs(as, asbuf, phys)
	struct as *as, *asbuf;
	u_long phys;
{
	struct seg *seg, *sseg;
	struct seg  segbuf;
	struct syment *sp;
	extern char * strtbl;
	extern struct syment *findsym();

	sseg = seg = asbuf->a_segs;

	if (seg == NULL)
		return;

	fprintf(fp, "      BASE       SIZE        NEXT       PREV          OPS        DATA\n");

	do {
		readmem(seg, 1, -1, (char *)&segbuf, sizeof segbuf,
			"seg structure");
		fprintf(fp, "0x%08x 0x%08x  0x%08x 0x%08x ",
			segbuf.s_base,
			segbuf.s_size,
			segbuf.s_next,
			segbuf.s_prev);

		/* Try to find a symbolic name for the sops vector. If
		 * can't find one print the hex address.
		 */
		sp = findsym((unsigned long)segbuf.s_ops);
		if ((!sp) || ((unsigned long)segbuf.s_ops != sp->n_value))
			fprintf(fp,"0x%08x  ", segbuf.s_ops);
		else
			fprintf(fp, "%12.12s  ", (char *)sp->n_offset);

		fprintf(fp,"0x%08x\n", segbuf.s_data);

		if (segbuf.s_as != as) {
			fprintf(fp, "WARNING - seg was not pointing to the correct as struct: 0x%8x\n",
				segbuf.s_as);
			fprintf(fp, "          seg list traversal aborted.\n");
			return;
		}
	} while((seg = segbuf.s_next) != sseg);
}
