/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:hier/TokenType.h	3.1" */
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

#if 0

The C++ tokens:

NEW		: "new"
DELETE		: "delete"
VOID		: "void"
OPERATOR	: "operator"
CONST		: "const"
FRIEND		: "friend"
VIRTUAL		: "virtual"
PUBLIC		: "public"
PRIVATE		: "private"
PROTECTED	: "protected"
CLASS		: "class"
STRUCT		: "struct"
UNION		: "union"
ID		: identifier or keyword other than the above
LITSTRING	: literal string or character constant
LITINT		: literal decimal integer
NL		: newline character (but only considered a token if the
		  global flag newlineIsToken is on)
CCOMMENT	: C-style comment (but only considered a token if the 
		  global flag commentIsToken is on)
CXXCOMMENT	: C++-style comment (but only considered a token if the
		  global flag commentIsToken is on)
COLON		: ":"
SEMI		: ";"
COMMA		: ","
STAR		: "*"
POUND		: "#"
LP		: "("
RP		: ")"
LC		: "{"
RC		: "}"
LS		: "["
RS		: "]"
LANGLE		: "<"
RANGLE		: ">"
QUAL		: "::"
EOFTOK		: end of input
OTHERTOK	: anything else

#endif


/* If you change this enum, make sure to also change the tokname array in 
*  CXXToken.c, and also the keyword table in CXXLexer.c.
*/

enum TokenType { NEW, DELETE, VOID, ID, LP, RP, LC, RC, STAR, 
	QUAL,  OPERATOR, SEMI, COMMA, POUND, LITINT, LITSTRING, 
	EOFTOK, OTHERTOK, COLON, PUBLIC, PRIVATE, PROTECTED, 
	VIRTUAL, CLASS, STRUCT, UNION, NL, LS, RS, CCOMMENT, CXXCOMMENT, 
	CONST, FRIEND, LANGLE, RANGLE, TEMPLATE
};

