/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtsched:prop.h	1.2"
#endif

/*
 * prop.h
 *
 */

#ifndef _prop_h
#define _prop_h

extern void   CreatePropertyWindow(Widget w, int i);
extern void   CreateInputPropertyWindow(Widget w, char * task);
extern char * BasenameOf(char * command);
extern char * DayOrDate(char * month, char * date, char * day);
extern char * TimeOf(char * hour, char * min);

#endif
