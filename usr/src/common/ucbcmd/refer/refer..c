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

#ident	"@(#)ucb:common/ucbcmd/refer/refer..c	1.3"
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
#include <ctype.h>
#include <assert.h>
#include <string.h>

#define FLAG 003
#define AFLAG 007
#define NRFTXT 10000
#define NRFTBL 500
#define NTFILE 20
#define QLEN 512
#define ANSLEN 4000
#define TAGLEN 400
#define NSERCH 20
#define MXSIG 200		/* max bytes in aggregate signal */

extern FILE *in;
extern int endpush, sort, labels, keywant, bare;
extern int biblio, science, postpunct;
extern char *smallcaps;
extern char comname;
extern char *keystr;
extern char *convert;
extern int authrev;
extern int nmlen, dtlen;
extern char *rdata[], **search;
extern int refnum;
extern char *reftable[];
extern char *rtp, reftext[];
extern int sep;
extern char tfile[];
extern char gfile[];
extern char ofile[];
extern char hidenam[];
extern char *Ifile; extern int Iline;
extern FILE *fo, *ftemp;
extern char *input(),*lookat();
extern char *class(),*caps(),*revauth();
extern char *artskp(),*fpar();
extern char *trimnl();

extern char *getenv();
