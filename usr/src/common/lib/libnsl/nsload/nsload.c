/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsload/nsload.c	1.2.1.2"
#ident  "$Header: $"
#include <stdio.h>
#include <dlfcn.h>
#include <netdb.h>
#include <ns.h>
#ifdef _REENTRANT
#include <mt.h>
#include <thread.h>
#include <synch.h>
#else 
char _nsload_dlp_lock;
#define MUTEX_LOCK(lockp)       (0)
#define MUTEX_UNLOCK(lockp)     (0)
#endif /* _REENTRANT */

static char *lib = "/usr/lib/ns.so";
static void  *dlp;
static char  *(*ns_getbyname)();
static char  *(*ns_getbynum)();
static char  *(*ns_getent)();
static int    (*ns_setent)();
static void   (*ns_endent)();
#ifdef _REENTRANT

MUTEX_T _nsload_dlp_lock;

_nsload_init()
{
	if (MULTI_THREADED) {
		MUTEX_INIT(&_nsload_dlp_lock, USYNC_THREAD, NULL);
	}
}
#endif


void *
_nsload(db, func)
int db;
int func;
{

	if (dlp == NULL){
		MUTEX_LOCK(&_nsload_dlp_lock);
		if (dlp == NULL){
			if ((dlp = (void *)_dlopen(lib,  RTLD_LAZY)) == NULL){
				dlp = (void *) &dlp;
				MUTEX_UNLOCK(&_nsload_dlp_lock);
				return((void *)0);
			}
		}
		MUTEX_UNLOCK(&_nsload_dlp_lock);
	} else if ( dlp == &dlp )
		return((void *)0);
		
	switch(func){
	case NSBYNAM:
		if (ns_getbyname == NULL){
			MUTEX_LOCK(&_nsload_dlp_lock);
			if (ns_getbyname == NULL){
				ns_getbyname = (char *(*)())_dlsym(dlp, "ns_getbyname");
			}
			MUTEX_UNLOCK(&_nsload_dlp_lock);
		}
		return((void *)ns_getbyname);
	case NSBYNUM:
		if (ns_getbynum == NULL){
			MUTEX_LOCK(&_nsload_dlp_lock);
			if (ns_getbynum == NULL){
				ns_getbynum = (char *(*)())_dlsym(dlp,"ns_getbynum");
			}
			MUTEX_UNLOCK(&_nsload_dlp_lock);
		}
		return((void *)ns_getbynum);
	case NSGETENT:
		if (ns_getent == NULL){
			MUTEX_LOCK(&_nsload_dlp_lock);
			if (ns_getent == NULL){
				ns_getent = (char *(*)())_dlsym(dlp, "ns_getent");
			}
			MUTEX_UNLOCK(&_nsload_dlp_lock);
		}
		return((void *)ns_getent);
	case NSSETENT:
		if (ns_setent == NULL){
			MUTEX_LOCK(&_nsload_dlp_lock);
			if (ns_setent == NULL){
				ns_setent = (int (*)())_dlsym(dlp, "ns_setent");
			}
			MUTEX_UNLOCK(&_nsload_dlp_lock);
		}
		return((void *)ns_setent);
	case NSENDENT:
		if (ns_endent == NULL){
			MUTEX_LOCK(&_nsload_dlp_lock);
			if (ns_endent == NULL){
				ns_endent = (void (*)())_dlsym(dlp, "ns_endent");
			}
			MUTEX_UNLOCK(&_nsload_dlp_lock);
		}
		return((void *)ns_endent);

	}
	return((void *)0);
}
