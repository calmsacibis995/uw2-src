/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:dir.c	1.1.4.1"
#endif

#include "ftp.h"
#include <locale.h>
#include <time.h>

extern Widget	Root;
extern void	ResetDirFlag();

static	void
ExecuteCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *		cr = FindCnxtRec (wid);
	DmObjectPtr		op;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	Arg			arg[2];

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(wid, d->item_index, arg, 1);
	OpenFile (cr, op, d->item_index);
}

void
FreeDir (CnxtRec *cr)
{
	int		i;
	DmItemPtr	itp;
	DirPtr		dirp = &(cr->dir);
	DmObjectPtr	op;
	DmObjectPtr	dop;
	DirEntry *	entryp;

	for (op=dirp->start; op!=(DmObjectPtr)0; ) {
		MYFREE (op->fcp);
		entryp = (DirEntry *)op->objectdata;
		MYFREE (entryp->permission);
		MYFREE (entryp->owner);
		MYFREE (entryp->group);
		MYFREE (entryp->date);
		MYFREE (entryp);
		dop = op->next;
		MYFREE (op);
		op = dop;
	}
	itp = cr->dir.itp; 
	if (itp != NULL) {
		for (i=0; i<cr->dir.container->num_objs; i++) {
			if (ITEM_MANAGED(itp) != False) {
				XtFree (ITEM_LABEL(itp));
			}
			itp += 1;
		}
	}
	dirp->start = (DmObjectPtr)0;
	dirp->end = (DmObjectPtr)0;
	dirp->container->num_objs = 0;
	cr->dir.itp = NULL;
	SetMultiSelectSensitivity (cr, 0);
}

static void
_ResolveLinkCmd (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	sprintf (buf, "dir %s %s\n", cp->srcfile, cp->cr->tmpfile);
	Output (cp->cr, buf);
}

/* Don't actually free anything */
static void
FreeNothing (XtPointer dummy)
{
}

extern StateTable ResolveTable[];

static void
ResolveLinkFileType (CnxtRec *cr, DmObjectPtr op, char *link)
{
	op->attrs = DM_B_SYMLINK;
	QueueCmd (
		"resolve link", cr, _ResolveLinkCmd, ResolveTable, 
		NextCmdGroup(), link, NULL, (XtPointer)op, FreeNothing, High
	);
}

static void
MenuProcCB (Widget wid, XtPointer clientData, XtPointer callData)
{
	CnxtRec *		cr = FindCnxtRec (wid);
	OlFlatCallData *	d = (OlFlatCallData *)callData;
	DmObjectPtr		op = (DmObjectPtr)clientData;
	DirEntry *		dp = (DirEntry *)op->objectdata;

	switch (d->item_index) {
		case 0: {	/* Open */
			OpenFile (cr, op, dp->itemIndex);
			break;
		}
		case 1: {	/* Property */
			DisplayProperty (cr, op);
			break;
		}
		case 2: {	/* Delete */
			DeleteFile (cr, op);
			DirCmd (cr, NextCmdGroup(), Medium);
			break;
		}
		case 3: {	/* Print */
			PrintFile (cr, op, dp->itemIndex);
			break;
		}
	}
}

static MenuItems iconItems[] = {
	{True, BUT_OPEN,	MNEM_ICON_OPEN,		NULL,	NULL},
	{True, BUT_PROPERTIES,	MNEM_ICON_PROP,		NULL,	NULL},
	{True, BUT_DELETE,	MNEM_ICON_DELETE,	NULL,	NULL},
	{True, BUT_PRINT,	MNEM_ICON_PRINT,	NULL,	NULL},
	{NULL}
};

static MenuGizmo IconMenu = {
	NULL,			/* help		*/
	"iconMenu",		/* name		*/
	"na",			/* title	*/
	iconItems,		/* menu		*/
	MenuProcCB,		/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDCOLS,		/* layoutType	*/
	1,			/* measure	*/
	0			/* default Item */
};

