/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:authen.c	1.14"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Auto_Authenticator/authen.c,v 1.26 1994/09/06 19:17:57 plc Exp $"
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#ident	"xauto:authen.c	1.0"
#endif
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <Xm/MessageB.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/Xm.h>
#include <Dt/Desktop.h>
#include <nw/nwclient.h>
#include <nw/nwconnec.h>
#include <nw/nwerror.h>
#include <nw/nwserver.h>

#include "main.h" 
#include "nct.h" 


/* forward declaration */

/**************************************************************
	 forward declaration 
*************************************************************/
extern void clearPassword(void);
extern void GuiError (Widget, char *,int);
extern char *GetStr (char *);
static int authenticate(char *serverName, char *userName, char *password);

static void getReply (Widget ,XtPointer , XEvent *, Boolean );
static void TimerProc (XtPointer, XtIntervalId *);
static void WaitForExclusiveUse (Widget shell);

extern char * getUserName();
extern char * getPassword();

static char		errmsg[256];

Atom exclusiveAtom;
Atom folderAtom;
Atom handShakeAtom;
static int fromPanel = False;

void sendSyncFolderReq(Widget shell);
void sendOpenFolderReq(Widget shell);
void sendCloseFolderReq(Widget shell);
int isUserAuthenticated(char *serverName, uid_t uid);

/*************************************************************************** 
 * apply_func
 * apply callback.  
 ***************************************************************************/
void
apply_func (Widget form, Widget widget, char *server)
{
	char        	*username, *passwd;
	int         	code; 
	Cursor			cursor;
	Widget			shell;
	DtRequest request;
	extern int sendDeskTopOpenReq;


  	/***********CREATE THE WATCH *********************/
	cursor = XCreateFontCursor(XtDisplay(form), XC_watch);
    for (shell = form; !XtIsShell(shell); shell = XtParent(shell));
	XDefineCursor(XtDisplay(shell), XtWindow(shell), cursor);
  	XSync (XtDisplay (form), 0);

	/*  
	 * get the user name from the text field and the password 
	 * from private storage
	 */
	username = getUserName();
	passwd = getPassword();
	clearPassword();

	/* if passwd does not exist then try with NULL*/
	if (strlen (passwd) < 1)
		passwd = NULL;

	/* if the login name is not entered return error */
	if (strlen (username) < 1) 
	{
		/*
		 * Display error popup for the user to enter a user name
		 * Leave the password panel up
		 */
		code = WARNING;
		strcpy (errmsg, GetStr (TXT_baduserID));
	}
	else
		code = authenticate (server, username, passwd);

	/*
	 * REMOVE THE WATCH 
	 */
	XUndefineCursor(XtDisplay(shell), XtWindow(shell));
   	XSync (XtDisplay (form), 0);

	if ( code == OK )
	{
		fromPanel = True;
		if ( sendDeskTopOpenReq == True )
		{
			/*
			 * Send a desktop open, handShakeTheRequest will
			 * cause an exit via a reply received event handler
			 * or a timeout event handler.
			 */
			sendOpenFolderReq(shell);
		}
		else
		{
			exit(0);
		}
	}
	else
		GuiError( widget, errmsg ,code);
	

} /* End of ApplyCB () */


static int
authenticate(char *serverName, char *userName, char *password)
{
	int	rc;
	NWCONN_HANDLE	connID;
	int ret;
	char *cp;

	/*	
	 * Attach to the server
	 */
	rc = NWAttach(serverName, &connID, 0);
    if( rc && (rc != NWERR_ALREADY_ATTACHED))
	{
		cp = AttachError(rc);
		strcpy (errmsg,cp);
       	return (FATAL_ERROR);
	}
	/*
	 * If this connection is already authenticated then
	 * leave
	 */
	if( isAuthenticated(connID) == True)
	{
		return (OK);
	}

	rc = NWLogin( (NWCONN_HANDLE)connID, userName, password, NULL);
	if (!rc) 
	{
		/*
		 * Return success
		 */
		return (OK);
	}
	else 
	{
		switch( rc )
		{
			/*
			 * OK
			 */
			case CONNECTION_LOGGED_IN:
				ret = OK;
				break;
			case ERR_CONN_ALREADY_LOGGED_IN :
				ret = OK;
				break;
			/*
			 * These are warnings
			 */
			case NO_SUCH_OBJECT:
				ret = WARNING;
				strcpy (errmsg, GetStr (TXT_baduserID));
				break;
			case NO_SUCH_OBJECT_OR_BAD_PASSWORD:
				ret = WARNING;
				strcpy (errmsg, GetStr (TXT_badpwd));
				break;
			/*
			 * Below here are fatal errors, get error message from 
			 * LoginError message routine, in libnct.so.
			 */
			default:
				ret = FATAL_ERROR;
				cp = LoginError(rc);
				strcpy (errmsg,cp);
				break;
		}
	}
	return (ret);
}
int
isUserAuthenticated(char *serverName, uid_t uid)
{
	NWCONN_HANDLE	connID;
	int	rc;
	int ret = False;
	uid_t oldeUid;
	char *cp;


	oldeUid = geteuid();
	if (seteuid(uid) < 0 )
	{
		return (ret);
	}

	/*	
	 * Attach to the server
	 */
	rc = NWAttach(serverName, &connID, 0);
    if( rc && (rc != NWERR_ALREADY_ATTACHED))
	{
		cp = AttachError(rc);
		strcpy (errmsg,cp);
       	ret =  False;
	}
	/*
	 * If this connection is already authenticated then
	 * leave
	 */
	else if( isAuthenticated(connID) == True)
	{
		ret = True;
	}
	seteuid(oldeUid);
	return (ret);
}

void
sendOpenFolderReq(Widget shell)
{
	DtRequest request;
	extern int sendDeskTopOpenReq;
	extern char *folderSpec;
	Window win;

	if ( sendDeskTopOpenReq == True && folderSpec != NULL)
	{
		DtInitialize(shell);
		WaitForExclusiveUse (shell);
		win = XtWindow(shell);
    	memset (&request, 0, sizeof (request));
        request.open_folder.rqtype= DT_OPEN_FOLDER;
        request.open_folder.path = folderSpec;
        request.open_folder.title = folderSpec;
        request.open_folder.options = DT_ICONIC_VIEW;
        (void) DtEnqueueRequest (XtScreen (shell),
                    _DT_QUEUE(XtDisplay(shell)),
                    folderAtom,
                    win, &request);
		handShakeTheRequest(shell);
	}
}

void
sendSyncFolderReq(Widget shell)
{
	DtRequest request;
	extern int sendDeskTopSyncReq;
	extern char *folderSpec;
	Window win;

	if ( sendDeskTopSyncReq == True && folderSpec != NULL)
	{
		DtInitialize(shell);
		WaitForExclusiveUse (shell);
		win = XtWindow(shell);
		memset (&request, 0, sizeof (request));
		request.sync_folder.rqtype= DT_SYNC_FOLDER;
		request.sync_folder.path = folderSpec;
		(void) DtEnqueueRequest (XtScreen (shell),
            _DT_QUEUE(XtDisplay(shell)),
            folderAtom,
			win, &request);
		handShakeTheRequest(shell);
	}
}

void
sendCloseFolderReq(Widget shell)
{
	DtRequest request;
	extern int sendDeskTopSyncReq;
	extern char *folderSpec;
	Window win;

	if ( sendDeskTopSyncReq == True && folderSpec != NULL)
	{
		DtInitialize(shell);
		WaitForExclusiveUse (shell);
		win = XtWindow(shell);
		memset (&request, 0, sizeof (request));
		request.sync_folder.rqtype= DT_CLOSE_FOLDER;
		request.sync_folder.path = folderSpec;
		(void) DtEnqueueRequest (XtScreen (shell),
            _DT_QUEUE(XtDisplay(shell)),
            folderAtom,
			win, &request);
		handShakeTheRequest(shell);
	}
}

/*
 *
 * Send a request for a property and get the reply.  This should ensure that any
 * previous desktop folder requests has been handled by dtm.
 *
 */
handShakeTheRequest(Widget widget)
{
    DtRequest   request;
	DtReply     reply;
	int			ret = 0;
    long        serial;
	Window win;
	Widget shell;
	XEvent event;
	XtAppContext	AppContext;
	static XtIntervalId	timer;
	Arg args[2];

    for (shell = widget; !XtIsShell(shell); shell = XtParent(shell));
	win = XtWindow(shell);

	handShakeAtom = XInternAtom(XtDisplay(shell),"XAUTO",False);
    memset(&request, 0, sizeof(request));
    request.header.rqtype= DT_GET_DESKTOP_PROPERTY;
    request.get_property.name = "FILEDB_PATH";
    (void) DtEnqueueRequest (XtScreen (shell),
                 _DT_QUEUE(XtDisplay(shell)),
                 handShakeAtom,
                 win, &request);

  	XtAddEventHandler (shell,StructureNotifyMask | SelectionNotify ,
   					True,(XtEventHandler) getReply, (XtPointer)handShakeAtom);
	/*
	 * Bail out if we don't get a selection event after 10 secs.
	 */
	AppContext = XtWidgetToApplicationContext(shell); 
	XtAppAddTimeOut (AppContext, 10000, TimerProc, shell);	
	if ( fromPanel == True )
		return;
	XtAppMainLoop(AppContext);
	exit(1);
}

