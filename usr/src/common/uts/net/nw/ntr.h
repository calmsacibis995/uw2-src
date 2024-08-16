/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_NTR_H  /* wrapper symbol for kernel use */
#define _NET_NW_NTR_H  /* subject to change without notice */

#ident	"@(#)kern:net/nw/ntr.h	1.11"
#ident	"$Id: ntr.h,v 1.22 1994/11/29 03:58:05 stevbam Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 */


#if defined(DEBUG_TRACE)

/*
**	Tracing under UnixWare
*/
#ifdef _KERNEL_HEADERS
#include <util/nuc_tools/trace/nwctrace.h>
#else
#include <sys/nwctrace.h>
#endif

#define NTR_TRACING 1

#define	NTRT_printf		NVLTT_printf
#define	NTRT_Strlog		NVLTT_strlog
#define	NTRT_String		NVLTT_String
#define	NTRT_Return		NVLTT_Return
#define	NTRT_Enter		NVLTT_Enter
#define	NTRT_Leave		NVLTT_Leave

/*	NWU Masks */
#define	NTRM_test		NVLTM_NWU_test		/* TEST			*/
#define	NTRM_lipmx		NVLTM_NWU_lipmx		/* lower ipx mux */
#define	NTRM_ipx		NVLTM_NWU_ipx		/* upper ipx mux */
#define	NTRM_ripx		NVLTM_NWU_ripx		/* ipx router	 */
#define	NTRM_ncpipx		NVLTM_NWU_ncpipx	/* ncpipx		*/
#define	NTRM_nwetc		NVLTM_NWU_nwetc		/* the do all driver */
#define	NTRM_nemux		NVLTM_NWU_nemux		/* nemux		*/
#define	NTRM_spx		NVLTM_NWU_spx		/* spx stack	*/
#define	NTRM_elap		NVLTM_NWU_elap		/* appletalk elap */
#define	NTRM_ddp		NVLTM_NWU_ddp		/* appletalk ddp  */
#define	NTRM_atp		NVLTM_NWU_atp		/* appletalk atp  */
#define	NTRM_pap		NVLTM_NWU_pap		/* appletalk pap  */
#define	NTRM_asp		NVLTM_NWU_asp		/* appletalk asp  */
#define	NTRM_nbio		NVLTM_NWU_nbio		/* NetBIOS connection protocol */
#define	NTRM_nbdg		NVLTM_NWU_nbdg		/* NetBIOS datagram protocol */
#define	NTRM_nbix		NVLTM_NWU_nbix		/* NetBIOS ipx shim */
#define	NTRM_nxfs		NVLTM_NWU_nxfs		/* NetWare eXtended FS*/
#define	NTRM_sfd		NVLTM_NWU_sfd		/* Shared file descriptor */

/*	NUC Masks */
#define NTRM_gipc		NVLTM_gipc
#define NTRM_ipxeng		NVLTM_ipxeng
#define NTRM_ncp		NVLTM_ncp
#define NTRM_nuc		NVLTM_nuc
#define NTRM_nwmp		NVLTM_nwmp
#define NTRM_spil		NVLTM_spil
#define NTRM_tool		NVLTM_tool

#define NTR_ENTER(argc,arg1,arg2,arg3,arg4,arg5) NVLT_ENTER(argc)
#define NTR_EXT_CALL(who)		NVLT_EXT_CALL(who)
#define NTR_EXT_RETURN(who)		NVLT_EXT_RETURN(who)
#define NTR_LEAVE(retcode)		NVLT_LEAVE(retcode)
#define NTR_VLEAVE()			NVLT_LEAVE(0)
#define NTR_TRACE(t,v1,v2,v3,v4) NVLT_TRACE(t,v1,v2,v3,v4)
#define	NTR_TRACE_OFF()			NVLT_TRACE_OFF()
#define	NTR_TRACE_ON()			NVLT_TRACE_ON()
#define	NTR_NWSLOG_OFF()		NVLT_STRLOG_OFF()
#define	NTR_NWSLOG_ON()			NVLT_STRLOG_ON()

/* Start and Stop the stopwatch		*/
#define	NTR_START_WATCH(s,u)	NVLT_START_WATCH(s,u)
#define	NTR_STOP_WATCH(s,u) 	NVLT_STOP_WATCH(s,u)
#define	NTR_START_UNIQUE(s)  	NVLT_START_UNIQUE(s)
#define NTR_STRING(s)			NVLT_STRING(s)

