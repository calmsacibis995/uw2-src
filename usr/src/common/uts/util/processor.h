/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UTIL_PROCESSOR_H	/* wrapper symbol for kernel use */
#define _UTIL_PROCESSOR_H	/* subject to change without notice */

#ident	"@(#)kern:util/processor.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/bind.h>		/* 4.0MP COMPAT */
#include <fs/profs/prosrfs.h>   /* 4.0MP COMPAT */

#else	

#include <sys/bind.h>		/* 4.0MP COMPAT */
#include <sys/prosrfs.h>	/* 4.0MP COMPAT */

#endif /* _KERNEL_HEADERS */

#define	P_ONLINE	0x1
#define	P_OFFLINE	0x2
#define	P_FORCE_OFFLINE	0x2
#define	P_QUERY		0x4
#define P_STATUS	0x4	/* alias for P_QUERY */
#define	P_BAD		0x5
#define	PROC_BAD	(-1)

#define COLD_ONLINE     0       /* as argument for psm_online_engine() */
#define WARM_ONLINE     1

#ifndef	_KERNEL

#ifdef __STDC__
int p_online(processorid_t, int);
#else
int p_online();
#endif /* __STDC__ */

#endif /* !KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_PROCESSOR_H */
