/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/libnwlocale_mt.h	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/head/inc/libnwlocale_mt.h,v 1.1 1994/09/26 17:09:22 rebekah Exp $"
/*
 * libnwlocale_mt.h
 */

#include "unicode.h"
#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

extern MUTEX_T		_libnwlocale_lsetloc_lock;
extern MUTEX_T		_libnwlocale__lconvInfo_lock;
extern MUTEX_T		_libnwlocale_loaded_lock;

struct _libnwlocale_tsd
{
	unicode N_FAR *last;
};

extern void _free_libnwlocale_keytbl (void *);

extern	THREAD_KEY_T	_libnwlocale_key;

#define _LIBNWLOCALE_KEYTBL_SIZE (sizeof (struct _libnwlocale_tsd) / sizeof (void *))

#endif /* _REENTRANT */
