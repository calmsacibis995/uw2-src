/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 *
 */

#ident	"@(#)fmli:inc/vt.h	1.9.4.3"

struct vt {
	char	*title;
	WINDOW	*win;
	WINDOW  *subwin;
	vt_id	next;
	vt_id	prev;
	int	number;
	int	flags;
};
/* les */
#define	WORK_LEN	7
#define	DATE_LEN	48

#define VT_USED		 01
#define VT_DIRTY	 02	/* contents of window changed */
#define VT_BDIRTY	 04	/* border of window changed */
#define VT_TDIRTY	010	/* title of window changed */
#define VT_SADIRTY	020	/* scroll "arrows" for window changed */
#define VT_PADIRTY	040	/* page "arrows" for window changed */ 

#define VT_ANYDIRTY	(VT_DIRTY | VT_BDIRTY | VT_TDIRTY | VT_PADIRTY | VT_SADIRTY)

extern vt_id		VT_front;
extern vt_id		VT_back;
extern vt_id		VT_curid;
extern struct vt	*VT_array;

/* attribute array                   abs: indirection removed.
extern chtype		Attr_list[];
#define highlights(x)	((chtype) Attr_list[x])
*/
