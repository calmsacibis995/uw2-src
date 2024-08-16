/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/novell/novell_mt.h	1.2"
#ident  "$Header: /SRCS/esmp/usr/src/nw/lib/nametoaddr/novell/novell_mt.h,v 1.2 1994/06/20 16:30:31 mcpowers Exp $"

/*
 * novell_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

/* thread-specific data structure definition */
struct _ntoa_novell_tsd {
	void *servdata_p;
	void *sapdata_p;
};

#define _NTOA_NOVELL_KEYTBL_SIZE \
			( sizeof(struct _ntoa_novell_tsd) / sizeof(void *) )
#ifdef __STDC__

extern void _free_ntoa_novell_keytbl(void *);
extern void _free_ntoa_novell_data(void *);
	
#else /* ! __STDC__ */

extern void _free_ntoa_novell_keytbl();
extern void _free_ntoa_novell_data();

#endif /* __STDC__ */

/* key for thread specific data */
extern THREAD_KEY_T	_ntoa_novell_key;

extern MUTEX_T		_ntoa_novell_firstTime_lock;
extern MUTEX_T		_ntoa_novell_bindresvport_lock;

#endif /* _REENTRANT */
