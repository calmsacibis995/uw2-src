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
#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/tools/sc_scanner.l	1.3"

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
#include <limits.h>

#include "sc_parser.h"
#include "y.tab.h"
%}

a	[Aa]
b	[Bb]
c	[Cc]
d	[Dd]
e	[Ee]
f	[Ff]
g	[Gg]
h	[Hh]
i	[Ii]
l	[Ll]
m	[Mm]
n	[Nn]
o	[Oo]
p	[Pp]
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
							lineno++;
					} while (c);
				}
\+				return(KW_ADD);
{b}{o}{o}{l}{e}{a}{n}		return(KW_BOOLEAN);
{d}{e}{c}{i}{m}{a}{l}		return(KW_DECIMAL);
\/				return(KW_DIV);
{h}{e}{x}{a}{d}{e}{c}{i}{m}{a}{l} |
{h}{e}{x}			return(KW_HEXADECIMAL);
{i}{d}{e}{n}{t}			return(KW_IDENT);
{i}{n}{t}{e}{g}{e}{r}		return(KW_INTEGER);
{m}{a}{x}{i}{n}{t}		{
					yylval.integer = ULONG_MAX;
					return(VAL_INTEGER);
				}
\%				return(KW_MOD);
\*				return(KW_MUL);
{o}{c}{t}{a}{l}			return(KW_OCTAL);
{s}{t}{r}{i}{n}{g}		return(KW_STRING);
\-				return(KW_SUB);
{u}{p}{p}{e}{r}{c}{a}{s}{e}	return(KW_UPPERCASE);
{t}{r}{u}{e}			|
{y}{e}{s}			|
{o}{n}				|
{a}{c}{t}{i}{v}{e}		{
					yylval.boolean = 1;
					return(VAL_BOOLEAN);
				}
{f}{a}{l}{s}{e}			|
{n}{o}				|
{o}{f}{f}			|
{i}{n}{a}{c}{t}{i}{v}{e}	{
					yylval.boolean = 0;
					return(VAL_BOOLEAN);
				}
0{O}*				|
0{x}{X}+			|
{D}+				{
					yylval.integer = strtoul(yytext, NULL, 0);
					return(VAL_INTEGER);
				}

\"[^"]*\"				{
					yytext[yyleng - 1] = 0;
					yylval.string = (char *)strdup(&yytext[1]);
					return(VAL_STRING);
				}
{A}{N}*\(\)			{
					yylval.string = (char *)strdup(yytext);
					yylval.string[strlen(yytext) - 2] = 0;
					return(VAL_FUNCTION);
				}
\(				return(KW_LPAREN);
\)				return(KW_RPAREN);
{A}{N}*				{
					yylval.string = (char *)strdup(yytext);
					return(VAL_TOKEN);
				}
{W}+				;
\n				{
					/*
					 * We can't rely on yylineno (which
					 * is built into many, but not all,
					 * scanner generators.
					 */
					lineno++;
				}
%%
