/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libexp/common/yystype.h	1.3"

#ifndef YYSTYPE_H
#define YYSTYPE_H

//
// -- ALL yacc semantics stack element types required by one or more
//    supported language must be included here.  In most cases integers
//    or pointers are sufficient.
//
// -- ALL expression lexers pass back YYSTYPE and Epos values.

#include "Const.h"

struct CCtree;

union YYSTYPE {
	int       i;
	int       r;  // really RegRef.
	char     *s;
	CCtree    *n;
	Const     c;
};

#endif /*YYSTYPE_H*/
