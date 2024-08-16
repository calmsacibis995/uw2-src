/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)instlsrvr:action.c	1.2"
#ident	"@(#)action.c	9.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Install_Server/action.c,v 1.1 1994/02/01 22:57:11 renu Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#ident	"@(#)install_server:action.c	1.0"
#endif
*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ulimit.h>
#include <dirent.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/MenuShell.h>
#include <Xol/PopupWindo.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/OlCursors.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>

#include <Dt/Desktop.h>
#include <DtI.h>
#include <FIconBox.h>

#include "main.h"
	 
/****************************************************************
  forward declation of  functions 
******************************************************************/
static void 	CancelCB (Widget, XtPointer,  XtPointer);
static void	ApplyCB (Widget, XtPointer,  XtPointer);
static void	selectCB (Widget, XtPointer,  XtPointer);
static void	unselectCB (Widget, XtPointer,  XtPointer);
static void	PopdownCB (Widget, XtPointer, XtPointer);
static void	reset_clock (Widget);
static void	set_clock (Widget);
static int	configure_file (Widget, char *);
static int	check_directory (Widget, char *);
static void 	GetFileInput (XtPointer client_data, int * fid, XtInputId * id);
static void 	PipeError (XtPointer, int *, XtInputId *);
static void 	pipe_load_tape (Widget);
static void 	pipe_errmsg ();
static void 	kill_pid ();

int		pipe_command (Widget, char *,char *);
void		RemoveCB (Widget, XtPointer, XtPointer);
extern void	VerifyCB (Widget, XtPointer, XtPointer);
extern void	HelpCB (Widget, XtPointer, XtPointer);
extern void	SetHelpLabels (HelpText *);
extern void  	Error (Widget, char *);
extern char	*GetStr (char *idstr);
extern void	SetLabels (MenuItem *,int );
extern void	SetButtonLbls (ButtonItem *,int );
extern void	DisplayHelp (Widget , HelpText *);

/***************************************************************
* 		Menu items and fields 
***************************************************************/
static  String   MenuFields [] = {
    XtNlabel, XtNmnemonic, XtNdefault, XtNselectProc, XtNsensitive,XtNuserData,
};
static int      NumMenuFields = XtNumber (MenuFields);

/**************************button fields *********************/
static String ButtonFields [] = {
        XtNlabel, XtNmnemonic, XtNset, XtNsensitive,
};
static int NumButtonFields = XtNumber (ButtonFields);

/**************************help fields *********************/
static HelpText SetupHelp = {
    TXT_setupHelp, HELP_FILE, TXT_setupHelpSect,
};

/*******************************************************************
	MENU ITEMS IN THE lower control area
***********************************************************************/
static MenuItem ActionItems [] = {
    { (XtArgVal) TXT_load, (XtArgVal) MNEM_load, (XtArgVal) True, 
	(XtArgVal)  ApplyCB, (XtArgVal) True, },        /* Apply */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel,   (XtArgVal) False,
	(XtArgVal)  CancelCB, (XtArgVal) True, },	/* Exit */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help,   (XtArgVal) False,
	(XtArgVal) HelpCB, (XtArgVal) True, (XtArgVal) &SetupHelp },  /* Help */
};

static ButtonItem ExecItems [] = {
    { (XtArgVal) TXT_LoadFromTape, (XtArgVal) MNEM_LoadfromTape,  0,
		(XtArgVal) True}, 
    { (XtArgVal) TXT_LoadFromCDROM, (XtArgVal) MNEM_LoadfromCDROM, 0,
		(XtArgVal) True},
};

/* extern and static variables used in action.c */
extern Atom		deleteWindowAtom;          /* WM_DELETE_WINDOW */

extern Boolean 		PopdownOK;
extern Widget  		panes ;
extern Widget  		foot_msg ;
extern XtAppContext 	AppContext;

static Boolean  	TAPE = True;
static Boolean  	CDROM = False;
static Widget  		path ;
static Widget  		popup , message, toname;
static char		workfile[BUFSIZ];
static char		*command;
static XtInputId 	read_id, error_id;
static FILE		*ppipe;

