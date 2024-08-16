/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:read.c	1.92"
#endif

#define READ_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/PopupWindo.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/FList.h>
#include "mail.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include "RMailGizmo.h"

/* From textbuff.c */
#define AppendLineToTextBuffer(text, b) \
	InsertLineIntoTextBuffer(text, (text-> lines.used), b)
#define NEW_MAIL	"New mail has arrived."
#define LOADED		"Loaded %d"

extern HelpInfo		ReaderHelp;
extern PopupGizmo	PropReadPopup;
extern char *		PrintCmd;
extern int		LastSelectedMessage;
extern MailRec *	LastMailRec;
extern HeaderSettings	ShowHeader;
extern ListGizmo	BriefList;
extern MailRec *	mailRec;
extern char *		ApplicationName;
extern char *		Mbox;
extern Widget		Root;
extern o_ino_t		DummyDir;
extern DblCMessage      dblCMessage;

ReadRec *		CreateReadRec ();

ReadRec *	readRec = (ReadRec *)0;

typedef enum {
	MenuOpen, MenuSave, MenuSaveAs, MenuPrint, MenuProperties, MenuExit
} ReadFileItemIndex;

typedef enum {
	MenuUndo, MenuDelete, MenuSelect, MenuUnselect
} ReadEditItemIndex;

typedef enum {
	MenuReplySender, MenuReplySenderAtt,
	MenuReplyAll, MenuReplyAllAtt, MenuForward
} ReadMailItemIndex;

typedef enum {
	MenuNext, MenuPrev
} ReadViewItemIndex;

typedef enum {
	MenuFile, MenuEdit, MenuView, MenuMail, MenuHelp
} ReadMenuItemIndex;

static void AdjustMenuBarButtons();

int
MessageNumber (mp, item)
MailRec *	mp;
int		item;
{
	if (mp->summary->items == NULL) {
		return 0;
	}
	return mp->summary->items[item].clientData;
}

int
ItemNumber (mp, message)
MailRec *	mp;
int		message;
{
	int	i;

	/* Look for this message number in the clientData part of the item */
	for (i=mp->summary->size-1; i>=0; i--) {
		if (message == mp->summary->items[i].clientData) {
			return i;
		}
	}
	for (i=mp->deleted->size-1; i>=0; i--) {
		if (message == mp->deleted->items[i].clientData) {
			return i;
		}
	}
	return i;
}

ReadRec *
FindReadRec (wid)
Widget		wid;
{
	Widget		shell;
	ReadRec *	rp;

	shell = GetToplevelShell (wid);
	FPRINTF((stderr, "FindReadRec: readRec=0x%x\n", readRec));
	for (rp=readRec; rp!=(ReadRec *)0; rp=rp->next) {
		FPRINTF((stderr, "FindReadRec: rp=0x%x\n", rp));
		if (rp->baseWGizmo->icon_shell == wid) {
			return rp;
		}
		if (shell == GetBaseWindowShell (rp->baseWGizmo)) {
			return rp;
		}
	}
	return (ReadRec *)0;
}

/* Delete the passed in ReadRec from the list of ReadRecs */

void
DeleteReadRec (rp)
ReadRec *	rp;
{
	Widget		readshell;
	ReadRec *	p;
	ReadRec *	last;
	ManageRec *	mng;

	if (rp == (ReadRec *)0) {
		return;
	}
	FPRINTF((stderr, "In DeleteReadRec for rp=0x%x\n", rp));
	/* Look for the readpopup in the readpopup list */

	last = (ReadRec *)0;
	/* Get the last item */
	for (p=readRec; p!=(ReadRec *)0; p=p->next) {
		if (p == rp) {
			break;
		}
		last = p;
	}

	/* Delete it from the global list */

	FPRINTF((stderr, "DeleteReadRec: p=0x%x\n", p));
	if (last == (ReadRec *)0) {
		readRec = rp->next;
	}
	else {
		last->next = rp->next;
	}

	/* now delete it from the manager's list */
	mng = rp->parent;
	if (mng->topreader == rp)
	{
		if (rp->mngnext == rp)
			mng->topreader = (ReadRec *)0;
		else
			mng->topreader = rp->mngnext;
	}
	rp->mngnext->mngprev = rp->mngprev;
	rp->mngprev->mngnext = rp->mngnext;
	mng->readercnt--;

	/* Free the gizmo and the record */

	readshell = GetBaseWindowShell (rp->baseWGizmo);
	XtDestroyWidget (rp->baseWGizmo->icon_shell);
#ifdef BROKEN
	FreeGizmo (BaseWindowGizmoClass, rp->baseWGizmo);
#endif
	XtDestroyWidget(readshell);
	rp->mp->numBaseWindows -= 1;
	FPRINTF ((stderr, "numBaseWindows = %d\n", rp->mp->numBaseWindows));
	if (rp->mp->rp == rp)
		rp->mp->rp = mng->topreader;

	FREEGIZMO (FileGizmoClass, rp->readSaveAsPopup);
	FREEGIZMO (FileGizmoClass, rp->readOpenUserPopup);
	FPRINTF((stderr, "DeleteReadRec done\n"));
}

/* Find the passed in message number in the list of read records */

ReadRec *
FindMessageInList (mp, message)
MailRec *	mp;
int		message;
{
	ReadRec *	rp;

	for (rp=readRec; rp!=(ReadRec *)0; rp=rp->next) {
		if (mp == rp->mp && rp->message == message) {
			return rp;
		}
	}
	return (ReadRec *)0;
}

static void
ReadExitCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);

	DeleteReadRec (rp);
}

static void
ReadSaveAs (rp)
ReadRec *	rp;
{
	LockCursor(GetBaseWindowShell (rp->baseWGizmo));
	if (rp->readSaveAsPopup == (FileGizmo *)0) {
		CreateReadSaveAsPopup (rp);
	}
	MapGizmo (FileGizmoClass, rp->readSaveAsPopup);
	UnlockCursor(GetBaseWindowShell (rp->baseWGizmo));
}

void
ReadSaveSaveAsCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	MailRec *	mp = rp->mp;
	char *		filename;
	char *		ml;
	int		item;
	char *		buf;
	int		n;
	int		flag;
	char		svcmd[1000];

	n = ExpandFileGizmoFilename (rp->readSaveAsPopup, &flag);
	if ((flag != 0 && n == 1) || n == 0) {
		filename = GetFilePath (rp->readSaveAsPopup);
	}
	else {
		return;
	}
	FPRINTF((stderr, "ReadSaveSaveAsCB: filename=\"%s\"\n", filename));
	ml = AddDotMl (filename);
	FREE (filename);
	if (ml == NULL) {
                DisplayErrorPrompt (GetBaseWindowShell (rp->baseWGizmo),
                                      GetGizmoText (TXT_INVALID_FILENAME));
		return;
	}
	FPRINTF((stderr, "ReadSaveSaveAsCB: ml=\"%s\"\n", ml));
	item = ItemNumber (mp, rp->message);
	FPRINTF((stderr, "ReadSaveSaveAsCB: item=%d\n", item));
	sprintf(svcmd, "%d %s", rp->message, ml);
	FPRINTF((stderr, "ReadSaveSaveAsCB: svcmd=\"%s\"\n", svcmd));
	FPRINTF((stderr, "CompactListOfSelectedItems=\"%s\"\n",
		CompactListOfSelectedItems(
			mp, mp->summary, ml, item, item+1
		)));
	buf = ProcessCommand (mp, SAVE_CMD, svcmd);
	FPRINTF((stderr, "ReadSaveSaveAsCB: buf=\"%s\"\n", buf));
	if (DisplaySaveStatus (rp->baseWGizmo, buf) == True) {
		UpdateStatusOfMessage (mp, item, item+1);
		BringDownPopup (GetFileGizmoShell (rp->readSaveAsPopup));
	}
	FREE (ml);
}

static void
ReadOpenUser (rp)
ReadRec *	rp;
{
	if (rp->readOpenUserPopup == (FileGizmo *)0) {
		CreateReadOpenUserPopup (rp);
	}
	MapGizmo (FileGizmoClass, rp->readOpenUserPopup);
}

static void
PrintMessage (rp)
ReadRec *	rp;
{
	char	message[BUF_SIZE];

	sprintf (message, "%d", rp->message);
	PrintMessageList (rp->mp, rp->baseWGizmo, message, TXT_PRINTED_MSG);
}

void
ReadSaveCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *	mp = FindMailRec (wid);
	ReadRec *	rp = FindReadRec (wid);
	char buf[BUF_SIZE];


	LockCursor(GetBaseWindowShell (rp->baseWGizmo));

	/* Create a 'save message#' command */
	sprintf (
		buf, "%s %d %s", SAVE_CMD, rp->message, Mbox
	);
	mp->rp = rp;
	if (
		DisplaySaveStatus (
			mp->rp->baseWGizmo, ProcessCommand (mp, buf, NULL)
		) == True
	) {
		/* Show the new status of this message */
		DisplayStatus (mp, ProcessCommand (mp, FROM_CMD, NULL));
	}
	UnlockCursor(GetBaseWindowShell (rp->baseWGizmo));
}

static void
ReadFileCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ReadRec *		rp = FindReadRec (wid);

	DisplayInLeftFooter (rp->baseWGizmo, "", True);

	switch (p->item_index) {
		case MenuOpen: {
			ReadOpenUser (rp);
			break;
		}
		case MenuSave: {
			ReadSaveCB (wid, client_data, call_data);
			break;
		}
		case MenuSaveAs: {
			ReadSaveAs (rp);
			break;
		}
		case MenuPrint: {
			PrintMessage (rp);
			break;
		}
		case MenuProperties: {
			ReadPropPopupCB (wid, client_data, call_data);
			break;
		}
		case MenuExit: {
			ReadExitCB (wid, client_data, call_data);
			break;
		}
	}
}

static void
SelectUnselectAll (rp, flag)
ReadRec *	rp;
Boolean		flag;
{
	Widget		body;
	int		i;
	ReadMailGizmo *	gizmo;

	gizmo = (ReadMailGizmo *)QueryGizmo (
		BaseWindowGizmoClass, rp->baseWGizmo, GetGizmoGizmo, READ_MAIL
	);
	body = GetReadGizmoBody (gizmo);

	OlTextEditGetLastPosition (body, &i);
	i = flag == False ? 0 : i;
	OlTextEditSetCursorPosition (body, 0, i, i);
}

static void
ReadEditCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ReadRec *		rp = FindReadRec (wid);
	ManageRec *		mng = rp->mp->mng;
	Boolean			flag;

	DisplayInLeftFooter (rp->baseWGizmo, "", True);

	ResetUndo (rp->mp);
	switch (p->item_index) {
		case MenuUndo: {
			ReadUndo (rp);
			flag = (rp->mp->summary->size < 1) ? False : True;
			AdjustMenuBarButtons(rp, flag);
			break;
		}
		case MenuDelete: {
			rp->lastOp = DeleteIt;
			ReadDelete (rp);
			if (mng != (ManageRec *)0) {
				DisplayInLeftFooter (
					mng->baseWGizmo, "", False
				);
			}
			break;
		}
		case MenuSelect: {
			SelectUnselectAll (rp, True);
			break;
		}
		case MenuUnselect: {
			SelectUnselectAll (rp, False);
			break;
		}
	}
}

void
ReadManageCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *	mp = FindMailRec (wid);

	RaiseManageWindow (mp);
}

static void
ReadMailCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ReadRec *		rp = FindReadRec (wid);

	DisplayInLeftFooter (rp->baseWGizmo, "", True);
	LockCursor(GetBaseWindowShell (rp->baseWGizmo));

	switch (p->item_index) {
		case MenuReplySender: {
			ReadReplySenderCB (wid, client_data, call_data);
			break;
		}
		case MenuReplySenderAtt: {
			ReadReplySenderAttCB (wid, client_data, call_data);
			break;
		}
		case MenuReplyAll: {
			ReadReplyAllCB (wid, client_data, call_data);
			break;
		}
		case MenuReplyAllAtt: {
			ReadReplyAllAttCB (wid, client_data, call_data);
			break;
		}
		case MenuForward: {
			ReadForwardCB (wid, client_data, call_data);
			break;
		}
	}
	UnlockCursor(GetBaseWindowShell (rp->baseWGizmo));
}

static void
Insert (mp, cp, hp, index)
MailRec *	mp;
char *		cp;
ListHead *	hp;
int		index;
{
	int	i;
	char **	tmp;
	char **	dst;

	hp->size += 1;
	hp->items = (ListItem *)REALLOC(hp->items, sizeof (ListItem)*hp->size);
	hp->items[hp->size-1].fields = (XtArgVal)MALLOC(sizeof (XtArgVal *)*1);

	for (i=hp->size-1; i>index; i--) {
		hp->items[i].set = hp->items[i-1].set;
		dst = (char **)hp->items[i].fields;
		tmp = (char **)hp->items[i-1].fields;
		dst[0] = tmp[0];
		hp->items[i].clientData = hp->items[i-1].clientData;
	}
	hp->items[index].set = False;
	hp->items[index].clientData = False;
	tmp = (char **)hp->items[index].fields;
	tmp[0] = STRDUP (cp);
}

static int
STRCMP (s1, s2)
char *s1, *s2;
{
	if(s1 == s2) {
		return 0;
	}
	while(toupper (*s1) == toupper (*s2++)) {
     		if(*s1++ == '\0') {
     			return(0);
		}
	}
     	return(toupper (*s1) - toupper (*--s2));
}

Boolean
InsertIntoList (mp, cp, hp, init)
MailRec *	mp;
char *		cp;
ListHead *	hp;
Boolean		init;		/* True = initializing list */
{
	Boolean		found;
	int		i;
	int		eq;
	char **		tmp;

	found = False;
	for (i=0; i<hp->size; i++) {
	    tmp = (char **)hp->items[i].fields;
	    if ((eq = STRCMP (cp, tmp[0])) == 0) {
		/* Since its already in the default list:
		 * if we are initializing the list this means
		 * that the items are coming from an ignore list and
		 * should therefore be unset. 
		 * Otherwise, the item is coming from a mail header 
		 * and the item in the list should remain unchanged.
		 */
		if (init == True) {
			hp->items[i].set = False;		/* Current */
			hp->items[i].clientData = False;	/* Previous */
		}
		found = True;
		break;
	    }
	    if (eq < 0) {
		/* Insert this new item before the item in the list */
		found = True;
		Insert (mp, cp, hp, i);
		break;
	    }
	}
	if (found == False) {
		/* Insert this new item after the item in the list */
		Insert (mp, cp, hp, i);
	}
	return hp->items[i].clientData;	/* Alway use the previous value */
}

static Boolean
AddToBriefList (mp, buf)
MailRec *	mp;
char *		buf;
{
	char *		head;
	char *		cp;
	ListHead *	hp = (ListHead *) BriefList.list;
	Boolean		b = True;
	static Boolean	lastHeadAccepted = False;

	if (buf[0] != ' '  && buf[0] != '\t') {
		head = STRDUP (buf);
		if ((cp = strchr (head, ':')) != NULL) {
			cp[0] = '\0';
			if (strncmp (head, ">From", 5) != 0 &&
			    strncmp (head, "From", 4) != 0) {
				b = InsertIntoList (mp, head, hp, False);
			}
		}
		FREE (head);
		UpdateList (&BriefList);
		lastHeadAccepted = b;
	}
	else {
		/*
		 * If the last header wasn't set in the brief list
		 * then the continuation lines should also be left out.
		 */
		if (lastHeadAccepted == False) {
			b = False;
		}
	}
	
	return b;
}

static char *
LoadNewMail (mp, size, work)
MailRec *	mp;
int *		size;
Buffer *	work;
{
	int		i;
	int		alloced;
	char *		newmail;
	char *		cp;
	size_t		this_size;
	size_t		total_size;

	/* Get "Loaded 1 new message" line
	 * and retrieve the number of new messsages.
	 *
	 * Unlike ReadStringIntoBuffer, ReadFileInfoBuffer() doesn't
	 * insert a `\0' when seeing EOF, this may upset wcstombs().
	 */
	if (ReadFileIntoBuffer(mp->fp[1], work) == EOF)
		work->p[work->used++] = 0;

	this_size = work->used * sizeof(BufferElement) + 1;
	cp = MALLOC(this_size);
	(void)wcstombs(cp, work->p, this_size);
	sscanf (cp, LOADED, size);
	FREE (cp);

	/*
	 * Gather up the messages headers from the New Mail output.
	 */
	for (newmail=NULL, i=0, alloced=0; i<*size; i++) {	

			 /* Unlike ReadStringIntoBuffer, ReadFileInfoBuffer()
			  * doesn't insert a `\0' when seeing EOF, this may
			  * upset wcstombs().
			  */
		if (ReadFileIntoBuffer(mp->fp[1], work) == EOF)
			work->p[work->used++] = 0;

		this_size = work->used * sizeof(BufferElement) + 1;
		newmail = REALLOC (newmail, alloced + this_size);
		cp = newmail + alloced;
		total_size = wcstombs (cp, work->p, this_size);

			/* `2' below is for `\n' and `\0'.
			 * note that total_size excludes `\0'.
			 */
		alloced += (total_size + 2);
		strcat (cp, "\n");
	}
	return newmail;
}

static void
ReadMessage (mp, rp)
MailRec *	mp;
ReadRec *	rp;
{
	TextBuffer *	text;
	TextBuffer *	head;
	TextFileStatus	status = READONLY;
	Boolean		first = True;
	Buffer *	work;
	Boolean		processingHeader = True;
	ReadMailGizmo * gizmo;
	int		i;
	int		size;
	char *		cp;
	char *		newmail = NULL;
	size_t		cp_size;

	gizmo = (ReadMailGizmo *)QueryGizmo (
		BaseWindowGizmoClass, rp->baseWGizmo,
		GetGizmoGizmo, READ_MAIL
	);

	text = AllocateTextBuffer(NULL, NULL, NULL);
	head = AllocateTextBuffer(NULL, NULL, NULL);
	text->status = status;
	head->status = status;

	work = AllocateBuffer(sizeof(BufferElement), LNMIN); 

	for (;;) {
			/* Unlike ReadStringIntoBuffer, ReadFileInfoBuffer()
			 * doesn't insert a `\0' when seeing EOF, this may
			 * upset wcstombs().
			 */
		if (ReadFileIntoBuffer(mp->fp[1], work) == EOF)
			work->p[work->used++] = 0;

		/* Skip over the first line, this is "Message #:" */
		if (first == True) {
			first = False;
			continue;
		}
		cp_size = work->used * sizeof(BufferElement) + 1;
		cp = MALLOC(cp_size);
		(void)wcstombs(cp, work->p, cp_size);
		/* Because of a bug in mailx V4.1 the following test is
		 * needed to catch the end of a message.  This is because
		 * mailx sometimes lies about the size of a message.
		 */
		if (strncmp (PROMPT_TEXT, cp, sizeof (PROMPT_TEXT)-1) == 0) {
			FREE (cp);
			break;
		}
		/*
		 * Check to see if there is any new mail.
		 */
		if (strcmp (cp, NEW_MAIL) == 0) {
			newmail = LoadNewMail (mp, &size, work);
			FREE(cp);	/* free cp */
			continue;
		}
		if (processingHeader == True) {
			if (work->used == 1) {
				processingHeader = False;
				FREE(cp);	/* free cp */
				continue;
			}
	/* Notes for AppendLineToTextBuffer (i.e., InsertLineIntoTextBuffer) -
	 * in JALE (japenese) version (see D. Hamilton), they converted
	 * work->p to multi-bytes before invoking this function. Reading
	 * the code sample in textbuff.c:InsertBlockIntoTextBuffer(),
	 * it seems to me it's not necessary... (Sam Chang, 10/13/94)
	 */
			if (AddToBriefList (mp, cp) == True ||
			    ShowHeader == Full) {
				(void) AppendLineToTextBuffer(head, work);
			}
		}
		else {
			i = strncmp (
				cp, NTS_NOT_PRINTABLE,
				sizeof (NTS_NOT_PRINTABLE)-1
			);
			if (i == 0) {
				DisplayMailText (
					gizmo, GetGizmoText (TXT_BINARY_TEXT),
					NULL, OL_STRING_SOURCE
				);
				FREE (cp);
				return;
			}
			(void) AppendLineToTextBuffer(text, work);
		}
		FREE (cp);
	}
	work->used = 1;
	work->p[0] = '\0';
	(void) AppendLineToTextBuffer(text, work);
	FreeBuffer(work);
	(void) wcGetTextBufferLocation(text, 0, (TextLocation *)NULL);

	DisplayMailText (
		gizmo, head, text, OL_TEXT_BUFFER_SOURCE
	);
	if (newmail != NULL) {
		AddNewMail (mp, newmail, size);
		FREE (newmail);
	}
}

void
GetCurrentItem (mp, rp)
MailRec		*mp;
ReadRec *	rp;
{
	char		cmd[20];

	/* Indicate what popup is to be operated on - used by
	 * ReadMessage() only.
	 */

	/* Look for new items to put into the brief list */

	/* Read the contents of message "item" */
	sprintf (cmd, "%s %d\n=\n", PrintCmd, rp->message);
	WriteToMailx (mp, cmd, strlen (cmd));
	ReadMessage (mp, rp);
	/* Remove the 2 prompts from the input stream */
	ProcessCommand (mp, NULL, NULL);

}

void
ReadItem (mp, rp, i)
MailRec *	mp;
ReadRec *	rp;
int		i;
{
	ReadMailGizmo * gizmo;
	Widget		body;
	Widget		scroller;
	char		buf[BUF_SIZE];

	rp->message = MessageNumber (mp, i);

	/* Read the currents of this mail message and displayit in the popup */

	GetCurrentItem (mp, rp);
	/*
	 * Set the cursor and scrollbar to the start of the message.
	 */
	gizmo = (ReadMailGizmo *)QueryGizmo (
		BaseWindowGizmoClass, rp->baseWGizmo, GetGizmoGizmo, READ_MAIL
	);
	body = GetReadGizmoBody (gizmo);
	OlTextEditSetCursorPosition (body, 0, 0, 0);
	XtVaGetValues (
		XtParent (XtParent (body)), XtNvScrollbar,
		&scroller, (String)0
	);
	XtVaSetValues (scroller, XtNsliderValue, 0, (String)0);

	/* Bring this base window to the surface */

	MapGizmo (
		BaseWindowGizmoClass,
		rp->baseWGizmo
	);

	/* Since the mail has been read its status has changed - update it */

	UpdateStatusOfMessage (mp, i, i+1);
	if (strcmp (GetUserMailFile (), mp->filename) == 0) {
		sprintf (
			buf, "%s: %d",
			GetGizmoText (TXT_EMAIL_READER),
			rp->message
		);
	}
	else {
		sprintf (
			buf, "%s %s: %d",
			GetGizmoText (TXT_EMAIL_READER),
			mp->filename,
			rp->message
		);
	}
	SetBaseWindowTitle (rp->baseWGizmo, buf);
}

void
SelectItem (mp, lp, item)
MailRec *	mp;
ListHead *	lp;
int		item;
{
	Widget	list;
	int	i;

	if (item == -1) {
		return;
	}
	list = (Widget)0;
	LastSelectedMessage = -1;
	if (lp->items != (ListItem *)0) {
		LastSelectedMessage = (int)lp->items[item].clientData;
	}
	LastMailRec = mp;
	if (mp->mng != (ManageRec *)0) {
		if (lp == mp->summary) {
			list = GetSummaryListWidget (mp->mng);
		}
		else if (lp == mp->deleted
		     && mp->mng->deleteListPopup != (PopupGizmo *)0) {
			list = GetDeletedListWidget (mp->mng);
			LastSelectedMessage = -1;
			LastMailRec = (MailRec *)0;
		}
	}
	if (lp->items != (ListItem *)0 && lp->items[item].set == False) {
		if (list != (Widget)0) {
			OlVaFlatSetValues (
				list, item, XtNset, True, (String)0
			);
			XtVaSetValues (
				list, XtNviewItemIndex, item, (String)0
			);
		}
		else {
			lp->items[item].set = True;
		}
	}
	lp->clientData = (XtArgVal)0;
	for (i=0; i<lp->size; i++) {
		if (lp->items[i].set == True) {
			lp->clientData += (XtArgVal)1;
		}
	}
	FPRINTF ((stderr, "SelectItem() items selected=%d\n", lp->clientData));
}

void
UnselectItem (mp, lp, item)
MailRec *	mp;
ListHead *	lp;
int		item;
{
	Widget	list;
	int	i;

	if (lp->items[item].set == True) {
		list = (Widget)0;
		if (mp->mng != (ManageRec *)0) {
			if (lp == mp->summary) {
				list = GetSummaryListWidget (mp->mng);
			}
			else if ( lp == mp->deleted &&
				mp->mng->deleteListPopup != (PopupGizmo *)0) {
				list = GetDeletedListWidget (mp->mng);
			}
		}
		if (list != (Widget)0) {
			OlVaFlatSetValues (
				list, item, XtNset, False, (String)0
			);
		}
		else {
			lp->items[item].set = False;
		}
	}
	/* Indicate there was no last selected message */
	LastSelectedMessage = -1;
	LastMailRec = (MailRec *)0;
	lp->clientData = (XtArgVal)0;
	for (i=0; i<lp->size; i++) {
		if (lp->items[i].set == True) {
			lp->clientData += (XtArgVal)1;
		}
	}
}

Boolean
Prev (rp)
ReadRec *	rp;
{
	MailRec *	mp = rp->mp;
	int		i, j;
	Boolean		itemFound = False;
	char		buf[BUF_SIZE];
	ReadRec *	firstrp = (ReadRec *)0;
	ReadRec *	prevrp;
	Widget		shell;

	for (i=ItemNumber(mp, rp->message)-1; i>= 0; i--) {
		/* Don't read an already displayed item */
		if (
			(prevrp=FindMessageInList (mp, MessageNumber (mp, i))
			) == (ReadRec *)0
		) {
			/* Unselect all other items */
			for (j=0; j<mp->summary->size; j++) {
				UnselectItem (mp, mp->summary, j);
			}
			SelectItem (mp, mp->summary, i);
			ReadItem (mp, rp, i);
			itemFound = True;
			break;
		}
		else {
			/* Remember the first read window for later */
			if (firstrp == (ReadRec *)0) {
				firstrp = prevrp;
			}
		}
	}
	if (itemFound == False) {
		/* We didn't find any message that wasn't already displayed
		 * so popup the first message before this one. */
		if (firstrp != (ReadRec *)0) {
			shell = GetBaseWindowShell (firstrp->baseWGizmo);
			XRaiseWindow (XtDisplay (shell), XtWindow (shell));
			MapGizmo (BaseWindowGizmoClass, firstrp->baseWGizmo);
		}
		else {
			sprintf (buf, GetGizmoText (TXT_AT_FIRST), mp->filename);
			DisplayInLeftFooter (rp->baseWGizmo, buf, True);
		}
	}
	return itemFound;
}

Boolean
Next (rp)
ReadRec *	rp;
{
	MailRec *	mp = rp->mp;
	int		i, j;
	Boolean		itemFound = False;
	char		buf[BUF_SIZE];
	ReadRec *	firstrp = (ReadRec *)0;
	ReadRec *	nextrp;
	Widget		shell;

	for (i=ItemNumber(mp, rp->message)+1; i<mp->summary->size; i++) {
		/* Don't read an already displayed item */
		if (
			(nextrp=FindMessageInList (mp, MessageNumber (mp, i))
			) == (ReadRec *)0
		) {
			/* Unselect all other items */
			for (j=0; j<mp->summary->size; j++) {
				UnselectItem (mp, mp->summary, j);
			}
			/* And select the next item */
			FPRINTF ((stderr, "select (%d)\n", i));
			SelectItem (mp, mp->summary, i);
			FPRINTF ((stderr, "read (%d)\n", i));
			ReadItem (mp, rp, i);
			itemFound = True;
			break;
		}
		else {
			/* Remember the first read window for later */
			if (firstrp == (ReadRec *)0) {
				firstrp = nextrp;
			}
		}
	}
	if (itemFound == False) {
		/* We didn't find any message that wasn't already displayed
		 * so popup the first message after this one. */
		if (firstrp != (ReadRec *)0) {
			shell = GetBaseWindowShell (firstrp->baseWGizmo);
			XRaiseWindow (XtDisplay (shell), XtWindow (shell));
			MapGizmo (BaseWindowGizmoClass, firstrp->baseWGizmo);
		}
	}
	return itemFound;
}

static void
ReadViewCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	ReadRec *		rp = FindReadRec (wid);

	DisplayInLeftFooter (rp->baseWGizmo, "", True);
	if (rp->mp->mng != (ManageRec *)0) {
		DisplayInLeftFooter (rp->mp->mng->baseWGizmo, "", False);
	}

	switch (p->item_index) {
		case MenuNext: {
			(void)Next (rp);
			break;
		}
		case MenuPrev: {
			Prev (rp);
			break;
		}
	}
}


static void
FileSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	MailRec *	mp = rp->mp;
	Boolean		flag = True;
        Arg             arg[2];

	if (rp->message == -1) {
		flag = False;
	}
	SetSensitivity (rp->baseWGizmo, "readFile", 1, 3, flag);

        XtSetArg(arg[0], XtNsensitive, (flag == True) ? 1 : 0);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadSaveMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadPrintMenu"),
                arg, 1);
}

static void
EditSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	MailRec *	mp = rp->mp;
	Boolean		flag = True;

	if (rp->lastOp == UndoIt) {
		flag = False;
	}
	SetSensitivity (rp->baseWGizmo, "readEdit", 0, 0, flag);

	flag = True;
	if (rp->message == -1) {
		flag = False;
	}
	SetSensitivity (rp->baseWGizmo, "readEdit", 1, 3, flag);
}

static void
ViewSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	MailRec *	mp = rp->mp;
	Boolean		next = True;
	Boolean		prev = True;
	int		item;
	Arg		arg[2];

	/*
	 * Set the sensitivity of Next and Prev buttons
	 */
	item = ItemNumber (mp, rp->message);	/* Get this item number */
	if (mp->summary->size < 2 || item == -1) {
		next = False;
		prev = False;
	}
	if (item == 0) {
		prev = False;
	}
	if (item == mp->summary->size-1) {
		next = False;
	}
	SetSensitivity (rp->baseWGizmo, "readView", 0, 0, next);
	SetSensitivity (rp->baseWGizmo, "readView", 1, 1, prev);
        XtSetArg(arg[0], XtNsensitive, (prev == True) ? 1 : 0);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadPrevMenu"),
                arg, 1);
        XtSetArg(arg[0], XtNsensitive, (next == True) ? 1 : 0);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadNextMenu"),
                arg, 1);
}

static void
MailSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	MailRec *	mp = rp->mp;
	Boolean		flag = True;
	Arg		arg[2];

	if (rp->message == -1) {
		flag = False;
	}
	SetSensitivity (rp->baseWGizmo, "read_Mail", 0, 4, flag);
	AdjustMenuBarButtons(rp, flag);
}


static void
barNextMsgCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	MailRec *	mp = rp->mp;
	Boolean		next = True;
	int		item;

	/*
	 * Set the sensitivity of Next and Prev buttons
	 */
	item = ItemNumber (mp, rp->message);	/* Get this item number */
	if (mp->summary->size < 2 || item == -1) {
		next = False;
	}
	if (item == mp->summary->size-1) {
		next = False;
	}
	if (next) {
		(void)Next (rp);
        	SetSensitivity (rp->baseWGizmo, "readView", 1, 1, True);
		XtVaSetValues((Widget)QueryGizmo (
				BaseWindowGizmoClass, rp->baseWGizmo,
				GetGizmoWidget, "barReadPrevMenu"),
			XtNsensitive, True, NULL);
		if (item == mp->summary->size-2)
        		XtVaSetValues((Widget)QueryGizmo (
					BaseWindowGizmoClass, rp->baseWGizmo,
					GetGizmoWidget, "barReadNextMenu"),
        			XtNsensitive, False, NULL);
	}
	else {
		_OlBeepDisplay(GetBaseWindowShell (manageRec->baseWGizmo), 1);
		SetSensitivity (rp->baseWGizmo, "readView", 0, 0, False);
        	XtVaSetValues((Widget)QueryGizmo (
				BaseWindowGizmoClass, rp->baseWGizmo,
				GetGizmoWidget, "barReadNextMenu"),
        		XtNsensitive, False, NULL);
	}
}

static void
barPrevMsgCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	MailRec *	mp = rp->mp;
	Boolean		prev = True;
	int		item;

	/*
	 * Set the sensitivity of Next and Prev buttons
	 */
	item = ItemNumber (mp, rp->message);	/* Get this item number */
	if (mp->summary->size < 2 || item == -1) {
		prev = False;
	}
	if (item == 0) {
		prev = False;
	}
	if (prev) {
		(void)Prev (rp);
        	SetSensitivity (rp->baseWGizmo, "readView", 0, 0, True);
		XtVaSetValues((Widget)QueryGizmo (
				BaseWindowGizmoClass, rp->baseWGizmo,
				GetGizmoWidget, "barReadNextMenu"),
			XtNsensitive, True, NULL);
		if (item == 1)
        		XtVaSetValues((Widget)QueryGizmo (
					BaseWindowGizmoClass, rp->baseWGizmo,
					GetGizmoWidget, "barReadPrevMenu"),
        			XtNsensitive, False, NULL);
	}
	else {
		_OlBeepDisplay(GetBaseWindowShell (manageRec->baseWGizmo),
				1);
		SetSensitivity (rp->baseWGizmo, "readView", 1, 1, False);
        	XtVaSetValues((Widget)QueryGizmo (
				BaseWindowGizmoClass, rp->baseWGizmo,
				GetGizmoWidget, "barReadPrevMenu"),
        		XtNsensitive, False, NULL);
	}
}

static void
barPrintCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *               rp = FindReadRec (wid);

	LockCursor(GetBaseWindowShell (rp->baseWGizmo));
	PrintMessage (rp);
	UnlockCursor(GetBaseWindowShell (rp->baseWGizmo));
}

static void
barDeleteCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *		rp = FindReadRec (wid);
	ManageRec *		mng = rp->mp->mng;
	Boolean		flag = True;

	ResetUndo (rp->mp);
	rp->lastOp = DeleteIt;
	ReadDelete (rp);
	if (rp->mp->summary->size < 1) {
		flag = False;
	}
	AdjustMenuBarButtons(rp, flag);
}

ReadMailGizmo ReadMail = {
	READ_MAIL
};

#define T	(XtPointer)True
#define F	(XtPointer)False


static MenuItems barNextItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barNextMsgCB},
	{NULL}
};
static MenuItems barPrevItems[] = {
	{True, "",	(XtArgVal)0,	NULL, barPrevMsgCB},
	{NULL}
};
static MenuItems barSaveItems[] = {
	{True, "", (XtArgVal)0, NULL, ReadSaveCB},
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
	{True, "",	(XtArgVal)0,	NULL, ReadReplySenderAttCB},
	{NULL}
};
static MenuItems barReplyAllItems[] = {
	{True, "",	(XtArgVal)0,	NULL, ReadReplyAllAttCB},
	{NULL}
};
static MenuItems barForwardItems[] = {
	{True, "",	(XtArgVal)0,	NULL, ReadForwardCB},
	{NULL}
};

static MenuGizmo barNextMenu = {
	NULL, "barReadNextMenu", NULL, barNextItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barPrevMenu = {
	NULL, "barReadPrevMenu", NULL, barPrevItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barSaveMenu = {
	NULL, "barReadSaveMenu", NULL, barSaveItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barPrintMenu = {
	NULL, "barReadPrintMenu", NULL, barPrintItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barDeleteMenu = {
	NULL, "barReadDeleteMenu", NULL, barDeleteItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barReplySenderMenu = {
	NULL, "barReadReplySenderMenu", NULL, barReplySenderItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barReplyAllMenu = {
	NULL, "barReadReplyAllMenu", NULL, barReplyAllItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barForwardMenu = {
	NULL, "barReadForwardMenu", NULL, barForwardItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};

static GizmoRec buttonArray0[] = {
	{MenuBarGizmoClass,	&barNextMenu}
};

static GizmoRec buttonArray1[] = {
	{MenuBarGizmoClass,	&barPrevMenu}
};

static GizmoRec buttonArray2[] = {
	{MenuBarGizmoClass,	&barSaveMenu}
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

static Arg buttonArgs[] = {
	XtNhPad,	0
};

static LabelGizmo label1 = {
	NULL, "readerBarNext", "",
	buttonArray0, XtNumber (buttonArray0),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label0 = {
	NULL, "readerBarPrev", "",
	buttonArray1, XtNumber (buttonArray1),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label2 = {
	NULL, "readerBarSave", "",
	buttonArray2, XtNumber (buttonArray2),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label3 = {
	NULL, "readerBarPrint", "",
	buttonArray3, XtNumber (buttonArray3),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label4 = {
	NULL, "readerBarDelete", "",
	buttonArray4, XtNumber (buttonArray4),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label5 = {
	NULL, "readerBarReplySender", "",
	buttonArray5, XtNumber (buttonArray5),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label6 = {
	NULL, "readerBarReplyAll", "",
	buttonArray6, XtNumber (buttonArray6),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label7 = {
	NULL, "readerBarForward", "",
	buttonArray7, XtNumber (buttonArray7),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static GizmoRec readBar[] = {
	{LabelGizmoClass,	&label0},
	{LabelGizmoClass,	&label1},
	{LabelGizmoClass,	&label5},
	{LabelGizmoClass,	&label6},
	{LabelGizmoClass,	&label7},
	{LabelGizmoClass,	&label2},
	{LabelGizmoClass,	&label3},
	{LabelGizmoClass,	&label4},
};

static Arg labelArgs[] = {
	XtNhSpace,	0,
	XtNhPad,	0
};

static LabelGizmo pixmapLabel = {
	NULL, "readPixmaps", "", readBar, XtNumber (readBar),
	OL_FIXEDROWS, 1, labelArgs, 2, False
};

static GizmoRec readGiz[] = {
	{LabelGizmoClass,	&pixmapLabel},
	{ReadMailGizmoClass,	&ReadMail}
};

/* Read Base Window Menubar ------------------------------------------- */

static MenuItems fileItems[] = {
    {True, BUT_OPEN_DDD,		MNEM_OPEN_DDD},
    {True, BUT_SAVE_MESSAGE,		MNEM_SAVE_MESSAGE},
    {True, BUT_SAVE_MESSAGE_AS_DDD,	MNEM_SAVE_MESSAGE_AS_DDD},
    {True, BUT_PRINT,			MNEM_PRINT},
    {True, BUT_READ_PROP,		MNEM_READ_PROP},
    {True, BUT_EXIT,			MNEM_EXIT_X},
    {NULL}
};
static MenuGizmo file = {
	NULL, "readFile", NULL, fileItems, ReadFileCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems editItems[] = {
    {True, BUT_UNDO,		MNEM_UNDO},
    {True, BUT_DELETE,		MNEM_DELETE},
    {True, BUT_SELECT_ALL,	MNEM_SELECT_ALL},
    {True, BUT_UNSELECT_ALL,	MNEM_UNSELECT_ALL},
    {NULL}
};
static MenuGizmo edit = {
	NULL, "readEdit", NULL, editItems, ReadEditCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mailItems[] = {
    {True, BUT_REPLY_SENDER_DDD,	MNEM_REPLY_SENDER_DDD},
    {True, BUT_REPLY_SENDER_ATT_DDD,	MNEM_REPLY_SENDER_ATT_DDD},
    {True, BUT_REPLY_ALL_DDD,		MNEM_REPLY_ALL_DDD},
    {True, BUT_REPLY_ALL_ATT_DDD,	MNEM_REPLY_ALL_ATT_DDD},
    {True, BUT_FORWARD_DDD,		MNEM_FORWARD_DDD},
    {NULL}
};

static MenuGizmo mail = {
	NULL, "read_Mail", NULL, mailItems, ReadMailCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems viewItems[] = {
    {True, BUT_NEXT,	MNEM_NEXT,	 0, 0, "Ctrl<+>"},
    {True, BUT_PREV,	MNEM_PREV,	 0, 0, "Ctrl<->"},
    {NULL}
};
static MenuGizmo view = {
	NULL, "readView", NULL, viewItems, ReadViewCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems readHelpItems[] = {
    {True, BUT_READER_DDD,	MNEM_READER_DDD,NULL,NULL,(XtPointer)HelpReader},
    {True, BUT_TOC_DDD,		MNEM_TOC_DDD,	NULL,NULL,(XtPointer)HelpTOC},
    {True, BUT_HELP_DESK_DDD,	MNEM_HELP_DESK_DDD,NULL,NULL,(XtPointer)HelpDesk},
    {NULL}
};
static MenuGizmo readHelp = {
	NULL, "readHelp", NULL, readHelpItems, HelpCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems readItems[] = {
    {True, BUT_FILE,	MNEM_FILE,		(char *) &file, FileSelectCB},
    {True, BUT_EDIT,	MNEM_EDIT,		(char *) &edit, EditSelectCB},
    {True, BUT_VIEW,	MNEM_VIEW,		(char *) &view, ViewSelectCB},
    {True, BUT_MESS,	MNEM_MESS,		(char *) &mail,	MailSelectCB},
    {True, BUT_HELP,	MNEM_HELP,		(char *) &readHelp},
    {NULL}
};

static MenuGizmo read =     {
	NULL, "readMenu", NULL, readItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

static BaseWindowGizmo ReadWindow = {
	&ReaderHelp,
	"read",
	"",
	&read,
	readGiz,
	XtNumber (readGiz),
	TXT_READER_ICON_NAME,
	"rdmail.icon",
	" ",
	" ",
	50
};

void
ClientDestroyCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);

	XtDestroyWidget (((BaseWindowGizmo *)client_data)->icon_shell);
	FreeGizmo (BaseWindowGizmoClass, (Gizmo)client_data);
}

MailRec *
GetMailx (inode)
o_ino_t	inode;
{
	MailRec *	mp;

	/* Look for a mailx that is open to the default file */
	for (mp=mailRec; mp!=(MailRec *)0; mp=mp->next) {
		if (inode == mp->inode) {
			return mp;
		}
	}
	return (MailRec *)0;
}

int
GetLastSelectedItem (mp)
MailRec *	mp;
{
	int	i;
	int	last = -1;

	for (i=0; i<mp->summary->size; i++) {
		if (mp->summary->items[i].set == True) {
			last = i;
		}
	}
	return last;
}

/* Only a mail file but display a read window and the
 * associated manage window (old name, may change later).
 */
MailRec *
OpenReadWindowOnly (filename, bw)
char *			filename;
BaseWindowGizmo *	bw;
{
	MailRec *	mp = mailRec;
	ReadRec *	rp;
	int		item;
	o_ino_t		inode;
	char *		nomail = NULL;
	char		buf[BUF_SIZE];
	
	/*
	 * If there are no open mailfiles yet, switch the first mailx
	 * over to the new file.
	 */
	if (nomail == NULL) {
		nomail = GetGizmoText (TXT_NO_MAIL);
	}
	if (mp->inode == DummyDir) {
		if (SwitchMailx (mp, filename, bw) == False) {
			return (MailRec *)0;
		}
		if (mp->noMail == True) {
			/* Open was successful, but there was no mail
			 * so just display "No mail" in the window
			 */
			DisplayInLeftFooter (bw, nomail, True);
			FREE (mp->filename);
			mp->filename = STRDUP (DUMMY_FILE);
			return mp;
		}
	}
	/*
	 * Look for a mailx that is open to the specified mail file.
	 */
	(void)StatFile(filename, &inode, (off_t *)0);
	if ((mp = GetMailx (inode)) == (MailRec *)0) {
		/* If none existes - create one */
		mp = OpenMailx ();
		/* If file doesn't exist SwitchMailx will catch it. */
		if (SwitchMailx (mp, filename, bw) == False) {
			/* No mail file */
			QuitMailx (mp);
			return (MailRec *)0;
		}
		if (mp->noMail == True) {
			/* Open was successful, but there was no mail
			 * so just display "No mail" in the window
			 */
			DisplayInLeftFooter (bw, nomail, True);
			FREE (mp->filename);
			mp->filename = STRDUP (DUMMY_FILE);
			return mp;
		}
		if (mp->mng == (ManageRec *)0)
			CreateManageRec(mp);
		rp = CreateReadRec (mp);
		item = mp->defaultItem;
		ReadItem (rp->mp, rp, item);
	}
	else {
		/* If one does exist see if there is any mail
		 * associated with it.
		 */
		if (mp->noMail == True) {
			DisplayInLeftFooter (bw, nomail, True);
			return mp;
		}
		/* If one does exist see if there is a manage window up.
		 * If there is and an items is set then this message 
		 * should be displayed
		 */
		item = -1;
		if (mp->mng == (ManageRec *)0)
			CreateManageRec(mp);
		if (mp->mng != (ManageRec *)0 && mp->mng->mapped == True) {
			item = GetLastSelectedItem (mp);
		}
		/* Otherwise, use the default item */
		if (item == -1) {
			item = mp->defaultItem;
		}
		/* Now, see if a read window existes for this item */
		rp = FindMessageInList (mp, MessageNumber (mp, item));
		if (rp != (ReadRec *)0) {
			DisplayAlreadyOpen (rp->baseWGizmo, rp->mp->filename);
		}
		else {
			if (mp->summary->size != 0) {
				/* Otherwise, create a new read window */
				rp = CreateReadRec (mp);
				ReadItem (rp->mp, rp, item);
			}
			else {
				sprintf (
					buf,
					GetGizmoText (TXT_NO_MORE_MESSAGES),
					mp->filename
				);
				DisplayInLeftFooter (bw, buf, True);
			}
		}
	}
	UpdateFooter (mp);
	return mp;
}

static void
ReadAppProc(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
	MailRec *	mp;
	char **		p;
	ReadRec *	rp = FindReadRec (wid);

	if (dip->error != 0) {
		return;
	}
	if (dip->nitems != 1) {
		DisplayErrorPrompt (
			GetBaseWindowShell (rp->baseWGizmo),
			GetGizmoText (TXT_ONLY_ONE_FILE)
		);
		return;
	}
	if (dip->files == (char **)0) {
		return;
	}
	mp = OpenReadWindowOnly (*dip->files, rp->baseWGizmo);
	if (mp == (MailRec *)0) {
		FPRINTF ((stderr, "Need to notify of failure\n"));
	}
}

static OlDnDTMNotifyProc
ReadDropNotify (
	Widget w, Window win, Position x, Position y, Atom selection,
	Time timestamp, OlDnDDropSiteID drop_site_id,
	OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
	XtPointer closure)
{
	FPRINTF ((stderr, "Got a drop on the read window\n"));
	DtGetFileNames (
		w, selection, timestamp, send_done, ReadAppProc, closure
	);
}

static
void
ReaderInFocus(w, client_data, event, continue_to_dispatch_return)
Widget w;
XtPointer client_data;
XEvent *event;
Boolean *continue_to_dispatch_return;
{
	ReadRec* rp = (ReadRec *)client_data;
	XFocusChangeEvent *fe = (XFocusChangeEvent *)event;

	if (fe->type == FocusIn)
		rp->parent->topreader = rp;
	*continue_to_dispatch_return = True;
}

static void
ManageMiniHelp(w, client, event)
Widget  w;
XtPointer       client;
XEvent *event;
{
	ReadRec *	rp = FindReadRec (w);

        DisplayInLeftFooter (rp->baseWGizmo,
                (event->type == EnterNotify) ? client : "",
                False);
}

ReadRec *
CreateReadRec (mp)
MailRec *mp;
{
	Arg			arg[10];
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)ReadExitCB},
		{NULL}
	};
	MenuGizmo *	gizmo;
	ReadRec *	rp;
	ManageRec *	mng;
	Widget		shell;
	char		buf[BUF_SIZE];
	Widget		menuWidget;
	Widget		cap;
	static char *	flatMenuFields[] = {
		XtNsensitive,  /* sensitive                      */
		XtNlabel,      /* label                          */ 
		XtNuserData,   /* mnemonic string		 */
		XtNuserData,   /* nextTier | resource_value      */
		XtNselectProc, /* function                       */
		XtNaccelerator,/* client_data                    */
		XtNset,        /* set                            */
		XtNpopupMenu,  /* button                         */
		XtNmnemonic,   /* mnemonic                       */ 
	};

	/* Create the popup shell to contain the mail message */

	FPRINTF((stderr, "entering CreateReadRec\n"));
	if (mp->mng == (ManageRec *)0)
		CreateManageRec(mp);
	mng = mp->mng;
	mp->numBaseWindows += 1;
	rp = (ReadRec *)MALLOC (sizeof (ReadRec));
	FPRINTF((stderr, "CreateReadRec: new rp=0x%x\n", rp));
	rp->mp = mp;
	rp->readSaveAsPopup = (FileGizmo *)0;
	rp->readOpenUserPopup = (FileGizmo *)0;
	rp->lastOp = UndoIt;
	mp->rp = rp;

	rp->next = readRec;
	readRec = rp;

	rp->parent = mng;
	if (mng->topreader)
	{
		rp->mngnext = mng->topreader;
		rp->mngprev = mng->topreader->mngprev;
		rp->mngnext->mngprev = rp;
		rp->mngprev->mngnext = rp;
	}
	else
	{
		rp->mngnext = rp->mngprev = rp;
	}
	mng->topreader = rp;
	mng->readercnt++;

	/* sprintf (buf, GetGizmoText (TXT_EMAIL_READER), mp->filename); */
	sprintf (buf, GetGizmoText (TXT_EMAIL_READER));
	ReadWindow.title = buf;
	rp->baseWGizmo = CopyGizmo (BaseWindowGizmoClass, &ReadWindow);
	XtSetArg(arg[0], XtNwmProtocol, protocolCB);
	shell = CreateGizmo (
		Root, BaseWindowGizmoClass, rp->baseWGizmo, arg, 1
	);
	OlDnDRegisterDDI (
		rp->baseWGizmo->icon_shell, OlDnDSitePreviewNone,
		(OlDnDTMNotifyProc)ReadDropNotify, (OlDnDPMNotifyProc)NULL,
		True, (XtPointer)NULL
	);
	OlDnDRegisterDDI (
		rp->baseWGizmo->form, OlDnDSitePreviewNone,
		(OlDnDTMNotifyProc) ReadDropNotify,
		(OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL
	);

	/* Change the item fields to add in a background pixmap */
	menuWidget = (Widget)QueryGizmo (
		BaseWindowGizmoClass, rp->baseWGizmo,
		GetGizmoWidget, "readView"
	);
	gizmo = (MenuGizmo *)QueryGizmo (
		BaseWindowGizmoClass, rp->baseWGizmo,
		GetGizmoGizmo, "readView"
	);
	XtVaSetValues (
		menuWidget,
		XtNitemFields,		flatMenuFields,
		XtNnumItemFields,	XtNumber(flatMenuFields),
		XtNitems,		gizmo->items,
		XtNnumItems,		XtNumber (viewItems)-1,
		(String)0
	);
	XtVaSetValues (
                QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "readPixmaps"
                ),
                XtNweight,      0,
                (String)0
        );

	XtAddEventHandler(shell, FocusChangeMask, False,
				ReaderInFocus, (XtPointer)rp);

	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadNextMenu"
		)),
		"m.nextmsg24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_NEXT)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadPrevMenu"
		)),
		"m.prevmsg24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_PREVIOUS)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadSaveMenu"
		)),
		"m.save24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_SAVE)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadPrintMenu"
		)),
		"dtm.print.24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_PRINT)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadDeleteMenu"
		)),
		"m.delete24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_DELETE)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadReplySenderMenu"
		)),
		"m.reply.snd24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_REPLY)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadReplyAllMenu"
		)),
		"m.reply.all24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_REPLYALL)
	);
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, rp->baseWGizmo,
			GetGizmoWidget, "barReadForwardMenu"
		)),
		"m.forward24"
	);
	XtAddEventHandler(cap,
		EnterWindowMask | LeaveWindowMask,
		False,
		(XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_READ_FORWARD)
	);
	if (mp->numBaseWindows == 1) {
		SelectItem (mp, mp->summary, mp->defaultItem);
	}

	XtSetArg(arg[0], XtNposition, OL_RIGHT);
        XtSetValues(XtParent(XtParent(XtParent(XtParent(cap)))), arg, 1);

	FPRINTF((stderr, "CreateReadRec done\n"));
	return rp;
}

static int  (*prev_xerror_handler) (Display *, XErrorEvent *);

static int
CatchXError (Display *	dpy, XErrorEvent * xevent)
{
    switch (xevent->error_code)
    {
    case BadWindow:	/* destroyed? */
    case BadMatch:	/* not viewable? */
	/* Ignore these errors because there are
	 * chances, a window is unmapped by
	 * a window manager or is destroyed
	 * by an application, while assigning
	 * the input focus...
	 */
	break;
    default:
	/* Report other error as usual... */
	return((*prev_xerror_handler)(dpy, xevent));
	break;
    }
    return(1);
} /* end of CatchXError */

void
ReadProc (mp, isInPlace)
MailRec *	mp;
Boolean isInPlace;
{
	int		i;
	ReadRec *	rp;
	Widget		shell;
	Boolean		itemSet = False;
	int		setcnt = 0;
	Display	*	readerDisplay;
	Window		readerWindow;

	FPRINTF((stderr, "ReadProc called: isInPlace=%d\n", isInPlace));
	for (i=0; i<mp->summary->size; i++) {
		if (mp->summary->items[i].set == True) {
			setcnt++;
		}
	}
	FPRINTF((stderr, "ReadProc: setcnt=%d\n", setcnt));
	if (setcnt == 0)
		return;
	if (setcnt > 1)
		isInPlace = False;
	for (i=0; i<mp->summary->size; i++) {

		/* For each item set in the list do a read */

		if (mp->summary->items[i].set == True) {
			itemSet = True;
			/* First look to see if item is already
			 * displayed.
			 */
			if ((
				rp = FindMessageInList (
					mp, MessageNumber (mp, i)
				)
			) == (ReadRec *)0) {
				/* If not create a new popup and add it to the list */
				FPRINTF((stderr, "ReadProc:reader not found, isInPlace=%d, top=0x%x\n",
					isInPlace, mp->mng->topreader));
				if (!isInPlace ||
					    mp->mng->topreader == (ReadRec *)0)

					rp = CreateReadRec (mp);
				else
				{
					rp = mp->mng->topreader;
					shell = GetBaseWindowShell (rp->baseWGizmo);
					FPRINTF((stderr, "ReadProc: raising topreader: shell=0x%x\n", shell));
					readerDisplay = XtDisplay (shell);
					readerWindow = XtWindow (shell);
					XRaiseWindow (
						readerDisplay,
						readerWindow
					);
					FPRINTF((stderr, "ReadProc: setting Input Focus\n"));
					XSync(readerDisplay, False);
					prev_xerror_handler = XSetErrorHandler(CatchXError);
					XSetInputFocus(readerDisplay,
							readerWindow,
							RevertToNone,
							CurrentTime);
					XSync(readerDisplay, False);
					(void)XSetErrorHandler(prev_xerror_handler);

					FPRINTF((stderr, "ReadProc: Input Focus done\n"));
				}
			}
			else {
				/* The window already existes so raise it
				 * to the top.
				 */
				shell = GetBaseWindowShell (rp->baseWGizmo);
				FPRINTF((stderr, "ReadProc: raising matched reader: shell=0x%x\n", shell));
				readerDisplay = XtDisplay (shell);
				readerWindow = XtWindow (shell);
				XRaiseWindow (
					readerDisplay,
					readerWindow
				);
				FPRINTF((stderr, "ReadProc: setting Input Focus\n"));
				XSync(readerDisplay, False);
				prev_xerror_handler = XSetErrorHandler(CatchXError);
				XSetInputFocus(readerDisplay,
						readerWindow,
						RevertToNone,
						CurrentTime);
				XSync(readerDisplay, False);
				(void)XSetErrorHandler(prev_xerror_handler);
				FPRINTF((stderr, "ReadProc: Input Focus done\n"));
			}
			FPRINTF((stderr, "ReadProc: ReadItem i=%d\n", i));
			ReadItem (mp, rp, i);
		}
	}
	if (itemSet == False) {
		DisplayInLeftFooter (
			mp->mng->baseWGizmo, GetGizmoText (TXT_NO_ITEM), True
		);
	}
	UpdateFooter (mp);

	ViewSelectCB (GetBaseWindowShell (rp->baseWGizmo), NULL, NULL);
}

void
ReadOpenCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	ReadRec *	rp = FindReadRec (wid);
	char *		filename;
	char *		tmp;
	int		n;
	int		flag;

	n = ExpandFileGizmoFilename(rp->readOpenUserPopup, &flag);
	FPRINTF ((stderr, "n=%d flag=%d\n", n, flag));
	if (n > 1) {
		return;
	}
	if (n == 0) {
		tmp = GetFilePath (rp->readOpenUserPopup);
		filename = AddDotMl (tmp);
		FREE (tmp);
	}
	else {
		filename = GetFilePath (rp->readOpenUserPopup);
	}
	if (OpenReadWindowOnly (filename, rp->baseWGizmo) != (MailRec *)0) {
		BringDownPopup (
			GetFileGizmoShell(rp->readOpenUserPopup)
		);
	}
	FREE (filename);
}

static void AdjustMenuBarButtons(rp, flag)
ReadRec *rp;
Boolean flag;
{

	Arg	arg[2];

        XtSetArg(arg[0], XtNsensitive, (flag == True) ? 1 : 0);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadNextMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadPrevMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadSaveMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadPrintMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadDeleteMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadReplySenderMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadReplyAllMenu"),
                arg, 1);
        XtSetValues((Widget)QueryGizmo (
                        BaseWindowGizmoClass, rp->baseWGizmo,
                        GetGizmoWidget, "barReadForwardMenu"),
                arg, 1);
}
