/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/calloc.c	1.1"

/*
 * calloc()
 *	Allocate zeroed memory.
 *
 * Done via bumping "curmem" value.
 *
 * callocrnd() is used to round up so that next allocation occurs
 * on the given boundary.
 */


#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>

extern int end;
extern void bzero(void *, size_t);

static caddr_t curmem = NULL;

/*
 * caddr_t
 * calloc(int)
 *	Core memory allocator.
 *
 * Calling/Exit State:
 *	"curmem" is set to the longword location just after the
 *	last used for loading this program and is updated upon
 *	each call to the next un-allocated location.
 *
 *	Returns the value of curmem, after zeroing "size"
 *	bytes starting at that location (size is rounded up
 *	to a int size).  This effectively returns a physically
 *	contiguous buffer.
 */
caddr_t
calloc(int size)
{
	caddr_t	val;

	if(curmem == NULL)
		/* loader aligns on long boundary - so just assign */
		curmem = (caddr_t)&end;
	size = roundup(size, sizeof (int));
	val = curmem;
	curmem += size;

	bzero(val, (unsigned)size);
	return (val);
}

/*
 * void
 * callocrnd(int)
 *	Cause the next allocation to be aligned on a specific boundary.
 *	
 * Calling/Exit State:
 *	"curmem" is set to the longword location just after the
 *	last used for loading this program, if not previously set.
 *	For each call it is then updated on to the next boundary
 *	which is a multiple of "bound", ensuring that the next
 *	allocation has the desired alignment.
 *
 * 	No return value.
 */
void
callocrnd(int bound)
{
	if(curmem == NULL)
		curmem = (caddr_t)&end;
	curmem = (caddr_t)roundup((int)curmem, bound);
}
