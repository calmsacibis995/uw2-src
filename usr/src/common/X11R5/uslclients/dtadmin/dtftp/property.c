/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtftp:property.c	1.1.1.1"
#endif

#include "ftp.h"
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/NumericGiz.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/InputGizmo.h>
#include <Xol/StaticText.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

static void
SetPermission (PropertyList *pl, char * name, char *perm)
{
	int	i;
	Widget	w;
	Boolean	set;

	w = (Widget) QueryGizmo (
		PopupGizmoClass, pl->popup, GetGizmoWidget, name
	);
	for (i=0; i<3; i++) {
		set = True;
		if (perm[i] == '-') {
			set = False;
		}
		OlVaFlatSetValues (w, i, XtNset, set, (String)0);
	}
	XtVaSetValues (w, XtNsensitive, False, (String)0);
}

static void
ClientDestroyCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	FreeGizmo (PopupGizmoClass, (Gizmo)client_data);
}

static void
RemoveProperty (CnxtRec *cr, PropertyList *pl)
{
	PropertyList *	last = (PropertyList *)0;
	PropertyList *	p;

	/* Remove this property from the list of properties */
	for (p=cr->propertyList; p!=(PropertyList *)0; p=p->next) {
		if (p == pl) {
			break;
		}
		last = p;
	}
	if (last == 0) {
		cr->propertyList = pl->next;
	}
	else {
		last->next = pl->next;
	}
	XtDestroyWidget (GetPopupGizmoShell (pl->popup));
	/*
	FreeGizmo (PopupGizmoClass, pl->popup);
	*/
	MYFREE (pl->name);
	MYFREE (pl);
}

static void
OkCB (Widget w, PropertyList *pl, XtPointer callData)
{
	CnxtRec *	cr = FindCnxtRec (w);

	XtPopdown ((Widget) _OlGetShellOfWidget (w));
	RemoveProperty (cr, pl);
}

void
CenterLabel (Gizmo g, GizmoClass gizmoClass, char *name)
{
	Widget	w;

	w = (Widget) QueryGizmo (
		gizmoClass, g, GetGizmoWidget, name
	);
	XtVaSetValues (w, XtNalignment, OL_CENTER, (String)0);
}

static StaticTextGizmo propName = {
	NULL, "propName", NULL, NorthWestGravity, NULL
};
static StaticTextGizmo propOwner = {
	NULL, "propOwner", NULL, NorthWestGravity, NULL
};
static StaticTextGizmo propGroup = {
	NULL, "propGroup", NULL, NorthWestGravity, NULL
};
static StaticTextGizmo propModTime = {
	NULL, "propModTime", NULL, NorthWestGravity, NULL
};
static MenuItems wreItems[] = {
	{True,	BUT_READ},
	{True,	BUT_WRITE},
	{True,	BUT_EXECUTE},
	{NULL}
};
static MenuGizmo ownerMenu = {
	NULL, "ownerMenu", NULL, wreItems, NULL, NULL, CHK, OL_FIXEDROWS, 1
};
static MenuGizmo groupMenu = {
	NULL, "groupMenu", NULL, wreItems, NULL, NULL, CHK, OL_FIXEDROWS, 1
};
static MenuGizmo otherMenu = {
	NULL, "otherMenu", NULL, wreItems, NULL, NULL, CHK, OL_FIXEDROWS, 1
};

static GizmoRec propNameArray[] = {
	{StaticTextGizmoClass, &propName}
};
static GizmoRec propOwnerArray[] = {
	{StaticTextGizmoClass, &propOwner}
};
static GizmoRec propGroupArray[] = {
	{StaticTextGizmoClass, &propGroup}
};
static GizmoRec propModTimeArray[] = {
	{StaticTextGizmoClass, &propModTime}
};
static GizmoRec ownerMenuArray[] = {
	{MenuBarGizmoClass, &ownerMenu}
};
static GizmoRec groupMenuArray[] = {
	{MenuBarGizmoClass, &groupMenu}
};
static GizmoRec otherMenuArray[] = {
	{MenuBarGizmoClass, &otherMenu}
};

