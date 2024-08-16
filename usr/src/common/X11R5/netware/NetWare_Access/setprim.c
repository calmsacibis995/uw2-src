/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:setprim.c	1.4"
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nw/nwcalls.h> 
#include <nw/nwclient.h> 
#include <nct.h> 
#include <pwd.h> 
#include <sys/fcntl.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/PopupWindo.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>

#include "main.h" 
#include "scroll.h" 

/**********************************************************
	 extern declaration 
************************************************************/
extern char *           AttachError (uint32);
extern void             GUIError (Widget, char *);
extern char *           GetStr (char *);
extern void           	SetLabels (MenuItem *, int);
extern void           	SetHelpLabels (HelpText *);
extern void 		CancelCB (Widget, XtPointer, XtPointer);
extern void 		VerifyCB (Widget, XtPointer, XtPointer);
extern void 		reset_clock (Widget);
extern void 		set_clock (Widget);
extern void 		HelpCB (Widget, XtPointer, XtPointer);

/********************************************************************
	global variables
********************************************************************/
#define	SetPrimHelpSect		"107"
#define PrimaryServerFile	".NWprimary"

/********************************************************************
	statically declared routines
********************************************************************/
static void 		ApplyCB (Widget, XtPointer, XtPointer);
static void 		PopdownCB (Widget, XtPointer, XtPointer);
static int		SetPrimaryServer (char *, char *);
static int		GetPrimaryServer (char *, char *);

/****************************************************************** 
	Menu items and fields 
*****************************************************************/
static String		SetPrimFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};
static int 		NumSetPrimFields = XtNumber (SetPrimFields);

/****************************************************************** 
	Help variables
*****************************************************************/
static HelpText SetPrimHelp = {
    TXT_SetPrimHelp, SetPrimHelpSect,
};

/*************************************************************
 	Lower Control Area buttons 
*************************************************************/
static MenuItem SetPrimCommandItems [] = {
    { (XtArgVal) TXT_save, (XtArgVal) MNEM_save, (XtArgVal) True,
	  (XtArgVal) ApplyCB, (XtArgVal) True, },	/* Save */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal)False,(XtArgVal)&SetPrimHelp, }, /*help*/
};

/********************************************************************
	statically declared variables
********************************************************************/
static char		NWprimary[1024]; /* Primary server file */
static Widget		foot, setprim_widget;

/******************************************************************
	setprim procedure - call back to popup set primary gui
*****************************************************************/
void
SetPrimaryServerCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	static Boolean  	once = False;
    	Widget   		setprimary_popup, lcaMenu, lca, uca;
    	Widget          	footer, caption;
	char			primaryServer[NWC_MAX_SERVER_NAME_LEN],
				errorbuf[1024];
	
	/* Set up the labels for the first time and store the
	 * primary server file name
	 */
	if (!once) {

		struct passwd	*pwd;
		uid_t		uid;

		once = True;
    		SetLabels (SetPrimCommandItems, XtNumber(SetPrimCommandItems));
    		SetHelpLabels (&SetPrimHelp);
		/*
	 	 * Build the path to the user's .NWprimary file.
		 * If root then do not append slash after pw_dir
	 	 */
		uid = geteuid ();	
		pwd = getpwuid (uid);
		if (strlen (pwd->pw_dir) > 1)
			(void) sprintf (NWprimary, "%s/%s", pwd->pw_dir, 
					PrimaryServerFile);
		else 
			(void) sprintf (NWprimary, "%s%s", pwd->pw_dir, 
					PrimaryServerFile);
     	}

    	/* Create setprim popup panel */
    	setprimary_popup = XtVaCreatePopupShell ("setprim",
				popupWindowShellWidgetClass, widget,
				XtNtitle,	(XtArgVal) GetStr 
						(TXT_setprimaryserver),
				0);

	/* Get the upper and lower control area
	 */
    	XtVaGetValues (setprimary_popup,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		XtNfooterPanel,		(XtArgVal) &footer,
		0);

	/* Create the caption area to include the label
	 * and the text field
	 */
    	caption = XtVaCreateManagedWidget ("setprimtext",
			captionWidgetClass, uca,
			XtNlabel, (XtArgVal)GetStr(TXT_primservername),
			0);

	/* Get the primary server.  Pass the primaryServer buffer
	 * and a buffer for error message if any 
	 */
	if ((GetPrimaryServer (primaryServer, errorbuf)))  {
		GUIError (widget,errorbuf );
		return;
	}
		
	/* Store server string name in the text field
	 * to be displayed
	 */	 
    	setprim_widget = XtVaCreateManagedWidget ("setprimfield", 
			textFieldWidgetClass, caption,
 			XtNcharsVisible, (XtArgVal)20,
			XtNstring,  	(XtArgVal) primaryServer,
			0);

	/* create the footer to display the error messages 
	 */
    	foot = XtVaCreateManagedWidget ("footermsg",
					staticTextWidgetClass, footer, 0);

    	/* We want an "apply", "cancel" & "help" button in lower control area 
	 */
    	lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNitemFields,		(XtArgVal) SetPrimFields,
		XtNnumItemFields,	(XtArgVal) NumSetPrimFields,
		XtNitems,		(XtArgVal) SetPrimCommandItems,
		XtNnumItems,		(XtArgVal)XtNumber(SetPrimCommandItems),
		0);

    	/* Add callbacks to verify and destroy all widget when the panel
     	 * goes away
     	 */
     	XtAddCallback (setprimary_popup, XtNverify, VerifyCB, (XtPointer) 
			&PopdownOK);
    	XtAddCallback (setprimary_popup, XtNpopdownCallback, PopdownCB, 0);
    	XtPopup (setprimary_popup, XtGrabNone);

}	/* End of setprim */

/*************************************************************************** 
 * ApplyCB
 * apply callback.  
 ***************************************************************************/
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	Widget		shell = XtParent (widget);
	char		*serverptr, errormsg[1024];
	int		retcode;

	/* Getting primary server from the text field
	 */
	serverptr = OlTextFieldGetString (setprim_widget, NULL);

	/* If nothing was entered then - error
	 */
	if (strlen (serverptr) < 1) {
		XtVaSetValues (foot, XtNstring, (XtArgVal) 
						GetStr(TXT_enterprimserver), 0);
		return;
	}
	else if (strlen (serverptr) > NWC_MAX_SERVER_NAME_LEN) {
		XtVaSetValues (foot, XtNstring, (XtArgVal) 
				GetStr(TXT_psserverlength), 0);
		return;
	}
		
	/* Set the primary server.  If it failed then display errormsg
	 * in the footer, else popdown
	 */
	set_clock (widget);
	retcode = SetPrimaryServer (serverptr, errormsg);
	reset_clock (widget);
	if (!retcode) {
		while (!XtIsShell (shell))
			shell = XtParent(shell);
    		XtDestroyWidget (shell);
	}
	else 
		XtVaSetValues (foot, XtNstring, (XtArgVal) errormsg, 0);

} /* End of ApplyCB () */

/*****************************************************************
 * PopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 *****************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	XtDestroyWidget (widget);
}	/* End of PopdownCB () */


/*****************************************************************
 * Set the primary server here.  
 *****************************************************************/
static int
SetPrimaryServer(char *pServer, char *errormsg)
{
	NWCCODE		ccode;
	NWCONN_HANDLE	connID;
	int		fd, i;
	size_t		len;
	char		primaryServer[NWC_MAX_SERVER_NAME_LEN];

	/*
	 * Setting the Primary NetWare Server. Make sure the requester
	 * is initialized.
	 */
#ifdef DEBUG
	printf ("initializing \n");
#endif
	ccode = NWCallsInit (NULL, NULL);
	if (ccode) {
		sprintf (errormsg, "%s", GetStr (TXT_initfail));
		return 1;
	}

	/*
	 * Convert the primary server name to upper case.
	 */
	strcpy (primaryServer, pServer);
	for (i = 0; primaryServer[i] != '\0'; i++)
	if (islower (primaryServer[i]))
		primaryServer[i] = toupper (primaryServer[i]);

	/*
	 * Attaching to the primary NetWare server.
	 */
#ifdef DEBUG
	printf ("attaching \n");
#endif
	ccode = NWAttach (primaryServer, &connID, 0);
	if (ccode && ccode != NWERR_ALREADY_ATTACHED) {
		strcpy (errormsg, AttachError (ccode));
		return 1;
	}

	/*
	 * Setting the primary NetWare server.
	 */
#ifdef DEBUG
	printf ("setting primary connection \n");
#endif
	ccode = NWSetPrimaryConn (connID);
	if (ccode) {
		sprintf (errormsg, "%s", GetStr (TXT_setprimconnfail));
		return 1;
	}

	/*
	 * Also need to create ".NWprimary in user's home directory to
	 * save the primary server name.
	 */
#ifdef DEBUG
	printf ("opening primary file\n");
#endif
	if ((fd = open (NWprimary, O_WRONLY | O_CREAT | O_TRUNC, 
				(mode_t)0644)) == -1) {
		/* 
		 * Could not open the primary server file.
		 */
		sprintf (errormsg, "%s %s", GetStr (TXT_psopenfail), 
			NWprimary);
		return 1;
	}

	len = strlen (primaryServer);
#ifdef DEBUG
	printf ("writing primary server %s %d\n", primaryServer, len);
#endif
	if ((write (fd, primaryServer, len)) != len) {
		/*
	 	 * Could not write to the primary server file.
		 */
#ifdef DEBUG
		printf ("cannot write pirmary file\n");
#endif
		sprintf (errormsg, "%s %s", GetStr (TXT_pswritefail),NWprimary);
		close(fd);
		return 1;
	}
	else {
		/* Wrote the file so close fd and return
		 */
#ifdef DEBUG
		printf ("writing primary file %s\n", NWprimary);
#endif
		close (fd);
		return 0;
	}
}

/*****************************************************************
 * Get the primary server here.  
 *****************************************************************/
static int
GetPrimaryServer(char *primaryServer, char *errormsg)
{
	NWCCODE		ccode;
	NWCONN_HANDLE	connID;
	int		n, fd;

#ifdef DEBUG
	printf ("getting ps from file %s\n", NWprimary);
#endif
	/*
	 * Check to see if ".NWprimary" file exists in the user's home
	 * directory.
	 */
	if ((fd = open (NWprimary, O_RDONLY, 0)) != -1) {
		/*
		 * Get the primary server name from the .NWprimary file
		 * in the user's home directory.
		 */
		if ((n = read (fd, primaryServer, NWC_MAX_SERVER_NAME_LEN)) 
				!= -1) { 
			primaryServer[n] = '\0';
#ifdef DEBUG
			printf ("primary server len %s %d\n", primaryServer, 
				strlen (primaryServer));
#endif
			close (fd);
			return 0;
		}
		else {
			/*
			 * Could not read the primary server file.
			 */
			sprintf (errormsg, "%s %s", GetStr (TXT_psreadfail), 
				NWprimary);
			close (fd);
			return 1;
		}
	}
	else {
		strcpy (primaryServer, "");
#ifdef DEBUG
		printf ("returning ps\n");
#endif
		return 0;
	}
}

#if 0
		/*
		 * Make sure the requester is initialized.
		 */
		ccode = NWCallsInit (NULL, NULL);
		if (ccode) {
			sprintf (errormsg, "%s", GetStr (TXT_initfail));
			return 1;
		}

		/*
		 * Getting the primary NetWare server using the APIs.  Get the
		 * primary server's connection ID.
		 */
		ccode = NWGetPrimaryConnID (&connID);
		if (ccode) {
			sprintf (errormsg, "%s", GetStr (TXT_getprimconnfail));
			return 1;
		}

		ccode = NWGetServerNameByConnID (connID, primaryServer);
		if (ccode) {
			sprintf (errormsg, "%s", GetStr (TXT_getserverfail));
			return 1;
		}
#endif
