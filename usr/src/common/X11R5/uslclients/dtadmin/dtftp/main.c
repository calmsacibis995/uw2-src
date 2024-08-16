/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:main.c	1.1.2.1"
#endif

#include "ftp.h"
#include <X11/Shell.h>
#include <Gizmo/MenuGizmo.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/fcntl.h>
#include <sys/utsname.h>

#define ROOT		RootWindowOfScreen(XtScreen(Root))
#define HELP_PATH	"dtftp" "/" "dtftp.hlp"
#define DROP_RESOURCE	"dtftp"

Widget		Root;
char *		FormalApplicationName;
char *		ApplicationName;
char *		ApplicationClass;
char *		Argv_0;

FtpRec		ftpRec;
FtpRec *	ftp = &ftpRec;

/*
 * Retrieve the resource showFullPaths from .Xdefaults
 */

#define OFFSET(field)	XtOffsetOf (FtpRec, field)

static OlDynamicCallbackProc
GetFullPathSetting (XtPointer clientData)
{
	CnxtRec *		cr;
	Boolean			full;
	static XtResource	resources [] = {
		{
			"showFullPaths", "ShowFullPaths",
			XtRBoolean, sizeof (Boolean),
			OFFSET (showFullPaths), XtRBoolean, NULL
		},
		{
			"gridHeight", XtCHeight, XtRInt, sizeof (int),
			OFFSET (gridHeight), XtRInt, NULL
		},
		{
			"gridWidth", XtCHeight, XtRInt, sizeof (int),
			OFFSET (gridWidth), XtRInt, NULL
		},
		{
			"folderCols", "Cols", XtRInt, sizeof (int),
			OFFSET(folderCols), XtRInt, NULL
		},
		{
			"folderRows", "Rows", XtRInt, sizeof (int),
			OFFSET(folderRows), XtRInt, NULL
		},
		{
			XtNfontGroup, XtCFontGroup, XtROlFontList,
			sizeof (OlFontList *),
			OFFSET(fontList), XtRImmediate, NULL
		},
	};

	FPRINTF ((stderr, "Resources changed\n"));
	full = ftp->showFullPaths;
	XtGetApplicationResources (
		Root, ftp, resources, XtNumber (resources), NULL, 0
	);
	if (ftp->showFullPaths != full) {
		/* Loop thru all of the base windows and set the title */
		cr = ftp->first;
		if (cr != (CnxtRec *)0) {
			do {
				BaseWindowTitle (cr);
				cr = cr->next;
			} while (cr != ftp->first);
		}
	}
}

/*
 * Write the updated properties info into the $HOME/.dtftp file.
 */
void
UpdateSystemsInfo ()
{
	char *		cp;
	Systems *	sp = ftp->systems;
	FILE *		fp;
	char		buf[BUF_SIZE];
	int		timeout;

	sprintf (buf, "%s/.dtftp", getenv ("HOME"));
	if ((fp = fopen (buf, "w")) == NULL) {
		return;
	}
	for (sp=ftp->systems; sp; sp=sp->next) {
		fprintf (fp, "%s\t", sp->systemAddr);
		timeout = 15;
		if (sp->prop->timeout > 0) {
			timeout = sp->prop->timeout;
		}
		fprintf (fp, "%d\t", timeout);
		fprintf (fp, "%s\t", sp->prop->disconnect==True ? "Y" : "N");
		if (sp->prop->transferMode == BinaryMode) {
			fprintf (fp, "B\t");
		}
		else if (sp->prop->transferMode == AsciiMode) {
			fprintf (fp, "A\t");
		}
		else { 
			fprintf (fp, "P\t");
		}
		fprintf (
			fp, "%s\t", sp->prop->displaySlider==True ? "Y" : "N"
		);
		fprintf (
			fp, "%s\t", sp->prop->showReadOnly == True ? "Y" : "N"
		);
		fprintf (fp, "%s\n", sp->prop->transferFolder);
	}
	fclose (fp);
}

void
SetDefaultProperties (ConnectionProp *p)
{
	p->showReadOnly = True;
	p->timeout = MAX_TIMEOUT;
	p->disconnect = False;
	p->transferMode = ProgramMode;
	p->displaySlider = True;
	p->transferFolder = STRDUP (DEFAULT_FOLDER);
}

#define NEXT(cp)	cp = strtok (NULL, "\t"); \
			if (cp == NULL) { \
				continue; \
			}

/*
 * Read the contents of the $HOME/.dtftp file and store it
 * in the systems list.  The $HOME/.dtftp file has the following
 * fields separated by tabs:
 * SYSTEM	<system adress>
 * TIMEOUT	<5-15>
 * DISCONNECT	<Y,N>
 * TRANSFERMODE	<A,B,P>
 * SHOWTRANSFER	<Y,N>
 * SHOWREADONLY	<Y,N>
 * FOLDER	$HOME/.ftp.transfers
 */
static void
ReadDtftpFile ()
{
	FILE *			fp;
	char			buf[BUF_SIZE];
	Systems *		last = (Systems *)0;
	Systems *		next = (Systems *)0;
	char *			cp;
	ConnectionProp *	p;

	sprintf (buf, "%s/.dtftp", getenv ("HOME"));
	if ((fp = fopen (buf, "r")) == NULL) {
		return;
	}
	while (fgets (buf, BUF_SIZE, fp) != NULL) {
		*(strchr (buf, '\n')) = '\0';
		cp = strtok (buf, "\t");
		if (cp == NULL) {
			continue;	/* Ignore this entry */
		}
		next = (Systems *)CALLOC (1, sizeof (Systems));
		next->prop = (ConnectionProp *) CALLOC (
			1, sizeof (ConnectionProp)
		);
		p = next->prop;
		if (last == (Systems *)0) {
			last = next;
			ftp->systems = last;
		}
		else {
			last->next = next;
			last = next;
		}
		next->systemAddr = STRDUP (cp);
		SetDefaultProperties (p);

		NEXT (cp);	/* Timeout interval */
		p->timeout = atoi (cp);
		if (p->timeout < MIN_TIMEOUT) {
			p->timeout = MIN_TIMEOUT;
		}

		NEXT (cp);	/* Disconnect after timeout */
		p->disconnect = strcmp (cp, "Y") == 0 ? True : False;

		NEXT (cp);	/* Transfer mode */
		p->transferMode = ProgramMode;
		switch (cp[0]) {
			case 'A': {
				p->transferMode = AsciiMode;
				break;
			}
			case 'B': {
				p->transferMode = BinaryMode;
				break;
			}
		}

		NEXT (cp);	/* Display progress window */
		p->displaySlider = strcmp (cp, "Y") == 0 ? True : False;

		NEXT (cp);	/* Display read only window */
		p->showReadOnly = (*cp ==  'Y') ? True : False;

		NEXT (cp);	/* Transfer folder */
		MYFREE (p->transferFolder);
		p->transferFolder = STRDUP (cp);
	}
	fclose (fp);
}

static char *	iconFilenames[] = {
	"unk.icon",
	"dir.icon",
	"exec.icon",
	"datafile.icon",
	"pipe.icon",
	"chrdev.icon",
	"blkdev.icon",
	"sem.icon",
	"shmem.icon"
};

static void
InitDtftp (argc, argv)
int	argc;
char *	argv[];
{
	char		buf[BUF_SIZE];
	struct utsname	name;
	int		type;

	/* Indicate the pixmaps have been obtained from hardcoded resources */
	/* until the values can be retrieved from dtm. */
	for (type=0; type<NUM_ICONS; type++) {
		ftp->icons[type] = (DmFclassPtr)CALLOC(1,sizeof(DmFclassRec));
		ftp->icons[type]->glyph = DmGetPixmap (
			XtScreen(Root), iconFilenames[type]
		);
		ftp->icons[type]->cursor = DmGetCursor (
			XtScreen(Root), iconFilenames[type]
		);
	}
	ftp->timeout = (XtIntervalId)0;
	ftp->suspended = NotSuspended;
	uname (&name);
	ftp->systemName = STRDUP (name.nodename);
	ftp->invalidPasswdModal = (ModalGizmo *)0;
	ftp->first = (CnxtRec *)0;
	ftp->last = (CnxtRec *)0;
	ftp->outOfDateFooter = 0;
	GetFullPathSetting ((XtPointer)0);
	ReadDtftpFile ();
	ftp->first = CreateConnection (argv[1], argv[2]);
	ftp->current = ftp->first;
	ftp->current->top = (CmdPtr)0;
	/* Get from the .Xdefaults showFullPaths resource value */
	OlRegisterDynamicCallback (
		(OlDynamicCallbackProc)GetFullPathSetting, NULL
	);
}

static void
OpenNewFtpConnection (char *username, char *systemAddr)
{
	(void)CreateConnection (username, systemAddr);
}

static void
StartCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
	if (dip->error != 0 || dip->nitems != 2) {
		return;
	}
	if (dip->files != (char **)0) {
		if (dip->files[0][0] != '\0') {
			OpenNewFtpConnection (dip->files[0], dip->files[1]);
		}
	}
}

