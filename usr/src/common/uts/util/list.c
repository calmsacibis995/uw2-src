/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/list.c	1.1"

/* Generic lists
 * Lists are circular, doubly-linked, with headers.
 * When a list is empty, both pointers in the header
 * point to the header itself.
 */

#include <util/list.h>

/*
 * void
 * ls_ins_before(ls_elt_t *, ls_elt_t *)
 *	Link new into list after old.
 *
 * Calling/Exit State:
 *
 *	None.
 */
void
ls_ins_before(ls_elt_t *old, ls_elt_t *new)
{
	new->ls_prev = old->ls_prev;
	new->ls_next = old;
	new->ls_prev->ls_next = new;
	new->ls_next->ls_prev = new;
}

/*
 * void
 * ls_ins_after(ls_elt_t *, ls_elt_t *)
 *
 *	Link new into list after old.
 *
 * Calling/Exit State:
 *
 *	None.
 */
void
ls_ins_after(ls_elt_t *old, ls_elt_t *new)
{
	new->ls_next = old->ls_next;
	new->ls_prev = old;
	new->ls_next->ls_prev = new;
	new->ls_prev->ls_next = new;
}

/*
 * ls_elt_t *
 * ls_remque(ls_elt_t *)
 *
 *	Unlink first element in the specified list.
 *
 * Calling/Exit State:
 *
 *	Returns the element's address or 0 if list is empty.
 *	Resets elements pointers to empty list state.
 */
ls_elt_t *
ls_remque(ls_elt_t *p)
{
	ls_elt_t *result = 0;

	if (!LS_ISEMPTY(p)) {
		result = p->ls_next;
		result->ls_prev->ls_next = result->ls_next;
		result->ls_next->ls_prev = result->ls_prev;
		LS_INIT(result);
	}
	return result;
}

/*
 * void
 * ls_remove(ls_elt_t *)
 *	Unlink donated element for list.
 *
 * Calling/Exit State:
 *	Resets elements pointers to empty list state.
 */
void
ls_remove(ls_elt_t *p)
{
	p->ls_prev->ls_next = p->ls_next;
	p->ls_next->ls_prev = p->ls_prev;
	LS_INIT(p);
}
