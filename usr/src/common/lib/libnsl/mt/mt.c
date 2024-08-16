/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/mt/mt.c	1.2.2.8"
#ident	"$Header: $"

/*
 * mt.c
 * This file includes the _init() function for initializing libnsl before
 * main() is called and also includes the function
 * _mt_get_thr_specific_storage() that subcomponent thread-specific-data
 * functions can use for initialization.
 */

#ifdef _REENTRANT

#include <mt.h>
#include <stdlib.h>

#define dlopen	_dlopen
#define dlsym	_dlsym
#define dlclose	_dlclose
#include <dlfcn.h>

/*
 * Part of patch to mask SIGWAITING and SIGLWP
 */
#include <signal.h>

extern int (*_sigprocmask)(int, const sigset_t *, sigset_t *);

/* End of patch section */

#endif /* _REENTRANT */

extern void _cr1_init();
extern void _des_init();
extern void _netdir_init();
extern void _netselect_init();
extern void _nsl_init();
extern void _rx_init();
extern void _rpc_init();
extern void _yp_init();

#ifdef _REENTRANT

/* Define multi-threading indicator variables */
int      _mt_multi_threaded;
thread_t _mt_first_thread;

/* Define storage for pointers to libthread functions */
thread_t (* _mt_thr_selfp)(void);
size_t (* _mt_thr_minstackp)(void);
int (* _mt_thr_createp)(void *, size_t, void *(*)(void *),
		void *, long, thread_t *);
int (* _mt_thr_exitp)(void *);
int (* _mt_thr_killp)(thread_t, int);
int (* _mt_mutex_initp)(mutex_t *, int, void *);
int (* _mt_mutex_lockp)(mutex_t *);
int (* _mt_mutex_trylockp)(mutex_t *);
int (* _mt_mutex_unlockp)(mutex_t *);
int (* _mt_mutex_destroyp)(mutex_t *);
int (* _mt_rwlock_initp)(rwlock_t *, int, void *);
int (* _mt_rw_rdlockp)(rwlock_t *);
int (* _mt_rw_wrlockp)(rwlock_t *);
int (* _mt_rw_unlockp)(rwlock_t *);
int (* _mt_rwlock_destroyp)(rwlock_t *);
int (* _mt_thr_keycreatep)(thread_key_t *key, void (*destructor)(void *value));
int (* _mt_thr_setspecificp)(thread_key_t key, void *value);
int (* _mt_thr_getspecificp)(thread_key_t key, void **value);
int (* _mt_sigprocmaskp)(int, const sigset_t *, sigset_t *);

#endif /* _REENTRANT */

/*
 * This function will be called by the dynamic linker before
 * user main programs get control of execution.
 * It should call the init function of each subcomponent of libnsl.
 */

