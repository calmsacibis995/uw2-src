/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_MSG_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_MSG_H	/* subject to change without notice */

#ident	"@(#)kern:proc/ipc/msg.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/ipc/ipc.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/ipc.h>		/* REQUIRED */

#else 

#include <sys/ipc.h>

#endif /* _KERNEL_HEADERS */

/*
 * X/Open XPG4 types.
 */

typedef unsigned long	msgqnum_t;
typedef unsigned long	msglen_t;

#ifndef _PID_T
#define _PID_T
typedef long	pid_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long	time_t;
#endif

#ifndef _KEY_T
#define _KEY_T
typedef int	key_t;
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int	size_t;
#endif


/*
 * IPC Message Facility.
 */

/*
 * Implementation Constants.
 */

#if !defined(_XOPEN_SOURCE)
#define	PRIMSG	(PRIMED)		/* message facility sleep priority */
#endif /* !defined(_XOPEN_SOURCE)*/


/*
 * Padding constants used to reserve space for future use.
 */

#define	MSG_PAD		4

/*
 * Permission Definitions.
 */
#define	MSG_R	IPC_R	/* read permission */
#define	MSG_W	IPC_W	/* write permission */

/*
 * ipc_perm Mode Definitions.
 */

#define	MSG_RWAIT	01000	/* a reader is waiting for a message */
#define	MSG_WWAIT	02000	/* a writer is waiting to send */

/*
 * Message Operation Flags.
 */

#define	MSG_NOERROR	010000	/* no error if message to big on rcv */


#if !defined(_STYPES)

/*
 * There is one msg queue id data structure for each msg queue in the system.
 */
struct msqid_ds {
	struct ipc_perm	msg_perm;	/* operation permission struct */
	struct msg	*msg_first;	/* ptr to first message on q */
	struct msg	*msg_last;	/* ptr to last message on q */
	ulong_t		msg_cbytes;	/* current # bytes on q */
	msgqnum_t	msg_qnum;	/* # of messages on q */
	msglen_t	msg_qbytes;	/* max # of bytes on q */
	pid_t		msg_lspid;	/* pid of last msgsnd */
	pid_t		msg_lrpid;	/* pid of last msgrcv */
	time_t		msg_stime;	/* last msgsnd time */
	long		msg_pad1;	/* reserved for time_t expansion */
	time_t		msg_rtime;	/* last msgrcv time */
	long		msg_pad2;	/* time_t expansion */
	time_t		msg_ctime;	/* last change time */
	long		msg_pad3;	/* time_t expansion */
	long		msg_pad4[MSG_PAD];	/* reserve area */
};

#else	/* _STYPES */

/* SVR3 binary compatibility msqid_ds structure - maps to kernel o_msqid_ds */
struct msqid_ds {
	struct ipc_perm	msg_perm;	/* operation permission struct */
	struct msg	*msg_first;	/* ptr to first message on q */
	struct msg	*msg_last;	/* ptr to last message on q */
	ushort_t	msg_cbytes;	/* current # bytes on q */
	ushort_t	msg_qnum;	/* # of messages on q */
	ushort_t	msg_qbytes;	/* max # of bytes on q */
	o_pid_t		msg_lspid;	/* pid of last msgsnd */
	o_pid_t		msg_lrpid;	/* pid of last msgrcv */
	time_t		msg_stime;	/* last msgsnd time */
	time_t		msg_rtime;	/* last msgrcv time */
	time_t		msg_ctime;	/* last change time */
};

#endif	/* _STYPES */


#if defined(_KERNEL) || defined(_KMEMUSER)


/*
 * Since the msqid_ds is a published data structure, it is
 * extended via the kmsqid_ds structure which encapsulates
 * a msqid_ds structure.
 *
 * The kernel manipulates kmsqid_ds structures, the application
 * sees msqid_ds structures.
 */
struct kmsqid_ds {
	struct msqid_ds	kmsq_ds;
	lock_t		kmsq_mutex;	/* per msqid_ds mutex */
	ulong_t		kmsq_flag;	/* per msqid_ds flags, see below */
	ulong_t		kmsq_rcvspec;	/* specific mtype or range on rcv */
	sv_t		kmsq_sv;	/* per msqid_ds synch variable */
	sv_t		kmsq_rcv_sv;	/* blocked in msgrcv(2) */
	sv_t		kmsq_snd_sv;	/* blocked in msgsnd(2) */
};

/* kmsq_flag values: */
#define	MSQID_BUSY	0x1		/* msqid_ds is busy, hands off */

/*
 * Macros to LOCK/UNLOCK a kmsqid_ds structure.
 * 'qp' is a pointer to the kmsqid_ds structure
 * to be locked/unlocked.
 */
#define	MSQID_LOCK(qp)		LOCK_PLMIN(&(qp)->kmsq_mutex);
#define	MSQID_UNLOCK(qp, pl)	UNLOCK_PLMIN(&(qp)->kmsq_mutex, (pl));

/*
 * Macro to convert from the address of the embedded ipc_perm structure
 * to the encapsulating kmsqid_ds structure.
 */
#define	IPC_TO_MSQDS(ipcp)	((struct kmsqid_ds *)(ipcp))

/* SVR3 binary compatibility msqid_ds structure */
struct o_msqid_ds {
	struct o_ipc_perm msg_perm;	/* operation permission struct */
	struct msg	*msg_first;	/* ptr to first message on q */
	struct msg	*msg_last;	/* ptr to last message on q */
	ushort_t	msg_cbytes;	/* current # bytes on q */
	ushort_t	msg_qnum;	/* # of messages on q */
	ushort_t	msg_qbytes;	/* max # of bytes on q */
	o_pid_t		msg_lspid;	/* pid of last msgsnd */
	o_pid_t		msg_lrpid;	/* pid of last msgrcv */
	time_t		msg_stime;	/* last msgsnd time */
	time_t		msg_rtime;	/* last msgrcv time */
	time_t		msg_ctime;	/* last change time */
};

/*
 * There is one msg structure for each message that may be in the system.
 */
struct msg {
	struct msg	*msg_next;	/* ptr to next message on queue */
	long		msg_type;	/* message type */
	void		*msg_vaddr;	/* ptr to msg text (and header) */
	ulong_t		msg_ts;		/* msg text size in bytes */
};

/*
 * Message information structure.
 */
struct msginfo {
	int	msgmap;	/* # of entries in msg map */
	int	msgmax;	/* max message size */
	int	msgmnb;	/* max # bytes on queue */
	int	msgmni;	/* # of message queue identifiers */
	int	msgssz;	/* msg segment size (should be word size multiple) */
	int	msgtql;	/* # of system message headers */
	ushort_t msgseg;	/* # of msg segments (MUST BE < 32768) */
};

struct msgstat {
	long	msgtxt;		/* bytes of message text configured */
	long	msgtxtresv;	/* bytes of message text currently reserved */
	long	msghdresv;	/* # of message headers currently reserved */
	long	msgtxtfail;	/* # of times failed message text reserve */
	long	msghdfail;	/* # of times failed message header reserve */
};

#endif  /* _KERNEL || _KMEMUSER */


/*
 * User message buffer template for msgsnd() and msgrcv() system calls.
 */
#if !defined(_XOPEN_SOURCE)
struct msgbuf {
	long	mtype;		/* message type */
	char	mtext[1];	/* message text */
};
#endif /* !defined(_XOPEN_SOURCE) */


#ifdef _KERNEL

extern int	msgconv(const int, struct kmsqid_ds **, const int);
extern void	msginit(void);

#else /* !_KERNEL */

/* Prototypes for application system call interfaces. */

#ifdef __STDC__

extern int	msgctl(int, int, struct msqid_ds *);
extern int	msgget(key_t, int);
extern int	msgrcv(int, void *, size_t, long, int);
extern int	msgsnd(int, const void *, size_t, int);

#else

extern int	msgctl();
extern int	msgget();
extern int	msgrcv();
extern int	msgsnd();

#endif /* __STDC__ */

#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_MSG_H */
