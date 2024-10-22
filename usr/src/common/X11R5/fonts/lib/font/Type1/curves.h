/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fonts:lib/font/Type1/curves.h	1.1"

/* $XConsortium: curves.h,v 1.2 91/10/10 11:17:59 rws Exp $ */
/* Copyright International Business Machines, Corp. 1991
 * All Rights Reserved
 * Copyright Lexmark International, Inc. 1991
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM or Lexmark not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM AND LEXMARK PROVIDE THIS SOFTWARE "AS IS", WITHOUT ANY WARRANTIES OF
 * ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  THE ENTIRE RISK AS TO THE
 * QUALITY AND PERFORMANCE OF THE SOFTWARE, INCLUDING ANY DUTY TO SUPPORT
 * OR MAINTAIN, BELONGS TO THE LICENSEE.  SHOULD ANY PORTION OF THE
 * SOFTWARE PROVE DEFECTIVE, THE LICENSEE (NOT IBM OR LEXMARK) ASSUMES THE
 * ENTIRE COST OF ALL SERVICING, REPAIR AND CORRECTION.  IN NO EVENT SHALL
 * IBM OR LEXMARK BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */
/*SHARED*/
 
#define   StepConic(R,xA,yA,xB,yB,xC,yC,r)      t1_StepConic(R,xA,yA,xB,yB,xC,yC,r)
#define   StepBezier(R,xA,yA,xB,yB,xC,yC,xD,yD) t1_StepBezier(R,xA,yA,xB,yB,xC,yC,xD,yD)
 
#define   FlattenConic(xM,yM,xC,yC,r)        t1_StepConic(NULL,(fractpel)0,(fractpel)0,xM,yM,xC,yC,r)
#define   FlattenBezier(xB,yB,xC,yC,xD,yD)   t1_StepBezier(NULL,(fractpel)0,(fractpel)0,xB,yB,xC,yC,xD,yD)
 
struct segment *t1_StepConic();
struct segment *t1_StepBezier();
 
/*END SHARED*/