_init()
{
#ifdef _REENTRANT
void *handle;

       if ((handle = _dlopen(NULL, RTLD_LAZY))
	   && (_mt_thr_selfp = (thread_t(*)())_dlsym(handle, "thr_self"))
	   && (_mt_thr_minstackp = (size_t(*)())_dlsym(handle, "thr_minstack"))
	   && (_mt_thr_createp = (int(*)())_dlsym(handle, "thr_create"))
	   && (_mt_thr_exitp = (int(*)())_dlsym(handle, "thr_exit"))
	   && (_mt_thr_killp = (int(*)())_dlsym(handle, "thr_kill"))
	   && (_mt_mutex_initp = (int(*)())_dlsym(handle, "mutex_init"))
	   && (_mt_mutex_lockp = (int(*)())_dlsym(handle, "mutex_lock"))
	   && (_mt_mutex_trylockp = (int(*)())_dlsym(handle, "mutex_trylock"))
	   && (_mt_mutex_unlockp = (int(*)())_dlsym(handle, "mutex_unlock"))
	   && (_mt_mutex_destroyp = (int(*)())_dlsym(handle, "mutex_destroy"))
	   && (_mt_rwlock_initp = (int(*)())_dlsym(handle, "rwlock_init"))
	   && (_mt_rw_rdlockp = (int(*)())_dlsym(handle, "rw_rdlock"))
	   && (_mt_rw_wrlockp = (int(*)())_dlsym(handle, "rw_wrlock"))
	   && (_mt_rw_unlockp = (int(*)())_dlsym(handle, "rw_unlock"))
	   && (_mt_rwlock_destroyp = (int(*)())_dlsym(handle, "rwlock_destroy"))
	   /* thread key stuff. */
	   && (_mt_thr_keycreatep = (int(*)())_dlsym(handle, "thr_keycreate"))
	   && (_mt_thr_getspecificp
		= (int(*)())_dlsym(handle,"thr_getspecific"))
	   && (_mt_thr_setspecificp = (int(*)())_dlsym(handle,"thr_setspecific"))
	   /* part of temporary patch for masking signals during ioctl()s */
	   && (_mt_sigprocmaskp = *((int(**)())
				  _dlsym(handle,"_sys_sigprocmask")))
	   )
	{
	/* We found all symbols we need. Must be linked with libthread. */
		_mt_multi_threaded = 1;
		_mt_first_thread = (* _mt_thr_selfp)();

	} else {
		_mt_sigprocmaskp = _sigprocmask;
	}
	if (handle != NULL)
		_dlclose(handle);
       
#endif /* _REENTRANT */

	/* 
	 * Initialize the subcomponents of libnsl.
	 * Note that the cs and saf subcomponents do not need initialization.
	 */
	_cr1_init();
	_des_init();
	_nsl_init();
	_rpc_init();
	_netdir_init();
	_netselect_init();
	_rx_init();
	_yp_init();
}

/*
 * Function to obtain the location of thread-specific storage
 * and to allocate the storage if none exists.
 * This function should be used only in a user-created thread.
 */

#ifdef _REENTRANT

void *
_mt_get_thr_specific_storage(key, size)
	thread_key_t key;
	size_t size;
{
	void *key_tbl;

	/*
	 * If THR_SETSPECIFIC() has not been called with the given key,
	 * then THR_GETSPECIFIC() returns with key_tbl set to NULL.
	 * So, storage must be allocated here and its address
	 * must be saved with THR_SETSPECIFIC().
	 * On subsequent accesses, THR_GETSPECIFIC() will provide
	 * this address.
	 *
	 * _mt_get_thr_specific_storage() returns NULL only if 
	 *    the call to THR_GETSPECIFIC() succeeds but key_tbl is NULL,
	 *    the allocation via calloc() fails, or
	 *    the address of the allocated storage cannot be saved.
	 */

	if (THR_GETSPECIFIC(key, &key_tbl) != 0
	 || key_tbl == NULL) {
		key_tbl = calloc(size, sizeof(void *));
		if (key_tbl == NULL) {
			return(NULL);
		}
		if (THR_SETSPECIFIC(key, key_tbl) != 0) {
			return (NULL);
		}
	}
	return(key_tbl);
}

/*
 * Warning!!  The following pair of functions are part of a temporary patch
 * to make sure that the ioctl() TI_BIND and TI_UNBIND requests are not
 * interrupted, since they cannot be restarted reliably.
 */

/* Mask SIGWAITING and SIGLWP.  */

int
_mt_masksigs(sigset_t *osetp)
{
	sigset_t	set;

	sigemptyset(&set);
	sigaddset(&set, SIGWAITING);
	sigaddset(&set, SIGLWP);
	return((*_mt_sigprocmaskp)(SIG_BLOCK, &set, osetp));
}

/* Unmask SIGWAITING and SIGLWP. */

int
_mt_unmasksigs(sigset_t *osetp)
{
	return((*_mt_sigprocmaskp)(SIG_SETMASK, osetp, (sigset_t *)NULL));
}

/*
 * End of patch
 */

#endif /* _REENTRANT */
