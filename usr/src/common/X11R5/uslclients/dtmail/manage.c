/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:manage.c	1.106"
#endif

#define MANAGE_C

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/Flat.h>
#include <Xol/FList.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <Xol/OlCursors.h>      /* For OlGetBusyCursor */
#include <X11/Shell.h>			/* need this for XtNtitle */
#include "mail.h"
#include "RMailGizmo.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>

extern HelpInfo		ManagerHelp;
extern ReadRec *	readRec;
extern int		LastSelectedMessage;
extern MailRec *	mailRec;
extern int		Version;
extern Widget		Root;
extern Boolean		SendOnly;
extern char *		UserName;
extern BaseWindowGizmo  AliasWindow;
extern AliasRec *       aliasRec;
extern uid_t		UserId;
extern DblCMessage      dblCMessage;
extern char * 		PrintCommand;

typedef enum {
	MenuOpenUser, MenuOpenNewUser, MenuSave, MenuSaveAs, MenuPrint,
	MenuAlias, MenuProperties, MenuExit
} MenuFileItemIndex;

typedef enum {
	MenuUndo, MenuDelete, MenuUndelete, MenuSelect, MenuUnselect
} MenuEditItemIndex;

typedef enum {
	MenuRead, MenuReadNew, MenuMailer, MenuReplySender, MenuReplySenderAtt,
	MenuReplyAll, MenuReplyAllAtt, MenuForward
} MenuMailItemIndex;

typedef enum {
	MenuFile, MenuEdit, MenuMail, MenuHelp
} MenuMenuItemIndex;

ManageRec *	manageRec = (ManageRec *)0;
int managecnt = 0;

#define DELETE_BUTTON	0
#define UNDELETE_BUTTON	1
#define FILE_BUTTON	0
#define EDIT_BUTTON	1
#define UPDATE_BUTTON	2

#define PRINT_FAILED  "Pipe to: \"%s\"\nUX:mailx: ERROR: Pipe to \"%s\" failed\n"

static void AdjustMenuBarButton(MailRec *	mp);

ManageRec * 
FindManageRec(wid)
Widget wid;
{
	ManageRec *	mng;
	Widget		shell;

	shell = GetToplevelShell (wid);
	if ((mng = manageRec) == (ManageRec *)0)
		return (ManageRec *)0;
	do
	{
		if (shell == GetBaseWindowShell (mng->baseWGizmo)) {
			return mng;
		}
	} while (mng = mng->mngnext, mng != manageRec);
	return (ManageRec *)0;
}

ListGizmo *
GetSummaryListGizmo (mng)
ManageRec *	mng;
{
	return (ListGizmo *) QueryGizmo (
		BaseWindowGizmoClass, mng->baseWGizmo,
		GetGizmoGizmo, SUMMARY_LIST
	);
}

Widget
GetSummaryListWidget (mng)
ManageRec *	mng;
{
	if (mng == (ManageRec *)0) {
		return (Widget)0;
	}
	return (Widget) QueryGizmo (
		BaseWindowGizmoClass, mng->baseWGizmo,
		GetGizmoWidget, SUMMARY_LIST
	);
}

Widget
GetSTextHeaderWidget (mng)
ManageRec *     mng;
{
        if (mng == (ManageRec *)0) {
                return (Widget)0;
        }
        return (Widget) QueryGizmo (
                BaseWindowGizmoClass, mng->baseWGizmo,
                GetGizmoWidget, "header_giz"
        );
}

/* Update the status field of the current mail message.
 * This is accomplished by rereading the header for the message
 * and extracting the status field.
 */

void
DisplayStatus (mp, buf)
MailRec *	mp;
char *		buf;
{
	int		item;
	int		i;
	char *		string;
	char *		tmp;
	char *		savept = NULL;
	ListGizmo *	gizmo;

	tmp = buf;
	while ((string = MyStrtok (tmp, "\n", &savept)) != NULL) {
		sscanf (string+2, "%d", &item);
		/* Look backwards because the index will always be
		 * less than the message number (item).
		 */
		i = (item>mp->summary->size)
			? mp->summary->size-1
			: item-1;
		for (; i>=0; i--) {
			if (item == mp->summary->items[i].clientData) {
				break;
			}
		}
		tmp = NULL;
		SetStatusField (
			string+1,
			(char **)mp->summary->items[i].fields
		);
		if (mp->mng != (ManageRec *)0) {
			OlVaFlatSetValues (
				GetSummaryListWidget (mp->mng),
				i,
				XtNformatData, mp->summary->items[i].fields,
				(String)0
			);
		}
	}
}

static Boolean
DisplayInRightFooter (bw, buf)
BaseWindowGizmo *       bw;
char *			buf;
{
	int i = strlen (buf)-1;
	if (buf [i] == '\n') {
		buf[i] = '\0';
	}
	SetBaseWindowStatus (bw, buf);
	return True;
}

void
UpdateFooter (mp)
MailRec *mp;
{
	static Pixmap	rdmail = 0;
	static Pixmap	nrdmail;
	static Pixmap	manmail;
	static Pixmap	nmanmail;
	ReadRec *	rp;
	char **		tmp;
	static int	height;
	static int	width;
	int		i;
	int		new = 0;
	int		unread = 0;
	char		buf[256];
	Widget		shell;

	for (i=0; i<mp->summary->size; i++) {
		tmp = (char **)mp->summary->items[i].fields;
		switch (tmp[F_TEXT][F_STATUS]) {
			case 'N': 
				new += 1;
			case 'U': 
				unread += 1;
				break;
			case 'D': 
			case 'O': 
			case 'H': 
			case 'R': 
			case 'S': 
			case 'M': 
				break;
		}
	}
	sprintf (
		buf, GetGizmoText (TXT_MANAGE_RIGHT_FOOTER),
		mp->summary->size, new, unread
	);
	if (mp->mng != (ManageRec *)0) {
		DisplayInRightFooter (mp->mng->baseWGizmo, buf);
	}
	if (rdmail == (Pixmap)0) {
		rdmail = PixmapOfFile (
			Root,
			"rdmail.icon",
			GetGizmoText (TXT_READER_ICON_NAME),
			&width, &height
		);
		nrdmail = PixmapOfFile (
			Root,
			"nrdmail.icon",
			GetGizmoText (TXT_READER_ICON_NAME),
			&width, &height
		);
		manmail = PixmapOfFile (
			Root,
			"manmail.icon",
			GetGizmoText (TXT_E_MAIL_ICON_NAME),
			&width, &height
		);
		nmanmail = PixmapOfFile (
			Root,
			"nmanmail.icon",
			GetGizmoText (TXT_E_MAIL_ICON_NAME),
			&width, &height
		);
	}
	/*
	 * Set the icon pixmaps values depending on whether there
	 * is new mail or not.
	 */
	for (rp=readRec; rp!=(ReadRec *)0; rp=rp->next) {
		if (rp->mp == mp) {
			DisplayInRightFooter (rp->baseWGizmo, buf);
			shell = rp->baseWGizmo->icon_shell;
			XtVaSetValues (
				shell,
				XtNbackgroundPixmap,
				(new == 0) ? rdmail : nrdmail,
				XtNwidth,		width,
				XtNheight,		height,
				(String)0
			);
			XClearWindow (XtDisplay (shell), XtWindow (shell));
		}
	}
	if (mp->mng != (ManageRec *)0) {
		shell = mp->mng->baseWGizmo->icon_shell;
		XtVaSetValues (
			shell,
			XtNbackgroundPixmap,	(new==0) ? manmail : nmanmail,
			XtNwidth,		width,
			XtNheight,		height,
			(String)0
		);
		XClearWindow (XtDisplay (shell), XtWindow (shell));
	}
}

/* Update the status of messages between start and end that
 * are selected.
 */
void
UpdateStatusOfMessage (mp, start, end)
MailRec *	mp;
int		start;
int		end;
{
	int i;
	char *buf;

	buf = CompactListOfSelectedItems (mp, mp->summary, NULL, start, end);
	if (buf[0] != '\0') {
		DisplayStatus (mp, ProcessCommand (mp, FROM_CMD, buf));
	}
	/* Update the footer of the main window */
	UpdateFooter (mp);
}

static void
ToggleItem (mp, list, lp, item)
MailRec *	mp;
Widget		list;
ListHead *	lp;
int		item;
{
	if (lp->items[item].set == False) {
		SelectItem (mp, lp, item);
	}
	else {
		UnselectItem (mp, lp, item);
	}
}

static void
SelectAll (mng)
ManageRec *	mng;
{
	MailRec *mp = mng->mp;
	Widget	list;
	int	i;

	/* Set the number of selected items = size */
	mp->summary->clientData = (XtArgVal)mp->summary->size;
	list = GetSummaryListWidget (mng);
	for (i=0; i<mp->summary->size; i++) {
		if (mp->summary->items[i].set == False) {
			OlVaFlatSetValues (
				list, i, XtNset, True, (String)0
			);
		}
	}
	AdjustMenuBarButton(mp);
}

void
UnselectAll (mng, lp, except)
ManageRec *	mng;
ListHead *	lp;
int		except;	/* Unselect all items except this one */
{
	Widget	list = (Widget)0;
	int	i;

	FPRINTF ((stderr, "UnselectAll\n"));
	/* Set the number of selected items = 0 */
	lp->clientData = (XtArgVal)0;
	if (mng != (ManageRec *)0) {
		list = GetSummaryListWidget (mng);
		if (lp == mng->mp->deleted) {
			list = (Widget)0;
			if (mng->deleteListPopup != (PopupGizmo *)0) {
				list = GetDeletedListWidget (mng);
			}
		}
	}
	for (i=0; i<lp->size; i++) {
		if (lp->items[i].set == True) {
			if (i == except) {
				lp->clientData = (XtArgVal)1;
				LastSelectedMessage =
					(int)lp->items[i].clientData;
			}
			else {
				if (list != (Widget)0) {
					OlVaFlatSetValues (
						list, i, XtNset,
						False, (String)0
					);
				}
				else {
					lp->items[i].set = False;
				}
			}
		}
	}
	AdjustMenuBarButton(mng->mp);
}

void
DisplayInLeftFooter (bw, buf, raise)
BaseWindowGizmo *	bw;
char *			buf;
{
	int i = strlen (buf)-1;
	if (buf [i] == '\n') {
		buf[i] = '\0';
	}
	SetBaseWindowMessage (bw, buf);
	if (raise == True) {
		MapGizmo (BaseWindowGizmoClass, bw);
		XRaiseWindow (XtDisplay (bw->shell), XtWindow (bw->shell));
	}
}

void
LookForAdjustCB (wid, lp, call_data)
Widget		wid;
ListHead *	lp;
XtPointer	call_data;
{
	OlVirtualEvent	ve = (OlVirtualEvent)call_data;
	MailRec		*mp = FindMailRec (wid);

	if (mp == (MailRec *)0 ||
	    mp->mng == (ManageRec *)0 ||
	    mp->mng->mapped == False) {
		FPRINTF ((stderr, "Record no longer there\n"));
		return;
	}
	switch (ve->virtual_name) {
		case OL_ADJUST:
		case OL_ADJUSTKEY: {
			if (
				ve->xevent->type == ButtonPress ||
				ve->xevent->type == KeyPress
			) {
				/* Toggle the current item */
				FPRINTF ((stderr, "Got Adjust\n"));
				ResetUndo (mp);
				ve->consumed = True;
				ToggleItem (mp, wid, lp, ve->item_index);
			}
			break;
		}
		case OL_SELECTKEY:
		case OL_SELECT: {
			ResetUndo (mp);
			if (ve->xevent->type == ButtonPress ||
			    ve->xevent->type == KeyPress
			) {
				FPRINTF ((stderr, "Got Select\n"));
				UnselectAll (mp->mng, lp, ve->item_index);
			}
			break;
		}
	}
}

void
UnselectCB (list, client_data, call_data)
Widget			list;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{
	MailRec *	mp = FindMailRec (list);
	int		item = call_data->item_index;
	ListHead *	lp = (ListHead *)call_data->user_data;
	Widget		w;
	Arg		arg[2];

	if (mp->mng->deleteListPopup) {
		OlVaFlatSetValues(
			(Widget)QueryGizmo(PopupGizmoClass,
						mp->mng->deleteListPopup,
						GetGizmoWidget, "undelete"),
			0, XtNsensitive, (XtArgVal)False, NULL);
	}

	UnselectItem (mp, lp, item);
	FPRINTF ((stderr, "UnselectCB=%d\n", lp->clientData));
	AdjustMenuBarButton(mp);
}

void
SelectCB (list, client_data, call_data)
Widget			list;
XtArgVal		client_data;
OlFlatCallData *	call_data;
{
	MailRec *	mp = FindMailRec (list);
	int		item = call_data->item_index;
	ListHead *	lp = (ListHead *)call_data->user_data;

		/* client_data contains the message id, the message id
		 * shall always be >= 1, if we see client_data == 0
		 * it means that this is an inappropriate message,
		 * see mail.c:CreateHeader() */
	if (client_data == 0)
		return;
	if (mp->mng->deleteListPopup) {
		OlVaFlatSetValues(
			(Widget)QueryGizmo(PopupGizmoClass,
						mp->mng->deleteListPopup,
						GetGizmoWidget, "undelete"),
			0, XtNsensitive, (XtArgVal)True, NULL);
	}
	SelectItem (mp, lp, item);
	FPRINTF ((stderr, "SelectCB=%d\n", lp->clientData));
	AdjustMenuBarButton(mp);
}

/* Double clicking on an item means read the item */

