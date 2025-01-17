/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bkstatus.d/msgs.c	1.5.5.2"
#ident  "$Header: msgs.c 1.2 91/06/21 $"

char *errmsgs[] = {
	"Option \"%c\" is invalid.\n",
	"Argument \"%s\" is invalid.\n",
	"No other options may be specified with the \"%c\" option.\n",
	"The \"%c\" and \"%c\" options are mutually exclusive.\n",
	"Warning: \"%s\" is not a valid field separator, using default.\n",
	"Unable to insert ROTATION comment into %s (return code = %d).\n",
	"Open of table %s failed (return code = %d).\n",
	"Unable to read table entry number %d (return code = %d).\n",
	"Unable to allocate memory for login-to-userid conversion.\n",
	"Warning: table %s has different format than expected.\n",
	"Warning: invalid argument %s ignored.\n",
	"Warning: invalid state %c ignored.\n",
	"All option arguments are invalid.\n",
	"Illegal state character \"%c\" encountered.\n",
	"Unable to allocate memory for table entry.\n",
	"Table file %s does not exist or is not accessible.\n",
	"Period value must be at least 1 and not greater than %d.\n",
	"Unable to assign status field value in table entry %d (return code = %d).\n",
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );
