/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/client.h	1.1"
#endif

	/* File : client.h 	
	 * To be included by the user application and the
	 * lookup table library
    	 */
typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;


typedef struct {
  Widget   text;        /* Text widget handle where the system name is to be
                           set */
  char     *prevVal; /* previous text string in the text */
  Boolean  hostSelected;
  HelpText *help;       /* help info to be passed */
} UserData;


