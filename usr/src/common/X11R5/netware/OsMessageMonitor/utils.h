/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osmsgmon:utils.h	1.2"
#ident  "@(#)utils.h    6.1 "
#ident "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/OsMessageMonitor/utils.h,v 1.4 1994/03/02 15:36:34 plc Exp $"

/*  Copyright (c) 1993 Univel                           */
/*    All Rights Reserved                               */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF      */
/*  Univel.                                             */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

/*
 * Client Data for the help callback
 */
#define APP_HELP      1
#define TABLE_HELP    2
#define HDESK_HELP    3
#define HELP_MASK     0xf

/*
 * utils function prototypes
 */
extern void mwmOverride(Widget w);
extern void helpCB( Widget w, caddr_t client_data, caddr_t call_data);
extern int  ChangePixmapColors(Widget w,char *colors,Pixmap pixmap);
extern Widget SetIconWindow(Widget w,Pixmap pixmap, unsigned int *,unsigned *,unsigned int *);

extern KeySym   GetMnemonic(char *idstr);

