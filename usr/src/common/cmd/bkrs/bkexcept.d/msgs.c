/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bkexcept.d/msgs.c	1.2.5.3"
#ident  "$Header: msgs.c 1.2 91/06/21 $"

char *errmsgs[] = {
	"Option \"%c\" is invalid.\n",
	"Only one argument may be specified.\n",
	"Unable to allocate memory for reading table entry.\n",
	"Unable to %s table %s (return code = %d).\n",
	"Search of table %s failed (return code = %d).\n",
	"Deletion of entry %d from table %s failed (return code = %d).\n",
	"Cannot open %s.\n",
	"'system' call failed, errno = %d.\n",
	"Unable to unlink temporary file %s, errno = %d.\n",
	"Unable to read entry %d of table %s (return code = %d).\n",
	"Table %s does not exist or is not accessible.\n",
	"Open of table %s failed (return code = %d).\n",
	"Warning: table %s has different format than expected.\n",
	"Unable to translate file %s, errno = %d.\n",
	"Pattern specified was null, unable to %s table.\n",
	"Warning: null pattern specified, displaying entire table.\n",
	"Argument \"%s\" is invalid.\n",
	"Warning: table %s has no ENTRY FORMAT record.\n",
	"Warning: table %s had no ENTRY FORMAT record.\n\t\tFormat record is being added to table.\n"
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );
