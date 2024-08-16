/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:fs/incl/fs.h	3.2" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#ifndef __FS_H
#define __FS_H

/* Prototypes of all the public freestore(3++) functions.
*
*  I made put them in the global name space and made them 
*  extern "C" so that the user can invoke them from the 
*  debugger, and without demangling the executable.
*/

extern "C" {
int fs_showall();
int fs_show(void *);
int fs_mark();
void fs_unmark();
int fs_since();
int fs_sincen(int);
int fs_watch(int);
int fs_unwatch(int);
void fs_watchall_d();
void fs_unwatchall_d();
void fs_watchall_n();
void fs_unwatchall_n();
void fs_break();
void fs_status();
void fs_dbxinit();
void fs_debugrc();
void fs_help();
void fs_verbose();
void fs_terse();
}

#endif
