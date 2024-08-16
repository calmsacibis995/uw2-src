/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)launchapp:la_login.h	1.2"
#ident	"@(#)la_login.h	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Launch_Application/la_login.h,v 1.1 1994/02/01 22:53:08 renu Exp $"

/*--------------------------------------------------------------------
** Filename : la_login.h
**
** Description : This is the header file for la_launch.c
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                       D E F I N E S 
**------------------------------------------------------------------*/
#define    MAX_LOGIN_CHARS          20
#define    MAX_PASSWD_CHARS         20

/*--------------------------------------------------------------------
**                       T Y P E D E F S 
**------------------------------------------------------------------*/
typedef struct _logitem {
    XtPointer    label;
    XtPointer    mnemonic;
    XtPointer    select;
    Boolean      deflt;
    XtPointer    clientData;
} logItem;


typedef struct buttondata {
    unsigned char        *server;
    Widget                userid;
    Widget                passwd;
    Widget                popup;
    Pixmap               *icon;
} buttonData;
    

static char      *logItemFields[] = { XtNlabel, 
                                      XtNmnemonic, 
                                      XtNselectProc,
                                      XtNdefault,
                                      XtNclientData };
static int        numLogItemFields = XtNumber( logItemFields );
