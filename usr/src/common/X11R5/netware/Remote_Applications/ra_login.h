/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_login.h	1.2"
#ident	"@(#)ra_login.h	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_login.h,v 1.1 1994/02/01 22:51:02 renu Exp $"

/*--------------------------------------------------------------------
** Filename : dl_login.h
**
** Description : This is the header file for dl_login.c and dl_launch.c
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        I N C L U D E S
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
    XtPointer    clientData;
} logItem;


typedef struct buttondata {
    char        *server;
    Widget       userid;
    Widget       passwd;
    Widget       popup;
    Pixmap      *icon;
} buttonData;
    

static char      *logItemFields[] = { XtNlabel, 
                                      XtNmnemonic, 
                                      XtNselectProc,
                                      XtNclientData };
static int        numLogItemFields = XtNumber( logItemFields );
