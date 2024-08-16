/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/queue.c	1.1.3.6"

#include <libthread.h>

/*
 *  _thrq_elt_ins_after(thrq_elt_t *old, thrq_elt_t *new)
 *	This function adds an element after the list element specified
 *	 by old on the list old is on.
 *	The function is called via appropriate macros.
 *	See calling macros for locking requirements.
 *
 *  Parameter/Calling State:
 *
 *	old - Pointer to the current list element.
 *	new - Pointer to the thread to be put on the queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */
void
_thrq_elt_ins_after(thrq_elt_t *old, thrq_elt_t *new)
{
	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(old != (thrq_elt_t *)new);
	ASSERT(old != (thrq_elt_t *)NULL);
	ASSERT(new != (thrq_elt_t *)NULL);

	new->thrq_next = old->thrq_next;
	new->thrq_prev = old;
	new->thrq_next->thrq_prev = new;
	new->thrq_prev->thrq_next = new;
	
}

/*
 *  _thrq_elt_add_end(thrq_elt_t *old, thrq_elt_t *new)
 *	This function adds an element to the list after the list 
 *	element specified by old on the list old is on.
 *	The function is called via appropriate macros.
 *	See calling macros for locking requirements.
 *
 *  Parameter/Calling State:
 *
 *	old - Pointer to the current list element.
 *	new - Pointer to the thread to be put on the queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */
void
_thrq_elt_add_end(thrq_elt_t *old, thrq_elt_t *new)
{
	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(old != (thrq_elt_t *)new);
	ASSERT(old != (thrq_elt_t *)NULL);
	ASSERT(new != (thrq_elt_t *)NULL);

	new->thrq_next = old;
	if (THRQ_ISEMPTY(old)) {
		new->thrq_prev = old;
	} else {
		new->thrq_prev = old->thrq_prev;
	}
	new->thrq_next->thrq_prev = new;
	new->thrq_prev->thrq_next = new;
}

/*
 *  _thrq_prio_ins(thrq_elt_t *qp, thread_desc_t *tp)
 *	This function adds a thread after the list specified
 *	by qp in order of thread priority. For threads of the same
 *	priority, they are put on the queue in order of arrival.
 *
 *  Parameter/Calling State:
 *	On entry, the lock of thread tp and the lock of queue qp
 *	must be held.
 *
 *	qp - Pointer to the current list.
 *	tp - Pointer to the thread to be put on the queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */
void
_thrq_prio_ins(thrq_elt_t *qp, thread_desc_t *tp)
{
	thrq_elt_t *temp;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(qp != (thrq_elt_t *)NULL);
	ASSERT(tp != (thread_desc_t *)NULL);

	if (THRQ_ISEMPTY(qp)) {
		_thrq_add_end(qp, tp);
	} else {
		temp = qp->thrq_prev;
		while (temp != qp && ((thread_desc_t *)temp)->t_pri < tp->t_pri) {
			temp = temp->thrq_prev;
		}
		_thrq_ins_after(temp, tp);
	}
}

/* thrq_elt_t *
 *  _thrq_elt_rem_first(thrq_elt_t *qp)
 *	This function removes the first element from the list specified
 *	by qp.
 *	The function is called via appropriate macros.
 *	See calling macros for locking requirements.
 *
 *  Parameter/Calling State:
 *	On entry, the queue lock of qp must be held.
 *
 *	qp - Pointer to the current list element.
 *
 *  Return Values/Exit State:
 *	Returns a pointer to the first element on the list, if the list is 
 *	is empty NULL is returned.
 */
thrq_elt_t *
_thrq_elt_rem_first(thrq_elt_t *qp)
{
	thrq_elt_t *result = NULL;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(qp != (thrq_elt_t *)NULL);

	if (!THRQ_ISEMPTY(qp)) {
		result = qp->thrq_next;
		if (THRQ_ONLY_MEMBER(result)) {
			THRQ_INIT(qp);
		} else {
			result->thrq_prev->thrq_next = result->thrq_next;
			result->thrq_next->thrq_prev = result->thrq_prev;
		}
		THRQ_INIT(result);
		ASSERT(result->thrq_next == result->thrq_prev);
		ASSERT(result->thrq_next == NULL);
	}
	return(result);
}

/*
 *  _thrq_elt_rem_from_q(thrq_elt_t *target)
 *	This function removes an element from the list on which it 
 *	is currently on.
 *      The function is called via appropriate macros.
 *      See calling macros for locking requirements.
 *
 *  Parameter/Calling State:
 *	On entry, the queue lock of the queue that target is on must be held.
 *
 *	target - Pointer to the element to be removed from the queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */
void
_thrq_elt_rem_from_q(thrq_elt_t *target)
{
	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(!THRQ_ISEMPTY(target));

	if (THRQ_ONLY_MEMBER(target)) {
		THRQ_INIT((target)->thrq_prev);
	} else {
		(target)->thrq_prev->thrq_next = (target)->thrq_next;
		(target)->thrq_next->thrq_prev = (target)->thrq_prev;
	}
	THRQ_INIT(target);
}

/*
 *  _thrq_is_on_q(thrq_elt_t *, thread_desc_t *target)
 *	This searches a queue for the thread specified as the second argument.
 * 	Used for condition variables since cond_broadcast may have moved
 *	all threads to a local queue.
 *
 *  Parameter/Calling State:
 *	On entry, the thread lock of target must be held and the
 *	queue lock of the queue that target is on must be held.
 *
 *	target - Pointer to the thread to be searched for.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */
boolean_t
_thrq_is_on_q(thrq_elt_t *queue, thread_desc_t *target)
{
	thrq_elt_t *temp;

	ASSERT(THR_ISSIGOFF(curthread));

	if (THRQ_ISEMPTY(queue))
		return(0);

        temp = queue->thrq_next;

	while (temp != queue) {
 		if ((thread_desc_t *)temp == target)
			return(1);

		temp = temp->thrq_next;
	}
	return(0);
}
