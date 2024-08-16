/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)logname:logname.c	1.4.3.1"

#include <stdio.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
main() {
	char *name;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue.abi");
	(void)setlabel("UX:logname");

	name = cuserid((char *)NULL);
	if (name == NULL) {
		pfmt(stderr, MM_ERROR, ":5:Cannot get login name\n");
		return (1);
	}
	(void) puts (name);
	return (0);
}
