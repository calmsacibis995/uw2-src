/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_TYPES_H	/* wrapper symbol for kernel use */
#define _UTIL_TYPES_H	/* subject to change without notice */
#define _SYS_TYPES_H	/* SVR4.0COMPAT */

#ident	"@(#)kern-i386:util/types.h	1.13"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Definitions of all the basic system types.
 */

/* POSIX Extensions */

typedef	unsigned char	uchar_t;

#ifndef _USHORT_T
#define _USHORT_T
typedef	unsigned short	ushort_t;
#endif

typedef	unsigned int	uint_t;

#ifndef _ULONG_T
#define _ULONG_T
typedef	unsigned long	ulong_t;
#endif


typedef char *	addr_t;			/* ?<core address> type (BSD compat) */
typedef	char *	caddr_t;		/* ?<core address> type */
typedef	long	daddr_t;		/* <disk address> type */
typedef char *	faddr_t;		/* XENIX compat */

#ifndef	_WCHAR_T
#define	_WCHAR_T
typedef long	wchar_t;		/* wide character type */
#endif

#ifndef _OFF_T
#define _OFF_T
typedef	long	off_t;			/* ?<offset> type */
#endif

typedef	short	cnt_t;			/* ?<count> type */
typedef	ulong_t	vaddr_t;		/* <virtual address> type */
typedef	ulong_t	paddr_t;		/* <physical address> type */
typedef uint_t	ppid_t;			/* <physical page id> type */
typedef	uchar_t	use_t;			/* use count for swap */
typedef	short	sysid_t;		/* system ID (RFS) */
typedef	short	index_t;		/* bitmap index (no longer used) */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
typedef enum boolean { B_FALSE, B_TRUE } boolean_t;
typedef	struct _label { int val[6]; } label_t;
typedef struct _quad { long val[2]; } quad;	/* used by UFS */
#define quad_low(x)	x.val[0]
#endif

/*
 * The following type is for various kinds of identifiers.  The
 * actual type must be the same for all since some system calls
 * (such as sigsend) take arguments that may be any of these
 * types.  The enumeration type idtype_t defined in procset.h
 * is used to indicate what type of id is being specified.
 */

typedef long	id_t;		/* A process id,	*/
				/* process group id,	*/
				/* session id, 		*/
				/* scheduling class id,	*/
				/* user id, or group id.*/

/* The type used to specify an LWP ID at user level */
typedef	long	lwpid_t;

/* A processor name */
typedef	int	processorid_t;

/* The type returned by the timeout interfaces */
typedef	int	toid_t;

/* Typedef for kernel privilege mechanism */

typedef ulong_t	pvec_t;		/* kernel privilege vector */

/* Typedefs for Mandatory Access Controls (MAC) */

typedef ulong_t	lid_t;		/* internal representation of security level */

typedef lid_t	level_t;	/* user view of security level */
				/* (currently the same as the internal view) */

/* Typedefs for dev_t components */

typedef ulong_t	major_t;	/* major part of device number */
typedef ulong_t	minor_t;	/* minor part of device number */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)

/* Typedef for AUDIT event mask */
#define ADT_EMASKSIZE	8	/* size(bytes) of auditable event mask */
typedef unsigned long  adtemask_t[ADT_EMASKSIZE]; /* audit event mask type */

#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */

/*
 * For compatibility reasons the following typedefs (prefixed o_)
 * can't grow regardless of the EFT definition.  Although
 * applications should not explicitly use these typedefs, they
 * may be included via a system header definition.  For example,
 * the definitions in s5inode.h use these old types in order to
 * preserve compatibility with existing S5 filesystem images.
 *
 * WARNING: These typedefs may be removed in a future release.
 */
typedef	ushort_t o_mode_t;		/* old file attribute type */
typedef short	 o_dev_t;		/* old device type	*/
typedef	ushort_t o_uid_t;		/* old UID type		*/
typedef	o_uid_t	 o_gid_t;		/* old GID type		*/
typedef	short	 o_nlink_t;		/* old file link type	*/
typedef short	 o_pid_t;		/* old process id type	*/
typedef ushort_t o_ino_t;		/* old inode type	*/


/* POSIX and XOPEN Declarations */

#ifndef _KEY_T
#define _KEY_T
typedef	int	key_t;			/* IPC key type		*/
#endif

#ifndef _MODE_T
#define _MODE_T
typedef	ulong_t	mode_t;			/* file attribute type	*/
#endif

#ifndef _UID_T
#define _UID_T
typedef	long	uid_t;			/* UID type		*/
#endif

