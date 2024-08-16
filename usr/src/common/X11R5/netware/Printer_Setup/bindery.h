/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:bindery.h	1.2"
//--------------------------------------------------------------
// Filename: bindery.h
//
// Description: This file contains function prototypes for the
//				bindery.c file.
//--------------------------------------------------------------

//--------------------------------------------------------------
//			F U N C T I O N   P R O T O T Y P E S
//--------------------------------------------------------------
char **GetNetWareFileServers();
XmString *GetNetWarePrintQueues( char *, short * );
XmString *GetNetWarePrintServers( char *, short * );
Boolean IsAuthenticated( char *name );
NWCCODE IsSelectable( char *name );
void FreeArrayOfXmStrings( XmString * );

#ifdef OWNER_OF_BINDERY
const char *xauto_path 	= { "/usr/X/bin/xauto " };
const char *space 		= { " " };
#else
extern char *xauto_path;
extern char *space;
#endif

