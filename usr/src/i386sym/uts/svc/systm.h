/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_SYSTM_H	/* wrapper symbol for kernel use */
#define _SVC_SYSTM_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:svc/systm.h	1.68"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Random set of variables and functions
 * used by more than one routine.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <svc/time.h>	/* REQUIRED */
#include <util/dl.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/dl.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

extern struct vnode *rootdir;	/* pointer to vnode of root directory */

extern int	rstchown;	/* 1 ==> restrictive chown(2) semantics */

extern dev_t	rootdev;	/* device used for the root filesystem */

extern int	cpurate;	/* cpu rate in Mhz */
extern int	lcpuspeed;	/* aprox. VAX MIPS (normalized to 100 Mhz) */
extern int	i486_lcpuspeed;	/* ditto for 25 Mhz 486 */
extern int	upyet;		/* non-zero when system is initialized */
extern int	console_slic_id;/* SLIC id of console */
extern int	console_board_type;	/* type of console hardware */
extern int	sys_clock_rate;	/* system clock rate */
extern struct ctlr_desc *slic_to_config[]; /* slic to cfg entry mapping */
extern char	etext[];	/* end of kernel text */
extern int	boothowto;	/* flags passed from boot procedure */

#ifdef __STDC__

extern int nodev(void);
extern int nulldev(void);
extern int getudev(void);
extern int stoi(char **);
extern void numtos(u_long, char *);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, size_t);
extern int strcmp(const char *, const char *);
extern int strlen(const char *);
extern int strncmp(const char *, const char *, size_t);
extern void ticks_to_timestruc(timestruc_t *, dl_t *);
extern int strcpy_len(char *, const char *);
extern int strcpy_max(char *, const char *, size_t);
extern int bcmp(const void *, const void *, size_t);

extern int ucopyin(const void *, void *, size_t, uint_t);
extern int copyin(const void *, void *, size_t);
extern int ucopyout(const void *, void *, size_t, uint_t);
extern int copyout(const void *, void *, size_t);
extern int fubyte(const char *);
extern int fushort(const ushort_t *, ushort_t *);
extern int fuword(const int *);
extern int subyte(char *, char);
extern int sushort(ushort_t *, ushort_t);
extern int suword(int *, int);
extern int copyinstr(const char *, char *, size_t, size_t *);
extern int copystr(const char *, char *, size_t, size_t *);
extern int uzero(void *, size_t);
extern int kzero(void *, size_t);

extern void bcopy(const void *, void *, size_t);
extern void ovbcopy(void *, void *, size_t);
extern void bzero(void *, size_t);
extern void struct_zero(void *, size_t);
extern void bscan(void *, size_t);

extern int setjmp(label_t *);
extern void longjmp(label_t *);

extern int arglistsz(vaddr_t, int *, size_t *, int);
extern int copyarglist(int, vaddr_t, int, vaddr_t, vaddr_t, boolean_t);

extern pl_t spl0(void);
extern pl_t spl1(void);
extern pl_t spl4(void);
extern pl_t spl5(void);
extern pl_t spl6(void);
extern pl_t spl7(void);
extern void splblockall(void);
extern pl_t splhi(void);
extern pl_t spldisk(void);
extern pl_t splstr(void);
extern pl_t spltimeout(void);
extern pl_t spltty(void);
extern pl_t splxcall(void);
extern void splx(pl_t);

extern void call_demon(void);
extern int hwpage_exists(int);
extern void loadldt(ushort);
extern void setup_seg_regs(void);
extern int min(uint, uint);
extern int max(uint, uint);
extern void enable_nmi(void);
extern void bootarg_parse(void);
extern void conf_mem(void);
extern void hold_proc(int);
extern void pause_self(void);
extern int clearnmi(void);
extern int calc_delay(int);
extern boolean_t mainstore_memory(paddr_t);
extern void drv_usecwait(clock_t);
extern void save_fpu(void);
extern int scan_func(int, int, int);
extern void configure(void);
extern void scan_init(int);
extern void init_fpu(void);
extern void yield(void);
extern void cl_trapret(void);

extern void t_diverr(void);
extern void t_dbg(void);
extern void t_nmi(void);
extern void t_int3(void);
extern void t_into(void);
extern void t_check(void);
extern void t_und(void);
extern void t_dna(void);
extern void t_syserr(void);
extern void t_extovr(void);
extern void t_res(void);
extern void t_badtss(void);
extern void t_notpres(void);
extern void t_stkflt(void);
extern void t_gpflt(void);
extern void t_pgflt(void);
extern void t_coperr(void);
extern void t_alignflt(void);
extern void t_mceflt(void);
extern void bin0int(void);
extern void bin1int(void);
extern void bin2int(void);
extern void bin3int(void);
extern void bin4int(void);
extern void bin5int(void);
extern void bin6int(void);
extern void bin7int(void);
extern int getchar(void);
extern void selfinit(int);
extern void sys_call(void);
extern void sig_clean(void);

struct emask;
extern void xcall_init(void);
extern void xcall(struct emask *, struct emask *, void (*)(), void *);
extern void xcall_all(struct emask *, boolean_t, void (*)(), void *);
extern void xcall_intr(void);

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Structure of the system-entry table.
 */
struct sysent {
	char	sy_narg;		/* total number of arguments */
	int	(*sy_call)();		/* handler */
};

#ifdef _KERNEL
extern struct sysent sysent[];
extern unsigned	sysentsize;
#endif

/*
 * Structure of the return-value parameter passed by reference to
 * system entries.
 */
union rval {
	struct	{
		int	r_v1;
		int	r_v2;
	} r_v;
	off_t	r_off;
	time_t	r_time;
};
#define r_val1	r_v.r_v1
#define r_val2	r_v.r_v2
	
typedef union rval rval_t;


struct panic_data {
	struct engine *pd_engine;	/* Panicing engine */
	struct lwp *pd_lwp;		/* LWP running at time of panic */
	struct kcontext *pd_rp;		/* saved regs in panic frame */
	struct kcontext *pd_dblrp;	/* saved regs in dblpanic frame */
};
 
#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern uint_t plocal_intr_depth;

#define servicing_interrupt()	plocal_intr_depth

#ifdef MERGE386

/* Parameter names for mrg_getparm(): */
enum mrg_parm {
	PARM_OFFSET_VM86P,	/* offset of u_vm86p in user_t */
	PARM_OFFSET_AR0,	/* offset of u_ar0 in user_t */
	PARM_OFFSET_FAULTCATCH,	/* offset of u_fault_catch in user_t */
	PARM_ULWPP,		/* pointer to current LWP (lwp_t) */
	PARM_OFFSET_L_SPECIAL,	/* offset of l_special in lwp_t */
	PARM_IDTP,		/* pointer to IDT for this engine */
	PARM_LDTP,		/* pointer to current LDT for this LWP */
	PARM_GDTP,		/* pointer to current GDT for this LWP */
	PARM_TSSP,		/* pointer to current TSS for this LWP */
	PARM_RUID,		/* real UID for this process */
	PARM_AS			/* address-space pointer for this process */
};

#ifdef __STDC__
extern void mrg_getparm(enum mrg_parm, void *);
struct lwp;
extern void mrg_post_evt(struct lwp *);
#else
extern void mrg_getparm();
extern void mrg_post_evt();
#endif

#endif /* MERGE386 */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_SYSTM_H */
