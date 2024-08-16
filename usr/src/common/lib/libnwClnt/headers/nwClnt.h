/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/nwClnt.h	1.2"
#ifndef _NWCLNT_H_
#define _NWCLNT_H_

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

#ifdef __STDC__

extern void _free_nwClnt_scan_keytbl( void *st );
extern void _free_nwClnt_req_buffer_keytbl( void *reqBuf );
extern void _free_nwClnt_reply_buffer_keytbl( void *replyBuf );

#else /* ! __STDC__ */

extern void _free_nwClnt_scan_keytbl( );
extern void _free_nwClnt_req_buffer_keytbl( );
extern void _free_nwClnt_reply_buffer_keytbl( );

#endif /* __STDC__ */

/* key for thread specific data */
extern MUTEX_T _nwClnt_connTable_lock;

extern THREAD_KEY_T _nwClnt_scan_key;
extern THREAD_KEY_T _nwClnt_req_buffer_key;
extern THREAD_KEY_T _nwClnt_reply_buffer_key;

#endif /* _REENTRANT */

#endif /* _NWCLNT_H_ */