void
ExecuteCB (wid, client_data, call_data)
Widget			wid;
XtPointer		client_data;
OlFlatCallData *	call_data;
{
	MailRec *	mp = FindMailRec (wid);
	ListHead *	lp = (ListHead *)call_data->user_data;
	int		i = call_data->item_index;

		/* client_data contains the message id, the message id
		 * shall always be >= 1, if we see client_data == 0
		 * it means that this is an inappropriate message,
		 * see mail.c:CreateHeader() */
	if (client_data == 0)
		return;

	/* First, unselect all other items */

	ResetUndo (mp);
	SelectItem (mp, lp, i);

	if (lp == mp->summary) {
		/* Then, go read the currently selected item */

		ReadProc (mp, (dblCMessage == InPlace ? True : False));
	}
	else {
		/* Then go and undelete the message */
		ManageUndelete (mp->mng);

		BringDownPopup (_OlGetShellOfWidget (wid));
	}
	FPRINTF ((stderr, "ExecuteCB=%d\n", lp->clientData));
}

void
PrintMessageList (mp, bw, messages, printed)
MailRec *		mp;
BaseWindowGizmo *	bw;
char *			messages;	/* List of messages to be printed */
char *			printed;	/* Message put in footer of bw */
{
	XEvent	event;
	char *	buf;
	int i;
	char *  embuf;

	while (XtPending ()) {
		XtNextEvent (&event);
		XtDispatchEvent (&event);
	}
	buf = ProcessCommand (mp, PIPE_CMD, messages);
	i = strlen(PRINT_FAILED) + 2*strlen(PrintCommand) + 1;
	embuf = MALLOC(i);
	sprintf(embuf, PRINT_FAILED, PrintCommand, PrintCommand);
	if (strcmp (buf, embuf) == 0) {
		DisplayInLeftFooter (
			bw, GetGizmoText (TXT_PRINT_FAILED), True
		);
	}
	else {
		DisplayInLeftFooter (bw, GetGizmoText (printed), True);
	}
	FREE(embuf);
	UpdateFooter (mp);
	UpdateStatusOfMessage (mp, 0, mp->summary->size);
}

static void
PrintMessages (mng)
ManageRec *	mng;
{
	char *		messages;
	MailRec *	mp = mng->mp;

	messages = CompactListOfSelectedItems (
		mp, mp->summary, NULL, 0, mp->summary->size
	);
	PrintMessageList (mp, mng->baseWGizmo, messages, TXT_PRINTED_MSGS);
}

static void
OpenDefaultAlias (wid, mng)
Widget		wid;
ManageRec *	mng;
{
	BaseWindowGizmo * AliasWindowP = &AliasWindow;
	Widget		shell;

	DtLockCursor (
		GetBaseWindowShell (mng->baseWGizmo),
		1000L, NULL, NULL, OlGetBusyCursor(wid)
	);

	if (aliasRec->baseWGizmo != (BaseWindowGizmo *) 0 ) {
		MapGizmo (BaseWindowGizmoClass, (Gizmo) AliasWindowP);
		shell = GetBaseWindowShell (AliasWindowP);
		XRaiseWindow (XtDisplay (shell), XtWindow (shell));
		if (aliasRec->mapped) {
			/*
			 * If it already exists and is mapped,
			 * pop it to the front
			 */
			return;
		}
		else {
			/*
			 * If it already exists and is not mapped,
			 * bring it back
			 */
			aliasRec->mapped = True;
			/* initialize menu sensitivies */
			SetSensitivity (AliasWindowP,"aliasEdit", 0, 0, False);
			SetSensitivity (AliasWindowP,"aliasEdit", 1, 1, True);
			SetSensitivity (AliasWindowP,"aliasEdit", 2, 2, True);
			/* SetSensitivity (AliasWindowP,"aliasEdit", 3, 3, True);
			/***  fprintf (stderr, "Remap set sens true\n"); /***/
			SetSensitivity (AliasWindowP,"aliasEdit", 4, 4, False);
		}
	}
	else {
		/* It doesn't yet exist.  Create it. */
		UserId = getuid ();
		UserName = GetUserName (UserId);
		(void)AliasWinInit (UserName);
		MapGizmo (BaseWindowGizmoClass, (Gizmo) AliasWindowP);
		aliasRec->mapped = True;
	}
	        {
                /* Set the minimum size for the base window */
                Dimension width, height;
                XSizeHints sz;
                Widget w;
                Arg args[10];

                w = GetBaseWindowShell (AliasWindowP);
                XtSetArg(args[0], XtNwidth, &width);
                XtSetArg(args[1], XtNheight, &height);
                XtGetValues(w, args, 2);
                sz.min_width = width;
                sz.min_height = height;
                sz.flags = PMinSize;
                XSetNormalHints(XtDisplay(w), XtWindow(w), &sz);

        }
}

