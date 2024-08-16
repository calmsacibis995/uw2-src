/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:exit.c	1.1.3.1"
#endif

#include "ftp.h"

void
ExitDtftp (CnxtRec *cr)
{
	CnxtRec *	ccr;

	/* Examine the list of connections to be sure that this isn't */
	/* one of the copy connections.  If it is a copy connection */
	/* then start with the real connection. */
	ccr = ftp->first;
	do {
		if (cr == ccr->copycr) {
			cr = ccr;
			break;
		}
		ccr = ccr->next;
	} while (ccr != ftp->first);

	ccr = cr->copycr;
	RemoveConnection (cr);
	if (ccr != (CnxtRec *)0) {
		RemoveConnection (ccr);
	}
	if (ftp->current == (CnxtRec *)0) {
		exit (1);
	}
}

static ModalGizmo *	CopyUpModal = (ModalGizmo *)0;

void
ReallyExitCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	RestartQueues (cr, NotSuspended);
	ExitDtftp (cr);
}

static void
CancelExitCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	RestartQueues (cr, NotSuspended);
}

static MenuItems copyUpItems[] = {
	{True,	BUT_EXIT,	MNEM_EXIT,	NULL, ReallyExitCB},
	{True,	BUT_DONT_EXIT,	MNEM_DONT_EXIT,	NULL, CancelExitCB},
	{True,	BUT_HELP,	MNEM_HELP,	NULL, NULL},
	{NULL}
};

static MenuGizmo copyUpMenu = {
	NULL, "copyUpMenu", NULL, copyUpItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo copyUpModal = {
	NULL, "copyUpModal", TXT_IN_PROGRESS_TITLE, &copyUpMenu,
	TXT_COPY_IN_PROGRESS
};

void
ExitCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *		cr = FindCnxtRec (wid);
	Widget			w;

	if (cr->copycr != NULL && cr->copycr->sliderUp == True) {
		/* Warn the user that a copy is in progress */
		if (CopyUpModal == (ModalGizmo *)0) {
			CopyUpModal = &copyUpModal;
			CreateGizmo(Root, ModalGizmoClass, CopyUpModal, 0, 0);
		}
		w = (Widget)QueryGizmo (
			ModalGizmoClass, CopyUpModal, GetGizmoWidget,
			"copyUpMenu"
		);
		OlVaFlatSetValues (w, 0, XtNclientData, cr, (String)0);
		OlVaFlatSetValues (w, 1, XtNclientData, cr, (String)0);
		ftp->suspended = StartSuspend;
		MapGizmo (ModalGizmoClass, CopyUpModal);
	}
	else {
		ExitDtftp (cr);
	}
}
