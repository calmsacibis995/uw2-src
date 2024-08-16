/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwalloc.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nwalloc.c,v 1.6 1994/07/22 20:51:37 vtag Exp $"
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

/*
 *	NetWare/SRC 
 *
 *	Description:
 *		Frontend to a local memory allocator.
 *
 *      Extensively modified by K. Ward on Feb 10, 1993 with the following
 *      features:
 *
 *          - Created a footer
 *          - Next pointer, previous pointer, file name and line number added
 *            to header
 *          - Only create and calculate header and footer information if DEBUG
 *            is set
 *          - Better debug routines
 */

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <mt.h>
#include <memory.h>
#include <util_proto.h>

#include "memmgr_types.h"
#ifdef _REENTRANT
#include "nwutil_mt.h"
#endif

#ifdef DEBUG

#define	MAGICHEADER		(unsigned int)0xDEADFACE
#define MAGICFOOTER             (unsigned int)0x600FBA11

typedef struct LOCALHEADER {
    unsigned int magicNumber;
    struct LOCALHEADER *next;
    struct LOCALHEADER *prev; 
    unsigned int blockSize;   /* size of block, header, and footer */
    int line;                 /* source line number of call to allocator */
    char *name;               /* source file name of call to allocator */
} LOCALHEADER;

typedef struct LOCALFOOTER {
    unsigned int magicNumber;
} LOCALFOOTER;


static unsigned int allocRunningTotal = 0;
static LOCALHEADER *listHead = 0;
static LOCALHEADER *listEnd = 0;


/*
 * prototypes for static functions
 */
static void addToAllocList(LOCALHEADER *ptr);
static void deleteFromAllocList(LOCALHEADER *ptr);


#endif

/*****************Local Allocator Routines*************************/


/*
 *	Function:	nwalloc()
 *	Description:
 *		Front end to malloc.  Could be re-written to not use malloc.
 */
#ifdef DEBUG

void *nwalloc(unsigned int bytes,
	      char *fileName,
	      int lineNumber)

#else

void *nwalloc(unsigned int bytes)

#endif

{
    char *ptr;
#ifdef DEBUG
    LOCALHEADER *hptr;
    LOCALFOOTER *fptr;
#endif

    if(bytes == 0)
        return (void *)NULL;

#ifdef DEBUG
    bytes += (sizeof(LOCALHEADER) + sizeof(LOCALFOOTER));
#endif

    /*
     * Make sure user is asking for bytes that are word multiples
     */
    while(bytes % sizeof(int))
        bytes++;

    ptr = (char *)malloc(bytes);
    
    if(ptr == NULL) {
#ifdef DEBUG
	printf("nwalloc: Process memory exausted (bytes wanted: %d)\n", bytes);
#endif
	return((void *)NULL);
    }


#ifdef DEBUG
    /*
     * Save off some informational stats in header and footer
     */
    hptr = (LOCALHEADER *)ptr;
    hptr->magicNumber = (unsigned int)MAGICHEADER;
    hptr->blockSize = bytes;
    hptr->name = fileName;
    hptr->line = lineNumber;
    
    fptr = (LOCALFOOTER *)(ptr + bytes - sizeof(LOCALFOOTER));
    fptr->magicNumber = MAGICFOOTER;

    addToAllocList(hptr);
	
    allocRunningTotal += hptr->blockSize;
    ptr += sizeof(LOCALHEADER);
#endif

    return((void *)ptr);
}

/*
 *	Function:	nwfree()
 *	Description:
 *		Sister procedure to nwalloc().
 */
void nwfree(void *ptr)
{
    char *tmp;
#ifdef DEBUG
    LOCALHEADER *hptr;
    LOCALFOOTER *fptr;
#endif

    if(ptr == (void *) NULL) {
#ifdef DEBUG
	printf("nwfree: null pointer..dumping core.\n");
	abort();
#endif
        return;
    }

#ifdef DEBUG
    tmp = ((char *)ptr) - sizeof(LOCALHEADER); 
    hptr = (LOCALHEADER *)tmp;

    if(hptr == (void *) NULL) {
	printf("nwfree: null pointer....dumping core.\n");
	abort();
    }

    if(hptr->magicNumber != MAGICHEADER) {
	printf("nwfree: Bad magic number in header...dumping core.\n");
	abort();
    }

    fptr = (LOCALFOOTER *)(tmp + hptr->blockSize - sizeof(LOCALFOOTER));
    if(fptr->magicNumber != MAGICFOOTER) {
	printf("nwfree: Bad magic number in footer...dumping ore.\n");
	abort();
    }

    deleteFromAllocList(hptr);

    allocRunningTotal -= hptr->blockSize;
#else
    tmp = (char *)ptr;
#endif

    free((void *)tmp);

    return;
}

