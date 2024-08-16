/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:xmtravel/dialog.h	1.1"
/*
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * Motif Release 1.2
 */


/* values which can be or'ed together */
#define RET_NONE        0
#define RET_OK          1
#define RET_CANCEL      2
#define RET_HELP        4
#define RET_DONE	8
#define RET_SAVE	16
#define RET_DISCARD	32
#define RET_APPLY       64

/* amount of time for working dialogs to remain on screen */
#define WORKING_TIME	5000

extern int 	Question();
extern int 	Warning();
extern int      Information() ;
extern int 	Error();
extern int      Selection();
extern int 	Working();
extern int 	SaveWarning();
extern int      PromptDialog();
extern int      FileSelectionDialog();


