/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/aux.h	1.4.3.2"
#ident "@(#)aux.h	1.5 'attmail mail(1) command'"
extern char *sysname_read proto((void));
extern char *domainname_read proto((void));
extern int delivery_status proto((string *line));
struct regsubexp;
extern void append_match proto((struct regsubexp *subexp, string *sp, int se));

/* mailbox types */
#define MF_NORMAL 0
#define MF_PIPE 1
#define MF_FORWARD 2
#define MF_NOMBOX 3
#define MF_NOTMBOX 4