static LabelGizmo nameLabel = {
	NULL, "nameLabel", TXT_FILE_NAME, propNameArray,
	XtNumber(propNameArray), OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo ownerLabel = {
	NULL, "ownerLabel", TXT_OWNER, propOwnerArray,
	XtNumber(propOwnerArray), OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo groupLabel = {
	NULL, "groupLabel", TXT_GROUP, propGroupArray,
	XtNumber(propGroupArray), OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo modTimeLabel = {
	NULL, "modTimeLabel", TXT_MOD_TIME, propModTimeArray,
	XtNumber(propModTimeArray), OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo ownerMenuLabel = {
	NULL, "ownerMenuLabel", TXT_OWNER_ACCESS, ownerMenuArray,
	XtNumber(ownerMenuArray), OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo groupMenuLabel = {
	NULL, "groupMenuLabel", TXT_GROUP_ACCESS, groupMenuArray,
	XtNumber(groupMenuArray), OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo otherMenuLabel = {
	NULL, "otherMenuLabel", TXT_OTHER_ACCESS, otherMenuArray,
	XtNumber(otherMenuArray), OL_FIXEDROWS, 1, NULL, 0, True
};

static GizmoRec array[] = {
	{LabelGizmoClass,	&nameLabel},
	{LabelGizmoClass,	&ownerLabel},
	{LabelGizmoClass,	&groupLabel},
	{LabelGizmoClass,	&modTimeLabel},
	{LabelGizmoClass,	&ownerMenuLabel},
	{LabelGizmoClass,	&groupMenuLabel},
	{LabelGizmoClass,	&otherMenuLabel}
};

MenuItems okItems[] = {
	{True, BUT_OK,	MNEM_OK,	NULL,	OkCB},
	{NULL}
};

static MenuGizmo okMenu = {
	NULL, "okMenu", "na", okItems, 0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static PopupGizmo propertyPopup = {
	NULL, "property", TXT_FILE_PROPERTY, &okMenu, array, XtNumber (array)
};

void
DisplayProperty (CnxtRec *cr, DmObjectPtr op)
{
	PropertyList *	pl;
	PropertyList *	last = (PropertyList *)0;
	Widget		w;
	Widget		shell;
	LabelGizmo *	g;
	DirEntry *	dp = (DirEntry *)op->objectdata;

	for (pl=cr->propertyList; pl!=NULL; pl=pl->next) {
		if (strcmp (pl->name, op->name) == 0) {
			MapGizmo (PopupGizmoClass, pl->popup);
			return;
		}
		last = pl;
	}
	/* Construct a new property sheet for this item */
	pl = (PropertyList *)CALLOC (1, sizeof (PropertyList));
	if (last != (PropertyList *)0) {
		last->next = pl;
	}
	else {
		cr->propertyList = pl;
	}
	propName.text = op->name;
	propOwner.text = dp->owner;
	propGroup.text = dp->group;
	propModTime.text = dp->date;
	pl->popup = CopyGizmo (PopupGizmoClass, &propertyPopup);
	pl->name = STRDUP (op->name);
	shell = CreateGizmo (
		GetBaseWindowShell (cr->base), PopupGizmoClass, pl->popup, 0, 0
	);
	XtUnmanageChild (pl->popup->message);
	XtAddCallback (shell, XtNdestroyCallback, ClientDestroyCB, pl->popup);
	SetPermission (pl, "ownerMenu", dp->permission+1);
	SetPermission (pl, "groupMenu", dp->permission+4);
	SetPermission (pl, "otherMenu", dp->permission+7);
	w = (Widget) QueryGizmo (
		PopupGizmoClass, pl->popup, GetGizmoWidget, "okMenu"
	);
	OlVaFlatSetValues (w, 0, XtNclientData, (XtPointer)pl, (String)0);

	CenterLabel (pl->popup, PopupGizmoClass, "nameLabel");
	CenterLabel (pl->popup, PopupGizmoClass, "ownerLabel");
	CenterLabel (pl->popup, PopupGizmoClass, "groupLabel");
	CenterLabel (pl->popup, PopupGizmoClass, "modTimeLabel");
	CenterLabel (pl->popup, PopupGizmoClass, "ownerMenuLabel");
	CenterLabel (pl->popup, PopupGizmoClass, "groupMenuLabel");
	CenterLabel (pl->popup, PopupGizmoClass, "otherMenuLabel");

	MapGizmo (PopupGizmoClass, pl->popup);
}

typedef struct _tmpClientData {
	CnxtRec *	cr;
	DropObject *	dobj;
	int		item_index;
	char *		filename;
	Boolean		fromProp;	/* Failure can be from copy */
					/* or the property sheet. */
} tmpClientData;

static void
CreateTmpDirCB (Widget w, tmpClientData *p, XtPointer callData)
{
	CnxtRec *	cr = p->cr;
	char		buf[BUF_SIZE];
	extern void	ApplyCB();

	XtPopdown ((Widget) _OlGetShellOfWidget (w));

	FPRINTF ((stderr, "Create %s\n", p->filename));
	if (mkdirp (p->filename, 0777) != 0) {
		sprintf (
			buf, GGT(TXT_CANT_CREATE_DIR),
			p->filename, strerror (errno)
		);
		DisplayNormalError (cr, buf, OL_ERROR);
	}
	if (p->fromProp == True) {
		ApplyCB (w, cr, callData);
	}
	else {
		CopyOutside (cr, p->dobj, p->item_index, p->filename);
	}
}

static void
DontCreateCB (Widget w, XtPointer clientData, XtPointer callData)
{
	CnxtRec *	cr = FindCnxtRec (w);

	XtPopdown ((Widget) _OlGetShellOfWidget (w));
}

static MenuItems tmpErrorItems[] = {
	{True, BUT_YES,		MNEM_YES,	NULL, CreateTmpDirCB},
	{True, BUT_NO,		MNEM_NO,	NULL, DontCreateCB},
	{True, BUT_HELP,	MNEM_HELP,	NULL, NULL },
	{NULL}
};
static MenuGizmo tmpErrorMenu = {
	NULL, "tmpError", NULL, tmpErrorItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};
static ModalGizmo tmpErrorPopup = {
	NULL, "tmpErrorPopup", TXT_TITLE_DIR_DNE, &tmpErrorMenu
};

static void
DisplayTmpDirError (CnxtRec *cr, char *error, char *fn, tmpClientData *p)
{
	static ModalGizmo *	tmpError = NULL;
	Widget			w;

	if (tmpError == NULL) {
		tmpError = &tmpErrorPopup;
		CreateGizmo (Root, ModalGizmoClass, tmpError, 0, 0);
	}
	SetModalGizmoMessage (tmpError, error);
	w = (Widget)QueryGizmo (
		ModalGizmoClass, tmpError,
		GetGizmoWidget, "tmpError"
	);
	OlVaFlatSetValues (w, 0, XtNclientData, (XtPointer)p, (String)0);
	MapGizmo (ModalGizmoClass, tmpError);
}

void
PropertyCB (Widget wid, XtPointer clientData, XtPointer callData)
{
	CnxtRec *	cr = FindCnxtRec (wid);
	DmObjectPtr	op;
	DmItemPtr	item;
	int		i;

	item = cr->dir.itp;
	/* Display a property window for each selected object */
	for (i=0; i<cr->dir.container->num_objs; i++) {
		op = (DmObjectPtr)item->object_ptr;
		if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
			DisplayProperty (cr, op);
		}
		item += 1;
	}
}

void
FreePropertyList (CnxtRec *cr)
{
	PropertyList *pl;

	for (pl=cr->propertyList; pl!=(PropertyList *)0; pl=pl->next) {
		RemoveProperty (cr, pl);
	}
}

/* In the following routines, if a property changes then this flag */
/* will get set to True indicating that the .dtftp file needs to be updated */

static Boolean	changed = False;

static void
GetTimeoutInterval (CnxtRec *cr)
{
	Setting *	setting;
	int		timeout;

	/* Don't do anything if remote can't change the idle time */
	if (cr->prop->maxTimeout < 1) {
		return;
	}
	setting = (Setting *)QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup,
		GetGizmoSetting, "interval"
	);
	if (setting->current_value == (XtPointer)cr->prop->timeout) {
		return;
	}
	cr->prop->timeout = (int)setting->current_value;
	SetIdleCmd (cr);
	changed = True;
}

static void
GetDisconnectSetting (CnxtRec *cr)
{
	Setting *	setting;

	setting = (Setting *)QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup,
		GetGizmoSetting, "disconnect"
	);
	if (setting->current_value == setting->previous_value) {
		return;
	}
	cr->prop->disconnect = (setting->current_value == 0) ? True : False;
	changed = True;
}

/*
 * Given a string containing a path with possible environment variables
 * return a real malloc'ed path.
 */
char *
ResolvePath (char *cp)
{
	char	buf[BUF_SIZE];
	FILE *	fp;

	if (cp == NULL || *cp == '\0') {
		return NULL;
	}
	sprintf (buf, "echo %s", cp);
	if ((fp = popen (buf, "r")) != NULL) {
		fgets (buf, BUF_SIZE, fp);
		*(strchr (buf, '\n')) = '\0';
		fclose (fp);
		return STRDUP (buf);
	}
	return NULL;
}

/*
 * Create the temporary directory.
 */
Boolean
MkTmpDir (CnxtRec *cr)
{
	static tmpClientData	p;
	char *			cp;
	int			i;
	char			buf[BUF_SIZE];
	mode_t			m;

	if (cr == NULL) {
		return False;
	}
	cp = ResolvePath (cr->prop->transferFolder);
	if (strcmp (cr->prop->transferFolder, DEFAULT_FOLDER) == 0) {
		FPRINTF ((stderr, "mkdir (%s)\n", cp));
		mkdir (cp, 0777);
	}
	i = StatFile (cp, &m);
	if (i > -1 && (m & S_IFMT) == S_IFDIR) {
		if (cr->tmpdir != NULL) {
			MYFREE (cr->tmpdir);
		}
		cr->tmpdir = cp;
	}
	else {
		/* Only print this error if name is coming from */
		/* the property sheet. */
		sprintf (
			buf, GGT (TXT_DIR_DOESNT_EXIST),
			cr->prop->transferFolder
		);
		p.cr = cr;
		p.filename = cr->prop->transferFolder;
		p.fromProp = True;
		DisplayTmpDirError (cr, buf, cp, &p);
		return False;
	}
	return True;
}

static Boolean
GetTransferFolder (CnxtRec *cr)
{
	Setting *	setting;

	setting = (Setting *)QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup,
		GetGizmoSetting, "transFolder"
	);
	if (strcmp (setting->previous_value, setting->current_value) == 0) {
		return True;
	}
	if (cr->prop->transferFolder != NULL) {
		MYFREE (cr->prop->transferFolder);
	}
	cr->prop->transferFolder = STRDUP (setting->current_value);
	if (MkTmpDir (cr) == False) {
		return False;
	}
	(void)MkTmpDir (cr->copycr);
	changed = True;
	return True;
}

static void
GetTransferMode (CnxtRec *cr)
{
	Setting *	setting;

	setting = (Setting *)QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup,
		GetGizmoSetting, "mode"
	);
	if (setting->current_value == setting->previous_value) {
		return;
	}
	switch ((int)setting->current_value) {
		case 0: {
			cr->prop->transferMode = AsciiMode;
			break;
		}
		case 1: {
			cr->prop->transferMode = BinaryMode;
			break;
		}
		case 2: {
			cr->prop->transferMode = ProgramMode;
			break;
		}
	}
	TypeCmd (cr);	/* Output type <binary, ascii> */
	changed = True;
}

static void
GetDisplaySliderSetting (CnxtRec *cr)
{
	Setting *	setting;

	setting = (Setting *)QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup,
		GetGizmoSetting, "progress"
	);
	if (setting->current_value == setting->previous_value) {
		return;
	}
	cr->prop->displaySlider = False;
	if (((char *)setting->current_value)[0] == 'x') {
		cr->prop->displaySlider = True;
	}
	changed = True;
}

static void
GetDisplayReadOnlySetting (CnxtRec *cr)
{
	Setting *	setting;

	setting = (Setting *)QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup,
		GetGizmoSetting, "warning"
	);
	if (setting->current_value == setting->previous_value) {
		return;
	}
	cr->prop->showReadOnly = False;
	if (((char *)setting->current_value)[0] == 'x') {
		cr->prop->showReadOnly = True;
	}
	changed = True;
}

static void
ApplyCB (Widget w, CnxtRec *cr, XtPointer callData)
{
	ManipulateGizmo (
		PopupGizmoClass, cr->connectPropPopup, GetGizmoValue
	);
	GetTimeoutInterval (cr);
	GetDisconnectSetting (cr);
	GetTransferMode (cr);
	GetDisplaySliderSetting (cr);
	GetDisplayReadOnlySetting (cr);
	if (GetTransferFolder (cr) == False) {
		/* Tmp file was invalid and an error was posted */
		/* so just return. */
		return;
	}
	ManipulateGizmo (
		PopupGizmoClass, cr->connectPropPopup, ApplyGizmoValue
	);
	XtPopdown (cr->connectPropPopup->shell);
	if (changed == True) {
		UpdateSystemsInfo ();
	}
}

static void
ResetCB (Widget w, XtPointer clientData, XtPointer callData)
{
	CnxtRec *	cr = FindCnxtRec (w);

	ManipulateGizmo (
		PopupGizmoClass, cr->connectPropPopup, ResetGizmoValue
	);
}

static void
CancelCB (Widget w, XtPointer clientData, XtPointer callData)
{
	CnxtRec *	cr = FindCnxtRec (w);

	ManipulateGizmo (
		PopupGizmoClass, cr->connectPropPopup, ResetGizmoValue
	);
	XtPopdown ((Widget) _OlGetShellOfWidget (w));
}

static void
SetBorders (CnxtRec *cr, char *label)
{
	Widget		w;

	w = (Widget)QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup, GetGizmoWidget, label
	);
	XtVaSetValues (
		w,
		XtNalignment,		OL_LEFT,
		XtNposition,		OL_TOP,
		XtNshadowThickness,	2,
		XtNshadowType,		OL_SHADOW_ETCHED_IN,
		(String)0
	);
}

MenuItems connectionItems[] = {
	{True, BUT_OK,		MNEM_OK,	NULL,	ApplyCB},
	{True, BUT_RESET,	MNEM_RESET,	NULL,	ResetCB},
	{True, BUT_CANCEL,	MNEM_CANCEL,	NULL,	CancelCB},
	{True, BUT_HELP,	MNEM_HELP,	NULL,	NULL},
	{NULL}
};

/* "Connection Site" *************************************************/

static StaticTextGizmo systemAddr = {
	NULL, "systemAddr", NULL, NorthWestGravity, NULL
};
static GizmoRec systemAddrArray[] = {
	{StaticTextGizmoClass, &systemAddr}
};
static LabelGizmo systemLabel = {
	NULL, "systemLabel", TXT_SYSTEM_NAME, systemAddrArray,
	XtNumber(systemAddrArray), OL_FIXEDROWS, 1, NULL, 0, True
};

