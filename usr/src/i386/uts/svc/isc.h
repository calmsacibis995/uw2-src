/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_ISC_H	/* wrapper symbol for kernel use */
#define _SVC_ISC_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/isc.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* Enhanced Application Compatibility Support */

#define ISC_SIGCONT	23
#define ISC_SIGSTOP	24
#define ISC_SIGTSTP	25

/* POSIX waitpid() ISC Defines */
#define ISC_WNOHANG	1
#define ISC_WUNTRACED	2

/* POSIX TIOC  ISC Defines */
#define ISC_TIOC	('T' << 8)
#define ISC_TCSETPGRP	(ISC_TIOC | 20)
#define ISC_TCGETPGRP	(ISC_TIOC | 21)

/* End Enhanced Application Compatibility Support */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_ISC_H */
