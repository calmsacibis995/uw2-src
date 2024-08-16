/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:util/nuc_tools/trace/traceuser.h	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/util/nuc_tools/trace/traceuser.h,v 1.2.4.1 1994/12/12 01:46:44 stevbam Exp $"

#ifndef _UTIL_NUC_TOOLS_TRACE_TRACEUSER_H
#define _UTIL_NUC_TOOLS_TRACE_TRACEUSER_H

/*
 *  Netware Unix Client 
 */

/*
 *	How to interpret the 64 bits stored in "time".
 */
enum timeStyle {
	ts_hres,										/* timestruc_t style						*/
	ts_32usec,										/* 32 bits of microseconds					*/
	ts_64nsec,										/* 64 bits of nanoseconds					*/
	ts_lboltAnd8254,								/* 32 bits lbolt, 32 bits i8254 counter		*/
	ts_P5
};


/*
 *	Structure returned on an NVLTioctl_Locate call.
 */
typedef struct NVLT_Locate {
													/* NOTE:  These are all pointers into kmem	*/
	trace_t			*start,							/* First trace table entry					*/
					*next,							/* Where the next entry will go				*/
					*end;							/* Last entry +1							*/

	int				nEngines,
					timingLoopCount,
					timingNanoSeconds;

	enum timeStyle	timeStyle;
} NVLT_Locate_t;


/* XXX  typedef char memData[1024]; */

typedef struct {
	int		region;									/* memory region				*/
	int		count;									/* number of allocated chunks	*/
	int		size;									/* size of allocated chunks		*/
} memRegion_t;


#define NVLT_MASK_ARRAY_SIZE		64				/* size of NVLTmask array				*/


#define	NREGIONS					16				/* number of memory regions to support	*/
typedef memRegion_t	memRegionSummary_t[NREGIONS];	/* array of region structs 				*/

/*
 *	ioctl "cmd" values for trace driver.
 */
#define	NVLTioctl_Locate		0xdeface1
#define	NVLTioctl_GetMask		0xdeface2
#define	NVLTioctl_SetMask		0xdeface3
#define	NVLTioctl_Reset			0xdeface4
#define	NVLTioctl_GetStrlogMask	0xdeface5
#define	NVLTioctl_SetStrlogMask	0xdeface6
#define	NVLTioctl_Suspend		0xdeface7
#define	NVLTioctl_Resume		0xdeface8
#define	NVLTioctl_SetSize		0xdeface9


/*
 *	ioctls for Memory Tracer
 */
#define	NVMTioctl_Set		0xdeface5
#define	NVMTioctl_Reset		0xdeface6

/* Generate generic Enter and Leave Events	*/

#define NVLT_Gen(mask)	{ ((mask)|NVLTT_Enter),"             Enter: %s( 0x%x, 0x%x) called by 0x%x" },\
						{ ((mask)|NVLTT_Leave),"             Leave: %s" }

#endif /* _UTIL_NUC_TOOLS_TRACE_TRACEUSER_H */
