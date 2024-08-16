/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_AUXV_H	/* wrapper symbol for kernel use */
#define _PROC_AUXV_H	/* subject to change without notice */

#ident	"@(#)kern:proc/auxv.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Format of auxillary vector entries.
 * The auxillary vector conveys information from
 * the operating system to the runtime loader.
 */
typedef struct {
       int     a_type;
       union {
               long    a_val;
#ifdef __STDC__
               void    *a_ptr;
#else
               char    *a_ptr;
#endif
               void    (*a_fcn)();
       } a_un;
} auxv_t;

/*
 * Auxillary vector types: a_type.
 */
#define	AT_NULL		0	/* terminates the vector */
#define	AT_IGNORE	1	/* ignore this entry; a_un undefined */
#define	AT_EXECFD	2	/* a_val: file descriptor for a.out */
#define	AT_PHDR		3	/* a_ptr: &phdr[0] */
#define	AT_PHENT	4	/* a_val: sizeof(phdr[0]) */
#define	AT_PHNUM	5	/* a_val: # of phdr entries */
#define	AT_PAGESZ	6	/* a_val: system page size in bytes */
#define	AT_BASE		7	/* a_ptr: base addr of interpreter */
#define	AT_FLAGS	8	/* a_val: processor flags */
#define	AT_ENTRY	9	/* a_ptr: a.out entry point */
#define	AT_LIBPATH	10	/* a_val: use LD_LIBRARY_PATH if non-zero */
#define	AT_FPHW		11	/* a_val: type of Floating Point hardware */
#define	AT_INTP_DEVICE	12	/* a_val: interpreter device number */
#define	AT_INTP_INODE	13	/* a_val: interpreter inode number */

#if defined(__cplusplus)
	}
#endif

#endif /*_PROC_AUXV_H */
