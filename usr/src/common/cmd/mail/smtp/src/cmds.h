/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/cmds.h	1.4.2.3"
#ident "@(#)cmds.h	1.5 'attmail mail(1) command'"
/* cmds.h */

/*  Copyright 1984 by the Massachusetts Institute of Technology  */
/*  See permission and disclaimer notice in file "notice.h"  */

/*
 * smtp command strings and associated codes.  Note that the command code
 * MUST be equal to the index of the command in the table.
 */


struct	cmdtab	{
	char	*c_name;		/* command name */
	int	c_len;			/* command length */
} cmdtab[] = {
#define	NONE		0		/* no such command */
	{ "", 0, },
#define	HELO		1
	{ "HELO", 4, },
#define	MAIL		2
	{ "MAIL FROM:", 10 },
#define	RCPT		3
	{ "RCPT TO:", 8, },
#define	DATA		4
	{ "DATA", 4, },
#define	QUIT		5
	{ "QUIT", 4, },
#define	RSET		6
	{ "RSET", 4, },
#define	NOOP		7
	{ "NOOP", 4, },
#define VRFY		8		/* for RFC 1123 compliance */
	{ "VRFY", 4, },
/* sendmail compatibility */
#define ONEX		9
	{ "ONEX", 4, },
#define VERB		10
	{ "VERB", 4, },
#define DEBG		11
	{ "DEBUG", 5, },

	{ 0, 0, }			/* end of table marker */
};
 
#define ISLOWER(c)	('a' <= (c) && (c) <= 'z')
#define TOUPPER(c)	(islower(c) ? ((c) - ('a' - 'A')) : (c))
