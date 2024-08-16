/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)localedef:common/cmd/localedef/colltbl/colltbl.h	1.1.5.2"

/* Diagnostic mnemonics */
#define WARNING		0
#define ERROR		1
#define ABORT		2

enum errtype {
	GEN_ERR,
	DUPLICATE,
	EXPECTED,
	ILLEGAL,
	TOO_LONG,
	INSERTED,
	NOT_FOUND,
	NOT_DEFINED,
	TOO_MANY,
	INVALID	,
	BAD_OPEN,
	NO_SPACE,
	NEWLINE,
	REGERR,
	CWARN,
	YYERR,
	PRERR
};

/* Diagnostics Functions and Subroutines */
void	error();
void	regerr();
void	usage();
