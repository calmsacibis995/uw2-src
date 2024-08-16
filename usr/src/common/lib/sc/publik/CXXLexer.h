/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:publik/CXXLexer.h	3.1" */
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


#include "Lexer.h"
#include "CXXToken.h"

class CXXLexer: public Lexer
{
public:
	CXXLexer(): Lexer() { nlIsTok = cmtIsTok = 0; }
	~CXXLexer() {}
	void newlineIsToken() { nlIsTok = 1; }
	void newlineIsntToken() { nlIsTok = 0; }
	void commentIsToken() { cmtIsTok = 1; }
	void commentIsntToken() { cmtIsTok = 0; }
	DEFINE_GET(Litint, LITINT)
	DEFINE_GET(Litstring, LITSTRING)
protected:
	Token *gettok();
private:
	int nlIsTok, cmtIsTok;
	Litint *getLitint(const Token *base);
	Litstring *getLitstring(const Token *base);
	void getKeywordOrId(Token *base);
	void getQualOrColon(Token *base);
	int gobbleCommentsAndWhitespace(Token *base);
	void getAngleOrShift(char c, Token *base);
};

