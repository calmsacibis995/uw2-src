/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/_thr_funcs.h	1.5"

extern void	*_get_tdata(int);

#define SETLOCALE	1
#define LOCALECONV	2
#define GTXT		3
#define HSEARCH		4
#define PTS		5
#define NATIVELOC	6
#define FULLOCALE	7
#define GETPASS		8
#define GETUT		9
#define GETSP		10
#define CRYPT		11