#define NTR_REGISTER_CALLOUT(mask,function) NVLT_REGISTER_CALLOUT(mask,function)
#define NTR_PRINTF(fmt,v1,v2,v3)   NVLT_PRINTF(fmt,v1,v2,v3)

#define NVLT_ModMask NTR_ModMask

#define NTR_ASSERT(EX) ((void)((EX) || NVLTassfail(NVLT_ModMask, #EX,	    \
										__FILE__, __LINE__)))

#else /* !defined(DEBUG_TRACE) */

/*
**	Tracing under NWU Portable trace driver
*/

#ifdef _KERNEL_HEADERS
#include <svc/time.h>
#include <net/nw/nwcommon.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/time.h>
#ifdef _KERNEL
#include "nwcommon.h"
#endif
#endif /* _KERNEL_HEADERS */

#if defined(NTR_TRACING) || defined(NTRDSP)

/*
 *	Number of times to loop to get average time for a trace entry
 */
#define	NTR_TIMING_LOOPS	10000	/* Number of trace entries for timing */
#define	NTR_ENTRIES			8192	/* Number of trace table entries	*/

/*
 *	Format of a trace table entry.
 */
typedef struct {
	nwkTime_t		time;			/* time structure tv_sec, tv_nsec */
	unsigned short	proc;			/* interrupted process id num	*/
			 short	intr;			/* interrupt level				*/
	unsigned int	type;			/* trace event type				*/
	unsigned int	v1,				/* caller's values				*/
					v2,
					v3,
					v4;
} trace_t;

/*
**	Mask definitions
*/

#define	NTR_Mask_Mask	0xffffff00
#define NTR_Type_Mask	0xff

/*
 *  Macros to isolate various event members in event x.
 */
#define TR_TYPE(x)      ((x) & NTR_Type_Mask)  /* isolate type   */
#define TR_MASK_BITS(x) ((x) & NTR_Mask_Mask)  /* mask bits only */

/* mask masks */
/*	If you change any of these names, you must also change
**	the list in kernel/ntr/ntrdsp/ntrstring.c
*/

#define	NTRM_test		0x00000100		/* TEST			*/
#define	NTRM_lipmx		0x00000200		/* lower ipx mux */
#define	NTRM_ipx		0x00000400		/* upper ipx mux */
#define	NTRM_ripx		0x00000800		/* ipx router	 */
#define	NTRM_spx		0x00001000		/* spx stack	*/
#define	NTRM_ncpipx		0x00002000		/* ncpipx		*/
#define	NTRM_nwetc		0x00004000		/* the do all driver */
#define	NTRM_nemux		0x00008000		/* nemux		*/
#define	NTRM_elap		0x00010000		/* appletalk elap */
#define	NTRM_ddp		0x00020000		/* appletalk ddp  */
#define	NTRM_atp		0x00040000		/* appletalk atp  */
#define	NTRM_pap		0x00080000		/* appletalk pap  */
#define	NTRM_asp		0x00100000		/* appletalk asp  */
#define	NTRM_nbio		0x00200000		/* NetBIOS connection protocol */
#define	NTRM_nbdg		0x00200000		/* NetBIOS datagram protocol */
#define	NTRM_nbix		0x00200000		/* NetBIOS ipx shim */
#define	NTRM_nxfs		0x00400000		/* NetWare eXtended FS*/
#define	NTRM_sfd		0x00800000		/* Shared file descriptor */
#define	NTRM_ipxeng		0x01000000		/* NUC */
#define	NTRM_ncp		0x02000000		/* NUC */
#define	NTRM_spil		0x04000000		/* NUC */
#define	NTRM_tool		0x08000000		/* NUC */
#define	NTRM_nuc		0x08000000		/* NUC */
#define	NTRM_nwmp		0x08000000		/* NUC */
#define	NTRM_gipc		0x08000000		/* NUC */

#define NVLTM_ipxeng	NTRM_ipxeng
#define NVLTM_ncp		NTRM_ncp
#define NVLTM_gipc		NTRM_gipc
#define NVLTM_nuc		NTRM_nuc
#define NVLTM_tool		NTRM_tool
#define NVLTM_spil		NTRM_spil
#define NVLTM_nwmp		NTRM_nwmp

