/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _JBLEN
#ident	"@(#)sgs-head:i386/head/setjmp.h	1.9.5.5"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__

#if #machine(i386)
#define _SIGJBLEN	128	/* (sizeof(ucontext_t) / sizeof (int)) */
#elif #machine(i860)
#define _SIGJBLEN	137
#elif #machine(sparc)
#define _SIGJBLEN	19
#else
#define _SIGJBLEN	64
#endif

#if #machine(i860)
#define _JBLEN		22
#elif #machine(m68k)
#define _JBLEN		40
#elif #machine(m88k)
#define _JBLEN		24
#elif #machine(sparc)
#define _JBLEN		12
#else
#define _JBLEN		10
#endif

#else /*!__STDC__*/

#if i386
#define _SIGJBLEN	128	/* (sizeof(ucontext_t) / sizeof (int)) */
#elif i860
#define _SIGJBLEN	137
#elif sparc
#define _SIGJBLEN	19
#else
#define _SIGJBLEN	64
#endif

#if i860
#define _JBLEN		22
#elif m68k
#define _JBLEN		40
#elif m88k
#define _JBLEN		24
#elif sparc
#define _JBLEN		12
#else
#define _JBLEN		10
#endif

#endif /*__STDC__*/

typedef int	jmp_buf[_JBLEN];

#ifdef __STDC__

extern int	setjmp(jmp_buf);
extern void	longjmp(jmp_buf, int);

#if __STDC__ == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)

typedef int	sigjmp_buf[_SIGJBLEN];

extern int	sigsetjmp(sigjmp_buf, int);
extern void	siglongjmp(sigjmp_buf, int);

#endif

#if __STDC__ != 0
#define setjmp(env)	setjmp(env)
#endif

#else /*!__STDC__*/

typedef int	sigjmp_buf[_SIGJBLEN];

extern int	setjmp();
extern void	longjmp();
extern int	sigsetjmp();
extern void	siglongjmp();

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_JBLEN*/
