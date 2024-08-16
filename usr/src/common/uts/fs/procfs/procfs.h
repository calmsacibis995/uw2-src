/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_PROCFS_PROCFS_H	/* wrapper symbol for kernel use */
#define _FS_PROCFS_PROCFS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/procfs/procfs.h	1.23"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <svc/time.h>
#include <proc/regset.h>
#include <proc/signal.h>
#include <proc/siginfo.h>
#include <svc/fault.h>
#include <proc/ucontext.h>
#include <svc/syscall.h>
#include <fs/procfs/procfs_f.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/time.h>
#include <sys/regset.h>
#include <sys/signal.h>
#include <sys/siginfo.h>
#include <sys/fault.h>
#include <sys/ucontext.h>
#include <sys/syscall.h>
#include <sys/procfs_f.h>

#else

#include <sys/types.h> /* SVR4.0COMPAT */
#include <sys/time.h> /* SVR4.0COMPAT */
#include <sys/regset.h> /* SVR4.0COMPAT */
#include <sys/signal.h> /* SVR4.0COMPAT */
#include <sys/siginfo.h> /* SVR4.0COMPAT */
#include <sys/ucontext.h> /* SVR4.0COMPAT */
#include <sys/fault.h> /* SVR4.0COMPAT */
#include <sys/syscall.h> /* SVR4.0COMPAT */
#include <sys/procfs_f.h> /* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#define	PRCLSZ		8	/* Maximum size of scheduling class name */
#define	PRSYSARGS	8	/* Maximum number of syscall arguments */

typedef	char	instr_t;	/* Size of an instruction */

/*
 * LWP status file.
 */
typedef struct lwpstatus {
	long	pr_flags;	/* Flags */
	short	pr_why;		/* Reason for stop (if stopped) */
	short	pr_what;	/* More detailed reason */
	lwpid_t	pr_lwpid;	/* Specific LWP identifier */
	short	pr_cursig;	/* Current signal */
	sigset_t pr_lwppend;	/* Set of LWP pending signals */
	siginfo_t pr_info;	/* Info associated with signal or fault */
	stack_t pr_altstack;	/* Alternate signal stack info */
	struct sigaction pr_action;	/* Signal action for current signal */
	ushort_t pr_syscall;	/* System call number (if in syscall) */
	ushort_t pr_nsysarg;	/* Number of arguments to this syscall */
	long	pr_sysarg[PRSYSARGS];	/* Arguments to this syscall */
	char	pr_clname[PRCLSZ];	/* Scheduling class name */
	long	pr_instr;	/* Current instruction */
	ucontext_t pr_context;	/* LWP context */
	pfamily_t pr_family;	/* Processor family-specific information */
	long	pr__pad[8];	/* Room to grow with binary compatibility */
} lwpstatus_t;

/* LWP status flags */

#define	PR_STOPPED	0x0001	/* LWP is stopped */
#define	PR_ISTOP	0x0002	/* LWP stopped on an event of interest */
#define	PR_DSTOP	0x0004	/* A stop directive is in effect */
#define	PR_ASLEEP	0x0008	/* LWP is sleep()ing in a system call */
#define	PR_PCINVAL	0x0080	/* %pc refers to an invalid virtual address */

/*
 * Process status file.
 */
typedef struct pstatus {
	long	pr_flags;	/* Flags */
	ushort_t pr_nlwp;	/* Number of lwps in the process */
	sigset_t pr_sigpend;	/* Set of process pending signals */
	vaddr_t	pr_brkbase;	/* Address of the process heap */
	ulong_t	pr_brksize;	/* Size of the process heap, in bytes */
	vaddr_t	pr_stkbase;	/* Address of the process stack */
	ulong_t	pr_stksize;	/* Size of the process stack, in bytes */
	pid_t	pr_pid;		/* Process id */
	pid_t	pr_ppid;	/* Parent process id */
	pid_t	pr_pgid;	/* Process group id */
	pid_t	pr_sid;		/* Session id */
	timestruc_t pr_utime;	/* Process user cpu time */
	timestruc_t pr_stime;	/* Process system cpu time */
	timestruc_t pr_cutime;	/* Sum of children's user times */
	timestruc_t pr_cstime;	/* Sum of children's system times */
	sigset_t pr_sigtrace;	/* Mask of traced signals */
	fltset_t pr_flttrace;	/* Mask of traced faults */
	sysset_t pr_sysentry;	/* Mask of system calls traced on entry */
	sysset_t pr_sysexit;	/* Mask of system calls traced on exit */
	long pr_res0[2];	/* Reserved */
	lwpstatus_t pr_lwp;	/* "representative" LWP */
} pstatus_t;

