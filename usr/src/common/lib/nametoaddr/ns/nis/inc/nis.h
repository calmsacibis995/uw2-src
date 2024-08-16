/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/inc/nis.h	1.1"
#ident  "$Header: $"
#include <sys/types.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#define yp_ismapthere(r) ((r) == YPERR_KEY || (r) == YPERR_NOMORE)
#define yp_isbusy(r) ((r) == YPERR_BUSY)
#define yp_retcode(err) if (yp_ismapthere(err)) \
			set_nsaction(NS_NOTFOUND); \
		else if (yp_isbusy(err)) \
			set_nsaction(NS_TRYAGAIN); \
		else \
			set_nsaction(NS_UNAVAIL); 

extern char *nis_domain();
