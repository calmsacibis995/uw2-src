/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/tsd/tsd.c	1.7"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/list.h>
#include <synch.h>
#include <thread.h>
#include <trace.h>
#include <memory.h>
#include <libthread.h>

/*
 * This file implements thread-specific data.  The implementation is based
 * on a global array of keys, and per-thread arrays of bindings.  The arrays
 * are grown on demand.  Functions:  thr_keycreate(), thr_keydelete(),
 * thr_setspecific(), thr_getspecific(), and _thr_key_exit().
 */

/*
 * The set of existing keys is global to a process, and is represented by the
 * _key_set structure.  It is statically initialized to describe the empty set.
 *
 * When the _key_set is nonempty, it points to an array of _key_rep structures,
 * which represent individual keys.  When creating a new key, and the table has
 * no free elements, it is reallocated, increasing the number of elements by
 * the constant number KEYCHUNK.  The size of the table is never decreased.
 * KEYCHUNK is chosen to be a power of 2 for fast modulus operations.  KEYMASK
 * is the constant used to perform the modulus.  The user-visible
 * representation of a key, i.e., the value returned by thr_key_create(), is an
 * index into the table underlying the _key_set.  This supports easy validation
 * and constant-time access.
 *
 * Each thread descriptor contains a _thr_binding structure, declared
 * elsewhere.  It holds a pointer to an array of values bound to keys,
 * and the number of elements in the array.  The pointer is initially NULL,
 * and the counter 0.
 */

/*
 * ks_nelements - the cardinality of the set; always a multiple of KEYCHUNK.
 * ks_ninuse - the number of elements of the set that are in use.
 * ks_nfree - the number of elements in the set that are not in use.
 *      (The counters are not strictly necessary, but provide redundancy that
 *      is useful for debugging.)
 * ks_keyrep - pointer to the underlying table of key_reps.
 * ks_freelist - free list of key_reps.
 * ks_setrwlock - read-write lock protecting updates of _key_set and
 *      reallocations of the underlying table.
 * ks_mutex - given a key (index) k, updates of the denoted key_rep are
 *      protected by _key_set.ks_mutex[k % KEYCHUNK].
 * Note that ks_setrwlock must not be taken while holding ks_mutex, and that
 * holding the rwlock for writing implies that the mutex need not be held.
 */

#define KEYCHUNK	8	/* Must be a power of 2 */
#define KEYMASK		(KEYCHUNK - 1)
#define KEY_HASH(k)     (((long)(k)) & KEYMASK)

struct {
	unsigned int ks_nelements;
	unsigned int ks_ninuse;
	unsigned int ks_nfree;
	struct _key_rep *ks_keyrep;
	struct _key_rep *ks_freelist;
	rwlock_t	ks_setrwlock;
	mutex_t ks_mutex[KEYCHUNK];
} _key_set;

/*
 * kr_inuse - distinguishes busy from free array elements, and discriminates
 *      the union.
 * kr_refcnt - defined iff kr_inuse; the number of bindings to the key.
 * kr_destructor - defined iff kr_inuse; the destructor for bound values,
 *      possibly NULL.
 * kr_nextfree - defined iff !kr_inuse; next element in the list rooted at
 *      _key_set.ks_freelist.
 */

struct _key_rep {
	boolean_t       kr_inuse;
	union {
		struct {
			unsigned int krun_refcnt;
			void (*krun_destructor)(void *);
		} s;
		struct _key_rep *krun_nextfree;
	} un;
};
#define kr_refcnt       un.s.krun_refcnt
#define kr_destructor   un.s.krun_destructor
#define kr_nextfree     un.krun_nextfree

#ifdef DEBUG
#define TSD_DEBUG(str, printargs) ((void)(getenv(str) && printf printargs))
void ks_keyrep_print(void);
void _key_rep_print(struct _key_rep *);
void _thr_binding_print(struct _thr_binding *);
#else
#define TSD_DEBUG(str, printargs)
#endif

STATIC void _thr_keyrele(thread_key_t key);

