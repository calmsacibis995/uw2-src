/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
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

#ident	"@(#)libmail:libmail/getmsgid.c	1.1"
#ident	"@(#)libmail:libmail/getmsgid.c	1.1"
#include "libmail.h"
/*
    NAME
	getmessageid - create a string appropriate for a message id

    SYNOPSIS
	string *getmessageid(int add_angle_bracket)

    DESCRIPTION
	Create and return a string appropriate for use in a message id
	or content id header. The caller is responsible for destroying
	the returned string. If add_angle_bracket is set, then the
	message id will be surrounded with angle brackets, <>.
*/

string *getmessageid(add_angle_bracket)
int add_angle_bracket;
{
    static long thiscall = 0;
    string *timestr = long_to_string_format((long)time((long*)0), "%lx");
    string *pidstr = long_to_string_format((long)getpid(), "%lx");
    string *unique = long_to_string_format(thiscall++, "%lx");
    const char *cluster = mailsystem(0);
    const char *nodename = mailsystem(1);
    const char *left_bracket = add_angle_bracket ? "<" : "";
    const char *right_bracket = add_angle_bracket ? ">" : "";
    string *msgid;

    if (strcmp(cluster, nodename) == 0)
	msgid = s_xappend((string*)0, left_bracket,
	    s_to_c(timestr), s_to_c(unique), ".",
	    s_to_c(pidstr), "@", nodename, maildomain(), right_bracket, (char*)0);
    else
	msgid = s_xappend((string*)0, left_bracket,
	    s_to_c(timestr), s_to_c(unique), ".", s_to_c(pidstr), "@",
	    nodename, ".", cluster, maildomain(), right_bracket, (char*)0);

    s_free(timestr);
    s_free(pidstr);
    s_free(unique);
    return msgid;
}
