/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_NUC_TOOLS_TRACE_NWCTRACE_H
#define _UTIL_NUC_TOOLS_TRACE_NWCTRACE_H

#ident	"@(#)kern-i386at:io/odi/msm/nwctrace2.h	1.1"
#ident	"$Header:$"

#ifdef _KERNEL_HEADERS

#include <svc/time.h>

#elif defined(_KERNEL)

#include <sys/time.h>

#endif	/* _KERNEL_HEADERS */

/*
 * format of a trace table entry.
 */
typedef struct {
	timestruc_t	time;
	ushort_t	proc;	/* proc table index */
	uchar_t		intr,	/* interrupt level */
			unused;	/* future expansion */
	uint_t		type;	/* trace event type */
	uint_t		v1,	/* caller's values */
			v2,
			v3,
			v4;
} trace_t;

/*
 * NVLT events fit in an unsigned long, comprised of 3 parts:
 *
 *	mmmmiitt
 *
 * where:
 *	mmmm		are the bitmask bits
 *	ii		is the 0-255 index into the NVLTmask array
 *	tt		is a 0-255 type value, for individual events
 */

/*
 * macros to make masks and events.
 */
#define NVLT_MAKE_MASK(mmmm, ii)	(((unsigned int)(mmmm)<<16) | ((ii)<<8))
#define NVLT_MAKE_EVENT(mmmmii, tt)	((mmmmii) | (tt))

/*
 * mask bits - index 0	- Core NUC.
 */
#define	NVLTM_fs	NVLT_MAKE_MASK(0x0001, 0)	/* nucfs */
#define	NVLTM_gts	NVLT_MAKE_MASK(0x0002, 0)	/* gts */
#define	NVLTM_gipc	NVLT_MAKE_MASK(0x0004, 0)	/* gipc	*/
#define	NVLTM_spil	NVLT_MAKE_MASK(0x0008, 0)	/* spil	*/
#define	NVLTM_spx	NVLT_MAKE_MASK(0x0010, 0)	/* spx stack */
#define	NVLTM_am	NVLT_MAKE_MASK(0x0020, 0)	/* AutoMounter */
#define	NVLTM_tool	NVLT_MAKE_MASK(0x0040, 0)	/* toolkit */
#define	NVLTM_wire	NVLT_MAKE_MASK(0x0080, 0)	/* network pkts	*/
#define	NVLTM_UNUSED0	NVLT_MAKE_MASK(0x0100, 0)	/* UNUSED */
#define	NVLTM_nwmp	NVLT_MAKE_MASK(0x0200, 0)	/* nwmp	*/
#define	NVLTM_ipxeng	NVLT_MAKE_MASK(0x0400, 0)	/* ipxeng */
#define	NVLTM_ipx	NVLT_MAKE_MASK(0x0800, 0)	/* ipx stack */
#define	NVLTM_UNUSED1	NVLT_MAKE_MASK(0x1000, 0)	/* UNUSED */	
#define	NVLTM_ncp	NVLT_MAKE_MASK(0x2000, 0)	/* ncp */
#define NVLTM_swatch	NVLT_MAKE_MASK(0x4000, 0)	/* StopWatch */
							/* start+stop */
#define NVLTM_ext	NVLT_MAKE_MASK(0x8000, 0)	/* External Calls */
							/* and Returns */

/*
 * mask bits - index 1	- Core NUC.
 */
#define	NVLTM_prof	NVLT_MAKE_MASK(0x0001, 1)	/* _prologue, */
							/* _epilogue calls */
#define	NVLTM_odi	NVLT_MAKE_MASK(0x0002, 1)	/* odi */

/*
 * mask bits - index 2 - NUC/Hardware drivers.
 */
#define	NVLTM_hba	NVLT_MAKE_MASK(0x0001, 2)	/* hba */
#define	NVLTM_hard	NVLT_MAKE_MASK(0x0002, 2)	/* hardware */

/*
 * mask bits - index 3 - reserved for NUC.
 */

/*
 * mask bits - index 4 - DOS/Merge/ipxtli.
 */
#define	NVLTM_mipx	NVLT_MAKE_MASK(0x0001, 4)	/* mipx STREAMS mod */
#define	NVLTM_mpip	NVLT_MAKE_MASK(0x0002, 4)	/* mpip STREAMS mod */
#define	NVLTM_merge	NVLT_MAKE_MASK(0x0004, 4)	/* merge drivers */

/*
 * mask bits - index 5 - NWU.
 */