static void
MenuProc (Widget wid, XtPointer clientData, XtPointer callData)
{
	CnxtRec *		cr = FindCnxtRec (wid);
	OlFIconBoxButtonCD *	p = (OlFIconBoxButtonCD *)callData;
	Position		rootx;
	Position		rooty;
	DmContainerPtr		cntrp;
	DmObjectPtr		op;
	DirEntry *		dp;
	DmItemPtr		item;
	Boolean			open = False;
	Boolean			delete = False;
	Boolean			print = False;

	/*
	 * Create the menu
	 */
	FPRINTF ((stderr, "MenuProc\n"));
	if (cr->iconMenu == (MenuGizmo *)0) {
		cr->iconMenu = CopyGizmo (MenuGizmoClass, &IconMenu);
		CreateGizmo (
			GetBaseWindowShell (cr->base),
			MenuGizmoClass, cr->iconMenu, 0, 0
		);
	}
	
	/*
	 * Determine what menu items are turned on.
	 */
	cntrp = cr->dir.container;
	item = cr->dir.itp + p->item_data.item_index;
	op = (DmObjectPtr)item->object_ptr;
	dp = (DirEntry *)op->objectdata;

	switch (op->ftype) {
		case DM_FTYPE_DIR: {
			open = True;
			break;
		}
		case DM_FTYPE_FIFO:
		case DM_FTYPE_CHR:
		case DM_FTYPE_BLK:
		case DM_FTYPE_SEM:
		case DM_FTYPE_SHD:
		case DM_FTYPE_UNK:
		case DM_FTYPE_EXEC: {
			delete = True;
			break;
		}
		case DM_FTYPE_DATA: {
			delete = True;
			print = True;
			open = True;
			break;
		}
	}

	OlVaFlatSetValues (
		GetMenu(cr->iconMenu), 0,
		XtNsensitive,	open,
		XtNclientData,	op,
		(String)0
	);
	OlVaFlatSetValues (
		GetMenu(cr->iconMenu), 1,
		XtNclientData,	op,
		(String)0
	);
	OlVaFlatSetValues (
		GetMenu(cr->iconMenu), 2,
		XtNsensitive,	delete,
		XtNclientData,	op,
		(String)0
	);
	OlVaFlatSetValues (
		GetMenu(cr->iconMenu), 3,
		XtNsensitive,	print,
		XtNclientData,	op,
		(String)0
	);
	XtVaSetValues (
		GetMenu(cr->iconMenu),
		XtNitemsTouched,True,
		XtNclientData,	op,
		(String)0
	);

	XtTranslateCoords(wid, p->x, p->y, &rootx, &rooty);
	OlPostPopupMenu (
		wid, cr->iconMenu->parent, p->reason, NULL,
		rootx, rooty, p->x, p->y
	);
}

static void
CreateDragCursor (Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFlatDragCursorCallData *	cursorData =
		(OlFlatDragCursorCallData *) call_data;
	DmGlyphPtr			glyph;
	DmObjectPtr			op;
	XColor				junk;
	Arg				arg[10];
	static unsigned int		xHot;
	static unsigned int		yHot;
	static XColor			white;
	static XColor			black;
	static Cursor			cursor;
	static Boolean			first = True;

	if (first == True) {
		first = False;
		XAllocNamedColor (
			XtDisplay (Root),
			DefaultColormapOfScreen (XtScreen(Root)),
			"white", &white, &junk
		);
		XAllocNamedColor (
			XtDisplay (Root),
			DefaultColormapOfScreen (XtScreen(Root)),
			"black", &black, &junk
		);
	}
	XtSetArg (arg[0], XtNobjectData, &op);
	OlFlatGetValues (w, cursorData->item_data.item_index, arg, 1);

	glyph =  op->fcp->cursor;
	if (glyph) {
		xHot = glyph->width / 2;
		yHot = glyph->height / 2;
		cursor = XCreatePixmapCursor (
			XtDisplay (Root), glyph->pix, glyph->mask,
			&black, &white, xHot, yHot
		);
	}
	else {
		cursor = None;
		xHot = yHot = 0;
	}

	cursorData->yes_cursor = cursor;
	cursorData->x_hot = xHot;
	cursorData->y_hot = yHot;
}

char *
GetShortName (DmItemPtr item)
{
	DmObjectPtr	op = (DmObjectPtr)item->object_ptr;

	return op->name;
}

char *
GetLongName (DmItemPtr item, int len)
{
	static char	buf[BUF_SIZE];
	DmObjectPtr	op = (DmObjectPtr)item->object_ptr;
	DirEntry *	dp = (DirEntry *)op->objectdata;

	buf[0] = '\0';

	sprintf(
		buf, "%-*s %-10s%-10s%-10s%7ld %s", len, op->name,
		dp->permission+1, dp->owner, dp->group, dp->size, dp->date
	);

	return buf;
}

static void
SetVerticalScroll (CnxtRec *cr, Widget w)
{
	int			height;
	extern Dimension	LongRowHeight (Widget w, OlFontList *fontList);
	extern Dimension	NameRowHeight (Widget w, OlFontList *fontList);

	if (cr->format == DM_ICONIC) {
		height = ftp->gridHeight/2;
	}
	else if (cr->format == DM_LONG) {
		height = LongRowHeight (w, ftp->fontList);
	}
	else {
		height = NameRowHeight (w, ftp->fontList);
	}
	XtVaSetValues (w, XtNvStepSize, height, (String)0);
}

void
FormatContainer (CnxtRec *cr)
{
	Arg			args[10];
	Widget			w;
	int			i = 0;
	int			width;
	PFV			func;

	FPRINTF ((stderr, "FormatContainer(s): dirInProgress = %d\n", cr->dirInProgress));
	w = QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "upperSW"
	);
	XtVaGetValues (w, XtNwidth, &width, (String)0);
	SetVerticalScroll (cr, w);
	ComputeLayout (
		cr->dir.cw, cr->dir.itp, cr->dir.container->num_objs,
		cr->format, width, DM_B_NO_INIT | DM_B_CALC_SIZE, 
		cr->layoutOptions,
		ftp->gridWidth, ftp->gridHeight, ftp->fontList,
		GetShortName, GetLongName
	);

	/* Change the routine for drawing the icons */
	func = DrawLinkIcon;
	if (cr->format == DM_NAME) {
		func = DrawNameIcon;
	}
	else if (cr->format == DM_LONG) {
		func = DrawLongIcon;
	}
	XtSetArg (args[i], XtNdrawProc, func); i++;
	XtSetArg (args[i], XtNitems, cr->dir.itp); i++;
	XtSetArg (args[i], XtNnumItems, cr->dir.container->num_objs); i++;
	XtSetArg (args[i], XtNitemsTouched, True); i++;

	XtSetValues (cr->dir.cw, args, i);
	ResetDirFlag (cr);
	FPRINTF ((stderr, "FormatContainer(e): dirInProgress = %d\n", cr->dirInProgress));
}

typedef struct _TempData {
	DmObjectPtr	op;
	PFV		func;
} TempData;

static void
GetIcon (CnxtRec *cr, DtReply *reply, TempData *data)
{
	DmObjectPtr	op = data->op;
	DirPtr		dirp = &(cr->dir);
	DmObjectPtr	dop;
	DtPropList	plist;
	char *		value;
	char *		iconProp;

	plist.ptr = 0;
	plist.count = 0;
	DtAddProperty (&plist, "F", reply->get_fclass.file_name, NULL);
	DtAddProperty (
		&plist, "f",
		(char *)basename(reply->get_fclass.file_name), NULL
	);
	iconProp = DtGetProperty (&reply->get_fclass.plist, "_ICONFILE", NULL);
	if (iconProp == NULL) {
		iconProp = DtGetProperty (
			&reply->get_fclass.plist, "_DFLTICONFILE", NULL
		);
	}
	if (iconProp != NULL) {
		value = (char *)DtExpandProperty (iconProp, &plist);
		op->fcp->glyph = DmGetPixmap (XtScreen(Root), value);
		op->fcp->cursor = DmGetCursor (XtScreen(Root), value);
	}

	/* Process next icon */
	op = data->op = op->next;
	if (op != NULL) {
		QueueGetFileClassRequest (
			GetBaseWindowShell (cr->base),
			GetIcon, op->name, op->ftype, (XtPointer)data
		);
	}
	else {
		/* FormatContainer of CreateContainer */
		(data->func) (cr);
		MYFREE (data);
	}
	DtFreeReply (reply);
}

static void
GetIcons (CnxtRec *cr, PFV func)
{
	TempData *	data;
	DmObjectPtr	op;
	DirPtr		dirp = &(cr->dir);

	FPRINTF ((stderr, "GetIcons(s): dirInProgress = %d\n", cr->dirInProgress));
	op = dirp->start;
	if (cr->format == DM_ICONIC && cr->hardCoded == True) {
		if (op != NULL) {
			data = (TempData *)CALLOC (1, sizeof (TempData));
			cr->hardCoded = False;
			data->op = op;
			data->func = func;
			QueueGetFileClassRequest (
				GetBaseWindowShell (cr->base),
				GetIcon, op->name, op->ftype, (XtPointer)data
			);
			return;
		}
	}
	(func)(cr);
	FPRINTF ((stderr, "GetIcons(e): dirInProgress = %d\n", cr->dirInProgress));
}

static void
_FormatWait (CmdPtr cp)
{
	CnxtRec *	cr = cp->cr;

	if (cr->dirInProgress == True) {
		QueueCmd (
			"format wait", cr, _FormatWait, NULL, NextCmdGroup(),
			0, 0, (XtPointer)cp->clientData, FreeNothing, Medium
		);
		return;
	}
	if (cr->format == DM_LONG) {
		cr->layoutOptions = UPDATE_LABEL;
	}
	else {
		cr->layoutOptions = 0;
	}
	cr->format = (DmViewFormatType)cp->clientData;
	/* Don't format this if there is no icon box */
	if (cr->dir.container->num_objs == 0) {
		return;
	}
	cr->dirInProgress = True;

	GetIcons (cr, FormatContainer);
}

void
FormatCB (Widget wid, XtPointer clientData, XtPointer callData)
{
	CnxtRec *		cr = FindCnxtRec (wid);
	OlFlatCallData *	p = (OlFlatCallData *)callData;

	if (p->item_index == cr->format) {
		return;
	}

	/* Go format the icon box, but wait for any other formating */
	/* or icon box creating */
	QueueCmd (
		"format wait", cr, _FormatWait, NULL, NextCmdGroup(),
		0, 0, (XtPointer)p->item_index, FreeNothing, Medium
	);
}

static void
RenumberItems (CnxtRec *cr)
{
	DmObjectPtr	op;
	DirEntry *	dp;
	DmContainerPtr	cntrp = cr->dir.container;
	DmItemPtr	item;
	int		i;

	item = cr->dir.itp;
	for (i=0; i<cntrp->num_objs; item++, i++) {
		op = (DmObjectPtr)item->object_ptr;
		dp = (DirEntry *)op->objectdata;
		dp->itemIndex = i;
	}
}

static int
SortByType (DmItemPtr n1, DmItemPtr n2)
{
	DmObjectPtr	op1 = (DmObjectPtr)n1->object_ptr;
	DmObjectPtr	op2 = (DmObjectPtr)n2->object_ptr;

	return (int)op1->ftype - (int)op2->ftype;
}

static int
SortByName (DmItemPtr n1, DmItemPtr n2)
{

	DmObjectPtr	op1 = (DmObjectPtr)n1->object_ptr;
	DmObjectPtr	op2 = (DmObjectPtr)n2->object_ptr;

	return strcmp((char *)(op1->name), (char *)(op2->name));
}

static int
SortBySize (DmItemPtr n1, DmItemPtr n2)
{
	DmObjectPtr	op1 = (DmObjectPtr)n1->object_ptr;
	DmObjectPtr	op2 = (DmObjectPtr)n2->object_ptr;
	DirEntry *	dirp1 = op1->objectdata;
	DirEntry *	dirp2 = op2->objectdata;

	return dirp1->size - dirp2->size;
}

static char * months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static long
ConvertDate (char *date)
{
	time_t		t;
	long		d;
	char *		cp;
	int		i;
	static int	year = 0;
	char *		f2;
	char *		f3;

	/*
	 * Dates can have two formats:
	 *	Jun 23 16:44
	 *	Jan 1 1970
	 */

	f2 = strchr (date, ' ')+1;	/* Point to second field */
	f3 = strchr (f2, ' ')+1;	/* Point to third field */
	if (year == 0) {
		t = time (0);
		cp = ctime (&t);
		year = atoi (cp+22);
	}

	if ((cp = strchr (f3, ':')) != NULL) {
		d = atoi (cp+1);
		d += atoi (f3)*64;
		d += year*64*32*32*16;
	}
	else {
		/* Convert the 93 of 1993 in field 3 */
		d = atoi (f3+2)*64*32*32*16;
	}
	/* Convert the day of the month */
	d += atoi (f2)*32*64;
	for (i=0; i<12; i++) {
		if (strncmp (months[i], date, 3) == 0) {
			d += i*32*32*64;
			break;
		}
	}
	return d;
}

static int
SortByTime (DmItemPtr n1, DmItemPtr n2)
{
	DmObjectPtr	op1 = (DmObjectPtr)n1->object_ptr;
	DmObjectPtr	op2 = (DmObjectPtr)n2->object_ptr;
	DirEntry *	dirp1 = op1->objectdata;
	DirEntry *	dirp2 = op2->objectdata;

	return ConvertDate (dirp2->date) - ConvertDate (dirp1->date);
}

static int
I18NByName (DmItemPtr n1, DmItemPtr n2)
{
}

PFI CompareFunctions[] = {SortByType, SortByName, SortBySize, SortByTime};

void
SecondaryNameSort (CnxtRec *cr)
{
	DmObjectPtr	op1;
	DmObjectPtr	op2;

	DmItemPtr	n1;
	DmItemPtr	n2;
	int		i;

	for (i=0; i<cr->dir.container->num_objs; i++) {
		n1 = &cr->dir.itp[i];
		n2 = n1;
		op1 = (DmObjectPtr)n1->object_ptr;
		for (i=i+1; i<cr->dir.container->num_objs; i++) {
			n2 = &cr->dir.itp[i];
			op2 = (DmObjectPtr)n2->object_ptr;

			if (op1->ftype != op2->ftype) {
				qsort (
					n1, n2-n1, sizeof(DmItemRec),
					SortByName
				);
				break;
			}
		}
		i = i-1;
	}
	qsort (n1, n2+1-n1, sizeof(DmItemRec), SortByName);
}

void
SortItems (CnxtRec *cr)
{
	Arg		args[10];
	int		i = 0;
	Widget		w;
	Dimension	width;

	/* Don't sort this if there is no icon box */
	if (cr->dir.cw == (Widget)0) {
		return;
	}
	/* Don't sort this if there are no items in the container */
	if (cr->dir.container->num_objs == 0) {
		return;
	}
	if (cr->sort == ByName && strcmp(setlocale(LC_COLLATE, NULL), "C")) {
		qsort (
			cr->dir.itp, cr->dir.container->num_objs,
			sizeof(DmItemRec), (int (*)())I18NByName
		);
	}
	else {
		qsort (
			cr->dir.itp, cr->dir.container->num_objs,
			sizeof(DmItemRec), CompareFunctions[cr->sort]
		);
		if (cr->sort == ByType) {
			SecondaryNameSort (cr);
		}
	}
	w = QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "upperSW"
	);
	XtVaGetValues (w, XtNwidth, &width, (String)0);
	SetVerticalScroll (cr, w);
	ComputeLayout (
		cr->dir.cw, cr->dir.itp, cr->dir.container->num_objs,
		cr->format, width, DM_B_NO_INIT | DM_B_CALC_SIZE, 0,
		ftp->gridWidth, ftp->gridHeight, ftp->fontList,
		GetShortName, GetLongName
	);
	RenumberItems (cr);	/* Reset itemIndex value */
	XtSetArg (args[i], XtNitems, cr->dir.itp); i++;
	XtSetArg (args[i], XtNnumItems, cr->dir.container->num_objs); i++;
	XtSetArg (args[i], XtNitemsTouched, True); i++;

	XtSetValues (cr->dir.cw, args, i);
	(void)SetBaseWindowFooter (cr, -1);
}

static void
CreateContainer (CnxtRec *cr)
{
	int		i;
	DirPtr		dirp = &(cr->dir);
	Arg		arg[20];
	Widget		w;
	PFV		func;

	FPRINTF ((stderr, "CreateContainer(s): dirInProgress = %d\n", cr->dirInProgress));
	/* Get the upper scrolling window */
	w = QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "upperSW"
	);
	/*
	 * Next, create the icon box containing the listing.
	 */
	func = DrawLinkIcon;
	if (cr->format == DM_NAME) {
		func = DrawNameIcon;
	}
	else if (cr->format == DM_LONG) {
		func = DrawLongIcon;
	}
	i = 0;
	XtSetArg(arg[i], XtNmovableIcons,	(XtArgVal)False); i++;
	XtSetArg(arg[i], XtNminWidth,		(XtArgVal)1); i++;
	XtSetArg(arg[i], XtNminHeight,		(XtArgVal)1); i++;
	XtSetArg(arg[i], XtNdrawProc,		(XtArgVal)func); i++;
	XtSetArg(arg[i], XtNdblSelectProc,	(XtArgVal)ExecuteCB); i++;
	XtSetArg(arg[i], XtNdropProc,		(XtArgVal)DropProcCB); i++;
	XtSetArg(arg[i], XtNtriggerMsgProc,	(XtArgVal)TriggerCB); i++;
	XtSetArg(arg[i], XtNpostSelectProc,	(XtArgVal)SelectCB); i++;
	XtSetArg(arg[i], XtNpostAdjustProc,	(XtArgVal)SelectCB); i++;
	XtSetArg(arg[i], XtNmenuProc,		(XtArgVal)MenuProc); i++;
	XtSetArg(arg[i], XtNfont,		(XtArgVal)GetDefaultFont (w));
	i++;
	XtSetArg(arg[i], XtNdragCursorProc,	(XtArgVal)CreateDragCursor);
	i++;

	dirp->cw = DmCreateIconContainer(
		w, DM_B_NO_INIT | DM_B_CALC_SIZE,
		arg, i, dirp->start, dirp->container->num_objs,
		&dirp->itp, dirp->container->num_objs, NULL, NULL,
		(XFontStruct *)GetDefaultFont (w), 1
	);
	BaseWindowTitle (cr);
	SortItems (cr);
	/* If the copy file window is up, go and update it. */
	UpdateCopyPopup (cr);
	SetMultiSelectSensitivity (cr, 0);
	ResetDirFlag (cr);
	FPRINTF ((stderr, "CreateContainer(e): dirInProgress = %d\n", cr->dirInProgress));
}

void
QueueGetFileClassRequest (
	Widget shell, PFV func, char *name, FileType type, XtPointer t
)
{
	static DtRequest	request;
	Atom			atom;
	long			serial;

	memset (&request, 0, sizeof (DtRequest));
	request.header.rqtype = DT_GET_FILE_CLASS;
	request.get_fclass.file_type = type;
	request.get_fclass.options = DT_GET_PROPERTIES;
	request.get_fclass.plist.ptr = NULL;
	request.get_fclass.plist.count = 0;
	request.get_fclass.file_name = name;

	atom = NextAtom (shell, func, t);
	serial = DtEnqueueRequest(
		XtScreen(shell), _DT_QUEUE(XtDisplay(shell)),
		atom, XtWindow(shell), (DtRequest *)&request
	);
}

/*
 * Read the temporary file created by the ResolveTable state table.
 * If there is only one file listing in this file then the line should
 * be examined to determine if the file is a link and if it is then
 * ResolveLinkFileType() should be called again.  If the file contains
 * a line starting with "total" then the file is a directory.
 */
void
Traverse (CnxtRec *cr)
{
	DmObjectPtr	op = (DmObjectPtr)cr->top->clientData;
	DirEntry *	dp = (DirEntry *)op->objectdata;
	FILE *		fp;
	char		line[BUF_SIZE];
	char *		cp;

	if ((fp = fopen (cr->tmpfile, "r")) == NULL) {
		return;
	}
	fgets (line, BUF_SIZE, fp);
	if (strncmp (line, "total", 5) == 0) {
		/* This is a directory */
		dp->permission[0] = 'd';
		op->ftype = DM_FTYPE_DIR;
	}
	else {
		*(line+strlen(line)-1) = '\0';	/* Remove CR */
		if ((cp = strstr (line, " -> ")) != NULL) {
			/* Follow another link */
			ResolveLinkFileType (cr, op, cp+4);
			return;
		}
		else if (strstr (line, "Cannot access") != NULL) {
			/* For some reason the file couldn't be accessed */
			/* so just leave the file type as unk. */
			cr->unresolvedLinks -= 1;
			if (cr->unresolvedLinks == 0) {
					GetIcons (cr, CreateContainer);
			}
			return;
		}
		/* This is a normal file */
		dp->permission[0] = line[0];
		op->ftype = SetType (line);
	}
	cr->unresolvedLinks -= 1;
	op->fcp->glyph = ftp->icons[op->ftype]->glyph;
	op->fcp->cursor = ftp->icons[op->ftype]->cursor;
	if (cr->unresolvedLinks == 0) {
		GetIcons (cr, CreateContainer);
	}
}

static void
AddToDirList (CnxtRec *cr, char *line, int itemIndex)
{
	DirPtr		dirp = &(cr->dir);
	DmObjectPtr	op;
	char *		cp;
	char *		name;
	DirEntry *	dp;

	op = (DmObjectPtr) CALLOC (1, sizeof (DmObjectRec));
	op->next = (DmObjectPtr)0;
	*(line+strlen(line)-1) = '\0';	/* Remove CR */
	if ((cp = strstr (line, " -> ")) != NULL) {
		*cp = '\0';
	}
	op->container = dirp->container;
	dp = (DirEntry *)MALLOC (sizeof (DirEntry));
	dp->itemIndex = itemIndex;
	name = GetFileName (line, dp, &op->ftype);
	op->name = STRDUP (name);
	if (dp->permission[0] == 'l') {
		/* This is a link to another file.  Go */
		/* figure out what type that file is. */
		ResolveLinkFileType (cr, op, cp+4);
		cr->unresolvedLinks += 1;
	}
	op->objectdata = (XtPointer)dp;
	op->fcp = (DmFclassPtr)CALLOC(1,sizeof(DmFclassRec));
	op->fcp->glyph = ftp->icons[op->ftype]->glyph;
	op->fcp->cursor = ftp->icons[op->ftype]->cursor;

	if (dirp->start == (DmObjectPtr)0) {
		dirp->start = op;
		dirp->container->op = dirp->start;
		dirp->end = dirp->start;
	}
	else {
		dirp->end->next = op;
		dirp->end = dirp->end->next;
	}
	dirp->container->num_objs += 1;
}

void
SelectCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	/* If the copy file window is up, go and update it. */
	UpdateCopyPopup (cr);
	/* Set sensitivity of Open and rename buttons */
	SetMultiSelectSensitivity (cr, -1);
}

void
DisplayDir (CnxtRec *cr)
{
	int			i;
	Boolean			first = True;
	char			line[BUF_SIZE];
	FILE *			fp;

	/*
	 * First, look into the tmpfile for the directory listing.
	 * For each line in there (except first line) make a directory
	 * entry.
	 */
	if ((fp = fopen (cr->tmpfile, "r")) == NULL) {
		DisplayError (cr, "Can't list directory", NULL);
		return;
	}
	i = 0;
	while (fgets (line, BUF_SIZE, fp) != NULL) {
		if (strncmp ("UX:ls: ERROR:", line, 13) == 0) {
			DisplayError (cr, "Can't list directory", NULL);
			return;
		}
		if (first == True) {
			first = False;
			FreeDir (cr);
			if (strncmp ("total", line, 5) == 0) {
				continue;
			}
		}
		AddToDirList (cr, line, i++);
	}
	fclose (fp);
	if (cr->unresolvedLinks == 0) {
		GetIcons (cr, CreateContainer);
	}
	(void)SetBaseWindowFooter (cr, -1);
}

static void
_DirCmd (CmdPtr cp)
{
	cp->cr->hardCoded = True;
	Output (cp->cr, "pwd\n");
}

void
ResetDirFlag (CnxtRec *cr)
{
	cr->dirInProgress = False;
}

void
OutputDir (CnxtRec *cr)
{
	char		buf[BUF_SIZE];

	sprintf (buf, "dir %s %s\n", cr->pwd, cr->tmpfile);
	Output (cr, buf);
}

/* Destroy the flat icon container associated with this connection */
static void
DestroyContainer (CnxtRec *cr)
{
	DirPtr		dirp = &(cr->dir);

	if (dirp != NULL && dirp->cw != (Widget)0) {
		XtDestroyWidget (dirp->cw);
		dirp->cw = (Widget)0;
		SetMultiSelectSensitivity (cr, 0);
	}
}

extern StateTable DirTable[];

void static
_WaitDirCmd (CmdPtr cp)
{
	CnxtRec *	cr = cp->cr;
	int		pri = cp->pri;

	if (cr->dirInProgress == True) {
		DirCmd (cr, cp->group, cp->pri);
	}
	else {
		cr->dirInProgress = True;
		/* Clear the container window */
		DestroyContainer (cr);
		QueueCmd (
			"dir", cr, _DirCmd, DirTable, cp->group,
			0, 0, 0, 0, pri
		);
	}
}

void
DirCmd (CnxtRec *cr, int grp, Priority pri)
{
	/* Clear the container window */
	if (cr->dirInProgress == False) {
		DestroyContainer (cr);
	}
	/* Queue up a command that will continue to queue itself */
	/* until any previous DirCmd has finished. */
	QueueCmd (
		"wait on other DirCmds",
		cr, _WaitDirCmd, NULL, grp, 0, 0, 0, 0, pri
	);
}

/* This command is only used by the CdTable when the cd command is */
/* successful.  If the cd command fails then a reconnect takes place */
/* and a dir is done by the reconnection. */
void
DoDirCmd (CnxtRec *cr)
{
	DirCmd (cr, cr->top->group, cr->top->pri);
}

void
SortCB (Widget wid, XtPointer clientData, XtPointer callData)
{
	CnxtRec *		cr = FindCnxtRec (wid);
	OlFlatCallData *	p = (OlFlatCallData *)callData;

	cr->sort = p->item_index;
	SortItems (cr);
}
