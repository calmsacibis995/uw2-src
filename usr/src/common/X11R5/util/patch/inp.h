/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5util:patch/inp.h	1.1"
/* $Header: inp.h,v 2.0 86/09/17 15:37:25 lwall Exp $
 *
 * $Log:	inp.h,v $
 * Revision 2.0  86/09/17  15:37:25  lwall
 * Baseline for netwide release.
 * 
 */

EXT LINENUM input_lines INIT(0);	/* how long is input file in lines */
EXT LINENUM last_frozen_line INIT(0);	/* how many input lines have been */
					/* irretractibly output */

bool rev_in_string();
void scan_input();
bool plan_a();			/* returns false if insufficient memory */
void plan_b();
char *ifetch();

