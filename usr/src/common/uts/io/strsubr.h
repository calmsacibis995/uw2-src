/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STRSUBR_H	/* wrapper symbol for kernel use */
#define _IO_STRSUBR_H	/* subject to change without notice */

#ident	"@(#)kern:io/strsubr.h	1.35"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* #define STRPERF 1 */

/*
 * WARNING:
 * Everything in this file is private, belonging to the
 * STREAMS subsystem.  The only guarantee made about the
 * contents of this file is that if you include it, your
 * code will not port to the next release.
 */

#ifdef _KERNEL_HEADERS

#include <fs/vnode.h>		/* SVR4.0COMPAT */
#include <io/poll.h>		/* SVR4.2COMPAT */
#include <io/stream.h>		/* REQUIRED */
#include <proc/session.h>	/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/mod/mod_k.h>	/* SVR4.2COMPAT */
#include <util/types.h>		/* REQUIRED */
#include <mem/kmem.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/vnode.h>		/* SVR4.0COMPAT */
#include <sys/poll.h>		/* SVR4.2COMPAT */
#include <sys/stream.h>		/* REQUIRED */
#include <sys/session.h>	/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/mod_k.h>		/* SVR4.2COMPAT */
#include <sys/types.h>		/* REQUIRED */
#include <sys/kmem.h>		/* REQUIRED */

#else

#include <sys/vnode.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Header for a stream: interface to rest of system.
 */
typedef struct stdata {
	lock_t *sd_mutex;		/* lock for stream mutexing */
	sleep_t *sd_plumb;		/* used to wait during plumbing ops */
	sv_t *sd_open;			/* used to wait during open */
	sv_t *sd_freeze;		/* used to wait for freeze completion */
	sv_t *sd_read;			/* used to wait for read/getmsg completion */
	sv_t *sd_write;			/* used to wait for write/putmsg completion */
	sv_t *sd_timer;			/* used to wait for data drain on close */
	sv_t *sd_timer2;		/* used to wait for control of ioctl mechanism */
	sv_t *sd_timer3;		/* used to wait for ioctl completion */
	sv_t *sd_waitbuf;		/* used to wait for buffers */
	sv_t *sd_upblock;		/* used to wait for message insertion */
	struct queue *sd_wrq;		/* write queue */
	struct msgb *sd_iocblk;		/* return block for ioctl */
	struct vnode *sd_vnode;		/* pointer to associated vnode */
	struct streamtab *sd_strtab;	/* pointer to streamtab for stream */
	long sd_flag;			/* state/flags */
	long sd_iocid;			/* ioctl id */
	ushort_t sd_iocwait;		/* count of procs waiting to do ioctl */
	short sd_pevents;		/* temp holding spot for pending poll events */
	struct sess *sd_sessp;		/* controlling session info */
	struct pid *sd_pgidp;		/* controlling process group info */
	ushort_t sd_wroff;		/* write offset */
	short sd_pushcnt;		/* number of pushes done on stream */
	int sd_rerror;			/* read error to set u.u_error */
	int sd_werror;			/* write error to set u.u_error */
	int sd_sigflags;		/* logical OR of all siglist events */
	struct strevent *sd_siglist;	/* pid linked list to rcv SIGPOLL sig */
	struct pollhead *sd_pollist;	/* list of all pollers to wake up */
	struct msgb *sd_mark;		/* "marked" message on read queue */
	int sd_closetime;		/* time to wait to drain q in close */
	int sd_upbcnt;			/* # of active write puts, for UP */
	struct engine *sd_cpu;		/* cpu to which stream is bound */
} stdata_t;

/*
 * stdata flag field defines
 */