/*	Special masks: Don't mess with 'em	*/
#define	NTRM_UNUSEDS1	0x10000000
#define	NTRM_UNUSEDS2	0x20000000
#define NTRM_swatch		0x40000000		/* StopWatch start+stop			*/
#define NTRM_ext		0x80000000		/* External Calls and Returns	*/

/* mask types */

/*
 *	NOTE:	The following codes are reserved for
 *			the generic Enter and Leave
 *			and other such events!
 */

#define	NTRT_Strlog		0xfb
#define	NTRT_String		0xfc
#define	NTRT_Return		0xfd
#define	NTRT_Enter		0xfe
#define	NTRT_Leave		0xff

/* ipx stack */
#define NTRT_ipxTopOfDo			(NTRM_ipx | 1)
#define NTRT_ipxEndOfDo			(NTRM_ipx | 2)

/*
 *	Stopwatch assignments, actually an index into
 *	the stopwatch_strings array found in ndtstring.c
 */
#define	NTR_SW_UNUSED1				1
#define	NTR_SW_UNUSED2				2
#define	NTR_SW_UNUSED3				3
#define	NTR_SW_UNUSED4				4
#define NTR_SW_UNUSED5				5
#define NTR_SW_ON_Q					6	/* Time spent on a streams queue	*/
#define NTR_SW_ReadMsg				7	/* lifetime of a read message		*/
#define NTR_SW_WriteMsg				8	/* lifetime of a write message		*/

/*
 *	Structure returned on an NTRioctl_Info call.
 */
typedef struct {
								/* NOTE:  These are all pointers into kmem	*/
	trace_t *start,				/* First trace table entry					*/
			*next,				/* Where the next entry will go				*/
		   **nextp,				/* Pointer to next entry kernel pointer		*/
			*end;				/* Last entry +1							*/

	int		timingLoopCount,
			timingNanoSeconds;
} NTR_Info;

typedef char memData[1024];

typedef struct {
	int		region;				/* memory region				*/
	int		count;				/* number of allocated chunks	*/
	int		size;				/* size of allocated chunks		*/
} memRegion_t;

#define	NREGIONS	16
typedef memRegion_t	memRegionSummary_t[NREGIONS];	/* array of region structs */

/*
 *	ioctl "cmd" values.
 */
#define	NTR_GET_INFO	0xD0ab1e1
#define	NTR_GET_MASK	0xD0ab1e2
#define	NTR_SET_MASK	0xD0ab1e3
#define	NTR_GET_NWSLOG	0xD0ab1e4
#define	NTR_SET_NWSLOG	0xD0ab1e5
#define	NTR_RESET		0xD0ab1e6

/* Generate generic Enter and Leave Events	*/

#define NTR_Gen(mask)	{ ((mask)|NTRT_Enter),"             Enter: %s( 0x%x, 0x%x) called by 0x%x" },\
						{ ((mask)|NTRT_Leave),"             Leave: %s" }

#endif /*  defined(NTR_TRACING) || defined(NTRDSP) */

#ifdef NTR_TRACING
		void NTRenter( unsigned int, long, long, long, long, long, long,
						unsigned int);
		void NTRcall( unsigned int, unsigned int);
		void NTRtrace( unsigned int, long, long, long, long);
		 int NTRleave( unsigned int, int, unsigned int);
		void NTRvleave( unsigned int, unsigned int);
unsigned int NTRtrace_off( void);
		void NTRtrace_on( unsigned int );
		void NTRreset( void);
		void NTRstrlog(unsigned long, short, char, unsigned, char	*, ...);
unsigned int NTRstrlog_off( void);
		void NTRstrlog_on( unsigned int );
		void NTRstring( unsigned int, char *);
unsigned int NTRuniqueWatch( unsigned int);
unsigned int NTRgetcaller( void);
unsigned int NTRgetaddress( void);

#define NTR_ENTER(argc,arg1,arg2,arg3,arg4,arg5) \
								NTRenter((NTR_ModMask|NTRT_Enter), (argc), \
								(long)(arg1), (long)(arg2), (long)(arg3), \
								(long)(arg4), (int)(arg5), \
								NTRgetcaller())
#define NTR_EXT_CALL(who)		NTRcall( (NTRM_ext|NTRT_Enter), (int)(who))
#define NTR_EXT_RETURN(who)		NTRtrace((NTRM_ext|NTRT_Return),0,0,(int)(who),0)
#define NTR_LEAVE(retcode)		NTRleave((NTR_ModMask|NTRT_Leave), \
								(int)(retcode), NTRgetaddress())
