/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_PSE_HAT_H	/* wrapper symbol for kernel use */
#define _MEM_PSE_HAT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/pse_hat.h	1.1"
#ident	"$Header: $"

#ifdef	_KERNEL

#include <util/types.h>	/* REQUIRED */
#include <mem/seg.h>

/*
 * PSE hat layer support routine, used by segpse and segkpse.
 */
void pse_hat_ptfree(hatpt_t *);
boolean_t pse_hat_chgprot(struct seg *, vaddr_t, ulong_t, uint_t, boolean_t);
void pse_hat_devload(struct seg *, vaddr_t, ppid_t, uint_t);
void pse_hat_unload(struct seg *, vaddr_t, ulong_t);
void pse_hat_statpt_devload(vaddr_t, ulong_t, ppid_t, uint_t);
void pse_hat_statpt_unload(vaddr_t, ulong_t);

#endif	/* _KERNEL */

#endif /* _MEM_PSE_HAT_H */
