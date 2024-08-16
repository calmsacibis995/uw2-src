/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/tiuser.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This file remains for compatibility only.
 * Kernel source files should now include
 * <net/xti.h> instead.
 */

#ifdef _KERNEL_HEADERS	/* SVR4.0 COMPATIBILITY */

#include <net/xti.h>

#else /* !_KERNEL_HEADERS */	/* SVR4.0 COMPATIBILITY */

#ifndef _T_ERROR
#define _T_ERROR

#ifdef __STDC__
	extern void	t_error(char *);
#else /* !__STDC__ */
	extern void	t_error();
#endif /* __STDC__ */

#endif /* _T_ERROR */

#include <sys/xti.h>

#endif /* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif
