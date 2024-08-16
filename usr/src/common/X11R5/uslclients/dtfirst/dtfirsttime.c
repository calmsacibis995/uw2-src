/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtfirst:dtfirsttime.c	1.7"
#endif
/*
 *	dtfirsttime 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/prosrfs.h>
#include <sys/unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <string.h>

#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include "dtfirst_msgs.h"

#include <DesktopP.h>
#include "DtI.h"

#define DesktopUserFile "/usr/X/desktop/LoginMgr/Users"
#define StartupFile	"Preferences/Startup_Items/Welcome"
#define DTFIRSTTIME	"dtfirsttime"
#define BUF_SIZE 	2048

extern 	int	errno;

/* These marcos are used to register the mnumonics support on buttons */
#define Xm_MNE(M,MNE_INFO,OP)\
        if (M) {\
                mne = (unsigned char *)mygettxt(M);\
                ks = XStringToKeysym((char *)mne);\
                (MNE_INFO).mne = (unsigned char *)(char *)mne;\
                (MNE_INFO).mne_len = strlen((char *)mne);\
                (MNE_INFO).op = OP;\
        } else {\
                mne = (unsigned char *)NULL;\
                ks = NoSymbol;\
        }

#define XmSTR_N_MNE(S,M,M_INFO,OP)\
        string = XmStringCreateSimple(S);\
        Xm_MNE(M,M_INFO,OP)

        /* Assume i */
#define REG_NME(W,MNE_INFO,N_MNE)\
        DmRegisterMnemonic(W,MNE_INFO,N_MNE)

#define MI_TOTAL	10

void	getHelpCB(Widget, XtPointer, XtPointer);
void	deleteCB(Widget, XtPointer, XtPointer); 	
void	closeCB(Widget, XtPointer, XtPointer);

Widget		toplevel, questionBox;
XtAppContext	app;
Arg		args[10];
Boolean		I_am_owner=FALSE;
char		*Home, *Login, *DTFIRSTTIMEAPP;

typedef struct {
	char	*title;
	char	*file;
	char	*section;
} HelpText;

HelpText AppHelp = {
	"","dtfirst/dtfirst.hlp","10",
};

Boolean
isOwner()
{
	FILE	*fd;
	char	buf[BUF_SIZE];
	int	len;

	
	sprintf(buf, "%s/%s", DesktopUserFile, Login); 
	if ((fd = fopen(buf, "r")) == NULL)
		return FALSE;	

	/* name = regcmp("^owner[\n$]", (char *)0);
		found = regex(name, buf);
	*/

	len = strlen("owner");
	while (fgets(buf, BUF_SIZE, fd)) {
		if ((strncmp(buf, "owner", len) == 0)  && 
			(buf[len] == '\n' || buf[len] == '\0')) 
			return TRUE;
	}
	fclose(fd);
	return FALSE;	
}

char *
mygettxt(char * label)
{
	char	*p;
	char	c='\001';
	
	if (label == NULL)
		return((char *)NULL);
	for (p = label; *p; p++)
		if (*p == c) {
			*p++ = 0;
			label = (char *)gettxt(label, p);
			*--p = c;
			break;
		}
		
	return(label);
}

void ExitWelcomeHandler(Widget w, XtPointer client_data, XEvent *xevent, Boolean *cont_to_dispatch)
{
	DtReply		reply;
	Display		*display;
	Screen		*screen;

	display = XtDisplay(w);
	screen = XtScreen(w);
	if (!((xevent->type == SelectionNotify) &&
		(xevent->xselection.selection == _DT_QUEUE(display))))
		return;

	memset(&reply, 0, sizeof(reply));
	DtAcceptReply(screen, _DT_QUEUE(display),
		XtWindow(w), &reply);

	if (reply.header.rptype == DT_SYNC) {
		XtUnmanageChild(questionBox);
		exit(0);
	}
		
}

/*
 * DisplayHelp() -- Send a message to dtm to display a help window.  
 *		If help is NULL, then ask dtm to display the help desk.
 */

void
DisplayHelp (Widget widget, HelpText *help)
{
    DtRequest			*req, *sreq;
    DtDisplayHelpRequest	displayHelpReq;
    DtSyncRequest		syncReq;
    Display			*display;
    Window			win;
    char *AppTitle=DTFIRSTTIME;
    char *AppName=DTFIRSTTIME;

    if (widget == NULL)
	/* what else we can do ?*/
	exit(0);

    display = XtDisplay(widget);
    win = XtWindow(XtParent(widget));

    req = (DtRequest *) &displayHelpReq;
    memset(req, 0, sizeof(displayHelpReq));
    displayHelpReq.rqtype = DT_DISPLAY_HELP;
    displayHelpReq.serial = 0;
    displayHelpReq.version = 1;
    displayHelpReq.client = win;
    displayHelpReq.nodename = NULL;

    if (help)
    {
	displayHelpReq.source_type =
	    help->section ? DT_SECTION_HELP : DT_TOC_HELP;
	displayHelpReq.app_name = AppName;
	displayHelpReq.app_title = AppTitle;
	displayHelpReq.title = help->title;
	displayHelpReq.help_dir = NULL;
	displayHelpReq.file_name = help->file;
	displayHelpReq.sect_tag = help->section;
    }
    else
	displayHelpReq.source_type = DT_OPEN_HELPDESK;

    (void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   _HELP_QUEUE (display), win, req);

	/* send a sync request */
	sreq = (DtRequest *) &syncReq;
	memset(sreq, 0, sizeof(syncReq));
	syncReq.rqtype = DT_SYNC;

	XtAddEventHandler(toplevel, (EventMask)NoEventMask, True,
		ExitWelcomeHandler, (XtPointer)NULL);

	(void)DtEnqueueRequest(XtScreen(widget), _DT_QUEUE(display),
		_DT_QUEUE(display), win, sreq);
   
}	/* End of DisplayHelp () */

void
getHelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DisplayHelp(questionBox, (HelpText *)&AppHelp);
}

void
createMsg(char *msg)
{
	Widget		err_dialog;
	Arg		arg[10];
	int		n;
	XmString	str;
	char		buf[BUF_SIZE];

	Widget			OKBtn;
	DmMnemonicInfoRec	mne_info[MI_TOTAL];
	unsigned char		*mne;
	KeySym			ks;
	XmString		string;
	int			i;	

	n=0;
	str = XmStringCreateLocalized(mygettxt(LABEL_Ok));
	XtSetArg(arg[n], XmNokLabelString, str); n++;
	XtSetArg(arg[n], XmNdialogType, XmDIALOG_ERROR); n++;
	XtSetArg(arg[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON); n++;
	err_dialog = XmCreateMessageBox(toplevel,
		"error", arg, n);
	XtUnmanageChild(XmMessageBoxGetChild(err_dialog,
		XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(err_dialog,
		XmDIALOG_CANCEL_BUTTON));
	XmStringFree(str);

	/* Need to resgister the mnemonic support on the OK button.
	 * To support the mnemonic support feature outside the libXm,
	 * a convenience routine, DmRegisterMnemonic() is written
	 * to support internal use.
	 *
	 * For detail information on this API, see the resolution
	 * described in ul94-21459.
	 */
	 
	OKBtn = XmMessageBoxGetChild(err_dialog, XmDIALOG_OK_BUTTON);
	XmSTR_N_MNE(LABEL_Ok, mnemonic_Ok, mne_info[0],
		DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
	XtVaSetValues(OKBtn, XmNmnemonic, ks, NULL);
	XmStringFree(string);	

	XtAddCallback(err_dialog, XmNokCallback,
		(void(*)())closeCB, NULL);

	mne_info[0].w = OKBtn;
	mne_info[0].cb = (XtCallbackProc)closeCB;
	
	REG_NME(err_dialog, mne_info, 1);	

	/* end of mnemonic registration */

	strcpy(buf, msg);
	str = XmStringCreateLtoR(buf, "charset");
	XtVaSetValues(err_dialog, XmNmessageString, str, NULL);
	XmStringFree(str);
	XtManageChild(err_dialog);
}

void
deleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	ret;

	/* unlink the file and exit */
	if ((ret = unlink(DTFIRSTTIMEAPP)) == 0)
		exit(0);
	else {
		/* popup the error message */
		createMsg(mygettxt(ERR_Unlink));
	}
}

void
closeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	/* just exit */
	exit(0);
}

main(argc, argv)
int	argc;
char	*argv[];
{
	char		atom[BUF_SIZE] = DTFIRSTTIME;
	Window		another_window;
	Widget		deleteBtn;
	Arg		arg[10];
	int		n, decor;
	XmString	str, str1, str2, str3;
	char		buf[BUF_SIZE];
	struct passwd	*pass;

	Widget			showBtn, closeBtn;
	DmMnemonicInfoRec	mne_info[MI_TOTAL];
	unsigned char		*mne;
	KeySym			ks;
	XmString		string;
	int			i;

	XtSetLanguageProc(NULL, NULL, NULL);

	Home = (char *)getenv("HOME");

	if ((pass = getpwuid(getuid())) == 0) 
		exit(0);

	Login = strdup(pass->pw_name);

	sprintf(buf, "%s/%s", 
		Home, Dm_DayOneName(StartupFile, Login));

	DTFIRSTTIMEAPP = strdup(buf);	
		
	I_am_owner = isOwner();

	if (!I_am_owner)  {
		/* if not an owner, remove the dtfirsttime from
		 * start_items folder and exit.
		 */
		unlink(DTFIRSTTIMEAPP);
		exit(0);
	}

	/* Initialize toolkit */
	toplevel = XtVaAppInitialize(&app, "DTFirstTime",
		NULL, 0, &argc, argv, NULL, NULL);

	DtiInitialize(toplevel);	

	XtVaSetValues(toplevel, 
		XmNtitle, mygettxt(TXT_APP), 
		NULL);

	decor = MWM_FUNC_MOVE | MWM_FUNC_CLOSE;
	XtVaSetValues(toplevel, XmNmwmFunctions, decor, NULL);


	/* main window contains a question box asking the user
	 * if they want to see instruction on how to use the system.
	 */

	n=0;
	XtSetArg(arg[n], XmNdialogType, XmDIALOG_INFORMATION); n++;

	str1 = XmStringCreateLocalized(mygettxt(LABEL_Show));	
	XtSetArg(arg[n], XmNokLabelString, str1); n++;
	
	str2 = XmStringCreateLocalized(mygettxt(LABEL_Close));
	XtSetArg(arg[n], XmNcancelLabelString, str2); n++;

	XtSetArg(arg[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON); n++;

	questionBox = XmCreateMessageBox(toplevel, "QuestionBox",
		arg, n);

	XtUnmanageChild(XmMessageBoxGetChild(questionBox, 
		XmDIALOG_HELP_BUTTON));

	showBtn = XmMessageBoxGetChild(questionBox,
		XmDIALOG_OK_BUTTON);

	closeBtn = XmMessageBoxGetChild(questionBox,
		XmDIALOG_CANCEL_BUTTON);

	str3 = XmStringCreateLocalized(mygettxt(LABEL_Delete));
	deleteBtn = XtVaCreateManagedWidget("pushButton", 
		xmPushButtonGadgetClass, questionBox,
		XmNlabelString, str3,
		NULL);
		
	XmStringFree(str1);
	XmStringFree(str2);
	XmStringFree(str3);

	XmSTR_N_MNE(LABEL_Show, mnemonic_Show, mne_info[0],
		DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
	XtVaSetValues(showBtn, XmNmnemonic, ks, NULL);
	XmStringFree(string);

	XmSTR_N_MNE(LABEL_Delete, mnemonic_Delete, mne_info[1],
		DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
	XtVaSetValues(deleteBtn, XmNmnemonic, ks, NULL);
	XmStringFree(string);

	XmSTR_N_MNE(LABEL_Close, mnemonic_Close, mne_info[2],
		DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
	XtVaSetValues(closeBtn, XmNmnemonic, ks, NULL);
	XmStringFree(string);

	XtAddCallback(questionBox, XmNokCallback, getHelpCB, NULL);
	XtAddCallback(deleteBtn, XmNactivateCallback, deleteCB, NULL);
	XtAddCallback(questionBox, XmNcancelCallback, closeCB, NULL);

	mne_info[0].w = showBtn;
	mne_info[0].cb = (XtCallbackProc)getHelpCB;
	
	mne_info[1].w = deleteBtn;
	mne_info[1].cb = (XtCallbackProc)deleteCB;

	mne_info[2].w = closeBtn;
	mne_info[2].cb = (XtCallbackProc)closeCB;

	
	REG_NME(questionBox, mne_info, 3);

	sprintf(buf, "%s%s", mygettxt(MSG2_2),
			mygettxt(MSG3));	
	str = XmStringCreateLtoR(buf, "charset");
	XtVaSetValues(questionBox, XmNmessageString, str, NULL);
	XmStringFree(str);

	XtManageChild(questionBox);
				
	XtRealizeWidget(toplevel);
	another_window = DtSetAppId(XtDisplay(toplevel), 
		XtWindow(toplevel), atom);
	if (another_window != None) {
		/* We are already running. Bring that window to the top 
		 * and die. 
		 */
		/* XMapWindow(XtDisplay(toplevel), another_window); */
		XRaiseWindow(XtDisplay(toplevel), another_window);
		XFlush(XtDisplay(toplevel));
		exit(0);
	}
	XtAppMainLoop(app);
}
