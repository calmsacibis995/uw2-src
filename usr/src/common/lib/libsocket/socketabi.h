/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/socketabi.h	1.1.1.4"
#ident "$Header: $"

#ifndef	_SOCK_SOCKABI_H
#define _SOCK_SOCKABI_H

#define fopen	_fopen

#ifdef _SOCK_ABI
/* For internal use only when building the sockets routines */
#define	select	_abi_select
#define	syslog	_nsl_syslog
#define	seteuid	_abi_seteuid
#define	setegid	_abi_setegid
#endif /* _SOCK_ABI */

#endif /* _SOCK_SOCKABI_H */
