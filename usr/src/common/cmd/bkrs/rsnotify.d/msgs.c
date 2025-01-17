/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/rsnotify.d/msgs.c	1.2.5.2"
#ident  "$Header: msgs.c 1.2 91/06/21 $"

char *errmsgs[] = {
	"Option \"%c\" is invalid.\n",
	"Argument \"%s\" is invalid.\n",
	"Unable to allocate memory for reading table entry.\n",
	"Unable to read table entry number %d (return code = %d).\n",
	"Unable to assign %s value to table entry number %d (return code = %d).\n",
	"Warning: table %s has different format than expected.\n",
	"Open of table %s failed (return code = %d).\n",
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );
