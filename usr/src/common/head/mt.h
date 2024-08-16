/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MT_H
#define _MT_H

#ident	"@(#)head.usr:mt.h	1.10"
#ident	"$Header: $"

/*
 * mt.h - this file has all of the information that the
 * libnsl subcomponents need to use to be thread-safe.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#include <errno.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

#define MULTI_THREADED (_mt_multi_threaded)
#define FIRST_OR_NO_THREAD \
	((MULTI_THREADED) ? ((* _mt_thr_selfp)() == _mt_first_thread) : 1)

#define THR_SELF() ((MULTI_THREADED) ? (* _mt_thr_selfp)() : 0)

#define THR_MINSTACK() ((MULTI_THREADED) ? (* _mt_thr_minstackp)() : 0)

#define THR_CREATE(stack_addr,stack_size,start_routine,arg,flags,new_thread) \
	   ((MULTI_THREADED) \
	   ? (* _mt_thr_createp) \
	     (stack_addr,stack_size,start_routine,arg,flags,new_thread) \
	   : EINVAL)

#define THR_EXIT(status) ((MULTI_THREADED) ? (* _mt_thr_exitp)(status) : EINVAL)

/* 
 * The following macro sends a signal to the specified thread (if linked
 * with libthread) or to the specified process (otherwise).
 */
#define MT_KILL(id, sig) \
		((MULTI_THREADED) ? (* _mt_thr_killp)(id, sig) : kill(id, sig))

/* 
 * The following macro sends a signal to the calling thread (if linked
 * with libthread) or to the calling process (otherwise).
 */
#define MT_KILL_SELF(sig) \
		((MULTI_THREADED) \
		? (* _mt_thr_killp)((* _mt_thr_selfp)(), sig) \
		: kill(getpid(), sig))

#define MUTEX_T mutex_t
#define MUTEX_INIT(lockp,type,argp) \
	 ((MULTI_THREADED) ? (* _mt_mutex_initp)(lockp,type,argp) : 0)
#define MUTEX_LOCK(lockp) \
	 ((MULTI_THREADED) ? (* _mt_mutex_lockp)(lockp) : 0)
#define MUTEX_TRYLOCK(lockp) \
	 ((MULTI_THREADED) ? (* _mt_mutex_trylockp)(lockp) : 0)
#define MUTEX_UNLOCK(lockp) \
	 ((MULTI_THREADED) ? (* _mt_mutex_unlockp)(lockp) : 0)
#define MUTEX_DESTROY(lockp) \
	 ((MULTI_THREADED) ? (* _mt_mutex_destroyp)(lockp) : 0)

#define RWLOCK_T rwlock_t
#define RWLOCK_INIT(lockp,type,argp) \
	 ((MULTI_THREADED) ? (* _mt_rwlock_initp)(lockp,type,argp) : 0)
#define RW_RDLOCK(lockp) \
	 ((MULTI_THREADED) ? (* _mt_rw_rdlockp)(lockp) : 0)
#define RW_WRLOCK(lockp) \
	 ((MULTI_THREADED) ? (* _mt_rw_wrlockp)(lockp) : 0)
#define RW_UNLOCK(lockp) \
	 ((MULTI_THREADED) ? (* _mt_rw_unlockp)(lockp) : 0)
#define RWLOCK_DESTROY(lockp) \
	 ((MULTI_THREADED) ? (* _mt_rwlock_destroyp)(lockp) : 0)

#define THREAD_KEY_T thread_key_t

#define THR_KEYCREATE(keyp, destructor) \
	((MULTI_THREADED) ? (* _mt_thr_keycreatep)(keyp,destructor) : EINVAL)

#define THR_SETSPECIFIC(key, valuep) \
	((MULTI_THREADED) ? (* _mt_thr_setspecificp)((key), (valuep)) : EINVAL)

#define THR_GETSPECIFIC(key, valuepp) \
	((MULTI_THREADED) ? (* _mt_thr_getspecificp)((key), (valuepp)) : EINVAL)

/*
 * The following two macros are part of a temporary patch required
 * for libnsl and libsocket to prevent the interruption of ioctl()
 * TI_BIND and TI_UNBIND requests, since they cannot be restarted.
 */

#define MT_MASKSIGS(osetp) \
	((MULTI_THREADED) ? _mt_masksigs((osetp)) : 0)

#define MT_UNMASKSIGS(osetp) \
	((MULTI_THREADED) ? _mt_unmasksigs((osetp)) : 0)

/* End of patch section */

#if defined(__STDC__)

extern thread_t (* _mt_thr_selfp)(void);
extern size_t (* _mt_thr_minstackp)(void);
extern int (* _mt_thr_createp)(void*, size_t, void *(*)(void *),
			       void *, long, thread_t *);
extern int (* _mt_thr_exitp)(void *);
extern int (* _mt_thr_killp)(thread_t, int);
extern int (* _mt_mutex_initp)(mutex_t *, int, void *);
extern int (* _mt_mutex_lockp)(mutex_t *);
extern int (* _mt_mutex_trylockp)(mutex_t *);
extern int (* _mt_mutex_unlockp)(mutex_t *);
extern int (* _mt_mutex_destroyp)(mutex_t *);
extern int (* _mt_rwlock_initp)(rwlock_t *, int, void *);
extern int (* _mt_rw_rdlockp)(rwlock_t *);
extern int (* _mt_rw_wrlockp)(rwlock_t *);
extern int (* _mt_rw_unlockp)(rwlock_t *);
extern int (* _mt_rwlock_destroyp)(rwlock_t *);
/* This function is supplied for support of thread-specific data. */
extern void *_mt_get_thr_specific_storage(thread_key_t, size_t);
/* if everyone uses _mt_get_thr_specific_storage() these should not be here. */
extern int (* _mt_thr_keycreatep)
     (thread_key_t *key, void (*destructor)(void *value));
extern int (* _mt_thr_setspecificp)(thread_key_t key, void *value);
extern int (* _mt_thr_getspecificp)(thread_key_t key, void **value);

/* The following three lines are part of the temporary patch */
extern int _mt_masksigs(sigset_t *);
extern int _mt_unmasksigs(sigset_t *);
extern int (* _mt_sigprocmaskp)(int, const sigset_t *, sigset_t *);
/* End of patch section */

#else /* !__STDC__ */

extern thread_t (* _mt_thr_selfp)();
extern size_t (* _mt_thr_minstackp)();
extern int (* _mt_thr_createp)();
extern int (* _mt_thr_exitp)();
extern int (* _mt_thr_killp)();
extern int (* _mt_mutex_initp)();
extern int (* _mt_mutex_lockp)();
extern int (* _mt_mutex_trylockp)();
extern int (* _mt_mutex_unlockp)();
extern int (* _mt_mutex_destroyp)();
extern int (* _mt_rwlock_initp)();
extern int (* _mt_rw_rdlockp)();
extern int (* _mt_rw_wrlockp)();
extern int (* _mt_rw_unlockp)();
extern int (* _mt_rwlock_destroyp)();
extern int (* _mt_thr_keycreatep)();
extern int (* _mt_thr_setspecificp)();
extern int (* _mt_thr_getspecificp)();
/* This function is supplied for support of thread-specific data. */
extern void *_mt_get_thr_specific_storage();
/* The following three lines are part of the temporary patch */
extern int _mt_masksigs();
extern int _mt_unmasksigs();
extern int (* _mt_sigprocmaskp)();
/* End of patch section */

#endif     /* defined(__STDC__) */

/* This flag is checked by MULTI_THREADED macro. Simple boolean */
extern int _mt_multi_threaded;

/* This variable is where we save the thread id of the first thread. */
extern thread_t	_mt_first_thread;

#else /* !_REENTRANT */

#define MULTI_THREADED                  (0)
#define FIRST_OR_NO_THREAD		(1)
#define THR_SELF()                      (0)
#define THR_MINSTACK()                  (0)
#define THR_CREATE(a,b,c,d,e,f)		(EINVAL)
#define THR_EXIT(status)		(EINVAL)
#define MT_KILL(id, sig) 		(kill(id, sig))
#define MT_KILL_SELF(sig) 		(kill(getpid(), sig))
#define MUTEX_INIT(lockp,type,argp)	(0)
#define MUTEX_LOCK(lockp)		(0)
#define MUTEX_TRYLOCK(lockp)		(0)
#define MUTEX_UNLOCK(lockp)		(0)
#define MUTEX_DESTROY(lockp)		(0)
#define THR_KEYCREATE(keyp,destructor)  (EINVAL)
#define THR_SETSPECIFIC(key,key_tbl)    (EINVAL)
#define THR_GETSPECIFIC(key,key_tblp)	(EINVAL)
#define RWLOCK_INIT(lockp,type,argp)	(0)
#define RW_RDLOCK(lockp)		(0)
#define RW_WRLOCK(lockp)		(0)
#define RW_UNLOCK(lockp)		(0)
#define RWLOCK_DESTROY(lockp)		(0)
#define MT_MASKSIGS(osetp) 		(0)
#define MT_UNMASKSIGS(osetp)		(0)

#endif /* _REENTRANT */

#if defined(__cplusplus)
}
#endif

#endif /* _MT_H */
