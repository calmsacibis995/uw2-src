/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/objform.h	1.4.4.3"

typedef struct {
	int flags;
	int curpage;		/* current form page */
	int lastpage;		/* last page of the form */
	int curfield;		/* current field number */
	int numactive;		/* number of active fields */
	char **holdptrs;	/* array of low-level field structures */
	char **mulvals;		/* field specific variables (F1, F2, etc.) */
	struct fm_mn fm_mn;	/* main structure for form descriptors */
	int *visible;		/* list of active/visible fields */
	int *slks;		/* list of SLKS specific to this form */ 
} forminfo;
