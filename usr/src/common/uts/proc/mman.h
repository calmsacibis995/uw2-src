/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_MMAN_H	/* wrapper symbol for kernel use */
#define _PROC_MMAN_H	/* subject to change without notice */

#ident	"@(#)kern:proc/mman.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

/*
 * Protections are chosen from these bits, or-ed together.
 * Note - not all implementations literally provide all possible
 * combinations.  PROT_WRITE is often implemented as (PROT_READ |
 * PROT_WRITE) and (PROT_EXECUTE as PROT_READ | PROT_EXECUTE).
 * However, no implementation will permit a write to succeed
 * where PROT_WRITE has not been set.  Also, no implementation will
 * allow any access to succeed where prot is specified as PROT_NONE.
 */
#define	PROT_READ	0x1		/* pages can be read */
#define	PROT_WRITE	0x2		/* pages can be written */
#define	PROT_EXEC	0x4		/* pages can be executed */

#ifdef _KERNEL
#define	PROT_USER	0x8		/* pages are user accessable */
#define	PROT_ALL	(PROT_READ | PROT_WRITE | PROT_EXEC | PROT_USER)
#endif	/* _KERNEL */

#define	PROT_NONE	0x0		/* pages cannot be accessed */

/* sharing types:  must choose either SHARED or PRIVATE */
#define	MAP_SHARED	1		/* share changes */
#define	MAP_PRIVATE	2		/* changes are private */
#define	MAP_TYPE	0xf		/* mask for share type */

/* other flags to mmap (or-ed in to MAP_SHARED or MAP_PRIVATE) */
#define	MAP_FIXED	0x10		/* user assigns address */

/* these flags not yet implemented */
#define	MAP_RENAME	0x20		/* rename private pages to file */
#define	MAP_NORESERVE	0x40		/* don't reserve needed swap area */

/* these flags are used by memcntl */
#define PROC_TEXT	(PROT_EXEC | PROT_READ)
#define PROC_DATA	(PROT_READ | PROT_WRITE | PROT_EXEC)
#define SHARED		0x10
#define PRIVATE		0x20

#ifdef _KERNEL
#define PROT_EXCL	0x20
#endif	/* _KERNEL */

#define VALID_ATTR  (PROT_READ|PROT_WRITE|PROT_EXEC|SHARED|PRIVATE)

#ifdef notdef
/*
 * Not clear that this flag will ever be implemented
 */
#define	MAP_INHERIT	0x80		/* inherit this mapping accross exec */
#endif /* notdef */

/*
 * For the sake of backward object compatibility, we use the _MAP_NEW flag.
 * This flag will be automatically or'ed in by the C library for all
 * new mmap calls.  Previous binaries with old mmap calls with continue
 * to get 0 or -1 for return values.  New mmap calls will get the mapped
 * address as the return value if successful and -1 on errors.  By default,
 * new mmap calls automatically have the kernel assign the map address
 * unless the MAP_FIXED flag is given.
 */
#define	_MAP_NEW	0x80000000	/* users should not need to use this */

#if !defined(_KERNEL)

#include <sys/types.h> /* SVR4.0COMPAT */
#if defined(__STDC__)
extern int memcntl(caddr_t, size_t, int, caddr_t, int, int);
extern caddr_t mmap(caddr_t, size_t, int, int, int, off_t);
extern int mprotect(caddr_t, size_t, int);
extern int munmap(caddr_t, size_t);
extern int msync(caddr_t, size_t, int);
extern int mlock(caddr_t, size_t);
extern int munlock(caddr_t, size_t);
extern int mlockall(int);
extern int munlockall(void);

#else
/*
 * Except for old binaries mmap() will return the resultant
 * address of mapping on success and (caddr_t)-1 on error.
 */
extern caddr_t mmap();
extern memcntl();
extern int mprotect();
extern int munmap();
extern int msync();
extern int mlock();
extern int munlock();
extern int mlockall();
extern int munlockall();
#endif /* __STDC__ */
#endif /* !_KERNEL */

/* advice to madvise */
#define	MADV_NORMAL	0		/* no further special treatment */
#define	MADV_RANDOM	1		/* expect random page references */
#define	MADV_SEQUENTIAL	2		/* expect sequential page references */
#define	MADV_WILLNEED	3		/* will need these pages */
#define	MADV_DONTNEED	4		/* don't need these pages */

/* flags to msync */
#define	MS_SYNC		0x0		/* wait for msync */
#define	MS_ASYNC	0x1		/* return immediately */
#define	MS_INVALIDATE	0x2		/* invalidate caches */

/* functions to mctl */
#define	MC_SYNC		1		/* sync with backing store */
#define	MC_LOCK		2		/* lock pages in memory */
#define	MC_UNLOCK	3		/* unlock pages from memory */
#define	MC_ADVISE	4		/* give advice to management */
#define	MC_LOCKAS	5		/* lock address space in memory */
#define	MC_UNLOCKAS	6		/* unlock address space from memory */

/* flags to mlockall */
#define	MCL_CURRENT	0x1		/* lock current mappings */
#define	MCL_FUTURE	0x2		/* lock future mappings */

#ifdef _KERNEL

extern int drv_mmap(vaddr_t vaddr, paddr_t paddr, size_t len, vaddr_t *uvaddrp,
		    uint_t prot, uint_t maxprot, uint_t flags);
extern void drv_munmap(vaddr_t, size_t);

extern vaddr_t mmap_k(vaddr_t, size_t, int, int, int, off_t, int *);
extern int munmap_k(vaddr_t, size_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_MMAN_H */
