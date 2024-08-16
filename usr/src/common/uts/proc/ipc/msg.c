/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/ipc/msg.c	1.13"

/*
 * Inter-Process Communication Message Facility.
 */
#include <util/types.h>
#include <util/ksynch.h>
#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <util/debug.h>
#include <util/param.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <acc/dac/acl.h>
#include <proc/ipc/ipcsec.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/msg.h>
#include <util/cmn_err.h>
#include <util/metrics.h>
#include <util/plocal.h>
#include <mem/kmem.h>
#include <acc/mac/mac.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#include <acc/mac/cca.h>
#endif

/*
 *+ msgdir_mutex: protects System V IPC message semaphore
 *+		  lookups and creation/deletion.
 */
STATIC LKINFO_DECL(msgdir_lkinfo, "PI::msgdir_mutex", 0);
STATIC rwlock_t		msgdir_mutex;
ipcdir_t		msgdir;

#define	MSQDIR_RDLOCK()		RW_RDLOCK_PLMIN(&msgdir_mutex)
#define	MSQDIR_WRLOCK()		RW_WRLOCK_PLMIN(&msgdir_mutex)
#define	MSQDIR_UNLOCK(pl)	RW_UNLOCK_PLMIN(&msgdir_mutex, (pl))

STATIC int		msgalloc(ipc_perm_t **);
STATIC void		msgdealloc(ipc_perm_t *);

STATIC ipcops_t		msgops = {msgalloc, msgdealloc};
STATIC ipcdata_t	msgdata = {&msgdir, &msgops};

/*
 *+ msgresv_mutex: controls System V message IPC message header
 *+		   and message text memory reservations.
 */
STATIC LKINFO_DECL(msgresv_lkinfo, "PI::msgresv_mutex", 0);
STATIC lock_t		msgresv_mutex;	/* msg header, msg text reservations */
STATIC struct msgstat	msgstat;
STATIC sv_t		msghd_sv;	/* wait for msg header space */
STATIC sv_t		msgtxt_sv;	/* wait for msg text space */

/*
 *+ kmsq_mutex: System V message queue IPC per msqid_ds mutex.
 */
STATIC LKINFO_DECL(kmsq_lkinfo, "PI::kmsq_mutex", 0);

/* msginfo is defined in proc/ipc/msg.cf */
extern struct msginfo msginfo;		/* configuration info structure */

/* Convienient macros: */
#define	MSQID_TO_SLOT(msgid)	IPCID_TO_SLOT((msgid), msginfo.msgmni)
#define	MSQID_TO_SEQ(msgid)	IPCID_TO_SEQ((msgid),  msginfo.msgmni)
#define	SLOT_TO_MSQID(slot, seq) SLOT_TO_IPCID((slot), (seq), msginfo.msgmni)

/*
 * Argument vectors for the various flavors of msgsys().
 */
#define	MSGGET	0
#define	MSGCTL	1
#define	MSGRCV	2
#define	MSGSND	3

union msgsysa {
	int			opcode;

	struct msgctla {			/* MSGCTL */
		int		opcode;
		int		msgid;
		int		cmd;
		struct msqid_ds	*buf;
	} msgctla;

	struct msggeta {			/* MSGGET */
		int		opcode;
		key_t		key;
		int		msgflg;
	} msggeta;

	struct msgrcva {			/* MSGRCV */
		int		opcode;
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		long		msgtyp;
		int		msgflg;
	} msgrcva;

	struct msgsnda {			/* MSGSND */
		int		opcode;
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		int		msgflg;
	} msgsnda;
};

/*
 * Support Functions:
 */

/*
 * int msgresv(struct kmsqid_ds *qp, int txtcnt, int flag)
 *	Stake memory claims for an incoming IPC message.  Reserve
 *	a message header and 'txtcnt' bytes for the message text.
 *
 * Calling/Exit State:
 *	On entry this function expects the message queue specified by
 *	'qp' to be locked.  This function can block if the requested
 *	resources are not available and the caller is willing to wait
 *	for the resources (as specified by 'flag').
 *
 * Return Values:
 * 	On success: 0 is returned and the message queue lock remains
 *		    held.
 *	If it was necessary to block waiting for space, and the caller must
 *		    retry, -1 is returned and the queue lock is dropped.
 *	On failure: Non-zero errno is returned and the message queue
 *		    lock is dropped.
 *
 * Remarks:
 *	msginfo.msgtql should be configured to be sufficiently large
 *	so that we never block for a message header reservation.
 */
STATIC int
msgresv(struct kmsqid_ds *qp, int txtcnt, int flag)
{
	ASSERT(LOCK_OWNED(&qp->kmsq_mutex) && KS_HOLD1LOCK());

	(void)LOCK_PLMIN(&msgresv_mutex);
	if (msgstat.msghdresv >= msginfo.msgtql) {
		MSQID_UNLOCK(qp, PLMIN);
#ifdef CC_PARTIAL
		CC_COUNT(CC_RE_MSG, CCBITS_RE_MSG);
#endif
		msgstat.msghdfail++;
		if (flag & IPC_NOWAIT) {
			UNLOCK(&msgresv_mutex, PLBASE);
			return EAGAIN;
		}
		if (!SV_WAIT_SIG(&msghd_sv, PRIMSG, &msgresv_mutex))
			return EINTR;
		return -1;
	}
	if (txtcnt + msgstat.msgtxtresv > msgstat.msgtxt) {
		MSQID_UNLOCK(qp, PLMIN);
#ifdef CC_PARTIAL
		CC_COUNT(CC_RE_MSG, CCBITS_RE_MSG);
#endif
		msgstat.msgtxtfail++;
		if (flag & IPC_NOWAIT) {
			UNLOCK(&msgresv_mutex, PLBASE);
			return EAGAIN;
		}
		if (!SV_WAIT_SIG(&msgtxt_sv, PRIMSG, &msgresv_mutex))
			return EINTR;
		return -1;
	}
	msgstat.msghdresv++;				/* stake my claim */
	msgstat.msgtxtresv += txtcnt;
	UNLOCK_PLMIN(&msgresv_mutex, PLMIN);
	return 0;
}