static StaticTextGizmo loginName = {
	NULL, "loginName", NULL, NorthWestGravity, NULL
};
static GizmoRec loginArray[] = {
	{StaticTextGizmoClass, &loginName}
};
static LabelGizmo loginLabel = {
	NULL, "loginLabel", TXT_LOGGED_IN_AS, loginArray,
	XtNumber(loginArray), OL_FIXEDROWS, 1, NULL, 0, True
};

static StaticTextGizmo actualName = {
	NULL, "actualName", NULL, NorthWestGravity, NULL
};
static GizmoRec actualNameArray[] = {
	{StaticTextGizmoClass, &actualName}
};
static LabelGizmo actualLabel = {
	NULL, "actualLabel", TXT_ACTUAL_SYSTEM, actualNameArray,
	XtNumber(actualNameArray), OL_FIXEDROWS, 1, NULL, 0, True
};
static GizmoRec bunch1[] = {
	{LabelGizmoClass,	&systemLabel},
	{LabelGizmoClass,	&loginLabel},
	{LabelGizmoClass,	&actualLabel}
};

static LabelGizmo siteLabel = {
	NULL, "siteLabel", TXT_CONNECT_SITE, bunch1, XtNumber (bunch1),
	OL_FIXEDCOLS, 1, NULL, 0, True
};

/* "Time-Out" ********************************************************/

static Setting timeoutSettings = {
	"timeout", (XtPointer)MAX_TIMEOUT, (XtPointer)MAX_TIMEOUT,
	(XtPointer)MAX_TIMEOUT
};
static NumericGizmo interval = {
	NULL, "interval", TXT_TIMEOUT_INTERVAL, MIN_TIMEOUT,
	MAX_TIMEOUT, &timeoutSettings, 2, NULL
};
static MenuItems disconnectItems[] = {
	{True, BUT_YES},
	{True, BUT_NO},
	{NULL}
};
static MenuGizmo disconnectMenu = {
	NULL, "disconnectMenu", NULL, disconnectItems, NULL, NULL, EXC,
	OL_FIXEDROWS, 1
};
static Setting disconnectSettings = {
	NULL, (XtPointer)1, (XtPointer)1, (XtPointer)1
};
static ChoiceGizmo disconnect = {
	NULL, "disconnect", TXT_DISCONNECT_AFTER, &disconnectMenu,
	&disconnectSettings
};
static GizmoRec bunch3[] = {
	{NumericGizmoClass,	&interval},
	{ChoiceGizmoClass,	&disconnect}
};
static LabelGizmo timeoutLabel = {
	NULL, "timeoutLabel", TXT_TIMEOUTS, bunch3, XtNumber (bunch3),
	OL_FIXEDCOLS, 1, NULL, 0, True
};

