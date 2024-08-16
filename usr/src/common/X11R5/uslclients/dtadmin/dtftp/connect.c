/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:connect.c	1.1.5.1"
#endif

#include "ftp.h"
#include "SWGizmo.h"
#include "TEditGizmo.h"
#include <OpenLookP.h>	/* For ICON_MARGIN */
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <signal.h>
#include <libgen.h>
#include <errno.h>	/* For EEXIST */

extern Widget	Root;

static void
_FtpCmd (CmdPtr cp)
{
	if ((cp->cr->pid = p3open ("ftp -in", cp->cr->fp)) == -1) {
		fprintf (stderr, "can't exec ftp\n");
		exit (1);
	}
}

static void
_User (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	sprintf (
		buf, "user %s %s\n",
		cp->cr->userName, cp->cr->passwd
	);
	Output (cp->cr, buf);
}

extern StateTable	UserTable[];

void
UserCmd (CnxtRec *cr, int group, Priority pri)
{
	QueueCmd ("user", cr, _User, UserTable, group, 0, 0, 0, 0, pri);
}

void
User (CnxtRec *cr)
{
	int	grp = NextCmdGroup();

	UserCmd (cr, grp, High);
}

static void
_OpenCmd (CmdPtr cp)
{
	char		buf[BUF_SIZE];

	sprintf (buf, "open %s\n", cp->cr->systemAddr);
	Output (cp->cr, buf);
}

extern StateTable	OpenTable[];

void
OpenCmd (CnxtRec *cr, int group, Priority pri)
{
	QueueCmd ("open", cr, _OpenCmd, OpenTable, group, 0, 0, 0, 0, pri);
}

void
Open (CnxtRec *cr)
{
	OpenCmd (cr, NextCmdGroup(), Medium);
}

static void
ReconnectCB (Widget wid, CnxtRec *cr, XtPointer callData)
{
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	/* First send a command to ftp to force it to disconnect */
	/* if the idle time infact has been exceeded, then reconnect */
	/* using the status command. */
	TypeCmd (cr);
}

static MenuItems timeoutItems[] = {
	{True,	BUT_RECONNECT,	MNEM_RECONNECT,	NULL,	ReconnectCB},
	{True,	BUT_EXIT,	MNEM_EXIT,	NULL,	ExitCB},
	{True,	BUT_HELP,	MNEM_HELP,	NULL,	NULL},
	{NULL}
};
static MenuGizmo timeoutMenu = {
	NULL, "timeoutMenu", NULL, timeoutItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1
};
static ModalGizmo timeoutModal = {
	NULL, "timeoutModal", TXT_TIMEOUT_TITLE, &timeoutMenu,
	" "
};

static void
CreateTimeOutPopup (CnxtRec *cr)
{
	static ModalGizmo *	TimeoutModal = (ModalGizmo *)0;
	char			buf[BUF_SIZE];
	Widget			w;

	if (TimeoutModal == NULL) {
		TimeoutModal = &timeoutModal;
		CreateGizmo(Root, ModalGizmoClass, TimeoutModal, 0, 0);
	}
	sprintf (
		buf, GGT (TXT_CONNECT_TIMEOUT), cr->systemAddr,
		cr->prop->timeout
	);
	w = (Widget)QueryGizmo (
		PopupGizmoClass, TimeoutModal, GetGizmoWidget, "timeoutMenu"
	);
	OlVaFlatSetValues (w, 0, XtNclientData, (XtPointer)cr, (String)0);
	SetModalGizmoMessage (TimeoutModal, buf);
	MapGizmo (ModalGizmoClass, TimeoutModal);
}

static void
UpdateCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	DirCmd (cr, NextCmdGroup(), Medium);
}

void
WindowManagerEventHandler(wid, client_data, call_data)
Widget		wid;
XtPointer	client_data;
XtPointer	call_data;
{
	PFV			func = (PFV)client_data;
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
		case OL_WM_DELETE_WINDOW:
			FPRINTF ((stdout, "Delete yourself\n"));

		case OL_WM_SAVE_YOURSELF:
			/*
			 *	Do nothing for now; just respond.
			 */
			FPRINTF ((stdout, "Save yourself\n"));
			(func) (wid, 0, call_data);
			break;

		default:
			FPRINTF ((stdout, "Default action\n"));
			OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
			break;
	}
}

extern StateTable FtpTable[];

void
Connect (CnxtRec *cr)
{
	QueueCmd (
		"ftp", cr, _FtpCmd, FtpTable,
		NextCmdGroup(), 0, 0, 0, 0, Medium
	);
}

/*
 * Indicate that the connection to the host has been established.
 */
