/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STRSTAT_H	/* wrapper symbol for kernel use */
#define _IO_STRSTAT_H	/* subject to change without notice */

#ident	"@(#)kern:io/strstat.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Streams Statistics header file.  This file
 * defines the counters which are maintained for statistics gathering
 * under Streams. 
 */

typedef struct {
	int use;	/* current item usage count */
	int total;	/* total item usage count */
	int max;	/* maximum item usage count */
	int fail;	/* count of allocation failures */
} alcdat;

struct strstat {
	alcdat stream;		/* stream allocation data */
	alcdat queue;		/* queue allocation data */
	alcdat msgblock;	/* message block allocation data */
	alcdat mdbblock;	/* mesg/data/buffer triplet allocation data */
	alcdat linkblk;		/* linkblk allocation data */
	alcdat strevent;	/* strevent allocation data */
};


/* in the following macro, x is assumed to be of type alcdat */

#define BUMPUP(X)	{(X).use++;  (X).total++;\
			 if ((X).use > (X).max) (X).max = (X).use; }


/* per-module statistics structure */

struct module_stat {
	long ms_pcnt;		/* count of calls to put proc */
	long ms_scnt;		/* count of calls to service proc */
	long ms_ocnt;		/* count of calls to open proc */
	long ms_ccnt;		/* count of calls to close proc */
	long ms_acnt;		/* count of calls to admin proc */
	char *ms_xptr;		/* pointer to private statistics */
	short ms_xsize;		/* length of private statistics buffer */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STRSTAT_H */
