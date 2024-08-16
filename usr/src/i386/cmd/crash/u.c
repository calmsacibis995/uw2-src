/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/u.c	1.2.1.10"

/*
 * This file contains code for the crash functions:  user, pcb, stack,
 * trace, and kfp.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/immu.h>
#include <sys/tss.h>
#include <sys/seg.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/signal.h>
#include <sys/cred.h>
#include "crash.h"
#include <sys/vmparam.h>
#include <sys/ioctl.h>
#include <sys/sysi86.h>
#include <sys/lwp.h>
#include <sys/resource.h>

#define DECR	4
#define UBADDR UVUBLK
#define UREG(x) ((long*)(ubp->u_stack))[((long)(&ubp->u_ar0[x]) - UBADDR)/sizeof(long)]
long tmp1;
extern int debugmode;
vaddr_t  KVUBLK;

#define min(a,b) (a>b ? b:a)

#define	DATE_FMT	"%a %b %e %H:%M:%S %Y\n"
/*
 *	%a	abbreviated weekday name
 *	%b	abbreviated month name
 *	%e	day of month
 *	%H	hour
 *	%M	minute
 *	%S	second
 *	%Y	year
 */

extern struct user *ubp;		/* ublock pointer */
extern struct rlimits *rlimitbp;	/* rlimit pointer */
extern int active;			/* active system flag */
struct proc procbuf;			/* proc entry buffer */
static unsigned long Kfp = 0;		/* kernel frame pointer */
static char	time_buf[50];		/* holds date and time string */
extern	char	*strtbl ;		/* pointer to string table */
unsigned long *stk_bptr;		/* stack pointer */
extern	struct	syment	*File,
	*Vnode, *Panic, *V;	/* namelist symbol pointers */
extern struct	syment	*findsym();
extern char *malloc();
void free();
unsigned long *temp;
extern struct syment *U;

char *rlimitstring[] = {
	"cpu time",
	"file size",
	"data size",
	"stack size",
	"coredump size",
	"file descriptors",
	"address space"
};

extern char * ublock;

/* read ublock into buffer */

int
getublock(slot,lwp_num)
int slot;
int lwp_num;
{
	return _getublock(slot, lwp_num, ublock);
}

/* find the address of the user structure for the
 * proc "slot" and lwp "lwp_num" 
 */

vaddr_t
get_uvalue(slot,lwp_num)
{
	int 	i,cnt;
	struct proc *procp;
	struct proc procbuf;
	struct lwp* lwpp;
	long length;
	lwp_t lwp;

	if (slot == (-1)) {
		slot= Cur_proc;
		lwp_num = Cur_lwp;
	}

	if (slot >= vbuf.v_proc || slot < 0) {
		prerrmes("slot %d out of range\n",slot);
		return(-1);
	}

	procp = slot_to_proc(slot);

	if (procp == NULL) {
		prerrmes("%d is not a valid process\n",slot);
		return(-1);
	}

	readmem((unsigned long)procp,1,-1,(char *)&procbuf,sizeof procbuf,
		"proc structure");
	if (procbuf.p_nlwp == NULL) {
		prerrmes("%d is a zombie process\n",slot);
		return(-1);
	}

	/* get address from the lwp number */
	if ((lwpp = slot_to_lwp(lwp_num,&procbuf)) == NULL ) {
		prerrmes("lwp %d non existant in process %d\n", lwp_num,slot);
		return(-1);
	}

	/* examine sysdump and U-Block was swapped-out */
	else if(!(procbuf.p_flag & P_LOAD)) {
		prerrmes("process %d was swapped-out\n", slot);
		return(-1);
	} else {
		readmem(lwpp,1,-1,&lwp,sizeof(lwp_t), "lwp structure");
	}

	return ( (vaddr_t) lwp.l_up );
}

/* find the address of the uarea for this context */

vaddr_t
get_kvublk(slot,lwp)
{
	vaddr_t uaddr,p;
	int page;

	if( slot < 0 ) {

		uaddr = (vaddr_t) get_uvalue(Cur_proc,Cur_lwp);
	} else 
		uaddr = (vaddr_t) get_uvalue(slot,lwp);
	
	return(UAREA_TO_UBLOCK(uaddr));
}