void
ActivateTimeout (CnxtRec *cr)
{
	static void	ConnectTimeOutTimeOut ();

	/* Send command to host every prop->timeout-15 seconds to prevent */
	/* the prop->timeout second timeout disconnect */
	cr->timeout = XtAddTimeOut (
		(cr->prop->timeout*60-15)*1000,
		(XtTimerCallbackProc)ConnectTimeOutTimeOut, cr
	);
}

/*
 * Reastablish a connection that has been broken because of
 * a cancel command.
 */
void
Reconnect (CnxtRec *cr)
{
	if (cr->connected == True) {
		return;
	}
	MarkOld (cr);
	FPRINTF ((stderr, "reconnect\n"));
	OpenCmd (cr, cr->top->group, High);
}

/*
 * Check the connection by issuing a status command.
 * If the connection is closed it will need to be reestablished.
 */
void
ChkStatus (CnxtRec *cr)
{
	RemoveGroup (cr->top->group);
	TypeCmd (cr);
}

static void
ConnectTimeOutTimeOut (CnxtRec *cr, XtIntervalId *id)
{
	cr->timeout = (XtIntervalId)0;
	/* Don't timeout if the user doesn't want to */
	if (cr->prop->disconnect == True) {
		CreateTimeOutPopup (cr);
		return;
	}
	TypeCmd (cr);
}

void
XConnect (CnxtRec *cr)
{
	cr->connected = False;
}

void
Connected (CnxtRec *cr)
{
	int	i;

	cr->connected = True;
	cr->realSystemAddr = STRDUP (cr->buffer+strlen ("Connected to "));

	i = strlen (cr->realSystemAddr)-1;
	cr->realSystemAddr[i] = '\0';	/* Remove trailing CR */
	if (cr->realSystemAddr[--i] == '.') {
		cr->realSystemAddr[i] = '\0';
	}
#ifdef davef
	cr->prop->timeout = 1;
#endif
	IdleCmd (cr);		/* Get the remote systems idle time */
	SetIdleCmd (cr);	/* Set the remote systems idle time */
}

/* Display select count in left footer
 * and total items in right footer.
 */
int
SetBaseWindowFooter (CnxtRec *cr, int count)
{
	char		buf[BUF_SIZE];
	static char *	selected = NULL;
	static char *	total;
	static char *	types[4];

	if (count == -1) {
		if (cr->dir.cw != NULL) {
			XtVaGetValues (
				cr->dir.cw, XtNselectCount, &count, (String)0
			);
			if (count == -1) {
				count = 0;
			}
		}
	}
	if (selected == NULL) {
		selected = GGT(TXT_SELECTED_ITEMS);
		total = GGT(TXT_TOTAL_ITEMS);
		types[0] = GGT(TXT_SORT_BY_TYPE);
		types[1] = GGT(TXT_SORT_BY_NAME);
		types[2] = GGT(TXT_SORT_BY_SIZE);
		types[3] = GGT(TXT_SORT_BY_TIME);
	}

	/* Only update the footer if it isn't marked as being out-of-date */
	if (cr->outOfDateFooter == False) {
		if (cr->dir.cw != NULL) {
			sprintf (buf, selected, count);
			SetBaseWindowMessage (cr->base, buf);
			sprintf (
				buf,
				total, cr->dir.container->num_objs,
				types[cr->sort]
			);
			SetBaseWindowStatus (cr->base, buf);
		}
		else {
			SetBaseWindowStatus (cr->base, "");
			SetBaseWindowMessage (cr->base, "");
		}
	}
	return count;
}

/*
 * Set the sensitivity of Open, Rename, Delete, and Copy buttons
 */
