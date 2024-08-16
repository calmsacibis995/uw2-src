%{
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
%}
%{
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
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
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
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
#ident	"@(#)libnwutil:common/lib/libnutil/cmscan.l	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nwcm/cmscan.l,v 1.2 1994/05/02 19:29:05 vtag Exp $"

/*
 * Copyright 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <string.h>

#ifdef OS_SUN5
extern char * strdup(char *);
#endif

#include "y.tab.h"

extern int	NWCMConfigFileLineNo;

#ifdef yywrap
/*
 * Ahh.  Flex has been here.
 */
#undef yywrap
#endif

%}

a	[Aa]
c	[Cc]
e	[Ee]
f	[Ff]
i	[Ii]
l	[Ll]
n	[Nn]
o	[Oo]
r	[Rr]
s	[Ss]
t	[Tt]
u	[Uu]
v	[Vv]
x	[Xx]
y	[Yy]

O	[0-7]
D	[0-9]
X	[0-9A-Fa-f]

A	[A-Za-z_]
N	[0-9A-Za-z_]

W	[ \t\r]

%%

[;#].*$				;
\/\*				{
					int	c;

					do {
						if ((c = input()) == '*') {
							if ((c = input())
							    == '/') {
								break;
							}
						}
						if (c == '\n')
							NWCMConfigFileLineNo++;
					} while (c);
				}
{t}{r}{u}{e}			|
{y}{e}{s}			|
{o}{n}				|
{a}{c}{t}{i}{v}{e}		{
					yylval.boolean = 1;
					return(BooleanConstant);
				}
{f}{a}{l}{s}{e}			|
{n}{o}				|
{o}{f}{f}			|
{i}{n}{a}{c}{t}{i}{v}{e}	{
					yylval.boolean = 0;
					return(BooleanConstant);
				}
0{O}*				|
0{x}{X}+			|
{D}+				{
					yylval.integer = strtoul(yytext, NULL, 0);
					return(IntegerConstant);
				}

\"[^"]*\"			{
					yytext[yyleng - 1] = 0;
					yylval.string = strdup(&yytext[1]);
					return(QuotedString);
				}
{A}{N}*				{
					yylval.string = strdup(yytext);
					return(TokenString);
				}
{W}+				/*EMPTY*/;
.				return(yytext[0]);
\n				NWCMConfigFileLineNo++;
%%

yywrap()
{
	return 1;
}
