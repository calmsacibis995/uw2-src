/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/strsubr.c	1.98"
#ident	"$Header: $"

#include <util/types.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <svc/errno.h>
#include <proc/bind.h>
#include <proc/signal.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/exec.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <io/conf.h>
#include <util/debug.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/stropts.h>
#include <io/strstat.h>
#include <io/poll.h>
#include <util/inline.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <io/uio.h>
#include <util/cmn_err.h>
#ifdef CC_PARTIAL
#include <acc/mac/mac.h>	/* for mac_installed */
#endif /* CC_PARTIAL */
#include <proc/session.h>
#include <mem/kmem.h>
#include <mem/seg_kvn.h>
#include <proc/siginfo.h>
#include <util/ksynch.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <proc/disp.h>		/* for RUNQUE_LOCK */
#include <util/ghier.h>
#include <proc/proc_hier.h>		/* for PL_SESS */
#include <util/mod/mod_hier.h>
#include <fs/buf.h>		/* for bcb_t */
#include <mem/memresv.h>
#include <util/mod/mod_k.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif /* NO_RDMA */

/*
 * WARNING:
 * The variables and routines in this file are private, belonging
 * to the STREAMS subsystem.  These should not be used by modules
 * or drivers.  Compatibility will not be guaranteed. 
 */

#define ncopyin(A, B, C, D)	copyin(A, B, C)
#define ncopyout(A, B, C, D)	copyout(A, B, C)

/*
 * Id value used to distinguish between different multiplexor links.
 */
STATIC int lnk_id;

/*
 * Queue scheduling control variables.
 */
struct qsvc qsvc;		/* list of queues to run */
char strbcwait;			/* bufcall functions waiting */
STATIC char strbcflag;		/* bufcall functions ready to go */
char *strbceng;			/* keeps track of scheduled UP bufcalls */
STATIC char *altstrbceng;	/* keeps track of scheduled UP bufcalls (for
				   poolrefresh daemon */
volatile char strbcrunning;	/* bufcalls are running */
struct bclist bcall;		/* list of waiting bufcalls */
lock_t svc_mutex;		/* guards queue service list */
lock_t bc_mutex;		/* guards bufcall list */
fspin_t mref_mutex;		/* serializes dupb's and freeb's */
#ifdef STRLEAK
fspin_t strleak_mutex;		/* for debugging memory leaks */
#endif
STATIC lock_t mux_mutex;	/* guards mux_node array during link/unlink */
fspin_t id_mutex;		/* guards ioctl ids */
STATIC fspin_t cm_mutex;	/* guards strevent freelist, and Secache */
lock_t strd_mutex;		/* guards streams deamon work list */
sv_t strd_sv;			/* place for the streams daemon to sleep */
fspin_t strcnt_mutex;		/* guards Strcount */
lock_t vstr_mutex;		/* guards v_stream */
lock_t strio_mutex;		/* guards post-processing ioctl list */
lock_t strsig_mutex;		/* serialize strevent chaining */
#ifdef STRPERF
fspin_t stat_mutex;		/* protect performance data */
struct strperf strperf;		/* performance data */
#endif
int Nsched;			/* # of streams schedulers currently running */
long Strcount;			/* count of streams resources in bytes */
mblk_t *strfree;		/* messages to be freed by strdaemon */
struct striopst *strioclist;	/* list of active post processing requests */
STATIC struct seinfo *sefreelist;	/* list of free event cells */
STATIC struct mux_node *mux_nodes;	/* mux info for cycle checking */
#define SECACHE 10
STATIC struct seinfo Secache[SECACHE];	/* emergency cache of seinfo's */
					/* in case memory runs out */
STATIC struct seinfo *secachep;		/* cache list head */
struct strinfo Strinfo[NDYNAMIC];	/* dynamic resource info */
static struct engine *lastpick;
physreq_t *strphysreq;			/* constrained physreq for compat */
extern int strnsched;
struct engine *kbind(engine_t *);
void svc_enqueue(queue_t *, struct qsvc *);
STATIC void str2time(stdata_t *);
STATIC void strwakebuf(stdata_t *);
STATIC void freeband(qband_t *);
STATIC void bcrun(void);
extern int strdoioctl(stdata_t *, struct strioctl *, mblk_t *, int, char *, cred_t *, int *);
extern int SAMESTR_l(queue_t *);
extern void alloctty(vnode_t *);
extern void pgsignal(struct pid *, int);
extern void dostrlog(mblk_t *);

extern rwlock_t mod_cdevsw_lock, mod_fmodsw_lock;
extern int mod_smod_open(queue_t *, dev_t *, int, int, cred_t *);
extern int mod_sdev_open(queue_t *, dev_t *, int, int, cred_t *);

/*
 * Lock info structures
 */

#define STR_HIER	STR_HIER_BASE + 1	/* beginning hierarchy value for 
						 * stream locks */

/*
 *+ sd_mutex is a per-stream spin lock that protects a stream from undergoing
 *+ any state changes
 */
STATIC LKINFO_DECL(strhead_lkinfo, "STR::sd_mutex", 0);
/*
 *+ mux_mutex is a global spin lock that protects the mux_node array during
 *+ link and unlink operations
 */
STATIC LKINFO_DECL(mux_lkinfo, "STR::mux_mutex", 0);
/*
 *+ svc_mutex is a global spin lock that protects the list of scheduled
 *+ service procedures
 */
STATIC LKINFO_DECL(svc_lkinfo, "STR::svc_mutex", 0);
/*
 *+ bc_mutex is a global spin lock that protects the bufcall list
 */
STATIC LKINFO_DECL(bc_lkinfo, "STR::bc_mutex", 0);
/*
 *+ strd_mutex is a global spin lock that protects the streams daemon work
 *+ list
 */
STATIC LKINFO_DECL(strd_lkinfo, "STR:strd_mutex", 0);
/*
 *+ vstr_mutex is a global spin lock that protects the v_stream field of a
 *+ vnode
 */
STATIC LKINFO_DECL(vstr_lkinfo, "STR::vstr_mutex", 0);
/*
 *+ strsig_mutex is a global spin lock that serializes updates to the
 *+ stream head siglists.
 */
STATIC LKINFO_DECL(strsig_lkinfo, "STR::strsig_mutex", 0);
/*
 *+ strio_mutex is a global spin lock that protects the list of ioctl
 *+ post processing requests
 */
STATIC LKINFO_DECL(strio_lkinfo, "STR::strio_mutex", 0);
/*
 *+ st_mutex/st_sleep are spin lock/sleep locks that protect the information
 *+ in the Strinfo structure
 */
STATIC LKINFO_DECL(strinf_lkinfo, "STR::st_mutex/st_sleep", 0);
/*
 *+ sd_plumb is a per-stream sleep lock that serializes plumbing operations
 */
STATIC LKINFO_DECL(plumb_lkinfo, "STR::sd_plumb", 0);

static void adjfmtp(char **, mblk_t *, int);
static int str2num(char **);
static void mux_rmvedge(stdata_t *, int);

/*
 * Two arrays are used for counting net allocation of streams memory
 * at each of the processors. At any time, only one of the arrays is
 * used for counting. The additional array is substituted in place of
 * the first array, when counts collected in the first array are added
 * into the global Strccount and zeroed out in the collection array.
 */
long	*Strcount_local; 		/* points to the collection array */
STATIC	long	*Strcount_local_1; 
STATIC	long	*Strcount_local_2; 
					/* 2 alternate collection areas */
STATIC	toid_t 	strcount_sync_timeid;	
STATIC	void	strcount_sync(void);		/* 
					 * function to periodically aggrgate 
					 * the per CPU allocations of memory.
					 */

/*
 * void
 * strinit()
 *	Init routine run from main at boot time.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

void
strinit(void)
{
	int i;

	/*
	 * Initialize global locks.
	 */
	LOCK_INIT(&strsig_mutex, STR_HIER_BASE, PLSTR, &strsig_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&mux_mutex, STR_HIER+1, PLSTR, &mux_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&svc_mutex, STR_HIER+1, PLSTR, &svc_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&bc_mutex, STR_HIER+1, PLSTR, &bc_lkinfo, KM_NOSLEEP);
	/*
	 * make this hierarchy big to be safe, really should never be called
	 * out of the driver range
	 */
	LOCK_INIT(&strd_mutex, CMNERR_HIER, PLSTR, &strd_lkinfo, KM_NOSLEEP);
	SV_INIT(&strd_sv);
	LOCK_INIT(&vstr_mutex, STR_HIER+1, PLSTR, &vstr_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&strio_mutex, STR_HIER+1, PLSTR, &strio_lkinfo, KM_NOSLEEP);
	FSPIN_INIT(&mref_mutex);
	FSPIN_INIT(&id_mutex);
	FSPIN_INIT(&strcnt_mutex);
	FSPIN_INIT(&cm_mutex);
#ifdef STRLEAK
	FSPIN_INIT(&strleak_mutex);
#endif
#ifdef STRPERF
	FSPIN_INIT(&stat_mutex);
#endif

	/*
	 * Each entry in this structure has a lock protecting the associated
	 * list.  In all cases except for the stream heads, a spin lock is
	 * both necessary and sufficient.  Stream heads are only allocated/
	 * deallocated during open/close, therefore sleeping is allowed.
	 * The reason a sleep lock is needed is that strpunlink must traverse
	 * the list and may sleep while doing so.  strpunlink is only called
	 * when unmounting file systems to get rid of persistant links and thus
	 * is called very infrequently.
	 */

	for (i = 0; i < NDYNAMIC; ++i) {
		if (i == DYN_STREAM)
			SLEEP_INIT(&Strinfo[i].st_sleep, 0, &strinf_lkinfo, KM_NOSLEEP);
		else
			LOCK_INIT(&Strinfo[i].st_mutex, STR_HIER+2, PLSTR, &strinf_lkinfo, KM_NOSLEEP);
	}

	/*
	 * allocate space to keep track of "UP bufcalls" for scheduling
	 * purposes
	 */
	if ((strbceng = (char *)kmem_zalloc(sizeof(char) * Nengine, KM_NOSLEEP)) == NULL) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not allocate space for strbceng\n");
	}

	if ((altstrbceng = (char *)kmem_zalloc(sizeof(char) * Nengine, KM_NOSLEEP)) == NULL) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not allocate space for altstrbceng\n");
	}

	/*
	 * Set up seinfo cache (if memory runs out, we need a few of
	 * these so things have a chance to recover).
	 */
	sefreelist = NULL;
	for (i = 0; i < SECACHE; ++i) {
		Secache[i].s_next = secachep;
		secachep = &Secache[i];
	}

	/*
	 * Set up mux_node structures.
	 */
	if ((mux_nodes =
	    (struct mux_node *)kmem_alloc((sizeof(struct mux_node) * cdevcnt),
	    KM_NOSLEEP)) == NULL) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not allocate space for mux_nodes\n");
	}
	for (i = 0; i < cdevcnt; i++) {
		mux_nodes[i].mn_imaj = i;
		mux_nodes[i].mn_indegree = 0;
		mux_nodes[i].mn_originp = NULL;
		mux_nodes[i].mn_startp = NULL;
		mux_nodes[i].mn_outp = NULL;
		mux_nodes[i].mn_flags = 0;
	}

	/* ioctl post processing list */
	strioclist = NULL;

	/* initialize scheduler algorithm */
	lastpick = engine;

	Strcount_local_1 = kmem_zalloc((sizeof(long) * Nengine), KM_NOSLEEP);
	Strcount_local_2 = kmem_zalloc((sizeof(long) * Nengine), KM_NOSLEEP);

	if ((Strcount_local_1 == NULL)	|| (Strcount_local_2 == NULL)) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not find space for Strcount_local\n");
	}
	Strcount_local = Strcount_local_1;

	if ((strphysreq = physreq_alloc(KM_NOSLEEP)) == NULL) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not allocate space for strphysreq\n");
	}
	strphysreq->phys_align = NBPW;
	strphysreq->phys_boundary = 0;
	strphysreq->phys_dmasize = rdma_dflt_bcb.bcb_physreqp->phys_dmasize;
	strphysreq->phys_max_scgth = 0;
	strphysreq->phys_flags |= PREQ_PHYSCONTIG;
	if (!physreq_prep(strphysreq, KM_NOSLEEP)) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not initialize strphysreq\n");
	}
}

/*
 * STATIC void
 * strcount_sync(void)
 *	Add in the per-procesor Stcount_local values into the global
 *	Strcount. And zero the per-processor values.
 *
 * Calling/Exit State:
 *	Called as a timeout function. No locks held on entry/exit.
 */
STATIC void
strcount_sync(void)
{
	int i;
	long my_strc = 0;
	long *my_strc_localp;
	long *my_strc_localp_other;

	my_strc_localp = Strcount_local;

	/* 
	 * Replace current Strcount_local array with the alternate
	 * Strcount_local array.
	 */ 

	if (my_strc_localp == Strcount_local_1) {
		my_strc_localp_other = Strcount_local_2;
	} else {
		my_strc_localp_other = Strcount_local_1;
	}

	Strcount_local = my_strc_localp_other;

	for (i = 0; i < Nengine; i++) {
		my_strc += *(my_strc_localp + i);
		*(my_strc_localp + i) = 0;
	}

	FSPIN_LOCK(&strcnt_mutex);
	Strcount += my_strc;
	FSPIN_UNLOCK(&strcnt_mutex);

	return;
}

/*
 * void
 * strsendsig(struct strevent *siglist, int event, long data)
 *	Send SIGPOLL signal to all processes registered on the given signal
 *	list that want a signal for the specified event.
 *
 * Calling/Exit State:
 *	The stream mutex lock (sd_mutex) be held by the caller upon entry.
 *	The sd_mutex remains held upon return.
#ifdef STRPERF
 *	Counted in b_sh
#endif
 */

void
strsendsig(struct strevent *siglist, int event, long data)
{
	struct strevent *sep;
	struct sigqueue *sqp;
	k_siginfo_t *info;

	for (sep = siglist; sep; sep = sep->se_next) {
		if (sep->se_events & event) {
			if (!(event & (S_RDNORM|S_WRBAND|S_RDBAND))) {
				if (sqp = siginfo_get(KM_NOSLEEP, 0)) {
					info = &sqp->sq_info;
					info->si_signo = SIGPOLL;
					info->si_errno = 0;
				}
			}
			switch (event) {
			case S_INPUT:
				if (sqp) {
					info->si_code = POLL_IN;
					info->si_band = data;
				}
				goto sendsig;

			case S_OUTPUT:
				if (sqp) {
					info->si_code = POLL_OUT;
					info->si_band = data;
				}
				goto sendsig;

			case S_HIPRI:
				if (sqp) {
					info->si_code = POLL_PRI;
					info->si_band = 0;
				}
				goto sendsig;

			case S_MSG:
				if (sqp) {
					info->si_code = POLL_MSG;
					info->si_band = data;
				}
				goto sendsig;

			case S_ERROR:
				if (sqp) {
					info->si_code = POLL_ERR;
					info->si_band = 0;
					info->si_errno = (int)data;
				}
				goto sendsig;

			case S_HANGUP:
				if (sqp) {
					info->si_code = POLL_HUP;
					info->si_band = 0;
				}
sendsig:
				if(!sigtolwp(sep->se_lwpp, SIGPOLL, sqp) && sqp)
						siginfo_free(sqp);
				break;

			case S_RDBAND:
				if (sep->se_events & S_BANDURG) {
					sigtolwp(sep->se_lwpp, SIGURG,
						 (sigqueue_t *) NULL);
					break;
				}
				/* FALLTHROUGH */

			case S_RDNORM:
			case S_WRBAND:
				sigtolwp(sep->se_lwpp, SIGPOLL,
					(sigqueue_t *) NULL);
				break;

			default:
				if (sqp)
					kmem_free((void *)sqp,
						  sizeof (sigqueue_t));
				/*
				 *+ strsendsig() was called with an invalid
				 *+ argument.  This indicates an error in
				 *+ the kernel code.
				 */
				cmn_err(CE_PANIC, "strsendsig: unknown event %x\n", event);
			}
		}
	}
}

/*
 * int
 * qattach(queue_t *qp, dev_t *devp, int flag, int table, int idx, cred_t *crp, int rflag)
 *	Attach a stream device or module.  qp is a read queue; the new queue
 *	goes in so its next read ptr is the argument, and the write queue
 *	corresponding to the argument points to this queue.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.  Return 0 on success, or a non-zero errno
 *	on failure.
 *
 * Description:
 *	If we're about to push a UP module onto the stream and there is
 *	a UP-unfriendly mux at the bottom, push the uniplexor onto the
 *	stream.
 *
 * Remarks:
 *	Both the I_PUSH code in the stream head and this routine have to
 *	contend with kbind'ing.  In the stream head, the case that is being
 *	handled is a stream which is already bound; whereas, this function
 *	is handling the case of an unbound stream that is about to become
 *	bound by having a UP module pushed onto it.
 *
 *	As an optimization, QBOUND is only set if a machine is configured
 *	with more than 1 processor.  Likewise, the uniplexor is only pushed
 *	on machines configured with more than 1 processor.  As a result,
 *	single processor systems will not pay the bulk of the UP support
 *	overhead.
 */

int
qattach(queue_t *qp, dev_t *devp, int flag, int table, int idx, cred_t *crp, int rflag)
{
	queue_t *rq;
	queue_t *tq;
	pl_t pl;
	pl_t pl_1;
	struct streamtab *qinfop;
	int sflg;
	dev_t odev;
	int error;
	int isup;
	int wasup;
	int upf;
	int cpu;
	int tmpidx;
	int auxflag;
	struct engine *engp;
	int unbind;
	struct module *modp;
	int (*qopen)();

	qopen = NULL;
	error = 0;
	unbind = 0;
	upf = 0;
	cpu = -1;
	if (!mem_resv_check())
		return(ENXIO);
	if ((rq = allocq()) == NULL)
		return(ENXIO);
	odev = *devp;
	/* don't need locks yet, no one can get at rq */
	if (table == CDEVSW) {
		pl = RW_RDLOCK(&mod_cdevsw_lock, PLDLM);
		if (*cdevsw[idx].d_flag & D_OLD) {
			RW_UNLOCK(&mod_cdevsw_lock, pl);
			/*
			 *+ An attempt to attach a pre-SVR4 driver is being
			 *+ made.  This is no longer supported.
			 */
			cmn_err(CE_WARN, "Attempt to open pre-SVR4 driver\n");
			if (rflag == 0)
				freeq(rq);
			return(EINVAL);
		}
		qinfop = cdevsw[idx].d_str;
		cpu = cdevsw[idx].d_cpu;
		auxflag = cdevsw[idx].d_auxflag;
		sflg = 0;
		isup = (*cdevsw[idx].d_flag & D_MP) ? 0 : 1;
		if (qinfop->st_muxwinit) {
			/* only mux's should set this flag */
			if (!(*cdevsw[idx].d_flag & D_UPF)) {
				/*
				 * set upf if we're a mux and we're not
				 * UP-friendly
				 */
				upf = 1;
			}
		} else {
			/*
			 * intercept only non-multiplexing drivers, that
			 * are ddi 5 or earlier.  At a later date, if needed
			 * we may differentiate between ddi 5 and ddi < 5
			 */
			if (auxflag)
				rq->q_flag |= QINTER;
		}
		modp = cdevsw[idx].d_modp;
		QU_MODIDX(rq) = idx;
		if ((QU_MODP(rq) = modp) != NULL) {
			if (MOD_IS_UNLOADING(modp)) {
				qopen = mod_sdev_open;
				RW_UNLOCK(&mod_cdevsw_lock, pl);
			} else {
				RW_UNLOCK(&mod_cdevsw_lock, PLDLM);
				MOD_HOLD_L(modp, pl);
			}		
		} else
			RW_UNLOCK(&mod_cdevsw_lock, pl);
	} else {
		ASSERT(table == FMODSW);
		pl = RW_RDLOCK(&mod_fmodsw_lock, PLSTR);
		if (*fmodsw[idx].f_flag & D_OLD) {
			RW_UNLOCK(&mod_fmodsw_lock, pl);
			/*
			 *+ An attempt to attach a pre-SVR4 module is being
			 *+ made.  This is no longer supported.
			 */
			cmn_err(CE_WARN, "Attempt to open pre-SVR4 module\n");
			if (rflag == 0)
				freeq(rq);
			return(EINVAL);
		}
		qinfop = fmodsw[idx].f_str;
		sflg = MODOPEN;
		isup = (*fmodsw[idx].f_flag & D_MP) ? 0 : 1;
		modp = fmodsw[idx].f_modp;
		QU_MODIDX(rq) = idx;
		if ((QU_MODP(rq) = modp) != NULL) {
			if (MOD_IS_UNLOADING(modp)) {
				qopen = mod_smod_open;
				RW_UNLOCK(&mod_fmodsw_lock, pl);
			} else {
				RW_UNLOCK(&mod_fmodsw_lock, PLDLM);
				MOD_HOLD_L(modp, pl);
			}		
		} else
			RW_UNLOCK(&mod_fmodsw_lock, pl);
	}

	pl = LOCK(qp->q_str->sd_mutex, PLSTR);
	if (isup && (table == FMODSW) && qp->q_str->sd_cpu && (qp->q_str->sd_cpu != engine)) {
		/*
		 * An attempt to push single threaded module on a
		 * stream bound to something other than the boot
		 * processor.  That configuration is not currently
		 * allowed.  Modules always bind to 0.  If this is
		 * a recursive call, the queues will be deleted in
		 * the previous call.
		 */
		if (rflag == 0)
			freeq(rq);
		UNLOCK(qp->q_str->sd_mutex, pl);
		return(EINVAL);
	}
	if (upf) {
		/*
		 * Non-friendly mux on stream, note, upf can only
		 * be set on the initial driver open.  We do this
		 * here to save a lock roundtrip above.
		 */
		qp->q_str->sd_flag &= ~UPF;
	}
	wasup = qp->q_flag & QBOUND;
	if ((isup || (cpu != -1)) && !wasup) {
		while (qp->q_str->sd_upbcnt) {
			/*
			 * message is being inserted into non-bound stream,
			 * have to wait.  Note: SV_WAIT returns at pl0 with
			 * no lock held
			 */
			qp->q_str->sd_flag |= UPBLOCK;
			if (SV_WAIT_SIG(qp->q_str->sd_upblock, PRIMED, qp->q_str->sd_mutex) == B_FALSE) {
				(void) LOCK(qp->q_str->sd_mutex, PLSTR);
				qp->q_str->sd_flag &= ~UPBLOCK;
				if (upf)
					/* restore UPF flag */
					qp->q_str->sd_flag |= UPF;
				if (rflag == 0)
					freeq(rq);
				UNLOCK(qp->q_str->sd_mutex, pl);
				return(EINTR);
			}
			(void) LOCK(qp->q_str->sd_mutex, PLSTR);
			qp->q_str->sd_flag &= ~UPBLOCK;
		}
		/* need to test table, could be a UP mux! */
		if ((Nengine > 1) && !(qp->q_str->sd_flag & UPF) && (table == FMODSW)) {
			/*
			 * We're about to push a UP module on an unbound
			 * stream and we have a UP-unfriendly mux at the
			 * bottom.  Have to push on the "uniplexor" first.
			 * Open never fails.
			 */
			if ((tmpidx = findmod("uni")) < 0) {
				UNLOCK(qp->q_str->sd_mutex, pl);
				/*
				 *+ Uniprocessor compatibility module is
				 *+ missing
				 */
				cmn_err(CE_WARN, "KERNEL:qattach: uniplexor is missing\n");
				if (rflag == 0)
					freeq(rq);
				return(EINVAL);
			}
			UNLOCK(qp->q_str->sd_mutex, pl);
			/*
			 * This is a recursive call.  Flag is set even though
			 * it's a don't care for uni (which never fails)
			 */
			(void) qattach(qp, devp, flag, FMODSW, tmpidx, crp, 1);
			pl = LOCK(qp->q_str->sd_mutex, PLSTR);
		}
		/*
		 * First UP module to go on stream, need to do some extra
		 * work.
		 *
		 * Important note: stream pipes share an sd_mutex, so holding
		 * the lock protects the whole pipe.
		 *
		 * First bind local stream head.  Can't hold sd_mutex across
		 * call to engine_disable_offline, but nothing important has
		 * happened yet.  If this is an I_PUSH, then sd_plumb is held
		 * and everything is ok.
		 */
		UNLOCK(qp->q_str->sd_mutex, pl);
		if (cpu == -1) {
			if (!engine_disable_offline(0)) {
				/*
				 *+ Could not lock engine on line
				 */
				cmn_err(CE_WARN, "KERNEL:qattach: engine_disable_offline failed on processor 0\n");
				if (rflag == 0)
					freeq(rq);
				return(EINVAL);
			}
			pl = LOCK(qp->q_str->sd_mutex, PLSTR);
			qp->q_str->sd_cpu = engine;
		} else {
			if (!engine_disable_offline(cpu)) {
				/*
				 *+ Could not lock engine on line
				 */
				cmn_err(CE_WARN, "KERNEL:qattach: engine_disable_offline failed on processor %d\n", cpu);
				if (rflag == 0)
					freeq(rq);
				return(EINVAL);
			}
			pl = LOCK(qp->q_str->sd_mutex, PLSTR);
			qp->q_str->sd_cpu = &engine[cpu];
		}
		/* If it's a pipe, need to find other stream head to bind it */
		if (qp->q_str->sd_vnode->v_type == VFIFO) {
			tq = qp->q_str->sd_wrq;
			while (SAMESTR_l(tq))
				tq = tq->q_next;
			ASSERT(tq);
			ASSERT(tq->q_next);
			/* other "half" of pipe points to remote streamhead */
			tq->q_next->q_str->sd_cpu = (cpu == -1) ? engine : &engine[cpu];
		}
		/*
		 * Since this is a first time UP module, we have to get to
		 * the right cpu.
		 */
		UNLOCK(qp->q_str->sd_mutex, pl);
		engp = kbind(qp->q_str->sd_cpu);
		/*
		 * Since the module is a UP module, disable 
		 * preemption and also mark the context as
		 * not reentrant.
		 */
		DISABLE_PRMPT();
		u.u_lwpp->l_notrt++;
		u.u_lwpp->l_cdevswp = &cdevsw[getmajor(*devp)];
		unbind = 1;
		pl = LOCK(qp->q_str->sd_mutex, PLSTR);
		tq = qp->q_str->sd_wrq;
		while (tq) {
			/*
			 * At this point, the new module/driver isn't attached
			 * yet.  We could have the situation in which this
			 * newly bound stream has currently enabled queues
			 * so we need to pick them off the global run list
			 * and put them on the per-cpu list.  If the open
			 * subsequently fails and the stream is unbound, it
			 * won't hurt to have the service procedures run on a
			 * designated cpu.  They'll begin to float after the
			 * qdetach().  We had to kbind first so l. refers to
			 * the correct structure.
			 *
			 * While we're in this loop, mark queues as being
			 * bound.  Note that even if it's a FIFO, this loop
			 * will get to the other stream head.  That's
			 * why we use OTHERQ.
			 */
			tq->q_flag |= QBOUND;
			OTHERQ(tq)->q_flag |= QBOUND;
			pl_1 = LOCK(&svc_mutex, PLSTR);
			if (tq->q_svcflag & QENAB) {
				svc_dequeue(tq, &qsvc);
				svc_enqueue(tq, &l.qsvc);
			}
			if (OTHERQ(tq)->q_svcflag & QENAB) {
				svc_dequeue(tq, &qsvc);
				svc_enqueue(tq, &l.qsvc);
			}
			UNLOCK(&svc_mutex, pl_1);
			tq = tq->q_next;
		}
	}
	rq->q_next = qp;
	WR(rq)->q_next = WR(qp)->q_next;
	if (WR(qp)->q_next)
		OTHERQ(WR(qp)->q_next)->q_next = rq;
	WR(qp)->q_next = WR(rq);
	rq->q_str = qp->q_str;
	WR(rq)->q_str = qp->q_str;
	setq(rq, qinfop->st_rdinit, qinfop->st_wrinit);
 	rq->q_flag |= QWANTR;
	WR(rq)->q_flag |= QWANTR;
	if (isup || (cpu != -1)) {
		/* mark queue as UP only */
		rq->q_flag |= (QUP|QBOUND);
		WR(rq)->q_flag |= (QUP|QBOUND);
	} else if (wasup) {
		/* MP module pushed on already bound stream */
		rq->q_flag |= QBOUND;
		WR(rq)->q_flag |= QBOUND;
	}

	/*
	 * Open the attached module or driver.
	 * The open may sleep, but it must always return here.  Therefore
	 * all sleeps must set PCATCH or ignore all signals to avoid a 
	 * longjmp if a signal arrives.
	 *
	 * Note spl, DDI says open called at PLSTR.
	 */
	UNLOCK(qp->q_str->sd_mutex, PLSTR);
	if (isup) {
		/*
		 * If it's a UP module/driver, it won't know to do a
		 * qprocson(), and will also expect to see responses to
		 * any messages that it sends, so we do this for it
		 * before the open is called.
		 */
		qprocson(rq);
	}

	if (qopen == NULL)
		qopen = rq->q_qinfo->qi_qopen;
	/*
	 * If the open fails, qdetach handles all of the UP clean up
	 */
	if (error=(*qopen)(rq, devp, flag, sflg, crp)) {
		splx(pl);
		/*
		 * If returned from a stub open, don't need to do the detach,
		 * but we do have to remove the partial queue. If this is
		 * a recursive call, then don't do any cleanup; it'll happen
		 * during the unwind
		 */
		if (rflag == 0) {
			if ((qopen != mod_smod_open) && (qopen != mod_sdev_open))
				qdetach(rq, 0, 0, crp);
			else {
				pl = LOCK(qp->q_str->sd_mutex, PLSTR);
				if (WR(rq)->q_next)
					backq_l(rq)->q_next = rq->q_next;
				if (rq->q_next)
					backq_l(WR(rq))->q_next = WR(rq)->q_next;
				UNLOCK(qp->q_str->sd_mutex, pl);
				freeq(rq);
			}
		}
		if (unbind) {
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
			ENABLE_PRMPT();
			kunbind(engp);
			u.u_lwpp->l_cdevswp = NULL;
		}
		return(error);
	}
	rq = RD(WR(qp)->q_next);
	/*
	 * It is expected that MP drivers will do their own qprocson
	 * as the ddi specifies.
	 */
#ifdef DEBUG
	(void) LOCK(rq->q_str->sd_mutex, PLSTR);
	ASSERT(rq->q_flag & QPROCSON);
	UNLOCK(rq->q_str->sd_mutex, PLSTR);
#endif
	splx(pl);
	idx = getmajor(*devp);
	if ((table == CDEVSW) && (idx != getmajor(odev))) {
		if (idx >= cdevcnt) {
			qdetach(rq, 0, 0, crp);
			if (unbind) {
				ASSERT(u.u_lwpp->l_notrt != 0);
				u.u_lwpp->l_notrt--;
				ENABLE_PRMPT();
				kunbind(engp);
				u.u_lwpp->l_cdevswp = NULL;
			}
			return(ENXIO);
		}
		qinfop = cdevsw[idx].d_str;
		pl = LOCK(rq->q_str->sd_mutex, PLSTR);
		setq(rq, qinfop->st_rdinit, qinfop->st_wrinit);
		UNLOCK(rq->q_str->sd_mutex, pl);
	}
	/*
	 * Note: all of the kunbind's are sprinkled by returns because we might
	 * have needed to be bound for the qdetachs if the open failed.
	 */
	if (unbind) {
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(engp);
		u.u_lwpp->l_cdevswp = NULL;
	}
	return(0);
}

/*
 * void
 * qdetach(queue_t *qp, int clmode, int flag, cred_t *crp)
 *	Detach a stream module or device.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 *
 * Description:
 *	If clmode == 1 then the module or driver was opened and its
 *	close routine must be called.  If clmode == 0, the module
 *	or driver was never opened or the open failed, and so its close
 *	should not be called.
 */

void
qdetach(queue_t *qp, int clmode, int flag, cred_t *crp)
{
	pl_t pl;
	int isup;
	queue_t *tq;
	struct module *modp;

	if (clmode) {
		/* DDI says to block interrupts for close */
		pl = splstr();
		(*qp->q_qinfo->qi_qclose)(qp, flag, crp);
	} else {
		pl = getpl();
	}

	/*
	 * Check if procs have been disabled; if not then do so.
	 */
	(void) LOCK(qp->q_str->sd_mutex, PLSTR);
	isup = qp->q_flag & QUP;
	if (qp->q_flag & QPROCSON) {
		/*
		 * this should only happen with UP modules/drivers
		 * Note: deferred puts can not fire because we are at an
		 * elevated IPL at this point so the turning off of QUSE
		 * is atomic with respect to the close from above.
		 */
		qp->q_flag &= ~QUSE;
		WR(qp)->q_flag &= ~QUSE;
		/* stay at PLSTR to keep out unwanted service procedures */
		UNLOCK(qp->q_str->sd_mutex, PLSTR);
		/*
		 * Note, qprocsoff will sleep until all deferred puts have
		 * cleared state
		 */
		qprocsoff(qp);
		(void) LOCK(qp->q_str->sd_mutex, PLSTR);
	}

	if (WR(qp)->q_next)
		backq_l(qp)->q_next = qp->q_next;
	if (qp->q_next)
		backq_l(WR(qp))->q_next = WR(qp)->q_next;

	/*
	 * At this point, the queues have been removed.  If the module/driver
	 * being detached was UP, and no UP things are left on the stream,
	 * then we need to unbind the other queues.  We might also have to
	 * get rid of the uniplexor.  If sd_wrq->q_next is NULL, then the
	 * stream is closing down so just skip this stuff.
	 */
	if (isup && qp->q_str->sd_wrq->q_next) {
		/* qp hasn't been freed yet, so it's still valid to reference */
		if (qp->q_str->sd_wrq->q_next->q_qinfo->qi_minfo->mi_idnum == UNI_ID) {
			/* It's the uniplexor */
			UNLOCK(qp->q_str->sd_mutex, pl);
			qdetach(RD(qp->q_str->sd_wrq->q_next), 1, flag, crp);
			pl = LOCK(qp->q_str->sd_mutex, PLSTR);
		}
		/* stream head itself is never QUP */
		tq = qp->q_str->sd_wrq->q_next;
		while (tq) {
			if (tq->q_flag & QUP) {
				/* still a UP thing on the stream */
				break;
			} else {
				tq = tq->q_next;
			}
		}
		if (tq == NULL) {
			/* removed the only UP thing on the stream, unbind it */
			qp->q_str->sd_cpu = NULL;
			if (qp->q_str->sd_vnode->v_type == VFIFO) {
				tq = qp->q_str->sd_wrq;
				while (SAMESTR_l(tq))
					tq = tq->q_next;
				ASSERT(tq);
				ASSERT(tq->q_next);
				/* other "half" of pipe points to remote stream head */
				tq->q_next->q_str->sd_cpu = NULL;
			}
			tq = qp->q_str->sd_wrq;
			while (tq) {
				tq->q_flag &= ~QBOUND;
				OTHERQ(tq)->q_flag &= ~QBOUND;
				tq = tq->q_next;
			}
		}
	}
	UNLOCK(qp->q_str->sd_mutex, pl);
	if ((modp = (QU_MODP(qp))) != NULL)
		MOD_RELE(modp);
	freeq(qp);
}