static Boolean
StartNewFtp (
	Widget w, Window win, Position x, Position y, Atom selection,
	Time timestamp, OlDnDDropSiteID drop_site_id,
	OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
	XtPointer closure
)
{
	FPRINTF ((stderr, "Attempting to open new connection\n"));
	DtGetFileNames (w, selection, timestamp, send_done, StartCB, closure);
}

#define DEFAULT_PRINT_COMMAND	     "/usr/X/bin/PrtMgr"

char *			PrintCommand = DEFAULT_PRINT_COMMAND;

static Boolean		Warnings = False;

static XtResource	Resources[] = {
	{
		"warnings", "Warnings", XtRBoolean, sizeof(Boolean),
		(Cardinal) &Warnings, XtRBoolean, (XtPointer) &Warnings
	},
	{
		"printCommand", "printCommand", XtRString, sizeof(char *),
		(Cardinal) &PrintCommand, XtRString, DEFAULT_PRINT_COMMAND
	}
};

/*
 * This main loop is here to capture events such that footers
 * can be updated if a key press of button press occurs in that window.
 * The variable ftp->outOfDateFooter is the number of connections
 * that have out-of-date footers.  If this number is zero then
 * nothing needs to be done.
 */
static void
MyAppMainLoop(app)
XtAppContext app;
{
	XEvent		event;
	Widget		wid;
	CnxtRec *	cr;

	for (;;) {
		XtAppNextEvent(app, &event);
		XtDispatchEvent(&event);
		/* First determine if there are any connections that */
		/* need their footers updated. */
		if (ftp->outOfDateFooter == 0) {
			continue;
		}
		if (event.type != ButtonPress && event.type != KeyPress) {
			continue;
		}
		wid = XtWindowToWidget (XtDisplay (Root), event.xkey.window);
		cr = FindCnxtRec (wid);
		if (cr == NULL) {
			continue;
		}
		if (cr->outOfDateFooter == True) {
			ftp->outOfDateFooter -= 1;
			cr->outOfDateFooter = False;
			SetBaseWindowFooter (cr, -1);
		}
	}
}

static void
MyMainLoop()
{
	MyAppMainLoop(_XtDefaultAppContext());
}

/*
 * Usage: dtftp username systemname
 */
void
main(argc, argv)
int argc;
char *argv[];
{
#ifdef MEMUTIL
	_SetMemutilDebug(2);
#endif

	FormalApplicationName = GGT(TXT_JUST_FTP);
	ApplicationName = "dtftp";
	ApplicationClass = "dtftp";
	Root = InitializeGizmoClient (
		ApplicationName,
		ApplicationClass,
		FormalApplicationName,
		NULL,
		NULL,
		NULL,
		0,
		&argc,
		argv,
		NULL,
		NULL,
		Resources,
		XtNumber(Resources),
		NULL,
		0,
		DROP_RESOURCE,
		StartNewFtp,
		NULL
	);

	PrintCommand = STRDUP (PrintCommand);

	if (Root) {
		InitDtftp (argc, argv);
	}
	MyMainLoop ();
}

#ifdef DEBUG
static void
PrintQueue (CmdPtr cp)
{
	FPRINTF ((stderr,
		"%20s %5s %20s %20s %5s %8s %3s %5s\n",
		"NAME", "GROUP", "SRC FILE", "DESTFILE",
		"INDEX", "CANCELED", "INV", "STATE"
	));
	for (; cp; cp=cp->next) {
		FPRINTF ((
			stderr, 
			"%20s %5d %20s %20s %5d %8d %3d %5d\n",
			cp->name, cp->group, cp->srcfile,
			cp->destfile, cp->index, cp->canceled,
			cp->inv, cp->state
		));
	}
	FPRINTF ((stderr, "\n"));
}

void
PrintQueues ()
{
	CnxtRec *	cr;

	cr = ftp->last;
	do {
		cr = cr->next;
		FPRINTF ((
			stderr,
			"Command Queue %s 0x%x %d -------------------------\n",
			cr->systemAddr, cr, cr->id
		));
		if (cr->queues->high) {
			FPRINTF ((stderr, "-----------High-------------\n"));
			PrintQueue (cr->queues->high);
		}
		if (cr->queues->med) {
			FPRINTF ((stderr, "-----------Medium-----------\n"));
			PrintQueue (cr->queues->med);
		}
		if (cr->queues->low) {
			FPRINTF ((stderr, "-----------Low--------------\n"));
			PrintQueue (cr->queues->low);
		}
		if (cr->queues->scum) {
			FPRINTF ((stderr, "-----------Scum-------------\n"));
			PrintQueue (cr->queues->scum);
		}
	} while (cr != ftp->last);
}
#endif /* DEBUG */