/* "File Transfers" **************************************************/

static Setting folderSettings;
static InputGizmo transFolder = {
	NULL, "transFolder", TXT_TRANSFER_FOLDER, DEFAULT_FOLDER,
	&folderSettings, 40
};
static MenuItems modeItems[] = {
	{True, BUT_ASCII},
	{True, BUT_BINARY},
	{True, BUT_PROGRAM},
	{NULL}
};
static MenuGizmo modeMenu = {
	NULL, "modeMenu", NULL, modeItems, NULL, NULL, EXC,
	OL_FIXEDROWS, 1
};
static Setting modeSettings = {
	NULL, (XtPointer)2, (XtPointer)2, (XtPointer)2
};
static ChoiceGizmo mode = {
	NULL, "mode", TXT_TRANSFER_MODE, &modeMenu,
	&modeSettings
};
static MenuItems progressItems[] = {
	{True, BUT_DISPLAY_PROGRESS},
	{NULL}
};
static MenuGizmo progressMenu = {
	NULL, "progressMenu", NULL, progressItems, NULL, NULL, CHK,
	OL_FIXEDROWS, 1
};
static Setting progressSettings = {
	NULL, (XtPointer)"x", (XtPointer)"x", (XtPointer)"x"
};
static ChoiceGizmo progress = {
	NULL, "progress", "", &progressMenu,
	&progressSettings
};
static MenuItems warningItems[] = {
	{True, BUT_WARN_READ_ONLY},
	{NULL}
};
static MenuGizmo warningMenu = {
	NULL, "warningMenu", NULL, warningItems, NULL, NULL, CHK,
	OL_FIXEDROWS, 1
};
static Setting warningSettings = {
	NULL, (XtPointer)"x", (XtPointer)"x", (XtPointer)"x"
};
static ChoiceGizmo warning = {
	NULL, "warning", "", &warningMenu,
	&warningSettings
};
/* The two following dummy labels are needed to prevent all */
/* captions from lining up */
static GizmoRec bunch4[] = {
	{InputGizmoClass,	&transFolder},
	{ChoiceGizmoClass,	&mode}
};
static LabelGizmo dummyLabel1 = {
	NULL, "dummyLabel1", "", bunch4, XtNumber (bunch4),
	OL_FIXEDCOLS, 1, NULL, 0, True
};
static GizmoRec bunch5[] = {
	{ChoiceGizmoClass,	&progress},
	{ChoiceGizmoClass,	&warning}
};
static LabelGizmo dummyLabel2 = {
	NULL, "dummyLabel2", NULL, bunch5, XtNumber (bunch5),
	OL_FIXEDCOLS, 1, NULL, 0, True
};
static GizmoRec bunch6[] = {
	{LabelGizmoClass,	&dummyLabel1},
	{LabelGizmoClass,	&dummyLabel2}
};
static LabelGizmo transferLabel = {
	NULL, "transferLabel", TXT_FILE_TRANSFERS, bunch6, XtNumber (bunch6),
	OL_FIXEDCOLS, 1, NULL, 0, False
};

static GizmoRec bunch2[] = {
	{LabelGizmoClass,	&siteLabel},
	{LabelGizmoClass,	&timeoutLabel},
	{LabelGizmoClass,	&transferLabel}
};

