/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:fp/fp_data.c	1.5"
/*
 * contains the definitions
 * of the global symbols used
 * by the floating point environment
 *
 * Cannot #include <math.h> because it believes __huge_val to be a double.
 */
#include "synonyms.h"

#ifdef __STDC__
const union {
	unsigned char uc[sizeof(double) / sizeof(unsigned char)];
	double d;
} __huge_val
	= { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } };
#else
unsigned long __huge_val[sizeof(double) / sizeof(unsigned long)]
	= { 0, 0x7ff00000 };
#endif

int	__flt_rounds = 1;
