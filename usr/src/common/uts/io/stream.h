/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STREAM_H	/* wrapper symbol for kernel use */
#define _IO_STREAM_H	/* subject to change without notice */

#ident	"@(#)kern:io/stream.h	1.24"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*#define STRPERF	1 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <proc/cred.h>		/* REQUIRED */
#include <io/strmdep.h>		/* SVR4.0COMPAT */
#include <util/engine.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */
#include <sys/strmdep.h>	/* SVR4.0COMPAT */
#include <sys/engine.h>

#else

/*
 * For source compatibility
 */
#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/cred.h>		/* SVR4.0COMPAT */
#include <sys/strmdep.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Data queue.
 *
 * Note that changes in the queue structure are mutexed via its stream's
 * sd_mutex.  The exceptions are q_link and q_svcflag - they are mutexed
 * by the svc_mutex.
 */
struct queue {
	struct qinit	*q_qinfo;	/* procs and limits for queue */
	struct msgb	*q_first;	/* first data block */
	struct msgb	*q_last;	/* last data block */
	struct queue	*q_next;	/* queue of next stream */
	struct queue	*q_link;	/* to next queue for scheduling */
	void		*q_ptr;		/* to private data structure */
	ulong_t		q_count;	/* number of bytes on queue */
	ulong_t		q_flag;		/* queue state */
	long		q_minpsz;	/* min packet size accepted by module */
	long		q_maxpsz;	/* max packet size accepted by module */
	ulong_t		q_hiwat;	/* queue high water mark */
	ulong_t		q_lowat;	/* queue low water mark */
	struct qband	*q_bandp;	/* separate flow information */
	uchar_t		q_nband;	/* number of priority bands > 0 */
	uchar_t		q_blocked;	/* number of bands flow controlled */
	uchar_t		q_svcflag;	/* svc flags (mutexed by svc_mutex) */
	struct stdata	*q_str;		/* synchronization of entire stream */
	struct queue	*q_blink;	/* to previous queue for scheduling */
	int		(*q_putp)();	/* actual put procedure */
	uint_t		q_putcnt;	/* number of put procs running */
	uint_t		q_defcnt;	/* number of deferred puts (UP) */
};

typedef struct queue queue_t;

/*
 * Queue flags
 */
#define QMOVED		0x0001		/* bit moved to new location (q_svcflag) */
#define	QWANTR		0x0002		/* someone wants to read queue */
#define	QWANTW		0x0004		/* someone wants to write queue */
#define	QFULL		0x0008		/* queue is considered full */
#define	QREADR		0x0010		/* this is the read queue */
#define	QUSE		0x0020		/* this queue in use (allocation) */
#define	QNOENB		0x0040		/* don't enable queue via putq */
#define QUP		0x0080		/* uniprocessor queues */
#define QBACK		0x0100		/* queue has been back-enabled */
#define QINTER		0x0200		/* put routine should be intercepted */
#define QPROCSON	0x0400		/* queue's procs are enabled */
#define QTOENAB		0x0800		/* queue is to be qenabled when unfrozen */
#define QFREEZE		0x1000		/* freeze all queue activity */
#define QBOUND		0x2000		/* queue is part of bound stream */

/*
 * Queue service flags
 */
#define	QENAB		0x0001		/* service procedure is scheduled to run */
#define QSVCBUSY	0x0002		/* service procedure is running */

/*
 * Structure that describes the separate information
 * for each priority band in the queue.
 */
struct qband {
	struct qband	*qb_next;	/* next band's info */
	ulong		qb_count;	/* number of bytes in band */
	struct msgb	*qb_first;	/* beginning of band's data */
	struct msgb	*qb_last;	/* end of band's data */
	ulong		qb_hiwat;	/* high water mark for band */
	ulong		qb_lowat;	/* low water mark for band */
	ulong		qb_flag;	/* see below */
	long		qb_pad1;	/* reserved for future use */
};

typedef struct qband qband_t;

/*
 * qband flags
 */
#define QB_FULL		0x01		/* band is considered full */
#define QB_WANTW	0x02		/* someone wants to write to band */
#define QB_BACK		0x04		/* queue has been back-enabled */

/*
 * Maximum number of bands.
 */
#define NBAND	256

/*
 * Fields that can be manipulated through strqset() and strqget().
 */
typedef enum qfields {
	QHIWAT	= 0,		/* q_hiwat or qb_hiwat */
	QLOWAT	= 1,		/* q_lowat or qb_lowat */
	QMAXPSZ	= 2,		/* q_maxpsz */
	QMINPSZ	= 3,		/* q_minpsz */
	QCOUNT	= 4,		/* q_count or qb_count */
	QFIRST	= 5,		/* q_first or qb_first */
	QLAST	= 6,		/* q_last or qb_last */
	QFLAG	= 7,		/* q_flag or qb_flag */
	QBAD	= 8
} qfields_t;

/*
 * Module information structure
 */
struct module_info {
	ushort	mi_idnum;		/* module id number */
	char 	*mi_idname;		/* module name */
	long	mi_minpsz;		/* min packet size accepted */
	long	mi_maxpsz;		/* max packet size accepted */
	ulong	mi_hiwat;		/* hi-water mark */
	ulong 	mi_lowat;		/* lo-water mark */
};

/*
 * Queue information structure.  On the read side, qi_open and qi_close
 * contain the module's open and close routines, respectively.  On the
 * write side, qi_open and qi_close are ignored.
 */
struct qinit {
	int	(*qi_putp)();		/* put procedure */
	int	(*qi_srvp)();		/* service procedure */
	int	(*qi_qopen)();		/* called on startup */
	int	(*qi_qclose)();		/* called on finish */
	int	(*qi_qadmin)();		/* for future use */
	struct module_info *qi_minfo;	/* module information structure */
	struct module_stat *qi_mstat;	/* module statistics structure */
};

/*
 * Streamtab (used in cdevsw and fmodsw to point to module or driver)
 */
struct streamtab {
	struct qinit *st_rdinit;
	struct qinit *st_wrinit;
	struct qinit *st_muxrinit;
	struct qinit *st_muxwinit;
};

/*
 * Structure sent to mux drivers to indicate a link.
 */
struct linkblk {
	queue_t *l_qtop;	/* lowest level write queue of upper stream */
				/* (set to NULL for persistent links) */
	queue_t *l_qbot;	/* highest level write queue of lower stream */
	int      l_index;	/* index for lower stream. */
	long	 l_pad[5];	/* reserved for future use */
};

/*
 * Class 0 data buffer freeing routine
 */
struct free_rtn {
	void (*free_func)();
	char *free_arg;
};

/*
 *  Data block descriptor
 */
struct datab {
	struct free_rtn *db_frtnp;	/* externally-supplied buffer info */
	uchar_t		*db_base;	/* start of data buffer */
	uchar_t		*db_lim;	/* one byte past end of data buffer */
	uchar_t		db_ref;		/* reference count */
	uchar_t		db_type;	/* message type */
	uchar_t		db_muse;	/* mblk inuse indicator */
	uint_t		db_size;	/* size of memory to kmem_free */
	caddr_t		db_addr;	/* starting address to kmem_free */
	struct datab	*db_odp;	/* old dblk left over after pullup */
	engine_t	*db_cpu;	/* cpu to be freed on when esb */
};


/*
 * Message block descriptor
 */
struct msgb {
	struct msgb	*b_next;
	struct msgb	*b_prev;
	struct msgb	*b_cont;
	unsigned char	*b_rptr;
	unsigned char	*b_wptr;
	struct datab 	*b_datap;
	unsigned char	b_band;
	unsigned char	b_pad1;
	unsigned short	b_flag;
	long		b_pad2;
#ifdef STRPERF
	int		b_sh;		/* stream head time */
	int		b_oh;		/* streams overhead */
	int		b_sched;	/* scheduling delays */
	int		b_work;		/* module/driver work */
	int		b_copyin;	/* copyin time */
	int		b_copyout;	/* copyout time */
	int		b_stamp;	/* time stamp */
	int		b_life;		/* message lifetime */
#endif
};

typedef struct msgb mblk_t;
typedef struct datab dblk_t;
typedef struct free_rtn frtn_t;

#ifdef STRLEAK
/*
 * overlay the message with debug info
 */
struct mbinfo {
	mblk_t	mb_msg;			/* message portion */
	dblk_t	mb_data;		/* data portion */
	long	mb_func;		/* allocating address */
	mblk_t	*mb_next;		/* in-use forward pointer */
	mblk_t	*mb_prev;		/* in-use previous pointer */
	long	mb_pad[5];		/* make struct into a power of 2 */
};
#endif


/*
 * Message flags.  These are interpreted by the stream head.
 */
#define MSGMARK		0x01		/* last byte of message is "marked" */
#define MSGNOLOOP	0x02		/* don't loop message around to */
					/* write side of stream */
#define MSGDELIM	0x04		/* message is delimited */
#define MSGNOGET	0x08		/* getq does not return message */
#define MSGLOG		0x10		/* handshake between strlog and strdaemon */
#define MSGFLIP		0x20		/* flipped triplet for alignment */

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

/*
 * Streams message types.
 */

/*
 * Data and protocol messages (regular and priority)
 */
#define	M_DATA		0x00		/* regular data */
#define M_PROTO		0x01		/* protocol control */

/*
 * Control messages (regular and priority)
 */
#define	M_BREAK		0x08		/* line break */
#define M_PASSFP	0x09		/* pass file pointer */
#define	M_SIG		0x0b		/* generate process signal */
#define	M_DELAY		0x0c		/* real-time xmit delay (1 param) */
#define M_CTL		0x0d		/* device-specific control message */
#define	M_IOCTL		0x0e		/* ioctl; set/get params */
#define M_SETOPTS	0x10		/* set various stream head options */
#define M_RSE		0x11		/* reserved for RSE use only */
#define M_TRAIL		0x12		/* data trailer */

/*
 * Control messages (high priority; go to head of queue)
 */
#define	M_IOCACK	0x81		/* acknowledge ioctl */
#define	M_IOCNAK	0x82		/* negative ioctl acknowledge */
#define M_PCPROTO	0x83		/* priority proto message */
#define	M_PCSIG		0x84		/* generate process signal */
#define	M_READ		0x85		/* generate read notification */
#define	M_FLUSH		0x86		/* flush your queues */
#define	M_STOP		0x87		/* stop transmission immediately */
#define	M_START		0x88		/* restart transmission after stop */
#define	M_HANGUP	0x89		/* line disconnect */
#define M_ERROR		0x8a		/* fatal error used to set u.u_error */
#define M_COPYIN	0x8b		/* request to copyin data */
#define M_COPYOUT	0x8c		/* request to copyout data */
#define M_IOCDATA	0x8d		/* response to M_COPYIN and M_COPYOUT */
#define M_PCRSE		0x8e		/* reserved for RSE use only */
#define	M_STOPI		0x8f		/* stop reception immediately */
#define	M_STARTI	0x90		/* restart reception after stop */
#define M_PCCTL		0x91		/* priority control message */
#define M_PCSETOPTS	0x92		/* priority set stream head options */

/*
 * Queue message class definitions.  
 */
#define QNORM		0x00		/* normal priority messages */
#define QPCTL		0x80		/* high priority cntrl messages */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 *  IOCTL structure - this structure is the format of the M_IOCTL message type.
 */
struct iocblk {
	int 	ioc_cmd;		/* ioctl command type */
	cred_t	*ioc_cr;		/* full credentials */
	uint	ioc_id;			/* ioctl id */
	uint	ioc_count;		/* count of bytes in data field */
	int	ioc_error;		/* error code */
	int	ioc_rval;		/* return value  */
	long	ioc_filler[4];		/* reserved for future use */
};

#define ioc_uid ioc_cr->cr_uid
#define ioc_gid ioc_cr->cr_gid

/*
 * structure for the M_COPYIN and M_COPYOUT message types.
 */
struct copyreq {
	int	cq_cmd;			/* ioctl command (from ioc_cmd) */
	cred_t	*cq_cr;			/* full credentials */
	uint	cq_id;			/* ioctl id (from ioc_id) */
	caddr_t	cq_addr;		/* address to copy data to/from */
	uint	cq_size;		/* number of bytes to copy */
	int	cq_flag;		/* see below */
	mblk_t *cq_private;		/* privtate state information */
	long	cq_filler[4];		/* reserved for future use */
};

#define cq_uid cq_cr->cr_uid
#define cq_gid cq_cr->cr_gid

/* cq_flag values */

#define STRCANON	0x01		/* b_cont data block contains */
					/* canonical format specifier */
#define RECOPY		0x02		/* perform I_STR copyin again, */
					/* this time using canonical */
					/* format specifier */

/*
 * structure for the M_IOCDATA message type.
 */
struct copyresp {
	int	cp_cmd;			/* ioctl command (from ioc_cmd) */
	cred_t	*cp_cr;			/* full credentials */
	uint	cp_id;			/* ioctl id (from ioc_id) */
	caddr_t	cp_rval;		/* status of request: 0 -> success */
					/*             non-zero -> failure */
	uint	cp_pad1;		/* reserved */
	int	cp_pad2;		/* reserved */
	mblk_t *cp_private;		/* private state information */
	long	cp_filler[4];		/* reserved for future use */
};

#define cp_uid cp_cr->cr_uid
#define cp_gid cp_cr->cr_gid

/*
 * Options structure for M_SETOPTS message.  This is sent upstream
 * by a module or driver to set stream head options.
 */
struct stroptions {
	ulong	so_flags;		/* options to set */
	short	so_readopt;		/* read option */
	ushort	so_wroff;		/* write offset */
	long	so_minpsz;		/* minimum read packet size */
	long	so_maxpsz;		/* maximum read packet size */
	ulong	so_hiwat;		/* read queue high water mark */
	ulong	so_lowat;		/* read queue low water mark */
	unsigned char so_band;		/* band for water marks */
};

/* flags for stream options set message */

#define SO_ALL		0x003f	/* set all old options */
#define SO_READOPT	0x0001	/* set read option */
#define SO_WROFF	0x0002	/* set write offset */
#define SO_MINPSZ	0x0004	/* set min packet size */
#define SO_MAXPSZ	0x0008	/* set max packet size */
#define SO_HIWAT	0x0010	/* set high water mark */
#define SO_LOWAT	0x0020	/* set low water mark */
#define SO_MREADON      0x0040	/* set read notification ON */
#define SO_MREADOFF     0x0080	/* set read notification OFF */
#define SO_NDELON	0x0100	/* old TTY semantics for NDELAY reads/writes */
#define SO_NDELOFF      0x0200	/* STREAMS semantics for NDELAY reads/writes */
#define SO_ISTTY	0x0400	/* the stream is acting as a terminal */
#define SO_ISNTTY	0x0800	/* the stream is not acting as a terminal */
#define SO_TOSTOP	0x1000	/* stop on background writes to this stream */
#define SO_TONSTOP	0x2000	/* do not stop on background writes to stream */
#define SO_BAND		0x4000	/* water marks affect band */
#define SO_DELIM	0x8000	/* messages are delimited */
#define SO_NODELIM	0x010000	/* turn off delimiters */
#define SO_STRHOLD	0x020000	/* obsolete, for compat. only */
#define SO_LOOP		0x040000	/* loopback that supports M_PASSFP */

/*
 * Miscellaneous parameters and flags.
 */

/*
 * New code for two-byte M_ERROR message.
 */
#define NOERROR	((unsigned char)-1)

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

/*
 * Values for stream flag in open to indicate module open, clone open;
 * return value for failure.
 */
#define DRVOPEN		0x0		/* open as a driver */
#define MODOPEN 	0x1		/* open as a module */
#define CLONEOPEN	0x2		/* open for clone, pick own minor device */
#define OPENFAIL	-1		/* backwards compatibility for open failure */

/*
 * Priority definitions for block allocation.
 */
#define BPRI_LO		1
#define BPRI_MED	2
#define BPRI_HI		3

/*
 * Value for packet size that denotes infinity
 */
#define INFPSZ		-1

/*
 * Flags for flushq()
 */
#define FLUSHALL	1	/* flush all messages */
#define FLUSHDATA	0	/* don't flush control messages */

/*
 * Flag for transparent ioctls
 */
#define TRANSPARENT	(unsigned int)(-1)

/*
 * Sleep priorities for stream I/O (for sleep() compatibility routine only).
 * New drivers should use primed or one of the other priority values from
 * ksynch.h instead.  (The closest equivalent would be (primed - 3), and
 * since it was a signalable priority it should be used with SV_WAIT_SIG.)
 */
#define	STIPRI	(PZERO + 3)
#define	STOPRI	(PZERO + 3)