/*
 * int
 * thr_keycreate(thread_key_t *key, void (*destructor)(void *))
 *      This function creates and returns a process-wide identifier key.
 *
 * Parameter/Calling State:
 *      "key" is an out parameter.  "destructor" is an in-parameter that
 *      can be called at thread exit to deallocate resourcess associated
 *      with any non-NULL bindings.  On entry, no locks are held.  During
 *      processing, signal handlers are disabled by _thr_sigoff(), the key
 *      set read-write lock is taken for writing, and the mutex lock
 *      associated with the hash bucket is acquired.
 *
 * Return/Exit State:
 *      All signals are enabled by _thr_sigon(), and no locks are held.
 *      Returns an errno.
 */
int
thr_keycreate(thread_key_t *key, void (*destructor)(void *))
{
	struct _key_rep *krp;
	thread_desc_t *curtp = curthread;
	int     error = 0;

	/*
	 * POSIX specifies EAGAIN returns for name space exhaustion, but
	 * that never happens in this implementation.  Note that we do not
	 * take the hash mutex here; holding the rwlock for writing is enough.
	 */

	_thr_sigoff(curtp);
	(void)_THR_RW_WRLOCK(&_key_set.ks_setrwlock);
	ASSERT((_key_set.ks_nelements == 0) == (_key_set.ks_keyrep == NULL));
	ASSERT(_key_set.ks_nfree + _key_set.ks_ninuse == _key_set.ks_nelements);
	if (!_key_set.ks_nfree) {
		struct _key_rep *new_table;
		struct _key_rep *end_table;

		/* Empty free list, and possibly empty set. */

#ifdef DEBUG
		if (getenv("TSDREALLOCKEYREP")) {
			printf("realloc keyrep\n");
			ks_keyrep_print();
		}
#endif
		new_table = (struct _key_rep *)realloc(_key_set.ks_keyrep,
					  (_key_set.ks_nelements + KEYCHUNK) *
					    sizeof(*new_table));
		if (new_table) {

			/* Chain new chunks into _key_set.ks_freelist. */

			for (krp = new_table + _key_set.ks_nelements,
			     end_table = krp + KEYCHUNK;
			     krp < end_table;
			     krp++) {
				krp->kr_inuse = B_FALSE;
				krp->kr_nextfree = krp + 1;
			}
			krp -= 1;
			krp->kr_nextfree = NULL;
			_key_set.ks_freelist =
				new_table + _key_set.ks_nelements;
			_key_set.ks_nfree += KEYCHUNK;
			_key_set.ks_nelements += KEYCHUNK;
			_key_set.ks_keyrep = new_table;
		} else {
			error = ENOMEM;
		}
#ifdef DEBUG
		if (getenv("TSDREALLOCKEYREP")) {
			printf("realloc keyrep\n");
			ks_keyrep_print();
		}
#endif
	}
	if (!error) {

		/*
		 * We have at least one free element; return index of first
		 * free.
		 */

		krp = _key_set.ks_freelist;
		_key_set.ks_freelist = krp->kr_nextfree;
		_key_set.ks_ninuse += 1;
		_key_set.ks_nfree -= 1;
		krp->kr_inuse = B_TRUE;
		krp->kr_refcnt = 0;
		krp->kr_destructor = destructor;
		TSD_DEBUG("TSDKEYCREATE", ("create key %d krp 0x%x\n",
			  *key, krp));
	}
	ASSERT((_key_set.ks_nelements == 0) == (_key_set.ks_keyrep == NULL));
	ASSERT(_key_set.ks_nfree + _key_set.ks_ninuse == _key_set.ks_nelements);
	if (!error) {
		*key = krp - _key_set.ks_keyrep;
	}
	(void)_THR_RW_UNLOCK(&_key_set.ks_setrwlock);
	_thr_sigon(curtp);
	TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_KEYCREATE, TR_CALL_ONLY,
	   key, destructor, error);
	return error;
}

/*
 * int
 * thr_keydelete(thread_key_t key)
 *      Deletes a key identifier.
 *
 * Calling/Parameter State:
 *      "key" must be a key previously returned from thr_keycreate(). On
 *      entry, no locks are held.  During processing,  signal handlers are
 *      disabled by_thr_sigoff, the key set read/write lock is taken for
 *      writing, and the mutex lock associated with the hash bucket is
 *      acquired.
 *
 * Return/Exit State:
 *      On exit, all signals are enabled by _thr_sigon(), and no locks are
 *      held.  Returns an errno.
 */