void
SetMultiSelectSensitivity (CnxtRec *cr, int count)
{
	Boolean		sensitive = True;
	Widget		fileMenu;
	Widget		barMenu;
	Widget		editMenu;
	DmItemPtr	item;
	DmContainerPtr	cntrp = cr->dir.container;
	DmObjectPtr	op;
	int		i;
	int		dir = 0;
	int		other = 0;
	int		exec = 0;

	editMenu = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "editMenu"
	);
	fileMenu = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "fileMenu"
	);
	barMenu = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "barMenu"
	);

	OlVaFlatSetValues (fileMenu, 0, XtNsensitive, True, (String)0);
	OlVaFlatSetValues (fileMenu, 4, XtNsensitive, True, (String)0);

	count = SetBaseWindowFooter (cr, count);
	if (count != 1) {
		sensitive = False;
	}

	/* Set sensitivity of edit:rename button and menuBar:rename. */
	OlVaFlatSetValues (editMenu, 1, XtNsensitive, sensitive, (String)0);
	/* Set sensitivity of buttonbar:rename button. */
	OlVaFlatSetValues (barMenu, 5, XtNsensitive, sensitive, (String)0);

	sensitive = True;
	if (count == 0) {
		sensitive = False;
	}

	/* Set edit:copy, edit:delete, edit:properties */
	OlVaFlatSetValues (editMenu, 0, XtNsensitive, sensitive, (String)0);
	OlVaFlatSetValues (editMenu, 2, XtNsensitive, sensitive, (String)0);
	OlVaFlatSetValues (editMenu, 5, XtNsensitive, sensitive, (String)0);

	/* Set buttonbar:copy, buttonbar:delete */
	OlVaFlatSetValues (barMenu, 2, XtNsensitive, sensitive, (String)0);
	OlVaFlatSetValues (barMenu, 3, XtNsensitive, sensitive, (String)0);
	OlVaFlatSetValues (barMenu, 6, XtNsensitive, sensitive, (String)0);

	/* Set sensitivity of edit:DeselectAll and SelectAll */
	OlVaFlatSetValues (editMenu, 4, XtNsensitive, sensitive, (String)0);
	sensitive = True;
	if (
		cr->dir.container->num_objs == 0 ||
		count == cr->dir.container->num_objs
	) {
		sensitive = False;
	}
	OlVaFlatSetValues (editMenu, 3, XtNsensitive, sensitive, (String)0);

	/* Open is insensitive if more than one object is selected */
	/* and only directories are selected. */
	sensitive = False;
	if (count != 0) {
		item = cr->dir.itp;
		for (i=0; i<cntrp->num_objs; item++, i++) {
			if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
				op = (DmObjectPtr)item->object_ptr;
				if (op->ftype == DM_FTYPE_DIR) {
					dir += 1;
				}
				else if (op->ftype == DM_FTYPE_EXEC) {
					exec += 1;
				}
				else {
					other += 1;
				}
			}
		}
		sensitive = True;
		/* Don't allow print or open if any file is an executable */
		if (exec != 0) {
			sensitive = False;
		}
		else {
			/* Don't allow open or print if more than one dir */
			/* is selected or a dir and other files are selected */
			if (dir > 1) {
				sensitive = False;
			}
			if (dir > 0 && count != 1) {
				sensitive = False;
			}
		}
	}
	/* Set sensitivity of file:open & file:print button. */
	/* Set sensitivity of buttonbar:open button and buttonbar print. */
	OlVaFlatSetValues (fileMenu, 1, XtNsensitive, sensitive, (String)0);
	OlVaFlatSetValues (barMenu, 2, XtNsensitive, sensitive, (String)0);
	if (count == 1) {
		if (dir == 1) {
			sensitive = False;
		}
	}
	OlVaFlatSetValues (fileMenu, 2, XtNsensitive, sensitive, (String)0);
	OlVaFlatSetValues (barMenu, 4, XtNsensitive, sensitive, (String)0);
}

static void
SelectUnselect (CnxtRec *cr, Boolean select)
{
	int		i;
	DmItemPtr	item;
	DmContainerPtr	cntrp;
	int		cnt = 0;

	cntrp = cr->dir.container;
	item = cr->dir.itp;
	for (i=0; i<cntrp->num_objs; item++, i++) {
		cnt += 1;
		item->select = select;
	}
	if (cnt != 0) {
		if (cr->dir.cw != (Widget)0) {
			XtVaSetValues (
				cr->dir.cw,
				XtNitemsTouched,	True,
				XtNselectCount,		(select ? cnt : 0),
				(String)0
			);
			UpdateCopyPopup (cr);
		}
	}
	SetMultiSelectSensitivity (cr, (select ? cnt : 0));
}

static void SelectAllCmd();
static void UnselectAllCmd();

static void
_SelectAllCmd (CmdPtr cp)
{
	CnxtRec *	cr = cp->cr;

	if (cr->dirInProgress == True) {
		SelectAllCmd (cr);
	}
	else {
		SelectUnselect (cr, True);
	}
}

static void
_UnselectAllCmd (CmdPtr cp)
{
	CnxtRec *	cr = cp->cr;

	if (cr->dirInProgress == True) {
		UnselectAllCmd (cr);
	}
	else {
		SelectUnselect (cr, False);
	}
}

static void
UnselectAllCmd (CnxtRec *cr)
{
	QueueCmd (
		"Select all wait for dir",
		cr, _UnselectAllCmd, NULL, NextCmdGroup(), 0, 0, 0, 0, Low
	);
}

static void
SelectAllCmd (CnxtRec *cr)
{
	QueueCmd (
		"Select all wait for dir",
		cr, _SelectAllCmd, NULL, NextCmdGroup(), 0, 0, 0, 0, Low
	);
}

static void
SelectAllCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	SelectAllCmd (cr);
}

static void
UnselectAllCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	UnselectAllCmd (cr);
}

void
DeleteFile (CnxtRec *cr, DmObjectPtr op)
{
	if (op->ftype == DM_FTYPE_DIR) {
		RmDirCmd (cr, op->name);
	}
	else {
		DelCmd (cr, op->name, NextCmdGroup ());
	}
}

