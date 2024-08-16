/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)listen:nsaddr.h	1.1.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/listen/nsaddr.h,v 1.1 91/02/28 17:44:04 ccs Exp $"

/*
	stoa - convert string to address

	atos - convert address to string

	header file
*/


#define	OCT	0	/* octal type */
#define	HEX	1	/* hexadecimal type */
#define	RAW	2	/* string type */
#define KEEP	8	/* keep protocol field	*/

#define	SBUFSIZE	128

struct netbuf {
	unsigned maxlen;
	unsigned len;
	char *buf;
};

struct address {
	char		*protocol;
	struct netbuf	addbuf;
};

struct netbuf	*stoa(/* str, netbuf */);
char		*atos(/* str, netbuf, type */);
struct address	*astoa(/* str, addr */);
char		*aatos(/* str, addr, type */);