#define	IOCWAIT		0x00000001	/* Someone wants to do ioctl */
#define RSLEEP		0x00000002	/* Someone wants to read/recv msg */
#define	WSLEEP		0x00000004	/* Someone wants to write */
#define STRPRI		0x00000008	/* An M_PCPROTO is at stream head */
#define	STRHUP		0x00000010	/* Device has vanished */
#define STWOPEN		0x00000020	/* open/close op is active */
#define STPLEX		0x00000040	/* stream is being multiplexed */
#define STRISTTY	0x00000080	/* stream is a terminal */
#define RMSGDIS		0x00000100	/* read msg discard */
#define RMSGNODIS	0x00000200	/* read msg no discard */
#define STRDERR		0x00000400	/* fatal read error from M_ERROR */
#define STRTIME		0x00000800	/* used with timeout strtime */
#define UPF		0x00001000	/* UP-friendly stream */
#define STR3TIME	0x00002000	/* used with timeout str3time */
#define UPBLOCK		0x00004000	/* UP push is blocked */

#define SNDMREAD	0x00008000	/* used for read notification */
#define OLDNDELAY	0x00010000	/* use old TTY semantics for NDELAY
					/* reads and writes */
#define STRTOHUP	0x00020000	/* waiting for M_TRAIL */
#define STRSNDZERO	0x00040000	/* send 0-length msg. down pipe/FIFO */
#define STRTOSTOP	0x00080000	/* block background writes */
#define	RDPROTDAT	0x00100000	/* read M_[PC]PROTO contents as data */
#define RDPROTDIS	0x00200000	/* discard M_[PC]PROTO blocks and */
					/* retain data blocks */
#define STRMOUNT	0x00400000	/* stream is mounted */
#define STRSIGPIPE	0x00800000	/* send SIGPIPE on write errors */
#define STRDELIM	0x01000000	/* generate delimited messages */
#define STWRERR		0x02000000	/* fatal write error from M_ERROR */
#define STRHOLD		0x04000000	/* obsolete - maintained for compat. */
#define FLUSHWAIT	0x08000000	/* stream is being flushed */
#define STRLOOP		0x10000000	/* stream terminates with loopback */
					/* that supports M_PASSFP */
#define STRPOLL		0x20000000	/* mark stream head as having been */
					/* polled, this closes a race */
#define STRNOCTTY	0x40000000	/* remember FNOCTTY flag for I_PUSH */

/*
 * Structure that defines the list of queues to be serviced.
 */
struct qsvc {
	queue_t *qs_head;
	queue_t *qs_tail;
};

/*
 * Structure of list of processes to be sent SIGPOLL signal
 * on request, or for processes sleeping on poll().  The valid 
 * SIGPOLL events are defined in stropts.h, and the valid poll()
 * events are defined in poll.h.
 */
struct strevent {
	union {
		struct {
			struct lwp	*lwpp;
			long		events;
			struct vnode	*vp;
			struct strevent	*chain;
		} e;	/* stream event */
		struct {
			void	(*func)();
			long	arg;
			int	size;
		} b;	/* bufcall event */
	} x;
	long se_id;
	struct strevent *se_next;
};

#define se_lwpp		x.e.lwpp
#define se_events	x.e.events
#define se_vp		x.e.vp
#define se_chain	x.e.chain
#define se_func		x.b.func
#define se_arg		x.b.arg
#define se_size		x.b.size

#define SE_SLEEP	0	/* ok to sleep in allocation */
#define SE_NOSLP	1	/* don't sleep in allocation */

/* masked into se_id */
#define SE_LOCAL	0x1	/* strevent queued on local cpu list */

/*
 * ioctl postprocessing info
 */

struct striopst {
	struct striopst	*sio_next;	/* next in line */
	int		(*sio_func)();	/* function to call */
	caddr_t		sio_arg;	/* its argument */
	long		sio_iocid;	/* ioctl id */
	queue_t		*sio_q;		/* queue that receives callback */
};

/*
 * Structure that defines the list of bufcall routines to be run.
 */
struct bclist {
	struct strevent	*bc_head;
	struct strevent	*bc_tail;
};

/*
 * flag values for strbcwait
 */

#define	B_MP	0x1	/* bufcall from MP module/driver */
#define B_UP	0x2	/* bufcall from UP module/driver */

/*
 * Structure used to track mux links and unlinks.
 */