int PopupActionWindow (Widget widget)
{
 	Widget          button, caption, w_set;
	Widget	   	ActionMenu;
	Widget	    	lca, uca, foot, footer;
	Cardinal	numActionItems;
	register    	i;
	int		code;
	char         	*title_header;
	static Boolean  first = False;
	DIR		*dirp;

	if (popup) 
		XRaiseWindow (XtDisplay (popup), XtWindow (popup));
	else {
		/* check the directory to see if it is there */
		if ((dirp = opendir (GetStr (TXT_path))) == NULL) {
			/* create the net_install directory if not there */
			if (mkdir (GetStr (TXT_path), S_IRUSR | S_IWUSR | 
				S_IXUSR |  S_IRGRP | S_IXGRP | S_IROTH | 
				S_IXOTH) == -1){
				Error (widget, GetStr (TXT_makefailed));
				return 1;
			}
		}
		/******************************************************
		  create the container to hold the others 
		******************************************************/ 
		popup =  XtVaCreatePopupShell ("popup", 
				popupWindowShellWidgetClass, widget, 0);
	
		/*****************************************************
	  		create the title header
   		******************************************************/ 
		title_header = (char *) XtMalloc 
					(strlen (GetStr(TXT_ActionMenu))+2);
		strcpy (title_header, GetStr (TXT_ActionMenu));
		XtVaSetValues (popup, XtNtitle,(XtArgVal) title_header, 0);
		XtFree (title_header);
	
		XtVaGetValues (popup, 
				XtNlowerControlArea,  	(XtArgVal) &lca,
				XtNupperControlArea,    (XtArgVal) &uca,
				XtNfooterPanel,    	(XtArgVal) &footer,
				0);


		if (first == False) {
	  		first  = True;
	  		SetLabels (ActionItems, XtNumber (ActionItems)); 
	  		SetButtonLbls (ExecItems, XtNumber (ExecItems)); 
	  		SetHelpLabels (&SetupHelp);
		}
		numActionItems = XtNumber (ActionItems);
			
		caption = XtVaCreateManagedWidget ("actiontype", 
				captionWidgetClass, uca,
	  			XtNlabel,    
				(XtArgVal)GetStr(TXT_ActionType), NULL);
	
		w_set = XtVaCreateManagedWidget("checkbox", 
			flatButtonsWidgetClass, 
			caption, 
			XtNtraversalOn,      (XtArgVal)True, 
			XtNexclusives,       (XtArgVal)True, 
			XtNselectProc,       (XtArgVal)selectCB, 
			XtNunselectProc,     (XtArgVal)unselectCB, 
			XtNbuttonType,       (XtArgVal)OL_CHECKBOX, 
			XtNlabelJustify,     (XtArgVal)OL_LEFT, 
			XtNlayoutType,       (XtArgVal)OL_FIXEDCOLS, 
			XtNitemFields,       (XtArgVal)ButtonFields,
		  	XtNnumItemFields,    (XtArgVal)NumButtonFields, 
			XtNitems,            (XtArgVal)ExecItems, 
			XtNnumItems,         (XtArgVal)XtNumber (ExecItems), 
			NULL);

		toname = XtVaCreateManagedWidget ("path", 
			captionWidgetClass, uca, 
			XtNlabel,        (XtArgVal)GetStr(TXT_Pathname), 
			NULL);

		path = XtVaCreateManagedWidget ("pathname", 
			textFieldWidgetClass,  toname, 
			XtNcharsVisible,        (XtArgVal) 25, 
			XtNstring,        	(XtArgVal) GetStr (TXT_path), 
			0);

		message  = XtVaCreateManagedWidget ("footer",
                		staticTextWidgetClass, footer, 0);
	
		ActionMenu = XtVaCreateManagedWidget ("actionMenu", 
			flatButtonsWidgetClass,  lca, 
			XtNtraversalOn,      	(XtArgVal)True, 
			XtNitemFields,		(XtArgVal) MenuFields, 
			XtNnumItemFields,	(XtArgVal) NumMenuFields, 
			XtNitems,		(XtArgVal) ActionItems, 
			XtNnumItems,		(XtArgVal) numActionItems, 
			0);
	
		XtAddCallback (popup, XtNverify, VerifyCB, (XtPointer) 
							&PopdownOK);
		XtAddCallback (popup, XtNpopdownCallback, PopdownCB, NULL);
	
		XtPopup (popup, XtGrabNone);
	}
	return 0;
} /* End of main () */

