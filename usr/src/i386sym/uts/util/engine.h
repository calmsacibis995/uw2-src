/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UTIL_ENGINE_H	/* wrapper symbol for kernel use */
#define	_UTIL_ENGINE_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:util/engine.h	1.25"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <svc/clock.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/clock.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Per-processor basic "engine" structure.  Fundamental representation
 * of a processor for dispatching and initialization.
 * Allocated per-processor at boot-time in an array.
 * Base address stored in kernel variable "engine".
 */

typedef struct	engine {
	int		e_slicaddr;	/* the processor's SLIC address	*/
	int		e_diag_flag;	/* copy of power-up diagnostic flags */
	int		e_cpu_speed;	/* cpu rate in MHz              */
	volatile int	e_flags;	/* processor flags - see below	*/
	struct ppriv_pages *e_local;	/* virtual address of local stuff */
	volatile int	e_pri;		/* current priority engine is running */
	volatile int	e_npri;		/* priority engine was nudged at */
	int	e_count;		/* count of lwp's user-bound to engine */
	int     e_nsets;                /* number of cache sets */
	int     e_setsize;              /* cache set size in kbytes */
	struct callout *e_local_todo;	/* list of pending local callouts */
#ifndef UNIPROC
	struct runque **e_lastpick;	/* last run queue we scheduled from */
#define	MAXRQS	128
	struct runque *e_rqlist[MAXRQS+1];/* run queues this engine is scheduling
					   from */
#else
	struct runque *e_rqlist;        /* run queue this engine is scheduling
					 * from */
#endif
	timestruc_t e_smodtime;		/* last time processor was turned
					 * online/offline */
} engine_t;

/* currently defined flag bits */
#define	E_OFFLINE	0x01		/* processor is off-line	*/
#define E_BAD		0x02		/* processor is bad		*/
#define	E_SHUTDOWN	0x04		/* shutdown has been requested	*/ 
#define E_DRIVER	0x08		/* processor has driver bound	*/
#define E_PAUSED	0x10		/* processor paused - see panic */
#define	E_FPU387	0x20		/* 1==387, 0==287 (i386 only) */
#define	E_FPA		0x40		/* processor has an FPA (i386 only) */
#define	E_SGS2		0x80		/* processor is SGS2 (i.e. scan based) */
#define	E_DEFAULTKEEP	0x100		/* keep processor in the default set */
#define	E_DRIVERBOUND	0x200		/* uniprocessor driver bound to engine */
#define	E_EXCLUSIVE	0x400		/* engine is exclusively bound */
#define E_NOWAY		(E_OFFLINE|E_BAD|E_SHUTDOWN|E_PAUSED)

/* defined for state field */
#define	E_BOUND		0x01		/* processor is running bound lwp */
#define	E_GLOBAL	0x00		/* processor not running bound lwp */

/* Cannot switch lwp to Engine - see runme */
#define E_UNAVAIL	-1

#ifdef _KERNEL
extern	int online_engine(int);		/* online an engine */
extern	int offline_engine(int);	/* offline an engine */
extern boolean_t engine_disable_offline(int engno);
					/* disable engine offline */

extern	struct engine	*engine;	/* Engine Array Base */
extern	struct engine	*engine_Nengine;/* just past Engine Array Base */
extern	uint_t		myengnum;	/* This engine's number (per-engine) */
extern	int 		Nengine;	/* # Engines to alloc at boot */
extern	int		nonline;	/* count of online engines */
extern	event_t eng_wait;		/* wait on this during online/offline */
extern	lock_t eng_tbl_mutex;		/* held when modifying the engine table */
extern	lkinfo_t eng_tbl_lkinfo;	/* information about the engine */
extern	fspin_t eng_count_mutex;	/* mutex all "e_count" fields */

/*
 * Map a processor-id to an engine pointer.
 */
#define	PROCESSOR_MAP(id)	(((id) < 0 || (id) >= Nengine)? \
					NULL : engine + (id))
#define PROCESSOR_UNMAP(e)	((e) - engine)
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_ENGINE_H */
