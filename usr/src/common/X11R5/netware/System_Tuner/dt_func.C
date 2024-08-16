/* dt_func.c */

/*****************************************************************

 Copyright (c) 1993 Univel
 All Rights Reserved

 THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF UNIVEL

 The copyright notice above does not evidence any
 actual or intended publication of such source code.

 *****************************************************************/

#include "caw.h"

void get_fonts (Widget w, XmFontList *tfontList, XmFontList *fontList)
{
    XrmValue value;
    char *str_type[20];
    char    env[256];
    char *tst;
    XrmDatabase db = NULL;
	XFontStruct *font = NULL;

    tst = getenv("HOME");
    strcpy(env,tst);
    strcat(env,"/.Xdefaults");
    db = XrmGetFileDatabase(env);

	/*
	 * Get the desktop window font
	 */
	if ( db != NULL )
    	XrmGetResource(db,"*font","*font", str_type,&value);

    if ( db != NULL &&  value.addr != NULL )
	{
		font = XLoadQueryFont(XtDisplay(w),value.addr);
	}
	if ( font == NULL )
	{
		font = XLoadQueryFont(XtDisplay(w),"fixed");
	}
	if ( font == NULL )
	{
		fprintf(stderr,"%s", get_str ( TXT_fontLoadErr ));
		exit(1);
	}
	if ((*fontList = XmFontListCreate(font,XmSTRING_DEFAULT_CHARSET)) == NULL)
	{
		fprintf(stderr,"%s", get_str ( TXT_fontListErr ));
		exit(1);
	}

	/*
	 * Get the default xterm font 
	 */
	if ( db != NULL )
    	XrmGetResource(db,"*xterm*font","*XTerm*font", str_type,&value);

    if ( db != NULL && value.addr != NULL )
	{
		font = XLoadQueryFont(XtDisplay(w),value.addr);
	}
	if ( font == NULL )
	{
		font = XLoadQueryFont(XtDisplay(w),"fixed");
	}
	if ( font == NULL )
	{
		fprintf(stderr,"%s", get_str ( TXT_fontLoadErr ));
		exit(1);
	}
	if ((*tfontList = XmFontListCreate(font,XmSTRING_DEFAULT_CHARSET)) == NULL)
	{
		fprintf(stderr,"%s", get_str ( TXT_fontListErr ));
		exit(1);
	}
	if ( db != NULL )
    	XrmDestroyDatabase(db);
}

char *get_str (char *idstr)
{
	char *sep;
	char *str;

	sep = (char *)strchr (idstr, FS_CHR);
	*sep = '\0';
	str = (char *)gettxt (idstr, sep + 1);
	*sep = FS_CHR;
	return (str);
}

Boolean _DtamIsOwner (char *adm_name)
{
	char    buf[256];

	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", adm_name);
	return (system(buf)==0);
}

void make_icon ()
{
	Pixmap icon, iconmask;
	int width, height;

	if (XReadPixmapFile(XtDisplay (toplevel), RootWindowOfScreen(XtScreen (toplevel)), DefaultColormapOfScreen(XtScreen (toplevel)), XPM_PATH, &width, &height, DefaultDepthOfScreen(XtScreen (toplevel)), &icon, 0) == BitmapSuccess)
		XtVaSetValues (toplevel, XtNiconPixmap, (XtArgVal) icon, XtNiconName, (XtArgVal) "System_Tuner", NULL);
}

void display_help (Widget w, HelpText *help)
{
	DtRequest                   *req;
	DtDisplayHelpRequest        displayHelpReq;
	Display                     *display = XtDisplay (w);
	Window                      win = XtWindow (w);

	DtInitialize (w);
	req = (DtRequest *) &displayHelpReq;
	displayHelpReq.rqtype = DT_DISPLAY_HELP;
	displayHelpReq.serial = 0;
	displayHelpReq.version = 1;
	displayHelpReq.client = win;
	displayHelpReq.nodename = NULL;
	if (help) {
		displayHelpReq.source_type = help->section ? DT_SECTION_HELP : DT_TOC_HELP;
		displayHelpReq.app_name = help->appname;
		displayHelpReq.app_title = help->appname;
		displayHelpReq.title = help->appname;
		displayHelpReq.help_dir = NULL;
		displayHelpReq.file_name = help->file;
		displayHelpReq.sect_tag = help->section;
	} else
		displayHelpReq.source_type = DT_OPEN_HELPDESK;
	(void)DtEnqueueRequest(XtScreen (w), _HELP_QUEUE (display), _HELP_QUEUE (display), win, req);
}

void watch_cursor (Widget w)
{
	Cursor c1;

	c1 = XCreateFontCursor (XtDisplay (w), XC_watch);
	XDefineCursor (XtDisplay (w), XtWindow (w), c1);
	XFlush (XtDisplay (w));
}

void normal_cursor (Widget w)
{
	XUndefineCursor (XtDisplay (w), XtWindow (w));
	XFlush (XtDisplay (w));
}
