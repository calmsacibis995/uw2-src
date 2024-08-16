/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:ftp.h	1.1.4.1"
#endif

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLook.h>
#include <X11/StringDefs.h>
#include <Xt/Shell.h>
#include <DtI.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/FileGizmo.h>
#include "text.h"
#include "dm.h"

#define DEFAULT_FOLDER	"$HOME/.ftp.transfers"
#define MAX_TIMEOUT	15
#define MIN_TIMEOUT	5
#define GGT		GetGizmoText
#define MAX_HISTORY	100
#define BUF_SIZE	1024
#define MAX_FOLDERS	7
#ifdef DEBUG
#define NUM_FOLDERS	2
#else
#define NUM_FOLDERS	1
#endif

#define BLAH_BLAH_BLAH	Widget wid, XtPointer clientData, XtPointer callData

#ifdef DEBUG
#define ABORT(x)	if (x == NULL) abort()
#define FPRINTF(x)	fprintf x
#define PRINTQ()	PrintQueues()
#define MYFREE	FREE
#else
#define ABORT(x)
#define FPRINTF
#define PRINTQ
#define MYFREE	FREE
#endif /* DEBUG */

#define NUM_ICONS	9

typedef enum	{ByType, ByName, BySize, ByTime} SortType;
typedef enum	{IoDone, IoNotDone, IoCanceled, IoError} IoCompletion;
typedef enum	{High, Medium, Low, Scum} Priority;
typedef enum	{NotSuspended, StartSuspend, OverWrite, DontOverWrite} Suspend;
typedef enum	{AsciiMode, BinaryMode, ProgramMode} TransferMode;

typedef int			FileType;
typedef void			(*PFV)();
typedef int			(*PFI)();
typedef IoCompletion		(*PFIO)();

typedef struct _PropertyList {		/* Data for each file's prop sheet */
	char *			name;
	PopupGizmo *		popup;
	struct _PropertyList *	next;
} PropertyList;

typedef struct _ConnectionProp {
	Boolean			showReadOnly;	/* Show Read only warning */
	int			timeout;	/* Timeout interval */
	int			maxTimeout;	/* Maximum timeout interval */
	Boolean			disconnect;	/* After timeout */
	TransferMode		transferMode;	/* ASCII, Binary, Program */
	Boolean			displaySlider;	/* Copy progress popup */
	char *			transferFolder;
} ConnectionProp;

typedef struct _Systems {		/* Data from $HOME/.dtftp */
	char *			systemAddr;
	ConnectionProp *	prop;
	struct _Systems *	next;
} Systems;

typedef struct _DropObject {		/* Stores data for remote/local copy */
	Position	x;
	Position	y;
	Time		time;
	Window		window;
	Widget		w;
	char **		srcfiles;	/* host1:<path>/name */
	char **		destfiles;	/* host2:<other path>/name */
	int		numfiles;
	int		fileindex;
	int *		numHash;
	char *		type;		/* '-' reg file, 'd' directory */
	/* There are two different commands that can get exec'ed after a */
	/* file has been copied.  The first is a function pointer that */
	/* either points to PrintCmd, EditCmd or ExecOpen.  The second */
	/* is a string that can get system()'ed.  This string is */
	/* constructed from the _DROP property of the drop site. */
	/* If this second value is !NULL then the function should be */
	/* ExecOpen and should be called. */
	PFV		cmd;		/* The edit or print command */
	char *		exec;		/* String for doing copy. */
	struct _CnxtRec *	cr;	/* This connect record is only */
					/* used by the EndPut routine  */
					/* to display the contents of */
					/* the directory at the end of a */
					/* put command.  It's the connection */
					/* to the host that isn't the copy */
					/* connection. */
} DropObject;

typedef struct _DirRec {
	DmObjectPtr		start;	/* Front of directory listing */
	DmObjectPtr		end;	/* Back of directory listing */
	DmContainerPtr		container;
	int			size;	/* Number of files */
	DmItemPtr		itp;
	Widget			cw;	/* Container widget */
} DirRec, *DirPtr;

typedef struct _DirEntry {	/* One line from "ls -l" (dir) */
	char *		permission;
	int		links;
	char *		owner;
	char *		group;
	long		size;
	char *		date;
	int		itemIndex;
} DirEntry;

