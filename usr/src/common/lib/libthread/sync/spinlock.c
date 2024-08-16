/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/sync/spinlock.c	1.4"

#include "libthread.h"
#include <trace.h>

/*
 * int _spin_init(spin_t *sp_lock, void *arg)
 * 	Initialize a spin lock object to the unlocked state.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 *	second argument is not currently used
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	0: The lock is initialized to the unlocked state.
 *
 *	On exit no library locks are held.
 */
/* ARGSUSED */
int
_spin_init(spin_t *sp_lock, void *arg)
{
	TRACE3(0, TR_CAT_SPIN, TR_EV_SPINIT, TR_CALL_ONLY, sp_lock, arg, 0);
	_lock_clear(&(sp_lock->m_lmutex.lock));
	return(0);
}

/*
 * int _spin_destroy(spin_t *sp_lock)
 *	Destroys a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	0: The lock was not held and is not changed.
 *	EBUSY: The lock was held by another thread.
 *
 *	On exit no library locks are held.
 */

int
_spin_destroy(spin_t *sp_lock)
{
        if (sp_lock->m_lmutex.lock == 0) {  /* no lock holders */
		TRACE2(0, TR_CAT_SPIN, TR_EV_SPDESTROY, TR_CALL_ONLY,
		   sp_lock, 0);
		return(0);
	}
	TRACE2(0, TR_CAT_SPIN, TR_EV_SPDESTROY, TR_CALL_ONLY, sp_lock, EBUSY);
	return(EBUSY);
}

/*
 * void _spin_unlock(spin_t *sp_lock)
 *	Release a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	None: The lock field is atomically cleared.
 *
 *	On exit no library locks are held.
 */

void
_spin_unlock(spin_t *sp_lock)
{
        _lock_clear(&(sp_lock->m_lmutex.lock));
	TRACE1(0, TR_CAT_SPIN, TR_EV_SPUNLOCK, TR_CALL_ONLY, sp_lock);
	/*
	 * WARNING: assumes STORE-ORDERED memory
	 */

}

/*
 * void _spin_lock(spin_t *sp_lock)
 * 	Acquire a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	None: The lock set to the locked state.
 *
 *	On exit no library locks are held.
 */

void
_spin_lock(spin_t *sp_lock)
{
#ifdef TRACE
	int waited_for_lock = 0;
#endif /* TRACE */

	TRACE1(0, TR_CAT_SPIN, TR_EV_SPLOCK, TR_CALL_FIRST, sp_lock);
        while (_lock_try((_simplelock_t *)(&(sp_lock->m_lmutex.lock))) == 0) {
                /*
                 * _lock_try does a memory bus locking test and set operation.
                 * Loop testing the field to assure that you are spinning
                 * in the local cache of the processor. The cache coherence
                 * protocol will change the value when _spin_unlock resets it.
                 * At this point try to acquire the lock with _lock_try.
                */
#ifdef TRACE
		waited_for_lock = 1;
#endif /* TRACE */

                while (sp_lock->m_lmutex.lock)
                        ;
        }

#ifdef TRACE
	TRACE1(0, TR_CAT_SPIN, TR_EV_SPLOCK, TR_CALL_SECOND, waited_for_lock);
#endif /* TRACE */
}

/*
 * int _spin_trylock(spin_t *sp_lock, void *arg)
 * 	Make a single attempt to acquire a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	0: The lock set to the locked state.
 *	EBUSY: The lock held by another thread.
 *
 *	On exit no library locks are held.
 */

int
_spin_trylock(spin_t *sp_lock)
{
        if (_lock_try((_simplelock_t *)(&(sp_lock->m_lmutex.lock))) != 0) {
		TRACE2(0, TR_CAT_SPIN, TR_EV_SPTRYLOCK, TR_CALL_ONLY,
		   sp_lock, 0);
		return(0);
	}
	TRACE2(0, TR_CAT_SPIN, TR_EV_SPTRYLOCK, TR_CALL_ONLY, sp_lock, EBUSY);
	return EBUSY;
}



/*
 * The following routines provide the same functionality as the above
 * external interfaces but do not record trace information.  They are
 * provided for internal use by the library and must be kept in sync
 * with the above interfaces, e.g., _thr_notrace_spin_init must be identical
 * to _spin_init except for calls to TRACE.
 */

#ifdef TRACE

/*
 * int _thr_notrace_spin_init(spin_t *sp_lock, void *arg)
 * 	Initialize a spin lock object to the unlocked state.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 *	second argument is not currently used
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	0: The lock is initialized to the unlocked state.
 *
 *	On exit no library locks are held.
 */
/* ARGSUSED */
int
_thr_notrace_spin_init(spin_t *sp_lock, void *arg)
{
	_lock_clear(&(sp_lock->m_lmutex.lock));
	return(0);
}

/*
 * int _thr_notrace_spin_destroy(spin_t *sp_lock)
 *	Destroys a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	0: The lock was not held and is not changed.
 *	EBUSY: The lock was held by another thread.
 *
 *	On exit no library locks are held.
 */

int
_thr_notrace_spin_destroy(spin_t *sp_lock)
{
        if (sp_lock->m_lmutex.lock == 0) {  /* no lock holders */
		return(0);
	}
	return(EBUSY);
}

/*
 * void _thr_notrace_spin_unlock(spin_t *sp_lock)
 *	Release a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	None: The lock field is atomically cleared.
 *
 *	On exit no library locks are held.
 */

void
_thr_notrace_spin_unlock(spin_t *sp_lock)
{
        _lock_clear(&(sp_lock->m_lmutex.lock));
	/*
	 * WARNING: assumes STORE-ORDERED memory
	 */

}

/*
 * void _thr_notrace_spin_lock(spin_t *sp_lock)
 * 	Acquire a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	None: The lock set to the locked state.
 *
 *	On exit no library locks are held.
 */

void
_thr_notrace_spin_lock(spin_t *sp_lock)
{

        while (_lock_try((_simplelock_t *)(&(sp_lock->m_lmutex.lock))) == 0) {
                /*
                 * _lock_try does a memory bus locking test and set operation.
                 * Loop testing the field to assure that you are spinning
                 * in the local cache of the processor. The cache coherence
                 * protocol will change the value when _spin_unlock resets it.
                 * At this point try to acquire the lock with _lock_try.
                */

                while (sp_lock->m_lmutex.lock)
                        ;
        }

}

/*
 * int _thr_notrace_spin_trylock(spin_t *sp_lock, void *arg)
 * 	Make a single attempt to acquire a spin lock.
 *
 * Parameter/Calling State:
 *	On entry no locks are held.
 *
 *	first argument is a pointer to a spin lock
 * 
 *	During processing no locks are acquired.
 *
 * Return Values/Exit State:
 *	0: The lock set to the locked state.
 *	EBUSY: The lock held by another thread.
 *
 *	On exit no library locks are held.
 */

int
_thr_notrace_spin_trylock(spin_t *sp_lock)
{
        if (_lock_try((_simplelock_t *)(&(sp_lock->m_lmutex.lock))) != 0) {
		return(0);
	}
	return EBUSY;
}

#endif /* TRACE */
