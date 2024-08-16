/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:main.c	1.89"
#endif

#define MAIN_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Dynamic.h>	/* For ButtonAction */
#include <Xol/OlCursors.h>	/* For OlGetBusyCursor */
#include <DtI.h>
#include <Shell.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include "mail.h"
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/Gizmos.h>

#define ROOT		RootWindowOfScreen(XtScreen(Root))

#define HELP_PATH	"dtmail" "/" "mail.hlp"

#define TIME_OUT	15000	/* 15 seconds */

#define DROP_RESOURCE	"dtmail"
#define DEFAULT_PRINT_COMMAND	     "/usr/X/bin/PrtMgr -T longline"


extern MailRec *	mailRec;
extern ReadRec *	readRec;
extern SendRec *	sendRec;
extern ManageRec *	manageRec;
extern o_ino_t		DummyDir;
extern BaseWindowGizmo	AliasWindow;
extern AliasRec *       aliasRec;

extern HelpInfo		MailHelp = {
	TXT_JUST_MAIL, HELP_MAIL_TITLE, HELP_PATH, HELP_MAIL_SECT
};

extern HelpInfo		ManagerHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_TITLE, HELP_PATH, HELP_MANAGER_SECT
};
extern HelpInfo		ReaderHelp = {
	TXT_JUST_MAIL, HELP_READER_TITLE, HELP_PATH, HELP_READER_SECT
};
extern HelpInfo		SenderHelp = {
	TXT_JUST_MAIL, HELP_SENDER_TITLE, HELP_PATH, HELP_SENDER_SECT
};
extern HelpInfo		AliasManagerHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_SECT
};

extern HelpInfo		TOCHelp = {
	TXT_JUST_MAIL, HELP_TOC_TITLE, HELP_PATH, HELP_TOC_SECT
};
extern HelpInfo		HelpDeskHelp = {
	TXT_JUST_MAIL, HELP_DESK_TITLE, HELP_PATH, HELP_DESK_SECT
};

extern HelpInfo		ManagerOpenHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_OPEN_TITLE, HELP_PATH, HELP_MANAGER_OPEN_SECT
};
extern HelpInfo		ManagerSaveAsHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_SAVEAS_TITLE,
	HELP_PATH, HELP_MANAGER_SAVEAS_SECT
};
extern HelpInfo		ManagerUndeleteHelp = {
	TXT_JUST_MAIL, HELP_MANAGER_UNDELETE_TITLE,
	HELP_PATH, HELP_MANAGER_UNDELETE_SECT
};

extern HelpInfo		ReaderOpenHelp = {
	TXT_JUST_MAIL, HELP_READER_OPEN_TITLE, HELP_PATH, HELP_READER_OPEN_SECT
};
extern HelpInfo		ReaderSaveAsHelp = {
	TXT_JUST_MAIL, HELP_READER_SAVE_TITLE, HELP_PATH, HELP_READER_SAVE_SECT
};

extern HelpInfo		SenderNewHelp = {
	TXT_JUST_MAIL, HELP_SENDER_NEW_TITLE, HELP_PATH, HELP_SENDER_NEW_SECT
};


extern HelpInfo		SenderOpenHelp = {
	TXT_JUST_MAIL, HELP_SENDER_OPEN_TITLE, HELP_PATH, HELP_SENDER_OPEN_SECT
};

extern HelpInfo		SenderExitHelp = {
	TXT_JUST_MAIL, HELP_SENDER_EXIT_TITLE, HELP_PATH, HELP_SENDER_EXIT_SECT
};

extern HelpInfo		SenderSaveAsHelp = {
	TXT_JUST_MAIL, HELP_SENDER_SAVE_TITLE, HELP_PATH, HELP_SENDER_SAVE_SECT
};

extern HelpInfo		ManagerPropertiesHelp = {
	TXT_JUST_MAIL, HELP_MANAGEPROP_TITLE, HELP_PATH, HELP_MANAGEPROP_SECT
};
extern HelpInfo		ReaderPropertiesHelp = {
	TXT_JUST_MAIL, HELP_READPROP_TITLE, HELP_PATH, HELP_READPROP_SECT
};
extern HelpInfo		SenderPropertiesHelp = {
	TXT_JUST_MAIL, HELP_SENDPROP_TITLE, HELP_PATH, HELP_SENDPROP_SECT
};