static void
DeleteCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);
	int		i;
	DmObjectPtr	op;
	DmItemPtr	item;
	DmContainerPtr	cntrp;
	Boolean		found = False;

	cntrp = cr->dir.container;
	item = cr->dir.itp;
	for (i=0; i<cntrp->num_objs; item++, i++) {
		if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
			op = (DmObjectPtr)item->object_ptr;
			DeleteFile (cr, op);
			found = True;
		}
	}
	if (found == True) {
		DirCmd (cr, NextCmdGroup(), Medium);
	}
}

static void
AbortCB (Widget wid, XtPointer clientData, XtPointer callData)
{
	CnxtRec *	cr = FindCnxtRec(wid);
	CnxtRec *	copycr = cr->copycr;
	DropObject *	dobj;

	FPRINTF ((stderr, "KILL %d\n", cr->pid));
	kill (cr->pid, SIGINT);

	/* Free up the command queue.*/
	while (cr->top != (CmdPtr)0) {
		/* If a free routine was supplied then call that */
		/* Otherwise, call FreeDobj() */
		if (cr->top->free != NULL) {
			(cr->top->free)(cr->top->clientData);
		}
		else {
			dobj = (DropObject *)cr->top->clientData;
			FreeDobj (dobj);
		}
		RemoveCmd (cr, cr->top);
	}
	/* If a dir was in progress then this flag may have been set */
	ResetDirFlag (cr);
	/* If a dir was in progress there could be pending requests */
	MarkOld (cr);
	cr->unresolvedLinks = 0;

	/* Bring down the slider popup if it's up */
	if (copycr != NULL) {
		FPRINTF ((stderr, "KILL %d\n", cr->pid));
		kill (copycr->pid, SIGINT);
		if (copycr->sliderUp == True) {
			PopDownPopup (copycr->top);
		}
		while (copycr->top != (CmdPtr)0) {
			/* If a free routine was supplied then call that */
			/* Otherwise, call FreeDobj() */
			if (copycr->top->free != NULL) {
				(copycr->top->free)(copycr->top->clientData);
			}
			else {
				dobj = (DropObject *)copycr->top->clientData;
				FreeDobj (dobj);
			}
			RemoveCmd (copycr, copycr->top);
		}
		InvCmd (copycr);
	}
	/* Sync up dtftp and ftp by looking for the ftp> prompt */
	InvCmd (cr);
	CdCmd (cr, cr->pwd, NextCmdGroup(), Medium);
}

static void
DisplayMessages (CnxtRec *cr, Boolean show)
{
	Widget		menu;
	Widget		sw;
	Widget		lw;
	char *		label;

	lw = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget,
		"messageLabel"
	);
	sw = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget,
		"lowerSW"
	);

	/* Switch the label on the view button */
	if (show == True) {
		label = GGT (BUT_HIDE_MSG);
		XtManageChild (sw);
		XtManageChild (lw);
	}
	else {
		label = GGT (BUT_VIEW_MSG);
		XtUnmanageChild (sw);
		XtUnmanageChild (lw);
	}

	menu = QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "viewMenu"
	);
	OlVaFlatSetValues (menu, 3, XtNlabel, label, (String)0);

	cr->showMessages = show;
}

static void
MessageCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	DisplayMessages (cr, !cr->showMessages);
}

static void
NewConnectCB (Widget w, XtPointer clientData, XtPointer callData)
{
	char *	path;
	char	buf[BUF_SIZE];

	path = (char *)GetXWINHome ("bin");
	sprintf (buf, "RACAPP=%s %s/rac_acc.sh -f &", path, path);
	system (buf);
}

static MenuItems fileItems[] = {
	{False, BUT_NEW_FOLDER,	MNEM_NEW_FOLDER, NULL, NewFolderCB},
	{False, BUT_OPEN,	MNEM_OPEN,	 NULL, OpenCB},
	{False, BUT_PRINT,	MNEM_PRINT,	 NULL, PrintCB},
	{True,  BUT_NEW_C,	MNEM_NEW_C,	 NULL, NewConnectCB},
	{False, BUT_RESET_C,	MNEM_RESET_C,	 NULL, AbortCB},
	{True,  BUT_EXIT,	MNEM_EXIT,	 NULL, ExitCB},
	{NULL}
};

static MenuGizmo fileMenu = {
	NULL, "fileMenu", NULL, fileItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems editItems[] = {
	{True, BUT_COPY_DDD,	MNEM_COPY,	 NULL, CopyCB},
	{True, BUT_EDIT_RENAME,	MNEM_RENAME,	 NULL, EditRenameCB},
	{True, BUT_DELETE,	MNEM_DELETE,	 NULL, DeleteCB},
	{True, BUT_SEL_ALL,	MNEM_SEL_ALL,	 NULL, SelectAllCB},
	{True, BUT_DESEL_ALL,	MNEM_DESEL_ALL,	 NULL, UnselectAllCB},
	{True, BUT_FPROPERTIES,	MNEM_FPROPERTIES,NULL, PropertyCB},
	{True, BUT_CPROPERTIES,	MNEM_CPROPERTIES,NULL, ConnectionPropertyCB},
	{NULL}
};

static MenuGizmo editMenu = {
	NULL, "editMenu", NULL, editItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems sortItems[] = {
	{True, BUT_BY_TYPE,	MNEM_BY_TYPE,	NULL, NULL},
	{True, BUT_BY_NAME,	MNEM_BY_NAME,	NULL, NULL},
	{True, BUT_BY_SIZE,	MNEM_BY_SIZE,	NULL, NULL},
	{True, BUT_BY_TIME,	MNEM_BY_TIME,	NULL, NULL},
	{NULL},
};

static MenuGizmo sortMenu = {
	NULL, "sortMenu", NULL, sortItems,
	SortCB, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems formatItems[] = {
	{True, BUT_ICONS,	MNEM_ICONS,	NULL, NULL},
	{True, BUT_SHORT,	MNEM_SHORT,	NULL, NULL},
	{True, BUT_LONG,	MNEM_LONG,	NULL, NULL},
	{NULL},
};

static MenuGizmo formatMenu = {
	NULL, "formatMenu", NULL, formatItems,
	FormatCB, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems viewItems[] = {
	{True, BUT_SORT,	MNEM_SORT,	(char *)&sortMenu},
	{True, BUT_FORMAT,	MNEM_FORMAT,	(char *)&formatMenu},
	{True, BUT_UPDATE,	MNEM_UPDATE,	NULL, UpdateCB},
	{True, BUT_HIDE_MSG,	MNEM_HIDE_MSG,	NULL, MessageCB},
	{NULL}
};

static MenuGizmo viewMenu = {
	NULL, "viewMenu", NULL, viewItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems foldersItems[MAX_FOLDERS+NUM_FOLDERS+1] = {
	{True, BUT_PARENT,	MNEM_PARENT},
#ifdef DEBUG
	{True, "Other...",	"O"},
#endif
	{True, "1", "1"},
	{True, "2", "2"},
	{True, "3", "3"},
	{True, "4", "4"},
	{True, "5", "5"},
	{True, "6", "6"},
	{True, "7", "7"},
	{NULL}
};

static MenuGizmo foldersMenu = {
	NULL, "foldersMenu", NULL, foldersItems,
	FoldersCB, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mainItems[] = {
	{True,  BUT_FILE,	MNEM_FILE,	 (char *)&fileMenu},
	{False, BUT_EDIT,	MNEM_EDIT,	 (char *)&editMenu},
	{False, BUT_VIEW,	MNEM_VIEW,	 (char *)&viewMenu},
	{False, BUT_FOLDERS,	MNEM_FOLDERS,	 (char *)&foldersMenu},
	{False, BUT_HELP,	MNEM_HELP,	 NULL, NULL},
	{NULL}
};

static MenuGizmo mainMenu = {
	NULL, "mainMenu", NULL, mainItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

#define T	(XtPointer)True
#define F	(XtPointer)False
static MenuItems barItems [] = {
	{False,	BUT_PARENT,	MNEM_PARENT,	 NULL, ParentDirCB, T},
	{False,	" ",		"1",		 NULL, NULL,	F},
	{False, BUT_OPEN,	MNEM_OPEN,	 NULL, OpenCB,	T},
	{False, BUT_COPY_DDD,	MNEM_COPY,	 NULL, CopyCB,	T},
	{False, BUT_PRINT,	MNEM_PRINT,	 NULL, PrintCB,	T},
	{False, BUT_EDIT_RENAME, MNEM_RENAME,	 NULL, EditRenameCB, T},
	{False, BUT_DELETE,	MNEM_BAR_DELETE, NULL, DeleteCB,T},
	{False,	" ",		"2",		 NULL, NULL,	F},
	{False, BUT_RESET_C,	MNEM_RESET_C,	 NULL, AbortCB,	T},
	{NULL}
};

static MenuGizmo barMenu = {
	NULL, "barMenu", NULL, barItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ScrolledWindowGizmo upperSW = {
	"upperSW", 0, 0, NULL, 0
};

static StaticTextGizmo messageLabel = {
	NULL, "messageLabel", TXT_MESSAGES, NorthWestGravity
};

static TextEditGizmo textEdit = {
		"textEdit", 4
};
static GizmoRec editArray [] = {
	{TextEditGizmoClass,	&textEdit}
};
static ScrolledWindowGizmo lowerSW = {
	"lowerSW", 0, 0, editArray, XtNumber (editArray)
};

static GizmoRec scrolledWindows[] = {
	{MenuBarGizmoClass,		&barMenu},
	{ScrolledWindowGizmoClass,	&upperSW},
	{StaticTextGizmoClass,		&messageLabel},
	{ScrolledWindowGizmoClass,	&lowerSW},
};

BaseWindowGizmo FtpWindow = {
	NULL,
	"FtpWindow",
	"ftp",
	&mainMenu,
	scrolledWindows,
	XtNumber (scrolledWindows),
	TXT_FTP_ICON_NAME,
	"dtftp.48",
	" ",
	" ",
	50,
};

static void
ConnectionProperties (CnxtRec *cr)
{
	Systems *	sp;
	Systems *	last = (Systems *)0;

	for (sp=ftp->systems; sp; sp=sp->next) {
		if (strcmp (sp->systemAddr, cr->systemAddr) == 0) {
			break;
		}
		last = sp;
	}
	if (sp != (Systems *)0) {
		cr->prop = sp->prop;
	}
	else {
		cr->prop = (ConnectionProp *) CALLOC (
			1, sizeof (ConnectionProp)
		);
		SetDefaultProperties (cr->prop);
		sp = (Systems *)CALLOC (1, sizeof (Systems));
		sp->prop = cr->prop;
		sp->systemAddr = cr->systemAddr;
		if (last != (Systems *)0) {
			last->next = sp;
		}
		else {
			ftp->systems = sp;
		}
		UpdateSystemsInfo ();
	}
}

static CnxtRec *
CreateConnectionRecord (char *userName, char *systemAddr)
{
	CnxtRec *	cr = (CnxtRec *) CALLOC (1, sizeof (CnxtRec));
#ifdef DEBUG
	static int	id = 0;

	cr->id = id++;
#endif
	if (cr == (CnxtRec *)0) {
		FPRINTF ((stderr, "Can't malloc\n"));
		exit(1);
	}
	cr->systemAddr = (char *)STRDUP (systemAddr);
	cr->userName = (char *)STRDUP (userName);
	cr->timeout = (XtIntervalId)0;
	cr->sliderPopup = (ModalGizmo *)0;
	cr->fp[0] = (FILE *)0;
	cr->fp[1] = (FILE *)0;
	cr->numFolders = 0;
	cr->passwd = NULL;
	cr->copyCnxt = False;
	cr->showMessages = True;
	cr->outOfDateFooter = False;
	cr->sort = ByType;
	cr->format = DM_NAME;
	cr->hardCoded = True;	/* Pixmaps need to be gotten from dtm */
	cr->top = (CmdPtr)0;
	cr->queues = (CmdQueue *)MALLOC (sizeof(CmdQueue));
	cr->queues->high = (CmdPtr)0;
	cr->queues->highBottom = (CmdPtr)0;
	cr->queues->med = (CmdPtr)0;
	cr->queues->medBottom = (CmdPtr)0;
	cr->queues->low = (CmdPtr)0;
	cr->queues->lowBottom = (CmdPtr)0;
	cr->queues->scum = (CmdPtr)0;
	cr->queues->scumBottom = (CmdPtr)0;
	cr->pwd = NULL;
	cr->messages = (char *)CALLOC (1, sizeof (char));
	cr->messageMarker = NULL;
	cr->copycr = NULL;
	cr->tmpdir = NULL;
	cr->tmpfile = STRDUP (tmpnam (0));

	Connect (cr);

	ConnectionProperties (cr);

	/* Insert new connection at end of list */
	if (ftp->first == (CnxtRec *)0) {
		cr->next = cr;
		ftp->first = cr;
	}
	else {
		cr->next = ftp->first;
		ftp->last->next = cr;
	}
	ftp->last = cr;
	return cr;
}

void
BaseWindowTitle (CnxtRec *cr)
{
	char	title[BUF_SIZE];
	char *	path;

	path = cr->pwd;
	if (path != NULL && path[0] != '\0') {
		path = ftp->showFullPaths ? path : basename(path);
	}
	sprintf (
		title, GGT(TXT_FTP_TITLE), cr->systemAddr, cr->userName, path
	);
	SetBaseWindowTitle (cr->base, title);
	sprintf (title, "%s: %s", cr->systemAddr, cr->userName);
	XtVaSetValues (cr->base->shell, XtNiconName, title, (String)0);
}

CnxtRec *
CreateConnection (char *userName, char *systemAddr)
{
	Arg			arg[10];
	Widget			shell;
	LabelGizmo *		label;
	DirPtr			dirp;
	CnxtRec *		cr;
	Widget			w;
	Dimension		margin;
	static char *		flatMenuFields[] = {
		XtNsensitive,  /* sensitive			 */
		XtNlabel,      /* label				 */
		XtNuserData,   /* mnemonic string		 */
		XtNuserData,   /* nextTier | resource_value	 */
		XtNselectProc, /* function			 */
		XtNmanaged,
		XtNset,	       /* set				 */
		XtNpopupMenu,  /* button			 */
		XtNmnemonic,   /* mnemonic			 */
	};
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)ExitCB},
		{NULL}
	};

	cr = CreateConnectionRecord (userName, systemAddr);
	cr->tmpdir = ResolvePath (cr->prop->transferFolder);

	/* Set the height and width of the window */
	margin = Ol_PointToPixel(OL_HORIZONTAL,ICON_MARGIN)*5;
	FPRINTF ((stderr, "margin = %d\n", margin));
	margin *= 2;
	FPRINTF ((stderr, "margin = %d\n", margin));
	upperSW.width = ftp->folderCols * ftp->gridWidth + margin;
	upperSW.height = ftp->folderRows * ftp->gridHeight + margin;
	lowerSW.width = upperSW.width;
	cr->base = CopyGizmo (BaseWindowGizmoClass, &FtpWindow);

	dirp = &cr->dir;
	dirp->start = (DmObjectPtr)0;
	dirp->end = (DmObjectPtr)0;
	dirp->container = (DmContainerPtr)CALLOC (1, sizeof (DmContainerRec));
	dirp->container->path = STRDUP ("/usr/davef");
	dirp->cw = (Widget)0;
	cr->dir.itp = NULL;

	XtSetArg(arg[0], XtNwmProtocol, protocolCB);
	shell = CreateGizmo (
		Root, BaseWindowGizmoClass, cr->base, arg, 1
	);
	/* Add event handler to each connection to handle DtEnqueueRequests */
	XtAddEventHandler(
		shell, (EventMask)StructureNotifyMask, True,
		(XtEventHandler) EventHandler, (XtPointer)cr
	);
	/* Set up callback for drops of the iconified window */
	OlDnDRegisterDDI (
		cr->base->icon_shell, OlDnDSitePreviewNone,
		TriggerCB, (OlDnDPMNotifyProc)NULL, True,
		(XtPointer)NULL
	);

	w = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "foldersMenu"
	);
	XtVaSetValues (
		w, XtNnumItems, cr->numFolders+NUM_FOLDERS, (String)0
	);
	w = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "textEdit"
	);
	XtVaSetValues (w, XtNlinesVisible, 4, (String)0);

	w = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget,
		"messageLabel"
	);
	XtVaSetValues (w, XtNweight, 0, (String)0);

	BaseWindowTitle (cr);
	DisplayMessages (cr, cr->showMessages);
	MapGizmo (BaseWindowGizmoClass, cr->base);

	w = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "barMenu"
	);
	XtVaSetValues (
		w,
		XtNitemFields,		flatMenuFields,
		XtNnumItemFields,	XtNumber(flatMenuFields),
		XtNweight,		0,
		(String)0
	);

	return cr;
}

CnxtRec *
CreateCopyConnection (CnxtRec *cr)
{
	CnxtRec *	ncr;

	ncr = CreateConnectionRecord (cr->userName, cr->systemAddr);
	ncr->base = cr->base;
	ncr->passwd = STRDUP (cr->passwd);
	ncr->copyCnxt = True;
	HashCmd (ncr, NextCmdGroup());
	return ncr;
}

static void
FreeFolderMenu (CnxtRec *cr)
{
	MenuGizmo *	g;
	MenuItems *	item;
	int		i;

	g = QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoGizmo, "foldersMenu"
	);
	item=g->items+1;	/* Skip over Parent */
#ifdef DEBUG
	item += 1;		/* Skip over Other */
#endif
	for (i=0; i<cr->numFolders; i++) {
		MYFREE (item->label);
		item += 1;
	}
}

