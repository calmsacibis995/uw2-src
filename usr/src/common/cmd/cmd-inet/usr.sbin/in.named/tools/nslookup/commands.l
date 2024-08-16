%{
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
%}
%{
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
%}
%{
/*	  All Rights Reserved  	*/
%}

%{
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
%}
%{
/*	The copyright notice above does not evidence any   	*/
%}
%{
/*	actual or intended publication of such source code.	*/
%}

%{
#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/tools/nslookup/commands.l	1.2.10.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
%}
%{
/*      SCCS IDENTIFICATION        */
%}
%{
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *      @(#)commands.l  5.9 (Berkeley) 6/18/88
 */

#ifndef lint
static char sccsid[] = "@(#)commands.l	5.13 (Berkeley) 7/24/90";
#endif /* not lint */

/*
 *******************************************************************************
 *
 *  commands.l
 *
 *	Andrew Cherenson	CS298-26  Fall 1985
 *
 *	Lex input file for the nslookup program command interpreter.
 *	When a sequence is recognized, the associated action
 *	routine is called. The action routine may need to
 *	parse the string for additional information.
 *
 *  Recognized commands: (identifiers are shown in uppercase)
 *
 *	server NAME	- set default server to NAME, using default server
 *	lserver NAME	- set default server to NAME, using initial server
 *	finger [NAME]	- finger the optional NAME
 *	exit		- exit the program
 *	root		- set default server to the root
 *	ls NAME		- list the domain NAME
 *	view FILE	- sorts and view the file with more
 *	set OPTION	- set an option
 *	help		- print help information
 *	?		- print help information
 *	NAME		- print info about the host/domain NAME
 *			  using default server.
 *	NAME1 NAME2	- as above, but use NAME2 as server
 *
 *
 *   yylex Results:
 *	0		upon end-of-file.
 *	1		after each command.
 *
 *******************************************************************************
 */

#include "res.h"
extern char rootServerName[];

%}
WS	[ \t]
FLET	[A-Za-z0-9.*\\]
LET	[A-Za-z0-9.*]
NAME	[A-Za-z0-9.*=_/-]
%%
^{WS}*server{WS}+{LET}{NAME}*{WS}*$	{
					    /*
					     * 0 == use current server to find
					     *	    the new one.
					     * 1 == use original server to find
					     *	    the new one.
					     */
					    SetDefaultServer(yytext, 0);
					    return(1);
					}
^{WS}*lserver{WS}+{LET}{NAME}*{WS}*$	{
					    SetDefaultServer(yytext, 1);
					    return(1);
					}
^{WS}*exit{WS}*$			{
					    return(0);
					}
^{WS}*root{WS}*$			{
					    SetDefaultServer(rootServerName, 1);
					    return(1);
					}
^{WS}*finger({WS}+{LET}{NAME}*)?{WS}+>>?{WS}*{NAME}+{WS}*$	{
					    /*
					     * 2nd arg.
					     *  0 == output to stdout
					     *  1 == output to file
					     */
					    Finger(yytext, 1);
					    return(1);
					}
^{WS}*finger({WS}+{LET}{NAME}*)?{WS}*$	{
					    Finger(yytext, 0);
					    return(1);
					}
^{WS}*view{WS}+{NAME}+{WS}*$	{
					    ViewList(yytext);
					    return(1);
					}
^{WS}*ls{WS}+(("-a"|"-d"|"-h"|"-m"|"-s"){WS}+)?{LET}{NAME}*{WS}+>>?{WS}+{NAME}+{WS}*$	{
					    /*
					     * 2nd arg.
					     *  0 == output to stdout
					     *  1 == output to file
					     */
					    ListHosts(yytext, 1);
					    return(1);
					}
^{WS}*ls{WS}+(("-a"|"-d"|"-h"|"-m"|"-s"){WS}+)?{LET}{NAME}*{WS}*$	{
					    ListHosts(yytext, 0);
					    return(1);
					}
^{WS}*ls{WS}+-t{WS}+({LET}{NAME}*{WS}+)?{LET}{NAME}*{WS}+>>?{WS}+{NAME}+{WS}*$ {
					    /*
					     * 2nd arg.
					     *  0 == output to stdout
					     *  1 == output to file
					     */
					    ListHostsByType(yytext, 1);
					    return(1);
					}
^{WS}*ls{WS}+-t{WS}+({LET}{NAME}*{WS}+)?{LET}{NAME}*{WS}*$ {
					    ListHostsByType(yytext, 0);
					    return(1);
					}
^{WS}*set{WS}+{NAME}+{WS}*$		{
					    SetOption(yytext);
					    return(1);
					}
^{WS}*help{WS}*$			{
					    extern void PrintHelp();

					    PrintHelp();
					    return(1);
					}
^{WS}*"?"{WS}*$				{
					    extern void PrintHelp();

					    PrintHelp();
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}+>>?{WS}*{NAME}+{WS}*$	{
					    /*
					     * 0 == output to stdout
					     * 1 == output to file
					     */
					    LookupHost(yytext, 1);
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}*$		{
					    LookupHost(yytext, 0);
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}+{LET}{NAME}*{WS}+>>?{WS}*{NAME}+{WS}*$	{
					    /*
					     * 0 == output to stdout
					     * 1 == output to file
					     */
					    LookupHostWithServer(yytext, 1);
					    return(1);
					}
^{WS}*{FLET}{NAME}*{WS}+{LET}{NAME}*{WS}*$	{
					    LookupHostWithServer(yytext, 0);
					    return(1);
					}
^{WS}*\n				{
					    return(1);
					}
^.*\n					{
					    printf("Unrecognized command: %s",
							yytext);
					    return(1);
					}
\n					{ ; }
%%
