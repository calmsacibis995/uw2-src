/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip_nis/ckypbind.c	1.2.1.1"
#ident  "$Header: $"
#ifdef _REENTRANT
#include "tcpip_nis_mt.h"
#else 
char _ntoa_ypbind_lock;
#define MUTEX_LOCK(lockp)       (0)
#define MUTEX_UNLOCK(lockp)     (0)
#endif

int ypbindisup=0;

_nis_check_ypbind(domain)
char *domain;
{
	if (ypbindisup){
		return(0);
	}
	MUTEX_LOCK(&_ntoa_ypbind_lock);
	if (ypbindisup){
		MUTEX_UNLOCK(&_ntoa_ypbind_lock);
		return(0);
	}
	ypbindisup = _ping_ypbind(domain);
	MUTEX_UNLOCK(&_ntoa_ypbind_lock);

	return(ypbindisup ? 0 : 1);
}