#define NTR_VLEAVE()			NTRvleave((NTR_ModMask|NTRT_Leave), \
								NTRgetaddress())
#define NTR_TRACE(t,v1,v2,v3,v4) NTRtrace((t),(v1),(v2),(v3),(v4))
#define	NTR_TRACE_OFF()			NTRtrace_off()
#define	NTR_TRACE_ON()			NTRtrace_on()
#define	NTR_NWSLOG_OFF()		NTRstrlog_off()
#define	NTR_NWSLOG_ON()			NTRstrlog_on()

/* Start and Stop the stopwatch		*/
#define	NTR_START_WATCH(s,u)	NTRtrace((NTRM_swatch|NTRT_Enter),0,0,(s),(u))
#define	NTR_STOP_WATCH(s,u) 	NTRtrace((NTRM_swatch|NTRT_Return),0,0,(s),(u))
#define	NTR_START_UNIQUE(s)  	NTRuniqueWatch((s))
#define NTR_PRINTF(fmt,v1,v2,v3)   NTRstrlog(NTR_ModMask | NTRT_Strlog, \
										0, 0, 0,					\
                                        (fmt),						\
                                        (unsigned int)(v1),			\
                                        (unsigned int)(v2),			\
                                        (unsigned int)(v3))

/*
#define NTR_STRING(s)			NTRtrace((NTR_ModMask|NTRT_String),	\
									*((unsigned int *)(s)),			\
									*((unsigned int *)(s)+1),		\
									*((unsigned int *)(s)+2),		\
									*((unsigned int *)(s)+3))
*/

#define NTR_STRING(s)			NTRstring((NTR_ModMask|NTRT_String), (s))

#define NTR_REGISTER_CALLOUT(mask,function) \
								NTRregisterCallout( (mask), function )

/*
**	Defines for Unix Client
*/
#define NVLT_ENTER(argc)
#define NVLT_LEAVE(arg) arg
#define NVLT_EXT_CALL(who)			NTR_EXT_CALL(who)
#define NVLT_EXT_RETURN(who)		NTR_EXT_RETURN(who)
#define NVLT_TRACE(t,v1,v2,v3,v4)	/* NTR_TRACE(t,v1,v2,v3,v4) */
#define NVLT_PRINTF(fmt,v1,v2,v3)	NTR_PRINTF(fmt,v1,v2,v3)
#define NVLT_STRING(s)				NVLT_STRING(s)

#endif	/* NTR_TRACING */

#ifdef DEBUG

#define NTR_ASSERT(EX)		((void)((EX) || assfail(#EX, __FILE__, __LINE__)))

#else /* !DEBUG && !DEBUG_TRACE */

#define NTR_ASSERT(x) 		((void)0)

#endif /* DEBUG */

#endif /* ! DEBUG_TRACE */

#if (!defined(NTR_TRACING) && !defined(DEBUG_TRACE))

#define NTR_EXT_CALL(who)
#define NTR_EXT_RETURN(who)
#define NTR_ENTER(argc,arg1,arg2,arg3,arg4,arg5)
#define NTR_LEAVE(retcode)		(retcode)
#define NTR_VLEAVE()
#define NTR_TRACE(t,v1,v2,v3,v4)
#define	NTR_TRACE_OFF()
#define	NTR_TRACE_ON()
#define	NTR_NWSLOG_OFF()
#define	NTR_NWSLOG_ON()
#define	NTR_START_WATCH(s,u)
#define	NTR_STOP_WATCH(s,u)
#define	NTR_START_UNIQUE(s)	0
#define NTR_STRING(s)
#define NTR_PRINTF(fmt,v1,v2,v3)
#define NTR_REGISTER_CALLOUT(mask,function)

/*
**	Defines for Unix Client
*/
#define NVLT_ENTER(argc)
#define NVLT_LEAVE(arg) arg
#define NVLT_EXT_CALL(who)
#define NVLT_EXT_RETURN(who)
#define NVLT_TRACE(t,v1,v2,v3,v4)
#define NVLT_PRINTF(fmt,v1,v2,v3)
#define NVLT_STRING(s)

#endif /* (!defined(NTR_TRACING) && !defined(DEBUG_TRACE)) */
#endif /* _NET_NW_NTR_H  wrapper symbol for kernel use */