/*
 * void
 * strtime(stdata_t *stp)
 *	This function is placed in the callout table to wake up a process
 *	waiting to close a stream that has not completely drained.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked and pl <= PLSTR.
 */

void
strtime(stdata_t *stp)
{
	pl_t pl;

	pl = LOCK(stp->sd_mutex, PLSTR);
	if (stp->sd_flag & STRTIME) {
		stp->sd_flag &= ~STRTIME;
		if (SV_BLKD(stp->sd_timer)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_timer, 0);
		} 
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}
}

/*
 * void
 * str2time(stdata_t *stp)
 *	This function is placed in the callout table to wake up all
 *	processes waiting to send an ioctl down a particular stream,
 *	as well as the process whose ioctl is still outstanding.  The
 *	process placing this function in the callout table will remove
 *	it if it gets control of the ioctl mechanism for the stream -
 *	this should only run if there is a failure.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked and spl <= PLSTR.
 */

STATIC void
str2time(stdata_t *stp)
{
	pl_t pl;

	pl = LOCK(stp->sd_mutex, PLSTR);
	if (SV_BLKD(stp->sd_timer2)) {
		UNLOCK(stp->sd_mutex, pl);
		SV_BROADCAST(stp->sd_timer2, 0);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}
}

/*
 * void
 * str3time(stdata_t *stp)
 *	This function is placed in the callout table to wake up the
 *	process that has an outstanding ioctl waiting acknowledgement
 *	on a stream.  It should be removed from the callout table
 *	when the acknowledgement arrives.  If this function runs, it
 *	is the result of a failure.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.  Assumes pl <= PLSTR.
 */

void
str3time(stdata_t *stp)
{
	pl_t pl;

	pl = LOCK(stp->sd_mutex, PLSTR);
	if (stp->sd_flag & STR3TIME) {
		stp->sd_flag &= ~STR3TIME;
		if (SV_BLKD(stp->sd_timer3)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_timer3, 0);
		}
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}
}

/*
 * int
 * putiocd(mblk_t *bp, mblk_t *ebp, caddr_t arg, int copymode, int flag,
 *	   char *fmt, stdata_t *stp)
 *	Put ioctl data from user land to ioctl buffers.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.  Return non-zero errno for failure,
 *	1 for success.
 */

int
putiocd(mblk_t *bp, mblk_t *ebp, caddr_t arg, int copymode, int flag,
	char *fmt, stdata_t *stp)
{
	mblk_t *tmp;
	int count;
	int n;
	mblk_t *obp;
	int error;

	obp = bp;
	error = 0;
	if (bp->b_datap->db_type == M_IOCTL)
		/* LINTED pointer alignment */
		count = ((struct iocblk *)bp->b_rptr)->ioc_count;
	else {
		ASSERT(bp->b_datap->db_type == M_COPYIN);
		/* LINTED pointer alignment */
		count = ((struct copyreq *)bp->b_rptr)->cq_size;
	}
	/*
	 * strdoioctl validates ioc_count, so if this assert fails it
	 * cannot be due to user error.
	 */
	ASSERT(count >= 0);

	while (count) {
		n = MIN(MAXIOCBSZ, count);
		if (flag == SE_SLEEP) {
			while (!(tmp = allocb(n, BPRI_HI))) {
				if (error = strwaitbuf(n, BPRI_HI, stp))
					return(error);
			}
		} else if (!(tmp = allocb(n, BPRI_HI)))
			return(EAGAIN);
		error = strcopyin((caddr_t)arg, (caddr_t)tmp->b_wptr, n, fmt, copymode);
		if (error) {
			freeb(tmp);
			return(error);
		}
		if (fmt && (count > MAXIOCBSZ) && (copymode == U_TO_K))
			adjfmtp(&fmt, tmp, n);
		arg += n;
		tmp->b_datap->db_type = M_DATA;
		tmp->b_wptr += n;
		count -= n;
		bp = (bp->b_cont = tmp);
	}

	/*
	 * If ebp was supplied, place it between the
	 * M_IOCTL block and the (optional) M_DATA blocks.
	 */
	if (ebp) {
		ebp->b_cont = obp->b_cont;
		obp->b_cont = ebp;
	}
	return(0);
}

/*
 * int
 * getiocd(mblk_t *bp, caddr_t arg, int copymode, char *fmt)
 *	Copy ioctl data to user-land.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.  Return non-zero errno on failure,
 *	0 for success.
 */

int
getiocd(mblk_t *bp, caddr_t arg, int copymode, char *fmt)
{
	int count;
	int n;
	int error;

	if (bp->b_datap->db_type == M_IOCACK)
		/* LINTED pointer alignment */
		count = ((struct iocblk *)bp->b_rptr)->ioc_count;
	else {
		ASSERT(bp->b_datap->db_type == M_COPYOUT);
		/* LINTED pointer alignment */
		count = ((struct copyreq *)bp->b_rptr)->cq_size;
	}
	ASSERT(count >= 0);

	for (bp = bp->b_cont; bp && count; bp = bp->b_cont) {
		n = MIN(count, bp->b_wptr - bp->b_rptr);
		error = strcopyout((caddr_t)bp->b_rptr, arg, n, fmt, copymode);
		if (error)
			return(error);
		if (fmt && bp->b_cont && (copymode == U_TO_K))
			adjfmtp(&fmt, bp, n);
		count -= n;
		arg += n;
	}
	ASSERT(count == 0);
	return(0);
}

/* 
 * struct linkinfo *
 * alloclink(queue_t *qup, queue_t *qdown, file_t *fpdown)
 *	Allocate a linkinfo table entry given the write queue of the
 *	bottom module of the top stream and the write queue of the
 *	stream head of the bottom stream.
 *
 * Calling/Exit State:
 *	Assumes st_mutex unlocked.
 */

struct linkinfo *
alloclink(queue_t *qup, queue_t *qdown, file_t *fpdown)
{
	struct linkinfo *linkp;
	struct linkinfo *lp;
	pl_t pl;
#ifdef CC_PARTIAL
	ulong rand;
#endif /* CC_PARTIAL */

	linkp = (struct linkinfo *) kmem_zalloc(sizeof(struct linkinfo), KM_SLEEP);
	linkp->li_lblk.l_qtop = qup;
	linkp->li_lblk.l_qbot = qdown;

	/*
	 * Assign link id, being careful to watch out
	 * for wrap-around leading to id clashes.  Unlike
	 * ioctl ids, link ids can be long-lived.
	 * If we're concerned about covert channels, start the id search
	 * at a random place rather than where the previous search stopped.
	 * If the random() routine is stubbed out, it will return 0,
	 * in which case we want to revert to the sequential method.
	 */

	/*
	 * Note: lnk_id is essentially a random number.  We just have to
	 * check to make sure it isn't in use by someone else.  However,
	 * since this is the only reference to it, it gets protected for
	 * free by Strinfo[DYN_LINKBLK].st_mutex.
	 */

	pl = LOCK(&Strinfo[DYN_LINKBLK].st_mutex, PLSTR);
#ifdef CC_PARTIAL
	if ((rand = random((ulong)INT_MAX)) != 0)
		lnk_id = (int)rand;
#endif /* CC_PARTIAL */

	do {
		linkp->li_lblk.l_index = ++lnk_id;
		if (lnk_id <= 0)
			linkp->li_lblk.l_index = lnk_id = 1;
		for (lp = (struct linkinfo *)Strinfo[DYN_LINKBLK].st_head; lp; lp = lp->li_next)
			if (lp->li_lblk.l_index == lnk_id)
				break;
	} while (lp);

	linkp->li_fpdown = fpdown;
	Strinfo[DYN_LINKBLK].st_cnt++;
	linkp->li_next = (struct linkinfo *) Strinfo[DYN_LINKBLK].st_head;
	if (linkp->li_next)
		linkp->li_next->li_prev = linkp;
	linkp->li_prev = NULL;
	Strinfo[DYN_LINKBLK].st_head = (void *) linkp;
	UNLOCK(&Strinfo[DYN_LINKBLK].st_mutex, pl);

	*(Strcount_local + l.eng_num) += sizeof(struct linkinfo);

	MET_STRLINK(1);
	return(linkp);
}

/*
 * void
 * lbfree(struct linkinfo *linkp)
 *	Free a linkinfo entry.
 *
 * Calling/Exit State:
 *	Assumes st_mutex unlocked.
 */

void
lbfree(struct linkinfo *linkp)
{
	pl_t pl;

	pl = LOCK(&Strinfo[DYN_LINKBLK].st_mutex, PLSTR);
	if (linkp->li_prev == NULL) {
		if (linkp->li_next)
			linkp->li_next->li_prev = NULL;
		Strinfo[DYN_LINKBLK].st_head = (void *) linkp->li_next;
	}
	else {
		if (linkp->li_next)
			linkp->li_next->li_prev = linkp->li_prev;
		linkp->li_prev->li_next = linkp->li_next;
	}
	Strinfo[DYN_LINKBLK].st_cnt--;
	UNLOCK(&Strinfo[DYN_LINKBLK].st_mutex, pl);
	kmem_free(linkp, sizeof(struct linkinfo));
	
	*(Strcount_local + l.eng_num) -= sizeof(struct linkinfo);

	MET_STRLINK(-1);
	return;
}

/*
 * int
 * linkcycle(stdata_t *upstp, stdata_t *lostp)
 *	Check for a potential linking cycle.
 *
 * Calling/Exit State:
 *	Assumes mux_mutex not locked.  Return 1 if a link will result
 *	in a cycle, and 0 otherwise.
 */

int
linkcycle(stdata_t *upstp, stdata_t *lostp)
{
	struct mux_node *np;
	struct mux_edge *ep;
	int i;
	long lomaj;
	long upmaj;
	pl_t pl;

	/*
	 * if the lower stream is a pipe/FIFO, return, since link
	 * cycles can not happen on pipes/FIFOs
	 */
	if (lostp->sd_vnode->v_type == VFIFO)
		return(0);

	pl = LOCK(&mux_mutex, PLSTR);
	for (i = 0; i < cdevcnt; i++) {
		np = &mux_nodes[i];
		MUX_CLEAR(np);
	}
	lomaj = getmajor(lostp->sd_vnode->v_rdev);
	upmaj = getmajor(upstp->sd_vnode->v_rdev);
	np = &mux_nodes[lomaj];
	for ( ; ; ) {
		if (!MUX_DIDVISIT(np)) {
			if (np->mn_imaj == upmaj) {
				UNLOCK(&mux_mutex, pl);
				return(1);
			}
			if (np->mn_outp == NULL) {
				MUX_VISIT(np);
				if (np->mn_originp == NULL) {
					UNLOCK(&mux_mutex, pl);
					return(0);
				}
				np = np->mn_originp;
				continue;
			}
			MUX_VISIT(np);
			np->mn_startp = np->mn_outp;
		} else {
			if (np->mn_startp == NULL) {
				if (np->mn_originp == NULL) {
					UNLOCK(&mux_mutex, pl);
					return(0);
				}
				else {
					np = np->mn_originp;
					continue;
				}
			}
			ep = np->mn_startp;
			np->mn_startp = ep->me_nextp;
			ep->me_nodep->mn_originp = np;
			np = ep->me_nodep;
		}
	}
}

/* 
 * struct linkinfo *
 * findlinks(stdata_t *stp, int index, int type)
 *	Find linkinfo table entry corresponding to the parameters.
 *
 * Calling/Exit State:
 *	st_mutex assumed unlocked.  mux_mutex assumed unlocked
 */

struct linkinfo *
findlinks(stdata_t *stp, int index, int type)
{
	pl_t pl;
	struct linkinfo *linkp;
	struct mux_edge *mep;
	struct mux_node *mnp;
	queue_t *qup;

	if ((type & LINKTYPEMASK) == LINKNORMAL) {
		qup = getendq(stp->sd_wrq);
		pl = LOCK(&Strinfo[DYN_LINKBLK].st_mutex, PLSTR);
		for (linkp = (struct linkinfo *) Strinfo[DYN_LINKBLK].st_head; linkp; linkp = linkp->li_next) {
			if ((qup == linkp->li_lblk.l_qtop) &&
		    	    (!index || (index == linkp->li_lblk.l_index))) {
				UNLOCK(&Strinfo[DYN_LINKBLK].st_mutex, pl);
				return(linkp);
			}
		}
		UNLOCK(&Strinfo[DYN_LINKBLK].st_mutex, pl);
	} else {
		ASSERT((type & LINKTYPEMASK) == LINKPERSIST);
		pl = LOCK(&mux_mutex, PLSTR);
		mnp = &mux_nodes[getmajor(stp->sd_vnode->v_rdev)];
		mep = mnp->mn_outp;
		while (mep) {
			if ((index == 0) || (index == mep->me_muxid))
				break;
			mep = mep->me_nextp;
		}
		if (!mep) {
			UNLOCK(&mux_mutex, pl);
			return(NULL);
		}
		LOCK(&Strinfo[DYN_LINKBLK].st_mutex, PLSTR);
		for (linkp = (struct linkinfo *) Strinfo[DYN_LINKBLK].st_head; linkp; linkp = linkp->li_next) {
			if ( (!linkp->li_lblk.l_qtop) &&
			    (mep->me_muxid == linkp->li_lblk.l_index)) {
				UNLOCK(&Strinfo[DYN_LINKBLK].st_mutex, PLSTR);
				UNLOCK(&mux_mutex, pl);
				return(linkp);
			}
		}
		UNLOCK(&Strinfo[DYN_LINKBLK].st_mutex, PLSTR);
		UNLOCK(&mux_mutex, pl);
	}
	return(NULL);
}

/* 
 * queue_t *
 * getendq(queue_t *q)
 *	Given a queue ptr, follow the chain of q_next pointers until you reach
 *	the last queue on the chain and return it.  Skip the uniplexor if you
 *	find it.
 *
 * Calling/Exit State:
 *	Assumes stream configuration can not change (i.e. sd_plumb held or
 *	we're in a close)
 */

queue_t *
getendq(queue_t *q)
{
	ASSERT( q!= NULL);
	while (SAMESTR(q)) {
		if (q->q_next->q_qinfo->qi_minfo->mi_idnum == UNI_ID)
			q = q->q_next->q_ptr;
		else
			q = q->q_next;
	}
	return(q);
}

/*
 * int
 * munlink(stdata_t *stp, struct linkinfo *linkp, int flag, cred_t *crp,
 *	int *rvalp)
 *	Unlink a multiplexor link.  Stp is the controlling stream for the
 *	link, fpdown is the file pointer for the lower stream, and
 *	linkp points to the link's entry in the linkinfo table.
 *
 * Calling/Exit State:
 *	Assumes sd_plumb held or that we are in a close.  Assumes sd_mutex
 *	unlocked.
 */

int
munlink(stdata_t *stp, struct linkinfo *linkp, int flag, cred_t *crp, int *rvalp)
{
	pl_t pl;
	struct strioctl strioc;
	struct stdata *stpdown;
	queue_t *rq;
	queue_t *q;
	int error;
	int renab;
	int nrband;
	int i;
	qband_t *qbp;

	error = 0;
	if ((flag & LINKTYPEMASK) == LINKNORMAL)
		strioc.ic_cmd = I_UNLINK;
	else
		strioc.ic_cmd = I_PUNLINK;
	strioc.ic_timout = 0;
	strioc.ic_len = sizeof(struct linkblk);
	strioc.ic_dp = (char *)&linkp->li_lblk;
	
	error = strdoioctl(stp, &strioc, NULL, K_TO_K, STRLINK, crp, rvalp);

	/*
	 * If there was an error and this is not called via strclose, 
	 * return to the user.  Otherwise, pretend there was no error 
	 * and close the link.  
	 */
	if (error) {
		if (flag & LINKCLOSE) {
			/*
			 *+ As part of the close system call on a stream
			 *+ device, all the streams linked to it must be
			 *+ unlinked.  For this purpose, an I_UNLINK ioctl
			 *+ message is sent down the control stream.  This
			 *+ ioctl message should not fail, but it has.  This
			 *+ failure will be ignored and the close call will
			 *+ be processed anyway.
			 */
			cmn_err(CE_CONT, "KERNEL: munlink: could not perform unlink ioctl, closing anyway\n");
			pl = LOCK(stp->sd_mutex, PLSTR);
			stp->sd_flag &= ~(STRDERR|STWRERR); /* allows strdoioctl() to work */
			UNLOCK(stp->sd_mutex, pl);
		} else
			return(error);
	}

	stpdown = linkp->li_fpdown->f_vnode->v_stream;
	if (stpdown->sd_wrq->q_next->q_qinfo->qi_minfo->mi_idnum == UNI_ID) {
		/* uniplexor on top, get rid of it */
		qdetach(RD(stpdown->sd_wrq->q_next), 1, 0, crp);
	}
	rq = RD(stpdown->sd_wrq);
	pl = LOCK(stpdown->sd_mutex, PLSTR);
	freezeprocs(rq);
	if (stpdown->sd_wrq->q_flag & QUP) {
		/*
		 * This was under a UP mux.  Clean up time.
		 */
		stpdown->sd_wrq->q_flag &= ~(QUP|QBOUND);
		RD(stpdown->sd_wrq)->q_flag &= ~(QUP|QBOUND);
		stpdown->sd_cpu = NULL;
	}
	setq(rq, &strdata, &stwdata);
	rq->q_ptr = (caddr_t)stpdown;
	WR(rq)->q_ptr = (caddr_t)stpdown;
	rq->q_putp = rq->q_qinfo->qi_putp;
	WR(rq)->q_putp = WR(rq)->q_qinfo->qi_putp;
	if ((flag & LINKNOEDGE) == 0)
		/* only remove edge if it was there in the first place */
		mux_rmvedge(stp, linkp->li_lblk.l_index);

	/*
	 * Unfreeze head queue-pair.  Reset the read side flow-control.  The
	 * write side reset is not needed since that is under the stream
	 * head's control.
	 */

	renab = 0;
	bzero((caddr_t)l.qbf, NBAND);
	if (rq->q_flag & QWANTW) {
		renab = 1;
		l.qbf[0] = 1;
	}
	nrband = (int)rq->q_nband;
	for (i = 1, qbp = rq->q_bandp; i <= nrband; i++) {
		if (qbp->qb_flag & QB_WANTW) {
			renab = 1;
			l.qbf[i] = 1;
		}
		qbp = qbp->qb_next;
	}
	if (renab) {
		q = backq_l(RD(stpdown->sd_wrq));
		for (; q && !q->q_qinfo->qi_srvp; q = backq_l(q))
			;
		if (q) {
			qenable_l(q);
			for (i = 0; i <= nrband; i++) {
				if (l.qbf[i])
					setqback(q, i);
			}
		}
	}

	stpdown->sd_flag &= ~STPLEX;
	UNLOCK(stpdown->sd_mutex, pl);
	/* only close in normal case (LINKNOEDGE is a startup error case) */
	if ((flag & LINKNOEDGE) == 0)
		closef(linkp->li_fpdown);
	lbfree(linkp);
	return(0);
}

/*
 * int
 * munlinkall(stdata_t *stp, int flag, cred_t *crp, int *rvalp)
 *	Unlink all multiplexor links for which stp is the controlling stream.
 *
 * Calling/Exit State:
 *	Return 0, or a non-zero errno on failure.  No locking assumptions.
 */

int
munlinkall(stdata_t *stp, int flag, cred_t *crp, int *rvalp)
{
	struct linkinfo *linkp;
	int error;

	error = 0;
	while (linkp = findlinks(stp, 0, flag)) {
		if (error = munlink(stp, linkp, flag, crp, rvalp))
			return(error);
	}
	return(0);
}


/*
 * int
 * mux_addedge(stdata_t *upstp, stdata_t *lostp, int muxid)
 *	A multiplexor link has been made.  Add an edge to the directed
 *	graph.
 *
 * Calling/Exit State:
 *	Assumes mux_mutex not locked.  Returns 0 on success and an error
 *	number on failure.
 */

int
mux_addedge(stdata_t *upstp, stdata_t *lostp, int muxid)
{
	struct mux_node *np;
	struct mux_edge *ep;
	pl_t pl;
	major_t upmaj;
	major_t lomaj;

	upmaj = getmajor(upstp->sd_vnode->v_rdev);
	lomaj = getmajor(lostp->sd_vnode->v_rdev);
	np = &mux_nodes[upmaj];
	pl = LOCK(&mux_mutex, PLSTR);
	if (np->mn_outp) {
		ep = np->mn_outp;
		while (ep->me_nextp)
			ep = ep->me_nextp;
		if ((ep->me_nextp = (struct mux_edge *)
		    kmem_alloc(sizeof(struct mux_edge), KM_NOSLEEP)) == NULL) {
			UNLOCK(&mux_mutex, pl);
			/*
			 *+ Kernel could not allocate memory
			 */
			cmn_err(CE_WARN, "Can not allocate memory for mux_edge\n");
			return(EAGAIN);
		}
		ep = ep->me_nextp;
	} else {
		if ((np->mn_outp = (struct mux_edge *)
		    kmem_alloc(sizeof(struct mux_edge), KM_NOSLEEP)) == NULL) {
			UNLOCK(&mux_mutex, pl);
			/*
			 *+ Kernel could not allocate memory
			 */
			cmn_err(CE_WARN, "Can not allocate memory for mux_edge\n");
			return(EAGAIN);
		}
		ep = np->mn_outp;
	}
	ep->me_nextp = NULL;
	ep->me_muxid = muxid;
	ep->me_nodep = &mux_nodes[lomaj];
	UNLOCK(&mux_mutex, pl);
	return(0);
}

/*
 * void
 * mux_rmvedge(stdata_t *upstp, int muxid)
 *	A multiplexor link has been removed.  Remove the
 *	edge in the directed graph.
 *
 * Calling/Exit State:
 *	Assume mux_mutex unlocked.
 */

static void
mux_rmvedge(stdata_t *upstp, int muxid)
{
	struct mux_node *np;
	struct mux_edge *ep;
	struct mux_edge *pep;
	pl_t pl;
	major_t upmaj;

	pep = NULL;
	upmaj = getmajor(upstp->sd_vnode->v_rdev);
	np = &mux_nodes[upmaj];
	pl = LOCK(&mux_mutex, PLSTR);
	ASSERT (np->mn_outp != NULL);
	ep = np->mn_outp;
	while (ep) {
		if (ep->me_muxid == muxid) {
			if (pep)
				pep->me_nextp = ep->me_nextp;
			else
				np->mn_outp = ep->me_nextp;
			kmem_free(ep, sizeof(struct mux_edge));
			UNLOCK(&mux_mutex, pl);
			return;
		}
		pep = ep;
		ep = ep->me_nextp;
	}
#ifndef lint
	ASSERT(0);	/* should not reach here */
#endif
	UNLOCK(&mux_mutex, pl);
}

/*
 * void
 * setq(queue_t *rq, struct qinit *rinit, struct qinit *winit)
 *	Set the interface values for a pair of queues (qinit structure,
 *	packet sizes, water marks).
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked, if required.
 *
 * Remarks:
 *	setq is called in 2 different contexts.  When it is called on an
 *	already existing queue, sd_mutex must be held since we're playing
 *	around with q_*.  In stropen, however, the queues have just been
 *	created and are not referenced, so sd_mutex is not needed.  It
 *	is the caller's responsibility to hold/not hold the lock as needed.
 */

void
setq(queue_t *rq, struct qinit *rinit, struct qinit *winit)
{
	queue_t  *wq;

	wq = WR(rq);
	rq->q_qinfo = rinit;
	rq->q_hiwat = rinit->qi_minfo->mi_hiwat;
	rq->q_lowat = rinit->qi_minfo->mi_lowat;
	rq->q_minpsz = rinit->qi_minfo->mi_minpsz;
	rq->q_maxpsz = rinit->qi_minfo->mi_maxpsz;
	wq->q_qinfo = winit;
	wq->q_hiwat = winit->qi_minfo->mi_hiwat;
	wq->q_lowat = winit->qi_minfo->mi_lowat;
	wq->q_minpsz = winit->qi_minfo->mi_minpsz;
	wq->q_maxpsz = winit->qi_minfo->mi_maxpsz;
}

/*
 * int
 * strmakemsg(struct strbuf *mctl, int count, struct uio *uiop,
 *	      stdata_t *stp, long flag, mblk_t **mpp, int wroff)
 *	Make a protocol message given control and data buffers.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 */

int
strmakemsg(struct strbuf *mctl, int count, struct uio *uiop,
	stdata_t *stp, long flag, mblk_t **mpp, int wroff)
{
	mblk_t *mp;
	mblk_t *bp;
	caddr_t base;
	pl_t pl;
	int pri;
	int msgtype;
	int offlg;
	int error;
#ifdef STRPERF
	int stamp1;
	int stamp2;
#endif

	offlg = 0;
	error = 0;
	mp = NULL;
	*mpp = NULL;
	if (flag & RS_HIPRI)
		pri = BPRI_MED;
	else
		pri = BPRI_LO;

	/*
	 * Create control part of message, if any.
	 */
	if ((mctl != NULL) && (mctl->len >= 0)) {
		int ctlcount;
		int allocsz;

		if (flag & RS_HIPRI) 
			msgtype = M_PCPROTO;
		else 
			msgtype = M_PROTO;

		ctlcount = mctl->len;
		base = mctl->buf;

		/*
		 * Give modules a better chance to reuse M_PROTO/M_PCPROTO
		 * blocks by increasing the size to something more usable.
		 */
		allocsz = MAX(ctlcount, 64);

		/*
		 * Range checking has already been done; simply try
		 * to allocate a message block for the ctl part.
		 */
		while (!(bp = allocb(allocsz, pri))) {
			if (uiop->uio_fmode  & (FNDELAY|FNONBLOCK))
				return(EAGAIN);
			if (error = strwaitbuf(allocsz, pri, stp))
				return(error);
		}

#ifdef STRPERF
		stamp1 = castimer();
#endif
		bp->b_datap->db_type = (uchar_t) msgtype;
#ifdef STRPERF
		bp->b_stamp = castimer();
#endif
		if (uiop->uio_segflg == UIO_SYSSPACE) {
			bcopy(base, (caddr_t) bp->b_wptr, ctlcount);
		} else {
			if (copyin(base, (caddr_t) bp->b_wptr, ctlcount)) {
#ifdef STRPERF
				bp->b_copyin += castimer() - bp->b_stamp;
				bp->b_stamp = 0;
				bp->b_sh += castimer() - stamp1;
#endif
				freeb(bp);
				return(EFAULT);
			}
		}
#ifdef STRPERF
		bp->b_copyin += castimer() - bp->b_stamp;
		bp->b_stamp = 0;
#endif

		/*
		 * We could have slept copying in user pages.
		 * Recheck the stream head state (the other end
		 * of a pipe could have gone away).
		 */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & (STRHUP|STWRERR|STPLEX)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
			    stp->sd_werror);
			UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
			bp->b_sh += castimer() - stamp1;
#endif
			freeb(bp);
			return(error);
		}
		UNLOCK(stp->sd_mutex, pl);
		bp->b_wptr += ctlcount;
		mp = bp;
	}

	/*
	 * Create data part of message, if any.
	 */
	if (count >= 0) {
		int size;

		size = count + (offlg ? 0 : wroff);
		while ((bp = allocb(size, pri)) == NULL) {
			if (uiop->uio_fmode  & (FNDELAY|FNONBLOCK))
				return(EAGAIN);
			if (error = strwaitbuf(size, pri, stp)) {
#ifdef STRPERF
				mp->b_sh += castimer() - stamp1;
#endif
				freemsg(mp);
				return(error);
			}
		}
#ifdef STRPERF
		stamp2 = castimer();
#endif
		if (wroff && !offlg++ &&
		    (wroff < bp->b_datap->db_lim - bp->b_wptr)) {
			bp->b_rptr += wroff;
			bp->b_wptr += wroff;
		}
#ifdef STRPERF
		bp->b_stamp = castimer();
#endif
		if (((size = MIN(count, bp->b_datap->db_lim - bp->b_wptr)) > 0) &&
		    ((error = uiomove((caddr_t)bp->b_wptr, size, UIO_WRITE, uiop)) > 0)) {
#ifdef STRPERF
			bp->b_copyin += castimer() - bp->b_stamp;
			bp->b_stamp = 0;
			bp->b_sh += castimer() - stamp2;
#endif
			freeb(bp);
#ifdef STRPERF
			mp->b_sh += castimer() - stamp1;
#endif
			freemsg(mp);
			return(error);
		}
#ifdef STRPERF
		bp->b_copyin += castimer() - bp->b_stamp;
		bp->b_stamp = 0;
#endif

		/*
		 * We could have slept copying in user pages.
		 * Recheck the stream head state (the other end
		 * of a pipe could have gone away).
		 */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & (STRHUP|STWRERR|STPLEX)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
			    stp->sd_werror);
			UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
			bp->b_sh += castimer() - stamp2;
#endif
			freeb(bp);
#ifdef STRPERF
			mp->b_sh += castimer() - stamp1;
#endif
			freemsg(mp);
			return(error);
		}
		UNLOCK(stp->sd_mutex, pl);
		bp->b_wptr += size;
		count -= size;
		if (!mp) {
#ifdef STRPERF
			bp->b_sh += castimer() - stamp2;
#endif
			mp = bp;
		} else {
#ifdef STRPERF
			bp->b_sh += castimer() - stamp2;
			mp->b_sh += castimer() - stamp1;
#endif
			linkb(mp, bp);
		}

	}
	*mpp = mp;
	return(0);
}


/*
 * void
 * strwakebuf(stdata_t *stp)
 *	Called via the bufcall mechanism to awaken a process waiting for
 *	buffers.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 */

STATIC void
strwakebuf(stdata_t *stp)
{
	pl_t pl;

	pl = LOCK(stp->sd_mutex, PLSTR);
	if (SV_BLKD(stp->sd_waitbuf)) {
		UNLOCK(stp->sd_mutex, pl);
		SV_BROADCAST(stp->sd_waitbuf, 0);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}
}

/*
 * int
 * strwaitbuf(int size, int pri, stdata_t *stp)
 *	Wait for a buffer to become available.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.  Return non-zero errno
 *	if not able to wait, 0 if buffer is probably there.
 */

int
strwaitbuf(int size, int pri, stdata_t *stp)
{
	pl_t pl;
	toid_t id;

	pl = LOCK(stp->sd_mutex, PLSTR);
	if ((id = bufcall(size, pri, strwakebuf, (long) stp)) == (toid_t) 0) {
		UNLOCK(stp->sd_mutex, pl);
		return(ENOSR);
	}
	/* note: SV_WAIT_SIG returns at pl0 with no lock held */
	if (SV_WAIT_SIG(stp->sd_waitbuf, PRIMED, stp->sd_mutex) == B_FALSE) {
		unbufcall(id);
		return(EINTR);
	}
	unbufcall(id);
	return(0);
}


/*
 * int
 * strwaitq(stdata_t *stp, int flag, off_t count, int fmode, int *done)
 *	Wait for a read or write event to happen on a stream.
 *
 * Calling/Exit State:
 * 	Assumes sd_mutex locked.
 */

int
strwaitq(stdata_t *stp, int flag, off_t count, int fmode, int *done)
{
	pl_t pl;
	int slpflg;
	int errs;
	int error;
	mblk_t *mp;
	sv_t *svaddr;
	long *rd_count;
	lwp_t *lwpp;
#ifdef STRPERF
	int stamp;
#endif

	pl = getpl();
	error = 0;
	if (fmode & (FNDELAY|FNONBLOCK)) {
		if (!(flag & NOINTR))
			error = EAGAIN;
		*done = 1;
		return(error);
	}

	if ((flag & READWAIT) || (flag & GETWAIT)) {
		slpflg = RSLEEP;
		svaddr = stp->sd_read;
		errs = STRDERR|STPLEX;

		/*
		 * If any module downstream has requested read notification
		 * by setting SNDMREAD flag using M_SETOPTS, send a message
		 * down stream.
		 */
		if ((flag & READWAIT) && (stp->sd_flag & SNDMREAD)) {
			while ((mp = allocb(sizeof(long), BPRI_MED)) == NULL) {
				UNLOCK(stp->sd_mutex, pl);
				error = strwaitbuf(sizeof(long), PRIMED, stp);
				(void) LOCK(stp->sd_mutex, pl);
				if (error) {
					*done = 1;
					return(error);
				}
			}
#ifdef STRPERF
			stamp = castimer();
#endif
			mp->b_datap->db_type = M_READ;
			/* LINTED pointer alignment */
			rd_count = (long *)mp->b_wptr;
			*rd_count = count;
			mp->b_wptr += sizeof(long);
			/*
			 * Send the number of bytes requested by the
			 * read as the argument to M_READ.  Note: reference
			 * to sd_wrq is ok unlocked since it is invariant.
			 */
			UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
			mp->b_sh += castimer() - stamp;
#endif
			putnext(stp->sd_wrq, mp);
			/*
			 * If any data arrived, don't sleep.  Could have
			 * come up from putnext, or while lock was dropped.
			 */
			(void) LOCK(stp->sd_mutex, pl);
			mp = RD(stp->sd_wrq)->q_first;
#ifdef STRPERF
			stamp = castimer();
#endif
			while (mp) {
				if (!(mp->b_flag & MSGNOGET))
					break;
				mp = mp->b_next;
			}
			if (mp != NULL) {
				*done = 0;
#ifdef STRPERF
				mp->b_sh += castimer() - stamp;
#endif
				return(error);
			}
		}
	} else {
		slpflg = WSLEEP;
		/* don't need to lock reference, addr is invariant */
		svaddr = stp->sd_write;
		errs = STWRERR|STRHUP|STPLEX;
	}
	
	stp->sd_flag |= slpflg;
	/* note: SV_WAIT_SIG returns at pl0 with no lock held */
	if (SV_WAIT_SIG(svaddr, PRIMED, stp->sd_mutex) == B_FALSE) {
		(void) LOCK(stp->sd_mutex, pl);
		stp->sd_flag &= ~slpflg;
		if (!(flag & NOINTR))
			error = EINTR;
		*done = 1;
		return(error);
	}
	/*
	 * special code to handle a race between sockmod and the stream
	 * head.  If urgent data arrived, sockmod sends up (sometimes)
	 * an M_PROTO message that causes this return path.  Immediately
	 * thereafter, a SIGURG is sent.  This is effectively checking
	 * for that SIGURG to prevent the mark from being lost if more
	 * data has arrived in the interim.  If it is another signal,
	 * processing it should be safe since the data will persist.
	 */
	lwpp = u.u_lwpp;
	switch (issig(NULL)) {
	case ISSIG_STOPPED:
		break;
	case ISSIG_NONE:
		UNLOCK(&lwpp->l_mutex, pl);
		break;
	case ISSIG_SIGNALLED:
		(void) LOCK(stp->sd_mutex, pl);
		stp->sd_flag &= ~slpflg;
		if (!(flag & NOINTR))
			error = EINTR;
		*done = 1;
		return(error);
	}

	(void) LOCK(stp->sd_mutex, pl);
	if (stp->sd_flag & errs) {
		*done = 1;
		if (stp->sd_flag & STPLEX)
			return(EINVAL);
		else if (stp->sd_flag & STRHUP)
			/* for POSIX compatibility */
			return(EIO);
		else if (stp->sd_flag & STWRERR)
			return(stp->sd_werror);
		else
			return(stp->sd_rerror);
	}
	*done = 0;
	return(error);
}

