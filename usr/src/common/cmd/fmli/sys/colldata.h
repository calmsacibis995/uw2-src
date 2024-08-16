/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fmli:sys/colldata.h	1.1"

/* structures common to colldata.c and strxfrm.c */

/* collation table entry */
typedef struct xnd {
	unsigned char	ch;	/* character or number of followers */
	unsigned char	pwt;	/* primary weight */
	unsigned char	swt;	/* secondary weight */
	unsigned char	ns;	/* index of follower state list */
} xnd;

/* substitution table entry */
typedef struct subnd {
	char	*exp;	/* expression to be replaced */
	long	explen; /* length of expression */
	char	*repl;	/* replacement string */
} subnd;

