/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_SECSYS_H	/* wrapper symbol for kernel use */
#define _SVC_SECSYS_H	/* subject to change without notice */

#ident	"@(#)kern:svc/secsys.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#else

#include <sys/types.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * Commands for secsys system call.
 */

#define	ES_MACOPENLID	1	/* open LID file for kernel use */
#define	ES_MACSYSLID	2	/* assign LID to system processes */
#define	ES_MACROOTLID	3	/* assign LID to root fs root vnode */
#define	ES_PRVINFO	4	/* Get the privilege mechanism information */
#define	ES_PRVSETCNT	5	/* Get the privilege mechanism set count */
#define	ES_PRVSETS	6	/* Get the privilege mechanism sets */
#define	ES_MACADTLID	7	/* assign LID to audit daemon */
#define	ES_PRVID	8	/* Get the privileged ID number */
#define	ES_TPGETMAJOR	9	/* Get major device number of Trusted Path */


/*
 * Commands for secsys system call.
 */

#define	SA_EXEC		001	/* execute access */
#define	SA_WRITE	002	/* write access */
#define	SA_READ		004	/* read access */
#define	SA_SUBSIZE	010	/* get sub_attr size */

/*
 * Following is the attributes structure for an
 * object used by the secadvise() call.
 */

struct obj_attr {
	uid_t uid;
	gid_t gid;
	mode_t mode;
	level_t lid;
	char filler[8];
};

/*
 * Following is the attributes structure for a
 * subject used by the secadvise() call.  Note
 * that this structure serves as a placeholder.
 */

struct sub_attr {
	char kernel_info[1];
};


#if defined(__STDC__) && !defined(_KERNEL)
int secsys(int, char *);
int secadvise(struct obj_attr *, int, struct sub_attr *);
#endif

#if defined(__cplusplus)
        }
#endif
#endif /* _SVC_SECSYS_H */
