/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $RCSfile: str.h,v $$Revision: 1.1.1.1 $$Date: 1993/10/11 20:27:12 $
 *
 *    Copyright (c) 1991, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * $Log: str.h,v $
 * Revision 1.1.1.1  1993/10/11  20:27:12  ram
 * NUC code from 1.1d release
 *
 * Revision 4.0.1.1  91/06/07  12:20:22  lwall
 * patch4: new copyright notice
 * 
 * Revision 4.0  91/03/20  01:58:21  lwall
 * 4.0 baseline.
 * 
 */

struct string {
    char *	str_ptr;	/* pointer to malloced string */
    double	str_nval;	/* numeric value, if any */
    int		str_len;	/* allocated size */
    int		str_cur;	/* length of str_ptr as a C string */
    union {
	STR *str_next;		/* while free, link to next free str */
    } str_link;
    char	str_pok;	/* state of str_ptr */
    char	str_nok;	/* state of str_nval */
};

#define Nullstr Null(STR*)

/* the following macro updates any magic values this str is associated with */

#define STABSET(x) (x->str_link.str_magic && stabset(x->str_link.str_magic,x))

EXT STR **tmps_list;
EXT long tmps_max INIT(-1);

char *str_2ptr();
double str_2num();
STR *str_mortal();
STR *str_make();
STR *str_nmake();
char *str_gets();
