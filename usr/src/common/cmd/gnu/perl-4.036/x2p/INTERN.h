/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $RCSfile: INTERN.h,v $$Revision: 1.1.1.1 $$Date: 1993/10/11 20:27:08 $
 *
 *    Copyright (c) 1991, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * $Log: INTERN.h,v $
 * Revision 1.1.1.1  1993/10/11  20:27:08  ram
 * NUC code from 1.1d release
 *
 * Revision 4.0.1.1  91/06/07  12:11:20  lwall
 * patch4: new copyright notice
 * 
 * Revision 4.0  91/03/20  01:56:58  lwall
 * 4.0 baseline.
 * 
 */

#undef EXT
#define EXT

#undef INIT
#define INIT(x) = x

#define DOINIT
