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

#ident	"@(#)ucb:common/ucbcmd/refer/refer3.c	1.2"
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


#include "refer..c"
#define move(x, y) close(y); dup(x); close(x);

corout(in, out, rprog, arg, outlen)
char *in, *out, *rprog;
{
	int pipev[2], fr1, fr2, fw1, fw2, n;
	int status;

	pipe(pipev); 
	fr1 = pipev[0]; 
	fw1 = pipev[1];
	pipe(pipev); 
	fr2 = pipev[0]; 
	fw2 = pipev[1];
	if (fork() == 0)
	{
		close(fw1); 
		close(fr2);
		move(fr1, 0);
		move(fw2, 1);
		execl(rprog, "deliv", arg, 0);
		err("Can't run %s", rprog);
	}
	close(fw2); 
	close(fr1);
	write(fw1, in , strlen(in));
	close(fw1);
	wait(&status);
	n = read(fr2, out, outlen);
	out[n] = 0;
	close(fr2);
	return(n);
}