int
_getublock(slot, lwp_num, buf)
int slot;
int lwp_num;
char * buf;

{
	int 	i,cnt;
	struct proc *procp;
	struct proc procbuf;
	struct lwp* lwpp;
	struct user* ubuf;
	vaddr_t uaddr;
	long length;
	lwp_t lwp;

	if( debugmode > 1 )

		fprintf(stderr,"_getublk: %x %x %x %x\n",slot, lwp_num,buf);

	/* slot (Cur_proc) of (-1) means refer to the current virtual ublock */

	if(slot == -1) {	/* use the current virtual ublock */

		if (debugmode > 1)
			fprintf(stderr,"_getublock: read current virtual ublock\
				l.userp is %x ublock is %x\n", l.userp, ublock);
		read_ublock(l.userp,ublock);
		return(0);
	}

	if(slot >= vbuf.v_proc || slot < 0) {
		prerrmes("slot %d out of range\n",slot);
		return(-1);
	}

	/* read in the ublock pages */

	uaddr = get_uvalue(slot,lwp_num);
	if (uaddr == -1)
		return -1;

	read_ublock(uaddr, buf);

        readmem((long)ubp->u_rlimits,1,-1,(char *)rlimitbp,
                sizeof(struct rlimits),"rlimits structure");

	return(0);
   }

read_ublock(uaddr,addr)
void * uaddr;
void * addr;
{
	vaddr_t p;
	ulong_t page;
	int err;

	p = UAREA_TO_UBLOCK(uaddr);
	err = readmem(p, 1, -1, addr, USIZE*MMU_PAGESIZE,
			"current virtual UBLK");
	if (err == -1) {
		printf("cannot read u-block\n");
		longjmp(syn, 0);
	}
}

/* allocate buffer for stack */
unsigned
setbf(top, bottom, slot)
unsigned long top;
unsigned long bottom;
int slot;
{
	unsigned range;
	char *bptr;
	long remainder;
	long nbyte;
	unsigned long paddr;


	if (bottom > top) 
		error("Top of stack value less than bottom of stack\n");
	range = (unsigned)(top - bottom);
	if((stk_bptr = (unsigned long *)malloc(range)) == NULL)
		error("Insufficient memory available for stack buffering.\n");
	
	bottom = top - range;
	bptr = (char *)stk_bptr;
	do {
		remainder = ((bottom + MMU_PAGESIZE) & ~((long)MMU_PAGESIZE -1)) - bottom;
		nbyte = min(remainder, top-bottom);
		if((paddr = cr_vtop(bottom,slot)) == -1) {
			free((char *)stk_bptr);
			stk_bptr = NULL;
			error("The stack lower bound, %x, is an invalid address\nThe saved stack frame pointer is %x\n",bottom,top);
		}
		readmem(paddr,0,0,bptr,nbyte,"stack");
		bptr += nbyte;
		bottom += nbyte;
	} while (bottom < top);
	return(range);
}

/* get arguments for user function */
int
getuser()
{
	int slot = Cur_proc;
	int full = 0;
	int all = 0;
	int lwp = 0;
	long arg1 = -1;
	long arg2 = -1;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"efw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(args[optind]) {
		arg1 = stol(args[optind]);

		if(args[++optind])
			arg2 = stol(args[optind]);
	}

	if( arg1 != (-1) ) 
		slot=arg1;
	if( arg2 != (-1) ) 
		lwp=arg2;

	pruser(full,slot,lwp);
}

