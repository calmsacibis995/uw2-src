/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:common/head/malloc.h	1.9"

#ifndef _MALLOC_H
#define _MALLOC_H

/*
	Constants defining mallopt operations
*/
#define M_MXFAST	1	/* set size of blocks to be fast */
#define M_NLBLKS	2	/* set number of block in a holding block */
#define M_GRAIN		3	/* set number of sizes mapped to one, for
				   small blocks */
#define M_KEEP		4	/* retain contents of block after a free until
				   another allocation */
/*
	structure filled by 
*/
#ifndef _MALLINFO
#define _MALLINFO
struct mallinfo  {
	int arena;	/* total space in arena */
	int ordblks;	/* number of ordinary blocks */
	int smblks;	/* number of small blocks */
	int hblks;	/* number of holding blocks */
	int hblkhd;	/* space in holding block headers */
	int usmblks;	/* space in small blocks in use */
	int fsmblks;	/* space in free small blocks */
	int uordblks;	/* space in ordinary blocks in use */
	int fordblks;	/* space in free ordinary blocks */
	int keepcost;	/* cost of enabling keep option */
};	
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int       size_t;
#endif

#if defined(__STDC__)

extern void *malloc(size_t);
extern void free(void *);
extern void *realloc(void *, size_t);
extern int mallopt(int, int);
extern struct mallinfo mallinfo(void);
extern void *calloc(size_t, size_t);

#else

extern char *malloc();
extern void free();
extern char *realloc();
extern int mallopt();
extern struct mallinfo mallinfo();
extern char *calloc();

#endif	/* __STDC__ */

#endif 	/* _MALLOC_H */
