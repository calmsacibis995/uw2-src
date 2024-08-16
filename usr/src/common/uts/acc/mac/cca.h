/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_MAC_CCA_H	/* wrapper symbol for kernel use */
#define _ACC_MAC_CCA_H	/* subject to change without notice */

#ident	"@(#)kern:acc/mac/cca.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This file is for Covert Channel Analysis use only.
 */

/*
 * #defines for MAC policy in the SysV 4.0 security enhancements.
 * These defines are used only in the covert channel analysis.
 */

/* @(#)uts-comm:acc/mac/cca.h	1.2 (Motorola) */

#if lint
#define MAC_ASSERT(PTR,REL)	
#define MAC_ASSUME(PTR,REL)	
#define MAC_VALIDPTR(PTR)	
#define MAC_GIVESVAL(EXPR,VAL)	
#define MAC_NOTGIVESVAL(EXPR,VAL)	
#define MAC_UNKLEV(PTR)		
#else
#define MAC_ASSERT(PTR,REL)	(0)
#define MAC_ASSUME(PTR,REL)	(0)
#define MAC_VALIDPTR(PTR)	(0)
#define MAC_GIVESVAL(EXPR,VAL)	(0)
#define MAC_NOTGIVESVAL(EXPR,VAL)	(0)
#define MAC_UNKLEV(PTR)		(0)
#endif /* lint */

#if defined(__cplusplus)
	}
#endif

#endif	/* _ACC_MAC_CCA_H */