/* print ublock */
int
pruser(full,slot,lwp)
int full,slot,lwp;
{
	register  int  i,j;
	unsigned offset;
	char buf[80];
	char *s;

	if(slot == -1)
		slot = getcurproc();

	if(getublock(slot,lwp) == -1)
		return;

	fprintf(fp,"User Structure for Process Slot %d Lwp Slot %d\n",
				proc_to_slot(ubp->u_procp),lwp);

	fprintf(fp,"\nu_acflag: %s%s\n",
		ubp->u_acflag & AFORK ? "fork" : "exec",
		ubp->u_acflag & ASU ? " su-user" : "");

	switch(ubp->u_aslock_stat) {
		case NOT_AS_LOCKED:
		s = "NOT_AS_LOCKED";
		break;

		case AS_READ_LOCKED:
		s= "AS_READ_LOCKED";
		break;

		case AS_WRITE_LOCKED:
		s= "AS_WRITE_LOCKED";
		break;

		default:
		sprintf(buf,"undefined value: %x\n",ubp->u_aslock_stat);
		s = buf;
		break;
	}
	fprintf(fp,"u_aslock_stat = %s\n",s);

	fprintf(fp,"u_syscall = %d with arguments ( %x %x %x %x %x %x )\n",
		ubp->u_syscall, ubp->u_arg[0], ubp->u_arg[1], ubp->u_arg[2],
		ubp->u_arg[3], ubp->u_arg[4], ubp->u_arg[5] );

	fprintf(fp, "u_sigflag = %x\n", ubp->u_sigflag);
	fprintf(fp, "u_stkbase = %x\n", ubp->u_stkbase);
	fprintf(fp, "u_stksize = %x\n", ubp->u_stksize);
	
	fprintf(fp,"u_ior = %x %x\t", ubp->u_ior.dl_lop, ubp->u_ior.dl_hop);
	fprintf(fp,"u_iow = %x %x\t", ubp->u_iow.dl_lop, ubp->u_iow.dl_hop);
	fprintf(fp,"u_ioch = %x %x\n", ubp->u_ioch.dl_lop, ubp->u_ioch.dl_hop);

	fprintf(fp, "u_procp = %x\n", ubp->u_procp);
	fprintf(fp, "u_lwpp = %x\n", ubp->u_lwpp);
	
	fprintf(fp, "u_privatedatap = %x\n", ubp->u_privatedatap);
	
	fprintf(fp, "u_kse_ptep = %x\n", ubp->u_kse_ptep);

#ifdef DEBUG
	fprintf(fp, "u_debugflags = %x\n", ubp->u_debugflags);
#endif

	fprintf(fp,"RESOURCE LIMITS:\n");
	for (i = 0; i < RLIM_NLIMITS; i++) {
		if (rlimitstring[i] == 0)
			continue;
		fprintf(fp,"\t%s: ", rlimitstring[i]);
		if (rlimitbp->rl_limits[i].rlim_cur == RLIM_INFINITY)
			fprintf(fp,"unlimited/");
		else
			fprintf(fp,"%d/", rlimitbp->rl_limits[i].rlim_cur);
		if (rlimitbp->rl_limits[i].rlim_max == RLIM_INFINITY)
			fprintf(fp,"unlimited\n");
		else
			fprintf(fp,"%d\n", rlimitbp->rl_limits[i].rlim_max);
	}
	fprintf(fp,"\n");
}

