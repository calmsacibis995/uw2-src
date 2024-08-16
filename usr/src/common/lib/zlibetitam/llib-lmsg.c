/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:llib-lmsg.c	1.2"
/*LINTLIBRARY*/
#include "tam.h"
#include "message.h"
#include <errno.h>
#include "menu.h"
#include "path.h"
#include <stdio.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

	/*VARARGS*/
int	message(mtype, file, title, format, args) int mtype; char *file, *title, *format; {return 0;}
int	_domsg (mtype, file, title, ptr) int mtype; char *file, *title, *ptr; {return 0;}
int	_mwcr (height, top_ju, help_fl) int height; char top_ju, help_fl; {return 0;}
int	exhelp (file, title) char *file, *title; {return 0;}