/*
 * void msgunresv(int txtcnt, int hdcnt)
 *	Release memory claims for an IPC message.
 *
 * Calling/Exit State:
 *	This function does not block and may be called with
 *	locks held as long as lock hierarchy is not violated.
 *	In particular, a lock on a message queue may be held.
 *
 * Remarks:
 *	If thrashing due to message text allocation/deallocation
 *	is a problem, it may be desirable to enforce a low water
 *	mark which msgstat.msgtxtresv must be below before any
 *	broadcast wakeups are done.
 */
STATIC void
msgunresv(int txtcnt, int hdcnt)
{
	pl_t	pl;
	int	havail = 0;
	int	tavail = 0;

	pl = LOCK_PLMIN(&msgresv_mutex);

	msgstat.msgtxtresv -= txtcnt;
	msgstat.msghdresv -= hdcnt;
	if (msgstat.msgtxtresv < msgstat.msgtxt && SV_BLKD(&msgtxt_sv))
		tavail = 1;
	if (msgstat.msghdresv < msginfo.msgtql && SV_BLKD(&msghd_sv))
		havail = 1;

	UNLOCK(&msgresv_mutex, pl);

	if (tavail)
		SV_BROADCAST(&msgtxt_sv, 0);
	if (havail)
		SV_BROADCAST(&msghd_sv, 0);
		
}

/*
 * Size used by msgalloc()/msgdealloc() when allocating/deallocating
 * a message queue descriptor (kmsqid_ds structure).
 * Note that the ipc_sec structure and the kmsqid_ds structure are
 * allocated together.
 */
STATIC size_t msqidsz = (sizeof(struct kmsqid_ds) + sizeof(struct ipc_sec)
				+ sizeof(long) - 1)
				& ~(sizeof(long) - 1);
/*
 * int msgalloc(ipc_perm_t **ipcpp)
 *	Allocate and partially initialize a kmsqid_ds data
 *	structure.  A pointer to the encapsulated ipc_perm
 *	structure is returned via the out argument 'ipcpp'.
 *
 * Calling/Exit State:
 *	Upon entry this function expects to be called with
 *	the msgdir_mutex held.  On return, the msgdir_mutex
 *	is held in exclusive mode.  However, msgdir_mutex is
 *	dropped and re-acquired internally to this function.
 *	This function can block (via kmem_alloc()).
 *	This function must return a nonzero value to indicate
 *	to ipcget() that the directory lock was dropped.
 *
 * Notes:
 *	Only called from ipcget() via the IPC_ALLOCATE() macro.
 */
STATIC int
msgalloc(ipc_perm_t **ipcpp)
{
	vaddr_t	vaddr;
	struct	kmsqid_ds *qp;

	ASSERT(RW_OWNED(&msgdir_mutex) && KS_HOLD1LOCK());

	MSQDIR_UNLOCK(PLBASE);

	/*
	 * Allocate the kmsqid_ds structure and the
	 * ipc_sec structure in one shot.
	 */
	qp = kmem_zalloc(msqidsz, KM_SLEEP);
	vaddr = (vaddr_t)qp;

	/* Align address for ipc_sec structure; reflected in msqidsz. */
	vaddr += (sizeof(struct kmsqid_ds) + sizeof(long) - 1)
			& ~(sizeof(long) - 1);
	qp->kmsq_ds.msg_perm.ipc_secp = (struct ipc_sec *)vaddr;

	/* Initialize lock and synch variables. */
	LOCK_INIT(&qp->kmsq_mutex, MSGDS_HIER, PLMIN, &kmsq_lkinfo,
			KM_SLEEP);
	SV_INIT(&qp->kmsq_sv);
	SV_INIT(&qp->kmsq_rcv_sv);
	SV_INIT(&qp->kmsq_snd_sv);
	*ipcpp = &qp->kmsq_ds.msg_perm;		/* out argument */

	/*
	 * Acquire the directory lock in exclusive mode in anticipation
	 * of a write to the directory by ipcget().
	 */
	MSQDIR_WRLOCK();
	return 1;			/* indicate that lock was dropped */
}

/*
 * void msgdealloc(ipc_perm_t *ipcp)
 *	Deallocate the passed in ipc data structure and associated
 *	data structures.
 *
 * Calling/Exit State:
 *	The msgdir_mutex may be held when called from ipcget() due
 *	to a failed allocation attempt.
 *	The ipc data structure must be invisible to any lookup channels
 *	prior to calling this function.
 *
 * Notes:
 *	Called from ipcget() via the IPC_DEALLOCATE() macro.
 *      Above call represents failed allocation attempt, which did
 *      not cause a mac_hold.  Therefore no mac_rele wanted in connection
 *      with it.
 *      Also called directly from msgctl(IPC_RMID).  This represents
 *      bona fide removal, needing associated mac_rel which is put
 *      into msgctl, which warrants eventual alternate more robust
 *      and readable form.
 */