int
thr_keydelete(thread_key_t key)
{
	struct _key_rep *krp;
	thread_desc_t *curtp = curthread;
	int     error = 0;
	int     hash;

	TSD_DEBUG("TSDKEYDELETE", ("thr_keydelete key %d\n", key));
	_thr_sigoff(curtp);
	(void)_THR_RW_WRLOCK(&_key_set.ks_setrwlock);
	ASSERT((_key_set.ks_nelements == 0) == (_key_set.ks_keyrep == NULL));
	ASSERT(_key_set.ks_nfree + _key_set.ks_ninuse == _key_set.ks_nelements);
	if (key >= _key_set.ks_nelements) {
		error = EINVAL;
	} else {
		hash = KEY_HASH(key);
		_THR_MUTEX_LOCK(_key_set.ks_mutex + hash);
		krp = _key_set.ks_keyrep + key;
		if (krp->kr_inuse == B_FALSE) {
			error = EINVAL;
		} else if (krp->kr_refcnt) {
			error = EBUSY;
#ifdef DEBUG
			if (getenv("TSDBUSY")) {
				printf("TSDBUSY key %d\n", key);
				ks_keyrep_print();
			}
#endif
		} else {

			/* Put key on free list. I*/

			TSD_DEBUG("TSDKEYFREE", ("free key %d\n", key));
			krp->kr_inuse = B_FALSE;
			krp->kr_nextfree = _key_set.ks_freelist;
			_key_set.ks_freelist = krp;
			_key_set.ks_ninuse -= 1;
			_key_set.ks_nfree += 1;
		}
		_THR_MUTEX_UNLOCK(_key_set.ks_mutex + hash);
	}
	ASSERT((_key_set.ks_nelements == 0) == (_key_set.ks_keyrep == NULL));
	ASSERT(_key_set.ks_nfree + _key_set.ks_ninuse == _key_set.ks_nelements);
	(void)_THR_RW_UNLOCK(&_key_set.ks_setrwlock);
	_thr_sigon(curtp);
	TSD_DEBUG("TSDKEYDELETE", ("thr_keydelete key %d error %d\n",
		  key, error));
	TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_KEYDELETE, TR_CALL_ONLY,
	   key, error);
	return error;
}

/*
 * int
 * thr_setspecific(thread_key_t key, void *value)
 *      Binds a thread-specific value with a key obtained via a previous call
 *      to thr_keycreate().
 *
 * Calling/Parameter State:
 *      "key" must be a key previously returned from thr_keycreate().  "value"
 *      is specific to the application.  On entry, no locks are held.  During
 *      processing,  signal handlers are disabled by _thr_sigoff, the key set
 *      read/write lock is taken for reading, and the mutex lock associated
 *      with the hash bucket is acquired.
 *
 * Return/Exit State:
 *      On exit, all signals are enabled by _thr_sigon(), and no locks are
 *      held.  Returns an errno.
 */