#define	NVLTM_NWU_test	NVLT_MAKE_MASK(0x0001, 5)	/* TEST	*/
#define	NVLTM_NWU_lipmx	NVLT_MAKE_MASK(0x0002, 5)	/* lower ipx mux */
#define	NVLTM_NWU_ipx	NVLT_MAKE_MASK(0x0004, 5)	/* upper ipx mux */
#define	NVLTM_NWU_ripx	NVLT_MAKE_MASK(0x0008, 5)	/* ipx router */
#define	NVLTM_NWU_ncpipx	NVLT_MAKE_MASK(0x0010, 5)	/* ncpipx */
#define	NVLTM_NWU_ipxecho	NVLT_MAKE_MASK(0x0020, 5)	/* ipx echo */
#define	NVLTM_NWU_nemux	NVLT_MAKE_MASK(0x0040, 5)	/* nemux */
#define	NVLTM_NWU_spx	NVLT_MAKE_MASK(0x0080, 5)	/* spx stack */
#define	NVLTM_NWU_elap	NVLT_MAKE_MASK(0x0100, 5)	/* appletalk elap */
#define	NVLTM_NWU_ddp	NVLT_MAKE_MASK(0x0200, 5)	/* appletalk ddp */
#define	NVLTM_NWU_atp	NVLT_MAKE_MASK(0x0400, 5)	/* appletalk atp */
#define	NVLTM_NWU_pap	NVLT_MAKE_MASK(0x0800, 5)	/* appletalk pap */
#define	NVLTM_NWU_asp	NVLT_MAKE_MASK(0x1000, 5)	/* appletalk asp */
#define	NVLTM_NWU_nbio	NVLT_MAKE_MASK(0x2000, 5)	/* NetBIOS */
							/* connection proto */
#define	NVLTM_NWU_nbdg	NVLT_MAKE_MASK(0x4000, 5)	/* NetBIOS */
							/* datagram proto */
#define	NVLTM_NWU_nbix	NVLT_MAKE_MASK(0x8000, 5)	/* NetBIOS ipx shim */

/*
 * mask bits - index 6 - reserved for NWU.
 * mask bits - index 7 - reserved for NWU.
 * mask bits - index 8 - reserved for NWU.
 *
 * end mask definitions.
 */

/*
 * NOTE: The following type codes are reserved for the generic Enter
 * and Leave and other such events.
 */
#define	NVLTT_printf		0xfa
#define	NVLTT_strlog		0xfb
#define	NVLTT_String		0xfc
#define	NVLTT_Return		0xfd
#define	NVLTT_Enter		0xfe
#define	NVLTT_Leave		0xff

/*
 * specific event definitions.
 *
 * ipx stack events.
 */
#define NVLTT_ipxTopOfDo	NVLT_MAKE_EVENT( NVLTM_ipx, 1)
#define NVLTT_ipxEndOfDo	NVLT_MAKE_EVENT( NVLTM_ipx, 2)

/*
 * NUCFS SLEEP AND WAKE UP CALLS.
 */
#define NVLTT_NWfsSleep		NVLT_MAKE_EVENT( NVLTM_fs, 1)
#define NVLTT_NWfsWakeup	NVLT_MAKE_EVENT( NVLTM_fs, 2)

/*
 * NUCFS NWFS events
 */
#define NVLTT_LookUpDosPathName	NVLT_MAKE_EVENT( NVLTM_fs, 3)
#define NVLTT_ReleaseNode	NVLT_MAKE_EVENT( NVLTM_fs, 4)

/*
 * SPIL events.
 */
#define NVLTT_SPIL_NSINFO	NVLT_MAKE_EVENT( NVLTM_spil, 1)

/*
 * GIPC events.
 */
#define	NVLTT_Gipcif_ioctl_top	NVLT_MAKE_EVENT( NVLTM_gipc, 1)
#define	NVLTT_Gipcif_ioctl_bot	NVLT_MAKE_EVENT( NVLTM_gipc, 2)
#define	NVLTT_Gipcif_iobuf	NVLT_MAKE_EVENT( NVLTM_gipc, 3)

/*
 * NCP events.
 */
#define	NVLTT_NCP_GetPacket	NVLT_MAKE_EVENT( NVLTM_ncp, 1)
#define	NVLTT_NCP_FreePacket	NVLT_MAKE_EVENT( NVLTM_ncp, 2)

/*
 * ipxeng events.
 */
#define NVLTT_ipxeng_times	NVLT_MAKE_EVENT( NVLTM_ipxeng, 1)
#define NVLTT_clock		NVLT_MAKE_EVENT( NVLTM_ipxeng, 2)

/*
 * wire events.
 */
#define NVLTT_xmit		NVLT_MAKE_EVENT( NVLTM_wire, 1)
#define NVLTT_rec		NVLT_MAKE_EVENT( NVLTM_wire, 2)

/*
 * events used internally by the trace driver for
 * _prologue and _epilogue calls.
 */
#define NVLTT_prologue		NVLT_MAKE_EVENT( NVLTM_prof, NVLTT_Enter)
#define NVLTT_epilogue		NVLT_MAKE_EVENT( NVLTM_prof, NVLTT_Leave)

/*
 * end event definitions..
 */

/*
 * stopwatch assignments, actually an index into
 * the stopwatch_strings array found in ndtstring.c.
 */
