/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:publik/Lexer.c	3.2" */
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

#include <stdlib.h>
#include <unistd.h>

//List_of_pimplement(Token)

int Lexer::docontract(int i, ContractAction ca, int explicit)
{
	if (explicit && frozen == 2) 
	{
		ContractingFrozenLexer.raise("Attempt to explicitly contract a completely frozen lexer!");
		return 0;
	}
	int n = 0;
	if (explicit || !frozen) 
	{
		while (i-- > 0 && curToki > 0 && theWindow.length() > 0) 
		{ 
			Token *t = (Token*)(theWindow.get());
			if (ca)
				(*ca)(t);
			delete t;
			curToki--; 
			n++; 
		}
	}
	return n;
}

static int byebye(const char *s)
{
	cerr << s << endl;
	abort();
	return 0;
}

Objection Lexer::DestroyingFrozenLexer(byebye);
Objection Lexer::ContractingFrozenLexer(byebye);
Objection Lexer::DiscardedToken(byebye);
Objection Lexer::BadHandshake(byebye);

