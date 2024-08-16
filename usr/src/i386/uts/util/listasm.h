/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UTIL_LISTASM_H	/* wrapper symbol for kernel use */
#define	_UTIL_LISTASM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/listasm.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

#if defined(lint) || defined(__cplusplus)
extern void remque(void *e);
extern void insque(void *e, void *p);
#else /* lint */

asm void
insque(void *e, void *p)
{
%ureg e,p;
	movl	p,4(e)		/* e->rlink = p */
	movl	(p),%ecx	/* e->flink = p->flink */
	movl	%ecx,(e)
	movl	(p),%ecx	/* p->flink->rlink = e */
	movl	e,4(%ecx)
	movl	e,(p)		/* p->flink = e */

%ureg e; mem p;
	movl	p,%eax
	movl	%eax,4(e)	/* e->rlink = p */
	movl	(%eax),%ecx	/* e->flink = p->flink */
	movl	%ecx,(e)
	movl	(%eax),%ecx	/* p->flink->rlink = e */
	movl	e,4(%ecx)
	movl	e,(%eax)	/* p->flink = e */

%treg e,p;
	pushl	e
	movl	p,%eax
	popl	%edx
	movl	%eax,4(%edx)	/* e->rlink = p */
	movl	(%eax),%ecx	/* e->flink = p->flink */
	movl	%ecx,(%edx)
	movl	(%eax),%ecx	/* p->flink->rlink = e */
	movl	%edx,4(%ecx)
	movl	%edx,(%eax)	/* p->flink = e */

%treg e; mem p;
	movl	e,%edx
	movl	p,%eax
	movl	%eax,4(%edx)	/* e->rlink = p */
	movl	(%eax),%ecx	/* e->flink = p->flink */
	movl	%ecx,(%edx)
	movl	(%eax),%ecx	/* p->flink->rlink = e */
	movl	%edx,4(%ecx)
	movl	%edx,(%eax)	/* p->flink = e */

%mem e,p;
	movl	p,%eax
	movl	e,%edx
	movl	%eax,4(%edx)	/* e->rlink = p */
	movl	(%eax),%ecx	/* e->flink = p->flink */
	movl	%ecx,(%edx)
	movl	(%eax),%ecx	/* p->flink->rlink = e */
	movl	%edx,4(%ecx)
	movl	%edx,(%eax)	/* p->flink = e */
}
#pragma asm partial_optimization insque

asm void
remque(void *e)
{
%ureg e;
	movl	(e),%edx	/* edx = e->flink */
	movl	4(e),%ecx	/* ecx = e->rlink */
	movl	%edx,(%ecx)	/* e->rlink->flink = e->flink */
	movl	%ecx,4(%edx)	/* e->flink->rlink = e->rlink */

%mem e;
	movl	e,%eax

	movl	(%eax),%edx	/* edx = e->flink */
	movl	4(%eax),%ecx	/* ecx = e->rlink */
	movl	%edx,(%ecx)	/* e->rlink->flink = e->flink */
	movl	%ecx,4(%edx)	/* e->flink->rlink = e->rlink */
}
#pragma asm partial_optimization remque

#endif /* lint */

#define remque_null(e) \
	(remque(e), ((list_t *)(e))->flink = ((list_t *)(e))->rlink = NULL)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_LISTASM_H */
