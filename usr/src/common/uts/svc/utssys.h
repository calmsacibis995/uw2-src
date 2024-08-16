/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_UTSSYS_H	/* wrapper symbol for kernel use */
#define _SVC_UTSSYS_H	/* subject to change without notice */

#ident	"@(#)kern:svc/utssys.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Definitions related to the utssys() system call. 
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * "commands" of utssys
 */
#define UTS_UNAME	0x0
#define UTS_USTAT	0x2	/* 1 was umask */
#define UTS_FUSERS	0x3
#define UTS_GETENG	0x4

/*
 * Flags to UTS_FUSERS.
 */
#define F_FILE_ONLY	0x1
#define F_CONTAINED	0x2

/*
 * Structure yielded by UTS_FUSERS:
 */
typedef struct f_user {
	pid_t	fu_pid;		/* pid of process using file. */
	int	fu_flags;	/* see below. */
	uid_t	fu_uid;		/* real uid of process using file. */
} f_user_t;

/*
 * fu_flags values:
 */
#define F_CDIR		0x1	/* current directory */
#define F_RDIR		0x2	/* root directory */
#define F_TEXT		0x4	/* a.out */
#define F_MAP		0x8	/* mapped file (unsupported) */
#define F_OPEN		0x10	/* open file */
#define F_TRACE		0x20	/* /proc vnode */
#define F_TTY		0x40	/* controlling terminal */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_UTSSYS_H */ 
