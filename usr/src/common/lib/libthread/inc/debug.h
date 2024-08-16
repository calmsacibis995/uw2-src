/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _DEBUG_H
#define _DEBUG_H

#ident	"@(#)libthread:common/lib/libthread/inc/debug.h	1.3.2.1"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * This file contains the definition of ASSERT used for debugging.
 */

#ifdef DEBUG
#define ASSERT(x)	((void)((x) || _thr_assfail(#x, __FILE__, __LINE__)))
extern int _thr_assfail(char *, char *, int);
#else /* DEBUG */
#define ASSERT(x)
#endif /* DEBUG */
#endif /* _DEBUG_H */

#ifdef THR_DEBUG
#define PRINTF(f)                       printf(f)
#define PRINTF1(f,x1)                   printf(f,x1)
#define PRINTF2(f,x1,x2)                printf(f,x1,x2)
#define PRINTF3(f,x1,x2,x3)             printf(f,x1,x2,x3)
#define PRINTF4(f,x1,x2,x3,x4)          printf(f,x1,x2,x3,x4)
#define PRINTF5(f,x1,x2,x3,x4,x5)       printf(f,x1,x2,x3,x4,x5)
#define PRINTF6(f,x1,x2,x3,x4,x5,x6)    printf(f,x1,x2,x3,x4,x5,x6)
#define PRINTF7(f,x1,x2,x3,x4,x5,x6,x7) printf(f,x1,x2,x3,x4,x5,x6,x7)
#define PRINTF8(f,x1,x2,x3,x4,x5,x6,x7,x8)   \
			printf(f,x1,x2,x3,x4,x5,x6,x7,x8)
#else
#define PRINTF(f)
#define PRINTF1(f,x1)
#define PRINTF2(f,x1,x2)
#define PRINTF3(f,x1,x2,x3)
#define PRINTF4(f,x1,x2,x3,x4)
#define PRINTF5(f,x1,x2,x3,x4,x5)
#define PRINTF6(f,x1,x2,x3,x4,x5,x6)
#define PRINTF7(f,x1,x2,x3,x4,x5,x6,x7)
#define PRINTF8(f,x1,x2,x3,x4,x5,x6,x7,x8)
#endif /* THR_DEBUG */