/* get arguments for pcb function */
int
getpcb()
{
	int proc = Cur_proc;
	int phys = 0;
	char type = 'n';
	long addr = -1;
	int c;
	struct syment *sp;

	optind = 1;
	while((c = getopt(argcnt,args,"iukpw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'i' :	type = 'i';
					break;
			case 'u' :	type = 'u';
					break;
			case 'k' :	type = 'k';
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(type == 'i') {
		if(!args[optind])
			longjmp(syn,0);
		if(*args[optind] == '(')  {
			if((addr = eval(++args[optind])) == -1)
				error("\n");
		}
		else if(sp = symsrch(args[optind])) 
			addr = (unsigned long)sp->n_value;
		else if(isasymbol(args[optind]))
			error("%s not found in symbol table\n",args[optind]);
		else if((addr = strcon(args[optind],'h')) == -1)
				error("\n");
		pripcb(phys,addr);
		}
	else {
		if(args[optind]) {
			if((proc = strcon(args[optind],'d')) == -1)
				error("\n");
			if((proc > vbuf.v_proc) || (proc < 0))
				error("%d out of range\n",proc);
			prpcb(proc,type);
		}
		else prpcb(proc,type);
	}
}


/* print user, kernel, or active pcb */
/* The kernel pcb is the 386 task state segment (pointed to by u.u_tss)
   There is no user pcb in the 386. The user task state is saved on the
   kernel stack, instead. This area is taken as "user pcb". */
   
int prpcb(proc,type)
int proc;
char type;
{

#ifdef notdef
	struct tss386 tss;
	struct syment *sym;

	if(getublock(proc) == -1)
		return;
	switch(type) {
		case 'n' :
		case 'k' :
			if (active && ((proc== -1) || onproc(proc)))
				error ("This is current process on active system\n");
			readmem(ubp->u_tss,1,proc,&tss,sizeof(tss),"TSS");
			test_tss(tss.t_esp0,UVUBLK+KSTKSZ,"esp0");
			test_tss(tss.t_esp1,UVUBLK+KSTKSZ,"esp1");
			test_tss(tss.t_esp2,UVUBLK+KSTKSZ,"esp2");
			test_tss(tss.t_ss0,KDSSEL,"ss0");
			test_tss(tss.t_ss1,KDSSEL,"ss1");
			test_tss(tss.t_ss2,KDSSEL,"ss2");
			if(!(sym = symsrch("kpd0"))) 
				error("kpd0 not found in symbol table\n");
			else if (tss.t_cr3 != ((sym->n_value-KVSBASE)|0x80000000))
				test_tss(tss.t_cr3,sym->n_value-KVSBASE,"cr3");
			test_tss(tss.t_ldt,LDTSEL,"ldt");
			printreg(tss.t_eax,tss.t_ebx,tss.t_ecx,tss.t_edx,
				 tss.t_esp,tss.t_ebp,tss.t_esi,tss.t_edi,
				 tss.t_eflags,tss.t_eip,tss.t_cs ,tss.t_ss,
				 tss.t_ds ,tss.t_es ,tss.t_fs ,tss.t_gs);
			break;
		case 'u' :
			if (procbuf.p_flag & SSYS)
				error ("This is a system process\n");
			/* u_ar0 points to location in kernel stack */
			fprintf(fp,"ERR=%d, TRAPNO=%d\n",
				UREG(ERR),UREG(TRAPNO));
			printreg(UREG(EAX),UREG(EBX), UREG(ECX), UREG(EDX),
				UREG(UESP),UREG(EBP), UREG(ESI), UREG(EDI),
				UREG(EFL), UREG(EIP), UREG( CS), UREG( SS),
				UREG( DS), UREG( ES), UREG( FS), UREG( GS));
			break;
		default  : longjmp(syn,0);
			   break;
	}
#endif
}

test_tss(actual,expected,regname)
unsigned long int actual,expected;
char *regname;
{
	if (actual != expected) fprintf(fp,
		"Field u_t%s in tss has strange value %08x, expected %08x\n",
		regname,actual,expected);
}

printreg(eax,ebx,ecx,edx,esp,ebp,esi,edi,efl,eip,cs,ss,ds,es,fs,gs)
unsigned int eax,ebx,ecx,edx,esp,ebp,esi,edi,efl,eip,cs,ss,ds,es,fs,gs;
{
	fprintf(fp,"cs:eip=%04x:%08x Flags=%03x\n",cs&0xffff,eip,efl&0x3ffff);
	fprintf(fp,
		"ds = %04x   es = %04x   fs = %04x   gs = %04x",
		ds&0xffff,es&0xffff,fs&0xffff,gs&0xffff);
	if (ss != -1) fprintf(fp,"   ss = %04x",ss&0xffff);
	fprintf(fp,"\nesi= %08x   edi= %08x   ebp= %08x   esp= %08x\n",
		esi,edi,ebp,esp);
	fprintf(fp,"eax= %08x   ebx= %08x   ecx= %08x   edx= %08x\n",
		eax,ebx,ecx,edx);
}


/* print interrupt pcb */
int
pripcb(phys,addr)
int phys;
unsigned long addr;
{
/*
	struct tss386 tss;
	readmem(addr,phys,-1,&tss,sizeof(tss),"TSS");
	fprintf(fp,"ss:esp [0] = %04x:%08x\n",tss.t_ss0,tss.t_esp0);
	fprintf(fp,"ss:esp [1] = %04x:%08x\n",tss.t_ss1,tss.t_esp1);
	fprintf(fp,"ss:esp [2] = %04x:%08x\n",tss.t_ss2,tss.t_esp2);
	fprintf(fp,"cr3 = %08x\n",tss.t_cr3);
	fprintf(fp,"ldt =     %04x\n",tss.t_ldt);
	printreg(tss.t_eax,tss.t_ebx,tss.t_ecx,tss.t_edx,
		 tss.t_esp,tss.t_ebp,tss.t_esi,tss.t_edi,
		 tss.t_eflags,tss.t_eip,tss.t_cs ,tss.t_ss,
		 tss.t_ds ,tss.t_es ,tss.t_fs ,tss.t_gs);
*/
	int	regs[19];
	readmem(addr,phys,-1,regs,sizeof(regs),"Register Set");
	printf("ERR=%d, TRAPNO=%d\n",
		regs[ERR],regs[TRAPNO]);
	printreg(regs[EAX],regs[EBX], regs[ECX], regs[EDX],
		regs[ESP], regs[EBP], regs[ESI], regs[EDI],
		regs[EFL], regs[EIP], regs[ CS], -1,
		regs[ DS], regs[ ES], regs[ FS], regs[ GS]);
}

/* get arguments for stack function */
int
getstack()
{
	int proc = Cur_proc;
	int phys = 0;
	int virt=0;
	char type = 'k';
	long addr = -1;
	int c;
	int sym=0;
	int lwp_num=Cur_lwp;
	struct syment *sp;

	optind = 1;
	while((c = getopt(argcnt,args,"iukpvsw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'i' :	type = 'i';
					break;
			case 'u' :	type = 'u';
					break;
			case 'k' :	type = 'k';
					break;
			case 's' :	sym=1;
					break;
			case 'v' :	virt=1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(type == 'i') {
		if(!args[optind])
			longjmp(syn,0);
		if(*args[optind] == '(') {
			if((addr = eval(++args[optind])) == -1)
				error("\n");
		}
		else if(sp = symsrch(args[optind])) 
			addr = sp->n_value;
		else if(isasymbol(args[optind]))
			error("%s not found in symbol table\n",args[optind]);
			else if((addr = strcon(args[optind],'h')) == -1)
				error("\n");
		pristk(phys,addr);
	} else {
		if(args[optind]) {
			if((proc = strcon(args[optind],'d')) == -1)
				error("\n");
			if((proc > vbuf.v_proc) || (proc < 0))
				error("%d out of range\n",proc);
		}
		optind++;

		if(args[optind]) {
			if((lwp_num = strcon(args[optind],'d')) == -1)
				error("\n");
		} 
		if(type == 'u')
			prustk(proc,lwp_num);
		else { 
			if( virt )
				prkstk(-1,lwp_num,sym);
			else
				prkstk(proc,lwp_num,sym);
		}
	}
}

/* print kernel stack */
int
prkstk(proc,lwp,sym)
int proc;
{

	int panicstr;
	unsigned long stkfp,stklo,stkhi;

	vaddr_t  kvublk = get_kvublk(proc,lwp);	/* address of user_t */

	if( proc == (-1) ) {
		printf("KERNEL STACK (CURRENT PROCESS)\n");
	} else
		printf("KERNEL STACK  PROCSLOT: %d LWP: %d\n",proc,lwp);

	if(getublock(proc,lwp) == -1)
		return;

	if (active && ((proc== -1) || onproc(proc)))
		error ("This is current process on active system\n");

	if( proc == (-1 ) ) {
		stklo = (unsigned long) kvublk;
		stkhi = (unsigned long) get_uvalue(proc,lwp);
		stkfp = stkhi;
	} else {
		stklo = ubp->u_kcontext.kctx_esp;
		stkfp = ubp->u_kcontext.kctx_ebp;
		stkhi = (unsigned long) get_uvalue(proc,lwp);
	}

	if ((stklo > stkhi ) || (stklo < kvublk ))
		error("kernel stack not valid\n");

	if( sym ) {
		prks_sym(stkfp,stklo,stkhi,proc);
	} else {
		printf("PRKSTACK(%x %x %x %x\n",stkfp,stklo,stkhi,proc);
		prkstack(stkfp,stklo,stkhi,proc);
	}
}

/* print user stack */
int
prustk(proc)
int proc;
{

#ifdef notdef
	int	panicstr;
	unsigned long	stkfp,stklo,stkhi ;
	if(getublock(proc) == -1)
		return;
	if (procbuf.p_flag & SSYS)
		error ("This is a system process\n");
	stkfp = UREG(EBP);
	stklo = UREG(UESP);
	stkhi = UVSTACK+sizeof(int);
	if ((stklo>stkhi) || (stkfp>stkhi)) error("user registers corrupted\n");
	prstack(stkfp,stklo,stkhi,proc);
#endif

}


/* print interrupt stack */
int
pristk(phys,addr)
int phys;
unsigned long addr;
{
	error("The iAPX386 has no interrupt stack\n");
}

/* dump stack */
int
prstack(stkfp,stklo,stkhi,slot)
unsigned long stkfp,stklo,stkhi; 
int slot;
{
	unsigned dmpcnt;
	unsigned long *stkptr;
	int prcnt;

	fprintf(fp,"FP: %x\n",stkfp);
	fprintf(fp,"LOWER BOUND: %x\n",stklo) ;
	
	if ( stkfp < stklo)
		error("upper bound < lower bound, unable to process stack\n") ;
	dmpcnt = setbf(stkhi, stklo, slot);
	stklo = stkhi - dmpcnt ;
	stkptr = (unsigned long *)(stk_bptr);

	prcnt = 0;
	for(; dmpcnt != 0; stkptr++, dmpcnt -= DECR)
	{
		if((prcnt++ % 4) == 0){
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)stk_bptr)+stklo));
		}
		fprintf(fp,"  %8.8x", *stkptr);
	}
	free((char *)stk_bptr);
	stk_bptr = NULL;

	fprintf(fp,"\n\n");
}

/* dump kernel stack symbolically */
int
prks_sym(stkfp,stklo,stkhi,slot)
unsigned long stkfp,stklo,stkhi;
int slot;
{
	unsigned dmpcnt;
	unsigned long *stkptr;
	int prcnt;
	proc_t *procp;
	extern struct syment *Stext, *Etext;
	extern char * db_sym_off();

	fprintf(fp,"FP: %x\n",stkfp);
	fprintf(fp,"LOWER BOUND: %x\n",stklo) ;
	
	if ( stkfp < stklo)
		error("upper bound < lower bound, unable to process stack\n") ;


	if(active) {
#ifdef notdef
		stk_bptr = (unsigned long *)malloc(MMU_PAGESIZE*MAXU_PAGES);
		procp = slot_to_proc(slot);
		if (procp == NULL) {
			prerrmes("%d is not a valid process\n",slot);
			return(-1);
		}
		readmem((long)procp, 1, slot, (char *)&procbuf, sizeof procbuf,
			"process table");
		sysi86(RDUBLK, slot_to_pid(slot), (char *)stk_bptr, 
					MMU_PAGESIZE*MAXU_PAGES);

		stkptr = (unsigned long *)((long)stk_bptr +(stklo - UBADDR));
		dmpcnt = stkhi - stklo;
		temp = stkptr;
#endif
	} else {
		dmpcnt = setbf(stkhi, stklo, slot);
		stklo = stkhi - dmpcnt;
		stkptr = (unsigned long *)(stk_bptr);
	}
	prcnt = 0;
	for(; dmpcnt != 0; stkptr++, dmpcnt -= DECR)
	{
		if((prcnt++ % 4) == 0){
		if(active)
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)temp)+stklo));
		else
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)stk_bptr)+stklo));
		}
		
		if( (*stkptr > Stext->n_value) && (*stkptr < Etext->n_value) )
			fprintf(fp,"  %s",db_sym_off(*stkptr,MMU_PAGESIZE));
		else
			fprintf(fp,"  %8.8x", *stkptr); 
	}
	fprintf(fp,"\n\n");

	free((char *)stk_bptr); 
	stk_bptr = NULL;
}

