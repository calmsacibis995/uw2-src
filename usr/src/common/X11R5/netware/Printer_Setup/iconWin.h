/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:iconWin.h	1.15"
/*----------------------------------------------------------------------------
 *	iconWin.h
 */
#ifndef ICONWIN_H
#define ICONWIN_H

extern "C" {
# include <libMDtI/DesktopP.h>
# include <libMDtI/DnDUtil.h>
# include <libMDtI/DtI.h>
# include <libMDtI/Flat.h>
}

#include "FIconBoxI.h"
#include "iconList.h"

/*----------------------------------------------------------------------------
 *
 */
typedef struct {
	XtArgVal					x;
	XtArgVal					y;
	XtArgVal					width;
	XtArgVal					height;
	XtArgVal					label;
	XtArgVal					set;
	XtArgVal					sensitive;
	XtArgVal					obj;
	XtArgVal					userData;
	XtArgVal					managed;
	XtArgVal					mappedWhenManaged;
} DisplayItem;

/*----------------------------------------------------------------------------
 *
 */
class IconWin : public BasicComponent {
public:
								IconWin (Widget	parent,
										 char*	name,
										 char**	pixmapFiles,
										 short	pixmapCnt);
								~IconWin ();

private:
	char**						_pixmapList;
	Cardinal					_droppedIndex;
	short						_pixmapCnt;

	short						_cellWidth;
	short						_cellHeight;
	short						_row;
	short						_col;
	int							_hOffset;
	int							_vOffset;
	Pixmap						_pixmap;

	Widget						_iconBox;

	long						_background;

protected:
	IconList*					_list;

public:
	inline char*				GetPixmap (int i);
	inline Cardinal				DroppedIndex ();

	const IconObj*				findByName (const char* name);

	void 						ReGenWin (Boolean firstTime = False);
	Boolean						AddIcon (IconObj *obj);
	Boolean						DeleteIcon (IconObj *obj);
	void						LayoutIcons (DisplayItem* items, Cardinal cnt);

	void						RefreshIconWin (Window		win,
												IconObj*	sel,
												IconObj*	prev,
									 			Boolean		updateFlag);
	virtual void				NotifyParentOfSel (IconObj* obj) = 0;
	virtual void				NotifyParentOfDblSel (IconObj* obj) = 0;

	static void					SelectCB (Widget, XtPointer, XtPointer);
	void						Select (ExmFlatCallData*);
	static void					UnselectCB (Widget, XtPointer, XtPointer);
	void						Unselect ();
	static void					DblSelectCB (Widget, XtPointer, XtPointer);
	void						DblSelect (ExmFlatCallData*);

	static void				 	TriggerMsg (Widget		w,
											XtPointer	clientData, 
											XtPointer	callData );
	void						Trigger (Widget		w,
										 XtPointer	clientData,
										 XtPointer	callData);
	static void					TransferProc (Widget	w,
											  XtPointer	clientData,
											  XtPointer	callData);
	virtual void				Transfer (Widget			w,
										  DmDnDDstInfoPtr	dip) = 0;

	static void				 	DropProcCB (Widget		w,
											XtPointer	clientData,
											XtPointer	callData);
	virtual void				DropProc (Widget	w,
										  XtPointer	clientData,
										  XtPointer	callData) = 0;
	static void					CreateIconCursor (Widget	w,
												  XtPointer	clientData,
												  XtPointer	callData);
	void 						InitDnDIcons (Screen* screen);
};

/*----------------------------------------------------------------------------
 *
 */
char*
IconWin::GetPixmap (int i)
{
	return (_pixmapList[i]);
}

Cardinal
IconWin::DroppedIndex ()
{
	return (_droppedIndex);
}

#endif		// ICONWIN_H
