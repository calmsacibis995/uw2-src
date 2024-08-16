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

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/hp2648/hp2648.h	1.3"
#ident	"$Header: $"

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Displays plot files on an hp2648a graphics terminals.  I have heard
 * that all hp plotting devices use the same control sequences, so this
 * might work for all hp devices capable of plotting.
 */

#include <stdio.h>

#define TERMINAL "/dev/tty"

#define	ENQ	05
#define ACK	06
#define ESC	033
#define GRAPHIC	'*'
#define MODE	'm'
#define PLOT	'p'
#define DISPLAY 'd'
#define PENUP	'a'
#define BINARY	'i'
#define ASCII	'f'
#define CR	'\n'

#define TRUE  1
#define FALSE 0

#define xsc(xi) (int)((int) (xi - lowx) * scalex + 0.5)
#define ysc(yi) (int)((int) (yi - lowy) * scaley + 0.5)

extern int shakehands;
extern int currentx;
extern int currenty;
extern int buffcount;
extern int fildes;
extern float lowx;
extern float lowy;
extern float scalex;
extern float scaley;
extern struct sgttyb sarg;
