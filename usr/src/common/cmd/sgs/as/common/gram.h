/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/gram.h	1.2"
/*
* common/gram.h - common assembler parsing header
*
* Depends on:
*	<stdio.h>
*/

#ifdef __STDC__
void	initchtab(void);	/* initialize tokenization code */
void	setinput(FILE *);	/* setup stream for parsing */
int	yyparse(void);		/* parse current input stream */
char	*tokstr(int);		/* printable version of token */
#else
void	initchtab();
void	setinput();
int	yyparse();
char	*tokstr();
#endif
