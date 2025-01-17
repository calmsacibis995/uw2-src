/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)link:link.c	1.4.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/link/link.c,v 1.1 91/02/28 17:43:27 ccs Exp $"

/***************************************************************************
 * Command: link
 * Inheritable Privileges: P_MACREAD,P_DACREAD,P_MACWRITE,P_DACWRITE,P_FILESYS
 *       Fixed Privileges: None
 * Notes: This command calls the link(2) system call directly.
 *	  The privileges passed to command are only used by link(2).
 *
 ***************************************************************************/

main(argc, argv)
int argc;
char *argv[];
{
	if(argc!=3) {
		write(2, "Usage: /usr/sbin/link from to\n", 30);
		exit(1);
	}
	exit((link(argv[1], argv[2]) == 0) ? 0 : 2);
}