void
RemoveConnection (CnxtRec *cr)
{
	DirPtr		dirp = &cr->dir;
	CnxtRec *	tp;
	CnxtRec *	last;
	CmdPtr		cp;
	CmdPtr		lcp;
	int		i;

	/*
	 * First free up the data associated with the connection.
	 */
	Output (cr, "quit\n");
	if (dirp->container != NULL) {
		MYFREE (dirp->container->path);
		MYFREE (dirp->container);
	}
	FreeFolderMenu (cr);
	if (cr->base != (BaseWindowGizmo *)0) {
		XtDestroyWidget (GetBaseWindowShell (cr->base));
		FreeGizmo (BaseWindowGizmoClass, cr->base);
	}
	if (cr->sliderPopup != (ModalGizmo *)0) {
		XtDestroyWidget (GetModalGizmoShell (cr->sliderPopup));
		FreeGizmo (ModalGizmoClass, cr->sliderPopup);
	}
	for (i=0; i<cr->lineIndex; i++) {
		MYFREE (cr->lineHistory[i]);
	}
	FreePropertyList (cr);
	MYFREE (cr->messages);
	MYFREE (cr->systemAddr);

	/*
	 * Close the connection to ftp.
	 */
	FPRINTF ((stderr, "KILL %d\n", cr->pid));
	kill (cr->pid, SIGINT);	/* First get ftp's attention */
	p3close (cr->fp);

	/*
	 * Free up the command queue.
	 */
	while (cr->top != (CmdPtr)0) {
		RemoveCmd (cr, cr->top);
	}
	MYFREE (cr->queues);

	/*
	 * Next, remove the connection from the list
	 */
	last = ftp->last;
	tp = ftp->last;
	do {
		tp = tp->next;
		if (tp == cr) {
			break;
		}
		last = tp;
	} while (tp != ftp->last);
	last->next = cr->next;
	if (ftp->first == cr) {
		ftp->first = cr->next;
	}
	if (ftp->last == cr) {
		ftp->last = last;
	}
	if (ftp->current == cr) {
		ftp->current = ftp->first;
	}

	if (ftp->current == cr) {
		ftp->current = (CnxtRec *)0;
		ftp->first = (CnxtRec *)0;
		ftp->last = (CnxtRec *)0;
	}
	if (cr->tmpdir != NULL) {
		MYFREE (cr->tmpdir);
	}
	if (cr->tmpfile != NULL) {
		unlink (cr->tmpfile);
		MYFREE (cr->tmpfile);
	}
	MYFREE (cr->prop->transferFolder);
	MYFREE (cr->prop);
	MYFREE (cr->home);
	MYFREE (cr->pwd);
	MYFREE (cr);
}