/* Process status flags */

#define	PR_FORK		0x0010	/* Inherit-on-fork is in effect */
#define	PR_RLC		0x0020	/* Run-on-last-close is in effect */
#define	PR_PTRACE	0x0040	/* Process is being controlled by ptrace(2) */
#define	PR_ISSYS	0x0100	/* System process */
#define	PR_KLC		0x0200	/* Kill-on-last-close is in effect */
#define	PR_ASYNC	0x0400	/* Asynchronous stop mode is in effect */

/* Reasons for stopping */

#define	PR_REQUESTED	1
#define	PR_SIGNALLED	2
#define	PR_SYSENTRY	3
#define	PR_SYSEXIT	4
#define	PR_JOBCONTROL	5
#define	PR_FAULTED	6
#define	PR_SUSPENDED	7

/*
 * LWP ps(1) information file.
 */
#define	PRFNSZ		16		/* Maximum size of execed filename */

typedef struct lwpsinfo {
	ulong_t	pr_flag;		/* LWP flags */
	lwpid_t	pr_lwpid;		/* LWP id */
	caddr_t	pr_addr;		/* internal address of LWP */
	caddr_t	pr_wchan;		/* wait addr for sleeping LWP */
	uchar_t	pr_stype;		/* synchronization event type */
	uchar_t	pr_state;		/* numeric scheduling state */
	char	pr_sname;	/* printable character representing pr_state */
	uchar_t	pr_nice;		/* nice for cpu usage */
	int	pr_pri;			/*priority, high value = high priority*/
	timestruc_t pr_time;		/* usr+sys cpu time for this LWP */
	char	pr_clname[8];		/* Scheduling class name */
	char	pr_name[PRFNSZ];	/* name of system LWP */
	processorid_t pr_onpro;		/* processor on which LWP is running */
	processorid_t pr_bindpro;	/* processor to which LWP is bound */
	processorid_t pr_exbindpro;	/* processor to which LWP is exbound */
	long	pr__pad[8];	/* Room to grow with binary compatibility */
} lwpsinfo_t;

/*
 * Process ps(1) information file.
 */
#define	PRARGSZ		80		/* Number of chars of arguments */

typedef struct psinfo {
	ulong_t	pr_flag;		/* process flags */
	ulong_t	pr_nlwp;		/* number of LWPs in process */
	uid_t	pr_uid;			/* real user id */
	gid_t	pr_gid;			/* real group id */
	pid_t	pr_pid;			/* unique process id */
	pid_t	pr_ppid;		/* process id of parent */
	pid_t	pr_pgid;		/* pid of process group leader */
	pid_t	pr_sid;			/* session id */
	caddr_t	pr_addr;		/* internal address of process */
	ulong_t	pr_size;		/* size of process image in pages */
	ulong_t	pr_rssize;		/* resident set size in pages */
	timestruc_t pr_start;		/*process start time, time since epoch*/
	timestruc_t pr_time;		/* usr+sys cpu time for this process */
	dev_t	pr_ttydev;		/* controlling tty device (or PRNODEV)*/
	char	pr_fname[PRFNSZ];	/* last component of exec()ed pathname*/
	char	pr_psargs[PRARGSZ];	/* initial characters of arg list */
	struct lwpsinfo pr_lwp;		/* "representative" LWP */
} psinfo_t;

#define	PRNODEV	(dev_t)(-1)	/* non-existent device */

#define	PRMAXOPERAND	124

/*
 * Control messages.
 */
