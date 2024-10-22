/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:form/fcurrent.c	1.6.3.3"

#include        <curses.h>
#include	"wish.h"
#include	"token.h"
#include	"winp.h"
#include	"form.h"
#include	"vtdefs.h"

/* the offset in FORM_array of the current form */
form_id		FORM_curid = -1;
/* a pointer to the beginning of the array of displayed forms */
struct form	*FORM_array;

/*
 * makes the given form current and old form noncurrent
 */
int
form_current(fid)
form_id	fid;
{

	register struct form	*f;

	if (fid != FORM_curid)	/* if changing to different form.. abs k13 */
	    form_noncurrent();	

	FORM_curid = fid;
	f = &FORM_array[FORM_curid];
	vt_current(f->vid);
	if (f->flags & (FORM_DIRTY | FORM_ALLDIRTY))
		form_refresh(fid);
	return(SUCCESS);
}

/*
 * makes current form noncurrent
 */
int
form_noncurrent()
{
	if (FORM_curid >= 0)
		FORM_array[FORM_curid].flags |= FORM_DIRTY;
	FORM_curid = -1;				/* abs s16 */
	return(SUCCESS);
}
