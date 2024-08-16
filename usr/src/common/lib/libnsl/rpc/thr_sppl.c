/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/thr_sppl.c	1.2.2.1"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * thr_sppl.c
 *
 * This file defines several fuctions that should be provided
 * by the thread library in later releases.
 *
 * int thr_keycreate(thread_key_t *key, void (*destructor)(void *value));
 * int thr_keyisnull(thread_key_t key);
 * int thr_setspecific(thread_key_t key, void *value);
 * int thr_getspecific(thread_key_t key, void *value);
 */

#ifdef _REENTRANT
#ifndef NOTYET

#include <stdlib.h>
#include <thread.h>
#include <synch.h>
#include "trace.h"

#pragma weak thr_self
#pragma weak mutex_lock
#pragma weak mutex_unlock

typedef int thread_key_t;
typedef void *destruct_func();

extern mutex_t __thr_sp_lock;

static int keys = 0;

static struct thr_pool {
	thread_t thrid;
	void *value;
	struct thr_pool *next;
} *pool_head;

int
thr_keycreate(key, destructor)
thread_key_t *key;
destruct_func destructor;
{
	*key = (thread_key_t) keys++;
	/*
	 * Do nothing on destructors.
	 */
	return(0);
}

int
thr_key_isnull(key)
thread_key_t key;
{
	if ( 0 <= key && key < keys)
		return(1);
	else
		return(0);
}

int
thr_setspecific(key, value)
thread_key_t key;
void *value;
{
	thread_t myid;
	struct thr_pool *p, *q;

	mutex_lock(&__thr_sp_lock);
	myid = thr_self();
	if (pool_head == NULL)
		pool_head = (struct thr_pool *) calloc(1, sizeof(*pool_head));
	for (p = pool_head; p != NULL; p = p->next) {
		if (p->thrid == myid)
			break;
	}
	if (p) {
		p->value = value;
	} else {
		q = (struct thr_pool *) calloc(1, sizeof(*pool_head));
		q->thrid = myid;
		q->value = value;
		q->next = pool_head;
		pool_head = q;
	}
	mutex_unlock(&__thr_sp_lock);
	return(0);
}

int
thr_getspecific(key, value)
thread_key_t key;
void **value;
{
	thread_t myid;
	struct thr_pool *p;

	mutex_lock(&__thr_sp_lock);
	myid = thr_self();
	for (p = pool_head; p != 0; p = p->next) {
		if (p->thrid == myid)
			break;
	}
	if (p == NULL)
		*value = NULL;
	else
		*value = p->value;
	mutex_unlock(&__thr_sp_lock);
	return(0);
}

#endif /* NOTYET */
#endif /* _REENTRANT */
