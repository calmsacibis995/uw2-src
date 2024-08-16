/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)instlsrvr:main.c	1.2"
#ident	"@(#)main.c	9.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Install_Server/main.c,v 1.1 1994/02/01 22:57:18 renu Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL Inc.                     		                              	                 */
/*	The copyright notice above does not evidence any   	             */
/*	actual or intended publication of such source code.	                */
/*
#ifndef NOIDENT
#ident	"@(#)install_server.c	1.0"
#endif
*/

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <search.h>
#include <unistd.h>
#include <signal.h>

#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/PopupWindo.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <DtI.h>

#include "main.h"


static void	ConfigureCB (Widget, XtPointer, XtPointer);
static void  	LoadCB (Widget, XtPointer, XtPointer);
static void  	SelectCB (Widget, XtPointer, XtPointer);
static void  	UnselectCB (Widget, XtPointer, XtPointer);
static void 	SetRemove ();
static int	SetToggle (ButtonItem *, int);
static void	SetSensitive (MenuItem *, int, int);
static  int 	ReadConfig (char *,int *);
static  void   	PopdownCB (Widget, XtPointer, XtPointer);

extern char   	*GetStr ();
extern void   	cancel_func ();
extern void   	Error (Widget, char *);
extern void 	SetLabels ();
extern void 	SetHelpLabels ();
extern void 	SetButtonLbls ();
extern void  	RemoveCB (Widget, XtPointer, XtPointer);
extern void	HelpCB ();
extern void	CancelCB (Widget, XtPointer, XtPointer);
extern void	VerifyCB (Widget, XtPointer, XtPointer);
extern void 	DisplayHelp ();
extern int    	PopupActionWindow (Widget);
extern void	DispatchOneEvent ();

/***************************************************************
*               Menu items and fields
***************************************************************/
static  String   MenuFields [] = {
    XtNlabel, XtNmnemonic, XtNdefault, XtNselectProc, XtNsensitive, XtNuserData,
};
static int      NumMenuFields = XtNumber (MenuFields);

/**************************help fields *********************/
static HelpText SetupHelp = {
    TXT_setupHelp, HELP_FILE, TXT_setupHelpSect,
};

/**************************button fields *********************/
static String ButtonFields [] = {
        XtNlabel, XtNmnemonic, XtNset, XtNsensitive, 
};
static int NumButtonFields = XtNumber (ButtonFields);

static ButtonItem SetItems [] = {
    {  (XtArgVal)TXT_AppServer,(XtArgVal)  MNEM_Appserver, },
    {  (XtArgVal)TXT_PersonalEdition, (XtArgVal) MNEM_Personaledition, },
  };

/***************** Lower Control Area buttons *****************/
static MenuItem CommandItems [] = {
  { (XtArgVal) TXT_configure, (XtArgVal) MNEM_configure, (XtArgVal) True, 
	(XtArgVal) ConfigureCB, },			/* Configure */
  { (XtArgVal) TXT_load, (XtArgVal) MNEM_load, (XtArgVal) False, 
	(XtArgVal) LoadCB,  },			/* Load */
  { (XtArgVal) TXT_remove, (XtArgVal) MNEM_remove, (XtArgVal) False, 
	(XtArgVal) RemoveCB, },			/* Remove */
  { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) False, 
	(XtArgVal) CancelCB, (XtArgVal) True,},/* Cancel */
  { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) False, 
	(XtArgVal) HelpCB, (XtArgVal) True, (XtArgVal) &SetupHelp, }, /* Help */
};

/* GLOBAL VARIABLES */
Atom 			  deleteWindowAtom;          /* WM_DELETE_WINDOW */
static char 	          *AppName;
static char               *AppTitle;
static Boolean	          APPSERVER = False;
static Boolean            PERSONAL_EDITION = False;
static Boolean            button_press = False;
static Boolean            both_were_false = False;

Widget	          	  action_buttons;
Widget	          	  panes;
Boolean                   PopdownOK = False;
XtAppContext		  AppContext;
Widget	          	  foot_msg;
Widget	          	  pkg_btns;