/*
 * int
 * str2num(char **str)
 *	convert string to number, updating pointer
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

static int
str2num(char **str)
{
	int n;

	n = 0;
	for (; **str >= '0' && **str <= '9'; (*str)++)
		n = 10 * n + **str - '0';
	return(n);
}

/*
 * void
 * adjfmtp(char **str, mblk_t *bp, int bytes)
 *	Update canon format pointer with "bytes" worth of data.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

static void
adjfmtp(char **str, mblk_t *bp, int bytes)
{
	caddr_t addr;
	caddr_t lim;
	long num;

	addr = (caddr_t)bp->b_rptr;
	lim = addr + bytes;
	while (addr < lim) {
		switch (*(*str)++) {
		case 's':			/* short */
			addr = SALIGN(addr);
			addr = SNEXT(addr);
			break;
		case 'i':			/* integer */
			addr = IALIGN(addr);
			addr = INEXT(addr);
			break;
		case 'l':			/* long */
			addr = LALIGN(addr);
			addr = LNEXT(addr);
			break;
		case 'b':			/* byte */
			addr++;
			break;
		case 'c':			/* character */
			if ((num = str2num(str)) == 0) {
				while (*addr++)
					;
			} else
				addr += num;
			break;
		case 0:
			return;
		default:
			break;
		}
	}
}


/*
 * int
 * straccess(stdata_t *stp, enum jcaccess mode)
 *	Perform job control discipline access checks.
 *
 * Calling/Exit State:
 *	The 'stp' stream's sd_mutex must be held locked upon entry.
 *	The sd_mutex lock remains held upon return.  This lock however,
 *	was dropped and then subsequently reacquired if
 *	the lwp was stopped or signalled. In all other cases, the
 *	sd_mutex lock was held the entire time.
 *
 *	This function returns the following values:
 *	CTTY_STOPPED (-2): 
 *		If LWP was stopped.  This return allows the STREAMS
 *		code to examine the stream for errors e.g. STRDERR, STWRERR,
 *		and STRHUP, etc  and take appropriate action.
 *	CTTY_EOF (-1): 
 *		if the I/O operation should return end-of-file.
 *	CTTY_OK (0): 
 *		if it is okay to perform the I/O operation.
 *	errno (>0):  
 *		if the I/O operation should fail with an errno code
 *		return (in this case, the return value identifies the
 *		errno to be returned).
 *
 * Remarks:
 *	This function can cause the calling LWP to enter the stopped state.
 *	As a consequence, the only spin lock that can be held upon entry to
 *	this function, is the sd_mutex lock of the stream. 
 */

/*ARGSUSED*/
int
straccess(stdata_t *stp, enum jcaccess mode)
{
	proc_t *p;
	sess_t *sp;
	proc_t *pgrp;
	lwp_t *lwpp;
	pl_t pl;


	ASSERT(LOCK_OWNED(stp->sd_mutex)); 

	p = u.u_procp;
	lwpp = u.u_lwpp;
loop:
	sp = p->p_sessp;

	/*
	 * If this is not the calling process's controlling terminal
	 * or the calling process is already in the foreground then 
	 * allow access.
	 * Note that we do not have to check for FIFO anymore because
	 * sd_sessp will never be set for FIFO stream.
	 */
	if ((sp != stp->sd_sessp) || (p->p_pgidp == stp->sd_pgidp)) {
		return (CTTY_OK);
	}

	/* else, retest with p_sess_mutex held */

	pl = LOCK(&p->p_sess_mutex, PL_SESS);
	sp = p->p_sessp;
	if ((sp != stp->sd_sessp) || (p->p_pgidp == stp->sd_pgidp)) {
		UNLOCK(&p->p_sess_mutex, pl);
		return (CTTY_OK);
	}

	/*
	 * We are in the background process group.
	 * Check to see if the controlling terminal has been
	 * relinquished.
	 */
	(void)LOCK(&sp->s_mutex, PL_SESS);
	if (sp->s_vp == NULL) {
		/*
		 * IEEE POSIX 1003.1: 7.1.1.3
		 * When a controlling process terminates, the
		 * controlling terminal is disassociated from the
		 * current session, allowing it to be acquired by a new
		 * session leader.  Subsequent access to the terminal
		 * by other processes in the earlier session may be
		 * denied, with attempts to access the terminal treated
		 * as if modem disconnect had been sensed.
		 */
		UNLOCK(&sp->s_mutex, PL_SESS);
		UNLOCK(&p->p_sess_mutex, pl);
		switch (mode) {

		case JCREAD:
		case JCGETP:
			/*
			 * POSIX 1003.1: 7.1.1.10 (Modem disconnect)
			 * ....Any subsequent read from the terminal
			 * device returns with an end-of-file indication
			 * until the device is closed.  Thus, processes
			 * that read a terminal file and test for end-
			 * of-file can terminate properly after a
			 * disconnect.
			 */
			return (CTTY_EOF);

		case JCWRITE:
		case JCSETP:
			/*
			 * IEEE POSIX 1003.1: 7.1.1.10 (Modem disconnect).
			 * ...Any subsequent write() to the terminal device
			 * returns -1, with errno set to [EIO], until the
			 * device is closed.
			 */
			return (EIO);
		}
	}

	/* The calling lwp is in background process group */

	switch (mode) {

	case JCREAD:
		/*
		 * IEEE POSIX 1003.1: 7.1.1.4:
		 * For those implementations that support job control,
		 * any attempts by a process in a background process
		 * group to read from its controlling terminal shall
		 * cause its process group to be sent a SIGTTIN signal
		 * unless one of the following special cases apply:
		 * If the reading process is ignoring or blocking the
		 * SIGTTIN signal, or if the process group of the
		 * reading process is orphaned, the read() returns -1
		 * with errno set to [EIO] and no signal is sent.
		 *
		 */
		(void)LOCK(&p->p_mutex, PLHI);
		if (sigismember(&p->p_sigignore, SIGTTIN) ||
		    sigismember(&lwpp->l_sigheld, SIGTTIN) ||
		    (p->p_flag & P_PGORPH)) {
			UNLOCK(&p->p_mutex, PL_SESS);
			UNLOCK(&sp->s_mutex, PL_SESS);
			UNLOCK(&p->p_sess_mutex, pl);
			return (EIO);
		}
		/*
		 * Signal the process with SIGTTIN; then invoke issig() to
		 * take the action and broadcast SIGTTIN to the process group.
		 * This saves additional lock round trips and duplication of
		 * issig() code.
		 */
		(void)sigtolwp_l(lwpp, SIGTTIN, (sigqueue_t *)NULL);
		UNLOCK(&p->p_mutex, PL_SESS);
		for(pgrp = p->p_pglinkb; pgrp != p; pgrp = pgrp->p_pglinkb) 
			(void)sigtoproc(pgrp, SIGTTIN, (sigqueue_t*)NULL);
		break;

	case JCWRITE:
	case JCSETP:
		/*
		 * IEEE POSIX 1003.1: 7.1.1.4:
		 * Attempts by a process in a background process
		 * group to write to its controlling terminal shall
		 * cause its process group to be sent a SIGTTOU signal
		 * unless one of the following special cases apply:
		 * If TOSTOP is not set, or if TOSTOP is set and the
		 * process is ignoring or blocking the SIGTTOU signal,
		 * the process is allowed to write to the terminal and
		 * the SIGTTOU signal is not sent.  If TOSTOP is set,
		 * and the process group of the writing process is
		 * orphaned, and the writing process is not ignoring
		 * or blocking SIGTTOU, the write() returns -1 with
		 * errno set to [EIO], and no signal is sent.
		 */
		(void)LOCK(&p->p_mutex, PLHI);
		if ((mode == JCWRITE && !(stp->sd_flag & STRTOSTOP)) ||
		    sigismember(&p->p_sigignore, SIGTTOU) ||
		    sigismember(&lwpp->l_sigheld, SIGTTOU)) {
			UNLOCK(&p->p_mutex, PL_SESS);
			UNLOCK(&sp->s_mutex, PL_SESS);
			UNLOCK(&p->p_sess_mutex, pl);
			return (CTTY_OK);
		}

		/*
		 * IEEE POSIX 1003.1: 7.1.1.4:
		 * If TOSTOP is set, and the process group of the
		 * writing process is orphaned, and the writing process
		 * is not ignoring or blocking SIGTTOU, the write()
		 * returns -1 with errno set to [EIO], and no signal
		 * is sent.
		 *
		 * NOTES:
		 *  TOSTOP is set, and the writing process is not
		 *  ignoring or blocking SIGTTOU if we get here.
		 */
		if (p->p_flag & P_PGORPH) {
			UNLOCK(&p->p_mutex, PL_SESS);
			UNLOCK(&sp->s_mutex, PL_SESS);
			UNLOCK(&p->p_sess_mutex, pl);
			return (EIO);
		}
		/*
		 * Signal the process with SIGTTOU; then invoke issig() to
		 * take the action and broadcast SIGTTOU to the process group.
		 * This saves additional lock round trips and duplication of
		 * issig() code.
		 */
		(void)sigtolwp_l(lwpp, SIGTTOU, (sigqueue_t *)NULL);
		UNLOCK(&p->p_mutex, PL_SESS);
		for(pgrp = p->p_pglinkb; pgrp != p; pgrp = pgrp->p_pglinkb) 
			(void)sigtoproc(pgrp, SIGTTOU, (sigqueue_t*)NULL);
		break;

	case JCGETP:
		UNLOCK(&sp->s_mutex, PL_SESS);
		UNLOCK(&p->p_sess_mutex, pl);
		return (CTTY_OK);

	default:
		/*
		 *+ The straccess() routine was invoked with an invalid
		 *+ mode argument.
		 */
		cmn_err(CE_PANIC,
			"Invalid access mode type given to straccess()");
	}

	UNLOCK(&sp->s_mutex, PL_SESS);
	UNLOCK(&p->p_sess_mutex, pl);

	switch (issig(stp->sd_mutex)) {
	case ISSIG_STOPPED:		/* stopped for job-control */
		(void)LOCK(stp->sd_mutex, pl);
		return (CTTY_STOPPED);

	case ISSIG_SIGNALLED:		/* got a signal */
		(void)LOCK(stp->sd_mutex, pl);
		return (EINTR);

	case ISSIG_NONE:		/* no signal found */
		/*
		 * Another LWP in the process changed the registration of the
		 * SIGTT{IN,OUT} signal at the very same time that we were
		 * trying to stop.  The application gets what they deserve
		 * for playing these shenanigans.
		 *
		 *	sd_mutex remains held.
		 */
		UNLOCK(&lwpp->l_mutex, pl);
		goto loop;
	default:
		/*
		 *+ The issig() function was invoked from the straccess()
		 *+ STREAMS function, and issig() returned an invalid
		 *+ return code.
		 */
		cmn_err(CE_PANIC,
			"Incorrect issig() return value in straccess()");
	}
	/* NOTREACHED */
}


/*
 * int
 * stralloctty(stdata_t *stp)
 *	Allocate the given stream as a controlling terminal.
 *	This function is called from the stropen() and strioctl()
 *	whenever a streams driver declares its intention for the stream
 *	to became a controlling terminal.
 *
 * Calling/Exit State:
 *	The stream mutex lock (sd_mutex) must be held by the caller.
 *	The sd_mutex lock remains held upon return.
 *	This function returns 1 if it allocate the given stream as a 
 *	controlling terminal. Otherwise, 0 is returned.
 */

int
stralloctty(struct stdata *stp)
{
	proc_t *p;
	sess_t *sp;
	pl_t pl;

	ASSERT(stp->sd_flag & STRISTTY);

	p = u.u_procp;
	/* Should we check the sd_flag in streamio.c before calling? */ 
	if ((stp->sd_flag & (STRHUP|STRDERR|STWRERR|STPLEX)) == 0
	  && stp->sd_sessp == NULL) {		/* not allocated as ctty */
		pl = LOCK(&p->p_sess_mutex, PL_SESS);
		sp = p->p_sessp;
		/* session leader and no controlling terminal */
		if (sp->s_sidp == p->p_pidp && sp->s_vp == NULL) {
			(void)LOCK(&sp->s_mutex, PL_SESS);
			ASSERT(sp->s_vp == NULL); 
			ASSERT(stp->sd_pgidp == NULL);
			stp->sd_sessp = sp;
			stp->sd_pgidp = sp->s_sidp;
			pid_hold(stp->sd_pgidp);
			/*
			 * Associate the common vnode of the 
			 * controlling terminal with the session 
			 * structure.
			 */
			alloctty(stp->sd_vnode);
			UNLOCK(&sp->s_mutex, PL_SESS);
			UNLOCK(&p->p_sess_mutex, pl);
			return (1);
		}
		UNLOCK(&p->p_sess_mutex, pl);
	}
	return (0);
}

 
/* 
 *
 * void 
 * strfreectty(struct stdata *stp)
 *	Free the given stream as a controlling termial.
 *
 * Calling/Exit State:
 *	The session of the caller must be locked upon entry.  The
 *	session lock is unlocked upon return.
 *
 * Remarks:
 *	This function is called by the exiting leader of a session.
 *	This function returns at PLBASE.
 * Notes: (TODO)
 *	We do not wakeup any readers/writers who might be in middle
 *	of read or write.  The same device may be established as
 *	a controlling device by other process and now these processes
 *	will interfere with the new session.  Need to resolve this once
 *	TP design is understood for ES/MP.
 */
void
strfreectty(struct stdata *stp)
{
	sess_t *sp = u.u_procp->p_sessp;
	struct pid *pgidp;

	ASSERT(LOCK_OWNED(&sp->s_mutex));

	/*
	 * Send SIGHUP to the stream's foreground process group if it is
	 * in the same session as the exiting process.
	 */
	if (stp->sd_pgidp->pid_pgprocs != NULL &&
	    stp->sd_pgidp->pid_pgprocs->p_sessp == sp)
		pgsignal(stp->sd_pgidp, SIGHUP);
	UNLOCK(&sp->s_mutex, PLBASE);

	/*
	 * Finish cleaning up session state.
	 */
	LOCK(stp->sd_mutex, PLSTR);
	stp->sd_sessp = NULL;
	pgidp = stp->sd_pgidp;
	stp->sd_pgidp = NULL;
	/* 
	 * Do not understand why we are doing this -- needs further 
	 * investigation.  Will be resolved with TP design.
	 */
	if (!(stp->sd_flag & STRHUP)) 
		strhup(stp);
	UNLOCK(stp->sd_mutex, PLBASE);

	pid_rele(pgidp);		/* release FGP-ID ref */
}

/*
 * int
 * xmsgsize(mblk_t *mp, int fromhead)
 *	Return size of message of block type (bp->b_datap->db_type)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	If fromhead is non-zero, then start at beginning of message.
 *	If fromhead is zero, then start at end of message.
 */

int
xmsgsize(mblk_t *mp, int fromhead)
{
	mblk_t *bp;
	unsigned char type;
	int count;
	mblk_t *endbp;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif
	
	if (fromhead) {
		bp = mp;
		type = bp->b_datap->db_type;
		count = 0;
		for (; bp; bp = bp->b_cont) {
			if (type != bp->b_datap->db_type)
				break;
			ASSERT(bp->b_wptr >= bp->b_rptr);
			count += bp->b_wptr - bp->b_rptr;
		}
	} else {
		for (bp = mp; bp->b_cont; bp = bp->b_cont)
			;
		type = bp->b_datap->db_type;
		ASSERT(bp->b_wptr >= bp->b_rptr);
		count = bp->b_wptr - bp->b_rptr;
		endbp = bp;
		while (bp != mp) {
			for (bp = mp; bp->b_cont != endbp; bp = bp->b_cont)
				;
			if (type != bp->b_datap->db_type)
				break;
			ASSERT(bp->b_wptr >= bp->b_rptr);
			count += bp->b_wptr - bp->b_rptr;
			endbp = bp;
		}
	}
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
	return(count);
}

/*
 * struct stdata *
 * shalloc(queue_t *qp)
 *	Allocate a stream head.
 *
 * Calling/Exit State:
 *	Assumes st_sleep not locked.
 *
 * Description:
 *	This is very subtle.  Before the stream head is allocated, you can
 *	race because the object to synchronize on has not been created yet.
 *	Because of that, multiple stream heads for the same device can be
 *	created.  This is handled in stropen by serializing the setting
 *	of v_stream, which binds the vnode to a stream.  The first one in
 *	wins and the others free any allocated resources and try again.
 *	Setting STWOPEN keeps any of these racers from continuing until
 *	we've finished.
 */