static void
FileMenuCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ManageRec *		mng = FindManageRec (wid);

	DisplayInLeftFooter (mng->baseWGizmo, "", True);

	switch (p->item_index) {
		case MenuOpenUser: {
			OpenInPlaceUser(mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuOpenNewUser: {
			OpenUser (mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuSave: {
			ManageSave(mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuSaveAs: {
			ManageSaveAs(mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuPrint: {
			PrintMessages (mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuProperties: {
			ManagePropPopupCB (wid, client_data, call_data);
			break;
		}
		case MenuAlias: {
			OpenDefaultAlias(wid, mng);
			break;
		}
		case MenuExit: {
			ManageExitCB (wid, client_data, call_data);
			break;
		}
	}
}

static void
Undo (mng)
ManageRec *	mng;
{
	if (mng->lastOp == DeleteOp) {
		ManageUndo (mng);
	}
	ResetUndo (mng->mp);
}

static void
EditMenuCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ManageRec *		mng = FindManageRec (wid);

	DisplayInLeftFooter (mng->baseWGizmo, "", True);
	switch (p->item_index) {
		case MenuUndo: {
			Undo (mng);
			break;
		}
		case MenuDelete: {
			ManageDelete (mng->mp);
			break;
		}
		case MenuUndelete: {
			UndeleteProc (mng);
			break;
		}
		case MenuSelect: {
			SelectAll (mng);
			ResetUndo (mng->mp);
			break;
		}
		case MenuUnselect: {
			UnselectAll (mng, mng->mp->summary, -1);
			ResetUndo (mng->mp);
			break;
		}
	}
}

static void
MailMenuCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ManageRec *		mng = FindManageRec (wid);

	DisplayInLeftFooter (mng->baseWGizmo, "", True);
	switch (p->item_index) {
		case MenuRead: {
			ReadProc (mng->mp, (dblCMessage == InPlace ? True : False));
 
			break;
		}
		case MenuReadNew: {
			ReadProc (mng->mp, False);
			break;
		}
		case MenuMailer: {
			ManageSend (wid);
			break;
		}
		case MenuReplySender: {
			ReplySenderProc (wid);
			break;
		}
		case MenuReplySenderAtt: {
			ReplySenderAttProc (wid);
			break;
		}
		case MenuReplyAll: {
			ReplyAll (wid);
			break;
		}
		case MenuReplyAllAtt: {
			ReplyAllAtt (wid);
			break;
		}
		case MenuForward: {
			Forward (mng->mp);
			break;
		}
	}
	ResetUndo (mng->mp);
}

static ManageRec *
FindScrollManageRec (wid)
Widget	wid;
{
	MailRec *	mp;
	Widget		w;

	for (mp=mailRec; mp!=(MailRec *)0; mp=mp->next) {
		if (mp->mng != (ManageRec *)0) {
			w = GetSummaryListWidget (mp->mng);
			if (w == wid) {
				return mp->mng;
			}
		}
	}
	return (ManageRec *)0;
}

typedef enum {PipeOpen, PipeClose} PipeType;

static XtCallbackProc
DeleteDropCB (wid, client_data, call_data)
Widget		wid;
XtPointer	client_data;
XtPointer	call_data;
{
	char *		filename = (char *)call_data;
	int		i;

	if (StatFile (filename, 0, 0) != 0) {
		unlink (filename);
	}
}

static void
DroppedOutsideDtmail (wid, p)
Widget		wid;
OlFlatDropCallData *    p;
{
	char		cmd[BUF_SIZE];
	ManageRec *	mng;
	MailRec *	mp;
	int		x, y;
	DtDnDSendPtr	ret;
	char **		files;
	Window		child;
	static char *	tmpfile;
	int		item;

	mng = FindScrollManageRec (wid);
	mp = mng->mp;
	if (mng == (ManageRec *)0) {
		return;
	}
	if (p->drop_status != OlDnDDropSucceeded) {
		FPRINTF ((stderr, "Drop cancelled or unregistered\n"));
		return;
	}

	XTranslateCoordinates (
		XtDisplay (Root),
		/* p->drop_data.window, */
		p->dst_info->window,
		RootWindowOfScreen(XtScreen(Root)),
		(Position)(p->dst_info)->x,
		(Position)(p->dst_info)->y,
		&x, &y,
		&child
	);

	tmpfile = AddDotMl(tmpnam(0));
	/* Save the message in a temp file */
	sprintf (cmd, "%s %d", SAVE_CMD, LastSelectedMessage);
	(void)ProcessCommand (mp, cmd, tmpfile);
	/* Then restore the message */
	sprintf (cmd, "%s %d", PRES_CMD, LastSelectedMessage);
	(void)ProcessCommand (mp, cmd, NULL);
	item = ItemNumber (mp, LastSelectedMessage);
	UpdateStatusOfMessage (mp, item, item+1);

	files = (char **)MALLOC (sizeof(char *)*2);
	files[0] = STRDUP (tmpfile);
	files[1] = NULL;

	ret = DtNewDnDTransaction (
		wid,
		files,
		0,
		x, y,
		(p->root_info)->drop_timestamp,
		(p->dst_info)->window,
		DT_MOVE_OP,			/* Hint */
		(XtCallbackProc)DeleteDropCB,
		0,
		0
	);
	if (ret == (DtDnDSendPtr)0) {
		FPRINTF ((stderr, "DtNewDnDTransaction failed\n"));
	}
}

static void
AppProc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
	char **		p;
	ManageRec *	mng = FindManageRec (w);
	Widget		shell;

	if (dip->error != 0) {
		return;
	}
	if (dip->nitems != 1) {
		if (mng == (ManageRec *)0) {
			shell = GetBaseWindowShell (manageRec->baseWGizmo);
		}
		else {
			shell = GetBaseWindowShell (mng->baseWGizmo);
		}
		DisplayErrorPrompt (shell, GetGizmoText (TXT_ONLY_ONE_FILE));
		return;
	}
	if (dip->files != (char **)0) {
		if (dip->files[0][0] == '\0') {
		}
		else {
			(void)OpenNewMailFile (*dip->files,
						manageRec->baseWGizmo);
		}
	}
}

Boolean
ManageDropNotify (
	Widget w, Window win, Position x, Position y, Atom selection,
	Time timestamp, OlDnDDropSiteID drop_site_id,
	OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
	XtPointer closure)
{
	FPRINTF ((stderr, "Got a drop on manage\n"));
	DtGetFileNames (w, selection, timestamp, send_done, AppProc, closure);
}

static void
DropProcCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
OlFlatDropCallData *call_data;
{
	int		i = call_data->item_data.item_index;
	OlFlatDropCallData *	p = call_data;
	MailRec *	mp = FindMailRec (wid);
	ManageRec *	mng;
	SendRec *	sp;
	ReadRec *	rp;
	ReadRec *	trp;
	Widget		widget;

	/*
	 * The button press that initiated this drop would have
	 * unselected all of the items.  We need to indicate that
	 * one item is selected here.  LastSelectedItem will be set
	 * by the previous select of the dragged item.
	 */
	mp->summary->clientData = 1;
	FPRINTF ((stderr, "DropCB=%d\n", 1));

	widget = XtWindowToWidget (XtDisplay (Root), (p->dst_info)->window);
	if (p->drop_status != OlDnDDropSucceeded) {
		FPRINTF ((stderr, "Drop cancelled or unregistered\n"));
		return;
	}

	if (widget == (Widget)0) {
		DroppedOutsideDtmail (wid, p);
		return;
	}
	if ((sp = FindSendRec (widget)) != (SendRec *)0) {
		FPRINTF ((stderr, "Dropped on send window\n"));
		SendDropReply (sp, wid);
		return;
	}

	if ((rp = FindReadRec (widget)) != (ReadRec *)0 && rp->mp == mp) {
		FPRINTF ((stderr, "Dropped on read window\n"));
		/*
		 * First see if the message is already being displayed
		 */
		trp = FindMessageInList (mp, MessageNumber (mp, i));
		if (trp != (ReadRec *)0) {
			/*
			 * If so, then just raise this window
			 */
			widget = GetBaseWindowShell (trp->baseWGizmo);
		}
		/*
		 * Otherwise, read the message into the window that
		 * received the drop.
		 */
		else {
			ReadItem (rp->mp, rp, i);
			widget = GetBaseWindowShell (rp->baseWGizmo);
		}
		XRaiseWindow (XtDisplay (widget), XtWindow (widget));
		return;
	}

	if ((mng = FindManageRec (widget)) != (ManageRec *)0) {
		FPRINTF ((stderr, "Dropped on manage window\n"));
		return;
	}

}


static void
FileSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	Boolean		flag = True;

	if (mp->summary->size == 0 || mp->summary->clientData == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageFile", 2, 4, flag);
}

static void
EditSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	Boolean		flag = True;

	/*
	 * Set the sensitivity of the edit buttons
	 * SelectAll and UnselectAll.
	 */
	if (mp->summary->size == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 3, 4, flag);
	/*
	 * Set the sensitivity of the edit button Delete,
	 */
	flag = True;
	if (mp->summary->size == 0 || mp->summary->clientData == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 1, 1, flag);
	/*
	 * Set the sensitivity of the edit Undo button.
	 */
	flag = True;
	if (mng->lastOp == UndoOp) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 0, 0, flag);
	/*
	 * Set the sensitivity of the edit Undelete button.
	 */
	flag = True;
	if (mp->deleted->size == 0) {
		flag = False;
	}
	SetSensitivity (mng->baseWGizmo, "manageEdit", 2, 2, flag);
}

static void
MailSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	Boolean		flag = True;

	/*
	 * Set the sensitivity of the Reader... button.
	 */
	if (mp->summary->clientData == 0) {
		flag = False;
	}
	SetSensitivity (mp->mng->baseWGizmo, "manageMail", 0, 1, flag);
	/*
	 * Set the sensitivity of the reply buttons.  This can only
	 * be sensitive if only one item is selected.
	 */
	flag = False;
	if (mp->summary->clientData == 1) {
		flag = True;
	}
	SetSensitivity (mp->mng->baseWGizmo, "manageMail", 3, 7, flag);
}

static void
ShowLimitsCB (wid, client_data, ep)
Widget				wid;
XtArgVal			client_data;
OlFListItemsLimitExceededCD *	ep;
{
	MailRec *	mp = FindMailRec (wid);
	char		buf[BUF_SIZE];

	sprintf (buf, GetGizmoText (TXT_LIMITS_EXCEEDED), ep->preferred);
	DisplayInLeftFooter (mp->mng->baseWGizmo, buf, False);
	ep->ok = True;
}

static XtCallbackRec	LimitsCB[] = {
	{(XtCallbackProc)ShowLimitsCB,	(XtPointer)NULL},
	{NULL}
};

static void
barOpenCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	LockCursor(GetBaseWindowShell (mng->baseWGizmo));
	OpenInPlaceUser (mng);
	ResetUndo (mng->mp);
	UnlockCursor(GetBaseWindowShell (mng->baseWGizmo));
}

static void
barSaveCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	LockCursor(GetBaseWindowShell (mng->baseWGizmo));
	ManageSave(mng);
	ResetUndo (mng->mp);
	UnlockCursor(GetBaseWindowShell (mng->baseWGizmo));
}

static void
barSaveToCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	LockCursor(GetBaseWindowShell (mng->baseWGizmo));
	ManageSaveAs(mng);
	ResetUndo (mng->mp);
	UnlockCursor(GetBaseWindowShell (mng->baseWGizmo));
}

static void
barPrintCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	LockCursor(GetBaseWindowShell (mng->baseWGizmo));
	PrintMessages (mng);
	ResetUndo (mng->mp);
	UnlockCursor(GetBaseWindowShell (mng->baseWGizmo));
}

static void
barDeleteCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	ManageDelete (mng->mp);
}

static void
barReplySenderAttCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	LockCursor(GetBaseWindowShell (mng->baseWGizmo));
	ReplySenderAttProc (wid);
	UnlockCursor(GetBaseWindowShell (mng->baseWGizmo));
}

static void
barReplyAllAttCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	LockCursor(GetBaseWindowShell (mng->baseWGizmo));
	ReplyAllAtt (wid);
	UnlockCursor(GetBaseWindowShell (mng->baseWGizmo));
}

static void
barForwardCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *		mng = FindManageRec (wid);

	LockCursor(GetBaseWindowShell (mng->baseWGizmo));
	Forward (mng->mp);
	UnlockCursor(GetBaseWindowShell (mng->baseWGizmo));
}

#define T	(XtPointer)True
#define F	(XtPointer)False

static MenuItems barOpenItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barOpenCB},
	{NULL}
};
static MenuItems barSaveItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barSaveCB},
	{NULL}
};
static MenuItems barSaveToItems[] = {
	{True, "", (XtArgVal)0, NULL, barSaveToCB},
	{NULL}
};
static MenuItems barPrintItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barPrintCB},
	{NULL}
};
static MenuItems barDeleteItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barDeleteCB},
	{NULL}
};
static MenuItems barReplySenderItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barReplySenderAttCB},
	{NULL}
};
static MenuItems barReplyAllItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barReplyAllAttCB},
	{NULL}
};
static MenuItems barForwardItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barForwardCB},
	{NULL}
};
static MenuItems barNewItems[] = {
        {True, "",      (XtArgVal)0,    NULL, ManageSend},
        {NULL}
};

static MenuGizmo barOpenMenu = {
	NULL, "barManageOpenMenu", NULL, barOpenItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barSaveMenu = {
	NULL, "barManageSaveMenu", NULL, barSaveItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barSaveToMenu = {
	NULL, "barManageSaveToMenu", NULL, barSaveToItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barPrintMenu = {
	NULL, "barManagePrintMenu", NULL, barPrintItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barDeleteMenu = {
	NULL, "barManageDeleteMenu", NULL, barDeleteItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barReplySenderMenu = {
	NULL, "barManageReplySenderMenu", NULL, barReplySenderItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barReplyAllMenu = {
	NULL, "barManageReplyAllMenu", NULL, barReplyAllItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barForwardMenu = {
	NULL, "barManageForwardMenu", NULL, barForwardItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barNewMenu = {
        NULL, "barManageNewMenu", NULL, barNewItems,
        0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};

static GizmoRec buttonArray0[] = {
	{MenuBarGizmoClass,	&barOpenMenu}
};

static GizmoRec buttonArray1[] = {
	{MenuBarGizmoClass,	&barSaveMenu}
};

static GizmoRec buttonArray2[] = {
	{MenuBarGizmoClass,	&barSaveToMenu}
};

static GizmoRec buttonArray3[] = {
	{MenuBarGizmoClass,	&barPrintMenu}
};

static GizmoRec buttonArray4[] = {
	{MenuBarGizmoClass,	&barDeleteMenu}
};

static GizmoRec buttonArray5[] = {
	{MenuBarGizmoClass,	&barReplySenderMenu}
};

static GizmoRec buttonArray6[] = {
	{MenuBarGizmoClass,	&barReplyAllMenu}
};

static GizmoRec buttonArray7[] = {
	{MenuBarGizmoClass,	&barForwardMenu}
};

static GizmoRec buttonArray8[] = {
        {MenuBarGizmoClass,     &barNewMenu}
};

static Arg buttonArgs[] = {
	XtNhPad,	0
};

static LabelGizmo label0 = {
	NULL, "managerBarOpen", "",
	buttonArray0, XtNumber (buttonArray0),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label1 = {
	NULL, "managerBarSave", "",
	buttonArray1, XtNumber (buttonArray1),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label2 = {
	NULL, "managerBarSaveTo", "",
	buttonArray2, XtNumber (buttonArray2),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label3 = {
	NULL, "managerBarPrint", "",
	buttonArray3, XtNumber (buttonArray3),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label4 = {
	NULL, "managerBarDelete", "",
	buttonArray4, XtNumber (buttonArray4),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label5 = {
	NULL, "managerBarReplySender", "",
	buttonArray5, XtNumber (buttonArray5),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label6 = {
	NULL, "managerBarReplyAll", "",
	buttonArray6, XtNumber (buttonArray6),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label7 = {
	NULL, "managerBarForward", "",
	buttonArray7, XtNumber (buttonArray7),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label8 = {
        NULL, "readerBarNew", "",
        buttonArray8, XtNumber (buttonArray8),
        OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static GizmoRec manageBar[] = {
	{LabelGizmoClass,	&label8},
	{LabelGizmoClass,	&label5},
	{LabelGizmoClass,	&label6},
	{LabelGizmoClass,	&label7},
	{LabelGizmoClass,	&label3},
	{LabelGizmoClass,	&label4},
	{LabelGizmoClass,	&label1},
	{LabelGizmoClass,	&label2},
	{LabelGizmoClass,	&label0},
};

static Arg labelArgs[] = {
	XtNhSpace,	0,
	XtNhPad,	0
};

static LabelGizmo pixmapLabel = {
	NULL, "managePixmaps", "", manageBar, XtNumber (manageBar),
	OL_FIXEDROWS, 1, labelArgs, 2, False
};

StaticTextGizmo fullHeaderLabel = {
        NULL, "header_giz",
        "n",
        NorthWestGravity,
        NULL, NULL
};

char *headerLabels[MAXHEADERLABELS] =
   {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* Starting position of header labels */
int headerLabelsPosition[MAXHEADERLABELS] =
   {0, 0, 0, 0, 0, 0, 0};

int headerLabelsWidth[MAXHEADERLABELS] =
   {0, 0, 0, 0, 0, 0, 0};

int headerLabelsFldWid[MAXHEADERLABELS] =
   {0, 0, 0, 0, 0, 0, 0};

int summaryListSpaceReq[MAXHEADERLABELS] =
   {    3,
        2,
        18,
        11,
        5,
        13,
        23
   };
int summaryListPixelReq[MAXHEADERLABELS];


/* Starting (ending) positions for each field, depending on
 * whether they are left or right justified.
 */
int summaryListPosition[MAXHEADERLABELS] =
  { 0, 0, 0, 0, 0, 0, 0 };


char *  headingLabelFontStr = NULL;
char * summaryListFontStr = NULL;

static Setting summaryListSetting = {
	"None",
	(XtPointer)NULL,
	(XtPointer)NULL,
	(XtPointer)"_"
};

static ListGizmo summaryList = {
	NULL, SUMMARY_LIST, "", &summaryListSetting,
	FORMAT, False, 5,
	NULL, NULL, ExecuteCB, SelectCB, UnselectCB, LimitsCB
};

static GizmoRec summaryListArray[] = {
	{LabelGizmoClass,	&pixmapLabel},
	{StaticTextGizmoClass,  &fullHeaderLabel},
	{ListGizmoClass,	&summaryList},
};

/* Manage Base Window Menubar ---------------------------------------- */

static MenuItems fileItems[] = {
    {True, BUT_OPEN_DDD,		MNEM_OPEN_DDD},
    {True, BUT_OPEN_NEW_DDD,		MNEM_OPEN_NEW_DDD},
    {True, BUT_SAVE_MESSAGES,		MNEM_SAVE_MESSAGES},
    {True, BUT_SAVE_MESSAGES_AS_DDD,	MNEM_SAVE_MESSAGES_AS_DDD},
    {True, BUT_PRINT,			MNEM_PRINT},
    {True, BUT_ALIAS_MNG_DDD,       	MNEM_ALIAS_DDD2},
    {True, BUT_MANAGE_PROP,		MNEM_MANAGE_PROP},
    {True, BUT_EXIT,			MNEM_EXIT_E},
    {NULL}
};

static MenuGizmo file =     {
	NULL, "manageFile", NULL, fileItems, FileMenuCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems editItems[] = {
    {True, BUT_UNDO,		MNEM_UNDO},
    {True, BUT_DELETE,		MNEM_DELETE},
    {True, BUT_UNDELETE_DDD,	MNEM_UNDELETE_DDD_N},
    {True, BUT_SELECT_ALL,	MNEM_SELECT_ALL},
    {True, BUT_UNSELECT_ALL,	MNEM_UNSELECT_ALL},
    {NULL}
};

static MenuGizmo edit =     {
	NULL, "manageEdit", NULL, editItems, EditMenuCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mailItems[] = {
    {True, BUT_READ_DDD,		MNEM_READ_DDD},
    {True, BUT_READ_NEW_DDD,		MNEM_READ_NEW_DDD},
    {True, BUT_MAILER_DDD,		MNEM_MAILER_DDD},
    {True, BUT_REPLY_SENDER_DDD,	MNEM_REPLY_SENDER_DDD},
    {True, BUT_REPLY_SENDER_ATT_DDD,	MNEM_REPLY_SENDER_ATT_DDD},
    {True, BUT_REPLY_ALL_DDD,		MNEM_REPLY_ALL_DDD},
    {True, BUT_REPLY_ALL_ATT_DDD,	MNEM_REPLY_ALL_ATT_DDD},
    {True, BUT_FORWARD_DDD,		MNEM_FORWARD_DDD},
    {NULL}
};
static MenuGizmo mail = {
	NULL, "manageMail", NULL, mailItems, MailMenuCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems manageHelpItems[] = {
    {True, BUT_MANAGER_DDD,	MNEM_MANAGER_DDD,NULL,NULL,(XtPointer)HelpManager},
    {True, BUT_TOC_DDD,		MNEM_TOC_DDD,	NULL,NULL,(XtPointer)HelpTOC},
    {True, BUT_HELP_DESK_DDD,	MNEM_HELP_DESK_DDD,NULL,NULL,(XtPointer)HelpDesk},
    {NULL}
};
static MenuGizmo manageHelp = {
	NULL, "sendHelp", NULL, manageHelpItems, HelpCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems menuItems[] = {
    {True, BUT_FILE,	MNEM_FILE,	(char *) &file,		FileSelectCB},
    {True, BUT_EDIT,	MNEM_EDIT,	(char *) &edit,		EditSelectCB},
    {True, BUT_MESS,	MNEM_MESS,	(char *) &mail,		MailSelectCB},
    {True, BUT_HELP,	MNEM_HELP,	(char *) &manageHelp},
    {NULL}
};

static MenuGizmo manage =     {
	NULL, "manageMenu", NULL, menuItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};


static BaseWindowGizmo ManageWindow = {
	&ManagerHelp,
	"ManageWindow",
	" ",
	&manage,
	summaryListArray,
	XtNumber (summaryListArray),
	TXT_E_MAIL_ICON_NAME,
	"manmail.icon",
	" ",
	" ",
	50,
};

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

/*
 * CreateHeaderString().  Work with headerLabelsFldWid[],
 * headerLabelsPosition[] (if necessary), pixels_per_hdr_char
 * Convert pixels to spaces!
 */
char *
CreateHeaderString()
{
static char buf[BUFSIZ];
char *bufptr = (char *)&(buf[0]);
char *fieldptr;
int fieldwid;
int labelpos;

	/* use headerLabelsPosition[] and headerLabelsWidth[] */

	memset((void *)bufptr, ' ', (size_t)(BUFSIZ));
        fieldwid = fround( ((double)(headerLabelsFldWid[LABEL_NO])) /
                        pixels_per_hdr_char);

		sprintf(bufptr,"%*s",fieldwid, headerLabels[LABEL_NO]);
		/* I have to do this because the strings gets a NULL
		 * terminator.
		 */
		bufptr[fieldwid] = ' ';

        fieldwid = fround( ((double)(headerLabelsFldWid[LABEL_STATUS])) /
                        pixels_per_hdr_char);
		labelpos=fround(((double)headerLabelsPosition[LABEL_STATUS]) /
			pixels_per_hdr_char);
		fieldptr = bufptr + labelpos;
		sprintf(fieldptr,"%-*s",fieldwid, headerLabels[LABEL_STATUS]);
		fieldptr[fieldwid] = ' ';

        fieldwid = fround( ((double)(headerLabelsFldWid[LABEL_FROM])) /
                        pixels_per_hdr_char);
		labelpos=fround(((double)headerLabelsPosition[LABEL_FROM]) /
			pixels_per_hdr_char);
		fieldptr = bufptr + labelpos;
		sprintf(fieldptr,"%-*s",fieldwid, headerLabels[LABEL_FROM]);
		fieldptr[fieldwid] = ' ';

        fieldwid = fround( ((double)(headerLabelsFldWid[LABEL_DATE])) /
                        pixels_per_hdr_char);
		labelpos=fround(((double)headerLabelsPosition[LABEL_DATE]) /
			pixels_per_hdr_char);
		fieldptr = bufptr + labelpos;
		sprintf(fieldptr ,"%-*s",fieldwid, headerLabels[LABEL_DATE]);
		fieldptr[fieldwid] = ' ';

        fieldwid = fround( ((double)(headerLabelsFldWid[LABEL_TIME])) /
                        pixels_per_hdr_char);
		labelpos=fround(((double)headerLabelsPosition[LABEL_TIME]) /
			pixels_per_hdr_char);
		fieldptr = bufptr + labelpos;
		sprintf(fieldptr,"%*s",fieldwid, headerLabels[LABEL_TIME]);
		fieldptr[fieldwid] = ' ';

	/* Size field: if this label width is < the field width, then
	 * center it; otherwise, right justify
	 */
        fieldwid = fround( ((double)(headerLabelsFldWid[LABEL_SIZE])) /
                        pixels_per_hdr_char);
	labelpos=fround(((double)headerLabelsPosition[LABEL_SIZE]) /
			pixels_per_hdr_char);
	fieldptr = bufptr + labelpos;
	if ((headerLabelsWidth[LABEL_SIZE] + (2 * pixels_per_hdr_char)) < 
				headerLabelsFldWid[LABEL_SIZE]) {
		int start_pixel_pos, start_space_pos ;
		start_pixel_pos = headerLabelsFldWid[LABEL_SIZE]/2 -
				headerLabelsWidth[LABEL_SIZE]/2;
		
	start_space_pos = fround( (double)(start_pixel_pos) / 
                        pixels_per_hdr_char);
		fieldptr+= start_space_pos;
		fieldwid -= start_space_pos;
		sprintf(fieldptr,"%-*s",fieldwid, headerLabels[LABEL_SIZE]);
	} /* LABEL_SIZE */
	else { /* right justify */
		sprintf(fieldptr,"%*s",fieldwid, headerLabels[LABEL_SIZE]);
	}
		fieldptr[fieldwid] = ' ';
        fieldwid = fround( ((double)(headerLabelsFldWid[LABEL_SUBJECT])) /
                        pixels_per_hdr_char);
		labelpos=fround(((double)headerLabelsPosition[LABEL_SUBJECT]) /
			pixels_per_hdr_char);
		fieldptr = bufptr + labelpos;
		sprintf(fieldptr,"%-*s\0",fieldwid,
						headerLabels[LABEL_SUBJECT]);

		return((char *)&buf[0]);
}

static void
ManageMiniHelp(w, client, event)
Widget	w;
XtPointer	client;
XEvent *event;
{
        ManageRec *             mng = FindManageRec (w);

        DisplayInLeftFooter (mng->baseWGizmo,
		(event->type == EnterNotify) ? client : "", 
		False);
}

static void
InitScrollingList (mng)
ManageRec *	mng;
{
	MailRec *		mp = mng->mp;
	char *			cp;
	char			title[BUF_SIZE];
	Widget			list;
	Widget			menuwidget;
	Arg			arg[10];
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)ManageExitCB},
		{NULL}
	};
	Widget                  stw = (Widget)NULL;
	Widget                  cap;

	summaryList.list = mp->summary;
        if (fullHeaderLabel.font == NULL) {
                fullHeaderLabel.font = GetGizmoText (TXT_LUCY_BOLD);
                summaryList.font = GetGizmoText (TXT_LUCY_MEDIUM);
        }

	mng->baseWGizmo = CopyGizmo (
		BaseWindowGizmoClass, &ManageWindow
	);
	XtSetArg(arg[0], XtNwmProtocol, protocolCB);
	(void)CreateGizmo (
		Root,
		BaseWindowGizmoClass,
		mng->baseWGizmo,
		arg,
		1
	);

        /* Set up wrap on static text widget */
        if ((stw = GetSTextHeaderWidget(mng)) != (Widget)NULL) {
                char *str =  CreateHeaderString();
                XtVaSetValues(stw,
                        XtNwrap, (XtArgVal)False,
                        XtNstrip, (XtArgVal)False,
                        XtNstring,  (XtArgVal)str,
                        XtNweight,  (XtArgVal)0,
                        (char *)NULL);
        }

	/* Set up callback for drops of the manage window */
	OlDnDRegisterDDI (
		mng->baseWGizmo->icon_shell, OlDnDSitePreviewNone,
		ManageDropNotify, (OlDnDPMNotifyProc)NULL, True,
		(XtPointer)NULL
	);
	OlDnDRegisterDDI (
		mng->baseWGizmo->form, OlDnDSitePreviewNone, ManageDropNotify,
		(OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL
	);
	/* Set the weights of the gizmos in the base window */
	XtVaSetValues (
		QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "manageMenu"
		),
		XtNweight,	0,
		(String)0
	);
	XtVaSetValues (
		GetSummaryListWidget (mng),
		XtNweight,		1,
		XtNdropProc,		DropProcCB,
		(String)0
	);
	XtVaSetValues (
		QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "managePixmaps"
		),
		XtNweight,	0,
		(String)0
	);
	XtVaSetValues (
		QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "header"
		),
		XtNweight,	0,
		(String)0
	);

	if (mp->numBaseWindows == 1) {
		SelectItem (mp, mp->summary, mp->defaultItem);
		FPRINTF((stderr, "Default selected item = %d\n", mp->defaultItem));
	}

	list = GetSummaryListWidget (mng);

	if (mp->summary->size != 0) {
		XtVaSetValues (
			list,
			XtNnumItems,		mp->summary->size,
			XtNitems,		mp->summary->items,
			XtNviewItemIndex,	GetLastSelectedItem (mp),
			XtNuserData,		mp->summary,
			(String)0
		);
	}
	else
	{
		XtVaSetValues (
			list,
			XtNuserData,		mp->summary,
			(String)0
		);
	}

	/* Display the number of new messages */

	UpdateFooter (mp);

	/* Set the title of the manage base window */

	cp = GetGizmoText (TXT_ELECTRONIC_MAIL);
	sprintf (title, cp, "");
	if (strcmp (GetUserMailFile (), mp->filename) != 0 &&
	    strcmp (mp->filename, DUMMY_FILE) != 0) {
		sprintf (title, cp, mp->filename);
	}
	XtVaSetValues (
		GetBaseWindowShell (mng->baseWGizmo),
		XtNtitle,			title,
		XtNwmProtocolInterested,	OL_WM_DELETE_WINDOW,
		(String)0
	);

	/* Suck up the adjust event */
	OlAddCallback (
		list,
		XtNconsumeEvent,
		(XtCallbackProc)LookForAdjustCB,
		(XtPointer)mp->summary
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManageOpenMenu"
		)),
		"m.open24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_OPEN)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManageSaveMenu"
		)),
		"m.save24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_SAVE)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManageSaveToMenu"
		)),
		"m.saveas24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_SAVEAS)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManagePrintMenu"
		)),
		"dtm.print.24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_PRINT)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManageDeleteMenu"
		)),
		"m.delete24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_DELETE)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManageReplySenderMenu"
		)),
		"m.reply.snd24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_REPLY)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManageReplyAllMenu"
		)),
		"m.reply.all24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_REPLYALL)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, mng->baseWGizmo,
			GetGizmoWidget, "barManageForwardMenu"
		)),
		"m.forward24"
	);
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_FORWARD)
	);
        SetMenuPixmap (
                (cap = (Widget)QueryGizmo (
                        BaseWindowGizmoClass, mng->baseWGizmo,
                        GetGizmoWidget, "barManageNewMenu"
                )),
                "m.new24"
        );
	XtAddEventHandler(cap, 
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_MANAGER_NEW)
	);

        /* one way of making the toolbar left-justified */
        XtSetArg(arg[0], XtNposition, OL_RIGHT);
        XtSetValues(XtParent(XtParent(XtParent(XtParent(cap)))), arg, 1);

	AdjustMenuBarButton(mng->mp);
}

void
CreateManageRec (mp)
MailRec *	mp;
{
	ManageRec *		mng;
	char *			cp;

	mp->numBaseWindows += 1;
	mng = (ManageRec *)MALLOC (sizeof(ManageRec));
	if (manageRec)
	{
		mng->mngnext = manageRec;
		mng->mngprev = manageRec->mngprev;
		manageRec->mngprev->mngnext = mng;
		manageRec->mngprev = mng;
	}
	else
	{
		mng->mngnext = mng->mngprev = mng;
	}
	manageRec = mng;
	managecnt++;

	mng->openUserPopup = (FileGizmo *)0;
	mng->openInPlaceUserPopup = (FileGizmo *)0;
	mng->saveAsPopup = (FileGizmo *)0;
	mng->deleteListPopup = (PopupGizmo *)0;
	mng->lastOp = UndoOp;
	mng->previousDeletes = NULL;
	mng->mp = mp;		/* Point to the appropriate mailx */
	mp->mng = mng;
	mng->topreader = (ReadRec *)0;
	mng->readercnt = 0;
	mng->topsender = (SendRec *)0;
	mng->sendercnt = 0;

	/* build and initialize BaseWGizmo */
	InitScrollingList (mng);

	XtAddCallback (
		GetBaseWindowShell (mng->baseWGizmo),
		XtNdestroyCallback, ClientDestroyCB, mng->baseWGizmo
	);

	MapGizmo (BaseWindowGizmoClass, mng->baseWGizmo);
	mp->mng->mapped = True;

	if (mp->noMail == True) {
		cp = GetGizmoText (TXT_NO_MAIL);
		DisplayInLeftFooter (mng->baseWGizmo, cp, True);
	}
}