/* dump stack */
int
prkstack(stkfp,stklo,stkhi,slot)
unsigned long stkfp,stklo,stkhi;
int slot;
{
	unsigned dmpcnt;
	unsigned long *stkptr, addr;
	struct syment *syme;
	char * symb;
	int prcnt;
	proc_t *procp;

	fprintf(fp,"FP: %x\n",stkfp);
	fprintf(fp,"LOWER BOUND: %x\n",stklo) ;
	
	if ( stkfp < stklo)
		error("upper bound < lower bound, unable to process stack\n") ;
	

	if(active) {
#ifdef notdef
		stk_bptr = (unsigned long *)malloc(MMU_PAGESIZE*MAXU_PAGES);
		procp = slot_to_proc(slot);
		if (procp == NULL) {
			prerrmes("%d is not a valid process\n",slot);
			return(-1);
		}
		readmem((long)procp, 1, slot, (char *)&procbuf, sizeof procbuf,
			"process table");
		sysi86(RDUBLK, slot_to_pid(slot), (char *)stk_bptr, 
					MMU_PAGESIZE*MAXU_PAGES);

		stkptr = (unsigned long *)((long)stk_bptr +(stklo - UBADDR));
		dmpcnt = stkhi - stklo;
		temp = stkptr;
#endif
	} else {
		dmpcnt = setbf(stkhi, stklo, slot);
		stklo = stkhi - dmpcnt;
		stkptr = (unsigned long *)(stk_bptr);
	}
	prcnt = 0;
	for(; dmpcnt != 0; stkptr++, dmpcnt -= DECR)
	{
		if((prcnt++ % 4) == 0){
		if(active)
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)temp)+stklo));
		else
			fprintf(fp,"\n%8.8x: ",
				(int)(((long)stkptr - (long)stk_bptr)+stklo));
		}
		addr = *stkptr;
		if ((syme = findsym(addr)) != 0)
			symb = (char *) syme->n_offset;
		if ((*stkptr != 0) && (syme != 0) && ((addr - (long)syme->n_value) == 0))
			fprintf(fp," %8.8s", (char *) syme->n_offset);
		else
			fprintf(fp," %8.8x", *stkptr);
	}

	free((char *)stk_bptr); 
	stk_bptr = NULL;

	fprintf(fp,"\n\n");
}