STATIC void
msgdealloc(ipc_perm_t *ipcp)
{
	struct kmsqid_ds	*qp = IPC_TO_MSQDS(ipcp);

	ASSERT(KS_HOLD0LOCKS() || (KS_HOLD1LOCK() && RW_OWNED(&msgdir_mutex)));
	ASSERT(!SV_BLKD(&qp->kmsq_sv));
	ASSERT(!SV_BLKD(&qp->kmsq_rcv_sv));
	ASSERT(!SV_BLKD(&qp->kmsq_snd_sv));

	LOCK_DEINIT(&qp->kmsq_mutex);

	/* Free ACL for object (if any). */
	FRIPCACL(qp->kmsq_ds.msg_perm.ipc_secp);

	/* Free up kmsqid_ds and ipc_sec structures in one shot. */
	kmem_free(qp, msqidsz);
}

/*
 * int msgconv(const int msqid, struct kmsqid_ds **qpp, const int hold)
 * 	Convert user supplied msqid into a pointer to the associated
 *	message queue descriptor.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry.  This function can
 *	block.
 *	On success, this function returns 0 with the message queue
 *	descriptor lock held.
 *	On failure, a non-zero errno is returned with no locks held.
 *
 *	Arguments:
 *		msqid	Message queue identifier.
 *		qpp	Out arg: corresponding message queue descriptor.
 *		hold	If true, acquire the directory lock in
 *			exclusive mode and keep the directory lock
 *			held on return.
 *			This is only used by IPC_RMID.  Unfortunate,
 *			but resolves a race when removing a descriptor.
 * Notes:
 *	This function cannot be static since it is called from ipcdac.c.
 */
int
msgconv(const int msqid, struct kmsqid_ds **qpp, const int hold)
{
	struct	kmsqid_ds *qp;		/* ptr to message queue */
	ipcdirent_t *dp;		/* ptr to directory entry */
	int	slot;			/* directory slot number */
	int	seq;			/* slot re-use sequence number */

	ASSERT(KS_HOLD0LOCKS());

	if (msqid < 0 || msgdir.ipcdir_nents == 0)
		return EINVAL;

	slot = MSQID_TO_SLOT(msqid);
	seq  = MSQID_TO_SEQ(msqid);
	dp = msgdir.ipcdir_entries + slot;

again:
	if (hold)
		(void)MSQDIR_WRLOCK();
	else
		(void)MSQDIR_RDLOCK();

	if ((qp = IPC_TO_MSQDS(dp->ipcd_ent)) == NULL || seq != dp->ipcd_seq) {
		MSQDIR_UNLOCK(PLBASE);
		return EINVAL;
	}

	(void)MSQID_LOCK(qp);

	/* Wait if msqid is marked busy. */
	if (qp->kmsq_flag & MSQID_BUSY) {
	        MSQDIR_UNLOCK(PLMIN);
		SV_WAIT(&qp->kmsq_sv, PRIMED, &qp->kmsq_mutex);
		goto again;
        }

	if (!hold) 
	        MSQDIR_UNLOCK(PLMIN);

	/* Must be allocated and not busy. */
	ASSERT((qp->kmsq_ds.msg_perm.mode & IPC_ALLOC) != 0);
	ASSERT((qp->kmsq_flag & MSQID_BUSY) == 0);

	*qpp = qp;			/* out argument */
	return 0;
}

/*
 * void msgremove(const int msqid)
 *	Remove msqid from the message queue directory.
 *
 * Calling/Exit State:
 *	Both the message queue directory lock and the corresponding
 *	msqid_ds structure must be locked by the caller.  The message
 *	queue directory lock must be held in exclusive mode.
 *	This function does not explicitly acquire or release any
 *	locks.
 */
STATIC void
msgremove(const int msqid)
{
	ipcdirent_t *dp;			/* ptr to directory entry */
	int	slot = MSQID_TO_SLOT(msqid);	/* directory slot */

	ASSERT(RW_OWNED(&msgdir_mutex));	/* held in exclusive mode */
	ASSERT(msqid >= 0 && msgdir.ipcdir_nents > slot);
	ASSERT(msgdir.ipcdir_nactive > 0);

	msgdir.ipcdir_nactive--;
	dp = msgdir.ipcdir_entries + slot;
	dp->ipcd_ent = NULL;

	/* Bump the sequence number, don't allow negative msqid's. */
	dp->ipcd_seq = (msqid + msginfo.msgmni < 0) ? 0 : dp->ipcd_seq + 1;
}

/*
 * System call handlers for msgctl(), msgget(), msgrcv(), and msgsnd():
 */

/*
 * int msgctl(struct msgctla *uap, rval_t *rvp)
 *	System call entry point for msgctl(2).
 *
 * Calling/Exit State:
 *	No locks are held on entry to this function, no locks
 *	are held on return.  These conditions are ASSERT()ed
 *	in msgsys().
 */
