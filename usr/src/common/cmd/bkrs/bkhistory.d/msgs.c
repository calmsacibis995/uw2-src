/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bkhistory.d/msgs.c	1.3.5.2"
#ident  "$Header: msgs.c 1.2 91/06/21 $"

char *errmsgs[] = {
	"Option \"%c\" is invalid.\n",
	"Period argument \"%s\" is invalid.\n",
	"No other options may be specified with the \"%c\" option.\n",
	"All date option arguments are invalid.\n",
	"Warning: \"%s\" is not a valid field separator, using default.\n",
	"Unable to insert ROTATION comment into %s.\n",
	"Open of table %s failed (return code = %d).\n",
	"Unable to read table entry number %d (return code = %d).\n",
	"Warning: table %s has different format than expected.\n",
	"Unable to allocate memory for reading table entry.\n",
	"Warning: invalid argument %s ignored.\n",
	"Unable to allocate memory for date conversion.\n",
	"Table file %s does not exist or is not accessible.\n",
	"Argument \"%s\" is invalid.\n",
	"%s is not a regular file, therefore it is not a valid table.\n",
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );
