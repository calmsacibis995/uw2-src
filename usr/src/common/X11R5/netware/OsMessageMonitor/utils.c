/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osmsgmon:utils.c	1.5"
#ident  "@(#)utisl.c.   6.1 "
#ident "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/OsMessageMonitor/utils.c,v 1.10 1994/07/08 15:46:07 plc Exp $"

/*  Copyright (c) 1993 Univel                           */
/*    All Rights Reserved                               */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF      */
/*  Univel.                                             */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>		
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <Xm/DrawingA.h>
#include <X11/Xatom.h>

#include <X11/Xos.h>

#include <Dt/Desktop.h>
#include <Dt/DtMsg.h>

#include <locale.h>
#include "dtFuncs.h"
#include "utils.h"
#include "msg.h"


/*
 * Quit function on osMessageMonitor.c
 */
extern void Quit( Widget, XEvent *, String *params, Cardinal *num_params);

/*
 * Routine: mwmOverride
 *
 * Purpose: To spawn a help window when user presses F1 while running
 *          a Motif app with the OLWM window manager.
 *
 * Description: OLWM receives the F1 key event and then sends a client
 *              message to the toplevel widget, message type is 
 *              XA_OL_HELP_KEY(dpy).  Therefore we need to setup 
 *              a ClientMessage event handler. The event handler ckecks
 *              the mesage type and calls the helpCB if the event message 
 *              type is XA_OL_HELP_KEY(dpy).
 *
 *              Has no effect when running mwm or twm, seems safe to  
 *              always call.
 *
 */
#include <Xol/OlClients.h>

static void ClientMessageCB( Widget , XtPointer , XClientMessageEvent *);

void mwmOverride(Widget w)
{
    XtAddEventHandler (w,
				ClientMessage ,
				True,(XtEventHandler) ClientMessageCB,(XtPointer )NULL);
}
static void 
ClientMessageCB( Widget w,
                      XtPointer clientData ,
                      XClientMessageEvent *xcev )
{
	if ( xcev->message_type == XA_OL_HELP_KEY(XtDisplay(w)))
	{
		helpCB( w, (XtPointer)APP_HELP,NULL);
	}
}

/*
 * Routine: helpCB
 *
 * Purpose: To display the applications help/desktop information.
 *
 * Description: Depending on the passed client_data display the help
 *              information.  Currently understands:
 *                     APP_HELP   = display apps specific help panel
 *                     TABLE_HELP = display apps table of contents
 *                     HDESK_HELP = display help desk panel
 *                 
 * In your msg.h file define the following:
 *
 *    #define APP_NAME           "osMessageMonitor"
 *    #define HELP_FILE_NAME     "osMessageMonitor/osMessageMonitor.hlp"
 *    #define ICON_NAME          "osMessageMonitor.icon"
 *    #define TXT_title          "osMessageMonitor:1" FS "osMessageMonitor"
 *    #define HELP_SECT          "10"                         
 *
 *  Note: Section specific information is not handled here.  Hopefully
 *        this code will be replaced by COSE soon.
 *        I quess COSE is dead now, but still no Section code.
 *
 */

void
helpCB( Widget w, caddr_t client_data, caddr_t call_data)
{
    DtRequest     request;
    long          serial;
    Display*  dp = XtDisplay(w);

    /*
     * Using relative path name so that XtResolvePathname will be
     * used to locate help file in the standard paths.  ( Hopefully
     * this means in the paths based on locale )
     */
    memset(&request,0,sizeof(request));
    request.header.rqtype = DT_DISPLAY_HELP;
    request.display_help.app_name = APP_NAME;
    request.display_help.icon_file = ICON_NAME;
    request.display_help.title = getStr(TXT_title);
    request.display_help.app_title = getStr(TXT_title);
    request.display_help.file_name = HELP_FILE_NAME;

	switch ( (int) client_data & 0xf)
	{
		case APP_HELP:
    		request.display_help.source_type = DT_SECTION_HELP;
    		request.display_help.sect_tag = HELP_SECT;
			break;
		case TABLE_HELP:
    		request.display_help.source_type = DT_TOC_HELP;
			break;
		case HDESK_HELP:
    		request.display_help.source_type = DT_OPEN_HELPDESK;
			break;
		default:
			break;
	}

    serial = DtEnqueueRequest( XtScreen( w ),
                               _HELP_QUEUE( dp ),
                               _HELP_QUEUE( dp ),
                               XtWindow( w ),
                               &request );
}

/* GetMnemonic
 *
 * Get an internationalized Mnemonic. 
 */
KeySym 
GetMnemonic(char *idstr)
{
	char *cp;
	cp = getStr(idstr);
	return(*cp);
}

/*
 * Routine: ChangePixmapColors
 *
 * Purpose: To change the colors in a pixmap
 *
 * Description: 
 *              All pixels values that match old_color are set to
 *              new_color. All errors are silently ignored, pixmap 
 *              is always left in a valid state.
 *
 * Input: colors  is a comma separated list of color tuples
 *                 old_color,new_color. "red,green,#FFFFFFFFFFFF,black"
 *        pixmap  is the image to change colors in
 *
 */

int
ChangePixmapColors(Widget w,char *colors,Pixmap pixmap)
{
	XImage *image;
	register int i;
	register int x;
	register int y;
    unsigned int ph,pw,bw,depth;
    int px,py;
    int screen;
    Display *dpy;
    Window root;
    Colormap colormap ;
	XColor xcolor ;
	GC gc;
	int numColors = 0;
	char *colorStr;
	unsigned long color;
	unsigned long *new_colors;
	unsigned long *old_colors;

    dpy = XtDisplay(w);
    screen = DefaultScreen(dpy);
	colormap = XDefaultColormap(dpy,screen);
	
	if ( colors == NULL )
		return (-1);
	if ( XGetGeometry(dpy,pixmap,&root,&px,&py,&pw,&ph,&bw,&depth) == 0 )
		return(-1);
    root = RootWindow(dpy, screen);
	/*
	 * Get the number of colors, colors are comma separated
	 * Remove any trailing commas just in case user put one
	 * there.
	 */
	if ( colors[strlen(colors) -1 ] == ',' )
		colors[strlen(colors) -1] = NULL;
	numColors = 0;
    for(colorStr = strtok(colors, ","); colorStr != NULL; colorStr = strtok(NULL, ",")) 
	{
		numColors++;
		colorStr[strlen(colorStr)] = ',';
    }
	/*
	 * Has to be an even amount 
	 */
	if ( numColors % 2 )
		numColors = (numColors/2 * 2);
	if ( numColors <= 0 )
		return(-1);

	if ((old_colors = (unsigned long *)
			XtMalloc(sizeof(unsigned long) * numColors)) == NULL )
		return(-1);
	if ((new_colors = (unsigned long *)
			XtMalloc(sizeof(unsigned long) * numColors)) == NULL )
	{
		XFree((char *)old_colors);
		return(-1);
	}
	/*
	 * Get the color values for the old_colors/new_colors
	 */
    colorStr = strtok(colors, ","); 
	for(x = i = 0; i < numColors; i++ , colorStr = strtok(NULL, ",")) 
	{ 
    	if (!XParseColor(dpy,colormap,colorStr,&xcolor))
		{
			/*
			 * Could not parse the color, probably miss typed
			 * go to the next pair
			 */
			if (! (i & 1 ))
			{
				i++;
				colorStr = strtok(NULL, ","); 
			}
			continue;
		}
    	if (!XAllocColor(dpy,colormap,&xcolor))
		{
			/*
			 * Could not alloc the color, no more left?
			 */
			if (! (i & 1 ))
			{
				i++;
				colorStr = strtok(NULL, ","); 
			}
			continue;
		}
		if ( i & 1 )
		{
			new_colors[x] =  xcolor.pixel;
			x++;
		}
		else
			old_colors[x] =  xcolor.pixel;
	}
	numColors = x;
	/*
	 * Go through the image data converting the old_colors
	 * to the new ones if there are any color changes to make
	 *  ( "old pixels for new" he cried )
	 */
	if ( numColors )
	{
		image = XGetImage(DisplayOfScreen(XtScreen(w)), pixmap, 0, 0,
					pw, ph, AllPlanes, ZPixmap);
		for(y=0;y<ph;y++)
		{
			for(x=0;x<pw;x++)
			{
				color = XGetPixel(image, x, y);
				for(i=0;i< numColors;i++)
				{
					if ( old_colors[i] == color )
					{
						XPutPixel(image,x,y,new_colors[i]);
					}
				}
			}
		}
		gc = XCreateGC(dpy,RootWindow(dpy,0),0,NULL);
		XPutImage(DisplayOfScreen(XtScreen(w)),pixmap,gc,image, 0, 0,
					0,0,pw, ph);
		XFreeGC(dpy,gc);
	}
	XtFree((char *)new_colors);
	XtFree((char *)old_colors);
}
/*
 * Routine: SetIconWindow
 *
 * Purpose: To create a window to be used for the application icon
 *
 * Description: 
 *              Create a simple window and set the pixmap into the
 *              windows background. 
 *
 * Input: w  widget to use for display macro
 *        pixmap  is the image to change colors in
 *
 */