Boolean
CollectMsg (CnxtRec *cr)
{
	Widget	w;
	int	x;
	char	buf[10];

	/* Check for start of message */
	if (cr->messageMarker == NULL) {
		if (cr->buffer[3] != '-') {
			return False;
		}
		strncpy (buf, cr->buffer, 3);
		buf[3] = ' ';
		buf[4] = '\0';
		cr->messageMarker = STRDUP (buf);
		if (cr->copyCnxt == True) {
			return True;
		}
	}
	if (cr->copyCnxt == False) {
		cr->messages = (char *)REALLOC (
			cr->messages,
			strlen (cr->messages)+strlen (cr->buffer)+1
		);
	}
	if (cr->copyCnxt == False) {
		strcat (cr->messages, cr->buffer);
	}
	if (cr->buffer[3] == '\n') {
		if (strncmp (cr->buffer, cr->messageMarker, 3) != 0) {
			return True;
		}
	}
	else if (strncmp (cr->buffer, cr->messageMarker, 4) != 0) {
		return True;
	}
	MYFREE (cr->messageMarker);
	cr->messageMarker = NULL;
	if (cr->copyCnxt == False) {
		/* Display the messages */
		w = (Widget)QueryGizmo (
			BaseWindowGizmoClass, cr->base,
			GetGizmoWidget, "textEdit"
		);
		OlTextEditGetLastPosition (w, &x);
		XtVaSetValues (w, XtNsource, cr->messages, (String)0);
		OlTextEditSetCursorPosition (w, x, x, x);
		if (cr->showMessages == True) {
			DisplayMessages (cr, True);
		}
		else {
			SetBaseWindowMessage (cr->base, GGT (TXT_NEW_MESSAGE));
			ftp->outOfDateFooter += 1;
			cr->outOfDateFooter = True;
		}
	}
	return False;
}