#ifndef _GID_T
#define _GID_T
typedef	uid_t	gid_t;			/* GID type		*/
#endif

typedef	ulong_t nlink_t;		/* file link type	*/
typedef ulong_t	dev_t;			/* expanded device type */
typedef ulong_t	ino_t;			/* expanded inode type	*/

#ifndef _PID_T
#define _PID_T
typedef long	pid_t;			/* process id type	*/
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef	uint_t	size_t;		/* len param for string funcs */
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef	int	ssize_t;	/* return byte count or indicate error */
#endif

#ifndef _TIME_T
#define _TIME_T
typedef	long	time_t;		/* time of day in seconds */
#endif

#ifndef _CLOCK_T
#define _CLOCK_T
typedef	long	clock_t;	/* relative time in a specified resolution */
#endif


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Define the internal kernel LWP ID type.
 * We define this as ushort to save space in the kernel (e.g, in the proc
 * structure and in the kernel synchronization objects, etc.), but still
 * allow USHRT_MAX LWPs per process.  The external lwpid_t type is "long"
 * however.
 */
typedef	ushort_t k_lwpid_t;

/*
 * The states an LWP may be in.
 */
typedef	enum {SONPROC, SRUN, SSLEEP, SSTOP, SIDL} lwpstat_t;

/*
 * The types of synchronization objects an LWP may block against.
 */
typedef	enum {ST_NONE, ST_COND, ST_EVENT, ST_RDLOCK, ST_WRLOCK,
	      ST_SLPLOCK, ST_USYNC} stype_t;

#endif /* _KERNEL || _KMEMUSER */


#if defined(_KERNEL) || ( !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) )

typedef	struct { int r[1]; } *	physadr;
typedef	unsigned char	unchar;
typedef	unsigned short	ushort;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;

#if defined(_KERNEL)

#define SHRT_MIN	-32768		/* min value of a "short int" */
#define SHRT_MAX	32767		/* max value of a "short int" */
#define USHRT_MAX	65535U		/* max value of "unsigned short int" */
#define INT_MIN		(-2147483647-1) /* min value of an "int" */
#define INT_MAX		2147483647	/* max value of an "int" */
#define UINT_MAX	4294967295U	/* max value of an "unsigned int" */
#define LONG_MIN	(-2147483647-1) /* min value of a "long int" */
#define LONG_MAX	2147483647	/* max value of a "long int" */
#define ULONG_MAX	4294967295U	/* max value of "unsigned long int" */
#define OFF_MAX		LONG_MAX	/* off_t is type long  */
#define CLOCK_MAX	LONG_MAX	/* max value of "clock_t" */

#ifndef O_NODEV
#define O_NODEV (o_dev_t)(-1)
#endif

#endif /* _KERNEL */


#define	P_MYPID	((pid_t)0)

/*
 * The following is the value of type id_t to use to indicate the
 * caller's current id.  See procset.h for the type idtype_t
 * which defines which kind of id is being specified.
 */

#define	P_MYID	(-1)
#define NOPID (pid_t)(-1)

#ifndef NODEV
#define NODEV (dev_t)(-1)
#endif

/*
 * A host identifier is used to uniquely define a particular node
 * on an rfs network.  Its type is as follows.
 */

typedef	long	hostid_t;

/*
 * The following value of type hostid_t is used to indicate the
 * current host.  The actual hostid for each host is in the
 * kernel global variable rfs_hostid.
 */

#define	P_MYHOSTID	(-1)

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;

typedef	unsigned int	pl_t;		/* priority level */

/*
 * Define the internal kernel priority level type as a uchar_t
 * in order to enable certain optimizations.
 * The external type is still an unsigned int.
 */
typedef	uchar_t		k_pl_t;

/*
 * Nested include for BSD/sockets source compatibility.
 * (The select macros used to be defined here).
 */
#ifdef _KERNEL_HEADERS

#include <fs/select.h>	/* SVR4.0COMPAT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/select.h>	/* SVR4.0COMPAT */

#else

#include <sys/select.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#endif /* _KERNEL || (!_POSIX_SOURCE && !_XOPEN_SOURCE) */

/*
 * Aliases to allow non-ANSI compilers to compile the system.
 */

#ifdef __STDC__

#ifndef _VOID
#define _VOID	void
#endif

	/* End of ANSI section */

#else

#ifndef _VOID
#define _VOID	char
#endif

#ifndef const
#define const
#endif

#ifndef volatile
#define volatile
#endif

#endif /* of non-ANSI section */

#if defined(__cplusplus)
        }
#endif
#endif /* _UTIL_TYPES_H */
