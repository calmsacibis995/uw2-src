/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _USTAT_H
#define _USTAT_H

#ident	"@(#)sgs-head:common/head/ustat.h	1.3.2.1"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/ustat.h>

#if defined(__STDC__)
extern int ustat(dev_t, struct ustat *);
#else
extern int ustat();
#endif	/* end defined(_STDC) */

#ifdef __cplusplus
}
#endif

#endif /* _USTAT_H */
