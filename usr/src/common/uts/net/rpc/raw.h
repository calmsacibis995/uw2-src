/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_RAW_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RAW_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/raw.h	1.18"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	raw.h, interface to raw rpc
 */

/*
 * The common memory area over which they will communicate
 */

#ifndef _KERNEL

extern	char	*_rawcombuf;

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RAW_H */