/********************************************************************* 
 *  check disk space
 *********************************************************************/
static int
check_dskspace (char *pathname)
{
	struct statvfs  	buf;
	int			retvalue;

	/* check for disk space */
	if (statvfs (pathname, &buf) == 0) {
		/* if the # of free blocks are greater that 60 mb */
		if (buf.f_bfree * buf.f_frsize >=  DISK_SPACE_REQD * BLOCK_SIZ){
			/* set ulimit to 100000 */
			if (ulimit (UL_SETFSIZE, DISK_SPACE_REQD + 1) != -1)
				return OK;
		}	/* if the no. of blocks are avail */
	}/* if the file system could be stated */
	return NULL;
}

/********************************************************************* 
 * selectCB
 *********************************************************************/
static void
selectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*p	=  (OlFlatCallData *) call_data;

	if (p->item_index == 0) {
		TAPE = True;
		CDROM = False;
		XtVaSetValues (path, XtNstring, GetStr (TXT_path), 0);
		XtVaSetValues (toname, XtNlabel, GetStr (TXT_Pathname), 0);
	}
	else { 
		CDROM = True;
		TAPE = False;
		XtVaSetValues (path, XtNstring, "", 0);
		XtVaSetValues (toname, XtNlabel, GetStr (TXT_On), 0);
	}
} /* selectCB */

/********************************************************************* 
 * unselectCB
 *********************************************************************/
static void
unselectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*p	=  (OlFlatCallData *) call_data;
	
	if (p->item_index == 0){ 
		TAPE = False;
		CDROM = True;
		XtVaSetValues (path, XtNstring, "", 0);
		XtVaSetValues (toname, XtNlabel, GetStr (TXT_On), 0);
	}
	else { 
		CDROM = False;
		TAPE = True;
		XtVaSetValues (path, XtNstring, GetStr (TXT_path), 0);
		XtVaSetValues (toname, XtNlabel, GetStr (TXT_Pathname), 0);
	}
} /* unselectCB */

/********************************************************************* 
 * ApplyCB
 * if load from cd rom - check to see if the pathname is a mounted file system
 * and if it is then, dont dd the file in but do a pkginfo on unixware on the
 * mounted cd rom and then put that path name into /etc/installd.conf file.
 * If it is load from tape - check for disk space, ulimit and then dd file
 * into the pathname, and also create /etc/installd.conf with the pathname
 * In either case - enable the install server before exitting this routine.
 *********************************************************************/
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	char 		*ptr, *pathname;
	struct stat	buf;
	Widget 		shell;
	char 		tape_buf[256];

	/* get the text field */
	pathname = OlTextFieldGetString (path, NULL);

	/* check to see if path is ok */
	if (check_directory(widget, pathname)) {
        	if (OlCanAcceptFocus(path, CurrentTime)) 
               	 	OlSetInputFocus(path, RevertToNone, CurrentTime);
		return ;
	}

	/* save the pathname */
	strcpy (workfile,pathname);
	strcat (workfile, PACKAGE);	

	if (TAPE) {
		/* add null terminator to the tape file name */
		ptr = XtMalloc (strlen (TAPE_FILE) + 1);
		strcpy(ptr, TAPE_FILE);
		if ((stat (ptr, &buf)) != 0) {
			Error (widget, GetStr (TXT_notape));
			return;
		}
		XtFree (ptr);
			
		/* create space for the command */
		command = XtMalloc (strlen (DD_COMMAND) + strlen (TAPE_FILE) 
			+ strlen (INPUT_FILE) + strlen (OUTPUT_FILE) 
			+ strlen (workfile) + strlen(BLOCK_SIZE) + 10);
		strcpy (command, DD_COMMAND);
		strcat (command, INPUT_FILE);
		strcat (command, TAPE_FILE);
		strcat (command, OUTPUT_FILE);
		strcat (command, workfile);
		strcat (command, BLOCK_SIZE);

		/* setup a msg saying that the tape is being loaded */
		XtVaSetValues (message, XtNstring, GetStr (TXT_loadingtape), 0);

		/* check to see if the tape device can be opened */
		strcpy (tape_buf, "<");
		strcat (tape_buf, TAPE_FILE);
		if (system  (tape_buf) != 0) {
			Error (widget, GetStr (TXT_failedtape));
			XtVaSetValues (	message, XtNstring, NULL, 0);
		}
		else
			/* setting of the command */
			pipe_load_tape (widget);
	}	/* LOAD FROM TAPE */
	else { 
		/*****************************
			CD ROM PATH
		 *****************************/ 
		set_clock (widget);

		/* configure the /etc/installd.conf file based
		 * on pkginfo from the unixware file on the 
		 * cd rom mounted file system
		 */
		if ((configure_file (widget, workfile)) != 0)  {
			if ((ptr = enableInstallSAP ()) == NULL){
				XtVaSetValues (message, 
						XtNstring, GetStr (TXT_loadCD),
						0);
			}
			else {
				Error (widget, GetStr (ptr));
				XtVaSetValues (	message, XtNstring, NULL, 0);
			}
		}

		reset_clock (widget);
	} /* LOAD FROM CD */
}

/***********************************************************************
		check the pathname of the enter field
**********************************************************************/
static int
check_directory (Widget widget, char *pathname) 
{
	struct statvfs  	buf;
	DIR			*dirp;
	char 			*cdpath;
	int			retvalue;

	/* pathname is null */
	if (strlen (pathname) < 1) {
		Error (widget, GetStr (TXT_enterfile));
		return 1;
	}
	/* make sure it is a dir not file */
	if ((dirp = opendir (pathname)) == NULL) {
		Error (widget, GetStr (TXT_mustbedir));
		return 1;
	}
	else {
		/* if directory then check for disk space */
		closedir (dirp);
		if (TAPE) {
			if (check_dskspace (pathname) == NULL) {
				Error (widget, GetStr (TXT_nospace));
				return 1;
			}
			else if (statvfs (pathname, &buf) != 0) {
				Error (widget, GetStr (TXT_invalidfs));
				return 1;
			}
			else 
				return 0;
		} 	/* if it is a TAPE */
		/* check for valid file package on the mounted fs */
		else if (CDROM) {
			cdpath = XtMalloc (strlen (pathname) + 
					strlen (PACKAGE) + 2);
			strcpy (cdpath, pathname);
			strcat (cdpath, PACKAGE);
			if (statvfs (cdpath, &buf) != 0) {
				Error (widget, GetStr (TXT_invalidcdpath));
				retvalue =  1;
			}
			else 
				retvalue =  0;
			XtFree (cdpath);
			return retvalue;
		} 	/* if it is a CD ROM */
	} 	/* if the dir could be opened */
}

/**********************************************************************
			set off the command thru a popen
**********************************************************************/
int 
pipe_command (Widget w, char *cmd, char *file1)
{
	FILE 		*pp;
	char 		cmd_buffer[BUFSIZ];
	
	strcpy (cmd_buffer, cmd);
	strcat (cmd_buffer, file1);
	if ((pp = popen (cmd_buffer, "r")) == NULL) {
		Error (w, GetStr (TXT_sorry));
		return 1;
	}	
	if (pclose (pp) == -1) {
		Error (w, GetStr (TXT_errpipe));
		return 1;
	}
	return 0;
}

/***************************************************
	configure the installd.conf file  in /etc 
****************************************************/
static int
configure_file (Widget w, char *pathname)
{
	FILE 		*pp, *fp;
	char 		*token, line_buf[BUFSIZ], buf[BUFSIZ], line[BUFSIZ];

	/* initialize the buffer */
	buf[0] = NULL;

	/* set the pkginfo command into a buffer */
	strcpy (line_buf, PKG_COMMAND);
	strcat (line_buf, pathname);

	/* set the command thru pipe */
	if ((pp = popen (line_buf, "r")) != NULL) {

		/* retrieve the line */
		while (fgets (line, BUFSIZ, pp)) {
			/* strtok for white spaces */
			token = strtok (line, WHITESPACE);
			if (token) {
				/* second token is set name */
				token = strtok (NULL, WHITESPACE);
				if (token) {
					/* if second token exists 
					 * create the buffer */
					if (strlen (buf) > 0)
						strcat (buf, token);
					else
						strcpy (buf, token);
					strcat (buf, SPACES);
					strcat (buf, pathname);
					strcat (buf, SPACES);
					strcat (buf, ONE);
					strcat (buf, SPACES);
					/* third token is description */
					while (token = strtok(NULL, WHITESPACE))
					{
						strcat (buf, ONE_SPACE);
						strcat (buf, token);
					}
				}/* second token*/
			}/*first token*/
		}	/* while loop for each line */
	}	/* if popen succeeded */	
	else {
		/* if the popen failed then return */
		Error (w, GetStr (TXT_sorry));
		return 0;
	}	

	/* close the pipe here */
	if (pclose (pp) == -1) {
		Error (w, GetStr (TXT_errpipe));
		return 0;
	}

	/* write into the config file */
	if (strlen (buf) > 0) {
		buf[strlen(buf)] = '\0';
		if ((fp = fopen (CONFIG_FILE, "w+")) == NULL) {
			Error (w, GetStr (TXT_fileopen));
			return 0;
		}
		else  {	
			fputs (buf,fp);
			fclose (fp);
			return 1;
		}
	}	/* if there is a buffer create the config file */
	else  {
		Error (w, GetStr (TXT_failedconfig));
		return 0;
	}
}

/***************************************************
 * PopdownCB
 * Popdown callback.
****************************************************/
void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	XtDestroyWidget (widget);
	popup = (Widget)NULL;
	if (panes) {
		XtDestroyWidget (panes);
		exit (1);
	}
}       
/* End of PopdownCB () */

/***************************************************
		set the clock 
****************************************************/
static void
set_clock (Widget w)
{
	Widget 	shell = XtParent (w); 

	while  (!XtIsShell (shell)) 
		shell = XtParent (shell);		

	/* set the clock */
	XDefineCursor ( XtDisplay (shell), XtWindow (shell), 
       			GetOlBusyCursor (XtScreen (shell)));
	XSync (XtDisplay (shell), 0);
}

/***************************************************
		reset the clock 
****************************************************/
static void
reset_clock (Widget w)
{
	Widget 	shell = XtParent (w); 

	while  (!XtIsShell (shell)) 
		shell = XtParent (shell);		
	
	/* reset the clock */
	XDefineCursor ( XtDisplay (shell), XtWindow (shell), 
			GetOlStandardCursor (XtScreen (shell)));
	XSync (XtDisplay (shell), 0);
}

/********************************************************************* 
 * RemoveCB
 *********************************************************************/
void
RemoveCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	XtVaSetValues (foot_msg, XtNstring, SPACES, 0);
	QuestionDialog (widget, GetStr (TXT_removeq));
}

/********************************************************************* 
 * 	Load the tape thru the pipe command 
 *********************************************************************/
static 
void pipe_load_tape (Widget w)
{
	/* set the clock */
	set_clock (w);

	/* set off the dd thru a popen command */
	if ((ppipe = popen(command, "r")) == NULL)  { 
		/*  Couldn't open the pipe error message*/
		Error (popup, GetStr (TXT_errpipe));
		return;
	}
		
	/* register a function to get IO errors  */
	error_id =XtAppAddInput(AppContext, fileno(ppipe), 
				(XtPointer)XtInputExceptMask, PipeError, 
				(XtPointer)w);

	/* register a function to get the stdout of the pipe. */
	read_id = XtAppAddInput(AppContext, fileno(ppipe), 
				(XtPointer)XtInputReadMask, GetFileInput, 
				(XtPointer)w);
}

/********************************************************************* 
 * 	get the file input from the open pipe	
 *********************************************************************/
static void
GetFileInput (XtPointer client_data, int *fid, XtInputId * id)
{
	char 		*ptr, buf[256];
	int 		nbytes, success = 0, i;
	struct stat 	file_buf;
	Widget  w = 	(Widget) client_data;

	/* read the bytes from the add input */
	nbytes = read(*fid, buf, 256);

	/* if the io had failed */
	if (nbytes == -1)  {
		pipe_errmsg ();
		Error (popup, GetStr (TXT_failedexec));
	}
	/* nothing else to be read */
	else   {
		/* check if the unixware is not there, if
		 * it is there check to see if it is less than 50 mb
		 * if it is - then that is a problem
		 */
		if (stat (workfile, &file_buf) != 0 || 
			 	(file_buf.st_size < 50000000))  {
			pipe_errmsg (); 
			Error (popup, GetStr (TXT_failedexec));
		}	
		else {
			/* if the file is there, create /etc/installd.conf,
			 * configure the file and enable sapd 
			 */
			if ((configure_file (w, workfile)) != 0)  {
				if ((ptr = enableInstallSAP()) == NULL)
					success = 1;
				else 
					Error (popup, GetStr (ptr));
			}
			else { 
				/* if configure failed remove unixware
				 * - if it is found
				 */
				if (access (workfile, F_OK) == 0)
					pipe_command (w, RM_COMMAND, workfile);
			}
			/* close pipes */
			pipe_errmsg ();
		}
		/* display msg based on success or failure */
		if (success)
			XtVaSetValues (message, XtNstring,GetStr(TXT_loaded),0);
		else
			XtVaSetValues (message, XtNstring, NULL, 0);
	}

	/* free the command space */
	XtFree (command);

	/* reset the clock */
	reset_clock (w);

}  /* end of GetFileInput() */

/********************************************************************* 
 * 	get input errors, if any, from the open pipe	
 *********************************************************************/
static void
PipeError (XtPointer client_data, int  *fid, XtInputId * id)
{
	Widget  w = (Widget) client_data;

	/* reset the clock */
	reset_clock (w);

	/* close the pipes */
	if (ppipe) 
		pipe_errmsg ();

	/* set up the error message */
	Error (popup, GetStr (TXT_failedexec));
}  /* end of PipeError() */

/********************************************************************* 
		cancel the popup and close the pipes
 *********************************************************************/
static void
CancelCB (Widget w, XtPointer  client_data, XtPointer  call_data)
{
	/* kill the dd process if it is there */	
	(void) kill_pid ();

	/* popdown the window */
  	XtPopdown (popup);
}  /* end of CancelCB() */

/********************************************************************* 
		close the pipe and the ids
 *********************************************************************/
static void
pipe_errmsg ()
{
	/*  end of file */
	pclose(ppipe);
	ppipe = NULL;
	XtRemoveInput(read_id);
	XtRemoveInput(error_id);
}

/********************************************************************* 
	kill pid routine - kills child process if it is found
 *********************************************************************/
static 
void kill_pid ()
{
	pid_t killpid, pid;

	/* get the local pid */
	pid = getpid ();

	/* get the pid of the dd command  and kill it*/
	if  ((killpid = ps (pid)) != 0) 
		kill (killpid, SIGKILL);
}

/********************************************************************* 
		close the window  -  make sure pids if any are killed
 *********************************************************************/
void DispatchOneEvent (event)
XEvent *event;
{
    	switch (event->type) {
        	case ClientMessage:
			/* if window is closed thru the window manager 
			 * make sure children are killed if any 
			 */
            		if (event->xclient.data.l[0] == deleteWindowAtom) {
				(void) kill_pid ();
				/* popdown the window */
				if (popup)
  					XtPopdown (popup);
				else 
					XtPopdown (panes);
           		} 
           		break;
	}
  	XtDispatchEvent(event);
}
