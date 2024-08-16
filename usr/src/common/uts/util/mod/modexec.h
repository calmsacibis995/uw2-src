/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODEXEC_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODEXEC_H	/* subject to change without notice */

#ident	"@(#)kern:util/mod/modexec.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/exec.h>	/* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/exec.h>	/* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Private data for EXEC types, initialized by configuration tools.
 */
struct	mod_exec_data	{
	char			*med_name;
	struct	execsw_info	med_info;
	struct	execsw		*med_execp;
};

#endif

/*
 * Structure used for EXEC type module registration.
 */
struct	mod_exec_tdata	{
	int		met_order;
	int		met_wildcard;
	int		met_nmagic;
	ushort_t	*met_magic;
};

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MODEXEC_H */
