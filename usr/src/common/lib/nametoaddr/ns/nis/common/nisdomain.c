/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/common/nisdomain.c	1.1.1.1"
#ident  "$Header: $"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <rpcsvc/yp_prot.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif

static char *yp_domain;

char *
nis_domain()
{
	if (yp_domain == NULL){
#ifdef _REENTRANT
		MUTEX_LOCK(&_nis_domain_lock);
#endif
		if (yp_domain == NULL) {
			yp_domain = calloc(1, YPMAXDOMAIN);
			if (getdomainname(yp_domain, YPMAXDOMAIN) < 0){
				free(yp_domain);
				yp_domain = NULL;
			}
		}
#ifdef _REENTRANT
		MUTEX_UNLOCK(&_nis_domain_lock);
#endif
	}
	return(yp_domain);
}