#define	NVLT_SW_Read_Path		1
#define	NVLT_SW_GIPC_Read_Queue		2
#define	NVLT_SW_Head_Write_Queue	3
#define	NVLT_SW_Head_Read_Queue		4
#define NVLT_SW_NWfi_Lookup		5
#define NVLT_SW_ON_Q			6 /* Time spent on a streams queue */
#define NVLT_SW_ReadMsg			7 /* lifetime of a read message	*/
#define NVLT_SW_WriteMsg		8 /* lifetime of a write message */
#define NVLT_SW_Merge_Latency		9 /* time between wput and rput	*/

/*
 * bitmasks to isolate various event members.
 */
#define	NVLT_Mask_Mask		0xffff0000	/* just the mask bits */
#define	NVLT_Mask_MnI		0xffffff00	/* mask and index */
#define NVLT_Mask_Type		0x000000ff	/* type */
#define NVLT_Mask_Index		0x0000ff00	/* index */

/*
 * macros to isolate various event members in event x.
 */
#define	TR_TYPE(x)		((x) & NVLT_Mask_Type)	/* isolate type */
#define TR_MASK_INDEX(x)	((x) & NVLT_Mask_MnI)	/* mask bits and */
							/* index, NVLTM_* */
#define TR_MASK_BITS(x)		((x) & NVLT_Mask_Mask)	/* mask bits only */
#define TR_INDEX(x)		(((x) & NVLT_Mask_Index) >> 8)
							/* usable index */

#ifdef DEBUG_TRACE

unsigned int 			NVLTuniqueWatch(unsigned int);
void				NVLTtrace_off(void),
				NVLTtrace_on(void),
				NVLTstrlog_off(void),
				NVLTstrlog_on(void),
				NVLTcall(unsigned int, void(*)() ),
				NVLTtrace(unsigned int, unsigned int,	\
				unsigned int, unsigned int, unsigned int),
				NVLTenter( unsigned int, int),
				NVLTstring( unsigned int, char *),
				NVLTvleave( unsigned int);

#define NVLT_EXT_CALL(who)	NVLTcall((NVLTM_ext|NVLTT_Enter),	\
				(void(*)())(who))

#define NVLT_EXT_RETURN(who)	NVLTtrace((NVLTM_ext|NVLTT_Return),	\
				0,0,(unsigned int)(who),0);
#define NVLT_ENTER(argc)	NVLTenter((NVLT_ModMask|NVLTT_Enter),(argc))
#define NVLT_LEAVE(retcode)	NVLTleave((NVLT_ModMask|NVLTT_Leave),(retcode))
#define NVLT_VLEAVE()		NVLTvleave((NVLT_ModMask|NVLTT_Leave))
#define NVLT_TRACE(t,v1,v2,v3,v4)	NVLTtrace((t),			\
														(unsigned int)(v1),	\
				(unsigned int)(v2),	\
				(unsigned int)(v3),	\
				(unsigned int)(v4))

#define	NVLT_TRACE_OFF()	NVLTtrace_off()
#define	NVLT_TRACE_ON()		NVLTtrace_on()
#define	NVLT_STRLOG_OFF()	NVLTstrlog_off()
#define	NVLT_STRLOG_ON()	NVLTstrlog_on()
#define	NVLT_START_WATCH(s,u)	NVLTtrace((NVLTM_swatch|NVLTT_Enter),	\
				0,0,(s),(u))

#define	NVLT_STOP_WATCH(s,u) 	NVLTtrace((NVLTM_swatch|NVLTT_Return),	\
				0,0,(s),(u))

#define	NVLT_START_UNIQUE(s)	NVLTuniqueWatch((s))
#define NVLT_STRING(s)							\
				NVLTstring((NVLT_ModMask|NVLTT_String), (s))

#define NVLT_REGISTER_CALLOUT(mask,function)				\
				NVLTregisterCallout((mask), function)

#define	NVLT_PRINTF(fmt,v1,v2,v3)					\
				NVLTtrace((NVLT_ModMask|NVLTT_printf),	\
					(unsigned int)(fmt),		\
					(unsigned int)(v1),		\
					(unsigned int)(v2),		\
					(unsigned int)(v3))

#else	/* not DEBUG_TRACE */

#define NVLT_EXT_CALL(who)
#define NVLT_EXT_RETURN(who)
#define NVLT_ENTER(argc)
#define NVLT_LEAVE(retcode)			(retcode)
#define NVLT_VLEAVE()
#define NVLT_TRACE(t,v1,v2,v3,v4)
#define	NVLT_TRACE_ON()
#define	NVLT_TRACE_OFF()
#define	NVLT_STRLOG_ON()
#define	NVLT_STRLOG_OFF()
#define	NVLT_START_WATCH(s,u)
#define	NVLT_STOP_WATCH(s,u)
#define	NVLT_START_UNIQUE(s)		0
#define NVLT_STRING(s)
#define NVLT_REGISTER_CALLOUT(mask,function)
#define	NVLT_PRINTF(fmt,v1,v2,v3)

#endif	/* DEBUG_TRACE */

#endif	/* _UTIL_NUC_TOOLS_TRACE_NWCTRACE_H */
