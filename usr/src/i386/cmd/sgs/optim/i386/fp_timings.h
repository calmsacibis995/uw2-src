/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/fp_timings.h	1.2"
#ifndef FP_TIMINGS_H
#define FP_TIMINGS_H
typedef struct times_s {
			int t_387;
			int t_486;
			int t_ptm;
		} times_t;

typedef struct opop_s {
		unsigned int op;
		char *op_code;
		} opopcode;

extern int gain_by_mem2st();
extern opopcode mem2st();
#endif
