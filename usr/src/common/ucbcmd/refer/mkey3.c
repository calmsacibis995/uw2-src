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

#ident	"@(#)ucb:common/ucbcmd/refer/mkey3.c	1.2"
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


#include <stdio.h>
#define COMNUM 500
#define COMTSIZE 997

char *comname = "/usr/ucblib/reftools/eign";
static int cgate = 0;
extern char *comname;
int comcount = 100;
static char cbuf[COMNUM*9];
static char *cwds[COMTSIZE];
static char *cbp;

common (s)
char *s;
{
	if (cgate==0) cominit();
	return (c_look(s, 1));
}

cominit()
{
	int i;
	FILE *f;
	cgate=1;
	f = fopen(comname, "r");
	if (f==NULL) return;
	cbp=cbuf;
	for(i=0; i<comcount; i++)
	{
		if (fgets(cbp, 15, f)==NULL)
			break;
		trimnl(cbp);
		c_look (cbp, 0);
		while (*cbp++);
	}
	fclose(f);
}

c_look (s, fl)
char *s;
{
	int h;
	h = hash(s) % (COMTSIZE);
	while (cwds[h] != 0)
	{
		if (strcmp(s, cwds[h])==0)
			return(1);
		h = (h+1) % (COMTSIZE);
	}
	if (fl==0)
		cwds[h] = s;
	return(0);
}