void
main (int argc, char **argv)
{
    	XEvent 		event;
  	Widget		uca, lca, header;
  	Widget	        TopLevel;
  	Widget		footer, enablew, w_pe,  caption;
	Window		owner;
  	int		index = 0, action_code;
	Boolean		i_am_owner;
	
	setlocale (LC_ALL, "");

	i_am_owner = _DtamIsOwner ("install_server");
  
  	AppTitle = GetStr (TXT_appName);
  	AppName = GetStr (TXT_appName);
 	 
  	OlToolkitInitialize (&argc, argv, NULL);
 	 
  	TopLevel = XtAppInitialize(
		&AppContext,		/* app_context_return	*/
		AppName,		/* application_class	*/
		NULL,			/* options		*/
		0,			/* num_options		*/
		&argc,			/* argc_in_out		*/
		argv,			/* argv_in_out		*/
		(String *) NULL,	/* fallback_resources	*/
		(ArgList) NULL,		/* args			*/
		(Cardinal) 0		/* num_args		*/
		);

  	/* Create control area */
  	panes = XtVaCreatePopupShell ("uca",
	  		popupWindowShellWidgetClass, TopLevel, 
			XtNtitle, 	(XtArgVal) GetStr (TXT_Title), 0);

  	XtRealizeWidget (panes);

    	/* Check if we are already running. */
    	owner = DtSetAppId(XtDisplay(panes), XtWindow(panes), "install_server");
    	if (owner != None) {
        	/* We are already running.  Bring that window to the top 
	         * and die. */
        	XRaiseWindow (XtDisplay (panes), owner);
        	XFlush (XtDisplay (panes));
        	exit (0);
    	}

    	deleteWindowAtom = XInternAtom(XtDisplay (TopLevel), 
						"WM_DELETE_WINDOW", True);

  	/************************************************************ 
	 initialize the desktop over here 
   	**********************************************************/ 
  	DtInitialize (TopLevel); 
  
  	/* Set Labels */
	APPSERVER = PERSONAL_EDITION = False;
  	SetButtonLbls (SetItems, XtNumber (SetItems));
  	action_code = SetToggle (SetItems, XtNumber (SetItems));
  	SetLabels (CommandItems, XtNumber (CommandItems));
  	SetSensitive (CommandItems, 3, action_code);
  	SetHelpLabels (&SetupHelp);
   	 
  	XtVaGetValues (panes,
  		XtNlowerControlArea,    (XtArgVal) &lca,
  		XtNupperControlArea,    (XtArgVal) &uca,
  		XtNfooterPanel,         (XtArgVal) &footer,
  		0);

	foot_msg = XtVaCreateManagedWidget ("text", staticTextWidgetClass, 
				footer, 0);
	
 	XtVaSetValues(uca,
 		XtNlayoutType,          (XtArgVal)OL_FIXEDCOLS,
                XtNcenter,              (XtArgVal)TRUE,
		0);

  	caption = XtVaCreateManagedWidget ("caption", captionWidgetClass, uca,
		XtNlabel,       (XtArgVal) GetStr (TXT_UnixWare), 0);
  
  	pkg_btns  = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
		  uca, 	
		  	XtNtraversalOn,      (XtArgVal)	True,
			XtNbuttonType,       (XtArgVal)	OL_CHECKBOX,
			XtNlayoutType,       (XtArgVal)	OL_FIXEDCOLS,
			XtNselectProc,       (XtArgVal)	SelectCB,
			XtNunselectProc,     (XtArgVal)	UnselectCB,
			XtNitemFields,	     (XtArgVal) ButtonFields,
			XtNnumItemFields,    (XtArgVal) XtNumber (ButtonFields),
			XtNitems,            (XtArgVal) SetItems,
			XtNnumItems,         (XtArgVal) XtNumber (SetItems), 
			NULL);

  	/* We want an "apply" and "reset" buttons in both the lower control
	* area and in a popup menu on the upper control area.
	*/
  	action_buttons = XtVaCreateManagedWidget ("lca",
		flatButtonsWidgetClass, lca,
		XtNtraversalOn,      	(XtArgVal)	True,
	 	XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

	if (!i_am_owner)
		for (index = 0; index < 3; index++)
			OlVaFlatSetValues (action_buttons, index,
				XtNsensitive,	(XtArgVal) False, 0);

	XtAddCallback (panes, XtNverify, VerifyCB, (XtPointer) &PopdownOK);
	XtAddCallback (panes, XtNpopdownCallback, PopdownCB, (XtPointer)0);

  	XtPopup (panes, XtGrabNone);
    	for(;;) {
        	XtAppNextEvent (AppContext, &event);
        	DispatchOneEvent (&event);
    	}

}	/* End of main () */

/*	set the toggle state of the buttons
 *	in the setup menu
 */
static int 
  SetToggle (ButtonItem *items, int cnt)
{
	int found, action_code, code;

	code = ReadConfig (CONFIG_FILE, &action_code); 
	if (action_code == LOAD) { 
		for (; --cnt >= 0; items++) 
			items->sensitive = (XtArgVal)False;
	}
	else if (action_code == ONLY_PE) {
		items->sensitive = (XtArgVal)False;
		items++;
		items->sensitive = (XtArgVal)True;
		if (code == pe_found) 
			items->set = (XtArgVal)True;
		else { 
			items->set = (XtArgVal)False;
			action_code = REMOVE;
		}
		PERSONAL_EDITION = True;
	}
	else {
		for (; --cnt >= 0; items++) {
			items->sensitive = (XtArgVal)True;
			switch (code) {
			case as_found: 
				items->set = cnt == 1 ? (XtArgVal)True :
							(XtArgVal)False;
				APPSERVER = True;
				break;
			case pe_found: 
				items->set = cnt == 0 ?	(XtArgVal)True :
							(XtArgVal)False; 
				PERSONAL_EDITION = True;
				break;
			case both_found: 
				items->set = (XtArgVal) True; 
				PERSONAL_EDITION = APPSERVER = True;
				break;
			case NULL : 
				items->set = (XtArgVal) False; 
				PERSONAL_EDITION = APPSERVER = False;
				break;
			}
		} /* for both items */
	} 
	/* in case it is not load, as/pe need to be set */
	return action_code;
}	/* End of SetToggle */

/**********************************************************************
 SelectCB
 ***********************************************************************/
static void
SelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*pflatData = (OlFlatCallData *) call_data;
	
	XtVaSetValues (foot_msg, XtNstring, 	SPACES,	0);
	button_press = True;

	/* if it is the appserver that was selected */
	if (pflatData->item_index == 0) 
		APPSERVER = True ;
	else 
		PERSONAL_EDITION = True ;

	SetRemove ();
}       /* End of selectCB  () */


/**********************************************************************
 unselectCB
 ***********************************************************************/
static void
UnselectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
        OlFlatCallData  *pflatData = (OlFlatCallData *) call_data;

	XtVaSetValues (foot_msg, XtNstring, 	SPACES,	0);
	button_press = True;

        /* if it is the appserver that was selected */
	if (pflatData->item_index == 0) 
		APPSERVER = False ;
	else 
		PERSONAL_EDITION = False ;

	SetRemove ();
}       /* End of unselectCB  () */

/**********************************************************************
 ConfigureCB
 ***********************************************************************/
static void
ConfigureCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	char 		line[BUFSIZ], *ptr, buffer[BUFSIZ];
	int		i = 0;
	FILE		*fp;

	/* clear the footer space */
	XtVaSetValues (foot_msg, XtNstring, 	SPACES,	0);

	/* if no button was clicked return */
	if (button_press == False) {
		Error (widget, GetStr (TXT_mustclick));
		return;
	}
	else
		button_press = False;

  	/* if the file cannot be opened then return NULL */
  	if ((fp = fopen (CONFIG_FILE, "r")) == NULL) {
		Error (widget, GetStr (TXT_configfile));
	 	return ;
	}
	/* get each line */
	while (fgets (line, BUFSIZ, fp)) {

		/* set the ptr to the beginning */
		ptr = line;

		/* if it is a comment bypass it */
		if (*ptr == '#') 
			ptr++;

		/* if it is app server line then....*/
		if (strncmp (ApplicationServer, ptr, 
			strlen (ApplicationServer)) == 0) {
			/*....if the appserver is false put comment*/
			if (APPSERVER == False) 
				buffer[i++] = POUND_SIGN;
		} 	/*if the line is as */

		/* if it is pe line then....*/
		else if (strncmp (PersonalEdition, ptr, 
			strlen (PersonalEdition)) == 0) {
			/*....if the pe is false put comment*/
			if (PERSONAL_EDITION == False) 
				buffer[i++] = POUND_SIGN;
		} /* if the line was pe */	

		/* till endofline store the buffer */
		while (*ptr != '\n')
			buffer[i++] = *ptr++;

		/* store newline */
		buffer[i++] = *ptr++;

	}/* while loop - get the next line */

	buffer[i] = '\0';
	fclose (fp);

  	/* if the file cannot be opened then return NULL */
  	if ((fp = fopen (CONFIG_FILE, "w+")) == NULL) {
		Error (widget, GetStr (TXT_configfile));
	 	return ;
	}
	/* we have a new buffer, so put it back */
	fputs (buffer,fp);
	fclose (fp);

	if (APPSERVER == False && PERSONAL_EDITION == False) {
		both_were_false = True;
		if ((ptr = disableInstallSAP ()) != NULL){
			Error (widget, GetStr (ptr));
	 		return ;
		}
	}
	else if (both_were_false == True) {
		both_were_false = False;
		if ((ptr = enableInstallSAP ()) != NULL) {
			Error (widget, GetStr (ptr));
	 		return ;
		}
	}

	/* set the message in the footer space */ 
	XtVaSetValues (foot_msg, 
			XtNstring, 	GetStr (TXT_configured),
			0);
			
}	/* End of ConfigureCB  () */

/**********************************************************************
LoadCB
 ***********************************************************************/
static void
LoadCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	if (PopupActionWindow (widget))
		return;
	XtUnmapWidget (panes);
}	/* End of  LoadCB () */

/**************************************************************
  read the config file 
 **************************************************************/
static 
  int ReadConfig (char *filename, int *action_code)
{
  	FILE            *fp;
  	char            buf[BUFSIZ], *ptr;
  	struct stat     stat_buf;
	int		found = 0;
	Boolean  	asfound, pefound, disabled, asonce, peonce;
	
	asfound = pefound = disabled = asonce = peonce = False;
	
  	/* if the file cannot be opened then return NULL */
  	if ((fp = fopen (filename, "r")) == NULL) {
		*action_code = LOAD;
	 	return NULL;
	}
	
  	/* check the stat on the file, read it into a buffer and get the size
	 *  close the file after the fread and set a ptr to the buffer
	 */
  	if ((stat (filename, &stat_buf)) == NULL) {
		if (stat_buf.st_size < 1) {
			*action_code = LOAD;
			return NULL;
		}
	 	fread(buf, sizeof(char), stat_buf.st_size, fp);
	 	fclose (fp);

	 	/* go thru the buffer and if either as or pe are found 	
		 * the return found */
	 	ptr = buf;
		/* while not eof */
	 	while (*ptr) {
			/* if it is a comment then disabled */
			if (*ptr == '#') {
				disabled = True; 
				ptr++;  
			}
			/* if the ptr points to as/pe  and it is 
			 * disabled then set asfound/pefound to false
			 */
			if (strncmp (ApplicationServer, ptr, 
				strlen (ApplicationServer)) == 0) {
				if (asonce == False) { 
					asonce = True;
		  			asfound = disabled == True ? False:True;
				}
			}
			else if (strncmp (PersonalEdition, ptr, 
					strlen (PersonalEdition)) == 0) {
				if (peonce == False) {
					peonce = True;
		  			pefound = disabled == True ? False:True;
				}
			}
			disabled = False;
			ptr++;
		 }
  	}
	/* if both asfound & pefound are false,they are disabled,
	 * and it means REMOVE can be activated
	 */
	if (asfound == False && pefound == False) {
		if (asonce == True && peonce == True) 
			*action_code = REMOVE;
		else if (asonce == False && peonce == True)  
			*action_code = ONLY_PE;
		else if (asonce == False && peonce == False)  
			*action_code = LOAD;
		found = NULL;
	}
	/* if both pe and as are found OR
	 * either one of them is found then CONFIGURE
	 */
	else if (asfound  == True && pefound == True)
  		found = both_found;
	else if (asfound == True && pefound == False)
		found = as_found;
	else if (asfound == False && pefound == True) {
		if (asonce == False && peonce == True) {
			*action_code = ONLY_PE;
			found = pe_found;
			return found;
		}
		else
			found = pe_found;
	}	
	if (found != NULL)
		*action_code = CONFIGURE;

	return found;
}

/**************************************************
sets the sensitivity state of the action buttons
**************************************************/
static void
SetSensitive (MenuItem *items, int cnt, int action_code)
{
  for ( ;--cnt>=0; items++)	 {
	switch (action_code){
		case LOAD:
			items->sensitive = cnt == 1 ? (XtArgVal) True :
							(XtArgVal) False;
			items->dflt = cnt == 1 ? (XtArgVal) True :
							(XtArgVal) False;
			break;
		case REMOVE:
			items->sensitive = cnt == 1 ? (XtArgVal) False :
							(XtArgVal) True;
			break;
		case ONLY_PE:
		case CONFIGURE:
			items->sensitive = cnt == 2 ? (XtArgVal) True :
						(XtArgVal) False;
			break;
	}
  }
}	/* End of SetSensitive */

/* PopdownCB
 *
 * Popdown callback.  Mark the property sheet as unposted.
 */
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	XtDestroyWidget (widget);
	exit (0);
}       
/* End of PopdownCB () */

/****************************************************
	set the remove button to sensitive if
	the appserver and pe are disabled
****************************************************/
static void
SetRemove ()
{
	if (APPSERVER == False && PERSONAL_EDITION == False) {
		OlVaFlatSetValues (action_buttons, REMOVE - 1, 
				XtNsensitive, (XtArgVal) True, 0);	
	}
	else
		OlVaFlatSetValues (action_buttons, REMOVE - 1, 
				XtNsensitive, (XtArgVal) False, 0);	
}
