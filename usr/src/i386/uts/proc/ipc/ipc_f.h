/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_IPC_F_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_IPC_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/ipc/ipc_f.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Family-specific IPC definitions, i386 version.
 */

/*
 * Shared memory segment low boundary address multiple, must be a power of two.
 * Defined here to avoid rendering shm.h machine-dependent or exposing user
 * code to the contents of param.h in violation of standards.
 */
#define _SHMLBA		((ulong_t)(1) << 12)	/* ptob(1) */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_IPC_F_H */
