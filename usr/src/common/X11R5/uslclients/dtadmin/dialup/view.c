/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/view.c	1.19"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <FButtons.h>
#include <MenuShell.h>
#include <PopupWindo.h>
#include <Notice.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

extern void		UnselectSelect();
extern Widget		AddMenu();

static void		FirstCB();
static void		LastCB();
extern void		FindPopupCB();

Arg arg[50];

static Items viewItems[] = {
	{FindPopupCB, NULL, (XA)TRUE},
	{FirstCB, NULL, (XA)TRUE},
	{LastCB, NULL, (XA)TRUE},
};

static Menus viewMenu = {
	"view",
	viewItems,
	XtNumber (viewItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE,
	NULL
};

Widget
AddViewMenu(wid)
Widget wid;
{

	SET_LABEL(viewItems,0,mfind);
	SET_LABEL(viewItems,1,first);
	SET_LABEL(viewItems,2,last);
	return AddMenu (wid, &viewMenu, False);
} /* AddViewMenu */

void
FirstCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	if (sf->numFlatItems == 0) {
		return;
	}
	sf->currentItem = 0;
	UnselectSelect ();
} /* FirstCB */

void
LastCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	if (sf->numFlatItems == 0) {
		return;
	}
	sf->currentItem = sf->numFlatItems-1;
	UnselectSelect ();
} /* LastCB */
