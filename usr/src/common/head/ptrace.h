/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PTRACE_H
#define _PTRACE_H

#ident	"@(#)head.usr:ptrace.h	1.2"

#include <sys/types.h>
#include <sys/regset.h>
#include <sys/ucontext.h>

/* fake "struct user" for ptrace emulation */
typedef struct user
{
    int *u_ar0;
    ucontext_t u_context;
} user_t;
#define USIZE 1
#ifndef ctob
#define ctob(x) ((x)<<12)
#endif

#endif _PTRACE_H
