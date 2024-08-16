/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MERGE_MERGE386_H	/* wrapper symbol for kernel use */
#define _UTIL_MERGE_MERGE386_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/merge/merge386.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************

       Copyright (c) 1991 Locus Computing Corporation.
       All rights reserved.
       This is an unpublished work containing CONFIDENTIAL INFORMATION
       that is the property of Locus Computing Corporation.
       Any unauthorized use, duplication or disclosure is prohibited.

***************************************************************************/


/*
**  merge386.h
**		structure definitions for Merge hooks. 
*/

#if defined _KERNEL || defined _KMEMUSER

struct mrg_com_data {
	int (*com_ppi_func)();		/* Pointer to a Merge function  */
					/* to call depending on the VPI */
					/* currently attached to this   */
					/* device. NULL if no VPI is    */
					/* attached.		        */
	unsigned char *com_ppi_data; 	/* Pointer to a structure used	*/
					/* by the above function.	*/
					/* Set/used only by Merge.	*/
	long unused;			/* Reserved for future use	*/
	unsigned long baseport;		/* Base I/O port for this dev.  */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern void *vm86_idtp;		/* Per-engine IDT pointer */

#ifdef __STDC__
struct queue;
struct msgb;
extern int com_ppi_strioctl(struct queue *, struct msgb *,
			    struct mrg_com_data *, int cmd);
extern int portalloc(unsigned long, unsigned long);
extern void portfree(unsigned long, unsigned long);
#else
extern int com_ppi_strioctl();
extern int portalloc();
extern void portfree();
#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MERGE_MERGE386_H */