struct mux_node {
	long		 mn_imaj;	/* internal major device number */
	ushort_t	 mn_indegree;	/* number of incoming edges */
	struct mux_node *mn_originp;	/* where we came from during search */
	struct mux_edge *mn_startp;	/* where search left off in mn_outp */
	struct mux_edge *mn_outp;	/* list of outgoing edges */
	uint_t		 mn_flags;	/* see below */
};

/*
 * Flags for mux_nodes.
 */
#define VISITED	1

/*
 * Edge structure - a list of these is hung off the
 * mux_node to represent the outgoing edges.
 */
struct mux_edge {
	struct mux_node	*me_nodep;	/* edge leads to this node */
	struct mux_edge	*me_nextp;	/* next edge */
	int		 me_muxid;	/* id of link */
};

/*
 * Structure to keep track of resources that have been allocated
 * for streams - an array of these are kept, one entry per
 * resource.  This is used by crash to dump the data structures.
 */

struct strinfo {
	union {
		lock_t	st_mutex;	/* lock to protect list */
		sleep_t	st_sleep;	/* for stream heads, need sleep lock */
	} st_locks;
	void	*st_head;	/* head of in-use list */
	int	st_cnt;		/* total # allocated */
};

#define st_mutex st_locks.st_mutex
#define st_sleep st_locks.st_sleep

#define DYN_STREAM	0	/* for stream heads */
#define DYN_QUEUE	1	/* for queues */
#define DYN_LINKBLK	2	/* for mux links */
#define DYN_STREVENT	3	/* for stream event cells */
#define DYN_QBAND	4	/* for queue bands */

#define NDYNAMIC	5	/* number of different data types that are */
				/* dynamically allocated */

/*
 * The following structures are mainly used to keep track of
 * the resources that have been allocated so crash can find
 * them (they are stored in a doubly-linked list with the head
 * of it stored in the Strinfo array.  Other data may be stored
 * away here too since this is private to streams.  Pointers
 * to these objects are returned by the allocating procedures,
 * which are later passed to the freeing routine.  The data
 * structure itself must appear first because the pointer is
 * overloaded to refer to both the structure itself or its
 * envelope, depending on context.
 */

/*
 * Stream head info
 */
struct shinfo {
	stdata_t	sh_stdata;	/* must be first */
	struct shinfo	*sh_next;	/* next in list */
	struct shinfo	*sh_prev;	/* previous in list */
};

/*
 * Stream event info
 */
struct seinfo {
	struct strevent	s_strevent;	/* must be first */
	struct seinfo	*s_next;	/* next in list */
	struct seinfo	*s_prev;	/* previous in list */
};

struct module;

/*
 * Queue info
 */
struct queinfo {
	struct queue	qu_rqueue;	/* read queue - must be first */
	struct queue	qu_wqueue;	/* write queue - must be second */
	struct queinfo	*qu_next;	/* next in list */
	struct queinfo	*qu_prev;	/* previous in list */
	struct module	*qu_modp;	/* for loadable module */
	int		qu_modidx;	/* for loadable module */
};

#define QU_MODP(q)	(((struct queinfo *)(q))->qu_modp)
#define QU_MODIDX(q)	(((struct queinfo *)(q))->qu_modidx)

/*
 * Multiplexed streams info
 */
struct linkinfo {
	struct linkblk	li_lblk;	/* must be first */
	struct file	*li_fpdown;	/* file pointer for lower stream */
	struct linkinfo	*li_next;	/* next in list */
	struct linkinfo *li_prev;	/* previous in list */
};

/*
 * Qband info
 */
struct qbinfo {
	struct qband	qbi_qband;	/* must be first */
	struct qbinfo	*qbi_next;	/* next in list */
	struct qbinfo	*qbi_prev;	/* previous in list */
};

/*
 * for uniprocessor support
 */
struct strunidata {
	queue_t *su_qp;		/* the queue of interest */
	mblk_t	*su_mp;		/* the message to send */
};

#ifdef STRPERF
/*
 * streams performance stats
 */

