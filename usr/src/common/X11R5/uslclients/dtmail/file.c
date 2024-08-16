/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:file.c	1.50"
#endif

#define FILE_C

#include <string.h>
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
#include <Xol/StaticText.h>
#include <Xol/OlCursors.h>	/* For OlGetBusyCursor */
#include <X11/Shell.h>          /* need this for XtNtitle */
#include "mail.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/BaseWGizmo.h>

extern int		Version;
extern char *		Mbox;
extern MailRec *	mailRec;
extern char *		GlobalBuf;
extern o_ino_t		DummyDir;

MenuItems ErrorItems[] = {
    {True,	BUT_OK,	MNEM_OK,	NUL,	CancelCB},
    {NULL}
};

static MenuGizmo errorMenu = {
	NULL,			/* help		*/
	"",			/* name		*/
	NULL,			/* title	*/
	ErrorItems,		/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	0			/* default Item	*/
};

void
DisplayErrorPrompt (Widget shell, char *buf)
{
	FPRINTF ((stderr, "error message = %s\n", buf));
	(void)DefaultModalGizmo (
		shell, &errorMenu, buf
	);
}

/*
 * Add the suffix .ml to the given text string.
 * Just return the text string if the file already
 * contains .ml.
 */

char *
AddDotMl (char *text)
{
	int	i;
	char *	buf;
	char *	filename;
	char *	cp;

	/* If there is already a .ml on the end of the name
	 * then just return the name.
	 */
	if ((cp = strrchr (text, '.')) != NULL) {
		if (strcmp (cp, GetGizmoText (TXT_DOTML)) == 0) {
			return STRDUP (text);
		}
	}

	/* Separate the name into directory and basename
	 */

	if ((filename = strrchr (text, '/')) == NULL) {
		filename = text;
		text = NULL;
	}
	else {
		*filename = '\0';
		filename += 1;
	}

	/* Add .ml to the end of the filename
	 */
	i = strlen (text);
	buf = (char *)MALLOC (i + 16);
	buf[0] = '\0';
	if (i != 0) {
		strcpy (buf, text);
		strcat (buf, "/");
	}
	strncat (buf, filename, 253);
	buf[i+1+253] = '\0';
	strcat (buf, GetGizmoText (TXT_DOTML));
	if (strchr (buf, ' ') != NULL) {
		FREE (buf);
		/* Filenames can't contain blanks */
		return NULL;
	}
	return buf;
}

/*
 * Display the output from a save command in the left footer of the 
 * manage window.
 */

Boolean
DisplaySaveStatus (bw, buf)
BaseWindowGizmo *       bw;
char *			buf;
{
	char *		reason;
	static char *	regx1 = NULL;
	static char *	regx2;
	static char *	regx3;
	static char *	regx4;
	char		filename[BUF_SIZE];
	char		text[BUF_SIZE];
	char *		cp;

	/* When doing a save the message that comes back
	 * should contain either [New file] of [Appended].
	 * Otherwize, there was an error and a notice
	 * should be displayed.
	 */

	if (regx1 == NULL) {
		regx1 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[Appended\\] [0-9]+/[0-9]+",
			0
		);
		regx2 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[New file\\] [0-9]+/[0-9]+",
			0
		);
		regx3 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[Appended\\] binary/[0-9]+",
			0
		);
		regx4 = (char *) regcmp (
			"\"([^ ]+)$0\" \\[New file\\] binary/[0-9]+",
			0
		);
	}
	if ((cp = (char *) regex (regx1, buf, filename)) != NULL ||
	    (cp = (char *) regex (regx2, buf, filename)) != NULL ||
	    (cp = (char *) regex (regx3, buf, filename)) != NULL ||
	    (cp = (char *) regex (regx4, buf, filename)) != NULL) {
		sprintf (text, GetGizmoText (TXT_MESSAGE_SAVED), filename);
		DisplayInLeftFooter (bw, text, True);
		return True;
	}
	else {
		/* Can't find either "New file" or "Appended"
		 * so this must be an error.
		 */
		if (Version == 41) {
			reason = strrchr (buf, ':')+1;
		}
		else {
			reason = strrchr (buf, '"')+1;
		}
		*reason = '\0';
		reason += 1;
		/* Pluck out the filename from the start of the
		 * error message.
		 */
		strcpy (filename, buf+1);
		cp = strchr (filename, '"');
		*cp = '\0';
		cp = GetErrorText (
			0, reason, TXT_CANT_SAVE_FILE, filename
		);
		DisplayErrorPrompt (GetBaseWindowShell (bw), cp);

		return False;
	}
}

void
ManageSaveSaveAsCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec (wid);
	MailRec *	mp = mng->mp;
	char *		filename;
	char *		ml;
	char *		buf;
	int		n;
	int		flag;

	n = ExpandFileGizmoFilename(mng->saveAsPopup, &flag);
	if ((flag != 0 && n == 1) || n == 0) {
		filename = GetFilePath (mng->saveAsPopup);
	}
	else {
		return;
	}
	ml = AddDotMl (filename);
	FREE (filename);
	if (ml == NULL) {
                DisplayErrorPrompt (GetBaseWindowShell (mng->baseWGizmo), 
                                      GetGizmoText (TXT_INVALID_FILENAME));
		return;
	}
	buf = CompactListOfSelectedItems (
		mp, mp->summary, ml, 0, mp->summary->size
	);
	if (buf[0] != '\0') {
		if (
			DisplaySaveStatus (
				mng->baseWGizmo,
				ProcessCommand (mp, SAVE_CMD, buf)
			) == True
		) {
			BringDownPopup (GetFileGizmoShell (mng->saveAsPopup));
			UpdateStatusOfMessage (mp, 0, mp->summary->size);
		}
		FREE (ml);
	}
}

void
ManageSaveAs (mng)
ManageRec *	mng;
{

	if (mng->saveAsPopup == (FileGizmo *)0) {
		CreateManageSaveAsPopup (mng);
	}
	MapGizmo (FileGizmoClass, mng->saveAsPopup);
}

void
ManageSave (mng)
ManageRec *	mng;
{
	MailRec *	mp = mng->mp;
	char *		buf;

	buf = CompactListOfSelectedItems (
		mp, mp->summary, Mbox, 0, mp->summary->size
	);

	if (buf[0] != '\0') {
		if (DisplaySaveStatus (
				mng->baseWGizmo,
				ProcessCommand (mp, SAVE_CMD, buf)
			) == True) {
			UpdateStatusOfMessage (mp, 0, mp->summary->size);
		}
	}
}

void
QuitMailx (mp)
MailRec *	mp;
{
	/* Tell mailx to quit */
	WriteToMailx (mp, QUIT_SAVE, strlen (QUIT_SAVE));
	p3close (mp->fp);
	DeleteMailRec (mp);
}

void
UnmapShell (sh)
Widget	sh;
{
	if ((GetWMState (XtDisplay(sh), XtWindow(sh))) == IconicState) {
		if (XWithdrawWindow (
			XtDisplay (sh), XtWindow (sh),
			XScreenNumberOfScreen (XtScreen(sh))
		)) {
			XSync (XtDisplay(sh), False);
		}
	}
	else {
		XtUnmapWidget (sh);
	}
}

void
ExitManager (mng)
ManageRec *	mng;
{
	SendRec *       sp;
	Boolean         changed = False;
        Widget          shell;
	ReadRec *       rp;

	/* Bring all popups down */
	if (mng == (ManageRec *)0) {
		return;
	}
	FPRINTF((stderr, "In ExitManager\n"));
	if (mng->openUserPopup != (FileGizmo *)0) {
		XtPopdown (GetFileGizmoShell (mng->openUserPopup));
	}
	if (mng->openInPlaceUserPopup != (FileGizmo *)0) {
		XtPopdown (GetFileGizmoShell (mng->openInPlaceUserPopup));
	}
	if (mng->deleteListPopup != (PopupGizmo *)0) {
		XtPopdown (GetPopupGizmoShell (mng->deleteListPopup));
	}
	if (mng->saveAsPopup != (FileGizmo *)0) {
		XtPopdown (GetFileGizmoShell (mng->saveAsPopup));
	}
	if (mng->topsender)
	{
		sp = mng->topsender;
		FPRINTF((stderr, "start sender check loop\n"));
		do
		{
			if (sp->used == True) {
				if (TextChanged (sp) == True) {
					changed = True;
				}
			}
		} while (sp = sp->mngnext, sp != mng->topsender);
		if (changed == True)
		{
			shell = GetBaseWindowShell (mng->baseWGizmo);
			CreateManagerTextChangedModal (shell);
			return;
		}
		FPRINTF((stderr, "start sender loop\n"));
		while ((sp = mng->topsender) != (SendRec *)0)
		{
			if (sp->used == True) {
				LeaveSend (sp);
			}
			DeleteSendRec(sp);
		}
	}
	FPRINTF((stderr, "start reader loop\n"));
	while ((rp = mng->topreader) != (ReadRec *)0)
		DeleteReadRec(rp);
	FPRINTF((stderr, "finished reader loop\n"));
	UnmapShell (GetBaseWindowShell (mng->baseWGizmo));
	mng->mapped = False;

	if (managecnt == 1)
	{
		UpdateMailrc (mng->mp);
		QuitMailx (mng->mp);
		exit(0);
	}
	/* If this isn't the only mailx open */
	if (mailRec->next != (MailRec *)0) {
		/* then exit it */
		QuitMailx (mng->mp);
	}
	else {
		/* else, keep it running but switch its input file to "/" */
		(void)SwitchMailx (mng->mp, DUMMY_FILE, mng->baseWGizmo);
		DeleteManageRec (mng);
	}
}

void
ManageExitCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ManageRec *	mng = FindManageRec(wid);

	ExitManager (mng);
}

void
FreeSummaryOrDeleteList (wid, lp)
Widget		wid;
ListHead *	lp;
{
	int	i;
	int	j;
	char **	tmp;

	if (wid != (Widget)0) {
		XtVaSetValues (
			wid,
			XtNnumItems,		0,
			XtNitems,		NULL,
			(String)0
		);
	}
	if (lp->size > 0) {
		for (i=0; i<lp->size; i++) {
			tmp = (char **)lp->items[i].fields;
			/* Don't free the first item because it is a pixmap */
			for (j=1; j<lp->numFields; j++) {
				FREE (tmp[j]);
			}
			FREE ((char *)lp->items[i].fields);
		}
		FREE (lp->items);
	}
	lp->size = 0;
}

void
RaiseManageWindow (mp)
MailRec *	mp;
{
	Widget		shell;

	if (mp->mng == (ManageRec *)0) {
		CreateManageRec (mp);
		return;
	}
	shell = GetBaseWindowShell (mp->mng->baseWGizmo);
	if (mp->mng->mapped == False) {
		mp->mng->mapped = True;
		MapGizmo (BaseWindowGizmoClass, mp->mng->baseWGizmo);
	}
	else {
		DisplayInLeftFooter (
			mp->mng->baseWGizmo,
			GetGizmoText (TXT_MNG_ALREADY_ACTIVE),
			True
		);
		if (GetWMState(XtDisplay (shell), XtWindow (shell)) == IconicState) {
			struct {
				unsigned long          state;
				Window          icon;
			} prop;
			Atom wm_state;

			wm_state = XA_WM_STATE(XtDisplay (shell));
			prop.state = NormalState;
			XChangeProperty(XtDisplay (shell), XtWindow (shell), 
				wm_state, wm_state, 32,
				PropModeReplace, (unsigned char *) (&prop), 1);
			/*  The Rasie-Window and Set-Input-Focus operations are
			    done for us by the window manager and if we try to
			    do them, we'll get protocol errors
			*/
			manageRec = mp->mng;
			return;
		}
	}
	XRaiseWindow (XtDisplay (shell), XtWindow (shell));
	XSetInputFocus(XtDisplay(shell), XtWindow(shell),
		RevertToNone, CurrentTime);
	manageRec = mp->mng;
}

MailRec *
OpenNewMailFile (filename, bw)
char *			filename;
BaseWindowGizmo *	bw;
{
	MailRec *	mp = mailRec;
	o_ino_t		inode;
	Widget		shell;

	if (bw)
	{
		shell = GetBaseWindowShell (bw);
		DtLockCursor (shell, 1000L, NULL, NULL,
				OlGetBusyCursor(shell));
	}

	/* First look and see if the default file is open.
	 * If so, use that mailx.
	 */
	if (mp->inode == DummyDir) {
		/* If this mailx is open to the default mailer */
		/* then use it. */
		if (SwitchMailx (mp, filename, bw) == True) {
			CreateManageRec (mp);
		}
		return mp;
	}

	/* Second, look to see if there is a mailRec with
	 * this filename already open.
	 */

	(void)StatFile(filename, &inode, (off_t *)0);
	if ((mp = GetMailx (inode)) != (MailRec *)0) {
		/* Raise the window */
		RaiseManageWindow (mp);
		return mp;
	}
	/*
	 * Otherwise, open a new mailx and create a new manage window.
	 */
	mp = OpenMailx ();
	if (SwitchMailx (mp, filename, bw) == True) {
		CreateManageRec (mp);
		return mp;
	}
	QuitMailx (mp);
	return (MailRec *)0;
}

void
DisplayAlreadyOpen (bw, filename)
BaseWindowGizmo *	bw;
char *			filename;
{
	char	buf[BUF_SIZE];

	sprintf (buf, GetGizmoText (TXT_FILE_ALREADY_OPEN), filename);
	DisplayInLeftFooter (bw, buf, True);
}

static
void
DoOpenNew(xmp, Id)
XtPointer xmp;
XtIntervalId Id;
{
	MailRec *		mp = (MailRec *) xmp;
	MailRec *		tmp;
	char *			filename;
	char *			fp;
	int			n;
	int			flag;

	n = ExpandFileGizmoFilename(mp->mng->openUserPopup, &flag);
	if (n > 1) {
		return;
	}
	if (n == 0) {
		fp = GetFilePath (mp->mng->openUserPopup);
		filename = AddDotMl (fp);
		FREE (fp);
	}
	else {
		filename = GetFilePath (mp->mng->openUserPopup);
	}

	tmp = OpenNewMailFile (filename, mp->mng->baseWGizmo);
	if (tmp != (MailRec *)0) {
		BringDownPopup (GetFileGizmoShell(mp->mng->openUserPopup));
	}
	FREE (filename);
}

void
OpenCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *		mp = FindMailRec (wid);
	
	XtAddTimeOut(0, (XtTimerCallbackProc)DoOpenNew, (XtPointer)mp);
}

static
void
DoOpenInPlace(xmp, Id)
XtPointer xmp;
XtIntervalId Id;
{
	MailRec *		mp = (MailRec *) xmp;
	MailRec *		nmp;
	ManageRec *		mng = mp->mng;
	Boolean			itworked;
	char *			filename;
	char *			fp;
	int			n;
	int			flag;
	o_ino_t         	inode;
	ListGizmo *		summaryList;
	char *			cp;
	char			title[BUF_SIZE];
	char 	*		tmp;

	n = ExpandFileGizmoFilename(mng->openInPlaceUserPopup, &flag);
	if (n > 1) {
		return;
	}
	if (n == 0) {
		fp = GetFilePath (mng->openInPlaceUserPopup);
		filename = AddDotMl (fp);
		FREE (fp);
	}
	else {
		filename = GetFilePath (mng->openInPlaceUserPopup);
	}

	(void)StatFile(filename, &inode, (off_t *)0);
	if ((nmp = GetMailx (inode)) != (MailRec *)0
			&& nmp != mp) {
		/* Raise the window - only 1 manager per file */
		RaiseManageWindow (nmp);
		BringDownPopup (GetFileGizmoShell(mp->mng->openInPlaceUserPopup));
		return;
	}

	/* either it is a new one or user wants to synchronize current file */

	tmp = STRDUP (mp->filename);
	manageRec = mng;
	FPRINTF((stderr, "calling SwitchMailx for DUMMY_FILE\n"));
	(void)SwitchMailx (mp, DUMMY_FILE, mp->mng->baseWGizmo);
	FPRINTF((stderr, "calling SwitchMailx for %s\n", filename));
	itworked = SwitchMailx (mp, filename, mp->mng->baseWGizmo);
	FPRINTF((stderr, "back from OpenNewMailFile\n"));
	if (!itworked) {
		FREE (filename);
		FPRINTF((stderr, "OpenInPlaceCB done\n"));
		mp->filename = STRDUP (tmp);
		FREE (tmp);
		/* opening a new mail file just failed,
		   restore the old one */
		(void)SwitchMailx (mp, mp->filename, mp->mng->baseWGizmo);
	}
	FPRINTF((stderr, "calling BringDownPopup\n"));
	BringDownPopup (GetFileGizmoShell(mp->mng->openInPlaceUserPopup));
	FPRINTF((stderr, "called BringDownPopup\n"));
	
	while(mng->topreader)
		DeleteReadRec(mng->topreader);
	summaryList = GetSummaryListGizmo(mng);
	summaryList->list = mp->summary;
	FPRINTF((stderr, "defaultItem=%d\n", mp->defaultItem));
	FPRINTF((stderr, "calling XtVaSetValues for list\n"));
	if (mp->summary->size != 0) {
		XtVaSetValues (
			GetSummaryListWidget (mng),
			XtNnumItems,		mp->summary->size,
			XtNitems,		mp->summary->items,
			XtNviewItemIndex,	GetLastSelectedItem (mp),
			XtNuserData,		mp->summary,
			(String)0
		);
		SelectItem (mp, mp->summary, mp->defaultItem);
	}
	FPRINTF((stderr, "done with list\n"));
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
	 
	FREE (filename);
	FPRINTF((stderr, "OpenInPlaceCB done\n"));
}

void
OpenInPlaceCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *		mp = FindMailRec (wid);
	
	XtAddTimeOut(0, (XtTimerCallbackProc)DoOpenInPlace, (XtPointer)mp);
}

void
OpenUser (mng)
ManageRec *	mng;
{
	if (mng->openUserPopup == (FileGizmo *)0) {
		CreateOpenUserPopup (mng);
	}
	MapGizmo (FileGizmoClass, mng->openUserPopup);
}

void
OpenInPlaceUser (mng)
ManageRec *	mng;
{
	int dummy;

	if (mng->openInPlaceUserPopup == (FileGizmo *)0) {
		CreateOpenInPlaceUserPopup (mng);
	}
	else ExpandFileGizmoFilename(mng->openInPlaceUserPopup, &dummy);
	MapGizmo (FileGizmoClass, mng->openInPlaceUserPopup);
}
