/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:nwClnt.c	1.4"

#ifdef _REENTRANT

#include <stdlib.h>
#include <dlfcn.h>
#include "nwClnt.h"

MUTEX_T	_nwClnt_connTable_lock;

THREAD_KEY_T _nwClnt_scan_key;
THREAD_KEY_T _nwClnt_req_buffer_key;
THREAD_KEY_T _nwClnt_reply_buffer_key;

void _free_nwClnt_scan_keytbl( void *st );
void _free_nwClnt_req_buffer_keytbl( void *reqBuf );
void _free_nwClnt_reply_buffer_keytbl( void *replyBuf );

void
_init()
{
	int j;
	extern int _mt_multi_threaded;


	/* Initialize synchronization primitives */
	
	MUTEX_INIT (&_nwClnt_connTable_lock, USYNC_THREAD, NULL);

	/* create keys for pre-thread storage */
	j = THR_KEYCREATE(&_nwClnt_scan_key, &_free_nwClnt_scan_keytbl);
	j = THR_KEYCREATE(&_nwClnt_req_buffer_key, 
		&_free_nwClnt_req_buffer_keytbl);
	j = THR_KEYCREATE(&_nwClnt_reply_buffer_key, 
		&_free_nwClnt_reply_buffer_keytbl);


#ifdef CL_THR_DEBUG
	printf("DEBUG: libNwClnt _init: j = 0x%x, _nwClnt_reply_buffer_key = 0x%x\n", 
		j, _nwClnt_reply_buffer_key );
#endif /* CL_THR_DEBUG */


}

/* This free function is called automatically by thr_exit */
void
_free_nwClnt_scan_keytbl( void *st )
{

	if( !FIRST_OR_NO_THREAD ) {
		if( st )
			free( st );
	}
	
	
}

/* This free function is called automatically by thr_exit */
void
_free_nwClnt_req_buffer_keytbl( void *reqBuf )
{

    if( !FIRST_OR_NO_THREAD ) {
        if( reqBuf )
            free( reqBuf );
	}

}

/* This free function is called automatically by thr_exit */
void
_free_nwClnt_reply_buffer_keytbl( void *replyBuf )
{

    if( !FIRST_OR_NO_THREAD ) {
        if( replyBuf )
            free( replyBuf );
    }

}

#endif /* _REENTRANT */
