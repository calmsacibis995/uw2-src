/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_SIGINFO_H	/* wrapper symbol for kernel use */
#define _PROC_SIGINFO_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/siginfo.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif
#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if !defined(_POSIX_SOURCE) 
/*
 * negative signal codes are reserved for future use for user generated
 * signals 
 */

#define SI_FROMUSER(sip)	((sip)->si_code <= 0)
#define SI_FROMKERNEL(sip)	((sip)->si_code > 0)

#define SI_USER		0	/* user generated signal */

/* 
 * SIGILL signal codes 
 */

#define	ILL_ILLOPC	1	/* illegal opcode */
#define	ILL_ILLOPN	2	/* illegal operand */
#define	ILL_ILLADR	3	/* illegal addressing mode */
#define	ILL_ILLTRP	4	/* illegal trap */
#define	ILL_PRVOPC	5	/* privileged opcode */
#define	ILL_PRVREG	6	/* privileged register */
#define	ILL_COPROC	7	/* co-processor */
#define	ILL_BADSTK	8	/* bad stack */
#define NSIGILL		8

/* 
 * SIGFPE signal codes 
 */

#define	FPE_INTDIV	1	/* integer divide by zero */
#define	FPE_INTOVF	2	/* integer overflow */
#define	FPE_FLTDIV	3	/* floating point divide by zero */
#define	FPE_FLTOVF	4	/* floating point overflow */
#define	FPE_FLTUND	5	/* floating point underflow */
#define	FPE_FLTRES	6	/* floating point inexact result */
#define	FPE_FLTINV	7	/* invalid floating point operation */
#define FPE_FLTSUB	8	/* subscript out of range */
#define NSIGFPE		8

/* 
 * SIGSEGV signal codes 
 */

#define	SEGV_MAPERR	1	/* address not mapped to object */
#define	SEGV_ACCERR	2	/* invalid permissions */
#define NSIGSEGV	2

/* 
 * SIGBUS signal codes 
 */

#define	BUS_ADRALN	1	/* invalid address alignment */
#define	BUS_ADRERR	2	/* non-existent physical address */
#define	BUS_OBJERR	3	/* object specific hardware error */
#define NSIGBUS		3

/* 
 * SIGTRAP signal codes 
 */

#define TRAP_BRKPT	1	/* process breakpoint */
#define TRAP_TRACE	2	/* process trace */
#define NSIGTRAP	2

/* 
 * SIGCLD signal codes 
 */

#define	CLD_EXITED	1	/* child has exited */
#define	CLD_KILLED	2	/* child was killed */
#define	CLD_DUMPED	3	/* child has coredumped */
#define	CLD_TRAPPED	4	/* traced child has stopped */
#define	CLD_STOPPED	5	/* child has stopped on signal */
#define	CLD_CONTINUED	6	/* stopped child has continued */
#define NSIGCLD		6

/*
 * SIGPOLL signal codes
 */

#define POLL_IN		1	/* input available */
#define	POLL_OUT	2	/* output buffers available */
#define	POLL_MSG	3	/* output buffers available */
#define	POLL_ERR	4	/* I/O error */
#define	POLL_PRI	5	/* high priority input available */
#define	POLL_HUP	6	/* device disconnected */
#define NSIGPOLL	6

#define SI_MAXSZ	128
#define SI_PAD		((SI_MAXSZ / sizeof (int)) - 3)

typedef struct siginfo {

	int	si_signo;			/* signal from signal.h	*/
	int 	si_code;			/* code from above	*/
	int	si_errno;			/* error from errno.h	*/

	union {

		int	_pad[SI_PAD];		/* for future growth	*/

		struct {			/* kill(), SIGCLD	*/
			pid_t	_pid;		/* process ID		*/
			union {
				struct {
					uid_t	_uid;
				} _kill;
				struct {
					ulong_t	_reserved1;
					int	_status;
				} _cld;
			} _pdata;
		} _proc;			

		struct {	/* SIGSEGV, SIGBUS, SIGILL and SIGFPE	*/
			caddr_t	_addr;		/* faulting address	*/
		} _fault;

		struct {			/* SIGPOLL, SIGXFSZ	*/
		/* fd not currently available for SIGPOLL */
			int	_fd;		/* file descriptor	*/
			long	_band;
		} _file;

	} _data;

} siginfo_t;

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * internal version is identical to siginfo_t but without the padding.
 * This must be maintained in sync with it.
 */

typedef struct k_siginfo {

	int	si_signo;			/* signal from signal.h	*/
	int 	si_code;			/* code from above	*/
	int	si_errno;			/* error from errno.h	*/

	union {
		struct {			/* kill(), SIGCLD	*/
			pid_t	_pid;		/* process ID		*/
			union {
				struct {
					uid_t	_uid;
				} _kill;
				struct {
					ulong_t	_reserved1;
					int	_status;
				} _cld;
			} _pdata;
		} _proc;			

		struct {	/* SIGSEGV, SIGBUS, SIGILL and SIGFPE	*/
			caddr_t	_addr;		/* faulting address	*/
		} _fault;

		struct {			/* SIGPOLL, SIGXFSZ	*/
		/* fd not currently available for SIGPOLL */
			int	_fd;		/* file descriptor	*/
			long	_band;
		} _file;

	} _data;

} k_siginfo_t;

#endif /* _KERNEL || _KMEMUSER */

#define si_pid		_data._proc._pid
#define si_status	_data._proc._pdata._cld._status
#define si_uid		_data._proc._pdata._kill._uid
#define si_addr		_data._fault._addr
#define si_fd		_data._file._fd
#define si_band		_data._file._band

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct sigqueue {
	fspin_t  sq_mutex;		/* spin lock for sq_ref */
	u_char  sq_qlist;		/* set iff sig queued via sq_next */
	/*
	 * If 'sq_qlist' is set, then this signal information object is
	 * queued via the 'sq_next' linkage (in which case, this signal
	 * information is for an LWP instance signal, and the 'p_mutex'
	 * lock of the process containing the LWP locks the 'sq_next' linkage).
	 * Otherwise, this signal information is a reference counted (shared)
	 * signal information object.  In this latter case, the signal
	 * information is for a process instance signal, and 'sq_mutex'
	 * locks 'sq_ref' tracking the number of references to this object.
	 */
	union {
		struct sigqueue *sqv_next;	/* next element in queue */
		u_long sqv_ref;		/* #of references to signal info */
	} sq_v;
#define sq_next sq_v.sqv_next		/* alias for signal info next field */
#define sq_ref  sq_v.sqv_ref		/* alias for signal references field */
	k_siginfo_t sq_info;		/* signal information */
} sigqueue_t;

#endif /* _KERNEL || _KMEMUSER */

#endif /* !defined(_POSIX_SOURCE) */ 

#ifdef _KERNEL

struct proc;				/* allow use for declarations below */
extern void winfo(struct proc *, k_siginfo_t *, boolean_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif
#endif /* _PROC_SIGINFO_H */
