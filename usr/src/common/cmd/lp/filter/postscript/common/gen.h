/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:filter/postscript/common/gen.h	1.1.5.3"
#ident	"$Header: $"

/*
 *
 * A few definitions that shouldn't have to change. They're used by most of the
 * programs in this package.
 *
 */


#define PROGRAMVERSION	"3.15"


#define NON_FATAL	0
#define FATAL		1
#define USER_FATAL	2

#define OFF		0
#define ON		1

#define FALSE		0
#define TRUE		1

#define BYTE		8
#define BMASK		0377

#define POINTS		72.3

#ifndef PI
#define PI		3.141592654
#endif


/*
 *
 * A few simple macros.
 *
 */


#define ABS(A)		((A) >= 0 ? (A) : -(A))
#define MIN(A, B)	((A) < (B) ? (A) : (B))
#define MAX(A, B)	((A) > (B) ? (A) : (B))

