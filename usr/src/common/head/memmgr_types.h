/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:memmgr_types.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: memmgr_types.h,v 1.7 1994/08/16 18:44:42 vtag Exp $"

#ifndef __memmgr_types_h__
#define __memmgr_types_h_
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdio.h>
#include <sys/nwportable.h>

typedef int Align;	/* Align to int boundary. */

#ifdef DEBUG

/*
 * This structure stores bad local memory blocks found from routine
 * LocalMemoryCheck.
 */
typedef struct BADBLOCKS {
    void *headPtr;         /* memory block has bad magic number in header */
    void *footPtr;         /* memory block has bad magic number in footer */
} BADBLOCKS;

enum state {FREE_BLOCK, ALLOCATED_BLOCK};

typedef struct trailer {
	unsigned char checksum[sizeof(uint32)];
} TRAILER;

#define NWALLOC(size)           nwalloc(size, __FILE__, __LINE__)
#define NWCALLOC(num, size)     nwcalloc(num, size, __FILE__, __LINE__)
#define NWREALLOC(ptr, size)    nwrealloc(ptr, size, __FILE__, __LINE__)
#define NWFREE(ptr)				nwfree(ptr)

#define MemPoolAlloc(poolp, size)   __MemPoolAlloc(poolp, size, __FILE__, __LINE__)
#define MemPoolCalloc(poolp, num, size)  __MemPoolCalloc(poolp, num, size, __FILE__, __LINE__)
#define MemPoolRealloc(poolp, memPtr, newSize)  __MemPoolRealloc(poolp, memPtr, newSize, __FILE__, __LINE__)

#else  /* DEBUG */

#define NWALLOC(size)           nwalloc(size)
#define NWCALLOC(num, size)     nwcalloc(num, size)
#define NWREALLOC(ptr, size)    nwrealloc(ptr, size)
#define NWFREE(ptr)				nwfree(ptr)

#define MemPoolAlloc(poolp, size)   __MemPoolAlloc(poolp, size)
#define MemPoolCalloc(poolp, num, size)  __MemPoolCalloc(poolp, num, size)
#define MemPoolRealloc(poolp, memPtr, newSize)  __MemPoolRealloc(poolp, memPtr, newSize)

#endif   /* DEBUG */

/*  File mempool.c */

void *MemCreateStaticPool(void *heapp, int size, int blkCnt,
		int semaFlag, char *name);
void *MemCreateDynamicPool(void *heapp, int minSize, int maxSize,
		int blkCnt, int semaFlag, char *name);
void MemDestroyPool(void *poolp);
void MemPoolUseBestFit(void *poolp);
void MemPoolUseFirstFit(void *poolp);
void MemPoolRoundAllocationSize(void *poolp, int roundSize);
#ifdef DEBUG
void *__MemPoolAlloc(void *poolp, int size, char *fileName, int lineNumber);
void *__MemPoolRealloc(void *poolp, void *memPtr, int newSize, char *fileName, int lineNumber);
void *__MemPoolCalloc(void *poolp, int num, int size, char *fileName, int lineNumber);
#else
void *__MemPoolAlloc(void *poolp, int size);
void *__MemPoolRealloc(void *poolp, void *memPtr, int newSize);
void *__MemPoolCalloc(void *poolp, int num, int size);
#endif
void MemPoolFree(void *poolp, void *ap);
char *MemPoolGetName(void *poolp);
int MemPoolGetType(void *poolp);
int MemGrowPool(void *poolp, int increment);
int MemPoolGetAlgorithm(void *poolp);
void MemPoolCoalesce(void *poolp);
double MemPoolGetFragmentationIndex(void *poolp);
double MemPoolGetRelativeFragIndex(double rawFragIndex, double base);
char *MemPoolCopyStr(void *poolp, char *string);


/* File memheap.c  */

void *MemCreateHeap(void *memPtr, int size);
void *MemAlloc(void *heapp, int size);
void MemFree(void *heapHeaderp, void *ap);
void *MemRealloc(void *heapp, void *memPtr, int newSize);
void *MemCalloc(void *heapp, int num, int size);
int MemHeapGetPoolCount(void *heapp);
void MemCoalescePools(void *heapp);
void MemValidatePools(void *heapp);
int MemEnumeratePools(void *heapp, void **poolTbl);

/* File nwalloc.c */

#ifdef DEBUG
void *nwalloc(unsigned int bytes, char *fileName, int lineNumber);
void *nwrealloc(void *ptr, unsigned int newSize, char *fileName, int lineNumber);
void *nwcalloc(unsigned int num, unsigned int bytes, char *fileName, int lineNumber);
void nwfree(void *ptr);
#else
void *nwalloc(unsigned int bytes);
void *nwrealloc(void *ptr, unsigned int newSize);
void *nwcalloc(unsigned int num, unsigned int bytes);
void nwfree(void *ptr);
#endif  /* DEBUG */


/*
** The following are for testing and debugging.
*/

/* File mempool.c */

int *MemGetPoolStats(void *poolp);
void MemPoolShowStats(void *poolp, FILE *fp);
int MemPoolValidate(void *poolp);
int MemPoolShowValidate(void *poolp, FILE *fp);
void MemPoolShowOutstandingBlocks(void *poolp, FILE *fp);
void MemPoolShowFreeBlocks(void *poolp, FILE *fp);
void MemPoolShowHeap(void *poolp, char *path);

/* File memheap.c */

int *MemHeapGetStats(void *heapp);
void MemHeapShowStats(void *heapp, FILE *fp);
void MemRegisterPool(void *heapp, void *poolp);
void MemUnregisterPool(void *heapp, void *poolp);
int MemHeapShowPoolStats(void *heapp, FILE *fp);
int MemHeapValidate(void *heapp);
int MemHeapShowValidate(void *heapp, FILE *fp);
void MemHeapShowOutstandingBlocks(void *heapp, FILE *fp);
void MemHeapShowFreeBlocks(void *heapp, FILE *fp);
void MemHeapShowHeap(void *heapp, char *path);


/* File nwalloc.c */

#ifdef DEBUG

BADBLOCKS *LocalMemoryCheck(void);
unsigned int GetAllocRunningTotal(void);
void ShowOutstandingBlocks(FILE *fp);
char *GetFileNameFromBlock(void *badBlockPtr);
int GetLineNumberFromBlock(void *badBlockPtr);

#endif

#endif /* __memmgr_types_h__ */ 