void
DeleteManageRec (mng)
ManageRec *	mng;
{
	ManageRec *	tp;

	FPRINTF ((stderr, "In DeleteManageRec\n"));
	FPRINTF((stderr, "In DeleteManageRec\n"));
	if (mng == (ManageRec *)0) {
		FPRINTF ((stderr, "Returning because mng == 0\n"));
		return;
	}
	if ((tp=manageRec) == (ManageRec *)0)
	{
		FPRINTF((stderr, "Returning because manageRec == 0\n"));
		return;
	}
	FPRINTF((stderr, "starting manage scan loop\n"));
	do
	{
		if (mng == tp)
		{
			managecnt--;
			if (mng->mngnext == mng)
			{
				manageRec = (ManageRec *)0;
				break;
			}
			if (manageRec == mng)
				manageRec = mng->mngnext;
			mng->mngnext->mngprev = mng->mngprev;
			mng->mngprev->mngnext = mng->mngnext;
			break;
		}
	} while (tp = tp->mngnext, tp != manageRec);
	if (mng != tp)
	{
		FPRINTF( (stderr, "Warning: mng not in manageRec list\n"));
	}
	FPRINTF((stderr, "starting reader scan loop\n"));
	while (mng->topreader)
		DeleteReadRec(mng->topreader);
	FPRINTF((stderr, "finished reader scan loop\n"));
	if (mng->mp != (MailRec *)0) {
		mng->mp->numBaseWindows -= 1;
		mng->mp->mng = (ManageRec *)0;
	}
	if (mng->mp->summary->size != 0) {
		XtDestroyWidget (GetBaseWindowShell (mng->baseWGizmo));
	}
	FREEGIZMO (FileGizmoClass, mng->openUserPopup);
	FREEGIZMO (FileGizmoClass, mng->openInPlaceUserPopup);
	FREEGIZMO (PopupGizmoClass, mng->deleteListPopup);
	FREEGIZMO (FileGizmoClass, mng->saveAsPopup);
	FREE (mng);
	mng = NULL;
}

static void 
AdjustMenuBarButton(mp)
MailRec *	mp;
{
	Arg		arg[2];

	XtSetArg(arg[0], XtNsensitive, (mp->summary->clientData == 1) ? 1 : 0);
	XtSetValues((Widget)QueryGizmo (
			BaseWindowGizmoClass, mp->mng->baseWGizmo,
			GetGizmoWidget, "barManageReplySenderMenu"),
		arg, 1);
	XtSetValues((Widget)QueryGizmo (
			BaseWindowGizmoClass, mp->mng->baseWGizmo,
			GetGizmoWidget, "barManageReplyAllMenu"),
		arg, 1);
	XtSetValues((Widget)QueryGizmo (
			BaseWindowGizmoClass, mp->mng->baseWGizmo,
			GetGizmoWidget, "barManageForwardMenu"),
		arg, 1);
	XtSetArg(arg[0], XtNsensitive, (mp->summary->clientData == 0) ? 0 : 1);
	XtSetValues((Widget)QueryGizmo (
			BaseWindowGizmoClass, mp->mng->baseWGizmo,
			GetGizmoWidget, "barManageSaveMenu"),
		arg, 1);
	XtSetValues((Widget)QueryGizmo (
			BaseWindowGizmoClass, mp->mng->baseWGizmo,
			GetGizmoWidget, "barManageSaveToMenu"),
		arg, 1);
	XtSetValues((Widget)QueryGizmo (
			BaseWindowGizmoClass, mp->mng->baseWGizmo,
			GetGizmoWidget, "barManagePrintMenu"),
		arg, 1);
	XtSetValues((Widget)QueryGizmo (
			BaseWindowGizmoClass, mp->mng->baseWGizmo,
			GetGizmoWidget, "barManageDeleteMenu"),
		arg, 1);
}