struct stdata *
shalloc(queue_t *qp)
{
	stdata_t *stp;
	struct shinfo *shp;

	shp = (struct shinfo *)kmem_zalloc(sizeof(struct shinfo), KM_SLEEP);
	stp = (stdata_t *)shp;
	stp->sd_mutex = LOCK_ALLOC(STR_HIER, PLSTR, &strhead_lkinfo, KM_SLEEP);
	stp->sd_plumb = SLEEP_ALLOC(0, &plumb_lkinfo, KM_SLEEP);
	stp->sd_open = SV_ALLOC(KM_SLEEP);
	stp->sd_freeze = SV_ALLOC(KM_SLEEP);
	stp->sd_read = SV_ALLOC(KM_SLEEP);
	stp->sd_write = SV_ALLOC(KM_SLEEP);
	stp->sd_timer = SV_ALLOC(KM_SLEEP);
	stp->sd_timer2 = SV_ALLOC(KM_SLEEP);
	stp->sd_timer3 = SV_ALLOC(KM_SLEEP);
	stp->sd_waitbuf = SV_ALLOC(KM_SLEEP);
	stp->sd_upblock = SV_ALLOC(KM_SLEEP);
	stp->sd_pollist = phalloc(KM_SLEEP);
	stp->sd_pollist->ph_events = 0;
	stp->sd_pollist->ph_list = NULL;
	/* By default, we are a UP-friendly stream */
	stp->sd_flag = STWOPEN|UPF;
	stp->sd_wrq = WR(qp);
	stp->sd_closetime = STRTIMOUT * HZ;
	qp->q_str = stp;
	WR(qp)->q_str = stp;
	MET_STREAM(1);
	
	*(Strcount_local + l.eng_num) += sizeof(struct shinfo);

	SLEEP_LOCK(&Strinfo[DYN_STREAM].st_sleep, PRIMED);
	Strinfo[DYN_STREAM].st_cnt++;
	shp->sh_next = (struct shinfo *) Strinfo[DYN_STREAM].st_head;
	if (shp->sh_next)
		shp->sh_next->sh_prev = shp;
	shp->sh_prev = NULL;
	Strinfo[DYN_STREAM].st_head = (void *) shp;
	SLEEP_UNLOCK(&Strinfo[DYN_STREAM].st_sleep);
	return(stp);
}

/*
 * void
 * shfree(stdata_t *stp, int freemutex)
 *	Free a stream head.
 *
 * Calling/Exit State:
 *	Assumes st_sleep not locked.
 */

void
shfree(stdata_t *stp, int freemutex)
{
	struct shinfo *shp;

	SLEEP_LOCK(&Strinfo[DYN_STREAM].st_sleep, PRIMED);
	shp = (struct shinfo *) stp;
	if (shp->sh_prev == NULL) {
		if (shp->sh_next)
			shp->sh_next->sh_prev = NULL;
		Strinfo[DYN_STREAM].st_head = (void *) shp->sh_next;
	}
	else {
		if (shp->sh_next)
			shp->sh_next->sh_prev = shp->sh_prev;
		shp->sh_prev->sh_next = shp->sh_next;
	}
	Strinfo[DYN_STREAM].st_cnt--;
	SLEEP_UNLOCK(&Strinfo[DYN_STREAM].st_sleep);
	if (freemutex)
		LOCK_DEALLOC(stp->sd_mutex);
	SLEEP_DEALLOC(stp->sd_plumb);
	SV_DEALLOC(stp->sd_open);
	SV_DEALLOC(stp->sd_freeze);
	SV_DEALLOC(stp->sd_read);
	SV_DEALLOC(stp->sd_write);
	SV_DEALLOC(stp->sd_timer);
	SV_DEALLOC(stp->sd_timer2);
	SV_DEALLOC(stp->sd_timer3);
	SV_DEALLOC(stp->sd_waitbuf);
	SV_DEALLOC(stp->sd_upblock);
	phfree(stp->sd_pollist);
	kmem_free(shp, sizeof(struct shinfo));

	*(Strcount_local + l.eng_num) -= sizeof(struct shinfo);

	MET_STREAM(-1);
	return;
}

/*
 * queue_t *
 * allocq()
 *	Allocate a pair of queues
 *
 * Calling/Exit State:
 *	Assumes st_mutex not locked.
 *	Queue created initially to putnext around this module.
 */

queue_t *
allocq(void)
{
	pl_t pl;
	queue_t *qp;
	struct queinfo *qip;

	/* allocate a queinfo struct (which contains two queues) */
	qip = (struct queinfo *) kmem_zalloc(sizeof(struct queinfo), KM_SLEEP);
	qp = (queue_t *) qip;
	qp->q_flag = QUSE | QREADR;
	qp->q_putp = putnext;
	WR(qp)->q_flag = QUSE;
	WR(qp)->q_putp = putnext;
	/* for accounting purposes, count as 2 */
	MET_STRQUE(2);

	*(Strcount_local + l.eng_num) += sizeof(struct queinfo);

	pl = LOCK(&Strinfo[DYN_QUEUE].st_mutex, PLSTR);
	Strinfo[DYN_QUEUE].st_cnt += 2;
	qip->qu_next = (struct queinfo *) Strinfo[DYN_QUEUE].st_head;
	if (qip->qu_next)
		qip->qu_next->qu_prev = qip;
	qip->qu_prev = NULL;
	Strinfo[DYN_QUEUE].st_head = (void *) qip;
	UNLOCK(&Strinfo[DYN_QUEUE].st_mutex, pl);
	return(qp);
}

/*
 * void
 * freeq(queue_t *qp)
 *	Free a pair of queues.
 *
 * Calling/Exit State:
 *	Assumes st_mutex unlocked.
 */

void
freeq(queue_t *qp)
{
	struct queinfo *qip;
	qband_t *qbp;
	qband_t *nqbp;
	pl_t pl;

	qbp = qp->q_bandp;
	while (qbp) {
		nqbp = qbp->qb_next;
		freeband(qbp);
		qbp = nqbp;
	}
	qbp = WR(qp)->q_bandp;
	while (qbp) {
		nqbp = qbp->qb_next;
		freeband(qbp);
		qbp = nqbp;
	}
	qip = (struct queinfo *) qp;
	pl = LOCK(&Strinfo[DYN_QUEUE].st_mutex, PLSTR);
	if (qip->qu_prev == NULL) {
		if (qip->qu_next)
			qip->qu_next->qu_prev = NULL;
		Strinfo[DYN_QUEUE].st_head = (void *) qip->qu_next;
	}
	else {
		if (qip->qu_next)
			qip->qu_next->qu_prev = qip->qu_prev;
		qip->qu_prev->qu_next = qip->qu_next;
	}
	Strinfo[DYN_QUEUE].st_cnt -= 2;
	UNLOCK(&Strinfo[DYN_QUEUE].st_mutex, pl);
	kmem_free(qip, sizeof(struct queinfo));

	*(Strcount_local + l.eng_num) -= sizeof(struct queinfo);

	/* for accounting purposes, count individually */
	MET_STRQUE(-2);
	return;
}

/*
 * qband_t *
 * allocband()
 *	Allocate a qband structure.
 *
 * Calling/Exit State:
 *	Assumes st_mutex unlocked.
 */

qband_t *
allocband(void)
{
	pl_t pl;
	struct qbinfo *qbip;

	if (!mem_resv_check())
		return(NULL);
	if ((qbip = (struct qbinfo *) kmem_zalloc(sizeof(struct qbinfo), KM_NOSLEEP)) == NULL) {
		return(NULL);
	}

	*(Strcount_local + l.eng_num) += sizeof(struct qbinfo);

	pl = LOCK(&Strinfo[DYN_QBAND].st_mutex, PLSTR);
	Strinfo[DYN_QBAND].st_cnt++;
	qbip->qbi_next = (struct qbinfo *) Strinfo[DYN_QBAND].st_head;
	if (qbip->qbi_next)
		qbip->qbi_next->qbi_prev = qbip;
	qbip->qbi_prev = NULL;
	Strinfo[DYN_QBAND].st_head = (void *) qbip;
	UNLOCK(&Strinfo[DYN_QBAND].st_mutex, pl);
	return((qband_t *) qbip);
}

/*
 * void
 * freeband(qband_t *qbp)
 *	Free a qband structure.
 *
 * Calling/Exit State:
 *	Assumes st_mutex unlocked.
 */

STATIC void
freeband(qband_t *qbp)
{
	pl_t pl;
	struct qbinfo *qbip;

	qbip = (struct qbinfo *) qbp;
	pl = LOCK(&Strinfo[DYN_QBAND].st_mutex, PLSTR);
	if (qbip->qbi_prev == NULL) {
		if (qbip->qbi_next)
			qbip->qbi_next->qbi_prev = NULL;
		Strinfo[DYN_QBAND].st_head = (void *) qbip->qbi_next;
	}
	else {
		if (qbip->qbi_next)
			qbip->qbi_next->qbi_prev = qbip->qbi_prev;
		qbip->qbi_prev->qbi_next = qbip->qbi_next;
	}
	Strinfo[DYN_QBAND].st_cnt--;
	UNLOCK(&Strinfo[DYN_QBAND].st_mutex, pl);
	kmem_free(qbip, sizeof(struct qbinfo));
	
	*(Strcount_local + l.eng_num) -= sizeof(struct qbinfo);

	return;
}

/*
 * struct strevent *
 * sealloc(int slpflag)
 *	Allocate a stream event cell.  May sleep if specified.
 *
 * Calling/Exit State:
 *	Assumes cm_mutex unlocked.  Assumes st_mutex unlocked.  Returns a
 *	pointer to a stream event cell on success, NULL on failure.
 *
 * Remarks:
 *	There is a little bit of duplicated code to optimize the locking.
 */

struct strevent *
sealloc(int slpflag)
{
	pl_t pl;
	struct seinfo *sep;
	int allocated;

	allocated = 0;
	FSPIN_LOCK(&cm_mutex);
	if (sefreelist) {
		sep = sefreelist;
		sefreelist = sep->s_next;
		FSPIN_UNLOCK(&cm_mutex);
		sep->s_strevent.se_lwpp = NULL;
		sep->s_strevent.se_events = 0;
		sep->s_strevent.se_next = NULL;
		sep->s_strevent.se_vp = NULL;
		sep->s_strevent.se_chain = NULL;
		sep->s_strevent.se_id = 0;
		MET_STREVENT(1);
		pl = LOCK(&Strinfo[DYN_STREVENT].st_mutex, PLSTR);
		sep->s_next = (struct seinfo *) Strinfo[DYN_STREVENT].st_head;
		if (sep->s_next)
			sep->s_next->s_prev = sep;
		sep->s_prev = NULL;
		Strinfo[DYN_STREVENT].st_head = (void *) sep;
		UNLOCK(&Strinfo[DYN_STREVENT].st_mutex, pl);
		return((struct strevent *) sep);
	}
	FSPIN_UNLOCK(&cm_mutex);
	if (sep = (struct seinfo *) kmem_zalloc(sizeof(struct seinfo), slpflag)) {
		allocated = 1;
		
		*(Strcount_local + l.eng_num) += sizeof(struct seinfo);

	} else {
		/*
		 * if we got here kmem_zalloc failed, which can only happen if
		 * slpflag == SE_NOSLP.
		 */
		ASSERT(slpflag == SE_NOSLP);
		/* use the cache for these */
		FSPIN_LOCK(&cm_mutex);
		if (secachep) {
			sep = secachep;
			secachep = secachep->s_next;
			FSPIN_UNLOCK(&cm_mutex);
			sep->s_strevent.se_lwpp = NULL;
			sep->s_strevent.se_events = 0;
			sep->s_strevent.se_next = NULL;
			sep->s_strevent.se_vp = NULL;
			sep->s_strevent.se_chain = NULL;
			sep->s_strevent.se_id = 0;
		} else {
			FSPIN_UNLOCK(&cm_mutex);
		}
		if (sep == NULL) {
			/* failed anyhow */
			MET_STREVENT_FAIL();
			return(NULL);
		}
	}
	MET_STREVENT(1);
	pl = LOCK(&Strinfo[DYN_STREVENT].st_mutex, PLSTR);
	sep->s_next = (struct seinfo *) Strinfo[DYN_STREVENT].st_head;
	if (sep->s_next)
		sep->s_next->s_prev = sep;
	sep->s_prev = NULL;
	Strinfo[DYN_STREVENT].st_head = (void *) sep;
	if (allocated)
		Strinfo[DYN_STREVENT].st_cnt++;
	UNLOCK(&Strinfo[DYN_STREVENT].st_mutex, pl);
	return((struct strevent *) sep);
}

/*
 * void
 * sefree(struct strevent *sep)
 *	Free a stream event cell
 *
 * Calling/Exit State:
 *	Assumes cm_mutex unlocked.  Assumes st_mutex unlocked.
 */

void
sefree(struct strevent *sep)
{
	pl_t pl;
	struct seinfo *seip;

	seip = (struct seinfo *) sep;
	pl = LOCK(&Strinfo[DYN_STREVENT].st_mutex, PLSTR);
	if (seip->s_prev == NULL) {
		if (seip->s_next)
			seip->s_next->s_prev = NULL;
		Strinfo[DYN_STREVENT].st_head = (void *) seip->s_next;
	}
	else {
		if (seip->s_next)
			seip->s_next->s_prev = seip->s_prev;
		seip->s_prev->s_next = seip->s_next;
	}
	UNLOCK(&Strinfo[DYN_STREVENT].st_mutex, pl);
	FSPIN_LOCK(&cm_mutex);
	if (seip >= &Secache[0] && seip <= &Secache[SECACHE]) {
		/* it's from the cache */
		seip->s_next = secachep;
		secachep = seip;
	}
	else {
		seip->s_next = sefreelist;
		sefreelist = seip;
	}
	FSPIN_UNLOCK(&cm_mutex);
	MET_STREVENT(-1);
	return;
}


/*
 * void bcrun(void)
 *	Run bufcalls.
 *
 * Calling/Exit State:
 *	Assumes bc_mutex unlocked.
 */

STATIC void
bcrun(void)
{
	pl_t pl;
	struct strevent *sep;
	int nevent;
	int i;
	struct bclist *bp;

	pl = LOCK(&bc_mutex, PLSTR);
	while (strbcflag) {
		strbcwait = strbcflag = 0;

		/*
		 * Run two iterations of bufcalls, first the bound ones, then
		 * the unbound ones.
		 */
		for (i = 0; i <= 1; i++) {
			if (i == 0)
				bp = &l.bcall;
			else
				bp = &bcall;

			/*
			 * count how many events are on the list
			 * now so we can check to avoid looping
			 * in low memory situations.  Note, since
			 * bc_mutex is held, unbufcall can not
			 * change the list out from under us.
			 */
			nevent = 0;
			for (sep = bp->bc_head; sep; sep = sep->se_next)
				nevent++;
			/*
			 * get estimate of available memory from kmem_avail().
			 * If there appears to be enough memory, go for it.
			 */
			while ( ((sep = bp->bc_head) != NULL) && nevent ) {
				--nevent;
				if (kmem_avail((size_t) sep->se_size,
					       KM_NOSLEEP)) {
					bp->bc_head = sep->se_next;
					strbcrunning = 1;
					if (i == 0)
						strbceng[l.eng_num] = 0;
					UNLOCK(&bc_mutex, pl);
					/*
					 * This window is why strbcrunning is
					 * necessary, it prevents an unbufcall
					 * from mistakenly returning while
					 * the bufcall it was shooting at is
					 * running.
					 */
					(*sep->se_func)(sep->se_arg);
					sefree(sep);
					pl = LOCK(&bc_mutex, PLSTR);
					strbcrunning = 0;
				}
				else {
					/*
					 * too big, try again later - note
					 * that nevent was decremented above
					 * so we won't retry this one on this
					 * iteration of the loop
					 */
					bp->bc_head = sep->se_next;
					sep->se_next = NULL;
					bp->bc_tail->se_next = sep;
					bp->bc_tail = sep;
				}
			}
			if (bp->bc_head) {
				/*
				 * still some bufcalls we couldn't do
				 * let kmem_free know
				 */
				if (i == 0) {
					strbcwait |= B_UP;
					strbceng[l.eng_num] = 1;
				} else {
					strbcwait |= B_MP;
				}
			} else {
				bp->bc_tail = NULL;
			}
		}
	}
	UNLOCK(&bc_mutex, pl);
}


/*
 * void runqueues(void)
 *	Scheduling loop for queue scheduling and bufcalls.  The
 *	bufcalls are run first.
 *
 * Calling/Exit State:
 *	Assumes svc_mutex unlocked.
 *
 * Description:
 *	When a queue is enabled via qenable_l, an instantiation of the
 *	streams scheduler is kicked off to run the service procedure.
 *	This work is done in this function.
 */

void
runqueues(void)
{
	pl_t pl_1;
	pl_t pl_2;
	queue_t *q;
	int tries;
	struct qsvc *svcp;

	tries = 0;
	/*
	 * heuristic - don't check under lock - bcrun can handle the case
	 * where another scheduler got there first, and if we "miss" one,
	 * we'll pick it up on the next invocation of the scheduler.  This
	 * optimizes the normal case of no bufcall's to do
	 */
	if (strbcflag)
		bcrun();

	for (;;) {
		/*
		 * Get next queue that is not already running.  Note
		 * that only one instance of the service procedure
		 * should run at any time.  Busy queues are in fact
		 * service procedures, that, while running, are re-
		 * enabled.  Give preference to bound queues.
		 */
		pl_1 = LOCK(&svc_mutex, PLSTR);
		if ((q = l.qsvc.qs_head) != NULL) {
			/* bound queues to run */
			svcp = &l.qsvc;
		} else {
			if ((q = qsvc.qs_head) == NULL) {
				/* nothing to do */
				UNLOCK(&svc_mutex, pl_1);
				return;
			}
			svcp = &qsvc;
			/*
			 * This can only happen on unbound streams (if a
			 * stream is bound, then the service procedure can
			 * not be running elsewhere.
			 */
			while (q->q_svcflag & QSVCBUSY) {
				if (q == qsvc.qs_tail) {
					/* all queues are busy */
					UNLOCK(&svc_mutex, pl_1);
					return;
				} else {
					q = q->q_link;
				}
			}
		}
		/* found one that isn't busy */
		svc_dequeue(q, svcp);

		q->q_svcflag &= ~QENAB;
		q->q_svcflag |= QSVCBUSY;
		Nsched++;
		UNLOCK(&svc_mutex, pl_1);
		(*q->q_qinfo->qi_srvp)(q);
		pl_1 = LOCK(q->q_str->sd_mutex, PLSTR);
		pl_2 = LOCK(&svc_mutex, PLSTR);
		q->q_svcflag &= ~QSVCBUSY;
		Nsched--;
		UNLOCK(&svc_mutex, pl_2);

		/* check for potential freeze */
		if (q->q_flag & QFREEZE) {
			UNLOCK(q->q_str->sd_mutex, pl_1);
			SV_SIGNAL(q->q_str->sd_freeze, 0);
		} else {
			UNLOCK(q->q_str->sd_mutex, pl_1);
		}

		/*
		 * Only allow a set limit of service procedures to be run
		 * at a time.  Then call setqsched() to see if there is more
		 * work to do.  If we're still running bound queues or we
		 * only have 1 processor, ignore the limit.
		 */
		if ((Nengine > 1) && (++tries == strnsched) && (svcp == &qsvc)) {
			setqsched(NULL);
			return;
		}
	}
}

#define BUMPENG(eng) {++eng; if (eng >= engine_Nengine) eng = engine;}

/*
 * void
 * setqsched(engine_t *engp)
 *	Kick off the scheduling mechanism.
 *
 * Calling/Exit State:
 *	Assumes no locks held.
 */

void
setqsched(engine_t *engp)
{
	struct engine *eng;
	struct engine *bestpick;

	/* optimization for 1 processor systems */
	if (Nengine == 1) {
		DISABLE();
		sendsoft(engine, STRSCHED);
		ENABLE();
		return;
	}

	/* quick check for qenable of a bound queue */
	if (engp) {
		DISABLE();
		sendsoft(engp, STRSCHED);
		ENABLE();
		return;
	}
	

	/* real work to do */
	RUNQUE_LOCK();

	/*
	 * if lastpick is likely to be busy bump it, else favour it
	 * Don't really need to lock since this is a heuristic.
	 */
	if (Nsched > 0) {
		BUMPENG(lastpick);
	}
	bestpick = eng = lastpick;
	/* -1 for idle engines */
	while (bestpick->e_pri != -1) {
		BUMPENG(eng);
		if (eng == lastpick) {
			break;
		}
		if ((eng->e_flags & E_NOWAY) == 0 &&
		    (eng->e_pri < bestpick->e_pri)) {
			bestpick = eng;
		}
	}
	if (bestpick == lastpick) {
		/* this engine has not been checked yet */
		if (bestpick->e_flags & E_NOWAY) {
			lastpick = l.eng;
		}
	} else {
		lastpick = bestpick;
	}
	sendsoft(lastpick, STRSCHED);
	RUNQUE_UNLOCK();
}




/* 
 * int
 * findmod(char *name)
 *	Find module
 * 
 * Calling/Exit State:
 *	No locking assumptions.  Return index into fmodsw or -1 if not found
 */

int
findmod(char *name)
{
	int i;
	int j;
	pl_t pl;

	/*
	 * The fmodsw table may contain NULL entries
	 */
	if (!*name)
		return(-1);

	pl = RW_RDLOCK(&mod_fmodsw_lock, PLSTR);
	for (i = 0; i < fmodswsz; i++) {
		for (j = 0; j < FMNAMESZ + 1; j++) {
			if (fmodsw[i].f_name[j] != name[j]) 
				break;
			if (name[j] == '\0') {
				RW_UNLOCK(&mod_fmodsw_lock, pl);
				return(i);
			}
		}
	}
	RW_UNLOCK(&mod_fmodsw_lock, pl);
	return(-1);
}


/*
 * void
 * setqback(queue_t *q, unsigned char pri)
 *	Set the QBACK or QB_BACK flag in the given queue for
 *	the given priority band.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex locked.
 */

void
setqback(queue_t *q, unsigned char pri)
{
	int i;
	qband_t *qbp;
	qband_t **qbpp;

	if (pri != 0) {
		if (pri > q->q_nband) {
			qbpp = &q->q_bandp;
			while (*qbpp)
				qbpp = &(*qbpp)->qb_next;
			while (pri > q->q_nband) {
				if ((*qbpp = allocband()) == NULL) {
					/*
					 *+ Kernel failed to allocate memory.
					 */
					cmn_err(CE_WARN, "setqback: can't allocate qband\n");
					return;
				}
				(*qbpp)->qb_hiwat = q->q_hiwat;
				(*qbpp)->qb_lowat = q->q_lowat;
				q->q_nband++;
				qbpp = &(*qbpp)->qb_next;
			}
		}
		qbp = q->q_bandp;
		i = pri;
		while (--i)
			qbp = qbp->qb_next;
		qbp->qb_flag |= QB_BACK;
	} else {
		q->q_flag |= QBACK;
	}
}

/*
 * int
 * strcopyin(caddr_t from, caddr_t to, unsigned int len, char *fmt, int copyflag)
 *	Copy data
 *
 * Calling/Exit State:
 *	Assumes no locks held (routine may sleep)
 */

/* ARGSUSED */
int
strcopyin(caddr_t from, caddr_t to, unsigned int len, char *fmt, int copyflag)
{
	if (copyflag == U_TO_K) {
		if (ncopyin(from, to, len, fmt))
			return(EFAULT);
	} else {
		ASSERT(copyflag == K_TO_K);
		bcopy(from, to, len);
	}
	return(0);
}

/*
 * int
 * strcopyout(caddr_t from, caddr_t to, unsigned int len, char *fmt, int copyflag)
 *	Copy data from "from" to "to", where "to" can be either user or
 *	kernel address space.
 *
 * Calling/Exit State:
 *	Assumes no locks held (routine may sleep).  Returns 0 on
 *	success, EFAULT on failure.
 */

/* ARGSUSED */
int
strcopyout(caddr_t from, caddr_t to, unsigned int len, char *fmt, int copyflag)
{
	if (copyflag == U_TO_K) {
		if (ncopyout(from, to, len, fmt))
			return(EFAULT);
	} else {
		ASSERT(copyflag == K_TO_K);
		bcopy(from, to, len);
	}
	return(0);
}



/*
 * void
 * strsignal(stdata_t *stp, int sig, long band)
 *	Send a signal baseed on the signal.
 *
 * Calling/Exit State:
 *	The stream's 'stp' sd_mutex must be held upon entry.
 *	The sd_mutex remains held upon exit. 
 */

void
strsignal(stdata_t *stp, int sig, long band)
{
	struct pid *pgidp;
	pl_t pl;

	switch (sig) {
	case SIGPOLL:				
		if (stp->sd_sigflags & S_MSG) 
			strsendsig(stp->sd_siglist, S_MSG, band);
		break;

	default:
		if ((pgidp = stp->sd_pgidp) != 0) {
			pl = LOCK(&stp->sd_sessp->s_mutex, PL_SESS);
			pgsignal(pgidp, sig);
			UNLOCK(&stp->sd_sessp->s_mutex, pl);
		}
		break;
	}
}


/*
 * void
 * strhup(stdata_t *stp)
 *	Notify pollers of hangup.
 *
 * Calling/Exit State:
 *	The stream's 'stp' sd_mutex must be held upon entry.
 *	The sd_mutex remains held upon exit. 
 * Notes:(TODO)
 *	Need to resolved once TP design for ES/MP is understood.
 *
 */
void
strhup(stdata_t *stp)
{
	if (stp->sd_sigflags & S_HANGUP) 
		strsendsig(stp->sd_siglist, S_HANGUP, 0L);
	pollwakeup(stp->sd_pollist, POLLHUP);
}

/*
 * void
 * strpunlink(cred_t *crp)
 *	Unlink "all" persistant links.
 *
 * Calling/Exit State:
 *	Assumes st_sleep unlocked.
 */

void
strpunlink(cred_t *crp)
{
	struct shinfo  *shp;
	int rval;

	/*
	 * for each allocated stream head, call munlinkall()
	 * with flag of LINKPERSIST to unlink any/all persistant
	 * links for the device.
	 */

	SLEEP_LOCK(&Strinfo[DYN_STREAM].st_sleep, PRIMED);
	shp = (struct shinfo *) Strinfo[DYN_STREAM].st_head;
	while(shp)
	{
		(void) munlinkall((stdata_t *)shp, LINKIOCTL|LINKPERSIST, crp, &rval);
		shp = shp->sh_next;
	}
	SLEEP_UNLOCK(&Strinfo[DYN_STREAM].st_sleep);
}


/*
 * void
 * strfreefp(mblk_t *mp)
 *	Queue up an M_PASSFP message for the streams daemon.
 *
 * Calling/Exit State:
 *	Assumes strd_mutex unlocked.
 */

void
strfreefp(mblk_t *mp)
{
	pl_t pl;

	ASSERT(mp->b_datap->db_type == M_PASSFP);
	ASSERT(mp->b_datap->db_ref == 0);
	mp->b_datap->db_ref = 1;
	pl = LOCK(&strd_mutex, PLSTR);
	mp->b_next = strfree;
	strfree = mp;
	if (SV_BLKD(&strd_sv)) {
		UNLOCK(&strd_mutex, pl);
		SV_SIGNAL(&strd_sv, 0);
	} else {
		UNLOCK(&strd_mutex, pl);
	}
}

/*
 * void
 * strfreesb(mblk_t *mp)
 *	Queue up an "esballoc'ed" message for the streams daemon.
 *
 * Calling/Exit State:
 *	Assumes strd_mutex unlocked.
 */

void
strfreesb(mblk_t *mp)
{
	pl_t pl;

	ASSERT(mp->b_datap->db_ref == 0);
	ASSERT(mp->b_datap->db_frtnp != NULL);
	ASSERT(mp->b_datap->db_frtnp->free_func != NULL);
	pl = LOCK(&strd_mutex, PLSTR);
	mp->b_next = strfree;
	strfree = mp;
	if (SV_BLKD(&strd_sv)) {
		UNLOCK(&strd_mutex, pl);
		SV_SIGNAL(&strd_sv, 0);
	} else {
		UNLOCK(&strd_mutex, pl);
	}
}

/*
 * void
 * strdolog(mblk_t *mp)
 *	Queue up a log message for the streams daemon.
 *
 * Calling/Exit State:
 *	Assumes strd_mutex unlocked.
 */

void
strdolog(mblk_t *mp)
{
	pl_t pl;
	mblk_t *bp;

	pl = LOCK(&strd_mutex, PLSTR);
	if (strfree) {
		/* stick it on the end of the list */
		bp = strfree;
		while (bp->b_next)
			bp = bp->b_next;
		bp->b_next = mp;
		mp->b_next = NULL;
	} else {
		mp->b_next = NULL;
		strfree = mp;
	}
	if (SV_BLKD(&strd_sv)) {
		UNLOCK(&strd_mutex, pl);
		SV_SIGNAL(&strd_sv, 0);
	} else {
		UNLOCK(&strd_mutex, pl);
	}
}

/*
 * void
 * strdaemon(void *str_argp)
 *	The streams daemon.
 *
 * Calling/Exit State:

/*
 * void
 * strdaemon(void *str_argp)
 *	The streams daemon.
 *
 * Calling/Exit State:
 *	Assumes strd_mutex unlocked.
 *
 * Description:
 *	This is a kernel daemon whose purpose in life is to close file
 *	descriptors associated with freed M_PASSFP messages and to invoke
 *	esbfree routines (on the correct processor, if necessary).
 *
 * Remarks:
 *	esbfree routines can't be invoked directly from freeb because locks
 *	might be held that could cause deadlocks.  The streams daemon handles
 *	all of them, binding to a processor if necessary.
 */

/* ARGSUSED */
void
strdaemon(void *str_argp)
{
	mblk_t *mp;
	dblk_t *dp;
	dblk_t *odp;
	frtn_t *fp;
	struct engine *engp;
	int unbind;
	pl_t pl;

	u.u_lwpp->l_name = "strdaemon";
	
	
	strcount_sync_timeid = itimeout(strcount_sync, (void *)NULL, 
			(HZ | TO_PERIODIC), PLTIMEOUT);	
	for ( ; ; ) {
		pl = LOCK(&strd_mutex, PLSTR);
		if (strfree) {
			mp = strfree;
			strfree = mp->b_next;
			UNLOCK(&strd_mutex, pl);
			if (mp->b_flag & MSGLOG) {
				dostrlog(mp);
			} else if (mp->b_datap->db_type == M_PASSFP) {
				ASSERT(mp->b_datap->db_ref == 1);
				/* LINTED pointer alignment */
				closef(((struct strrecvfd *)mp->b_rptr)->f.fp);
				mp->b_datap->db_type = M_DATA;
				freeb(mp);
			} else {
				unbind = 0;
				dp = mp->b_datap;
				ASSERT(dp->db_ref == 0);
				if (dp->db_cpu) {
					engp = kbind(dp->db_cpu);
					unbind = 1;
					DISABLE_PRMPT();
					u.u_lwpp->l_notrt++;
				}
				fp = dp->db_frtnp;
				ASSERT(fp != NULL);
				ASSERT(fp->free_func != NULL);
				if ((caddr_t)mp == dp->db_addr) {
					/* DDI says be at plstr for this */
					pl = splstr();
					(*fp->free_func)(fp->free_arg);
					splx(pl);
					MET_STRMDB(-1);
					*(Strcount_local + l.eng_num) -= 
						dp->db_size;
					kmem_free(dp->db_addr, dp->db_size);
				} else {
					odp = dp->db_odp;
					if (odp) {
						odp->db_muse = 0;
						if (odp->db_ref == 0) {
							MET_STRMDB(-1);
							
							*(Strcount_local + 
								l.eng_num) -= 
								odp->db_size;

							kmem_free(odp->db_addr, odp->db_size);
						}
					} else {
						MET_STRMSG(-1);
						*(Strcount_local + l.eng_num) 
							-= sizeof(mblk_t);

#ifdef STRLEAK
						kmem_free((caddr_t)mp, sizeof(struct mbinfo));
#else
						kmem_free((caddr_t)mp, sizeof(mblk_t));
#endif
					}
					/* DDI says be at plstr for this */
					pl = splstr();
					(*fp->free_func)(fp->free_arg);
					splx(pl);
					if (dp->db_muse == 0) {
						MET_STRMDB(-1);
						
						*(Strcount_local + l.eng_num) 
							-= dp->db_size;

						kmem_free(dp->db_addr, dp->db_size);
					}
				}
				if (unbind){
					ASSERT(u.u_lwpp->l_notrt != 0);
					u.u_lwpp->l_notrt--;
					ENABLE_PRMPT();
					kunbind(engp);
				}
			}
		} else {
			/* nothing to do, hang out until there's work */
			SV_WAIT(&strd_sv, PRIMED, &strd_mutex);
		}
	}
}

/*
 * void
 * svc_enqueue(queue_t *q, struct qsvc *qlist)
 *	Enqueue 'q' at the tail of the service queue, 'qlist'.
 *
 * Calling/Exit State:
 *	Assumes svc_mutex locked.
 */

void
svc_enqueue(queue_t *q, struct qsvc *qlist)
{
	if (qlist->qs_head) {
		qlist->qs_tail->q_link = q;
		q->q_blink = qlist->qs_tail;
	} else {
		qlist->qs_head = q;
		q->q_blink = NULL;
	}
	q->q_link = NULL;
	qlist->qs_tail = q;
}

/*
 * void
 * svc_dequeue(queue_t *q, struct qsvc *qlist)
 *	Dequeue 'q' from the service queue, 'qlist'.
 *
 * Calling/Exit State:
 *	Assumes svc_mutex locked.
 */

void
svc_dequeue(queue_t *q, struct qsvc *qlist)
{
	if (q->q_blink)
		q->q_blink->q_link = q->q_link;
	else
		qlist->qs_head = q->q_link;
	if (q->q_link)
		q->q_link->q_blink = q->q_blink;
	else
		qlist->qs_tail = q->q_blink;
	q->q_link = NULL;
	q->q_blink = NULL;
}

/*
 * freezeprocs(queue_t *q);
 *	Freeze the state of the queue-pair with respect to ongoing activity.
 *	The activities are handled in the following manner:
 *	 - Put procedures:
 *		puts already running => loop-and-wait until they finish
 *		puts yet to come     => locks these out with sd_mutex
 *	 - Service procedure:
 *		svc procedure running   => waits until it finishes
 *		svc procedure enqueued  => waits until it runs and finishes
 *
 * Calling/Exit State:
 *	Assumes sd_mutex is locked.
 */

void
freezeprocs(queue_t *q)
{
	pl_t pl;
	queue_t *wq;

	wq = WR(q);

	/* 
	 * Mark both queues freezing.  This causes subsequently finishing put
	 * and service procedures to SV_SIGNAL sd_freeze
	 */

	q->q_flag |= QFREEZE;
	wq->q_flag |= QFREEZE;

	/* 
	 * Wait until all ongoing queue-pair activity is finished.
	 *
	 * The ongoing activities may be:
	 *	put procedures running (indicated by putcnt > 0)
	 *	service procedure running (indicated by QSVCBUSY flag)
	 *	service procedure enqueued (indicated by QENAB flag)
	 *
	 * Note that all subsequent qenables will not enqueue the queue
	 * until de-freeze occurs.  Instead, they set the QTOENAB flag
	 * which may then be used by the caller to re-qenable after the
	 * freeze is over.
	 */
 retry:
	if ((q->q_putcnt == 0) && (wq->q_putcnt == 0)) {
		/* puts are not active, check for service procs now */
		pl = LOCK(&svc_mutex, PLSTR);
		if (((q->q_svcflag & (QSVCBUSY | QENAB)) == 0) && 
		    ((wq->q_svcflag & (QSVCBUSY | QENAB)) == 0)) {
			/* service procs are also not active: q-pair is frozen */
			UNLOCK(&svc_mutex, pl);
			q->q_flag &= ~QFREEZE;
			wq->q_flag &= ~QFREEZE;
			return;
		}
		UNLOCK(&svc_mutex, pl);
	} 
	
	/* 
	 * Wait for current activity to end.  The following is
	 * non-interruptible.
	 */
	SV_WAIT(q->q_str->sd_freeze, PRIMED, q->q_str->sd_mutex);
	LOCK(q->q_str->sd_mutex, PLSTR);
	goto retry;
}

/*
 * int
 * getplumb(stdata_t *stp)
 *	Acquire the sd_plumb sleep lock to exclude other plumbing operations.
 *	Block for up to STRTIMOUT seconds if there is an outstanding plumbing
 *	operation for this stream.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex is locked, but unlocks it before returning.  Returns
 *	0 on success, non-zero on failure.
 *
 * Description:
 *	All blocked processes will be awakened by an SV_BROADCAST on sd_plumb
 *	which is done as a result of an ACK or NAK being received for the
 *	outstanding ioctl, or as a result of the timer expiring on the
 *	outstanding ioctl (a failure), or as a result of any other process
 *	waiting on sd_timer2 timing out (also a failure).
 */

int
getplumb(stdata_t *stp)
{
	toid_t id;
	clock_t olbolt;
	long ticks;
	pl_t pl;
	int error;

	error = 0;
	/* first see if sd_plumb is available, if so, no need to set timer */
	if (SLEEP_TRYLOCK(stp->sd_plumb) == B_TRUE) {
		UNLOCK(stp->sd_mutex, pl0);
		return(0);
	}

	/*
	 * set timeout, don't care if we're off a tick or so on lbolt,
	 * since we're dealing in second granularities.
	 */
	ticks = STRTIMOUT * HZ;
	olbolt = lbolt;
	id = itimeout(str2time, (void *) stp, ticks, PLSTR);
	if (id == 0) {
		UNLOCK(stp->sd_mutex, pl0);
		return(EAGAIN);
	}

	while (SLEEP_TRYLOCK(stp->sd_plumb) == B_FALSE) {
		/* note: SV_WAIT_SIG returns at pl0 with no lock held */
		if (SV_WAIT_SIG(stp->sd_timer2, PRIMED, stp->sd_mutex) == B_FALSE) {
			untimeout(id);
			return(EINTR);
		}
		if (lbolt >= (olbolt + ticks)) {
			untimeout(id);
			return(ETIME);
		}
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & (STPLEX | STRDERR | STWRERR | STRHUP)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
				(stp->sd_werror ? stp->sd_werror : stp->sd_rerror));
			UNLOCK(stp->sd_mutex, pl);
			untimeout(id);
			return(error);
		}
	}
	
	/*
	 * At this point sd_plumb has been acquired.  Release sd_mutex and reset
	 * the timer mechanism.
	 */

	UNLOCK(stp->sd_mutex, pl);
	untimeout(id);
	return(0);
}

/*
 * void
 * relplumb(stdata_t *stp)
 *	Release sd_plumb and wake up anybody waiting for it
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 */

void
relplumb(stdata_t *stp)
{
	pl_t pl;

	pl = LOCK(stp->sd_mutex, PLSTR);
	SLEEP_UNLOCK(stp->sd_plumb);
	if (SV_BLKD(stp->sd_timer2)) {
		UNLOCK(stp->sd_mutex, pl);
		SV_BROADCAST(stp->sd_timer2, 0);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}
}


/*
 * void
 * struniput(struct strunidata *sup)
 *	Uniprocess "put" routine.
 *
 * Calling/Exit State:
 *	Assumes no locks held.  Assumes put count has been bumped by the
 *	"front" half of the put.
 *
 * Description:
 *	If a message is about to be put onto a queue that suddenly finds
 *	itself bound, a dtimeout of this routine is scheduled on the correct
 *	cpu.  The put count is bumped before this is done; this guarantees
 *	that the queue can not disappear because of a pop.  This routine
 *	will complete the put.  Note that these are all scheduled at (now+0)
 *	and the algorithm in addcallout() will put them in sequential order
 *	so they will fire "in order" maintaining message ordering.
 */

void
struniput(struct strunidata *sup)
{
	queue_t *qp;
	pl_t pl;

	qp = sup->su_qp;

	/*
	 * only call put procedure if queue is still valid (not half closed)
	 * QUSE is essentially invariant, so no lock needed (it will only
	 * be turned off if the driver/module is UP and that implies binding
	 * so no race can occur)
	 */
	if (qp->q_flag & QUSE)
		(*qp->q_putp)(qp, sup->su_mp);
	pl = LOCK(qp->q_str->sd_mutex, PLSTR);
	qp->q_defcnt--;
	qp->q_putcnt--;
	if ((qp->q_flag & QFREEZE) && (qp->q_putcnt == 0)) {
		UNLOCK(qp->q_str->sd_mutex, pl);
		SV_SIGNAL(qp->q_str->sd_freeze, 0);
	} else {
		UNLOCK(qp->q_str->sd_mutex, pl);
	}
	kmem_free(sup, sizeof(struct strunidata));
	
	*(Strcount_local + l.eng_num) -= sizeof(struct strunidata);
}

/*
 * int
 * strintercept(queue_t *qp, mblk_t *mp)
 *	Intercept messages intended for a driver that is ddi 5 or earlier
 *	and modify the message to match the physical characteristics
 *	required (i.e. DMA-able and physically contiguous)
 *
 * Calling/Exit State:
 *	No locks held.
 */

int
strintercept(queue_t *qp, mblk_t *mp)
{
	mblk_t *newmp;

	newmp = msgphysreq(mp, strphysreq);
	/*
	 * If newmp is non-NULL, then msgphysreq has already freed mp.
	 * If newmp is NULL, we have a problem.  That means that msgphysreq
	 * found an mblk that needed to be modified and failed so we know
	 * that mp (which is preserved on failure) does not meet the
	 * criteria specified in strphysreq so it is not safe to pass the
	 * message on.  Dropping it seems the less of two evils.
	 */
	if (newmp == NULL) {
		/*
		 *+ Could not allocate new message block matching physreq,
		 *+ dropping message
		 */
		cmn_err(CE_WARN, "strintercept: dropping message\n");
		freemsg(mp);
		return;
	}
	/*
	 * This is tricky.  From the point of view of streams, we *are* the
	 * put procedure, so the putcnt accounting has been done above us
	 * as has any UP correction so we can call the real put routine
	 * directly.  This field is invarient, so no locking is needed.
	 */
	(*qp->q_qinfo->qi_putp)(qp, newmp);
	return;
}

/*
 * void
 * upbackfix(queue_t *qp)
 *	Handle the case where we are a bound stream that got backenabled
 *	from the wrong processor.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 * Description:
 *	This routine exists to handle the case where strread was running
 *	on the wrong processor and backenabled a queue in a bound stream.
 *	The alternative is to kbind the strread, which isn't necessary most
 *	of the time.  This is a performance optimization.  q_putcnt is
 *	bumped in qenable_l when the mismatch is found to keep the queue from
 *	being popped in the interim.  It's not elegant, but it works.
 */

void
upbackfix(queue_t *qp)
{
	pl_t pl;

	pl = LOCK(qp->q_str->sd_mutex, PLSTR);
	qenable_l(qp);
	qp->q_putcnt--;
	if ((qp->q_flag & QFREEZE) && (qp->q_putcnt == 0)) {
		UNLOCK(qp->q_str->sd_mutex, pl);
		SV_SIGNAL(qp->q_str->sd_freeze, 0);
	} else {
		UNLOCK(qp->q_str->sd_mutex, pl);
	}
}

/*
 * void
 * streams_check_bufcall(void)
 *      Check if bufcalls should be run; used by poolrefresh daemon.
 *
 * Calling/Exit State:
 *      Called at PLBASE and returns the same.
 */

void
streams_check_bufcall()
{
	int i;
	static int last;

	if (strbcwait && !strbcflag) {
		ASSERT(getpl() == PLBASE);
		(void) LOCK(&bc_mutex, PLSTR);
		if (strbcwait && !strbcflag) {
			strbcflag = 1;
			if (strbcwait & B_UP) {
				/*
				 * if there is a UP bufcall to run, the
				 * scheduler will also handle the non-bound
				 * bufcalls to so the strbcwait & B_MP case
				 * below need not be executed
				 */
				for (i = 0; i < Nengine; i++)
					altstrbceng[i] = strbceng[i];
				UNLOCK(&bc_mutex, PLBASE);
				last = (last + 1) % Nengine;
				for (i = 0; i < Nengine; i++) {
					if (altstrbceng[last]) {
						setqsched(&engine[last]);
						return;
					}
					if (++last == Nengine)
						last = 0;
				}
				return;
			}
			if (strbcwait & B_MP) {
				UNLOCK(&bc_mutex, PLBASE);
				setqsched(NULL);
			}
		} else
			UNLOCK(&bc_mutex, PLBASE);
	}
}

/*
 * struct striopst *
 * findioc(long iocid)
 *	Find a post-processing request, remove it from list, and return it.
 *
 * Calling/Exit State:
 *	Assumes strio_mutex not locked
 */

struct striopst *
findioc(long iocid)
{
	struct striopst *sp;
	struct striopst *prevsp;
	pl_t pl;

	prevsp = NULL;
	pl = LOCK(&strio_mutex, PLSTR);
	sp = strioclist;
	while (sp) {
		if (sp->sio_iocid != iocid) {
			prevsp = sp;
			sp = sp->sio_next;
			continue;
		}
		/* found it */
		if (prevsp)
			prevsp->sio_next = sp->sio_next;
		else
			strioclist = sp->sio_next;
		UNLOCK(&strio_mutex, pl);
		return(sp);
	}
	UNLOCK(&strio_mutex, pl);
	return(NULL);
}

/*
 * void
 * str_kmadv(void)
 *	Call kmem_advise() for STREAMS data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */

void
str_kmadv(void)
{
	kmem_advise(sizeof(struct shinfo));
	kmem_advise(sizeof(struct queinfo));
}

/*
 * void
 * io_postroot(void)
 *	Post-root initializations.
 *
 * Calling/Exit State:
 *	Called from main() after mounting root.
 */
void
io_postroot(void)
{
	extern void strdaemon(void *);

	/* Spawn the streams daemon. */
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL, strdaemon, NULL);
}

/*
 * mblk_t *
 * mblkprune(mblk_t *)
 *	Prune 0 length messages
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Remove all 0 length messages.  If the head mblk is 0 length, though,
 *	leave it intact.  Can be called with a null bp if an allocation failed.
 */

mblk_t *
mblkprune(mblk_t *bp)
{
	mblk_t *tmp;
	mblk_t *otmp;

	if (bp == NULL)
		return(NULL);
	otmp = bp;
	/* skip the head, start with b_cont */
	for (tmp = bp->b_cont; tmp; tmp = tmp->b_cont) {
		if (tmp->b_rptr == tmp->b_wptr) {
			otmp->b_cont = tmp->b_cont;
			freeb(tmp);
			tmp = otmp;
		} else
			otmp = tmp;
	}
	return(bp);
}


#ifdef STRPERF
/*
 * void
 * gatherstats(mblk_t *bp)
 *	gather performance data
 *
 * Calling/Exit State:
 *	Assumes stat_mutex not held.
 */

void
gatherstats(mblk_t *bp)
{
	FSPIN_LOCK(&stat_mutex);
	strperf.sp_sh += bp->b_sh;
	strperf.sp_oh += bp->b_oh;
	strperf.sp_sched += bp->b_sched;
	strperf.sp_work += bp->b_work;
	strperf.sp_copyin += bp->b_copyin;
	strperf.sp_copyout += bp->b_copyout;
	strperf.sp_life += bp->b_life;
	if (bp->b_sh > strperf.sp_maxsh)
		strperf.sp_maxsh = bp->b_sh;
	if (bp->b_oh > strperf.sp_maxoh)
		strperf.sp_maxoh = bp->b_oh;
	if (bp->b_sched > strperf.sp_maxsched)
		strperf.sp_maxsched = bp->b_sched;
	if (bp->b_work > strperf.sp_maxwork)
		strperf.sp_maxwork = bp->b_work;
	if (bp->b_copyin > strperf.sp_maxcopyin)
		strperf.sp_maxcopyin = bp->b_copyin;
	if (bp->b_copyout > strperf.sp_maxcopyout)
		strperf.sp_maxcopyout = bp->b_copyout;
	if (bp->b_life > strperf.sp_maxlife)
		strperf.sp_maxlife = bp->b_life;
	strperf.sp_cnt++;
	FSPIN_UNLOCK(&stat_mutex);
}
#endif


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_queue(const queue_t *)
 *	Formatted print of queue structure. Can be invoked from
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_queue(const queue_t *qp)
{
	debug_printf("\n STREAM queue struct (queue_t): size=0x%x(%d)\n",
		sizeof(queue_t), sizeof(queue_t));
	debug_printf("\tq_qinfo=0x%x, \tq_first=0x%x\n",
		qp->q_qinfo, qp->q_first);
	debug_printf("\tq_last=0x%x, \tq_next=0x%x\n",
		qp->q_last, qp->q_next);
	debug_printf("\tq_link=0x%x, \tq_ptr=0x%x\n",
		qp->q_link, qp->q_ptr);
	debug_printf("\tq_count=0x%x, \tq_flag=0x%x\n",
		qp->q_count, qp->q_flag);
	debug_printf("\tq_minpsz=0x%x, \tq_maxpsz=0x%x\n",
		qp->q_minpsz, qp->q_maxpsz);
	debug_printf("\tq_hiwat=0x%x, \tq_lowat=0x%x\n",
		qp->q_hiwat, qp->q_lowat);
	debug_printf("\tq_bandp=0x%x, \tq_nband=0x%x\n",
		qp->q_bandp, qp->q_nband);
	debug_printf("\tq_blocked=0x%x, \tq_svcflag=0x%x\n",
		qp->q_blocked, qp->q_svcflag);
	debug_printf("\tq_str=0x%x, \tq_blink=0x%x\n",
		qp->q_str, qp->q_blink);
	debug_printf("\tq_putp=0x%x, \tq_putcnt=0x%x\n",
		qp->q_putp, qp->q_putcnt);
	debug_printf("\tq_defcnt=0x%x\n",
		qp->q_defcnt);
}

/*
 * void
 * print_dblk(const dblk_t *)
 *	Formatted print of datab (dblk_t) structure. Can be invoked from
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_dblk(const dblk_t *dp)
{
	debug_printf("\n STREAM datab struct (dblk_t): size=0x%x(%d)\n",
		sizeof(dblk_t), sizeof(dblk_t));
	debug_printf("\tdb_frtnp=0x%x, \tdb_base=0x%x\n",
		dp->db_frtnp, dp->db_base);
	debug_printf("\tdb_lim=0x%x, \tdb_ref=0x%x\n",
		dp->db_lim, dp->db_ref);
	debug_printf("\tdb_type=0x%x, \tdb_muse=0x%x\n",
		dp->db_type, dp->db_muse);
	debug_printf("\tdb_size=0x%x, \tdb_addr=0x%x\n",
		dp->db_size, dp->db_addr);
	debug_printf("\tdb_odp=0x%x, \tdb_cpu=0x%x\n",
		dp->db_odp, dp->db_cpu);
}

/*
 * void
 * print_mblk(const mblk_t *)
 *	Formatted print of msgb (mblk_t) structure. Can be invoked from
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_mblk(const mblk_t *mp)
{
	debug_printf("\n STREAM msgb struct (mblk_t): size=0x%x(%d)\n",
		sizeof(mblk_t), sizeof(mblk_t));
	debug_printf("\tb_next=0x%x, \tb_prev=0x%x\n",
		mp->b_next, mp->b_prev);
	debug_printf("\tb_cont=0x%x, \tb_rptr=0x%x\n",
		mp->b_cont, mp->b_rptr);
	debug_printf("\tb_wptr=0x%x, \tb_datap=0x%x\n",
		mp->b_wptr, mp->b_datap);
	debug_printf("\tb_band=0x%x, \tb_flag=0x%x\n",
		mp->b_band, mp->b_flag);
	print_dblk(mp->b_datap);
}
#endif /* DEBUG */