Widget
SetIconWindow(Widget w,Pixmap pixmap,unsigned int *pw,unsigned int *ph,unsigned int *depth)
{
	register int i;
	register int x;
	register int y;
    unsigned int bw;
    int px,py;
    int screen;
    Display *dpy;
    Window root;
    Window window;
	Widget icon_shell;
	Widget drawArea;
	Arg arg[10];
	int argc;
	Atom wm_delete_window;

    dpy = XtDisplay(w);
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
	
	if ( XGetGeometry(dpy,pixmap,&root,&px,&py,pw,ph,&bw,depth) == 0 )
		return((Widget)-1);

	argc = 0;
	XtSetArg(arg[argc], XtNborderWidth,0); argc++;
	XtSetArg(arg[argc], XtNmappedWhenManaged,False); argc++;
	XtSetArg(arg[argc], XtNtranslations,XtParseTranslationTable("")); argc++;
	XtSetArg(arg[argc], XmNmaxHeight,*ph); argc++;
	XtSetArg(arg[argc], XmNmaxWidth,*pw); argc++;
	XtSetArg(arg[argc], XmNminHeight,*ph); argc++;
	XtSetArg(arg[argc], XmNminWidth,*pw); argc++;
	icon_shell =
		XtCreateApplicationShell("_X_", vendorShellWidgetClass, arg, argc);
	argc = 0;
	XtSetArg(arg[argc], XtNbackgroundPixmap,  pixmap); argc++;
	XtSetArg(arg[argc], XtNwidth,  *pw); argc++;
	XtSetArg(arg[argc], XtNheight, *ph); argc++;
	XtSetValues(icon_shell, arg, argc); argc++;

	drawArea = XtVaCreateManagedWidget( "DrawArea",
                        xmDrawingAreaWidgetClass, icon_shell,
                        XmNwidth, *pw,
                        XmNheight,*ph,
                        NULL );

	/*
	 * Call the Quit routine if we are sent a delete
	 * message while iconified.
	 */
    wm_delete_window = XmInternAtom(XtDisplay(icon_shell), 
	                                  "WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback (icon_shell, wm_delete_window,
	                              (XtCallbackProc)Quit,NULL);
	XtRealizeWidget(icon_shell);

	XtVaSetValues(w,XmNiconWindow,XtWindow(icon_shell),NULL);
	XSetWindowBackgroundPixmap(dpy,XtWindow(icon_shell),pixmap);
	return(drawArea);
}
