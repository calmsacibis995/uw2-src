/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/resolv/resolvabi.h	1.1.4.2"
#ident	"$Header: $"

#ifndef	_RESOLV_RESOLVABI_H
#define _RESOLV_RESOLVABI_H

#ifdef _RESOLV_ABI
/* For internal use only when building the resolv routines */
#define	select	_abi_select
#define	syslog	_nsl_syslog
#define	seteuid	_abi_seteuid
#endif /* _RESOLV_ABI */

#endif /* _RESOLV_RESOLVABI_H */