typedef struct _CmdRec {
	struct _CnxtRec *	cr;	/* Connection queueing the command */
	struct _CmdRec *	next;	/* Next in list */
	char *			srcfile;/* Source filename for copy commands */
	char *			destfile;/* Dest filename for copy cmds */
	int			index;	/* # times output called */
	int			group;	/* Used to group commands together */
	int			pri;	/* Priority of this command */
	Boolean			canceled;/* Command has been canceled */
	Boolean			inv;	/* Invalid command encountered */
	struct _StateTable *	stateTable;
	int			state;
	PFV			cmd;	/* Command to get exec'ed 1st time */
	PFV			free;	/* Cmd to call to free clientData */
	XtPointer		clientData;	/* List of files for copy */
						/* or DmObjectPtr */
	char			name[100];
} CmdRec, *CmdPtr;

typedef struct _CmdQueue {
	struct _CmdRec *	high;
	struct _CmdRec *	highBottom;
	struct _CmdRec *	med;
	struct _CmdRec *	medBottom;
	struct _CmdRec *	low;
	struct _CmdRec *	lowBottom;
	struct _CmdRec *	scum;
	struct _CmdRec *	scumBottom;
} CmdQueue;

typedef struct _CnxtRec {
	char *			systemAddr;	/* From argv[2] */
	char *			realSystemAddr;	/* As reported by ftp */
	BaseWindowGizmo *	base;
	PopupGizmo *		connectPropPopup;/* Connection prop sheet */
	PopupGizmo *		newdirPopup;
	PopupGizmo *		renamePopup;
	PopupGizmo *		passwdPopup;
	ModalGizmo *		sliderPopup;
	FileGizmo *		copyPopup;
	MenuGizmo *		iconMenu;	/* Icon box popdown menu */
	FileGizmo *		otherPopup;	/* Open other folder */
	int			numFolders;	/* # folders in folder menu */
	Boolean			connected;	/* False after time out */
	Boolean			copyMapped;	/* copyPopup mapped */
	Boolean			sliderUp;	/* Copy slider is mapped */
	Boolean			dirInProgress;	/* A DirCmd is on the queue */
	ConnectionProp *	prop;		/* Connection properties */
	PropertyList *		propertyList;
	FILE *			fp[2];
	int			layoutOptions;	/* 0 or UPDATE_LABEL */
	Boolean			hardCoded;	/* pixmaps not from dtm */
	int			hash;	/* # of #'s recieved so far in copy */
	int			hashSize;	/* # of bytes/# */
	int			numHash;/* Total # of #'s expected in copy */
	int			selectedFiles;	/* # selected files < 3 */
	pid_t			pid;	/* Process id of child ftp process */
	char *			tmpdir;	/* Name of dir for local copies */
	char *			tmpfile;/* Name of temp file for this conn. */
	char *			passwd;	/* Actual password on remote system */
	char *			pwd;	/* Current directory on remote */
	char *			home;	/* Directory when first connected */
	char			buffer[BUF_SIZE];
	char *			messages;/* User login messages */
	char *			messageMarker;/* Ex: 230- */
	char *			userName;
	int			unresolvedLinks;/* Means don't do dir update */
	SortType		sort;	/* Type of icon sorting */
	DmViewFormatType	format;
	Boolean			showMessages;	/* Show message pane */
	Boolean			copyCnxt;	/* True for copycr */
	char *			lineHistory[MAX_HISTORY]; /* 4 error logging */
	int			lineIndex;	/* Next line in lineHistory */
	XtIntervalId		timeout;	/* Id from XtAddTimeOut */
	Boolean			outOfDateFooter;/* Footer is out-of-date */
	struct _DirRec		dir;	/* List of files on remote */
	struct _CmdQueue *	queues;	/* 4 various priority command queues */
	struct _CmdRec *	top;	/* Top of a command queue */
	struct _CnxtRec *	copycr;	/* Connection used for copying */
	struct _CnxtRec *	next;
#ifdef DEBUG
	int			id;	/* Number for id'ing cr's */
#endif
} CnxtRec;

typedef struct _FtpRec {
	char *			defaultPrinter;
	Systems *		systems;

	/* dtm resources from .Xdefaults file */
	Boolean			showFullPaths;
	int			gridHeight;
	int			gridWidth;
	int			folderCols;
	int			folderRows;
	OlFontList *		fontList;

	int			outOfDateFooter;/* Number of cnxt with */
						/* footers needing update */
	DmFclassPtr		icons[NUM_ICONS];/* Pointers to icons */
	ModalGizmo *		invalidPasswdModal;
	char *			systemName;	/* Name of this system */
	XtIntervalId		timeout;	/* Id from XtAddTimeOut */
	Suspend			suspended;	/* Queue's been shutdown */
	struct _CnxtRec *	first;		/* Start of connect list */
	struct _CnxtRec *	last;		/* End of connect list */
	struct _CnxtRec *	current;	/* Points to connection */
						/* with next command */
} FtpRec;

