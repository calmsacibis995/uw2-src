/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libdlist:dl_common.h	1.2"
#ident	"@(#)dl_common.h	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/libdlist/dl_common.h,v 1.1 1994/02/01 22:52:06 renu Exp $"

/*--------------------------------------------------------------------
** Filename : dl_common.h
**
** Description : This file contains definitions  and typedefs common
**               to all 3 applications that make up the Remote
**               Application functionality.
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                            D E F I N E S 
**------------------------------------------------------------------*/
#define  SUCCESS             0x00
#define  NO_APPS_FOUND       0x20
#define  FAILURE             0xFF

#define  ADD_XHOST           0x10
#define  CLEANUP_XHOST       0x20

#define  MAX_XHOST_LINE      270

#define  UC                  ( unsigned char * )
#define  SC                  ( char * )


/*--------------------------------------------------------------------
**                            T Y P E D E F S 
**------------------------------------------------------------------*/
typedef struct name_list
{ 
    unsigned char     *name;
    struct name_list  *next;
} nameList;

typedef struct _item {
    XtPointer label;
    XtPointer mnemonic;
    XtPointer select;
    XtPointer sensitive;
} item;