/*
 * Stream head default high/low water marks 
 */
#define STRHIGH 5120
#define STRLOW	1024

/*
 * Block allocation parameters
 */
#define MAXIOCBSZ	1024		/* max ioctl data block size */

/*
 * amount of time to hold small messages in strwrite hoping to to
 * able to append more data from a subsequent write.  one tick min.
 */
#define STRSCANP	((10*HZ+999)/1000)	/* 10 ms in ticks */

/*
 * Definitions of Streams macros and function interfaces.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * canenable - check if queue can be enabled by putq().
 */
#define canenable(q)	!((q)->q_flag & QNOENB)

/*
 * Extract queue class of message block.
 */
#define queclass(bp) (((bp)->b_datap->db_type >= QPCTL) ? QPCTL : QNORM)

/*
 * Align address on next lower word boundary.
 */
#define straln(a)	(caddr_t)((long)(a) & ~(sizeof(int)-1))

/*
 * Find the max size of data block.
 */
#define bpsize(bp) ((unsigned int)(bp->b_datap->db_lim - bp->b_datap->db_base))

/*
 * declarations of common routines
 */
#ifdef STRPERF
extern int castimer(void);
#endif
struct physreq;

extern mblk_t *allocb(int, uint_t);
extern mblk_t *allocb_physreq(int, uint_t, struct physreq *);
extern mblk_t *msgphysreq(mblk_t *, struct physreq *);
extern mblk_t *esballoc(uchar_t *, int, int, frtn_t *);
extern int esbbcall(int, void (*)(), long);
extern int testb(int, uint_t);
extern toid_t bufcall(uint_t, int, void (*)(), long);
extern void freeb(mblk_t *);
extern void freemsg(mblk_t *);
extern mblk_t *dupb(mblk_t *);
extern mblk_t *dupmsg(mblk_t *);
extern mblk_t *copyb(mblk_t *);
extern mblk_t *copymsg(mblk_t *);
extern void linkb(mblk_t *, mblk_t *);
extern mblk_t *unlinkb(mblk_t *);
extern mblk_t *rmvb(mblk_t *, mblk_t *);
extern int pullupmsg(mblk_t *, int);
extern mblk_t *msgpullup(mblk_t *, int);
extern mblk_t *msgpullup_physreq(mblk_t *, int, struct physreq *);
extern int adjmsg(mblk_t *, int);
extern int msgdsize(mblk_t *);
extern mblk_t *getq(queue_t *);
extern void rmvq(queue_t *, mblk_t *);
extern void flushq(queue_t *, int);
extern void flushband(queue_t *, uchar_t, int);
extern int canput(queue_t *);
extern int canputnext(queue_t *);
extern int bcanput(queue_t *, uchar_t);
extern int bcanputnext(queue_t *, uchar_t);
extern void put(queue_t *, mblk_t *);
extern int putq(queue_t *, mblk_t *);
extern int putbq(queue_t *, mblk_t *);
extern int insq(queue_t *, mblk_t *, mblk_t *);
extern int putctl(queue_t *, int);
extern int putnextctl(queue_t *, int);
extern int putctl1(queue_t *, int, int);
extern int putnextctl1(queue_t *, int, int);
extern int putnext(queue_t *, mblk_t *);
extern void qreply(queue_t *, mblk_t *);
extern void qenable(queue_t *);
extern int qsize(queue_t *);
extern void noenable(queue_t *);
extern void enableok(queue_t *);
extern int strqset(queue_t *, qfields_t, uchar_t, long);
extern int strqget(queue_t *, qfields_t, uchar_t, long *);
extern void unbufcall(toid_t);
extern pl_t freezestr(queue_t *);
extern void unfreezestr(queue_t *, pl_t);
extern void qprocson(queue_t *);
extern void qprocsoff(queue_t *);
extern int SAMESTR(queue_t *);
extern queue_t *RD(queue_t *);
extern queue_t *WR(queue_t *);
extern queue_t *OTHERQ(queue_t *);
extern int datamsg(uchar_t);
extern int pcmsg(uchar_t);
extern int strioccall(int (*)(), caddr_t, long, queue_t *);

/*
 * shared or externally configured data structures
 */
extern int nstrpush;			/* maxmimum number of pushes allowed */

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STREAM_H */
