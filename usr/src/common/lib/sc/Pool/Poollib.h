/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:Pool/Poollib.h	3.3" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include <Pool.h>
#include <stddef.h>
#include <memory.h>	/* used by includers, not here */
#include <assert.h>	/* used by includers, not here */

static const int minblocksz = 1000;
static const int maxblocksz = 10000;

// Note:  the next two 'static const int' definitions are designed to be
// evaluated at compile time (and are by the UnixWare C++ compiler), but
// if they aren't for some reason, they should probably be replaced by
// hard-coded constants for the system at hand, lest order of initialization
// problems crop up. 

// The padding in bytes that may be required in the case that the allocation
// is smaller than sizeof(Pool_element_header):  that is, the least power 
// of 2 >= sizeof(Pool_element_header).  (Since Pool_element_header is
// a class with just a pointer in it, this code handles pointers of
// 16, 32, and 64 bit sizes, which will give results of 2, 4, or 8.)

static const int round = sizeof(Pool_element_header) > 4 ? 8 :
                           (sizeof(Pool_element_header) > 2 ? 4 : 2);

struct align_ATTLC {
        char            c;
        long double     ld;
};

// The most stringent alignment in bytes, that must be preserved to satisfy
// all alignment restrictions.  Must be a power of 2 for the code to work.
// (Examples:  4 for i486, 8 for HP PA RISC implementation and SPARC spec.)
// We assume that the most stringent alignment comes with long doubles.

static const int block_round = offsetof(align_ATTLC, ld);

// offset in chars of the data part of a block
static const int blockoff = 
	((int)sizeof(Block_header_ATTLC)+block_round-1) & -block_round;

// rounds up n to the nearest multiple of round, except
// for 0, which gets rounded up to round
static size_t 
roundup(size_t n)
{
	return (( (int) n + (n == 0) + round - 1) & -round);
}