struct strperf {
	int	sp_sh;		/* total stream head time */
	int	sp_oh;		/* total overhead time */
	int	sp_sched;	/* total scheduling latency */
	int	sp_work;	/* total module/driver work */
	int	sp_copyin;	/* total copyin time */
	int	sp_copyout;	/* total copyin time */
	int	sp_life;	/* total lifetime */
	int	sp_maxsh;	/* max stream head time */
	int	sp_maxoh;	/* max overhead time */
	int	sp_maxsched;	/* max sheduling latency */
	int	sp_maxwork;	/* max work time */
	int	sp_maxcopyin;	/* max copyin time */
	int	sp_maxcopyout;	/* max copyin time */
	int	sp_maxlife;	/* max life time */
	int	sp_cnt;		/* # of messages */
};
#endif /* STRPERF */

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

/*
 * Miscellaneous parameters and flags.
 */

/*
 * Default timeout in seconds for ioctls and close
 */
#define STRTIMOUT 15

/*
 * Flag values for stream io waiting procedure (strwaitq)
 */
#define WRITEWAIT	0x1	/* waiting for write event */
#define READWAIT	0x2	/* waiting for read event */
#define NOINTR		0x4	/* error is not to be set for signal */
#define GETWAIT		0x8	/* waiting for getmsg event */

/*
 * Copy modes for tty and I_STR ioctls
 */
#define	U_TO_K 	01			/* User to Kernel */
#define	K_TO_K  02			/* Kernel to Kernel */

/*
 * canonical structure definitions
 */

#define STRLINK		"lli"
#define STRIOCTL	"iiil"
#define STRPEEK		"iiliill"
#define STRFDINSERT	"iiliillii"
#define O_STRRECVFD	"lssc8"
#define STRRECVFD	"lllc8"
#define S_STRRECVFD	"lllsslllllllllll"
#define STRNAME		"c0"
#define STRINT		"i"
#define STRTERMIO	"ssssc12"
#define STRTERMCB	"c6"
#define STRSGTTYB	"c4i"
#define STRTERMIOS	"llllc20"
#define STRLIST		"il"
#define STRSEV		"issllc1"
#define STRGEV		"ili"
#define STREVENT	"lssllliil"
#define STRLONG		"l"
#define STRBANDINFO	"ci"
#define STRPIDT		"l"

/*
 * Tables we reference during open(2) processing.
 */
#define CDEVSW	0
#define FMODSW	1

/*
 * Mux defines.
 */
#define LINKNORMAL	0x01		/* normal mux link */
#define LINKPERSIST	0x02		/* persistent mux link */
#define LINKTYPEMASK	0x03		/* bitmask of all link types */
#define LINKCLOSE	0x04		/* unlink from strclose */
#define LINKIOCTL	0x08		/* unlink from strioctl */
#define LINKNOEDGE	0x10		/* no edge to remove from graph */

/*
 * Definitions of Streams macros and function interfaces.
 */

/*
 * straccess return codes
 */
#define CTTY_STOPPED	(-2)
#define CTTY_EOF	(-1)
#define CTTY_OK		(0)


/*
 * Macros dealing with mux_nodes.
 */
#define MUX_VISIT(X)	((X)->mn_flags |= VISITED)
#define MUX_CLEAR(X)	((X)->mn_flags &= (~VISITED)); \
			((X)->mn_originp = NULL)
#define MUX_DIDVISIT(X)	((X)->mn_flags & VISITED)

/*
 * For UP compatibility
 */
#define UNI_ID	0x756e

#if defined(_KERNEL)
/*
 * qinit structures for stream head read and write queues.
 */
extern struct qinit	strdata;
extern struct qinit	stwdata;

/*
 * Declarations of private variables.
 * Not to be referenced outside the Streams code!
 */
extern struct qsvc qsvc;		/* head of queues to run */
extern struct bclist bcall;		/* list of waiting bufcalls */
extern struct strinfo Strinfo[];	/* dynamic resource info */
extern long Strcount;			/* count of streams resources */
extern fspin_t mref_mutex;		/* serializes dupb's and freeb's */
extern lock_t bc_mutex;			/* guards bufcall list */
extern lock_t svc_mutex;		/* guards queue service list */
extern fspin_t strcnt_mutex;		/* guards strcount */
extern lock_t vstr_mutex;		/* guards v_stream */
extern fspin_t id_mutex;		/* guards ioc_id */
extern lock_t strio_mutex;		/* guards ioctl post processing list */
#ifdef STRPERF
extern fspin_t stat_mutex;		/* guards performance stats */
#endif
extern char strbcwait;			/* bufcall functions ready to go */
extern physreq_t *strphysreq;		/* constrained physreq for compat */

/*
 * Declarations of private routines.
 */
struct uio;	/* To satisfy compiler for the following prototypes */
struct strbuf;
struct engine;
struct cred;
struct file;
extern void strsendsig(struct strevent *, int, long);
extern int qattach(queue_t *, dev_t *, int, int, int, struct cred *, int);
extern void qdetach(queue_t *, int, int, struct cred *);
extern void strtime(stdata_t *);
extern void str3time(stdata_t *);
extern int putiocd(mblk_t *, mblk_t *, caddr_t, int, int, char *, stdata_t *);
extern int getiocd(mblk_t *, caddr_t, int, char *);
extern struct linkinfo *alloclink(queue_t *, queue_t *, struct file *);
extern void lbfree(struct linkinfo *);
extern int linkcycle(stdata_t *, stdata_t *);
extern struct linkinfo *findlinks(stdata_t *, int, int);
extern queue_t *getendq(queue_t *);
extern int munlink(stdata_t *, struct linkinfo *, int, struct cred *, int *);
extern int munlinkall(stdata_t *, int, struct cred *, int *);
extern int mux_addedge(stdata_t *, stdata_t *, int);
extern void setq(queue_t *, struct qinit *, struct qinit *);
extern int strmakemsg(struct strbuf *, int, struct uio *, stdata_t *, long, mblk_t **, int);
extern int strwaitbuf(int, int, stdata_t *);
extern int strwaitq(stdata_t *, int, off_t, int, int *);
extern int straccess(stdata_t *, enum jcaccess);
extern int stralloctty(struct stdata *stp);
extern void strfreectty(struct stdata *stp);
extern int xmsgsize(mblk_t *, int);
extern struct stdata *shalloc(queue_t *);
extern void shfree(stdata_t *, int);
extern queue_t *allocq(void);
extern void freeq(queue_t *);
extern qband_t *allocband(void);
extern struct strevent *sealloc(int);
extern void sefree(struct strevent *);
extern void runqueues(void);
extern int findmod(char *);
extern void setqback(queue_t *, uchar_t);
extern int strcopyin(caddr_t, caddr_t, uint_t, char *, int);
extern int strcopyout(caddr_t, caddr_t, uint_t, char *, int);
extern void strsignal(stdata_t *, int, long);
extern void qenable_l(queue_t *);
extern void setqsched(struct engine *);
extern void svc_dequeue(queue_t *, struct qsvc *);
extern queue_t *backq_l(queue_t *);
extern mblk_t *getq_l(queue_t *);
extern void strhup(stdata_t *);
extern void freezeprocs(queue_t *);
extern void struniput(struct strunidata *);
extern struct striopst *findioc(long);
extern int strintercept(queue_t *, mblk_t *);
extern mblk_t *copyb_physreq(mblk_t *, physreq_t *);
extern mblk_t *copymsg_physreq(mblk_t *, physreq_t *);
extern int putnext_l(queue_t *, mblk_t *);
extern int putbq_l(queue_t *, mblk_t *);
extern int bcanput_l(queue_t *, uchar_t);
extern void flushq_l(queue_t *, int);
extern int SAMESTR_l(queue_t *);
extern int putq_l(queue_t *, mblk_t *);
extern int getplumb(stdata_t *);
extern void relplumb(stdata_t *);
extern mblk_t *mblkprune(mblk_t *);

#endif /* defined(_KERNEL) */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_STRSUBR_H */
