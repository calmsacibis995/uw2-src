/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/yp_mt.h	1.1.2.4"
#ident  "$Header:$"

/*
 * yp_mt.h
 */
#ifndef _YP_MT_H
#define _YP_MT_H


#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

extern MUTEX_T		_yp_match_lock;
extern RWLOCK_T   	_yp_domain_list_lock;

#endif /* _REENTRANT */

#endif /* _YP_MT_H */
