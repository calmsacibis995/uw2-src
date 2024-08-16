/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_UADMIN_H	/* wrapper symbol for kernel use */
#define _SVC_UADMIN_H	/* subject to change without notice */

#ident	"@(#)kern:svc/uadmin.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define	A_REBOOT	1
#define	A_SHUTDOWN	2
#define	A_REMOUNT	4
#define A_CLOCK		8
#define	A_SWAPCTL	16
#define A_SETCONFIG	128

#define	AD_HALT		0
#define	AD_BOOT		1
#define	AD_IBOOT	2
#define AD_NOSYNC	4

#define AD_QUERY	1
#define AD_NOQUERY	2

#define UADMIN_SYNC 0
#define UADMIN_UMOUNT 1

/*
 * fcn's for A_SETCONFIG
 */
#define AD_PANICBOOT	1

#if defined (__STDC__) && !defined(_KERNEL)
int uadmin(int, int, int);
#endif

#ifdef _KERNEL

/*
 * Request types for drv_shutdown().
 */
typedef enum {
	SD_SOFT,	/* soft ("clean") shutdown */
	SD_HARD,	/* hard (immediate) shutdown */
	SD_PANIC	/* panic the machine */
} shutdown_request_t;

#ifdef __STDC__
extern void drv_shutdown(shutdown_request_t sd_rqt, int fcn);
#else
extern void drv_shutdown();
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_UADMIN_H */
