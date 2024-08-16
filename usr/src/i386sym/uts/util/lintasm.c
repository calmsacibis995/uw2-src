/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:util/lintasm.c	1.25"
#ident	"$Header: $"

/*
 * Provide C-declarations for asm-declared procedures/data, for lint.
 */

#include <util/debug.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <util/dl.h>
#include <util/cmn_err.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <util/list.h>
#include <util/bitmasks.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <proc/user.h>
#include <proc/priocntl.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <svc/memory.h>
#include <svc/systm.h>

extern int bin_alloc[];
#ifdef DEBUG
extern void print_kernel_addrs(void);
#endif

extern void misc_refs(void);
extern void init_mmu(void);
extern void sysinit(void);
extern void trap(int, uint, uint, uint, uint, uint, uint, uint, uint, uint, uint, uint, uint, uint);

static void lint_ref(int, ...);

char *initclass;

/*
 * Make references to variables and functions from assembly or
 * conditionally configured context.
 */

static void
refstuff(int i)
{
	extern void sys_init(int, int, void (**)(), int *);
	char *cp = "";
	size_t	j = 0;
	dl_t	dl = { 0 };
	ls_elt_t lold = { 0 };
	ls_elt_t lnew = { 0 };
	lock_t	ref_lock;
	void unlock(lock_t *, pl_t);
	void (*fn)();
	int foo;
	extern lock_t eng_tbl_mutex;
	extern paddr_t online_kl1pt;
	page_t *pp = NULL;

	cmn_err(CE_NOTE, "reference to var %d\n", j);
	cmn_err(CE_NOTE, "reference to var %d\n", i);
	cmn_err(CE_NOTE, "reference to var %ld\n", rootdev);
	cmn_err(CE_NOTE, "reference to var %d\n", totalmem);
	cmn_err(CE_NOTE, "reference to var %ld\n", online_kl1pt);
	sysinit();
#ifdef DEBUG
	print_kernel_addrs();
#endif
	page_rdonly(pp);
	selfinit(0);
	lint_ref(0, boothowto);
	lint_ref(0, nonline);
	(void)getchar();
	putchar(' ');
	(void)clearnmi();
	swtch((lwp_t *)0);
	(*softvec[0])();
	(void)nodev();
	(void)nulldev();
	(void)getudev();
	sys_init(0, 0, &fn, &foo);
	(void)dispdeq((lwp_t *)0);
	(void)getcid(cp, (id_t *)&foo);
	(void)LOCK(&eng_tbl_mutex, PLHI);
	(void)strncpy("", "", 0);
	(void)strncmp("", "", 0);
	(void)bcmp("", "", 0);
	(void)stoi(&cp);
	unlock((lock_t *)0, (pl_t)0);
	numtos((ulong)0, "");
	init_mmu();
	trap(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	(void)assfail("", "", 0);
	(void)ldivide(dl, dl);
	(void)lmul(dl, dl);
	(void)spl1();
	(void)spl4();
	(void)spl5();
	(void)spl6();
	(void)spl7();
	(void)spltty();
	ls_ins_before(&lold, &lnew);
	ls_ins_after(&lold, &lnew);
	(void)ls_remque(&lold);
	ls_remove(&lold);
	misc_refs();
	bin_alloc[0] = 0;
	int_bin_table[0].bh_size = 0;
	LOCK_INIT(&ref_lock, 0, 0, (lkinfo_t *)0, 0);
	LOCK_DEINIT(&ref_lock);
	(void)strcpy("", "");
	(void)strcat("", "");
	(void)strncat("", "", (size_t)0);
	(void)strcmp("", "");
	(void)strlen("");
	(void)min(0, 0);
	(void)max(0, 0);
	loadldt((ushort)0);
	bzero("", 0);
	struct_zero("", 0);
	(void)__priocntl();
	(void)__priocntlset();
	yield();
	mrg_getparm(PARM_OFFSET_VM86P, &cp);
	initclass = "foobar";
}

/*
 * Procedure to reference an integer, to satisfy lint.
 */

static void
lint_ref(int i, ...) { refstuff(i); }

/*ARGSUSED*/
void bcopy(const void *from, void *to, size_t count) { }

/*ARGSUSED*/
void bzero(void *base, size_t length) { }

/*ARGSUSED*/
void struct_zero(void *base, size_t length) { }

/*ARGSUSED*/
char *strcpy(char *a, const char *b) { return(a); }

/*ARGSUSED*/
char *strcat(char *a, const char *b) { return(a); }

/*ARGSUSED*/
char *strncat(char *a, const char *b, size_t n) { return(a); }

/*ARGSUSED*/
int strcmp(const char *a, const char *b) { return(0); }

/*ARGSUSED*/
int strlen(const char *a) { return(0); }

/*ARGSUSED*/
int min(uint i, uint j) { return(0); }

/*ARGSUSED*/
int max(uint i, uint j) { return(0); }

/*ARGSUSED*/
void loadldt(ushort s) { }

void bin0int(void) { }
void bin1int(void) { }
void bin2int(void) { }
void bin3int(void) { }
void bin4int(void) { }
void bin5int(void) { }
void bin6int(void) { }
void bin7int(void) { }

void enable_nmi(void) { }
void init_fpu(void) { }
dl_t ladd(dl_t a, dl_t b) { a = b; return (a); }
dl_t lshiftl(dl_t a, int i) { lint_ref(i); return (a); }
int  lsign(dl_t a) { (void)lshiftl(a, 0); return (0); }
dl_t lsub(dl_t a, dl_t b) { a = b; return (a); }
void setup_seg_regs(void) { }
pl_t spl0(void)   { return (0); }
pl_t spl1(void)   { return (0); }
pl_t spl4(void)   { return (0); }
pl_t spl5(void)   { return (0); }
pl_t spl6(void)   { return (0); }
pl_t spl7(void)   { return (0); }
pl_t spltty(void) { return (0); }
pl_t splhi(void)  { return (0); }
void splx(pl_t s) { lint_ref(s); }
void t_nmi(void) { }
void t_diverr(void) { }
void t_dbg(void) { }
void t_int3(void) { }
void t_into(void) { }
void t_check(void) { }
void t_und(void) { }
void t_dna(void) { }
void t_syserr(void) { }
void t_extovr(void) { }
void t_badtss(void) { }
void t_notpres(void) { }
void t_stkflt(void) { }
void t_gpflt(void) { }
void t_pgflt(void) { }
void t_coperr(void) { }
void t_alignflt(void) { }
void t_mceflt(void) { }
/*ARGSUSED*/
void setup_newcontext(dupflags_t dupflags, boolean_t is_sysproc, struct user
		      *userp, void (*funcp)(void *), void *argp)
{}
/*ARGSUSED*/
void resume(struct lwp *p, struct lwp *q) { }
/*ARGSUSED*/
void use_private(struct lwp *p, void (*func)(struct lwp *), struct lwp *a) {extern int nclass; nclass = 0; return ; }
/*ARGSUSED*/
int save(struct lwp *p) { return(0); }
/*ARGSUSED*/
void reginit(struct user *u, void (*fn)()) {
}
/*ARGSUSED*/
long __priocntl() { return 0;}
/*ARGSUSED*/
long __priocntlset() { return 0;}
/*ARGSUSED*/
int kmem_avail(size_t size) {return 0;}


char	etext[1];
int	upyet;
struct user *upointer;
struct user ueng;
struct plocal l;
uint_t myengnum;