#include "states.h"

extern Widget		Root;
extern FtpRec *		ftp;

extern void		WindowManagerEventHandler (BLAH_BLAH_BLAH);
extern void		ReadTimeOutCB (
				XtPointer *client_data, XtIntervalId *id
			);
extern Boolean		Expect (CmdPtr cp, char *expect);
extern Boolean		ErrorIf (CmdPtr cp, char *errorString);
extern void		User (CnxtRec *cr);
extern void		UserCmd (CnxtRec *cr, int group, Priority pri);
extern void		OpenCmd (CnxtRec *cr, int group, Priority pri);
extern CnxtRec *	FindCnxtRec (Widget w);
extern void		OpenCB (BLAH_BLAH_BLAH);
extern void		PrintCB (BLAH_BLAH_BLAH);
extern void		FoldersCB (BLAH_BLAH_BLAH);
extern void		QueueCmd (
				char *name,
				CnxtRec *cr,
				PFV cmd,
				StateTable *table,
				int group,
				char *srcfile,
				char *destfile,
				XtPointer clientData,
				PFV free,
				Priority pri
			);
extern void		ExitDtftp (CnxtRec *cr);
extern CnxtRec *	CreateConnection (char *username, char *systemName);
extern CnxtRec *	CreateCopyConnection (CnxtRec *cr);
extern void		RemoveConnection (CnxtRec *cr);
extern void		RemoteLocalCopy (
				CnxtRec *cr, DropObject *dobj, int grp,
				Priority p
			);
extern void		DisplayError (CnxtRec *cr, char *string, PFV cancel);
extern void		SetSliderFilenames (
				CnxtRec *cr, char *fname, char *toname
			);
extern void		CreateSliderPopup (CmdPtr cp);
extern void		FreeGroup (int grp);
extern void		Connect (CnxtRec *cr);
extern void		CdCmd (
				CnxtRec *cr, char *name,
				int group, Priority pri
			);
extern void		DirCmd (CnxtRec *cr, int grp, Priority pri);
extern void		DelCmd (CnxtRec *cr, char *name, int grp);
extern void		CancelCB (BLAH_BLAH_BLAH);
extern void		DteditCmd (CnxtRec *cr, char *name);
extern void		RequeueCmd (
				CmdPtr cp,
				CnxtRec *workingcr,
				PFV cmd,
				StateTable *table,
				int group,
				char *srcfile,
				char *destfile
			);
extern void		RemoveGroup (int grp);
extern void		SetDir (CnxtRec *cr, char *name);
extern void		PwdCmd (CnxtRec *cr, int grp, Priority pri);
extern void		StatusCmd (CnxtRec *cr, Priority pri);
extern void		HashCmd (CnxtRec *cr, int grp);
extern void		PromptCmd (CnxtRec *cr);
extern void		LocalRemoteCopy (
				CnxtRec *cr, DropObject *dobj, int grp
			);
extern void		Output (CnxtRec *cr, char *string);
extern int		NextCmdGroup ();
extern void		EditRenameCB (BLAH_BLAH_BLAH);
extern void		RenameCmd (CnxtRec *cr, char *oldname, char *newname);
extern void		UpdateCopyPopup (CnxtRec *cr);
extern void		CopyCB (BLAH_BLAH_BLAH);
extern void		SelectCB (BLAH_BLAH_BLAH);
extern void		CopyOutside (
				CnxtRec *cr, DropObject *dobj,
				int item_index, char *dir
			);

extern off_t		StatFile (char *filename, mode_t *m);
extern void		BaseWindowTitle (CnxtRec *cr);
extern void		RmDirCmd (CnxtRec *cr, char *name);
extern void		MkDirCmd (
				CnxtRec *cr, int grp, char *dir, Priority p
			);
extern void		FileExistsPopup (
				CnxtRec *cr, char *name, char *system
			);
