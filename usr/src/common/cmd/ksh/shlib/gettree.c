/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ksh:shlib/gettree.c	1.2.6.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ksh/shlib/gettree.c,v 1.1 91/02/28 17:41:52 ccs Exp $"

/*
 *   GETTREE.C
 *
 *   Programmer:  D. A. Lambeth
 *
 *        Owner:  D. A. Lambeth
 *
 *         Date:  April 17, 1980
 *
 *
 *
 *   GETTREE (MSIZE)
 *
 *        Create a shell associative memory with MSIZE buckets,
 *        and return a pointer to the root of the memory.
 *        MSIZE must be a power of 2.
 *
 *
 *
 *   See Also:  nam_link(III), nam_search(III), libname.h
 */

#include "name.h"

/*
 *   GETTREE (MSIZE)
 *
 *      int MSIZE;
 *
 *   Create an associative memory containing MSIZE headnodes or
 *   buckets, and return a pointer to the root of the memory.
 *
 *   Algorithm:  Memory consists of a hash table of MSIZE buckets,
 *               each of which holds a pointer to a linked list
 *               of namnods.  Nodes are hashed into a bucket by
 *               namid.
 */

struct Amemory *gettree(msize)
register int msize;
{
	register struct Amemory *root;

	--msize;
	root = new_of(struct Amemory,msize*sizeof(struct namnod*));
	root->memsize = msize;
	root->nexttree = NULL;
	while (msize>=0)
		root->memhead[msize--] = NULL;
	return (root);
}
