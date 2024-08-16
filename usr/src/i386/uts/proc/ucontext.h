/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_UCONTEXT_H	/* wrapper symbol for kernel use */
#define _PROC_UCONTEXT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/ucontext.h	1.1"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {

#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/regset.h>	/* REQUIRED */
#include <proc/signal.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/regset.h>	/* REQUIRED */
#include <sys/signal.h>	/* REQUIRED */

#else

#include <sys/types.h> /* SVR4.0COMPAT */
#include <sys/regset.h> /* SVR4.0COMPAT */
#include <sys/signal.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

typedef struct {
	gregset_t	gregs;	/* general register set */
	fpregset_t 	fpregs;	/* floating point register set */
} mcontext_t;

typedef struct ucontext {
	u_long		uc_flags;
	struct ucontext	*uc_link;
	sigset_t   	uc_sigmask;
	stack_t 	uc_stack;
	mcontext_t 	uc_mcontext;
	void		*uc_privatedatap;
	long		uc_filler[4];		/* pad the structureto 512 bytes */
} ucontext_t;

#define GETCONTEXT	0
#define SETCONTEXT	1

/* 
 * values for uc_flags
 * these are implementation dependent flags, that should be hidden
 * from the user interface, defining which elements of ucontext
 * are valid, and should be restored on call to setcontext
 */

#define	UC_SIGMASK	001
#define	UC_STACK	002
#define	UC_CPU		004
#define	UC_FP		010

#ifdef WEITEK
#define UC_WEITEK	020
#endif /* WEITEK */

#ifdef WEITEK
#define UC_MCONTEXT (UC_CPU|UC_FP|UC_WEITEK)
#else
#define UC_MCONTEXT (UC_CPU|UC_FP)
#endif /* WEITEK */

/* 
 * UC_ALL specifies the default context
 */

#define UC_ALL		(UC_SIGMASK|UC_STACK|UC_MCONTEXT)

#ifdef _KERNEL

void savecontext(ucontext_t *, k_sigset_t);
void restorecontext(ucontext_t *);

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif
#endif /* _PROC_UCONTEXT_H */