int
thr_setspecific(thread_key_t key, void *value)
{
	thread_desc_t *curtp = curthread;
	struct _thr_binding *bindp;
	struct _key_rep *krp;
	void    **valp;
	int     error = 0;
	int     hash;
	int	nkeysneeded;

	TSD_DEBUG("TSDSETSPECIFIC", ("setspecific key %d value 0x%x\n",
		  key, value));

	/*
	 * We make this check to avoid malloc-ing ourselves into oblivion due
	 * to a garbage argument.  It is safe to check without taking the lock
	 * because ks_nelements never decreases.  Note that we don't check
	 * kr_inuse, because we don't want to take the rwlock here.
	 */

	if (key >= _key_set.ks_nelements) {
		TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_SETSPECIFIC, 
		   TR_CALL_ONLY, key, value, EINVAL);
		return EINVAL;
	}
	_thr_sigoff(curtp);
	bindp = &curtp->t_binding;
	ASSERT(!bindp->tb_count || bindp->tb_data);
	if (key >= bindp->tb_count) {

		/*
		 * No space in (possibly empty)  binding array.  Note that we
		 * can be adding more than a single chunk.  KEYCHUNK and
		 * following terms below deal with rounding up to a multiple
		 * of KEYCHUNK.
		 */

		nkeysneeded = key - bindp->tb_count + KEYCHUNK -
				(key & KEYMASK);
#ifdef DEBUG
		if (getenv("TSDREALLOCTB")) {
			printf("realloc bindings\n");
			_thr_binding_print(bindp);
		}
#endif
		valp = (void **)realloc(bindp->tb_data,
			    (bindp->tb_count + nkeysneeded) * sizeof(*valp));
		if (valp) {
			memset(valp + bindp->tb_count, '\0',
			       sizeof(*valp) * nkeysneeded);
			bindp->tb_count += nkeysneeded;
			bindp->tb_data = valp;
		} else {
			error = ENOMEM;
		}
#ifdef DEBUG
		if (getenv("TSDREALLOCTB")) {
			printf("realloc bindings\n");
			_thr_binding_print(bindp);
		}
#endif
	}
	if (!error) {

		/*
		 * There is space in the binding array; take global read lock
		 * to prevent table from being reallocated under us.
		 */

		hash = KEY_HASH(key);
		(void)_THR_RW_RDLOCK(&_key_set.ks_setrwlock);
		_THR_MUTEX_LOCK(_key_set.ks_mutex + hash);
		if (key < _key_set.ks_nelements &&
		    (krp = _key_set.ks_keyrep + key)->kr_inuse) {
			TSD_DEBUG("TSDSETSPECIFIC",
	  ("setspecific krp 0x%x &bindp->tb_data[key] 0x%x old binding 0x%x\n",
			   krp, &bindp->tb_data[key], bindp->tb_data[key]));
			if (value) {
				if (!bindp->tb_data[key]) {

					/* New reference. */

					krp->kr_refcnt += 1;
				}
			} else if (bindp->tb_data[key]) {

				/* Destroy old reference. */

				krp->kr_refcnt -= 1;
			}
			bindp->tb_data[key] = value;
			TSD_DEBUG("TSDSETSPECIFIC",
	  ("setspecific krp 0x%x &bindp->tb_data[key] 0x%x new binding 0x%x\n",
			   krp, &bindp->tb_data[key], bindp->tb_data[key]));
		} else {

			/* Key out of range or unused. */

			error = EINVAL;
		}
		_THR_MUTEX_UNLOCK(_key_set.ks_mutex + hash);
		(void)_THR_RW_UNLOCK(&_key_set.ks_setrwlock);
	}
	_thr_sigon(curtp);
	TRACE3(curtp, TR_CAT_THREAD, TR_EV_THR_SETSPECIFIC, 
	   TR_CALL_ONLY, key, value, error);
	return error;
}

/*
 * int
 * thr_getspecific(thread_key_t key, void **retval)
 *      Obtains the value currently bound to the specified key by the
 *      calling thread.
 *
 * Calling/Parameter State:
 *      "key" must be a key previously returned from thr_keycreate(). On
 *      entry, no locks are held.  During processing,  signal handlers are
 *      disabled by_thr_sigoff.  No locks are taken.
 *
 * Return/Exit State:
 *      On exit, all signals are enabled by _thr_sigon(), and no locks are
 *      held.  On success, returns 0 and places the bound value, which may 
 *	be NULL, or NULL if no binding exists, in the address indicated 
 *	by retval.
 */
int
thr_getspecific(thread_key_t key, void **retval)
{
	thread_desc_t *curtp = curthread;
	struct _thr_binding *bindp;
	void    *res = NULL;

	if (key >= _key_set.ks_nelements) {
		TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_GETSPECIFIC,
		   TR_CALL_ONLY, key, EINVAL);
		return EINVAL;
	}
	/*
	 * We disable signals here to avoid having a signal handler change
	 * bindings under us.
	 */
	_thr_sigoff(curtp);
	bindp = &curtp->t_binding;
	ASSERT(!bindp->tb_count || bindp->tb_data);
	bindp = &curtp->t_binding;
	if (key < bindp->tb_count) {
		res = bindp->tb_data[key];
	}
	_thr_sigon(curtp);
	TRACE2(curtp, TR_CAT_THREAD, TR_EV_THR_GETSPECIFIC, TR_CALL_ONLY, 
	   key, 0);
	*retval = res;
	return 0;
}

/*
 * boolean_t
 * _thr_key_exit(thread_desc_t *curtp)
 *      Calls the destructors associated with the bindings of curtp, which
 *      must be the current thread.
 *
 * Calling/Parameter State:
 *      curtp must have a non-NULL tb_data pointer.  During processing, the
 *	key set read/write lock is taken for reading, and the mutex lock
 *      associated with the hash bucket is acquired for each key for which
 *      there is a binding that has a destructor.
 *
 * Return/Exit State:
 *      On exit, no locks are held.
 */
