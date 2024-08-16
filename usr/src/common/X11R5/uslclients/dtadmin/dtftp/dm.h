/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma #ident	"@(#)dtftp:dm.h	1.1"
#endif

#ifndef _dm_h
#define _dm_h

/* View Format menu items */
typedef enum {
	DM_ICONIC,
	DM_NAME,
	DM_LONG
} DmViewFormatType;

/* options for layout routines  from Dtm.h */
#define UPDATE_LABEL		(1 << 1)   /* 0 for NONE, undefined for now */
#define RESTORE_ICON_POS	(1 << 2)
#define SAVE_ICON_POS		(1 << 3)
#define NO_ICONLABEL		(1 << 4)

#endif /* _dm_h */
