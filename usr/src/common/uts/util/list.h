/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_LIST_H	/* wrapper symbol for kernel use */
#define _UTIL_LIST_H	/* subject to change without notice */

#ident	"@(#)kern:util/list.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* 
 * Generic lists support.
 */

#ifdef _KERNEL_HEADERS

#include <util/listasm.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/listasm.h>

#endif /* _KERNEL_HEADERS */

/*
 * Lists are circular and doubly-linked, with headers.
 * When a list is empty, both pointers in the header
 * point to the header itself.
 */

#if defined(_KERNEL) || defined (_KMEMUSER)

/* list element */
typedef struct ls_elt {
	struct ls_elt *ls_next;
	struct ls_elt *ls_prev;
} ls_elt_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/* 
 * All take as arguments side effect-free pointers to list structures
 */
#define LS_ISEMPTY(listp)	\
	(((ls_elt_t *)(listp))->ls_next == (ls_elt_t *)(listp))
#define LS_INIT(listp) {			\
	((ls_elt_t *)(listp))->ls_next =	\
	((ls_elt_t *)(listp))->ls_prev =	\
	((ls_elt_t *)(listp));		\
}

#define LS_REMOVE(listp)	ls_remove((ls_elt_t *)(listp))

/* 
 * For these five, ptrs are to list elements, but qp and stackp are
 * implicitly headers.
 */
#define LS_INS_BEFORE(oldp, newp)	\
	ls_ins_before((ls_elt_t *)(oldp), (ls_elt_t *)(newp))
 
#define LS_INS_AFTER(oldp, newp)	\
	ls_ins_after((ls_elt_t *)(oldp), (ls_elt_t *)(newp))

#define LS_INSQUE(qp, eltp)	\
	ls_ins_before((ls_elt_t *)(qp), (ls_elt_t *)(eltp))

/* result needs cast; 0 result if empty queue */
#define LS_REMQUE(qp)		ls_remque((ls_elt_t *)(qp))

#define LS_PUSH(stackp, newp) \
	ls_ins_after((ls_elt_t *)(stackp), (ls_elt_t *)(newp))

/* result needs cast; 0 result if empty stack */
#define LS_POP(stackp)		ls_remque((ls_elt_t *)(stackp))

/* public function declarations */
extern void	 ls_ins_before(ls_elt_t *, ls_elt_t *);
extern void	 ls_ins_after(ls_elt_t *, ls_elt_t *);
extern ls_elt_t *ls_remque(ls_elt_t *);
extern void	 ls_remove(ls_elt_t *);

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct list {
	struct list * volatile flink;		/* forward link */
	struct list * volatile rlink;		/* reverse link */
} list_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#define INITQUE(l)	((l)->flink = (l)->rlink = (l))
#define EMPTYQUE(l)	((l)->flink == (l))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_LIST_H */
