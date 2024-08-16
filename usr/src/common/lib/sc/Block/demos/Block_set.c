/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Block/demos/Block_set.c	3.1" */
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

#include "Block_set.h"
#include <stream.h>

template <class T>
static void compare(ptrdiff_t n, T* p)
{
	assert(n==0 || *p >= *(p-1));
}

template <class T>
void Blockset<T>::check()
{
	//  Check the representation invariant:
	//      n<=b.size()
	//      b is sorted
	//      b contains no repetitions
	T* t1 = &b[0];
	T* t2 = &b[n];
	assert(n<=b.size());
	generate(compare,t1,t2);
	assert(unique(t1,t2)==t2);
}