extern void		RestartQueues (CnxtRec *cr, Suspend s);
extern void		StopCB (BLAH_BLAH_BLAH);
extern void		NewFolderCB (BLAH_BLAH_BLAH);
extern void		PropertyCB (BLAH_BLAH_BLAH);
extern void		ConnectionPropertyCB (BLAH_BLAH_BLAH);
extern void		AddToFolderMenu (CnxtRec *cr, char *filename);
extern void		FreePropertyList (CnxtRec *cr);
extern void		FreeDobj (DropObject *dobj);
extern void		RemoveCmd (CnxtRec *cr, CmdPtr cp);
extern void		SortCB (BLAH_BLAH_BLAH);
extern void		OpenFile (CnxtRec *cr, DmObjectPtr op, int index);
extern void		PrintFile (CnxtRec *cr, DmObjectPtr op, int index);
extern void		DisplayProperty (CnxtRec *cr, DmObjectPtr op);
extern void		DeleteFile (CnxtRec *cr, DmObjectPtr op);
extern void		EventHandler (
				Widget w, CnxtRec *cr, XEvent *xevent,
				Boolean cont
			);
extern void		GetDirCmd (
				CnxtRec *cr, int grp, char *buf,
				char *dir, DropObject *dobj, Priority p
			);
extern void		PopDownPopup (CmdPtr cp);
extern void		PutCmd (
				CnxtRec *cr, int grp, char *src, char *dest,
				DropObject *dobj
			);
extern void		Connected (CnxtRec *cr);
extern void		CopyForEdit (CnxtRec *cr, int itemIndex, char *name);
extern void		DropProcCB (
				Widget wid, XtPointer client_data,
				OlFlatDropCallData *call_data
			);
extern Boolean		TriggerCB (
				Widget w, Window win, Position rootx,
				Position rooty, Atom selection,
				Time timestamp, OlDnDDropSiteID drop_site_id,
				OlDnDTriggerOperation op, Boolean send_done,
				Boolean forwarded, XtPointer client_data
			);
extern char *		GetFileName (char *name, DirEntry *dp, DmFileType *f);
extern Atom		NextAtom (Widget shell, PFV cb, XtPointer clientData);
extern void		GetCmd (
				CnxtRec *cr, int grp, DropObject *dobj,
				char *src, char *dest
			);
extern void		ExitCB (BLAH_BLAH_BLAH);
extern void		SetMultiSelectSensitivity (CnxtRec *cr, int count);
extern void		Abbend (CnxtRec *cr);
extern void		ReallyExitCB (
				Widget wid, CnxtRec *cr, XtPointer call_data
			);
extern Boolean		CollectMsg (CnxtRec *cr);
extern void		ParentDirCB (BLAH_BLAH_BLAH);
extern FileType		SetType (char *line);
extern int		SetBaseWindowFooter (CnxtRec *cr, int count);
extern void		SetIdleCmd (CnxtRec *cr);
extern void		IdleCmd (CnxtRec *cr);
extern void		WindowManagerEventHandler (BLAH_BLAH_BLAH);
extern char *		ResolvePath (char *cp);
extern Boolean		MkTmpDir (CnxtRec *cr);
extern void		SetDefaultProperties (ConnectionProp *p);
extern void		TypeCmd (CnxtRec *cr);
extern void		UpdateSystemsInfo ();
extern void		TmpDirError (
				CnxtRec *cr, DropObject *dobj, int item_index,
				char *dir
			);
extern void		ActivateTimeout (CnxtRec *cr);
extern void		QueueGetFileClassRequest (
				Widget shell, PFV func, char *name,
				FileType type, XtPointer t
			);
extern void		CenterLabel (
				Gizmo g, GizmoClass gizmoClass, char *name
			);
extern void		GetDataProperties (CnxtRec *cr, PFV func, char *file);
extern void		Execute (char *name, char *value);
extern void		FreeDir (CnxtRec *cr);
extern void		MarkOld (CnxtRec *cr);
extern void		DrawLinkIcon (
				Widget w, XtPointer client_data,
				OlFIconDrawPtr draw_info
			);
extern void		DrawNameIcon (BLAH_BLAH_BLAH);
extern void		DrawLongIcon (BLAH_BLAH_BLAH);
extern char *		GetShortName (DmItemPtr item);
extern char *		GetLongName (DmItemPtr item, int len);
extern void		FormatCB (BLAH_BLAH_BLAH);
extern void		SortItems (CnxtRec *cr);
extern void		DisplayNormalError (CnxtRec *cr, char *buf, int type);
extern void		CopyFileName (
				CnxtRec *cr, DmFileType type, long size,
				char *name, DropObject *dobj, int *num,
				char* src, char *dest
			);
extern void		DropOnFolder (
				CnxtRec *cr, char *name, DropObject *dobj
			);
extern void		PrintCmd (CnxtRec *cr, char *name, DropObject *dobj);
extern void		EditCmd (CnxtRec *cr, char *name, DropObject *dobj);