/*
 * Insert this path into the folder menu
 */
void
AddToFolderMenu (CnxtRec *cr, char *filename)
{
	int			n;
	int			cnt;
	MenuGizmo *		g;
	MenuItems *		items;
	MenuItems *		item;
	Boolean			found = False;

	g = QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoGizmo,
		"foldersMenu"
	);
	items = g->items+NUM_FOLDERS;

	for (item=items; item<items+cr->numFolders; item++) {
		if (strcmp (item->label, filename) == 0) {
			found = True;
			if (item != items) {
				break;
			}
			return;
		}
	}
	if (cr->numFolders == MAX_FOLDERS) {
		if (found == False) {
			item -= 1;
			found = True;
		}
	}
	else {
		if (found == False) {
			cr->numFolders += 1;
		}
	}
	if (found == True) {
		/* Free any path that will "fall off the bottom" */
		MYFREE (item->label);
	}
	cnt = (item - items) * sizeof(MenuItems);

	/* Move all of the items down one */
	memmove (items + 1, items, cnt);

	items->label = STRDUP (filename);
	items->mnemonic = NULL;
	items->real_mnemonic = '\0';
	XtVaSetValues (
		GetMenu (g),
		XtNitems,	g->items,
		XtNnumItems,	cr->numFolders+NUM_FOLDERS,
		(String)0
	);
}
