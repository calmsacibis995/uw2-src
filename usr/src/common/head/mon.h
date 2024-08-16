/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:common/head/mon.h	1.8.4.1"

#ifndef _MON_H
#define _MON_H

#ifdef __cplusplus
extern "C" {
#endif

struct hdr {
	char	*lpc;
	char	*hpc;
	int	nfns;
};

struct cnt {
	char	*fnpc;
	long	mcnt;
};

typedef unsigned short WORD;

#define MON_OUT	"mon.out"
#define MPROGS0	(150 * sizeof(WORD))	/* 300 for pdp11, 600 for 32-bits */
#define MSCALE0	4
#define NULL	0

#if defined(__STDC__)
extern void monitor(int (*)(), int (*)(), WORD *, int, int);
#else
extern void monitor();
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MON_H */
