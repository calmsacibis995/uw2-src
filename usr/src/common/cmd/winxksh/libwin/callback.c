/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/callback.c	1.1"
#include	"win.h"
#include	"keys.h"
#include	"callback.h"

/*
 * Execute a callback:
 * 
 */
int callback(int (*fp)(int, void *), int id, void *parm)
{
	if (fp)
		return(fp(id,parm));
	return 0;
}
