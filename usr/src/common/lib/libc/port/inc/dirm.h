/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/dirm.h	1.2"

/*
 * needs <dirent.h>
 * Holding structure, used by reentrant opendir, readdir
 */
#include <stdlock.h>

struct dirplus {
	DIR dirdir;
	union {
		struct dirent align;
		char buf[DIRBUF];
	} u;
#ifdef _REENTRANT
	StdLock	dirlock;
#endif
};
