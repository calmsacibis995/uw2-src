/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:iconObj.h	1.3"
/*----------------------------------------------------------------------------
 *	iconObj.h
 */
#ifndef ICONOBJ_H
#define ICONOBJ_H

#include <Xm/Xm.h>

/*----------------------------------------------------------------------------
 *
 */
class IconObj {
friend class IconList;							// FIX

public:
								IconObj (char* label, char* pix = 0);
								~IconObj ();
private:
	char*						d_label;
	char*						d_pix;

	IconObj*					d_next;			// Used by friend IconList-FIX
	IconObj*					d_prev;			// Used by friend IconList-FIX

public:
	Boolean						operator== (IconObj* obj);
	Boolean						operator!= (IconObj* obj);
	Boolean						operator> (IconObj* obj);

public:
	void						Label (char* newLbl);		// Change to label
	inline void					UpdatePixmap (char* pix);	// Change to pixmap
	inline char*				GetLabel ();				// Change to label
	inline char*				GetPixmap ();				// Change to pixmap
};

/*----------------------------------------------------------------------------
 *
 */
void
IconObj::UpdatePixmap (char* pix)
{
	d_pix = pix;
}

char*
IconObj::GetLabel ()
{
	return (d_label);
}

char*
IconObj::GetPixmap ()
{
	return (d_pix);
}

#endif		// ICONOBJ_H
