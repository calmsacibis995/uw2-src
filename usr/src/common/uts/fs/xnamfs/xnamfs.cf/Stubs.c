/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/xnamfs/xnamfs.cf/Stubs.c	1.2"
#ident	"$Header: $"

int xnamvp() { return 0; }
int xnampreval() { return nopkg(); }

int xsdfree() { return nopkg(); }
void xsd_cleanup() {}
void xsdexit() {}
int xsdfork() {return 0; }
int sdget() { return nopkg(); }
int sdenter() { return nopkg(); }
int sdleave() { return nopkg(); }
int sdgetv() { return nopkg(); }
int sdwaitv() { return nopkg(); }
int sdsrch() { return 0; }

int xsemfork() { return 0; }
int creatsem() { return nopkg(); }
int opensem() { return nopkg(); }
int sigsem() { return nopkg(); }
int waitsem() { return nopkg(); }
int nbwaitsem() { return nopkg(); }
void closesem() {}
