/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STDARG_H
#define _STDARG_H
#ident	"@(#)sgs-head:i386/head/stdarg.h	1.15"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__

#ifndef _VA_LIST
#if #machine(i860)
#	define _VA_LIST	struct _va_list
	struct _va_list
	{
		unsigned	_ireg_used;
		unsigned	_freg_used;
		long		*_reg_base;
		long		*_mem_ptr;
	};
#else
#	define _VA_LIST	void *
#endif
#endif /*_VA_LIST*/

#ifndef __VA_LIST
#   define __VA_LIST
	typedef _VA_LIST	va_list;
#endif

#if #machine(i860)

#ifdef __HIGHC__

extern void	*_va_arg(va_list *, unsigned, unsigned, unsigned);

#define va_start(ap, pn)				\
	{extern char	_ADDRESS_OF_MEMOFLO_AREA[];	\
	extern long	_ADDRESS_OF_INT_END_AREA[];	\
	extern int	_PARMBYTES_USEDI##pn[],		\
			_PARMBYTES_USEDF##pn[],		\
			_PARMBYTES_USEDM##pn[];		\
	ap._ireg_used = (int)_PARMBYTES_USEDI##pn>>2,	\
	ap._freg_used = (int)_PARMBYTES_USEDF##pn>>2,	\
	ap._mem_ptr = (void *)&pn,			\
	ap._mem_ptr = _ADDRESS_OF_MEMOFLO_AREA +	\
		(int)_PARMBYTES_USEDM##pn;		\
	ap._reg_base = (int)_ADDRESS_OF_INT_END_AREA-80;\
	}
#define va_arg(ap, ty)	(*(ty *)_va_arg(&ap,sizeof(ty),_INFO(ty,0),_INFO(ty,2)))

#else /*!__HIGHC__*/

extern void	__i860_builtin_va_start(va_list *);
extern void	*__builtin_va_arg();

#define va_start(ap,pn)	__i860_builtin_va_start(&ap)
#define va_arg(ap, ty)	(*(ty *)__builtin_va_arg(&ap, (ty *)0))

#endif /*__HIGHC__*/

#else /*!i860*/

#if #machine(sparc)

#define va_start(ap,pn)	((void)(ap = (va_list)&__builtin_va_alist))

#elif __STDC__ != 0 || !defined(__USLC__) || defined(__cplusplus)

#define va_start(ap,pn)	((void)(ap = (void *)((char *)&pn \
		+ ((sizeof(pn)+(sizeof(int)-1)) & ~(sizeof(int)-1)))))

#else

#define va_start(ap,pn)	((void)(ap = (void *)((char *)&...)))

#endif

#if #machine(m88k)

#define va_align(ap,ty)	((char *)(((int)ap + sizeof(ty) - 1) & (~(sizeof(ty)-1))))
#define va_arg(ap, ty)	(((ty *)(ap = va_align(ap, ty) + sizeof(ty)))[-1])

#elif #machine(sparc)

#define va_arg(ap, ty)	(((ty *)__builtin_va_arg_incr((ty *)ap))[0])

#elif #machine(i386) && defined(__cplusplus)

#define va_arg(ap, mode) ( \
		ap = (va_list)((char *)ap + (sizeof(mode) + sizeof(int) - 1 \
			 & ~(sizeof(int)-1))), \
		sizeof(mode) < sizeof(int) ? \
			*(mode *)((char *)ap - sizeof(int)) : \
			((mode *)ap)[-1])

#else

#define va_arg(ap, ty)	(((ty *)(ap = (char *)ap + sizeof(ty)))[-1])

#endif

#endif /*i860*/

extern void	va_end(va_list);

#define va_end(ap)	((void)0)

#else /*!__STDC__*/

#include <varargs.h>

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_STDARG_H*/
