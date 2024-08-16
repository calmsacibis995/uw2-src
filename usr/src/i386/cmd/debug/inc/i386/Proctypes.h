/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Proctypes_h
#define _Proctypes_h

#ident	"@(#)debugger:inc/i386/Proctypes.h	1.1"

#include <sys/types.h>
#include <signal.h>
#include <sys/regset.h>
#include <sys/procfs.h>
#include <sys/fault.h>

/* NOTE: uses C style comments so it may be included by C code */

/* System and machine specific definitions for access to
 * the process control mechanism.
 */

#define BKPTSIG			SIGTRAP
#define BKPTFAULT		FLTBPT

#ifdef OLD_PROC
#define TRACEFAULT		FLTBPT
#else
#define TRACEFAULT		FLTTRACE
#endif

#ifdef PTRACE
#define STOP_TRACE		BKPTSIG
#define STOP_BKPT		BKPTSIG
#define STOP_TYPE		PR_SIGNALLED
#else
#define STOP_TRACE		TRACEFAULT
#define STOP_BKPT		BKPTFAULT
#define STOP_TYPE		PR_FAULTED
#endif

#ifdef OLD_PROC
typedef prstatus_t	pstatus_t;
typedef prpsinfo_t	psinfo_t;
#endif

/* special signals that shouldn't be ignored */
#if PTRACE
#define SIG_SPECIAL(I) ((I) == BKPTSIG)
#else
#define SIG_SPECIAL(I) (0)
#endif

/* structure definitions for functions that write sets of data:
 * gregs, fpregs, dbregs, sigset, sysset and fltset.  These
 * structures have room at the beginning for a control word,
 * to make access to the new /proc more efficient.  The control
 * word is unused for ptrace or old /proc
 */

typedef prmap_t map_ctl;

struct	greg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	gregset_t	gregs;
};

struct	fpreg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	fpregset_t	fpregs;
};

struct	sys_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	sysset_t	scalls;
};

struct	sig_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	sigset_t	signals;
};

struct	flt_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	fltset_t	faults;
};

struct	dbreg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	dbregset_t	dbregs;
};

/* Local signal set manipulation routines 
 * assume sigset_t is declared as :
 * typedef struct {
 *  unsigned long sa_sigbits[4];
 *  }; sigset_t
 *  we only read as many words as are necessary for number of signals
 * currently implemented
 *
 * the routines prfillset, premptyset, prismember are declared
 * in procfs.h
 */

/* result = union(set1, set2) */
#define  mysigsetcombine(set1, set2, result) \
{\
	for (int _i_ = 0; _i_ <= ((MAXSIG-1)/WORD_BIT); _i_++)\
	{ \
		(result)->sa_sigbits[_i_] = (set1)->sa_sigbits[_i_] | \
		(set2)->sa_sigbits[_i_];\
	}\
}


#if MAXSIG <= WORD_BIT
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0)

#elif MAXSIG <= 2 * WORD_BIT
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0 && \
	(set)->sa_sigbits[1] == 0)

#elif MAXSIG <= 3 * WORD_BIT
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0 && \
	(set)->sa_sigbits[1] == 0 && (set)->sa_sigbits[2] == 0)

#else
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0 && \
	(set)->sa_sigbits[1] == 0 && (set)->sa_sigbits[2] == 0 && \
	(set)->sa_sigbits[3] == 0)

#endif

#endif
