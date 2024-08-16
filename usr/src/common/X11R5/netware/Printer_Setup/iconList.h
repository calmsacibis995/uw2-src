/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:iconList.h	1.2"
/*----------------------------------------------------------------------------
 *
 */
#ifndef ICONLIST_H
#define ICONLIST_H

#include "iconObj.h"

/*----------------------------------------------------------------------------
 *
 */
class IconList {
public:
								IconList ();
								~IconList ();

private:
	IconObj*					d_list;
	IconObj*					d_cur;
	IconObj*					d_selected;
	short						d_cnt;

public:
	void						Insert (IconObj* insObj);
	void						Remove (IconObj* remObj);

	IconObj*					FindObj (char* label);
	void						DelObj ();
	IconObj*					GetFirst ();
	IconObj*					GetNext ();

	inline IconObj*				GetSelected ();
	inline void					UpdateSelected (IconObj* selObj);
	inline short				GetObjCnt ();
};

/*----------------------------------------------------------------------------
 *
 */
IconObj*
IconList::GetSelected ()
{
	return (d_selected);
}

void
IconList::UpdateSelected (IconObj* selObj)
{
	d_selected = selObj;
}

short
IconList::GetObjCnt ()
{
	return (d_cnt);
}

#endif
