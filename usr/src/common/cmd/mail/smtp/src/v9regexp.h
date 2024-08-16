/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/v9regexp.h	1.2.7.1"
#ident "@(#)v9regexp.h	1.3 'attmail mail(1) command'"
/* the structure describing a sub-expression match */
typedef struct regsubexp {
	char *sp;
	char *ep;
} regsubexp;

/* a compiled regular expression */
typedef struct regexp regexp;

/* the routines */
extern regexp *regcomp proto((char*));
extern int regexec proto((regexp *progp, char *starts, regsubexp *mp, int ms));
extern void regsub proto((char *sp, char *dp, regsubexp *mp, int ms));

