/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/restore.d/msgs.c	1.3.5.3"
#ident  "$Header: msgs.c 1.2 91/06/21 $"

char *errmsgs[] = {
	"option \"%c\" is invalid.\n",
	"argument \"%s\" is invalid.\n",
	"\"%s\" is not a valid user name.\n",
	"All option arguments are invalid.\n",
	"\"%s\" is an invalid argument for the %c option.\n",
	"Only one of the \"%s\" options may be used at a time.\n",
	"Only able to restore %d in one %s command.\n",
	"Unable to read pending request entry for restore id %s.\n",
	"Restore id %s does not exist.\n",
	"Pending restore request table has bad format.\n",
	"Must have the same effective uid to cancel restore id %s.\n",
	"Unable to cancel restore id %s.\n",
	"Unable to open pending request table.\n",
	"Unable to get new restore id for %s, please try again.\n",
	"Unable to read pending restore request table.\n",
	"Restore request id for %s is %s.\n",
	"There is no information about %s.\n",
	"\"%s\" is an invalid job.\n",
	""
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );
