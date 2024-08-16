/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:common/head/ucontext.h	1.3"

#ifndef _UCONTEXT_H
#define _UCONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/ucontext.h>

#if defined(__STDC__)

extern int getcontext(ucontext_t *);
extern int setcontext(ucontext_t *);
extern int swapcontext(ucontext_t *, ucontext_t *);
extern void makecontext(ucontext_t *, void(*)(), int, ...);

#else

extern int getcontext();
extern int setcontext();
extern int swapcontext();
extern void makecontext();

#endif

#ifdef __cplusplus
}
#endif

#endif 	/* _UCONTEXT_H */
