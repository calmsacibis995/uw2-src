/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucbcmd/tbl/t7.c	1.3"
#ident	"$Header: $"
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
     
/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */
  

 /* t7.c: control to write table entries */
# include "t..c"
# define realsplit ((ct=='a'||ct=='n') && table[ldata][c].rcol)
runout()
{
int i;
if (boxflg || allflg || dboxflg) need();
if (ctrflg)
	{
	fprintf(tabout, ".nr #I \\n(.i\n");
	fprintf(tabout, ".in +(\\n(.lu-\\n(TWu-\\n(.iu)/2u\n");
	}
fprintf(tabout, ".fc %c %c\n", F1, F2);
fprintf(tabout, ".nr #T 0-1\n");
deftail();
for(i=0; i<nlin; i++)
	putline(i,i);
if (leftover)
	yetmore();
fprintf(tabout, ".fc\n");
fprintf(tabout, ".nr T. 1\n");
fprintf(tabout, ".T# 1\n");
if (ctrflg)
	fprintf(tabout, ".in \\n(#Iu\n");
}
runtabs(lform, ldata)
{
int c, ct, vforml, lf;
fprintf(tabout, ".ta ");
for(c=0; c<ncol; c++)
	{
	vforml=lform;
	for(lf=prev(lform); lf>=0 && vspen(table[lf][c].col); lf=prev(lf))
		vforml=lf;
	if (fspan(vforml,c))
		continue;
	switch(ct=ctype(vforml,c))
		{
		case 'n':
		case 'a':
			if (table[ldata][c].rcol)
			  if (lused[c]) /*Zero field width*/
				fprintf(tabout, "\\n(%du ",c+CMID);
		case 'c':
		case 'l':
		case 'r':
		    if (realsplit? rused[c]: (used[c]+lused[c]))
			fprintf(tabout, "\\n(%du ",c+CRIGHT);
			continue;
		case 's':
			if (lspan(lform, c))
				fprintf(tabout, "\\n(%du ", c+CRIGHT);
			continue;
		}
	}
fprintf(tabout, "\n");
}
ifline(s)
	char *s;
{
if (!point(s)) return(0);
if (s[0] == '\\') s++;
if (s[1] ) return(0);
if (s[0] == '_') return('-');
if (s[0] == '=') return('=');
return(0);
}
need()
{
int texlin, horlin, i;
for(texlin=horlin=i=0; i<nlin; i++)
	{
	if (allh(i))
		horlin++;
	else
	if (instead[i]!=0)
		continue;
	else
		texlin++;
	}
fprintf(tabout, ".ne %dv+%dp\n",texlin,2*horlin);
}
deftail()
{
int i, c, lf, lwid;
for(i=0; i<MAXHEAD; i++)
	if (linestop[i])
		fprintf(tabout, ".nr #%c 0-1\n", linestop[i]+'a'-1);
fprintf(tabout, ".nr #a 0-1\n");
fprintf(tabout, ".eo\n");
fprintf(tabout, ".de T#\n");
fprintf(tabout, ".ds #d .d\n");
fprintf(tabout, ".if \\(ts\\n(.z\\(ts\\(ts .ds #d nl\n");
	fprintf(tabout, ".mk ##\n");
	fprintf(tabout, ".nr ## -1v\n");
	fprintf(tabout, ".ls 1\n");
	for(i=0; i<MAXHEAD; i++)
		if (linestop[i])
			fprintf(tabout, ".if \\n(#T>=0 .nr #%c \\n(#T\n",linestop[i]+'a'-1);
if (boxflg || allflg || dboxflg) /* bottom of table line */
	if (!pr1403)
		fprintf(tabout, ".if \\n(T. .vs \\n(.vu-\\n(.sp\n");
	fprintf(tabout, ".if \\n(T. ");
	drawline(nlin,0,ncol, dboxflg ? '=' : '-',1,0);
	fprintf(tabout, "\n.if \\n(T. .vs\n");
	/* T. is really an argument to a macro but because of 
	   eqn we don't dare pass it as an argument and reference by $1 */
	for(c=0; c<ncol; c++)
		{
		if ((lf=left(nlin-1,c, &lwid))>=0)
			{
			fprintf(tabout, ".if \\n(#%c>=0 .sp -1\n",linestop[lf]+'a'-1);
			fprintf(tabout, ".if \\n(#%c>=0 ", linestop[lf]+'a'-1);
			tohcol(c);
			drawvert(lf, nlin-1, c, lwid);
			fprintf(tabout, "\\h'|\\n(TWu'\n");
			}
		}
	if (boxflg || allflg || dboxflg) /* right hand line */
		{
		fprintf(tabout, ".if \\n(#a>=0 .sp -1\n");
		fprintf(tabout, ".if \\n(#a>=0 \\h'|\\n(TWu'");
		drawvert (0, nlin-1, ncol, dboxflg? 2 : 1);
		fprintf(tabout, "\n");
		}
fprintf(tabout, ".ls\n");
fprintf(tabout, "..\n");
fprintf(tabout, ".ec\n");
}
