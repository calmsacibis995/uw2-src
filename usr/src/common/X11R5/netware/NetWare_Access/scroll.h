/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:scroll.h	1.2"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/scroll.h,v 1.3 1994/03/18 21:22:51 plc Exp $"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#include <nw/nwerror.h>

#define STDIN		0
#define USERTYPE	1
#define MAXSERVER	1024	
#define NWMAX_SERVER_LENGTH 50
#define MAX_OBJECT_NAME_LENGTH	0x30 
#define SERVER_ALLOC_SIZE  30
#define SHORTVIEW			1
#define	LONGVIEW	 		0
#define	SPACES	 	 " "	
#ifndef uint8 
#define uint8 		unsigned char
#endif
#define	NWsp	 	uint8* 


typedef struct SlistStruct {
	char	ObjectName[MAX_OBJECT_NAME_LENGTH];
	uint8	Network[4], NodeAddress[6];
} SLIST_STRUCT;

typedef struct {
    XtPointer	FileServers;
    XtPointer	NwAddress;
    XtPointer	NodeAddress;
    XtPointer	userName;
} HeaderFormatData;

typedef struct {
    XtPointer	FileServers;
    XtPointer	NwAddress[4];
    XtPointer	NodeAddress[6];
    XtPointer	userName;
} FormatData;

typedef struct {
    XtArgVal	formatData;
} ListItem;

typedef struct {
    XtPointer	FileServers;
    XtPointer	userName;
} ShortFormatData;

typedef struct {
   Widget    serverWidget;
   Widget    headerWidget;
   FormatData *list;
   ShortFormatData *shortlist;
   unsigned   cnt;
   unsigned   pgone_count;
   unsigned   pgtwo_count;
   unsigned   allocated;
   int        item_index_count[MAXSERVER];
   int        selected_item_index[MAXSERVER];
   int        selected_value_index[MAXSERVER];
   Boolean    set_toggle;
   Boolean   viewformat_flag;
} ServerList;

typedef struct {
    XtPointer	File_Servers;
    XtPointer	Nw_Address;
    XtPointer	Node_Address;
    XtPointer	user_Name;
} HdrData;

typedef struct {
    XtArgVal	HdrData;
} ColItems;


