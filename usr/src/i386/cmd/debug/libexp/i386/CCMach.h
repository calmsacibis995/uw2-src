/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	CCMACH_H
#define CCMACH_H
#ident	"@(#)debugger:libexp/i386/CCMach.h	1.1"

#include "Link.h"
#include "Iaddr.h"

// The current value of the stack pointer is in stack_ptr
// An expression may include nested function calls - f(g(x1, x2), x3)
// Since the debugger may have already pushed stuff on to the stack for
// the outer function when it starts to evaluate the inner function call,
// it saves the current stack pointer in addr_stack.

class Stack_addr : public Stack
{
public:
	Iaddr	address;
};

class node_eval_mach
{
public:
	Stack_addr	addr_stack;
	Iaddr		stack_ptr;
};

#endif	// CCMACH_H
