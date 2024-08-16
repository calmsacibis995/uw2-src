/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libl:lib/yyless.c	1.9"
#if defined(__cplusplus) || defined(__STDC__)
void yyless(int x)
#else
yyless(x)
#endif
{
	extern char yytext[];
	register char *lastch, *ptr;
	extern int yyleng;
	extern int yyprevious;
	lastch = yytext+yyleng;
	if (x>=0 && x <= yyleng)
		ptr = x + yytext;
	else
	/*
	 * The cast on the next line papers over an unconscionable nonportable
	 * glitch to allow the caller to hand the function a pointer instead of
	 * an integer and hope that it gets figured out properly.  But it's
	 * that way on all systems .   
	 */
		ptr = (char *) x;
	while (lastch > ptr)
		yyunput(*--lastch);
	*lastch = 0;
	if (ptr >yytext)
		yyprevious = *--lastch;
	yyleng = ptr-yytext;
}