/*
 *	Function:	nwrealloc()
 *	Description:
 *		Front-end to realloc.  See nwalloc() for more details.
 */
#ifdef DEBUG

void *nwrealloc(void *ptr,
		unsigned int newSize,
		char *fileName,
		int lineNumber)

#else

void *nwrealloc(void *ptr,
		unsigned int newSize)

#endif

{
    char *tmp, *tmpnew;
#ifdef DEBUG
    LOCALHEADER *hptr;
    LOCALFOOTER *fptr;
#endif

    /*
     * If user asked for nothing, return now.
     */
    if(newSize == 0)
        return ptr;

    /*
     * If ptr is NULL, user really meant a normal nwalloc.
     */
#ifdef DEBUG
    if(ptr == (void *) NULL)
        return nwalloc(newSize, fileName, lineNumber);
#else
    if(ptr == (void *) NULL)
        return nwalloc(newSize);
#endif

#ifdef DEBUG
    tmp = (char *)ptr - sizeof(LOCALHEADER);
    hptr = (LOCALHEADER *)tmp;

    if(hptr->magicNumber != MAGICHEADER) {
	printf("nwrealloc: Bad magic number in header..dumping core.\n");
	abort();
    }

    fptr = (LOCALFOOTER *)(tmp + hptr->blockSize - sizeof(LOCALFOOTER));
    if(fptr->magicNumber != MAGICFOOTER) {
	printf("nwrealloc: Bad magic number in footer..dumping core.\n");
	abort();
    }

    allocRunningTotal -= hptr->blockSize;
#else
    tmp = (char *)ptr;
#endif

#ifdef DEBUG
    newSize += sizeof(LOCALHEADER);
#endif

    while(newSize % sizeof(int))
        newSize++;

    tmpnew = (char *) realloc(tmp, newSize);

    if(tmpnew == (char *)NULL) {
#ifdef DEBUG
	printf("nwrealloc: Process memory exausted (bytes wanted: %d)\n", newSize);
#endif
	return((void *)NULL);
    }
	
#ifdef DEBUG
    hptr = (LOCALHEADER *) tmpnew;
    /*
     * magicNumber should already be set in header.
     * Adjust header's blockSize.
     */
    hptr->blockSize = newSize;

    fptr = (LOCALFOOTER *) (tmpnew + hptr->blockSize - sizeof(LOCALFOOTER));
    fptr->magicNumber = MAGICFOOTER;

    allocRunningTotal += hptr->blockSize;
    tmpnew += sizeof(LOCALHEADER);
#endif

    return(tmpnew);
}

/*
 *	Function:	nwcalloc()
 *	Description:
 */
#ifdef DEBUG

void *nwcalloc(unsigned int num,
	       unsigned int bytes,
	       char *fileName,
	       int lineNumber)

#else

void *nwcalloc(unsigned int num,
	       unsigned int bytes)

#endif

{
    void *ptr;

#ifdef DEBUG
    ptr = (void *) nwalloc(num*bytes, fileName, lineNumber);
#else
    ptr = (void *) nwalloc(num*bytes);
#endif

    if(ptr != NULL) 
        memset(ptr, 0, num*bytes);

    return(ptr);
}

#ifdef DEBUG
/*
 * The following routines are defined only for DEBUG.
 */
BADBLOCKS *LocalMemoryCheck()
{
    register LOCALHEADER *ptr;
    register LOCALFOOTER *fptr;
    int allocTotal;
    static BADBLOCKS badBlocks;

    ptr = listEnd;

    badBlocks.headPtr = 0;
    badBlocks.footPtr = 0;

    /*
     * Scan forward through the list looking for bad magic
     * numbers in the header and footer.
     *
     * Note: since the list was build in reverse order, scanning
     * forward really means traversing the list backward.
     */
    while(ptr) {

	if(ptr->magicNumber != MAGICHEADER) {
	    badBlocks.headPtr = (void *)((char *)ptr + sizeof(LOCALHEADER));
	    break;
	}

	/*
	 * If a block with a bad footer was found, return it now.
	 */
	fptr = (LOCALFOOTER *)((char *)ptr + ptr->blockSize - sizeof(LOCALFOOTER));
	if(fptr->magicNumber != MAGICFOOTER) {
	    badBlocks.footPtr = (void *)((char *)ptr + sizeof(LOCALHEADER));
	    return(&badBlocks);
	}

	ptr = ptr->prev;
    }


    /*
     * If a block with a bad header was found, scan backward through
     * the list to see if there also exists a block with a bad footer.
     * If process memory is quite fragmented, there is little merrit
     * to this part of the algorithm. But, what the heck...
     *
     * Note: since the list was build in reverse order, scanning
     * backward really means traversing the list forward.
     */

    if(badBlocks.headPtr) {

	ptr = listHead;
	while(ptr) {

	    /*
	     * Oops, found another block with a bad header (it may
	     * (be the same one). Just return the first one
	     * found.
	     *
	     * If there isn't a bad footer, we will eventually come
	     * back around to the bad header found above, and return.
	     */
	    if(ptr->magicNumber != MAGICHEADER)
		return(&badBlocks);

	    fptr = (LOCALFOOTER *)((char *)ptr + ptr->blockSize - sizeof(LOCALFOOTER));
	    if(fptr->magicNumber != MAGICFOOTER) {

		/*
		 * Ah ha, found a block with a bad footer. Store it along
		 * with the previously found block that had a bad header,
		 * and return.
		 */
		badBlocks.footPtr = (void *)((char *)ptr + sizeof(LOCALHEADER));
		return(&badBlocks);
	    }

	    ptr = ptr->next;
	}

    }

    /*
     * Add up all of the block sizes.
     */
    ptr = listHead;
    allocTotal = 0;
    while(ptr) {
	allocTotal += ptr->blockSize;
	ptr = ptr->next;
    }

    if(allocTotal != GetAllocRunningTotal()) {
	fprintf(stderr, "LocalMemoryCheck: Sum of all memory block sizes is %d. ", allocTotal);
	fprintf(stderr, "Does not match allocated running total of %d.\n", GetAllocRunningTotal());
    }

    /*
     * Didn't find anything wrong.
     */
    return 0;
}

/*
 *	Function:	GetAllocRunningTotal()
 *	Description:
 *		Returns current local memory usage.
 */
unsigned int GetAllocRunningTotal(void)
{
    return(allocRunningTotal);
}

static void
addToAllocList(LOCALHEADER *ptr)
{
    register LOCALHEADER *old;

	MUTEX_LOCK( &head_list_lock );
    /*
     * add forward link
     */
    old = listHead;
    listHead = ptr;
    ptr->next = old;
    ptr->prev = 0;

    /*
     * If old listHead is NULL, this is the first link in the list.
     * Setup listEnd pointer. Otherwise, add a reverse link in the
     * list.
     */
    if(old == 0)
        listEnd = ptr;
    else
        old->prev = ptr;

	MUTEX_UNLOCK( &head_list_lock );
      
}

static void
deleteFromAllocList(LOCALHEADER *ptr)
{
    register LOCALHEADER *next, *prev;

    next = ptr->next;
    prev = ptr->prev;

    /*
     * If prev is NULL, the first link in the list is being
     * deleted. We will need to update the listHead pointer.
     */

	MUTEX_LOCK( &head_list_lock );
    if(prev == 0)
	listHead = next;
    else
        prev->next = next;

    /*
     * If next is NULL, the last link in the list is being
     * deleted. We will need to update the listEnd pointer.
     */
    if(next == 0)
        listEnd = prev;
    else
        next->prev = prev;
	MUTEX_UNLOCK( &head_list_lock );
}

void
ShowOutstandingBlocks(FILE *fp)
{
    register LOCALHEADER *ptr;
    register int allocTotal;

    ptr = listHead;
    allocTotal = 0;

    fprintf(fp, " Address    BlockSize       Line       File\n");
    fprintf(fp, "--------    ---------     --------     -------\n");
    while(ptr) {
	allocTotal += ptr->blockSize;
	fprintf(fp, "%8x     %8d     %8d     %s\n",
		ptr, ptr->blockSize, ptr->line, ptr->name);
	ptr = ptr->next;
    }

    if(allocTotal != GetAllocRunningTotal()) {
	fprintf(stderr, "ShowOutstandingBlocks: Sum of all memory block sizes is %d. ", allocTotal);
	fprintf(stderr, "Does not match allocated running total of %d.\n", GetAllocRunningTotal());
    }


    fprintf(fp, "\n");
}

char *
GetFileNameFromBlock(void *ptr)
{
    register LOCALHEADER *hptr;

    hptr = (LOCALHEADER *)((char *)ptr - sizeof(LOCALHEADER));

    return(hptr->name);
}


int
GetLineNumberFromBlock(void *ptr)
{
    register LOCALHEADER *hptr;

    hptr = (LOCALHEADER *)((char *)ptr - sizeof(LOCALHEADER));
    
    return(hptr->line);
}

#endif
