/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MEM_PHYSMAP_F_H	/* wrapper symbol for kernel use */
#define	_MEM_PHYSMAP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/physmap_f.h	1.3"
#ident	"$Header: $"

/*
 * family-specific macros for physmap and physmap_free
 */

extern caddr_t pse_physmap(paddr_t, ulong_t, uint_t);
extern boolean_t pse_physmap_free(caddr_t, ulong_t, uint_t);
extern vaddr_t physmap0(paddr_t, size_t);
extern boolean_t physmap0_free(vaddr_t, size_t);

#define	PHYSMAP_F(physaddr, nbytes, flags)	\
		{ \
		caddr_t va; \
		if ((va = (caddr_t)physmap0(physaddr, nbytes)) != NULL) \
			return va; \
		va = pse_physmap(physaddr, nbytes, flags); \
		if (va != NULL) \
			return va; \
		}

#define	PHYSMAP_FREE_F(vaddr, nbytes, flags)	\
		{ \
		if (physmap0_free((vaddr_t)vaddr, nbytes) == B_TRUE) \
			return; \
		if (pse_physmap_free(vaddr, nbytes, flags)) \
			return; \
		}

#endif	/* _MEM_PHYSMAP_F_H */