/*
 * Desktop reply timeout routine. Deletes the exclusive use atom.
 */
static void
TimerProc (XtPointer client_data, XtIntervalId * timeID)
{
	Widget widget = (Widget)client_data;
	Display *dp = XtDisplay(widget);

	/*
	 * Delete the property so that other xauto's can use it.
	 */
	XDeleteProperty(dp, DefaultRootWindow(dp),exclusiveAtom);
	XFlush(dp);
	exit (1);
}

/*
 * Callback routine to get the desktop's reply to our DT_GET_DESKTOP_PROPERTY
 * request
 */
static void
getReply (Widget widget,XtPointer clientData, XEvent *event, Boolean cont)
{
	Window win;
	DtReply     reply;
	Widget shell;
	Atom handShakeAtom = (Atom)clientData;

    for (shell = widget; !XtIsShell(shell); shell = XtParent(shell));
	win = XtWindow(shell);

   	if (event->type == SelectionNotify) 
	{
   		if (event->xselection.selection == handShakeAtom ) 
		{
       		memset(&reply, 0, sizeof(reply));
       		DtAcceptReply( XtScreen(widget), handShakeAtom, 
			               win, &reply);
			/*
			 * Delete the property so that other xauto's can use
			 * it.
			 */
			XDeleteProperty(XtDisplay(widget), 
			                DefaultRootWindow(XtDisplay(widget)),
			                exclusiveAtom);
			XFlush(XtDisplay(widget));
			exit(0);
   		}
	}
}

/*
 *  Function for waiting for exclusive use of the desktop folder atoms.
 *
 *  Why:  There is a possibility that there could be simultaneous
 *        instances of xauto running trying to send desktop folder
 *        request.  For instance: user double clicks on a NetWare
 *        server icon, already authenticated, and a background job
 *        attempts to a directory operation that cause a NUC 
 *        auto-mount. Both process will want to send a folder open
 *        request to dtm using the folderAtom. The contents of the 
 *        the folderAtom could be changed by process 2 before process 
 *        1 gets completely processed by dtm if we don't enforce 
 *        an exclusive use policy.
 */
static void  
WaitForExclusiveUse ( Widget widget)
{
	Window		*returned_win = NULL;	
	Atom		actual_type;
	int			actual_format;
	unsigned	long    nitems; 
	unsigned	long    bytes_after;
	int 		counter = 0;
	Widget 		shell;
	Window 		win;
	Display 	*dp;

    for (shell = widget; !XtIsShell(shell); shell = XtParent(shell));
	/*
	 * Have the realize the shell so we get a window number but
	 * I don't want to see it on the display.
	 */
	if ( XtIsRealized(shell))
		XtUnmapWidget(shell);
	else
	{
		XtSetMappedWhenManaged(shell,False);
		XtRealizeWidget(shell);
	}
	win = XtWindow(shell);
	dp = XtDisplay(shell);
	exclusiveAtom = XInternAtom(dp,"XAUTO_EXCLU_ATOM",False);
	folderAtom = XInternAtom(dp,"XAUTO_FOLDER_ATOM",False);

	/*
	 * Wait until the current user of XAUTO_EXCLU_ATOM deletes
	 * it, signifying no longer in use.  Max wait time is 10 seconds.
	 */
	while(True)
	{
		XGrabServer (dp);
		XGetWindowProperty (dp, DefaultRootWindow(dp), exclusiveAtom, 0L, 
						2L, False,
						AnyPropertyType, &actual_type, &actual_format, 
						&nitems, &bytes_after,(unsigned char **)&returned_win);

		if (actual_type == XA_WINDOW && returned_win != NULL)
		{
			XUngrabServer (dp);
			XFlush(dp);
			sleep(2);
			counter++;
			if ( counter > 10 )
			{
				break;
			}
			continue;
		}
		else
			break;
	}
	/*
	 * No one running so set property to the current window ID
	 */
	XChangeProperty (dp, DefaultRootWindow(dp), exclusiveAtom, 
	                     XA_WINDOW, 32, PropModeReplace, 
	                     (unsigned char *)&win, 1);
	XUngrabServer (dp);
	XFlush(dp);
	if ( returned_win != NULL )
		XFree ((char*) returned_win);
}