/* get arguments for trace function */
int
gettrace()
{
	int proc = Cur_proc;
	int lwp  = Cur_lwp;
	int phys = 0;
	int all = 0;
	int kfpset = 0;
	char type = 'k';
	int unknown=0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = 0;
	int c;
	unsigned lastproc;
	struct syment *sp;

	optind = 1;
	while((c = getopt(argcnt,args,"xierpw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'e' :	all = 1;
					break;
			case 'r' :	kfpset = 1;
					break;
			case 'i' :	type = 'i';
					break;
			case 'x' :	unknown++;
					break;

			default  :	longjmp(syn,0);
		}
	}
	if(type == 'i') {
		if(!args[optind])
			longjmp(syn,0);
		if(*args[optind] == '(') {
			if((addr = eval(++args[optind])) == -1)
				error("\n");
		}
		else if(sp = symsrch(args[optind])) 
			addr = sp->n_value;
		else if(isasymbol(args[optind]))
			error("%s not found in symbol table\n",args[optind]);
			else if((addr = strcon(args[optind],'h')) == -1)
				error("\n");
		pritrace(phys,addr,kfpset,proc);
	}
	else {
/*horror*/
		if(args[optind]) {
			if(args[optind]) {
				arg1 = stol(args[optind]);
		
				if (args[++optind])
					arg2 = stol(args[optind]);
			}
			prktrace(arg1,arg2,kfpset,unknown);
			proc = arg1 = -1;
			arg2 =0;

		} else if(all) {
			for (proc =0; proc < vbuf.v_proc; proc++) 
				prktrace(proc,lwp,kfpset,unknown);
		} else
			prktrace(proc,lwp,kfpset,unknown);
	}
}

/* print kernel trace */
int
prktrace(proc,lwp,kfpset,unknown)
int proc,kfpset,lwp,unknown;
{
	unsigned long	stklo, stkhi, pcbaddr;
	unsigned long	savefp,savesp,saveap,savepc;
	struct syment *symsrch();
	unsigned range;
	struct pcb *ptr;
	proc_t pr;
	int crash_prf();
	static int first_trace=1;
	vaddr_t  uaddr;
	lwp_t *lwpp;

	if (proc == (-1)) {
		fprintf(fp,"STACK TRACE FOR KERNEL IDLE PROCESS\n");
	} else {
		fprintf(fp,"STACK TRACE FOR PROCESS %d LWP %d:\n",proc,lwp);
	}

	KVUBLK= get_kvublk(proc,lwp);
	uaddr = (vaddr_t)UBLOCK_TO_UAREA(KVUBLK);

	if (active && ((proc == -1) || onproc(proc))) {
		prerrmes("This is current process on active system\n");
		return;
	}
	if (first_trace) { 
		st_init(); 
		first_trace = 0;
	}

	read_ublock(uaddr, ublock);

	savesp =  ubp->u_kcontext.kctx_esp;
	savefp =  ubp->u_kcontext.kctx_ebp;
	savepc =  ubp->u_kcontext.kctx_eip;
	saveap = 0; 

	if ((savesp<KVUBLK) || (savesp>uaddr) ) {
		printf("ublock = %x, uaddr = %x, savesp = %x\n",
			KVUBLK, uaddr, savesp);
		printf("ublock = %x ubp = %x u_kc = %x\n",
			ublock, ubp, &ubp->u_kcontext);

		prerrmes("kernel stack not valid\n");
		return;
	}

	stkhi = uaddr;	/* top of stack is begining of _u */
	stklo = KVUBLK;

	if (stkhi < stklo) {
		prerrmes("upper bound < lower bound\n");
		return;
	}
	stk_bptr = (unsigned long *)ublock;
	if (kfpset) {
		if (Kfp)
			savefp = Kfp;
		else {
			prerrmes("stack frame pointer not saved\n");
			return;
		}
	}


	if (ubp->u_procp != NULL) {
		readmem(ubp->u_procp, 1, -1, (char *)&pr, sizeof(struct proc),
			"proc structure");
		lwpp = slot_to_lwp(lwp, &pr);
	} else
		lwpp = NULL;

	if (unknown) {
		printf("stacktrace( %x %x %x %x %x %x %x %x)\n",
		crash_prf, KVUBLK, Kfp, UNKNOWN, UNKNOWN, 0, 0, 0);

		stacktrace(crash_prf,KVUBLK,Kfp, UNKNOWN, UNKNOWN, 0, 0, 0,
				lwpp);
	} else {
		stacktrace(crash_prf, KVUBLK, savesp, savefp, savepc, 0, 0, 0,
				lwpp);
	}
	stk_bptr = NULL;
}

/* print interrupt trace */
int
pritrace(phys,addr,kfpset,proc)
int phys,kfpset,proc;
unsigned long addr;
{
	error("The iAPX386 has no interrupt stack\n");
}

invalkfp() { error("Invalid kfp\n"); }


crash_prf(fmt,a,b,c,d,e,f)
{
	fprintf(fp,(char*)fmt,a,b,c,d,e,f);
	return(0);
}

prfuncname(addr)
unsigned long addr;
{
	struct syment *func_nm;

	if (addr == 0 || (func_nm = findsym(addr)) == 0)
		fprintf(fp, " %8.8x", addr);
	else {
		fprintf(fp, " %-8.8s", (char *) func_nm->n_offset);
	}
}

/* get arguments for kfp function */
int
getkfp()
{
	int c;
	struct syment *sp;
	int reset = 0;
	int proc = Cur_proc;
	long value;

	optind = 1;
	while((c = getopt(argcnt,args,"w:s:r")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			case 'r' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		if(*args[optind] == '(') {
			if ((value = eval(++args[optind])) == -1)
				error("\n");
			prkfp(value,proc,reset);
		}
		else if (sp = symsrch(args[optind])) 
			prkfp(sp->n_value,proc,reset);
		else {
			if ((value = strcon(args[optind],'h')) == -1)
				error("\n");
			prkfp(value,proc,reset);
		}
	}
	else prkfp(-1,proc,reset);
}

/* print kfp */
int
prkfp(value,proc,reset,lwp)
long value;
int proc,reset;
{

	if(value != -1)
		Kfp = value;
	else if(reset) {
		if(getublock(proc,lwp) == -1)
			return;
		if (active && ((proc== -1) || onproc(proc)))
			error ("This is current process on active system\n");
		Kfp = ubp->u_kcontext.kctx_ebp;
	}
	fprintf(fp,"Kfp = %08x\n",Kfp);
}