extern HelpInfo		AliasesHelp = {
	TXT_JUST_MAIL, HELP_ALIASES_TITLE, HELP_PATH, HELP_ALIASES_SECT
};
extern HelpInfo		AliasManagerUndeleteHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_UNDELETE_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_UNDELETE_SECT
};
extern HelpInfo		AliasManagerOverwriteHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_OVERWRITE_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_OVERWRITE_SECT
};
extern HelpInfo		AliasManagerSureHelp = {
	TXT_JUST_MAIL, HELP_ALIAS_MANAGER_SURE_TITLE,
	HELP_PATH, HELP_ALIAS_MANAGER_SURE_SECT
};
extern HelpInfo		MainExitHelp = {
	TXT_JUST_MAIL, HELP_MAIN_EXIT_TITLE, HELP_PATH, HELP_MAIN_EXIT_SECT
};


/*
 * The SendOnly variable, when True, means that only the send
 * window should be displayed.  This means that the mailer will
 * be open to "/" and will have nothing to display.
 */
Widget		Root;
uid_t		UserId;
char *		UserName;
char *		Home;
o_ino_t		DummyDir;
char		MailDirectory[] = NTS_VAR_MAIL;
char *		FormalApplicationName;
char *		ApplicationName;
char *		ApplicationClass;
char *		Argv_0;
int		LastSelectedMessage;	/* For include in the mailer */
MailRec *	LastMailRec;		/* MailRec containing */
					/* LastSelectedMessage. */

char *			PrintCommand = DEFAULT_PRINT_COMMAND;
MailProperties		Mail_Properties = {
		New,
		DoIt,
		Brief
	};
Boolean		Warnings = False;

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

Arg arg[50];

char *
GetUserName (id)
uid_t id;
{
	struct passwd *p;

	p = getpwuid (id);
	return STRDUP (p->pw_name);
}

char *
GetUserMailFile ()
{
	static char	filename[BUF_SIZE];
	static Boolean	first = True;
	char *		tmp;

	if (first == True) {
		/*
		 * Check MAIL environment variable, if set return this as the
		 * system mailbox name else use the default.
		 */
		tmp = (char *)getenv ("MAIL");
		if (tmp == NULL || tmp[0] == '\0') {
			strcpy (filename, MailDirectory);
			strcat (filename, "/");
			strcat (filename, UserName);
		}
		else {
			strcpy (filename, tmp);
		}
		first = False;
	}
	return filename;
}

void
HelpCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Widget			shell;
	shell = GetBaseWindowShell (manageRec->baseWGizmo);

 	switch ((int)client_data) {
		case HelpMail: {
			PostGizmoHelp (shell, &MailHelp);
			break;
		}
		case HelpManager: {
			PostGizmoHelp (shell, &ManagerHelp);
			break;
		}
		case HelpReader: {
			PostGizmoHelp (shell, &ReaderHelp);
			break;
		}
		case HelpSender: {
			PostGizmoHelp (shell, &SenderHelp);
			break;
		}
		case HelpAliasManager: {
			PostGizmoHelp (shell, &AliasManagerHelp);
			break;
		}
		case HelpTOC: {
			PostGizmoHelp (shell, &TOCHelp);
			break;
		}
		case HelpDesk: {
			PostGizmoHelp (shell, &HelpDeskHelp);
			break;
		}
		case HelpManagerOpen: {
			PostGizmoHelp (shell, &ManagerOpenHelp);
			break;
		}
		case HelpManagerSaveAs: {
			PostGizmoHelp (shell, &ManagerSaveAsHelp);
			break;
		}
		case HelpManagerUndelete: {
			PostGizmoHelp (shell, &ManagerUndeleteHelp);
			break;
		}
		case HelpReaderOpen: {
			PostGizmoHelp (shell, &ReaderOpenHelp);
			break;
		}
		case HelpReaderSaveAs: {
			PostGizmoHelp (shell, &ReaderSaveAsHelp);
			break;
		}
		case HelpSenderNew: {
			PostGizmoHelp (shell, &SenderNewHelp);
			break;
		}
		case HelpSenderOpen: {
			PostGizmoHelp (shell, &SenderOpenHelp);
			break;
		}
		case HelpSenderExit: {
			PostGizmoHelp (shell, &SenderExitHelp);
			break;
		}
		case HelpSenderSaveAs: {
			PostGizmoHelp (shell, &SenderSaveAsHelp);
			break;
		}
		case HelpManagerProperties: {
			PostGizmoHelp (shell, &ManagerPropertiesHelp);
			break;
		}
		case HelpReaderProperties: {
			PostGizmoHelp (shell, &ReaderPropertiesHelp);
			break;
		}
		case HelpSenderProperties: {
			PostGizmoHelp (shell, &SenderPropertiesHelp);
			break;
		}
		case HelpAliases: {
			PostGizmoHelp (shell, &AliasesHelp);
			break;
		}
		case HelpAliasManagerUndelete: {
			PostGizmoHelp (shell, &AliasManagerUndeleteHelp);
			break;
		}
		case HelpAliasManagerOverwrite: {
			PostGizmoHelp (shell, &AliasManagerOverwriteHelp);
			break;
		}
		case HelpAliasManagerSure: {
			PostGizmoHelp (shell, &AliasManagerSureHelp);
			break;
		}
		case HelpMainExit: {
			PostGizmoHelp (shell, &MainExitHelp);
			break;
		}
		default: {
			fprintf (
			    stderr,
			    "Impossible help entry requested, AliasHelpCB\n"
			);
		}
	}
}

void
DisplayNewMailMessage (mp, filename)
MailRec *	mp;
char *		filename;
{
	char	buf[BUF_SIZE];
	sprintf (buf, GetGizmoText (TXT_NEW_MAIL), filename);
	if (mp != (MailRec *)0) {
		UpdateFooter (mp);
		if (mp->mng != (ManageRec *)0) {
			SetBaseWindowMessage (mp->mng->baseWGizmo, buf);
			_OlBeepDisplay(
				GetBaseWindowShell (mp->mng->baseWGizmo), 1);
		}
	}
}

static void
ShowNewMail (filename, inode)
char *		filename;
o_ino_t		inode;
{
	MailRec *	mp;
	char *		cp;

	/* is this mail file already open?
	 */
	if (
		(mp = GetMailx (inode))         != 0 ||
		(mp = GetMailx (0))             != 0 ||
		(mp = GetMailx ((o_ino_t)-1))   != 0 ||
		(mp = GetMailx (DummyDir))      != 0
	) {
		if (mp->noMail == True) {
			if (SwitchMailx(mailRec,filename,
					manageRec->baseWGizmo)==True) {
				if (mp->mng != (ManageRec *)0) {
					XtVaSetValues (
						GetSummaryListWidget (mp->mng),
						XtNnumItems, mp->summary->size,
						XtNitems, mp->summary->items,
						(String)0
					);
				}
			}
		}
		else {
			/* This will check for new mail */
			(void)ProcessCommand (mp, EQUALS, NULL);
			/* Display new mail message in
			 * main window */
		}
	}
	DisplayNewMailMessage (mp, filename);
}

/*
 * IsThereNewMail? - periodically check the user's mail file to see
 * if new mail has arrived.
 */
void
IsThereNewMail (filename, id)
char *		filename;
XtIntervalId *	id;
{
	o_ino_t		inode;
	static time_t	mdate = (time_t)-1;
	time_t		t;
	static off_t	lastsize = 0;
	off_t		size;

	/* See if times on the file have changed
	 */
	if ((t = StatFile (filename, &inode, &size)) != mdate) {
		/* Don't do this the first time here */
		if (mdate != -1 && size > lastsize) {
			/* Times have changed */
			ShowNewMail (filename, inode);
		}
	}
	lastsize = size;
	mdate = t;
	XtAddTimeOut (TIME_OUT, (XtTimerCallbackProc)IsThereNewMail, filename);
}


void
SetMenuPixmap (menu, filename)
Widget	menu;
char *	filename;
{
	Screen *	screen = XtScreen(menu);
	DmGlyphRec *	gp;
	static char *	flatMenuFields[] = {
		XtNsensitive,  /* sensitive                      */
		XtNlabel,      /* label                          */ 
		XtNuserData,   /* mnemonic string		 */
		XtNuserData,   /* nextTier | resource_value      */
		XtNselectProc, /* function                       */
		XtNclientData, /* client_data                    */
		XtNset,        /* set                            */
		XtNpopupMenu,  /* button                         */
		XtNmnemonic,   /* mnemonic                       */ 
		XtNbackgroundPixmap,
		XtNbusy
	};

	/* Change the item fields to add in a background pixmap */
	XtVaSetValues (
		menu,
		XtNitemFields,		flatMenuFields,
		XtNnumItemFields,	XtNumber(flatMenuFields),
		XtNhSpace,		0,
		(String)0
	);

	if ((gp = DmGetPixmap (screen, filename)) != NULL) {
		OlVaFlatSetValues (
			menu, 0,
			XtNlabel, NULL,
			XtNbackgroundPixmap, DmMaskPixmap(menu, gp),
			(String)0
		);
	}

}

void
InitDtmail (argc, argv)
int	argc;
char *	argv[];
{
	char			buf[BUF_SIZE];
	Widget			shell;
	MailRec *		mp;

	UserId = getuid ();
	UserName = GetUserName (UserId);
	Home = (char *)getenv ("HOME");
	if (Home == NULL || Home[0] == '\0') {
		Home = "./";
	}

	/* Indicated there was no previously selected message */
	LastSelectedMessage = -1;
	LastMailRec = (MailRec *)0;
	mp = OpenMailx ();
	/* Look for mailx variables: "record", "folder", "outfolder" */
	GetSettings (mp, ProcessCommand (mp, SET_CMD, NULL));
	DummyDir = mp->inode;

	/* Construct the brief list for the read property */
	InitBriefList (mp);

	if (argc == 2) {
		(void)OpenNewMailFile (argv[1], (BaseWindowGizmo *) 0);
	}
	else
	{
		(void)OpenNewMailFile (GetUserMailFile(),
						(BaseWindowGizmo *) 0);
	}
	XtAddTimeOut (
		TIME_OUT, (XtTimerCallbackProc)IsThereNewMail,
		GetUserMailFile()
	);

	/* Initialize Alias Management */
	aliasRec->baseWGizmo = (BaseWindowGizmo *) 0;
	AliasWinInit (UserName);
}

void
GetMailProperties()
{
	char *filename;
	FILE *fp;
	MailProperties tmp;

	filename = MALLOC(strlen(Home) + strlen(Dm_DayOneName(NTS_MAILBOX, 
				UserName)) + strlen(PROPERTY_FILE) + 2);
	sprintf(filename, "%s/%s/%s", Home, 
		Dm_DayOneName(NTS_MAILBOX, UserName), PROPERTY_FILE);
	if ((fp = fopen (filename, "r")) != NULL && 
		(fread((char *) &tmp, 1, sizeof(MailProperties), fp)) == sizeof(MailProperties))
		memcpy ((char *) &Mail_Properties, (char *) &tmp, sizeof(MailProperties));
	fclose (fp);
	FREE(filename);
}

void
SaveMailProperties()
{
	char *filename;
	FILE *fp;
	MailProperties tmp;
	
	filename = MALLOC(strlen(Home) + strlen(Dm_DayOneName(NTS_MAILBOX, 
				UserName)) + strlen(PROPERTY_FILE) + 2);
	sprintf(filename, "%s/%s/%s", Home, 
		Dm_DayOneName(NTS_MAILBOX, UserName), PROPERTY_FILE);
	if ((fp = fopen (filename, "w")) != NULL ) {
		fwrite((char *) &Mail_Properties, 1, sizeof(MailProperties), fp);
	}
	FREE(filename);
        fclose (fp);
}

void
main(argc, argv)
int argc;
char *argv[];
{
#ifdef MEMUTIL
	_SetMemutilDebug(2);
#endif

	FormalApplicationName = GetGizmoText (TXT_JUST_MAIL);
	ApplicationName = "dtmail";
	ApplicationClass = "dtmail";
	Root = InitializeGizmoClient (
		ApplicationName,
		ApplicationClass,
		FormalApplicationName,
		NULL,
		NULL,
		NULL,
		0,
		&argc, argv,
		NULL,
		NULL,
		Resources, XtNumber(Resources),
		NULL,
		0,
		DROP_RESOURCE,
		(Boolean) ManageDropNotify,
		NULL
	);

	PrintCommand = STRDUP (PrintCommand);

	if (Root) {
		InitDtmail (argc, argv);
	}
	else {
		fprintf(stderr, "Can't initialize Gizmo Client -- another %s maybe running already\n", argv[0]);
		exit(0);
	}
	XtMainLoop ();
}
