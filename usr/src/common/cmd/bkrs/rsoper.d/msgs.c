/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/rsoper.d/msgs.c	1.3.6.2"
#ident  "$Header: msgs.c 1.2 91/06/21 $"

char *errmsgs[] = {
	"option \"%c\" is invalid.\n",
	"argument \"%s\" is invalid.\n",
	"\"%s\" is not a valid user name.\n",
	"All option arguments are invalid.\n",
	"\"%s\" is an invalid argument for the %c option.\n",
	"The \"%c\" option may not be used with the \"%c\" option.\n",
	"Unable to open a temporary file (%s). Mail was not sent to %s.\n",
	"Unable to read pending request entry for restore id %s.\n",
	"Restore id %s does not exist.\n",
	"Pending restore request table has bad format.\n",
	"Must have the same effective uid to cancel restore id %s.\n",
	"Unable to %s restore id %s.\n",
	"Unable to open pending request table.\n",
	"Must have one of the following options: \"%s\".\n",
	"Unable to read pending restore request table.\n",
	"Unable to satisfy any restore requests;\n\tthe following information is needed:\n",
	"The \"%c\" option is not yet implemented.\n",
	"Restore request %s for %s was not completed.\n",
	"Unable to read pending restore request table: %s.\n",
	"Unable to spawn %s method\n",
	"Unable to read label from %s.\n",
	"Unable to satisfy any restore requests with this volume.\n",
	"Unable to spawn %s method: %s.\n",
	"Unable to reserve devices %s and %s to service restore reqests.\n",
	"Devices %s and/or %s are currently busy.  Please try later.\n",
	"\"%s\" is not a valid jobid.\n",
	"-m option passed null method name.\n" 
	""
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );
