/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/tune.h	1.2"


/* Tunable parameters
 *	This file defines parameters that one can change to improve
 *	performance for particular machines.
 *
 *	PGSZ	Used in input.c as the size of "pages" to read from
 *		the file.  Generally, smaller sizes give the opportunity
 *		to read less; larger sizes make the reads that happen
 *		more efficient.
 *
 *		Recommendation:  Use at least the block size for the most
 *		common file systems on the machine.  Twice the common
 *		size seems to work well, 4 blocks seems a little too big.
 *		SVR3 uses 1k blocks, SVR4 uses 2K.
 */


#define PGSZ	2048
