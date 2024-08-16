/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CORE_H	/* wrapper symbol for kernel use */
#define _PROC_CORE_H	/* subject to change without notice */

#ident	"@(#)kern:proc/core.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * core.h - definitions of ELF core file note types.
 */

#define CF_T_PRSTATUS	10
#define CF_T_FPREGS	12
#define CF_T_PRPSINFO	13
#define CF_T_PRCRED	14
#define CF_T_UTSNAME	15
#define CF_T_LWPSTATUS	16
#define CF_T_LWPSINFO	17

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CORE_H */