boolean_t
_thr_key_exit(thread_desc_t *curtp)
{
	unsigned int oldcount;
	void    **data;		/* local ptr to binding array. */
	int     bnx;		/* index in binding array. */
	struct _thr_binding *bindp;
	void    (*destructor)(void *);
	boolean_t res = B_TRUE;

	/* Call destructors for current bindings until no progress. */

	bindp = &curtp->t_binding;
	ASSERT(bindp->tb_data);
	if (bindp->tb_count) {
		do {

			/*
			 * We save current bindings locally and wipe them out
			 * of the thread context to be able to detect when we
			 * are making progress on reducing their number.  We
			 * continue in this loop while we progress in doings
			 * so.  The inner loop scans each "generation" of
			 * bindings and calls destructors when necessary.
			 */

			oldcount = bindp->tb_count;
			data = bindp->tb_data;
			bindp->tb_count = 0;
			bindp->tb_data = NULL;

			/* Scan current bindings and call destructors. */

			for (bnx = 0; bnx < oldcount; bnx++) {
				if (!data[bnx]) {
					continue;
				}
				(void)_THR_RW_RDLOCK(&_key_set.ks_setrwlock);

				/*
				 * Destructor cannot change on the fly.  We
				 * release the lock to avoid deadlock with
				 * destructors that call into TSD functions.
				 */

				destructor =
				  _key_set.ks_keyrep[bnx].kr_destructor;
				_thr_keyrele((thread_key_t)bnx);
				(void)_THR_RW_UNLOCK(&_key_set.ks_setrwlock);
				 if (destructor) {
					(*destructor)(data[bnx]);
				}
			}
			free(data);
		} while (bindp->tb_count && oldcount > bindp->tb_count);
		if (bindp->tb_count) {
			res = B_FALSE;
		}
	}

	/* Clean up even if we abandoned calling destructors. */

	if (bindp->tb_data) {
		free(bindp->tb_data);
	}
	return res;
}

/*
 * void
 * _thr_keyrele(thread_key_t key)
 *      Give up a reference to a key.
 *
 * Calling/Parameter State:
 *	"key" must be a valid, used key with a non-zero reference count.
 *
 *      On entry, the key set read-write lock must be held for reading.  
 *
 *	During processing, the mutex lock associated with the hash
 *      bucket is acquired.
 *
 * Return/Exit State:
 *      On exit, the key set read-write lock is held for reading.  
 *	Returns no value.
 */
STATIC void
_thr_keyrele(thread_key_t key)
{
	struct _key_rep *krp;
	int     hash;

	/*
	 * Take global read lock to prevent table from being reallocated under
	 * us.
	 */

	hash = KEY_HASH(key);
	_THR_MUTEX_LOCK(_key_set.ks_mutex + hash);
	krp = _key_set.ks_keyrep + key;
	ASSERT(key < _key_set.ks_nelements);
	ASSERT(krp->kr_inuse);
	ASSERT(krp->kr_refcnt);
	krp->kr_refcnt -= 1;
	_THR_MUTEX_UNLOCK(_key_set.ks_mutex + hash);
}

void
_thr_binding_print(struct _thr_binding *bindp)
{
	unsigned	inx;

	printf("nbindings %d\tdata 0x%x\n", bindp->tb_count, bindp->tb_data);
	for (inx = 0; inx < bindp->tb_count; inx++) {
		printf("%d\t0x%x", inx, bindp->tb_data[inx]);
	}
}

#ifdef DEBUG
void
ks_keyrep_print(void)
{
	int	inx;

	printf("%d key_reps\n\t", _key_set.ks_nelements);
	for (inx = 0; inx < _key_set.ks_nelements; inx++) {
		printf("%d\t", inx);
		_key_rep_print(_key_set.ks_keyrep + inx);
		fflush(stdout);
	}
}

void
_key_rep_print(struct _key_rep *krp)
{
	if (krp->kr_inuse == B_TRUE) {
		printf("busy\tref count %d\tdestructor 0x%x\n",
		       krp->kr_refcnt, krp->kr_destructor);
	} else {
		printf("free\tnext 0x%x\n", krp->kr_nextfree);
	}
}
#endif
