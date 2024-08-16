/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:publik/scope.h	3.1" */
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

#include "String.h"
#include "TokenType.h"

typedef enum { classScope, otherScope } ScopeType;

class Scope 
{
public:
	ScopeType type;
	String name;  // name of class, if this is a class scope.
	TokenType pr;	// current protection level in this scope, if this is a class scope
	Scope(ScopeType t): type(t), pr(PRIVATE) {}
	Scope(ScopeType t, String n): type(t), pr(PRIVATE), name(n) {}
	~Scope() {}
};

