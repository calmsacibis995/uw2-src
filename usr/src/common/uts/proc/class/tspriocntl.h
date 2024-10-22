/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_TSPRIOCNTL_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_TSPRIOCNTL_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class/tspriocntl.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Time-sharing class specific structures for the priocntl system call.
 */

typedef struct tsparms {
	short	ts_uprilim;	/* user priority limit */
	short	ts_upri;	/* user priority */
} tsparms_t;


typedef struct tsinfo {
	short	ts_maxupri;	/* configured limits of user priority range */
} tsinfo_t;

#define	TS_NOCHANGE	-32768

/*
 * The following is used by the dispadmin(1M) command for
 * scheduler administration and is not for general use.
 */

typedef struct tsadmin {
	struct tsdpent	*ts_dpents;
	short		ts_ndpents;
	short		ts_cmd;
} tsadmin_t;

#define	TS_GETDPSIZE	1
#define	TS_GETDPTBL	2
#define	TS_SETDPTBL	3


#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CLASS_TSPRIOCNTL_H */
