/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmalloc:common/malloc.c	1.4"

/* 
 * stub function for mallopt. libc malloc is the one you
 * really want to use. mallinfo has been added to libc.
 * We keep this stub around for the off chance that someone
 * may actually depend on mallopt being around, and we don't
 * want to break old source apps from compiling...
 */

int
#ifdef __STDC__
mallopt(int cmd, int value)
#else
mallopt(cmd, value)
int cmd;
int value;
#endif
{
	return 0;
}
