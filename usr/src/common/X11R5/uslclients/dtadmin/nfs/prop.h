/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/prop.h	1.1"
#endif

	/* File: prop.h 
	 * 	To be included by the graphics of the lookuptable library
	 * 	i.e prop.c
	 */
#define VISIBLE_ITEMS 6
typedef struct _HostTable{
	Widget dialog;		/* dialog for the lookup table window */
	Widget etcHostsRC;	/* rc to contain etc label and list   */
	Widget domainHostsRC;   /* rc to contain the domain text, listsys button
						and domain list */
	Widget controlRC;   	/* rc to contain the selected host, action area
						and footer */
	Widget selectedHost;	/* label to display the selected host on every
						selection */
	Widget etcList;		/* list widget to contain etc hosts list */
	Widget domainList;  	/* list widget to contain domain hosts list */
	Widget domainText;	/* text widget to type the domain name by
						the user */
	Widget defaultBtn;	/* default button from the action area */
	Widget footer;		/* label for the footer help messages */
	Widget errDialog;       /* handle to the error dialog */
	char *domainName;	/* string to hold the current domain name */
}HostTable;

typedef struct _ActionAreaItems{
    char *label;
    void (*callback)();
    caddr_t data;
    Widget	widget;
} ActionAreaItems;