static MenuGizmo connectionMenu = {
	NULL, "connectionMenu", "na",
	connectionItems, 0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static PopupGizmo connectionPopup = {
	NULL, "connectionProp", TXT_CONNECTION_PROPERTY,
	&connectionMenu, bunch2, XtNumber (bunch2)
};

static void
GetSystemInfo (CnxtRec *cr)
{
	ConnectionProp *	prop = cr->prop;
	char *			str;

	/* System address found */
	if (cr->prop->maxTimeout < 1) {
		timeoutSettings.previous_value = (XtPointer)15;
		interval.max = 15;
	}
	else {
		timeoutSettings.previous_value = (XtPointer)prop->timeout;
		interval.max = cr->prop->maxTimeout;
	}
	disconnectSettings.previous_value = (XtPointer)1;
	if (prop->disconnect == True) {
		disconnectSettings.previous_value = (XtPointer)0;
	}
	transFolder.text = prop->transferFolder;
	modeSettings.previous_value = (XtPointer)2;
	if (prop->transferMode == AsciiMode) {
		modeSettings.previous_value = (XtPointer)0;
	}
	else if (prop->transferMode == BinaryMode) {
		modeSettings.previous_value = (XtPointer)1;
	}
	str = "_";
	if (prop->displaySlider == True) {
		str = "x";
	}
	progressSettings.initial_value = (XtPointer)STRDUP (str);
	progressSettings.current_value = (XtPointer)STRDUP (str);
	progressSettings.previous_value = (XtPointer)STRDUP (str);
	str = "_";
	if (prop->showReadOnly == True) {
		str = "x";
	}
	warningSettings.initial_value = (XtPointer)STRDUP (str);
	warningSettings.current_value = (XtPointer)STRDUP (str);
	warningSettings.previous_value = (XtPointer)STRDUP (str);
}

void
ConnectionPropertyCB (Widget win, XtPointer clientData, XtPointer callData)
{
	NumericGizmo *	g;
	Arg		arg[10];
	Widget		shell;
	CnxtRec *	cr = FindCnxtRec (win);
	Setting *	setting;
	Widget		w;
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)CancelCB},
		{NULL}
	};

	if (cr->connectPropPopup == (PopupGizmo *)0) {
		/* Get property settings from system list */
		GetSystemInfo (cr);
		systemAddr.text = cr->systemAddr;
		loginName.text = cr->userName;
		actualName.text = cr->realSystemAddr;
		cr->connectPropPopup = CopyGizmo (
			PopupGizmoClass, &connectionPopup
		);
		XtSetArg(arg[0], XtNwmProtocol, protocolCB);
		shell = CreateGizmo (
			GetBaseWindowShell (cr->base), PopupGizmoClass,
			cr->connectPropPopup, arg, 1
		);
		XtUnmanageChild (cr->connectPropPopup->message);
		CenterLabel (
			cr->connectPropPopup, PopupGizmoClass, "systemLabel"
		);
		CenterLabel (
			cr->connectPropPopup, PopupGizmoClass, "loginLabel"
		);
		CenterLabel (
			cr->connectPropPopup, PopupGizmoClass, "actualLabel"
		);
		SetBorders (cr, "siteLabel");
		SetBorders (cr, "timeoutLabel");
		SetBorders (cr, "transferLabel");

		/* Need to set initial_value of input gizmo */
		setting = (Setting *)QueryGizmo (
			PopupGizmoClass, cr->connectPropPopup,
			GetGizmoSetting, "transFolder"
		);

		/* If maximum timeout is 0 then desensitize the timeout */
		if (cr->prop->maxTimeout < 1) {
			/* This host can't set the idle timeout so */
			/* the numeric field needs to be replaced with */
			/* a static text field. */
			g = (NumericGizmo *)QueryGizmo (
				PopupGizmoClass, cr->connectPropPopup,
				GetGizmoGizmo, "interval"
			);
			XtUnmanageChild (g->textFieldWidget);
			XtVaCreateManagedWidget (
				"staticTimeout", staticTextWidgetClass,
				g->controlWidget,
				XtNstring, "15",
				(String)0
			);
		}
		XtVaGetValues (
			cr->connectPropPopup->shell,
			XtNupperControlArea, &w, (String)0
		);
		XtVaSetValues (
			w, XtNsameSize, OL_COLUMNS, (String)0
		);
	}
	w = (Widget) QueryGizmo (
		PopupGizmoClass, cr->connectPropPopup,
		GetGizmoWidget, "connectionMenu"
	);
	OlVaFlatSetValues (w, 0, XtNclientData, (XtPointer)cr, (String)0);
	MapGizmo (PopupGizmoClass, cr->connectPropPopup);
}

void
TmpDirError (CnxtRec *cr, DropObject *dobj, int item_index, char *dir)
{
	char			buf[BUF_SIZE];
	static tmpClientData	p;

	sprintf (buf, GGT(TXT_DIR_DOESNT_EXIST), dir);
	p.cr = cr;
	p.dobj = dobj;
	p.item_index = item_index;
	p.filename = dir;
	p.fromProp = False;
	DisplayTmpDirError (cr, buf, dir, &p);
}
