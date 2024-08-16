/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/messages.c	1.2.11.6"
#ident  "$Header: messages.c 2.0 91/07/13 $"

char *errmsgs[] = {
	":1326:gid %ld is reserved.\n",
	":1327:invalid syntax.\nusage:  groupadd [-g gid [-o]] group\n",
	":1328:invalid syntax.\nusage:  groupdel group\n",
	":1329:invalid syntax.\nusage:  groupmod [-g gid [-o]] [-n name] group\n",
	":1330:Cannot update system files - group cannot be %s.\n",
	":1331:%s is not a valid group id.  Choose another.\n",
	":1332:%s is already in use.  Choose another.\n",
	":1333:%s is not a valid group name.  Choose another.\n",
	":1334:%s does not exist.\n",
	":1335:Group id %ld is too big.  Choose another.\n"
};

int lasterrmsg = sizeof( errmsgs ) / sizeof( char * );