STATIC int
msgctl(struct msgctla *uap, rval_t *rvp)
{
	struct kmsqid_ds	*qp;	/* ptr to associated message queue */
	struct msqid_ds		ds;	/* application view */
	struct o_msqid_ds	ods;	/* "old" application view */
	int			error;
	int			hdcnt, txtcnt;
	struct msg		*mp, *nxtmp;
	cred_t			*crp = CRED();
	lid_t			lid;

	rvp->r_val1 = 0;

	switch (uap->cmd) {
	case IPC_O_RMID:
	case IPC_RMID:
		if (error = msgconv(uap->msgid, &qp, 1))
			return error;

		lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&qp->kmsq_ds.msg_perm, MSG_R|MSG_W,
				      IPC_MAC, crp)) {
		        MSQDIR_UNLOCK(PLMIN);
			break;
		}

#ifdef CC_PARTIAL
		MAC_ASSERT(qp, MAC_SAME);
#endif
		
		/* must have ownership */
		if (crp->cr_uid != qp->kmsq_ds.msg_perm.uid
		  && crp->cr_uid != qp->kmsq_ds.msg_perm.cuid
		  && pm_denied(crp, P_OWNER)) {
		        MSQDIR_UNLOCK(PLMIN);
			error = EPERM;
			break;
		}

		/*
		 * Committed to removing the entry.
		 * The caller holds an exclusive lock on the directory
		 * and the lock on the message queue descriptor.
		 * Msgremove() makes the message queue descriptor
		 * invisible to all lookup channels (msgget(), msgconv()).
		 */
		msgremove(uap->msgid);
		qp->kmsq_ds.msg_perm.mode &= ~IPC_ALLOC;
	        MSQDIR_UNLOCK(PLMIN);
		MSQID_UNLOCK(qp, PLBASE);	/* invisible, drop all locks */

		/* Kick out any contexts blocked on this message queue. */
		if (SV_BLKD(&qp->kmsq_sv))
			SV_BROADCAST(&qp->kmsq_sv, 0);
		if (SV_BLKD(&qp->kmsq_rcv_sv))
			SV_BROADCAST(&qp->kmsq_rcv_sv, 0);
		if (SV_BLKD(&qp->kmsq_snd_sv))
			SV_BROADCAST(&qp->kmsq_snd_sv, 0);

		/*
		 * Free up memory for message headers and message text.
		 * Release our claims on message headers and text with
		 * msgunresv().
		 */
		hdcnt = txtcnt = 0;
		for (mp = qp->kmsq_ds.msg_first; mp != NULL; mp = nxtmp) {
			hdcnt++;
			txtcnt += mp->msg_ts;
			nxtmp = mp->msg_next;
			kmem_free(mp, sizeof(struct msg) + mp->msg_ts);
		}

		if (hdcnt != 0)
			msgunresv(txtcnt, hdcnt);

                /*
                 * Decrement the mac levels reference.
                 * This must be done before deallocation since the ipc_secp
                 * referenced structure is also freed as part of the
                 * deallocation.
                 *
                 */
                mac_rele(lid);

		/* Free up the message queue descriptor. */
		msgdealloc(&qp->kmsq_ds.msg_perm);

		goto nolock;

	case IPC_O_SET:
		if (copyin((caddr_t)uap->buf, &ods, sizeof ods)) 
			return EFAULT;
		
		if (ods.msg_perm.uid > MAXUID || ods.msg_perm.gid > MAXUID)
			return EINVAL;

		if (error = msgconv(uap->msgid, &qp, 0))
			return error;

		lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&qp->kmsq_ds.msg_perm, MSG_R|MSG_W,
				      IPC_MAC, crp))
			break;

#ifdef CC_PARTIAL
		MAC_ASSERT(qp, MAC_SAME);
#endif

		/* must have ownership */
		if (crp->cr_uid != qp->kmsq_ds.msg_perm.uid
		  && crp->cr_uid != qp->kmsq_ds.msg_perm.cuid
		  && pm_denied(crp, P_OWNER)) {
			error = EPERM;
			break;
		}

		if (ods.msg_qbytes > qp->kmsq_ds.msg_qbytes
		  && pm_denied(crp, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		qp->kmsq_ds.msg_perm.uid = ods.msg_perm.uid;
		qp->kmsq_ds.msg_perm.gid = ods.msg_perm.gid;
		qp->kmsq_ds.msg_perm.mode =
			(qp->kmsq_ds.msg_perm.mode & ~IPC_PERM) |
					  (ods.msg_perm.mode & IPC_PERM);
		qp->kmsq_ds.msg_qbytes = ods.msg_qbytes;
		qp->kmsq_ds.msg_ctime = hrestime.tv_sec;
		break;

	case IPC_SET:
		if (copyin((caddr_t)uap->buf, &ds, sizeof ds)) 
			return EFAULT;

		if (ds.msg_perm.uid < (uid_t)0 || ds.msg_perm.uid > MAXUID ||
		    ds.msg_perm.gid < (gid_t)0 || ds.msg_perm.gid > MAXUID) 
			return EINVAL;

		if (error = msgconv(uap->msgid, &qp, 0))
			return error;

		lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&qp->kmsq_ds.msg_perm, MSG_R|MSG_W,
				      IPC_MAC, crp))
			break;
#ifdef CC_PARTIAL
		MAC_ASSERT(qp, MAC_SAME);
#endif

		/* must have ownership */
		if (crp->cr_uid != qp->kmsq_ds.msg_perm.uid
		  && crp->cr_uid != qp->kmsq_ds.msg_perm.cuid
		  && pm_denied(crp, P_OWNER)) {
			error = EPERM;
			break;
		}

		if (ds.msg_qbytes > qp->kmsq_ds.msg_qbytes
		  && pm_denied(crp, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		qp->kmsq_ds.msg_perm.uid = ds.msg_perm.uid;
		qp->kmsq_ds.msg_perm.gid = ds.msg_perm.gid;
		qp->kmsq_ds.msg_perm.mode =
			(qp->kmsq_ds.msg_perm.mode & ~IPC_PERM) |
					(ds.msg_perm.mode & IPC_PERM);
		qp->kmsq_ds.msg_qbytes = ds.msg_qbytes;
		qp->kmsq_ds.msg_ctime = hrestime.tv_sec;
		break;

	case IPC_O_STAT:
		if (error = msgconv(uap->msgid, &qp, 0))
			return error;

		lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&qp->kmsq_ds.msg_perm, MSG_R,
				      IPC_MAC|IPC_DAC, crp))
			break;
#ifdef CC_ARTIAL
		MAC_ASSERT(qp, MAC_DOMINATES);
#endif

		/*
		 * Provide an "old" application the view it desires.
		 * Support for non-EFT applications.
		 */
		if (qp->kmsq_ds.msg_perm.uid	> USHRT_MAX
		 || qp->kmsq_ds.msg_perm.gid	> USHRT_MAX
		 || qp->kmsq_ds.msg_perm.cuid	> USHRT_MAX
		 || qp->kmsq_ds.msg_perm.cgid	> USHRT_MAX
		 || qp->kmsq_ds.msg_perm.seq	> USHRT_MAX
		 || qp->kmsq_ds.msg_cbytes	> USHRT_MAX
		 || qp->kmsq_ds.msg_qnum	> USHRT_MAX
		 || qp->kmsq_ds.msg_qbytes	> USHRT_MAX
		 || qp->kmsq_ds.msg_lspid	> SHRT_MAX
		 || qp->kmsq_ds.msg_lrpid	> SHRT_MAX) {
			error = EOVERFLOW;
			break;
		}
		ods.msg_perm.uid  = (o_uid_t) qp->kmsq_ds.msg_perm.uid;
		ods.msg_perm.gid  = (o_gid_t) qp->kmsq_ds.msg_perm.gid;
		ods.msg_perm.cuid = (o_uid_t) qp->kmsq_ds.msg_perm.cuid;
		ods.msg_perm.cgid = (o_gid_t) qp->kmsq_ds.msg_perm.cgid;
		ods.msg_perm.mode = (o_mode_t) qp->kmsq_ds.msg_perm.mode;
		ods.msg_perm.seq  = (ushort) qp->kmsq_ds.msg_perm.seq;
		ods.msg_perm.key  = qp->kmsq_ds.msg_perm.key;
		ods.msg_first = NULL;		/* NULL out kernel addrs */
		ods.msg_last  = NULL;
		ods.msg_cbytes = (ushort) qp->kmsq_ds.msg_cbytes;
		ods.msg_qnum   = (ushort) qp->kmsq_ds.msg_qnum;
		ods.msg_qbytes = (ushort) qp->kmsq_ds.msg_qbytes;
		ods.msg_lspid  = (o_pid_t) qp->kmsq_ds.msg_lspid;
		ods.msg_lrpid  = (o_pid_t) qp->kmsq_ds.msg_lrpid;
		ods.msg_stime  = qp->kmsq_ds.msg_stime;
		ods.msg_rtime  = qp->kmsq_ds.msg_rtime;
		ods.msg_ctime  = qp->kmsq_ds.msg_ctime;

		MSQID_UNLOCK(qp, PLBASE);
		if (copyout(&ods, (caddr_t)uap->buf, sizeof ods))
			error = EFAULT;
		goto nolock;

	case IPC_STAT:
		if (error = msgconv(uap->msgid, &qp, 0))
			return error;

		lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&qp->kmsq_ds.msg_perm, MSG_R,
				      IPC_MAC|IPC_DAC, crp))
			break;
#ifdef CC_APRTIAL
		MAC_ASSERT(qp, MAC_DOMINATES);
#endif

		ds = qp->kmsq_ds;			/* struct assignment */
		ds.msg_first = ds.msg_last = NULL;	/* NULL kernel addrs */
		ds.msg_perm.ipc_secp = NULL;

		MSQID_UNLOCK(qp, PLBASE);
		if (copyout(&ds, (caddr_t)uap->buf, sizeof ds))
			error = EFAULT;
		goto nolock;

	default:
		return EINVAL;
	}

	MSQID_UNLOCK(qp, PLBASE);

nolock:					/* common return for auditing hooks */
	ADT_LIDCHECK(lid);
	return error;
}

/*
 * int msgget(struct msggeta *uap, rval_t *rvp)
 *	System call entry point for msgget(2).
 *
 * Calling/Exit State:
 *	No locks are held on entry to this function, no locks
 *	are held on return.  These conditions are ASSERT()ed
 *	in msgsys().
 */
STATIC int
msgget(struct msggeta *uap, rval_t *rvp)
{
	struct kmsqid_ds *qp;	/* ptr to associated message queue */
	ipcdirent_t	*dp;	/* ptr to associated directory entry */
	boolean_t	new;	/* true if new message queue */
	int		slot;	/* directory slot number */
	int		error;
	lid_t		lid;

	(void)MSQDIR_RDLOCK();
	if (error = ipcget(uap->key, uap->msgflg, &msgdata, &new, &dp)) {
		MSQDIR_UNLOCK(PLBASE);
		return error;
	}

	qp = IPC_TO_MSQDS(dp->ipcd_ent);
#ifdef CC_PARTIAL
	MAC_ASSERT(qp, MAC_SAME);
#endif
	lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

	slot = dp - msgdir.ipcdir_entries;
	rvp->r_val1 = SLOT_TO_MSQID(slot, dp->ipcd_seq);
	
	if (new == B_TRUE) {
		/*
		 * This is a new message queue.  Finish initialization.
		 * For a new message queue, the directory lock (msgdir_mutex)
		 * is held in exclusive mode.  See msgalloc() to see how
		 * this is done.
		 */
		qp->kmsq_ds.msg_first = qp->kmsq_ds.msg_last = NULL;
		qp->kmsq_ds.msg_qnum = 0;
		qp->kmsq_ds.msg_qbytes = msginfo.msgmnb;
		qp->kmsq_ds.msg_lspid = qp->kmsq_ds.msg_lrpid = 0;
		qp->kmsq_ds.msg_stime = qp->kmsq_ds.msg_rtime = 0;
		qp->kmsq_ds.msg_ctime = hrestime.tv_sec;
		qp->kmsq_ds.msg_perm.mode |= IPC_ALLOC;	/* init complete */
	}

	MSQDIR_UNLOCK(PLBASE);
	ADT_LIDCHECK(lid);
	return 0;
}

/*
 * int msgrcv(struct msgrcva *uap, rval_t *rvp)
 *	System call handler for msgrcv(2).
 *
 * Calling/Exit State:
 *	No locks are held on entry, none are held on return.  This
 *	condition is ASSERT()ed in msgsys().
 */
STATIC int
msgrcv(struct msgrcva *uap, rval_t *rvp)
{
	struct msg		*mp;		/* ptr to msg on q */
	struct msg		*pmp;		/* ptr to mp's predecessor */
	struct msg		*smp;		/* ptr to best msg on q */
	struct msg		*spmp;		/* ptr to smp's predecessor */
	struct kmsqid_ds	*qp;		/* ptr to associated msg q */
	size_t			sz;
	int			error;
	int			blocked = 0;
	cred_t			*crp = CRED();	/* callers credentials */
	lid_t			lid;

	MET_MSG();				/* bump msgop cnt (snd/rcv) */

	if (uap->msgsz < 0)
		return EINVAL;

	if ((error = msgconv(uap->msqid, &qp, 0)) != 0)
		return error;

	lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

	/* write MAC access is needed since operation modifies the object */
	if (mac_installed &&
	    (error =
	     ipcaccess(&qp->kmsq_ds.msg_perm, MSG_R|MSG_W, IPC_MAC, crp)) != 0
	    || (error =
		ipcaccess(&qp->kmsq_ds.msg_perm, MSG_R, IPC_DAC, crp)) != 0)
		goto msgrcv_out;

#ifdef CC_PARTIAL
	MAC_ASSERT(qp, MAC_SAME);
#endif

	smp = spmp = NULL;

loop:
	ASSERT(LOCK_OWNED(&qp->kmsq_mutex) && KS_HOLD1LOCK());

	pmp = NULL;
	mp = qp->kmsq_ds.msg_first;
	if (uap->msgtyp == 0) {	
		/* first msg on queue */
		smp = mp;
	} else {
		for (; mp; pmp = mp, mp = mp->msg_next) {
			if (uap->msgtyp > 0) {
				/* first msg of type 'msgtyp' */
				if (uap->msgtyp != mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
				break;
			}
			if (mp->msg_type <= -uap->msgtyp) {
				/* first msg of lowest type <= |'msgtyp'| */
				if (smp && smp->msg_type <= mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
			}
		}
	}

	if (smp != NULL) {
		if ((unsigned)uap->msgsz < smp->msg_ts) {
			if (!(uap->msgflg & MSG_NOERROR)) {
				error = E2BIG;
				goto msgrcv_out;
			} else
				sz = uap->msgsz;		/* truncate */
		} else
			sz = smp->msg_ts;

		/*
		 * Mark this message queue as busy and drop the
		 * kmsq_mutex while doing copyout().  Marking
		 * this messaqe queue as busy will block anyone
		 * trying to get a handle on it via msgconv().
		 */
		qp->kmsq_flag |= MSQID_BUSY;
		MSQID_UNLOCK(qp, PLBASE);

		if (copyout(&smp->msg_type, &uap->msgp->mtype,
			    sizeof(smp->msg_type)) ||
		    (sz != 0 &&
		      copyout(smp->msg_vaddr, &uap->msgp->mtext, sz)))
			error = EFAULT;
		
		MSQID_LOCK(qp);
		qp->kmsq_flag &= ~MSQID_BUSY;

		if (error != 0)
			goto msgrcv_out;

		rvp->r_val1 = sz;
		qp->kmsq_ds.msg_cbytes -= smp->msg_ts;
		qp->kmsq_ds.msg_lrpid = u.u_procp->p_pidp->pid_id;
		qp->kmsq_ds.msg_rtime = hrestime.tv_sec;

		/* Dequeue the message. */
		if (spmp == NULL)
			qp->kmsq_ds.msg_first = smp->msg_next;
		else
			spmp->msg_next = smp->msg_next;

		if (smp->msg_next == NULL)
			qp->kmsq_ds.msg_last = spmp;

		qp->kmsq_ds.msg_qnum--;
		if (SV_BLKD(&qp->kmsq_snd_sv))
			SV_BROADCAST(&qp->kmsq_snd_sv, KS_NOPRMPT);

		if (SV_BLKD(&qp->kmsq_sv))
			SV_BROADCAST(&qp->kmsq_sv, 0);

		MSQID_UNLOCK(qp, PLBASE);
		ADT_LIDCHECK(lid);

		/* Free resources associated with this message. */
		sz = smp->msg_ts;
		kmem_free(smp, sizeof(struct msg) + sz);
		msgunresv(sz, 1);
		return 0;
	}

	if (uap->msgflg & IPC_NOWAIT) {
		error = ENOMSG;
		goto msgrcv_out;
	}

	qp->kmsq_ds.msg_perm.mode |= MSG_RWAIT;	

	/*
	 * Slight optimization here.  If nobody is waiting
	 * for a specific message type (everybody is waiting
	 * for any message) then msgsnd() will do a SV_SIGNAL()
	 * to kick a single context which is guaranteed to
	 * accept the message.  Otherwise, msgsnd() will do
	 * a SV_BROADCAST() to let the receivers thrash it out.
	 */
	if (uap->msgtyp != 0)
		qp->kmsq_rcvspec++;

	blocked++;
	if (!SV_WAIT_SIG(&qp->kmsq_rcv_sv, PRIMSG, &qp->kmsq_mutex))
		error = EINTR;

	ASSERT(smp == NULL && spmp == NULL);
	ASSERT(KS_HOLD0LOCKS());

	if (msgconv(uap->msqid, &qp, 0) != 0) {
		ADT_LIDCHECK(lid);
		return EIDRM;
	}

	if (uap->msgtyp != 0)
		qp->kmsq_rcvspec--;
	
	if (error == 0)
		goto loop;

msgrcv_out:

	/*
	 * Since we did not consume the message due to an error,
	 * and msgsnd() may have done a SV_SIGNAL() instead of a
	 * SV_BROADCAST(), signal a blocked reciever.
	 */
	if (blocked && SV_BLKD(&qp->kmsq_rcv_sv)) {
		if (qp->kmsq_rcvspec == 0)
			SV_SIGNAL(&qp->kmsq_rcv_sv, KS_NOPRMPT);
		else
			SV_BROADCAST(&qp->kmsq_rcv_sv, KS_NOPRMPT);
	}

	if (SV_BLKD(&qp->kmsq_sv))
		SV_BROADCAST(&qp->kmsq_sv, 0);

	MSQID_UNLOCK(qp, PLBASE);
	ADT_LIDCHECK(lid);
	return error;
}

/*
 * int msgsnd(struct msgsnda *uap, rval_t *rvp)
 *	System call entry point for msgsnd(2).
 *
 * Calling/Exit State:
 *	No locks are held on entry, none are held on return.
 *	This condition is ASSERT()ed in msgsys().
 */
STATIC int
msgsnd(struct msgsnda *uap, rval_t *rvp)
{
	struct kmsqid_ds	*qp;	/* ptr to associated q */
	struct msg		*mp;	/* ptr to allocated msg hdr */
	int			cnt;	/* byte count */
	long			mtype;	/* msg type */
	int			error;
	int			again;
	lid_t			lid;

	MET_MSG();			/* bump msgop count (snd/rcv) */

	if (copyin((caddr_t)uap->msgp, &mtype, sizeof mtype))
		return EFAULT;

	if ((cnt = uap->msgsz) < 0 || cnt > msginfo.msgmax || mtype < 1)
		return EINVAL;

	if (error = msgconv(uap->msqid, &qp, 0))
		return error;

	lid = qp->kmsq_ds.msg_perm.ipc_secp->ipc_lid;

	if (error = ipcaccess(&qp->kmsq_ds.msg_perm, MSG_W, IPC_MAC|IPC_DAC,
			      CRED()))
		goto msgsnd_out;

#ifdef CC_PARTIAL
	MAC_ASSERT(qp, MAC_SAME);
#endif

	again = 0;
getres:
	if (again) {
		/* Be sure that msg queue has not been removed. */

		if ((error = msgconv(uap->msqid, &qp, 0)) != 0) {
			ADT_LIDCHECK(lid);
			return EIDRM;
		}
	} else
		again = 1;

	ASSERT(LOCK_OWNED(&qp->kmsq_mutex) && KS_HOLD1LOCK());

	/* Allocate space on q, message header, & buffer space. */
	if (cnt + qp->kmsq_ds.msg_cbytes > (uint)qp->kmsq_ds.msg_qbytes) {
		if (uap->msgflg & IPC_NOWAIT) {
			error = EAGAIN;
			goto msgsnd_out;
		}
		qp->kmsq_ds.msg_perm.mode |= MSG_WWAIT;
		if (!SV_WAIT_SIG(&qp->kmsq_snd_sv, PRIMSG, &qp->kmsq_mutex)) {
			ADT_LIDCHECK(lid);
			return EINTR;
		}
		goto getres;
	}

	if ((error = msgresv(qp, cnt, uap->msgflg)) != 0) {
		if (error == -1) {
			ASSERT(!(uap->msgflg & IPC_NOWAIT));
			goto getres;		/* try again */
		}
		ADT_LIDCHECK(lid);
		return error;
	}

	/* Prepare to block for KMA and copyout(). */
	qp->kmsq_flag |= MSQID_BUSY;
	MSQID_UNLOCK(qp, PLBASE);

	mp = kmem_alloc(sizeof(struct msg) + cnt, KM_SLEEP);

	/* Everything is available, copy in text and put msg on queue. */
	if (cnt != 0) {
		mp->msg_vaddr = mp + 1;
		if (copyin((caddr_t)((vaddr_t)uap->msgp + sizeof(mtype)),
			   mp->msg_vaddr, cnt)) {
			error = EFAULT;
			kmem_free(mp, sizeof(struct msg) + cnt);
			msgunresv(cnt, 1);
		}
	} else
		mp->msg_vaddr = NULL;

	MSQID_LOCK(qp);
	qp->kmsq_flag &= ~MSQID_BUSY;

	if (error)
		goto msgsnd_out;
	qp->kmsq_ds.msg_qnum++;
	qp->kmsq_ds.msg_cbytes += cnt;
	qp->kmsq_ds.msg_lspid = u.u_procp->p_pidp->pid_id;
	qp->kmsq_ds.msg_stime = hrestime.tv_sec;

	mp->msg_next = NULL;
	mp->msg_type = mtype;
	mp->msg_ts = cnt;

	/* enqueue the message */
	if (qp->kmsq_ds.msg_last == NULL)
		qp->kmsq_ds.msg_first = qp->kmsq_ds.msg_last = mp;
	else {
		qp->kmsq_ds.msg_last->msg_next = mp;
		qp->kmsq_ds.msg_last = mp;
	}

	if (SV_BLKD(&qp->kmsq_rcv_sv)) {
		/*
		 * If no one is blocked waiting for a specific message
		 * type, just unblock a single receiver.  Otherwise,
		 * unblock all receivers.
		 */
		if (qp->kmsq_rcvspec == 0)
			SV_SIGNAL(&qp->kmsq_rcv_sv, KS_NOPRMPT);
		else
			SV_BROADCAST(&qp->kmsq_rcv_sv, KS_NOPRMPT);

	}
	rvp->r_val1 = 0;

msgsnd_out:

	ASSERT(LOCK_OWNED(&qp->kmsq_mutex));

	if (SV_BLKD(&qp->kmsq_sv))
		SV_BROADCAST(&qp->kmsq_sv, 0);
	MSQID_UNLOCK(qp, PLBASE);
	ADT_LIDCHECK(lid);
	return error;
}

/*
 * int msgsys(union msgsysa *uap, rval_t *rvp)
 *	Entry point for msgctl(2), msgget(2), msgrcv(2),
 *	and msgsnd(2) system calls.
 *
 * Calling/Exit State:
 *	No locks are held on entry, none are held on return.
 */
int
msgsys(union msgsysa *uap, rval_t *rvp)
{
	int	error;

	ASSERT(KS_HOLD0LOCKS());

	switch (uap->opcode) {
	case MSGGET:
		error = msgget(&uap->msggeta, rvp);
		break;
	case MSGCTL:
		error = msgctl(&uap->msgctla, rvp);
		break;
	case MSGRCV:
		error = msgrcv(&uap->msgrcva, rvp);
		break;
	case MSGSND:
		error = msgsnd(&uap->msgsnda, rvp);
		break;
	default:
		error = EINVAL;
		break;
	}

	ASSERT(KS_HOLD0LOCKS());

	return error;
}

/*
 * void msginit(void)
 *	One time initialization routine for System V message queue IPC.
 *
 * Calling/Exit State:
 *	Called early during system initialization.
 *	KMA must be available.
 */
void
msginit(void)
{
	const int n = msginfo.msgmni;

	RW_INIT(&msgdir_mutex, MSGDIR_HIER, PLMIN, &msgdir_lkinfo,
			KM_NOSLEEP);
	LOCK_INIT(&msgresv_mutex, MSGRESV_HIER, PLMIN, &msgresv_lkinfo,
			KM_NOSLEEP);
	SV_INIT(&msghd_sv);
	SV_INIT(&msgtxt_sv);

	/*
	 * Set total number of bytes of message text allowed.
	 * Used by msgresv()/msgunresv().
	 */
	msgstat.msgtxt = msginfo.msgssz * msginfo.msgseg;

	/* Initialize the IPC directory for message queues */
	msgdir.ipcdir_nents = n;
	msgdir.ipcdir_nactive = 0;
	msgdir.ipcdir_entries =
		kmem_zalloc(n * sizeof(ipcdirent_t), KM_NOSLEEP);

	if (msgdir.ipcdir_entries == NULL && n != 0) {
		/*
		 *+ Could not allocate memory for the SystemV
		 *+ message queue IPC directory.
		 *+ Instead of PANIC'ing the system, this IPC mechanism
		 *+ is disabled and a warning message is printed.
		 *+ The system configuration parameter msginfo.msgmni
		 *+ should be checked to make sure that it is not
		 *+ inordinately large.
		 */
		cmn_err(CE_WARN,
		"msginit: Can't alloc IPC directory, message queues disabled");

		/*
		 * Setting ipcdir_nents to zero will cause
		 * ipcget() to always fail with ENOSPC and
		 * msgconv() to fail with EINVAL.
		 */
		msgdir.ipcdir_nents = 0;
	}
}
