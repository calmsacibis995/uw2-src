/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_LWP_F_H	/* wrapper symbol for kernel use */
#define _PROC_LWP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/lwp_f.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined _KERNEL || defined _KMEMUSER

/*
 * i386-specific part of LWP structure:
 */
struct lwp_f {
	struct stss *_l_tssp;	/* if non-NULL, private TSS */
	uchar_t _l_special;	/* LWP needs special context-switch handling */
};

/* _LWP_F is the family-specific part of the lwp_t structure */
#define _LWP_F		struct lwp_f _l_f;

#define l_tssp		_l_f._l_tssp
#define l_special	_l_f._l_special

/* Flags for l_special */
#define SPECF_DEBUGON	(1 << 0)
#define SPECF_PRIVTSS	(1 << 1)
#define SPECF_PRIVGDT	(1 << 2)
#define SPECF_PRIVLDT	(1 << 3)
#define SPECF_NONSTDLDT	(1 << 4)
#define SPECF_FPINTGATE	(1 << 5)
#ifdef MERGE386
#define SPECF_VM86	(1 << 6)
#endif

/* i386-specific trap event (l_trapevf) flag definitions: */
#define EVF_L_B1_ESCBUG	EVF_F_1		/* i386 B1 ESC bug workaround */
#define EVF_L_SIGFPE	EVF_F_2		/* send SIGFPE to the LWP */
#define EVF_L_SIGTRAP	EVF_F_3		/* send SIGTRAP to the LWP */
#ifdef MERGE386
#define EVF_L_MERGE	EVF_F_4		/* MERGE386 event */
#endif

/* i386-specific trap exit flags. */
#ifdef MERGE386
#define F_TRAPEXIT_FLAGS	(EVF_L_B1_ESCBUG|EVF_L_SIGFPE| \
				 EVF_L_SIGTRAP|EVF_L_MERGE)
#else
#define F_TRAPEXIT_FLAGS	(EVF_L_B1_ESCBUG|EVF_L_SIGFPE|EVF_L_SIGTRAP)
#endif


#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_LWP_F_H */
