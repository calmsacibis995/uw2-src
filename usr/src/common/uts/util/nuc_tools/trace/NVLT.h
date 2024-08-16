/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:util/nuc_tools/trace/NVLT.h	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/util/nuc_tools/trace/NVLT.h,v 1.2.4.1 1994/12/12 01:46:12 stevbam Exp $"

#ifndef _UTIL_NUC_TOOLS_TRACE_NVLT_H
#define _UTIL_NUC_TOOLS_TRACE_NVLT_H

/*
 *  Netware Unix Client 
 */


#define	NVLT_TIMING_LOOPS	10000
#define	NVLT_ENTRIES		8192

#define DPRINTF(bit, param) \
		do { \
			if( trace_debug & bit) { \
				printf param ; \
			} \
		} while(0);

#endif /* _UTIL_NUC_TOOLS_TRACE_NVLT_H */

