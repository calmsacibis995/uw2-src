/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_TARGET_SDI_SDI_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/target/sdi/sdi.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * PDI_VERSION is defined as follows for the versions:
 *      UnixWare 1.1    1
 *      SVR4.2 DTMT     undefined by O/S; use 2
 *      SVR4.2 MP       3
 *      UnixWare 2.0    4
 * 
 * These are defined in both sdi.h and hba.h 
 */
#ifndef PDI_UNIXWARE11
#define PDI_UNIXWARE11  1
#endif
#ifndef PDI_SVR42_DTMT
#define PDI_SVR42_DTMT  2
#endif
#ifndef PDI_SVR42MP
#define PDI_SVR42MP     3
#endif
#ifndef PDI_UNIXWARE20
#define PDI_UNIXWARE20  4
#endif

#define PDI_VERSION	4		/* Version number for UnixWare 2.0 */

#ifdef PDI_SVR42
#undef PDI_SVR42
#endif  /* PDI_SVR42 */

#ifdef _KERNEL_HEADERS
#include <io/target/sdi/sdi_comm.h>
#else
#include <sys/sdi_comm.h>
#endif	/* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif

#endif	/* ! _IO_TARGET_SDI_SDI_H */
