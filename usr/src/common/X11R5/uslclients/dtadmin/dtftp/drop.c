/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:drop.c	1.1"
#endif

#include "ftp.h"
#include <libgen.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_ATOMS	100

Atom		Atoms[MAX_ATOMS];
PFV		Callbacks[MAX_ATOMS];
XtPointer	ClientData[MAX_ATOMS];
Boolean		StaleRequest[MAX_ATOMS];
Widget		Shell[MAX_ATOMS];

typedef struct _TargetData {
	DropObject *		dobj;
	OlFlatDropCallData *	p;
	int			item_index;
} TargetData;

static void
_ExecOpen (CmdPtr cp)
{
	DtPropList	plist;
	char *		value;
	DropObject *	dobj = (DropObject *)cp->clientData;

	/* dobj->destfile can be NULL because of a get error. */
	if (dobj->destfiles[dobj->fileindex-1] != NULL) {
		plist.count = 0;
		plist.ptr = NULL;
		DtAddProperty (&plist, "S", cp->srcfile, NULL);
		DtAddProperty (&plist, "s", basename(cp->srcfile), NULL);
		value = (char *)DtExpandProperty (dobj->exec, &plist);
		FPRINTF ((stderr, "DtExecuteShellCmd (%s)\n", value));
		if (DtExecuteShellCmd (value) != 1) {
			FPRINTF ((stderr, "Exec failed\n"));
		}
	}
	FreeDobj (dobj);
}

static void
ExecOpen (CnxtRec *cr, char *name, DropObject *dobj)
{
	/* The name can be NULL because the last few files copies */
	/* could have been canceled (No Overwrite).  It is neccessary */
	/* to call dobj->cmd for closure. */
	if (name == NULL) {
		FreeDobj (dobj);
	}
	else {
		QueueCmd (
			"exec drop command",
			cr, _ExecOpen, 0, cr->top->group, name, 0,
			(XtPointer)dobj, 0, Low
		);
	}
}

static void
_DropOnFolder (CmdPtr cp)
{
	FreeDobj ((DropObject *)cp->clientData);
}

void
DropOnFolder (CnxtRec *cr, char *name, DropObject *dobj)
{
	if (name == NULL) {
		FreeDobj (dobj);
	}
	else {
		QueueCmd (
			"drop on folder", cr, _DropOnFolder, 0, cr->top->group,
			name, 0, (XtPointer) dobj, 0, Low
		);
	}
}

/*
 * Expand the _DROP property for this file.  If there is a _DROP
 * property set the exec string to this property and call ExecOpen()
 * after the file has been copied.  If there is no _DROP property
 * just copy the file.
 */
static void
GetFileClass (CnxtRec *cr, DtReply *reply, TargetData *t)
{
	char *		dropProp;
	DtPropList	plist;
	char *		fn;

	dropProp = DtGetProperty (
		&reply->get_fclass.plist, "_DROP", NULL
	);

	plist.count = 0;
	plist.ptr = NULL;
	DtAddProperty (&plist, "F", reply->get_fclass.file_name, NULL);
	DtAddProperty (
		&plist, "f", basename (reply->get_fclass.file_name), NULL
	);

	fn = cr->tmpdir;
	if (dropProp != NULL) {
		t->dobj->exec = (char *)DtExpandProperty (dropProp, &plist);
		/* ExecOpen() will expand the %S value and execute */
		/* the drop command. */
		t->dobj->cmd = ExecOpen;
	}
	else {
		if (strcmp (reply->get_fclass.class_name, "DIR") == 0) {
			fn = reply->get_fclass.file_name;
		}
		else {
			DisplayNormalError (cr, GGT (TXT_NO_DROPS), OL_ERROR);
			return;
		}
		t->dobj->cmd = DropOnFolder;
		t->dobj->exec = NULL;
	}
	/* Gather up all the names of the selected files and store */
	/* in the connection's DropObject */
	CopyOutside (cr, t->dobj, t->item_index, fn);
	DtFreeReply (reply);
	MYFREE (t);
}

static void
EndDropCB (Widget w, DropObject *dobj, XtPointer callData)
{
	char **	cp;

	FPRINTF ((stderr, "EndDropCB\n"));
	FreeDobj (dobj);
}

static void
DeleteDropCB (Widget w, DropObject *dobj, XtPointer callData)
{
	FPRINTF ((stderr, "DeleteDropCB\n"));
	EndDropCB (w, dobj, callData);
}

static void
_DropOnOther (CmdPtr cp)
{
	DropObject *	dobj = (DropObject *)cp->clientData;
	DtDnDSendPtr	ret;
	char **		files;
	int		i;
	int		j = -1;

	/* Don't start transaction with client until all files */
	/* have been copied. */
	if (dobj->fileindex != dobj->numfiles) {
		return;
	}

	/* Remove all NULL entries from the destination file list - this */
	/* may be neccessary because DontOverwrite may have been selected */
	/* on one or more files. */
	for (i=0; i<dobj->numfiles; i++) {
		if (dobj->destfiles[i] == NULL) {
			if (j == -1) {
				j = i;
			}
		}
		else if (j != -1) {
			if (i != j) {
				dobj->destfiles[j++] = dobj->destfiles[i];
				dobj->destfiles[i] = NULL;
			}
		}
	}

	ret = DtNewDnDTransaction (
		dobj->w,
		dobj->destfiles,
		DT_B_STATIC_LIST,
		dobj->x,
		dobj->y,
		dobj->time,
		dobj->window,
		DT_MOVE_OP,
		(XtCallbackProc)DeleteDropCB,
		(XtCallbackProc)EndDropCB,
		(XtPointer)dobj
	);
	if (ret == (DtDnDSendPtr)0) {
		FPRINTF ((stderr, "DtNewDnDTransaction failed\n"));
	}
}

/*
 * The file just copied was dropped outside and outside of dtm.  Therefor,
 * a drop transaction must be initiated with that client.
 */
static void
DropOnOther (CnxtRec *cr, char *name, DropObject *dobj)
{
	/* The name can be NULL because the last few files copies */
	/* could have been canceled (No Overwrite).  It is neccessary */
	/* to call dobj->cmd for closure. */
	if (name == NULL) {
		/* Indicate that this file wasn't copied */
		MYFREE (dobj->destfiles[dobj->fileindex-1]);
		dobj->destfiles[dobj->fileindex-1] = NULL;
	}
	QueueCmd (
		"start drop", cr, _DropOnOther, 0,
		cr->top->group, name, 0, dobj, 0, Low
	);
}

static void
GetTargetFile (CnxtRec *cr, DtReply *reply, TargetData *t)
{

	CnxtRec *		copycr = cr->copycr;
	Widget			shell = GetBaseWindowShell (cr->base);

	/* If the return status is -1 then the drop occurred outside */
	/* dtm.  This means that we need to copy the file to a temporary */
	/* location and then notify the process that was dropped on. */
	if (reply->header.status == -1) {
		FPRINTF ((
			stderr, "FILENAME = %s\n", reply->get_fname.file_name
		));
		/* Gather up all the names of the selected files and store */
		/* in the connection's DropObject */
		t->dobj->cmd = DropOnOther;
		t->dobj->exec = NULL;
		CopyOutside (cr, t->dobj, t->item_index, cr->tmpdir);
		MYFREE (t);
	}
	else {
		/* Ask dtm for to class this file */
		QueueGetFileClassRequest (
			shell, GetFileClass,
			STRDUP (reply->get_fname.file_name), DT_NO_FILETYPE, t
		);
	}
	DtFreeReply (reply);
}
void
MarkOld (CnxtRec *cr)
{
	int	i;
	Widget	shell = GetBaseWindowShell (cr->base);

	for (i=0; i<MAX_ATOMS; i++) {
		if (Shell[i] == shell) {
			StaleRequest[i] = True;
		}
	}
}

Atom
NextAtom (Widget shell, PFV cb, XtPointer clientData)
{
	int	i;

	for (i=0; i<MAX_ATOMS; i++) {
		if (Atoms[i] == 0) {
			Atoms[i] = OlDnDAllocTransientAtom (shell);
			Callbacks[i] = cb;
			ClientData[i] = clientData;
			Shell[i] = shell;
			/* Indicate the request is still valid */
			StaleRequest[i] = False;
			return Atoms[i];
		}
	}
	return (Atom)0;
}

void
EventHandler (Widget w, CnxtRec *cr, XEvent *xevent, Boolean cont)
{
	int	i;
	DtReply	reply;
	int	ret;
	Widget	shell = GetBaseWindowShell (cr->base);
	static unsigned long	serial = 0;

	if (xevent->type == ConfigureNotify) {
		if (serial == xevent->xconfigure.serial) {
			return;
		}
		serial = xevent->xconfigure.serial;
		if (cr->dirInProgress == False) {
			/* Go restructure the icon box. */
			FPRINTF ((stderr, "ResizeEvent\n"));
			SortItems (cr);
			return;
		}
	}
	if (xevent->type != SelectionNotify) {
		return;
	}
	for (i=0; i<MAX_ATOMS; i++) {
		if (xevent->xselection.selection == Atoms[i]) {
			memset(&reply, 0, sizeof(reply));
			ret = DtAcceptReply (
				XtScreen(shell), Atoms[i], XtWindow(shell),
				&reply
			);
			if (StaleRequest[i] == False) {
				(Callbacks[i])(cr, &reply, ClientData[i]);
			}
			else {
				FPRINTF ((stderr, "stale request\n"));
			}
			OlDnDFreeTransientAtom (shell, Atoms[i]);
			Atoms[i] = (Atom)0;
		}
	}
}

static void
DroppedOutsideDtftp (
	CnxtRec *cr, DropObject *dobj, OlFlatDropCallData *p, int item_index
)
{
	static DtRequest	request;
	long			serial;
	Widget			shell = GetBaseWindowShell (cr->base);
	Atom			atom;
	int			rootx;
	int			rooty;
	Window			dummyWin;
	TargetData *		t;
	
	t = (TargetData *)CALLOC (1, sizeof(TargetData));

	/* Save the drop data so it can be used later to move the file */

	XTranslateCoordinates(
		XtDisplay (shell), RootWindowOfScreen(XtScreen(shell)),
		dobj->window, dobj->x, dobj->y, &rootx, &rooty, &dummyWin
	);
	request.header.rqtype = DT_GET_FILE_NAME;
	request.get_fname.win_id = dobj->window;
	request.get_fname.icon_x = rootx;
	request.get_fname.icon_y = rooty;

	t->dobj = dobj;
	t->p = p;
	t->item_index = item_index;
	atom = NextAtom (shell, GetTargetFile, t);
	serial = DtEnqueueRequest(
		XtScreen(shell), _DT_QUEUE(XtDisplay(shell)),
		atom, XtWindow(shell), (DtRequest *)&request
	);
	FPRINTF ((stderr, "serial = %d\n", serial));
}

static DropObject *
FillDobj (CnxtRec *cr, OlFlatDropCallData *p, Widget w)
{
	DropObject *	dobj;

	dobj = (DropObject *)CALLOC (1, sizeof (DropObject));
	if (dobj == NULL) {
		return NULL;
	}
	dobj->x = p->root_info->root_x;
	dobj->y = p->root_info->root_y;
	dobj->time = p->ve->xevent->xbutton.time;
	dobj->window = p->dst_info->window;
	dobj->w = w;
	return dobj;
}

void
DropProcCB (Widget wid, XtPointer client_data, OlFlatDropCallData *call_data)
{
	OlFlatDropCallData *	p = call_data;
	Widget			widget;
	CnxtRec *		cr = FindCnxtRec (wid);
	DropObject *		dobj;
	DmItemPtr		item = cr->dir.itp + p->item_data.item_index;
	int			i;

	widget = XtWindowToWidget (XtDisplay (Root), (p->dst_info)->window);

	FPRINTF ((stderr, "Got drop\n"));
	if (p->drop_status != OlDnDDropSucceeded) {
		FPRINTF ((stderr, "Drop cancelled or unregistered\n"));
		return;
	}

	/* Make sure this isn't a link operation */
	if (p->ve->virtual_name == OL_LINK) {
		DisplayNormalError (cr, GGT (TXT_NO_LINKS), OL_ERROR);
		return;
	}

	cr = FindCnxtRec (wid);

	if (widget == (Widget)0) {
		dobj = FillDobj (cr, p, wid);
		if (dobj == (DropObject *)0) {
			return;
		}
		i = p->item_data.item_index;
		/* If this item is selected then all selected items */
		/* should be copied.  Otherwise, only this item gets */
		/* copied. */
		if (ITEM_SELECT(item)) {
			i = -1;
		}
		DroppedOutsideDtftp (cr, dobj, p, i);
		return;
	}
	else {
		DisplayNormalError (cr, GGT (TXT_NO_REMOTE_REMOTE), OL_ERROR);
	}
}

FileType
SetType (char *line)
{
	switch (line[0]) {
		case 'd': {
			return DM_FTYPE_DIR;
		}
		case 'b': {
			return DM_FTYPE_BLK;
		}
		case 'c': {
			return DM_FTYPE_CHR;
		}
		case 'm': {
			return DM_FTYPE_SHD;
		}
		case 'p': {
			return DM_FTYPE_FIFO;
		}
		case 's': {
			return DM_FTYPE_SEM;
		}
		case '-': {
			if (
				line[3] == '-' &&
				line[6] == '-' &&
				line[9] == '-'
			) {
				return DM_FTYPE_DATA;
			}
			else {
				return DM_FTYPE_EXEC;
			}
		}
		default: {
			return DM_FTYPE_UNK;
		}
	}
}

char *
GetFileName (char *name, DirEntry *dp, DmFileType *ftype)
{
	char *		cp;
	char *		p;
	char *		p1;
	char		buf[BUF_SIZE];

	cp = strtok (name, " ");
	dp->permission = STRDUP (cp);
	cp = strtok (NULL, " ");
	dp->links = atoi (cp);
	cp = strtok (NULL, " ");
	dp->owner = STRDUP (cp);
	cp = strtok (NULL, " ");
	dp->group = STRDUP (cp);
	p = strtok (NULL, " ");
	if (name[0] == 'b' || name[0] == 'c') {
		/* Device information can have two possible forms: */
		/*     35,147536 */
		/*     39,  0    */
		if (p[strlen(p)-1] == ',') {
			cp = strtok (NULL, " ");	/* Skip over next # */
		}
	}
	else {
		dp->size = atoi (p);
	}
	cp = strtok (NULL, " ");
	p = strtok (NULL, " ");
	p1 = strtok (NULL, " ");
	sprintf (buf, "%s %s %s", cp, p, p1);
	dp->date = STRDUP (buf);
	cp = strtok (NULL, " ");

	/* Set the appropriate file type for this file */
	*ftype = SetType (dp->permission);

	return cp;
}

/* Callback from GetDirTable when parsing dir command */
/* Get all file names in temp file and stick in cp->clientData */
void
GatherNames (CnxtRec *cr)
{
	/* This routine deals only with the copy (copycr) connection */
	FILE *		fp;
	char		line[BUF_SIZE];
	char		buf[BUF_SIZE];
	char		dir[BUF_SIZE];
	DirEntry	dp;
	char *		name;
	DropObject *	dobj = (DropObject *)cr->top->clientData;
	int		i;
	int		numItems = 0;
	char *		cp;
	DmFileType	ftype;

	/*
	 * First, look into the tmpfile for the directory listing.
	 * For each line in there (except first line) make a directory
	 * entry.
	 */
	if ((fp = fopen (cr->tmpfile, "r")) == NULL) {
		DisplayError (cr, "Can't list directory", NULL);
		return;
	}
	while (fgets (line, BUF_SIZE, fp) != NULL) {
		if (strncmp (line, "total", 5) == 0) {
			continue;
		}
		numItems += 1;
		*(line+strlen(line)-1) = '\0';	/* Remove CR */
		if ((cp = strstr (line, " -> ")) != NULL) {
			*cp = '\0';
		}
		i = dobj->numfiles+1;
		dobj->srcfiles = (char **)REALLOC (
			dobj->srcfiles, sizeof (char *)*i
		);
		dobj->destfiles = (char **)REALLOC (
			dobj->destfiles, sizeof (char *)*(i+1)
		);
		dobj->numHash = (int *)REALLOC(dobj->numHash, sizeof(char*)*i);
		dobj->type = (char *)REALLOC (dobj->type, sizeof (char *)*i);
		name = GetFileName (line, &dp, &ftype);
		dobj->type[i-1] = dp.permission[0];
		/* Add this file to the list of files */
		CopyFileName (
			cr, ftype, dp.size, name, dobj, &dobj->numfiles,
			cr->top->srcfile, cr->top->destfile
		);
	}
}

static Boolean
ReadDir (CnxtRec *cr, DropObject *dobj, char *srcdir, char *destdir)
{
	DIR *		dirp;
	struct dirent *	d;
	char		name[BUF_SIZE];
	Boolean		result;
	int		j;
	mode_t		m;
	off_t		size;

	if ((dirp = opendir (srcdir)) == NULL) {
		return False;
	}
	while ((d = readdir (dirp)) != NULL) {
		if (strcmp (d->d_name, ".") == NULL) {
			continue;
		}
		if (strcmp (d->d_name, "..") == NULL) {
			continue;
		}
		sprintf (name, "%s/%s", srcdir, d->d_name);
		if ((size = StatFile (name, &m)) == -1) {
			continue;
		}
		j = ++dobj->numfiles;
		dobj->srcfiles = (char **)REALLOC (
			dobj->srcfiles, sizeof (char *)*j
		);
		if (dobj->srcfiles == NULL) {
			closedir (dirp);
			return False;
		}
		dobj->destfiles = (char **)REALLOC (
			dobj->destfiles, sizeof (char *)*(j+1)
		);
		if (dobj->destfiles == NULL) {
			MYFREE (dobj->srcfiles);
			closedir (dirp);
			return False;
		}
		dobj->numHash = (int *)REALLOC (
			dobj->numHash, sizeof (char *)*j
		);
		if (dobj->numHash == NULL) {
			MYFREE (dobj->srcfiles);
			MYFREE (dobj->destfiles);
			closedir (dirp);
			return False;
		}
		dobj->type = (char *)REALLOC (
			dobj->type, sizeof (char)*j
		);
		if (dobj->type == NULL) {
			MYFREE (dobj->srcfiles);
			MYFREE (dobj->destfiles);
			MYFREE (dobj->numHash);
			closedir (dirp);
			return False;
		}
		j -= 1;
		dobj->srcfiles[j] = STRDUP (name);
		if (dobj->srcfiles[j] == NULL) {
			MYFREE (dobj->srcfiles);
			MYFREE (dobj->destfiles);
			MYFREE (dobj->numHash);
			MYFREE (dobj->type);
			closedir (dirp);
			return False;
		}
		sprintf (name, "%s/%s", destdir, d->d_name);
		dobj->destfiles[j] = STRDUP (name);
		if (dobj->destfiles[j] == NULL) {
			MYFREE (dobj->srcfiles);
			MYFREE (dobj->destfiles);
			MYFREE (dobj->numHash);
			MYFREE (dobj->type);
			MYFREE (dobj->srcfiles[j]);
			closedir (dirp);
			return False;
		}
		dobj->numHash[j] = (size/1024) + 1;
		result = ReadDir (
			cr, dobj, dobj->srcfiles[j], dobj->destfiles[j]
		);
		if (result == True) {
			dobj->type[j] = 'd';
		}
		else {
			dobj->type[j] = '-';
		}
	}
	closedir (dirp);
	return True;
}

void
PopDownPopup (CmdPtr cp)
{
	BringDownPopup (GetModalGizmoShell (cp->cr->sliderPopup));
	cp->cr->sliderUp = False;
}

static void
DroppedIntoDtftp (Widget wid, int itemIndex, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);
	CnxtRec *	copycr;
	DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
	DropObject *	dobj;
	int		i;
	int		len;
	off_t		size;
	char *		cp;
	mode_t		m;
	char		destdir[BUF_SIZE];
	DmObjectPtr	op;
	DmItemPtr	item;

	if (dip->error != 0) {
		return;
	}
	if (dip->files == (char **)0) {
		return;
	}
	MapGizmo (BaseWindowGizmoClass, cr->base);
	XRaiseWindow (
		XtDisplay (GetBaseWindowShell (cr->base)),
		XtWindow (GetBaseWindowShell (cr->base))
	);
	dobj = (DropObject *)CALLOC (1, sizeof (DropObject));
	if (dobj == NULL) {
		return;
	}
	if (cr->copycr == NULL) {
		cr->copycr = CreateCopyConnection (cr);
	}
	/* Place files in directory if needed */
	if (itemIndex != -1) {
		item = cr->dir.itp + itemIndex;
		op = (DmObjectPtr)item->object_ptr;
		if (op->ftype == DM_FTYPE_DIR) {
			sprintf (destdir, "%s/%s", cr->pwd, op->name);
		}
		else {
			/* The drop site is a folder, which is an error */
			DisplayNormalError (
				cr, GGT(TXT_INVALID_DROP_SITE), OL_ERROR
			);
			return;
		}
	}
	else {
		strcpy (destdir, cr->pwd);
	}
	copycr = cr->copycr;
	QueueCmd (
		"create slider popup",
		copycr, CreateSliderPopup, 0, NextCmdGroup(),
		ftp->systemName, cr->systemAddr, 0, 0, Low
	);
	dobj->fileindex = 0;
	dobj->srcfiles = (char **)MALLOC (sizeof (char *)*dip->nitems);
	dobj->destfiles = (char **)MALLOC (sizeof (char *)*(dip->nitems+1));
	dobj->numHash = (int *)MALLOC (sizeof (int)*dip->nitems);
	dobj->type = (char *)MALLOC (sizeof (char)*dip->nitems);
	len = strlen (destdir);
	for (i=0; i<dip->nitems; i++) {
		if ((size = StatFile (dip->files[i], &m)) == -1) {
			continue;
		}
		dobj->srcfiles[i] = STRDUP (dip->files[i]);
		dobj->destfiles[i] = (char *)MALLOC (
			strlen (dip->files[i])+len+1
		);
		dobj->numHash[i] = (size/1024) + 1;
		dobj->type[i] = '-';
		if ((m & S_IFMT) == S_IFDIR) {
			dobj->type[i] = 'd';
		}
		strcpy (dobj->destfiles[i], destdir);
		strcat (dobj->destfiles[i], "/");
		strcat (dobj->destfiles[i], basename (dip->files[i]));
	}
	dobj->numfiles = i;
	dobj->cr = cr;	/* Remember connection for DirCmd */
	/* Go thru the list an read the directories */
	for (i=0; i<dip->nitems; i++) {
		if (dobj->type[i] == 'd') {
			ReadDir (
				copycr, dobj,
				dobj->srcfiles[i], dobj->destfiles[i]
			);
		}
	}
	LocalRemoteCopy (copycr, dobj, NextCmdGroup());
	QueueCmd (
		"pop down popup", copycr, PopDownPopup, 0,
		NextCmdGroup(), 0, 0, 0, 0, Scum
	);
}

Boolean
TriggerCB (
	Widget w, Window win, Position rootx, Position rooty, Atom selection,
	Time timestamp, OlDnDDropSiteID drop_site_id,
	OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
	XtPointer client_data
)
{
	int		item;
	int		x;
	int		y;
	Window		dummyWin;

	FPRINTF ((stderr, "Got trigger\n"));
	XTranslateCoordinates (
		XtDisplay (Root), RootWindowOfScreen(XtScreen(Root)), win,
		rootx, rooty, &x, &y, &dummyWin
	);
	item = OlFlatGetItemIndex (w, x, y);
	DtGetFileNames (
		w, selection, timestamp, send_done,
		DroppedIntoDtftp, (XtPointer)item
	);
}