#define	PCSTOP		1	/* Direct process or LWP to stop, and wait */
#define	PCDSTOP		2	/* Direct process or LWP to stop */
#define	PCWSTOP		3	/* Wait for process or LWP to stop */
#define	PCRUN		4	/* Make runnable after a stop */
#define	PCSTRACE	5	/* Set signal-trace mask */
#define	PCSSIG		6	/* Set current signal */
#define	PCKILL		7	/* Send signal to process or LWP */
#define	PCUNKILL	8	/* Delete a signal from pending signal set */
#define	PCSHOLD		9	/* Define the set of held signals */
#define	PCSFAULT	10	/* Define the set of traced hardware faults */
#define	PCCFAULT	11	/* Clear current fault */
#define	PCSENTRY	12	/* Set syscall entry mask */
#define	PCSEXIT		13	/* Set syscall exit mask */
#define	PCSET		14	/* Set modes */
#define	PCRESET		15	/* Reset modes */
#define	PCSREG		16	/* Set general registers */
#define	PCSFPREG	17	/* Set FP registers */
#define	PCNICE		18	/* Set process "nice" value */

/*
 * PCRUN operand flags.
 */
#define	PRCSIG		0x001	/* Clear current signal */
#define	PRCFAULT	0x002	/* Clear current fault */
#define	PRSTEP		0x040	/* Single-step the process */
#define	PRSABORT	0x080	/* Abort syscall */
#define	PRSTOP		0x100	/* Set directed stop request */

/* Memory-management interface */

#define	PRMAPSZ	64
typedef struct prmap {
	vaddr_t	pr_vaddr;	/* Virtual address base */
	ulong_t	pr_size;	/* Size of mapping in bytes */
	char	pr_mapname[PRMAPSZ]; /* Name in /proc/pid/object */
	off_t	pr_off;		/* Offset into mapped object, if any */
	long	pr_mflags;	/* Protection and attribute flags */
	long	pr_filler[9];	/* Filler for future use */
} prmap_t;

/* Protection and attribute flags */

#define	MA_READ		0x04	/* Readable by the traced process */
#define	MA_WRITE	0x02	/* Writable by the traced process */
#define	MA_EXEC		0x01	/* Executable by the traced process */
#define	MA_SHARED	0x08	/* Changes are shared by mapped object */
#define	MA_BREAK	0x10	/* Grown by brk(2) */
#define	MA_STACK	0x20	/* Grown automatically on stack faults */

/* Process credentials */

typedef struct prcred {
	uid_t	pr_euid;	/* Effective user id */
	uid_t	pr_ruid;	/* Real user id */
	uid_t	pr_suid;	/* Saved user id (from exec) */
	gid_t	pr_egid;	/* Effective group id */
	gid_t	pr_rgid;	/* Real group id */
	gid_t	pr_sgid;	/* Saved group id (from exec) */
	uint_t	pr_ngroups;	/* Number of supplementary groups */
	gid_t	pr_groups[1];	/* Array of supplementary groups */
} prcred_t;

/*
 * Macros for manipulating sets of flags.
 * sp must be a pointer to one of sigset_t, fltset_t, or sysset_t.
 * flag must be a member of the enumeration corresponding to *sp.
 */

/* turn on all flags in set */
#define	prfillset(sp) \
	{ register int _i_ = sizeof(*(sp))/sizeof(ulong_t); \
		while(_i_) ((ulong_t*)(sp))[--_i_] = 0xFFFFFFFF; }

/* turn off all flags in set */
#define	premptyset(sp) \
	{ register int _i_ = sizeof(*(sp))/sizeof(ulong_t); \
		while(_i_) ((ulong_t*)(sp))[--_i_] = 0L; }

/* turn on specified flag in set */
#define	praddset(sp, flag) \
	(((unsigned)((flag)-1) < 32*sizeof(*(sp))/sizeof(ulong_t)) ? \
	(((ulong_t*)(sp))[((flag)-1)/32] |= (1L<<(((flag)-1)%32))) : 0)

/* turn off specified flag in set */
#define	prdelset(sp, flag) \
	(((unsigned)((flag)-1) < 32*sizeof(*(sp))/sizeof(ulong_t)) ? \
	(((ulong_t*)(sp))[((flag)-1)/32] &= ~(1L<<(((flag)-1)%32))) : 0)

/* query: != 0 iff flag is turned on in set */
#define	prismember(sp, flag) \
	(((unsigned)((flag)-1) < 32*sizeof(*(sp))/sizeof(ulong_t)) \
	&& (((ulong_t*)(sp))[((flag)-1)/32] & (1L<<(((flag)-1)%32))))


#if defined(__cplusplus)
        }
#endif
#endif	/* _FS_PROCFS_PROCFS_H */
