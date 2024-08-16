/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:fs/fsippsrc/scope.c	3.1" */
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

#include "fsipp.h"
#include "scope.h"

List_of_p<Scope> scopeStack;

void pushClassScope(String tag)
{
	scopeStack.put(new Scope(classScope, tag));
}

void pushScope()
{
	scopeStack.put(new Scope(otherScope));
}

void popScope()
{
	if (scopeStack.length() == 0)
		syntaxError("Unmatched right brace");
	else
	{
		delete (scopeStack.unput());
	}
}

static String foo;
bool containingClass()
{
	return containingClass(foo);
}

bool containingClass(String &className)
{
	List_of_piter<Scope> scopeStacki(scopeStack);

	for (scopeStacki.end_reset(); !scopeStacki.at_head(); scopeStacki.step_prev())
	{
		if (scopeStacki.peek_prev()->type == classScope)
		{
			className = scopeStacki.peek_prev()->name;
			return yes;
		}
	}
	return no;
}

