/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:inc/io.h	1.4.3.3"

/*#include        <stdio.h>*/
#include        "mio.h"
/*#include        "mess.h"  abs s12 */

#define STR_SIZE 512
#define ANY 99
#define DV      87      /* dummy var for exit code */

#ifndef YES
#define YES     1
#endif

#ifndef NO
#define NO      0
#endif

#define FILENO 7
#define FILECOUNT 10
#define ISDIR   99
#define ISFILE  88
#define RESTART LBUT
#define DONE    LBUT-1
#define MORE    LBUT-2
#define PRE     LBUT-3
#define BUT1    FBUT
#define BUT2    FBUT+1
#define BUT3    FBUT+2
#define BUT4    FBUT+3
#define BUT5    FBUT+4
#define BUT1R LBUT-6
#define BUT2R LBUT-5
#define BUT3R LBUT-4
#define CRYPT   70
#define RECRYPT 90
#define REG	64
#define FULL    65
#define STRUCT  66
#define BOTTOMLEVEL     4
#define GCOUNT  9

#define PROCESS 0
#define THROW_OUT -1
#define EDIT	-2
#define MAXPGS  102


#define	NOTSET	0
#define ENDDOC	1
#define ASCII	2

char *ctime();

#ifndef MAIL
#define	NEWS	16	
#define	MAIL	17	
#endif
