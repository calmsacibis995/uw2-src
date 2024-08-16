/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_SEIZE_H	/* wrapper symbol for kernel use */
#define _PROC_SEIZE_H	/* subject to change without notice */

#ident	"@(#)kern:proc/seize.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/proc.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/proc.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * use PROC_SEIZED when all the lwp's are seized
 */
#define PROC_SEIZED(lwp)	EVENT_SIGNAL(&(lwp)->l_procp->p_seized, 0)

#ifdef _KERNEL
extern boolean_t vm_seize(proc_t *);
extern void vm_unseize(proc_t *);
extern boolean_t become_seized(lwp_t *);
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_SEIZE_H */
