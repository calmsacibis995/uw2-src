/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_KDB_XDEBUG_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_XDEBUG_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kdb/xdebug.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * xdebug.h - kernel debugging macros
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h> /* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * Reasons for Entering Debugger
 */
#define DR_USER		0	/* User request via key sequence */
#define DR_BPT1		1	/* Int 1 breakpoint trap */
#define DR_STEP		2	/* (Int 1) single-step trap */
#define DR_BPT3		3	/* Int 3 (breakpoint) trap */
#define DR_PANIC	4	/* Panic occurred */
#define DR_OTHER	5	/* Miscellaneous */
#define DR_INIT		6	/* At init time; apply debugger_init string */
#define DR_SECURE_USER	7	/* User request via sysi86 (privileged) */
#define DR_SLAVE	8	/* Slave mode; only used in MP mode */
#define DR_ONLINE	9	/* MP only: engine has just come online */
#define DR_RELEASE_DBREGS 10	/* Release use of debug registers to user */
#define DR_UNLOAD	11	/* Debugger is being unloaded */

#define NO_FRAME	((int *)0)

#ifndef NODEBUGGER

extern void (* volatile cdebugger)(), nullsys();
extern uint_t user_debug_count;
extern fspin_t debug_count_mutex;

extern char bugbits[];

#define debugentry(r0ptr)	((*cdebugger)(DR_OTHER, r0ptr))
#define calldebug()		debugentry(NO_FRAME)
#define console_debugentry(consswp, minor)	\
		((conschan.cnc_consswp == (consswp) && \
		  conschan.cnc_minor == (minor)) ? \
			((*cdebugger)(DR_USER, NO_FRAME), B_TRUE) : \
			B_FALSE)

#else

#define debugentry(r0ptr)			/**/
#define calldebug()				/**/
#define console_debugentry(consswp, minor)	/**/

#endif

#ifdef MSDEBUGGER
# ifndef MSDEBUG
#  define MSDEBUG
# endif
# define debugger(class) if ( debug(class, DL_HALT) ) calldebug();
#else
# define debugger(class)
#endif

#ifdef MSDEBUG
# define debug(class, level) (bugbits[class] & level)
# define dentry(class, name) if (debug(class,DL_CALL)) printf(entryfmt,name);
# define dexit(class, name) if (debug(class,DL_CALL)) printf(exitfmt, name);
  extern char entryfmt[];
  extern char exitfmt[];
#else
# define debug(class, level)	0
# define dentry(class, name) 
# define dexit(class, name)
#endif

/*
 * Debugging classes
 */
#define DB_EXEC		0	/* Exec system call */
#define DB_FP		1	/* Floating Point */
#define DB_MAIN		2	/* System initialization */
#define DB_MALLOC	3	/* Memory allocation */
#define DB_MMU		4	/* memory management */
#define DB_PHYSIO	5	/* raw I/O */
#define DB_SIG		6	/* Signals */
#define DB_SLP		7	/* Process switching */
#define DB_TEXT		8	/* Text table management */
#define DB_TRAP		9	/* Traps and faults */
#define DB_PFAULT	10
#define DB_PGOUT	11
#define DB_SCALL	12
#define DB_PHASH	13	/* filesystem page cache */
#define DB_FORK		14	/* copy on write fork */
#define DB_SWAP		15	/* swapping */
#define DB_IPC		16	/* ipc msgs, sems, shm */
#define DB_CONSOLE	17	/* console */
#define	DB_DISK		18	/* disk driver */
#define DB_SYNC		19	/* page synchronization */
#define DB_SWTCH	20	/* context switching */
#define	DB_DMA		21	/* dma */
#define NDBC		22	/* number of debugging classes */

/* 
 * Debugging levels
 */
#define DL_TERSE	1	/* terse output */
#define DL_VERB		2	/* verbose output */
#define DL_HALT		4	/* call debug() at strategic points */
#define DL_CALL		8	/* function entry/exit trace */

/*
 * Manifest constants for debugger
 */
#define NAR	19		/* can't match anything in reg.h */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KDB_XDEBUG_H */
