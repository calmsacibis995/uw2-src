/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $RCSfile: hash.h,v $$Revision: 1.1.1.1 $$Date: 1993/10/11 20:27:11 $
 *
 *    Copyright (c) 1991, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * $Log: hash.h,v $
 * Revision 1.1.1.1  1993/10/11  20:27:11  ram
 * NUC code from 1.1d release
 *
 * Revision 4.0.1.1  91/06/07  12:16:04  lwall
 * patch4: new copyright notice
 * 
 * Revision 4.0  91/03/20  01:57:53  lwall
 * 4.0 baseline.
 * 
 */

#define FILLPCT 60		/* don't make greater than 99 */

#ifdef DOINIT
char coeff[] = {
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1,
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1,
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1,
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1,
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1,
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1,
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1,
		61,59,53,47,43,41,37,31,29,23,17,13,11,7,3,1};
#else
extern char coeff[];
#endif

typedef struct hentry HENT;

struct hentry {
    HENT	*hent_next;
    char	*hent_key;
    STR		*hent_val;
    int		hent_hash;
};

struct htbl {
    HENT	**tbl_array;
    int		tbl_max;
    int		tbl_fill;
    int		tbl_riter;	/* current root of iterator */
    HENT	*tbl_eiter;	/* current entry of iterator */
};

STR *hfetch();
bool hstore();
bool hdelete();
HASH *hnew();
int hiterinit();
HENT *hiternext();
char *hiterkey();
STR *hiterval();
